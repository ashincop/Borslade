#include "tbcc.h"
#include "syscall.h"
void print_ast(ASTNode* node, int indent) {
    if (!node) return;

    // Simple indentation logic
    for (int i = 0; i < indent; i++) printf("  ");

    switch (node->type) {
        case AST_VAR_DECL:
            printf("[Variable Decl] name: %s, type: INT\n", node->data.var_decl.name);
            if (node->data.var_decl.init) {
                print_ast(node->data.var_decl.init, indent + 1);
            }
            break;

        case AST_SIMPLE_EXPR:
            printf("[Simple Expr] %d (OP: %d) %d\n", 
                   node->data.simple_expression.a, 
                   node->data.simple_expression.op, 
                   node->data.simple_expression.b);
            break;

        case AST_NUMBER:
            printf("[Number] %d\n", node->data.number.value);
            break;

        default:
            printf("[Unknown Node Type]\n");
            break;
    }
}
int _start() {
    init_compiler();
    printf("1");
    // Source string to test
    const char* input = "int x = 10 + 20;";
    printf("2");
    // 1. Lexing
    lex(input);
    printf("3");
    // 2. Parsing
    ASTNode* root = parse_statement();
    printf("4");
    // 3. Output AST
    if (root) {
        printf("\n--- AST Output ---\n");
        printf("5");
        print_ast(root, 0);
        printf("6");
    } else {
        printf("\nParsing failed.\n");
        printf("5&6");
    }
    for(;;);
}