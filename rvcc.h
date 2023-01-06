// POSIX.1
// for use strndup
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

// token helper functions
bool tokenCompare(Token const *tok, const char *expected);
Token *tokenSkip(Token *tok, const char *expected);
bool tokenConsume(Token **rest, Token *tok, char *str);

// error helper functions
void error(const char *fmt, ...);
void errorAt(char *idx, char *fmt, ...);
void errorTok(Token *tok, char *fmt, ...);

typedef enum {
  TY_INT,
  TY_POINTER,
} TypeKind;

typedef struct Type {
  TypeKind kind;
  struct Type *base;
  Token *name;
} Type;

// local variable
typedef struct Obj {
  struct Obj *next;
  char *name;
  Type *dataType;
  int offSet; // the offset of fp
} Obj;

// define in type.c
extern Type *TyInt;

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
  ND_LOOP,   // for or while
  ND_ADDR,   // &
  ND_DEREF,  // *

  ND_EXPR_STMT, // Expression Statements
  ND_BLOCK,     // code block {}
} NodeType;

// AST tree node
typedef struct Node {
  NodeType nodeType;
  Type *dataType; // the data type in the node

  struct Node *next; // Referring to the next statement
  Token *tok;        // reference to the token

  struct Node *left;
  struct Node *right;

  struct Node *body; // code block

  int val;  // store value for ND_NUM
  Obj *var; // store value for ND_VAR

  // if or for will use
  struct Node *cond; // condition
  struct Node *then; // then
  struct Node *els;  // else

  struct Node *init; // initialization for ND_LOOP(for)
  struct Node *inc;  // increment for ND_LOOP(for and while)

} Node;

// function
typedef struct Function {
  Node *body;
  Obj *locals;
  int stackSize;
} Function;

// judge if is int
bool isInteger(Type *ty);

// all nodes within a node add type
void addType(Node *Nd);

// builds a pointer type and points to the base class
Type *pointerTo(Type *base);

// =================================================================

// Syntax parsing entry
Token *tokenize();

// Semantic analysis and code entry
Function *parse(Token *Tok);

// Code Generation entry
void codegen(Function *prog);