#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void error(const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);         // get all arguments after fmt
  vfprintf(stderr, fmt, va); // printf the arguments with va_list types
  fprintf(stderr, "\n");
  va_end(va);
  exit(1);
}

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
  const char *idx;
} Token;

static Token *newToken(TokenKind kind, const char *start, const char *end) {
  // to simplify, not considering memory recovery
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->idx = start;
  tok->len = end - start;
  return tok;
}

static bool tokenCompare(Token const *tok, const char *str) {
  return memcmp(tok->idx, str, tok->len) == 0 && str[tok->len] == '\0';
}

static Token *tokenSkip(Token *tok, const char *str) {
  if (!tokenCompare(tok, str)) {
    char dest[tok->len];
    memcpy(dest, tok->idx, tok->len);
    dest[tok->len] = '\0';
    error("expected str is %s, but got %s\n", str, dest);
  }
  return tok->next;
}

static int getNumber(Token *tok) {
  if (tok->kind != TK_NUM)
    error("expected number");
  return tok->val;
}

static Token *tokenize(char *p) {
  Token head = {}; // empty head
  Token *cur = &head;

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

    error("invalid token: %c", *p);
  }
  // add eof to represents the end
  cur->next = newToken(TK_EOF, p, p);
  return head.next;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: Invalid number of arguments %d", argv[0], argc);
  }

  Token *tok = tokenize(argv[1]);

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