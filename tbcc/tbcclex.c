#include "tbcc.h"
#include "syscall.h"

/* --- Globals --- */
Token *tokenarray; 
int lp = 0;    // Lexer count
int pos = 0;   // Parser index

void init_compiler() {
    // 2048 tokens is plenty for a test string
    tokenarray = (Token*)calloc(2048 * sizeof(Token));
}

/* --- Lexer --- */

void lex(const char* line) {
    int i = 0;
    while (line[i]) {
        if (line[i] == ' ' || line[i] == '\n' || line[i] == '\t') {
            i++; continue;
        }

        // Numbers
        if (isdigit(line[i])) {
            int num = 0;
            int start = i;
            while (isdigit(line[i])) {
                num = num * 10 + (line[i] - '0');
                i++;
            }
            int len = i - start;
            char* lx = (char*)malloc(len + 1);
            memcpy(lx, &line[start], len);
            lx[len] = 0;

            tokenarray[lp++] = (Token){TOK_NUMBER, lx, num};
            continue;
        }

        // Identifiers / Keywords
        if (isalpha(line[i])) {
            int start = i;
            while (isalnum(line[i])) i++;
            int len = i - start;
            char* str = (char*)malloc(len + 1);
            memcpy(str, &line[start], len);
            str[len] = 0;

            TokenType type = TOK_IDENTIFIER;
            if      (strcmp(str, "int")    == 0) type = TOK_INT;
            else if (strcmp(str, "if")     == 0) type = TOK_IF;
            else if (strcmp(str, "return") == 0) type = TOK_RETURN;

            tokenarray[lp++] = (Token){type, str, 0};
            continue;
        }

        // Symbols
        if (line[i] == '=') {
            tokenarray[lp++] = (Token){TOK_EQU, "=", 0};
        } else if (line[i] == ';') {
            tokenarray[lp++] = (Token){TOK_SEMICOLON, ";", 0};
        } else if (line[i] == '+' || line[i] == '-' || line[i] == '*' || line[i] == '/') {
            char* op = (char*)malloc(2);
            op[0] = line[i]; op[1] = 0;
            tokenarray[lp++] = (Token){TOK_MATH, op, 0};
        }
        i++;
    }
}

/* --- Parser Helpers --- */

Token* cur() { return &tokenarray[pos]; }

Token* expect(TokenType t) {
    if (cur()->type != t) {
        printf("[ERRO] Expected %d, got %s\n", t, cur()->lexeme);
        return (void*)0; 
    }
    return &tokenarray[pos++]; // Return the token and advance
}

/* --- Parser Core --- */

ASTNode* parse_expression() {
    // 1. First Number
    Token* t1 = expect(TOK_NUMBER);
    if (!t1) return (void*)0;
    int val_a = t1->value;

    // 2. Check for optional Math
    if (cur()->type == TOK_MATH) {
        char* op_str = tokenarray[pos++].lexeme; // consume the '+'
        
        Token* t2 = expect(TOK_NUMBER);
        if (!t2) return (void*)0;
        int val_b = t2->value;

        ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
        n->type = AST_SIMPLE_EXPR;
        n->data.simple_expression.a = val_a;
        n->data.simple_expression.b = val_b;

        // Map OpType
        if      (strcmp(op_str, "+") == 0) n->data.simple_expression.op = OP_ADD;
        else if (strcmp(op_str, "-") == 0) n->data.simple_expression.op = OP_SUBTRACT;
        else if (strcmp(op_str, "*") == 0) n->data.simple_expression.op = OP_MULTIPLY;
        else if (strcmp(op_str, "/") == 0) n->data.simple_expression.op = OP_DIVIDE;
        
        return n;
    }

    // 3. Just a literal number
    ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
    n->type = AST_NUMBER;
    n->data.number.value = val_a;
    return n;
}

ASTNode* parse_statement() {
    // Match: int x = 10 + 20;
    if (cur()->type == TOK_INT) {
        pos++; // consume 'int'

        ASTNode* n = (ASTNode*)malloc(sizeof(ASTNode));
        n->type = AST_VAR_DECL;
        n->data.var_decl.c_type = TYPE_INT;

        Token* id = expect(TOK_IDENTIFIER);
        if (!id) return (void*)0;
        n->data.var_decl.name = id->lexeme;

        if (!expect(TOK_EQU)) return (void*)0;

        n->data.var_decl.init = parse_expression();

        if (!expect(TOK_SEMICOLON)) return (void*)0;

        return n;
    }
    return (void*)0;
}

/* --- Debug Visualizer --- */

void print_ast(ASTNode* n) {
    if (!n) return;
    if (n->type == AST_VAR_DECL) {
        printf("VAR_DECL: %s\n", n->data.var_decl.name);
        printf("  |_ INIT: ");
        print_ast(n->data.var_decl.init);
    } else if (n->type == AST_SIMPLE_EXPR) {
        printf("BINARY_OP (val:%d, op:%d, val:%d)\n", 
               n->data.simple_expression.a, 
               n->data.simple_expression.op, 
               n->data.simple_expression.b);
    } else if (n->type == AST_NUMBER) {
        printf("LITERAL: %d\n", n->data.number.value);
    }
}
void print_int(int x) {
    char buf[16];
    int i = 0;

    if (x == 0) {
        print("0");
        return;
    }

    if (x < 0) {
        print("-");
        x = -x;
    }

    while (x > 0) {
        buf[i++] = '0' + (x % 10);
        x /= 10;
    }

    while (i--) {
        char c[2] = { buf[i], 0 };
        print(c);
    }
}

void print_tokens() {
    print("--- Lexer Output ---\n");

    for (int i = 0; i < lp; i++) {
        print("TOKEN\n");
        print("  type: ");
        print_int(tokenarray[i].type);
        print("\n");

        print("  lexeme: ");
        if (tokenarray[i].lexeme)
            print(tokenarray[i].lexeme);
        else
            print("NULL");
        print("\n");

        print("  value: ");
        print_int(tokenarray[i].value);
        print("\n\n");
    }

    print("-------------------\n");
}

void _start() {
    init_compiler();
    const char* input = "int x = 10 + 20;";
    lex(input);
    print_tokens();
    ASTNode* root = parse_statement();
    if (root) {
        printf("AST SUCCESS:\n");
        print_ast(root);
    }
    for(;;);
}