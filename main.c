#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *currentInput;
static int Depth;

static void error(const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);         // get all arguments after fmt
  vfprintf(stderr, fmt, va); // printf the arguments with va_list types
  fprintf(stderr, "\n");
  va_end(va);
  exit(1);
}

// printf where error occurred
static void _errorAt(char *idx, char *fmt, va_list va) {
  // 0. print current input
  fprintf(stderr, "%s\n", currentInput);

  // 1. calculate the index where error occurred
  int errorIdx = idx - currentInput;

  // 2. print
  fprintf(stderr, "%*s", errorIdx, ""); // filled spaces, similar to "%5d"
  fprintf(stderr, "^ ");                // point out where error occurred
  vfprintf(stderr, fmt, va);            // print specific error message
  fprintf(stderr, "\n");

  va_end(va);
}

// char parsing error
static void errorAt(char *idx, char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  _errorAt(idx, fmt, va);
  exit(1);
}

/*
 * Lexical analysis
 */

typedef enum {
  TK_INVALID = 0,
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

static Token *newToken(TokenType type, char *start, char *end) {
  // to simplify, not considering memory recovery recycle
  Token *tok = (Token *)calloc(1, sizeof(Token));
  tok->type = type;
  tok->idx = start;
  tok->len = end - start;
  return tok;
}

// tok parsing error
static void errorTok(Token *tok, char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  _errorAt(tok->idx, fmt, va);
  exit(1);
}

// verify that a token is expected
static bool tokenCompare(Token const *tok, const char *expected) {
  return memcmp(tok->idx, expected, tok->len) == 0 &&
         expected[tok->len] == '\0';
}

// if the token is expected, then skip it
static Token *tokenSkip(Token *tok, const char *expected) {
  if (!tokenCompare(tok, expected)) {
    char dest[tok->len];
    memcpy(dest, tok->idx, tok->len);
    dest[tok->len] = '\0';
    errorTok(tok, "expected str is %s, but got %s\n", expected, dest);
  }
  return tok->next;
}

// lexical analysis
static Token *tokenize() {
  Token head = {}; // empty head
  Token *cur = &head;
  char *p = currentInput;

  while (*p) {
    // Skip all blank characters
    if (isspace(*p)) {
      ++p;
      continue;
    }

    if (isdigit(*p)) {
      cur->next = newToken(TK_NUM, p, p);
      cur = cur->next;
      const char *oldPtr = p;
      cur->val = strtoul(p, &p, 10);
      cur->len = p - oldPtr;
      continue;
    }

    if (ispunct(*p)) {
      cur->next = newToken(TK_PUNCT, p, p + 1);
      cur = cur->next;
      ++p;
      continue;
    }

    errorAt(p, "invalid token");
  }
  // add eof to represents the end
  cur->next = newToken(TK_EOF, p, p);
  return head.next;
}

// get the value of TK_NUM
static int getNumber(Token *tok) {
  if (tok->type != TK_NUM)
    errorTok(tok, "expected number");
  return tok->val;
}

/*
 * Generate AST (Abstract Syntax Tree), syntax analysis
 * here are some rules for how to generate AST tree to support basic arithmetic
 * like (), *, /
 */

typedef enum {
  ND_INVALID = 0,

  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /

  ND_NEG, //

  ND_NUM, // int
} NodeType;

typedef struct Node {
  NodeType type;
  struct Node *left;
  struct Node *right;
  int val;
} Node;

static Node *newNode(NodeType type) {
  Node *d = calloc(1, sizeof(Node));
  d->type = type;
  return d;
}

static Node *newNum(int val) {
  Node *d = newNode(ND_NUM);
  d->val = val;
  return d;
}

static Node *newUnary(NodeType type, Node *expr) {
  Node *d = newNode(type);
  d->left = expr;
  return d;
}

static Node *newBinary(NodeType type, Node *left, Node *right) {
  Node *d = newNode(type);
  d->left = left;
  d->right = right;
  return d;
}

// expr = mul ( "+" mul | "-" mul ) *
static Node *expr(Token **rest, Token *tok);

// mul = unary ("*" unary | "/" unary) *
static Node *mul(Token **rest, Token *tok);

// unary = ( "+" | "-" ) unary | primary
static Node *unary(Token **rest, Token *tok);

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok);

static Node *expr(Token **rest, Token *tok) {
  Node *d = mul(&tok, tok);

  // ( "+" mul | "-" mul ) *
  while (true) {
    if (tokenCompare(tok, "+")) {
      d = newBinary(ND_ADD, d, mul(&tok, tok->next));
      continue;
    }

    if (tokenCompare(tok, "-")) {
      d = newBinary(ND_SUB, d, mul(&tok, tok->next));
      continue;
    }

    break;
  }

  *rest = tok;
  return d;
}

static Node *mul(Token **rest, Token *tok) {
  // unary
  Node *d = unary(&tok, tok);

  // ("*" unary | "/" unary) *
  while (true) {
    if (tokenCompare(tok, "*")) {
      d = newBinary(ND_MUL, d, unary(&tok, tok->next));
      continue;
    }

    if (tokenCompare(tok, "/")) {
      d = newBinary(ND_DIV, d, unary(&tok, tok->next));
      continue;
    }

    break;
  }

  *rest = tok;
  return d;
}

// unary = ( "+" | "-" ) unary | primary
static Node *unary(Token **rest, Token *tok) {
  if (tokenCompare(tok, "+"))
    return unary(rest, tok->next);
  if (tokenCompare(tok, "-"))
    return newUnary(ND_NEG, unary(rest, tok->next));
  return primary(rest, tok);
}

static Node *primary(Token **rest, Token *tok) {
  // "(" expr ")"
  if (tokenCompare(tok, "(")) {
    Node *d = expr(&tok, tok->next);
    *rest = tokenSkip(tok, ")");
    return d;
  }

  // num
  if (tok->type == TK_NUM) {
    Node *d = newNum(tok->val);
    *rest = tok->next;
    return d;
  }

  errorTok(tok, "invalid expression");
  return NULL;
}

/*
 *  Semantic analysis and code generation
 */
static int StackDepth;

// push the value of a0 onto the stack
static void push() {
  //  sp is the stack pointer, the stack grows downwards, under 64
  //  bits, 8 bytes is a unit, so sp-8
  printf("  addi sp, sp, -8\n");
  //  press the value of a0 onto the stack
  printf("  sd a0, 0(sp)\n");
  StackDepth++;
}

// pop the value of the address pointed to by sp into the register
static void pop(char *reg) {
  printf("  ld %s, 0(sp)\n", reg);
  printf("  addi sp, sp, 8\n");
  StackDepth--;
}

// traversing the AST tree to generate assembly code
static void genExpr(Node *node) {

  // load data to a0 register
  switch (node->type) {
  case ND_NUM:
    printf("  li a0, %d\n", node->val);
    return;
  case ND_NEG:
    genExpr(node->left);
    printf("  neg a0, a0\n");
    return;
  default:
    break;
  }

  // consider the priority, recurse to the right node first
  genExpr(node->right);
  push();
  genExpr(node->left);
  pop("a1");

  // generate what each binary tree node does in assembly code
  switch (node->type) {
  case ND_ADD:
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB:
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL:
    printf("  mul a0, a0, a1\n");
    return;
  case ND_DIV:
    printf("  div a0, a0, a1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: Invalid number of arguments %d", argv[0], argc);
  }

  // parse argv[1] to generate a stream of tokens
  currentInput = argv[1];
  Token *tok = tokenize();

  // parse the stream of tokens
  Node *node = expr(&tok, tok);

  printf("  .global main\n");
  printf("main:\n");

  genExpr(node);

  // ret is alias for "jalr x0, x1, 0"
  printf("  ret\n");

  assert(Depth == 0);

  return 0;
}