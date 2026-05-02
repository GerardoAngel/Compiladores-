#include "semantic_analyzer.h"

void SemanticAnalyzer::analyze(Node* root) {
    if (root) {
        visit(root);
    }
}

bool SemanticAnalyzer::has_errors() const {
    return !errors_.empty();
}

void SemanticAnalyzer::print_errors(std::ostream& out) const {
    for (const auto& error : errors_) {
        out << "semantic error: " << error.message
            << " at line " << error.line
            << ", column " << error.column << '\n';
    }
}

void SemanticAnalyzer::report_error(int line, int column, const std::string& message) {
    errors_.push_back({message, line, column});
}

bool SemanticAnalyzer::is_numeric(DataType type) const {
    return type == DataType::Int || type == DataType::Float;
}

bool SemanticAnalyzer::can_assign(DataType target, DataType source) const {
    if (target == DataType::Invalid || source == DataType::Invalid) {
        return false;
    }
    if (target == source) {
        return true;
    }
    return target == DataType::Float && source == DataType::Int;
}

DataType SemanticAnalyzer::combine_numeric_type(DataType left, DataType right) const {
    if (!is_numeric(left) || !is_numeric(right)) {
        return DataType::Invalid;
    }
    if (left == DataType::Float || right == DataType::Float) {
        return DataType::Float;
    }
    return DataType::Int;
}

void SemanticAnalyzer::inject_cast_if_needed(Node* parent, size_t child_index, DataType target_type) {
    Node* child = parent->child(child_index);
    if (!child) return;
    
    if (child->type() == DataType::Int && target_type == DataType::Float) {
        Node* cast_node = new Node("Cast_IntToFloat");
        cast_node->set_type(DataType::Float);
        cast_node->set_location(child->line(), child->column());
        Node* extracted = parent->release_child(child_index);
        cast_node->add_child(extracted);
        parent->replace_child(child_index, cast_node);
    }
}

void SemanticAnalyzer::visit(Node* node) {
    if (!node) return;
    const std::string& kind = node->kind();
    
    if (kind == "Program") {
        visit_program(node);
    } else if (kind == "StatementList") {
        visit_statement_list(node);
    } else if (kind == "Block") {
        visit_block(node);
    } else if (kind == "Declaration") {
        visit_declaration(node);
    } else if (kind == "Assignment") {
        visit_assignment(node);
    } else if (kind == "If") {
        visit_if(node);
    } else if (kind == "While") {
        visit_while(node);
    } else if (kind == "BinaryOp") {
        visit_binary_op(node);
    } else if (kind == "Identifier") {
        visit_identifier(node);
    } else if (kind == "IntLiteral" || kind == "FloatLiteral") {
        // leaf nodes with pre-assigned types, nothing to do
    } else {
        for (size_t i = 0; i < node->num_children(); ++i) {
            visit(node->child(i));
        }
    }
}

void SemanticAnalyzer::visit_program(Node* node) {
    symbols_.enter_scope();
    for (size_t i = 0; i < node->num_children(); ++i) {
        visit(node->child(i));
    }
    symbols_.leave_scope();
}

void SemanticAnalyzer::visit_statement_list(Node* node) {
    for (size_t i = 0; i < node->num_children(); ++i) {
        visit(node->child(i));
    }
}

void SemanticAnalyzer::visit_block(Node* node) {
    symbols_.enter_scope();
    for (size_t i = 0; i < node->num_children(); ++i) {
        visit(node->child(i));
    }
    symbols_.leave_scope();
}

void SemanticAnalyzer::visit_declaration(Node* node) {
    // Check if it has an assignment expression
    if (node->num_children() > 0) {
        visit(node->child(0)); // visit expr
        
        DataType expr_type = node->child(0)->type();
        if (!can_assign(node->type(), expr_type)) {
            report_error(node->line(), node->column(), "no se puede asignar un valor de tipo " +
                std::string(data_type_name(expr_type)) + " a una variable de tipo " +
                std::string(data_type_name(node->type())));
        } else {
            inject_cast_if_needed(node, 0, node->type());
        }
    }
    
    if (!symbols_.define(node->lexeme(), node->type())) {
        report_error(node->line(), node->column(), "redeclaracion de '" + node->lexeme() + "' en el mismo ambito");
    }
}

void SemanticAnalyzer::visit_assignment(Node* node) {
    // evaluate rhs
    visit(node->child(0));
    
    const Symbol* symbol = symbols_.lookup(node->lexeme());
    if (symbol == nullptr) {
        report_error(node->line(), node->column(), "uso de variable no declarada '" + node->lexeme() + "'");
        node->set_type(DataType::Invalid);
    } else {
        node->set_type(symbol->type);
        DataType expr_type = node->child(0)->type();
        if (!can_assign(symbol->type, expr_type)) {
            report_error(node->line(), node->column(), "no se puede asignar un valor de tipo " +
                std::string(data_type_name(expr_type)) + " a una variable de tipo " +
                std::string(data_type_name(symbol->type)));
        } else {
            inject_cast_if_needed(node, 0, symbol->type);
        }
    }
}

void SemanticAnalyzer::visit_if(Node* node) {
    visit(node->child(0)); // cond
    visit(node->child(1)); // stmt
    
    if (!is_numeric(node->child(0)->type())) {
        report_error(node->line(), node->column(), "la condicion de if debe ser numerica");
    }
}

void SemanticAnalyzer::visit_while(Node* node) {
    visit(node->child(0)); // cond
    visit(node->child(1)); // stmt
    
    if (!is_numeric(node->child(0)->type())) {
        report_error(node->line(), node->column(), "la condicion de while debe ser numerica");
    }
}

void SemanticAnalyzer::visit_binary_op(Node* node) {
    visit(node->child(0));
    visit(node->child(1));
    
    DataType left = node->child(0)->type();
    DataType right = node->child(1)->type();
    
    DataType result = combine_numeric_type(left, right);
    if (result == DataType::Invalid) {
        report_error(node->line(), node->column(), "el operador requiere operandos numericos");
    } else {
        inject_cast_if_needed(node, 0, result);
        inject_cast_if_needed(node, 1, result);
    }
    node->set_type(result);
}

void SemanticAnalyzer::visit_identifier(Node* node) {
    const Symbol* symbol = symbols_.lookup(node->lexeme());
    if (symbol == nullptr) {
        report_error(node->line(), node->column(), "uso de variable no declarada '" + node->lexeme() + "'");
        node->set_type(DataType::Invalid);
    } else {
        node->set_type(symbol->type);
    }
}
