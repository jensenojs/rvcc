#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *currentInput;

typedef enum {
  TK_OPERATOR,
  TK_NUM,
  TK_EOF,
} TokenKind;

typedef struct Token {
  TokenKind kind;
  struct Token *next;
  int val;
  int len;
  char *idx;
} Token;

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

// tok parsing error
static void errorTok(Token *tok, char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  _errorAt(tok->idx, fmt, va);
  exit(1);
}

static Token *newToken(TokenKind kind, char *start, char *end) {
  // to simplify, not considering memory recovery recycle
  Token *tok = (Token *)calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->idx = start;
  tok->len = end - start;
  return tok;
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

    if (*p == '+' || *p == '-') {
      cur->next = newToken(TK_OPERATOR, p, p + 1);
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
  if (tok->kind != TK_NUM)
    errorTok(tok, "expected number");
  return tok->val;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: Invalid number of arguments %d", argv[0], argc);
  }

  currentInput = argv[1];
  Token *tok = tokenize();

  printf("  .global main\n");
  printf("main:\n");

  // num (op num) (op num)
  printf("  li a0, %ld\n", (long)getNumber(tok));
  tok = tok->next;

  // resolve (op num)
  while (tok->kind != TK_EOF) {
    if (tokenCompare(tok, "+")) {
      tok = tok->next;
      printf("  addi a0, a0, %ld\n", (long)getNumber(tok));
      tok = tok->next;
      continue;
    }

    tok = tokenSkip(tok, "-");
    printf("  addi a0, a0, -%ld\n", (long)getNumber(tok));
    tok = tok->next;
    continue;
  }

  // ret is alias for "jalr x0, x1, 0"
  printf("  ret\n");

  return 0;
}