// POSIX.1标准
// 使用了strndup函数
#define _POSIX_C_SOURCE 200809L

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
  TK_IDENT, // mark for variable name and function name
  TK_PUNCT, // opertor like +, -
  TK_NUM,
  TK_KEYWORD, // return
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

// local variable
typedef struct Obj {
  struct Obj *next;
  char *name;
  int offSet; // the offset of fp
} Obj;

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
  ND_VAR,    // variable
  ND_RETURN, // return
  ND_IF,     // if
  ND_FOR,    // for

  ND_EXPR_STMT, // Expression Statements
  ND_BLOCK,     // code block {}
} NodeType;

// AST tree node
typedef struct Node {
  NodeType type;
  struct Node *next; // Referring to the next statement
  struct Node *left;
  struct Node *right;

  struct Node *body; // code block

  int val;  // store value for ND_NUM
  Obj *var; // store value for ND_VAR

  // if or for will use
  struct Node *cond; // condition
  struct Node *then; // then
  struct Node *els;  // else

  struct Node *init; // initialization for ND_FOR
  struct Node *inc;  // increment for ND_FOR

} Node;

// function
typedef struct Function {
  Node *body;
  Obj *locals;
  int stackSize;
} Function;

// Syntax parsing entry
Token *tokenize();

// Semantic analysis and code entry
Function *parse(Token *Tok);

// Code Generation entry
void codegen(Function *prog);