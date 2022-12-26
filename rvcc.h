#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// token for parsing
typedef enum {
  TK_INVALID = 0,
  TK_INDET, // mark for variable name and function name
  TK_PUNCT,
  TK_NUM,
  TK_EOF,
} TokenType;

typedef struct Token {
  TokenType type;
  struct Token *next;
  char *idx; // indicates the starting position of this  in the input stream
  int val;   // use if TokenType is TK_NUM
  int len;
} Token;

bool tokenCompare(Token const *tok, const char *expected);
Token *tokenSkip(Token *tok, const char *expected);

// error
void error(const char *fmt, ...);
void errorAt(char *idx, char *fmt, ...);
void errorTok(Token *tok, char *fmt, ...);

// Types of Nodes for AST
typedef enum {
  ND_INVALID = 0,

  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /

  ND_NEG, // negative number
  ND_NUM, // int

  ND_EQ, // ==
  ND_NE, // !=
  ND_LT, // < (or >)
  ND_LE, // <= (or >=)

  ND_ASSIGN,
  ND_VAR, // variable

  ND_EXPR_STMT, // Expression Statements
} NodeType;

typedef struct Node {
  NodeType type;
  struct Node *next; // Referring to the next statement
  struct Node *left;
  struct Node *right;
  int val;   // store ND_NUM
  char name; // store ND_VAR
} Node;

// Syntax parsing entry
Token *tokenize();

// Semantic analysis and code entry
Node *parse(Token *Tok);

// Code Generation entry
void codegen(Node *node);