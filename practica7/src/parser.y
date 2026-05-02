%{
#include "ast.h"
#include "semantic_analyzer.h"

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

static std::string take_lexeme(char* text) {
    std::string value = text == nullptr ? "" : text;
    free(text);
    return value;
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
    stmt_list
    {
        root = new Node("Program");
        root->add_child($1);
    }
    ;

stmt_list:
    /* empty */
    {
        $$ = new Node("StatementList");
        $$->set_location(@$.first_line, @$.first_column);
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
        $$->set_location(@1.first_line, @1.first_column);
        $$->add_child($3);
        $$->add_child($5);
    }
    | KW_WHILE LPAREN expr RPAREN stmt
    {
        $$ = new Node("While");
        $$->set_location(@1.first_line, @1.first_column);
        $$->add_child($3);
        $$->add_child($5);
    }
    ;

block:
    LBRACE stmt_list RBRACE
    {
        $$ = new Node("Block");
        $$->set_location(@1.first_line, @1.first_column);
        $$->add_child($2);
    }
    ;

declaration:
    type_spec IDENTIFIER
    {
        std::string name = take_lexeme($2);
        $$ = new Node("Declaration", name, $1);
        $$->set_location(@2.first_line, @2.first_column);
    }
    | type_spec IDENTIFIER ASSIGN expr
    {
        std::string name = take_lexeme($2);
        $$ = new Node("Declaration", name, $1);
        $$->set_location(@2.first_line, @2.first_column);
        $$->add_child($4);
    }
    ;

assignment:
    IDENTIFIER ASSIGN expr
    {
        std::string name = take_lexeme($1);
        $$ = new Node("Assignment", name);
        $$->set_location(@1.first_line, @1.first_column);
        $$->add_child($3);
    }
    ;

expr:
    IDENTIFIER
    {
        std::string name = take_lexeme($1);
        $$ = new Node("Identifier", name);
        $$->set_location(@1.first_line, @1.first_column);
    }
    | INT_LITERAL
    {
        $$ = new Node("IntLiteral", take_lexeme($1), DataType::Int);
        $$->set_location(@1.first_line, @1.first_column);
    }
    | FLOAT_LITERAL
    {
        $$ = new Node("FloatLiteral", take_lexeme($1), DataType::Float);
        $$->set_location(@1.first_line, @1.first_column);
    }
    | LPAREN expr RPAREN
    {
        $$ = $2;
    }
    | expr PLUS expr
    {
        $$ = new Node("BinaryOp", "+");
        $$->set_location(@2.first_line, @2.first_column);
        $$->add_child($1);
        $$->add_child($3);
    }
    | expr MINUS expr
    {
        $$ = new Node("BinaryOp", "-");
        $$->set_location(@2.first_line, @2.first_column);
        $$->add_child($1);
        $$->add_child($3);
    }
    | expr STAR expr
    {
        $$ = new Node("BinaryOp", "*");
        $$->set_location(@2.first_line, @2.first_column);
        $$->add_child($1);
        $$->add_child($3);
    }
    | expr SLASH expr
    {
        $$ = new Node("BinaryOp", "/");
        $$->set_location(@2.first_line, @2.first_column);
        $$->add_child($1);
        $$->add_child($3);
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

int main(int argc, char** argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            std::cerr << "Cannot open " << argv[1] << "\n";
            return 1;
        }
    }

    if (yyparse() != 0) {
        if (yyin) fclose(yyin);
        return 1;
    }
    
    if (yyin) fclose(yyin);

    if (root != nullptr) {
        SemanticAnalyzer analyzer;
        analyzer.analyze(root);
        
        if (analyzer.has_errors()) {
            analyzer.print_errors(std::cerr);
            delete root;
            return 1;
        }

        root->print(std::cout);
        delete root;
    }

    return 0;
}
