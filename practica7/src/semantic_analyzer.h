#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "ast.h"
#include <vector>
#include <string>
#include <iostream>

class SemanticAnalyzer {
public:
    void analyze(Node* root);
    bool has_errors() const;
    void print_errors(std::ostream& out) const;

private:
    SymbolTable symbols_;
    std::vector<SemanticError> errors_;

    void report_error(int line, int column, const std::string& message);
    bool is_numeric(DataType type) const;
    bool can_assign(DataType target, DataType source) const;
    DataType combine_numeric_type(DataType left, DataType right) const;

    void visit(Node* node);
    void visit_program(Node* node);
    void visit_statement_list(Node* node);
    void visit_block(Node* node);
    void visit_declaration(Node* node);
    void visit_assignment(Node* node);
    void visit_if(Node* node);
    void visit_while(Node* node);
    void visit_binary_op(Node* node);
    void visit_identifier(Node* node);
    
    void inject_cast_if_needed(Node* parent, size_t child_index, DataType target_type);
};

#endif
