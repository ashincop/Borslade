#include <stdint.h>
// Forward declaration needed for recursive pointers in ASTNode
typedef struct ASTNode ASTNode;

typedef enum {
    TOK_INT,
    TOK_IF,
    TOK_RETURN,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_MATH,
    TOK_BITWISE_AND,
    TOK_BITWISE_OR,
    TOK_BITWISE_XOR,
    TOK_BITWISE_RIGHT_SHIFT,
    TOK_BITWISE_LEFT_SHIFT,
    TOK_BITWISE_NOT,
    TOK_POINTER,
    TOK_CHAR,
    TOK_SEMICOLON,
    TOK_VOID,
    TOK_EQU
} TokenType;

typedef struct {
    TokenType type;
    char* lexeme;   // The raw string from the source
    int value;      // Pre-parsed integer value for TOK_NUMBER
} Token;

typedef enum {
    AST_VAR_DECL,
    AST_NUMBER,
    AST_BINARY_OP,
    AST_SIMPLE_EXPR
} ASTType;

typedef enum {
    TYPE_INT,
    TYPE_CHAR,
    TYPE_PTR,
    TYPE_VOID
} CType;  

typedef enum {
    OP_MULTIPLY,
    OP_ADD,
    OP_SUBTRACT,
    OP_DIVIDE,
    OP_MODULUS
} OpType;

struct ASTNode {
    ASTType type;

    union {
        // Variable Declaration: int x = 5;
        struct {
            char* name;
            CType c_type;
            ASTNode* init;
        } var_decl;

        // Literal Numbers: 42
        struct {
            int value;
        } number;

        // Binary Operations: left + right
        struct {
            int op; // Can be a TokenType or OpType
            ASTNode* left;
            ASTNode* right;
        } binary;

        // Simple Math Expressions
        struct {
            int a;
            int b;
            OpType op;
        } simple_expression;
    } data;
};

void init_compiler();
void lex(const char* line);
ASTNode* parse_statement();