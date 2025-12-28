#include <stdint.h>
enum TokenType {
    TOK_INT,
    TOK_IF,
    TOK_RETURN,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULTIPLY,
    TOK_DIVIDE,
    TOK_MODULUS,
    TOK_BITWISE_AND,
    TOK_BITWISE_OR,
    TOK_BITWISE_XOR,
    TOK_BITWISE_RIGHT_SHIFT,
    TOK_BITWISE_LEFT_SHIFT,
    TOK_BITWISE_NOT,
    TOK_POINTER,
    TOK_CHAR
    };

struct Token {
    TokenType type;
    char* lexeme;   // actual string
    int value;      // for numbers
};
