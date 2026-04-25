%{
#include "ast.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

extern int yylex();
extern int yylineno;
extern int yycolumn;
extern FILE* yyin;

void yyerror(const char* message);

Node* root = nullptr;
SymbolTable symbols;
std::vector<SemanticError> semantic_errors;

static std::string take_lexeme(char* text) {
    std::string value = text == nullptr ? "" : text;
    free(text);
    return value;
}

static void push_scope() {
    symbols.enter_scope();
}

static void pop_scope() {
    symbols.leave_scope();
}

static void report_semantic_error(int line, int column, const std::string& message) {
    semantic_errors.push_back({message, line, column});
}

static bool is_numeric(DataType type) {
    return type == DataType::Int || type == DataType::Float;
}

static bool can_assign(DataType target, DataType source) {
    if (target == DataType::Invalid || source == DataType::Invalid) {
        return false;
    }
    if (target == source) {
        return true;
    }
    return target == DataType::Float && source == DataType::Int;
}

static DataType combine_numeric_type(DataType left, DataType right) {
    if (!is_numeric(left) || !is_numeric(right)) {
        return DataType::Invalid;
    }
    if (left == DataType::Float || right == DataType::Float) {
        return DataType::Float;
    }
    return DataType::Int;
}
%}

%locations
%error-verbose

%union {
    char* sval;
    Node* nptr;
    DataType dtype;
}

%token <sval> IDENTIFIER INT_LITERAL FLOAT_LITERAL
%token KW_INT KW_FLOAT KW_IF KW_WHILE
%token ASSIGN PLUS MINUS STAR SLASH SEMI
%token LBRACE RBRACE LPAREN RPAREN
%token INVALID_TOKEN

%type <nptr> program stmt_list stmt block declaration assignment expr
%type <dtype> type_spec

%left PLUS MINUS
%left STAR SLASH

%%

program:
    {
        push_scope();
    }
    stmt_list
    {
        root = new Node("Program");
        root->add_child($2);
        pop_scope();
    }
    ;

stmt_list:
    /* empty */
    {
        $$ = new Node("StatementList");
    }
    | stmt_list stmt
    {
        $1->add_child($2);
        $$ = $1;
    }
    ;

stmt:
    declaration SEMI
    {
        $$ = $1;
    }
    | assignment SEMI
    {
        $$ = $1;
    }
    | block
    {
        $$ = $1;
    }
    | KW_IF LPAREN expr RPAREN stmt
    {
        $$ = new Node("If");
        if (!is_numeric($3->type())) {
            report_semantic_error(@3.first_line, @3.first_column, "la condicion de if debe ser numerica");
        }
        $$->add_child($3);
        $$->add_child($5);
    }
    | KW_WHILE LPAREN expr RPAREN stmt
    {
        $$ = new Node("While");
        if (!is_numeric($3->type())) {
            report_semantic_error(@3.first_line, @3.first_column, "la condicion de while debe ser numerica");
        }
        $$->add_child($3);
        $$->add_child($5);
    }
    ;

block:
    LBRACE
    {
        push_scope();
    }
    stmt_list
    RBRACE
    {
        $$ = new Node("Block");
        $$->add_child($3);
        pop_scope();
    }
    ;

declaration:
    type_spec IDENTIFIER
    {
        std::string name = take_lexeme($2);
        $$ = new Node("Declaration", name, $1);
        if (!symbols.define(name, $1)) {
            report_semantic_error(@2.first_line, @2.first_column, "redeclaracion de '" + name + "' en el mismo ambito");
        }
    }
    | type_spec IDENTIFIER ASSIGN expr
    {
        std::string name = take_lexeme($2);
        $$ = new Node("Declaration", name, $1);
        $$->add_child($4);
        if (!symbols.define(name, $1)) {
            report_semantic_error(@2.first_line, @2.first_column, "redeclaracion de '" + name + "' en el mismo ambito");
        } else if (!can_assign($1, $4->type())) {
            report_semantic_error(@4.first_line, @4.first_column, "no se puede asignar un valor de tipo " +
                std::string(data_type_name($4->type())) + " a una variable de tipo " +
                std::string(data_type_name($1)));
        }
    }
    ;

assignment:
    IDENTIFIER ASSIGN expr
    {
        std::string name = take_lexeme($1);
        $$ = new Node("Assignment", name);
        $$->add_child($3);

        const Symbol* symbol = symbols.lookup(name);
        if (symbol == nullptr) {
            report_semantic_error(@1.first_line, @1.first_column, "uso de variable no declarada '" + name + "'");
            $$->set_type(DataType::Invalid);
        } else {
            $$->set_type(symbol->type);
            if (!can_assign(symbol->type, $3->type())) {
                report_semantic_error(@3.first_line, @3.first_column, "no se puede asignar un valor de tipo " +
                    std::string(data_type_name($3->type())) + " a una variable de tipo " +
                    std::string(data_type_name(symbol->type)));
            }
        }
    }
    ;

expr:
    IDENTIFIER
    {
        std::string name = take_lexeme($1);
        const Symbol* symbol = symbols.lookup(name);
        if (symbol == nullptr) {
            report_semantic_error(@1.first_line, @1.first_column, "uso de variable no declarada '" + name + "'");
            $$ = new Node("Identifier", name, DataType::Invalid);
        } else {
            $$ = new Node("Identifier", name, symbol->type);
        }
    }
    | INT_LITERAL
    {
        $$ = new Node("IntLiteral", take_lexeme($1), DataType::Int);
    }
    | FLOAT_LITERAL
    {
        $$ = new Node("FloatLiteral", take_lexeme($1), DataType::Float);
    }
    | LPAREN expr RPAREN
    {
        $$ = $2;
    }
    | expr PLUS expr
    {
        $$ = new Node("BinaryOp", "+");
        $$->add_child($1);
        $$->add_child($3);
        DataType result = combine_numeric_type($1->type(), $3->type());
        if (result == DataType::Invalid) {
            report_semantic_error(@2.first_line, @2.first_column, "el operador + requiere operandos numericos");
        }
        $$->set_type(result);
    }
    | expr MINUS expr
    {
        $$ = new Node("BinaryOp", "-");
        $$->add_child($1);
        $$->add_child($3);
        DataType result = combine_numeric_type($1->type(), $3->type());
        if (result == DataType::Invalid) {
            report_semantic_error(@2.first_line, @2.first_column, "el operador - requiere operandos numericos");
        }
        $$->set_type(result);
    }
    | expr STAR expr
    {
        $$ = new Node("BinaryOp", "*");
        $$->add_child($1);
        $$->add_child($3);
        DataType result = combine_numeric_type($1->type(), $3->type());
        if (result == DataType::Invalid) {
            report_semantic_error(@2.first_line, @2.first_column, "el operador * requiere operandos numericos");
        }
        $$->set_type(result);
    }
    | expr SLASH expr
    {
        $$ = new Node("BinaryOp", "/");
        $$->add_child($1);
        $$->add_child($3);
        DataType result = combine_numeric_type($1->type(), $3->type());
        if (result == DataType::Invalid) {
            report_semantic_error(@2.first_line, @2.first_column, "el operador / requiere operandos numericos");
        }
        $$->set_type(result);
    }
    ;

type_spec:
    KW_INT
    {
        $$ = DataType::Int;
    }
    | KW_FLOAT
    {
        $$ = DataType::Float;
    }
    ;

%%

void yyerror(const char* message) {
    std::cerr << "syntax error: " << message
              << " at line " << yylloc.first_line
              << ", column " << yylloc.first_column << '\n';
}

int main() {
    if (yyparse() != 0) {
        return 1;
    }

    if (!semantic_errors.empty()) {
        for (const auto& error : semantic_errors) {
            std::cerr << "semantic error: " << error.message
                      << " at line " << error.line
                      << ", column " << error.column << '\n';
        }
        delete root;
        return 1;
    }

    if (root != nullptr) {
        root->print(std::cout);
        delete root;
    }

    return 0;
}
