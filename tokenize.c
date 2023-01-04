#include "rvcc.h"

static char *currentInput;

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

void error(const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);         // get all arguments after fmt
  vfprintf(stderr, fmt, va); // printf the arguments with va_list types
  fprintf(stderr, "\n");
  va_end(va);
  exit(1);
}

// char parsing error
void errorAt(char *idx, char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  _errorAt(idx, fmt, va);
  exit(1);
}

// tok parsing error
void errorTok(Token *tok, char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  _errorAt(tok->idx, fmt, va);
  exit(1);
}

// =========================================================================

static Token *newToken(TokenType type, char *start, char *end) {
  // to simplify, not considering memory recovery recycle
  Token *tok = (Token *)calloc(1, sizeof(Token));
  tok->type = type;
  tok->idx = start;
  tok->len = end - start;
  return tok;
}

// verify that a token is expected
bool tokenCompare(Token const *tok, const char *expected) {
  return memcmp(tok->idx, expected, tok->len) == 0 &&
         expected[tok->len] == '\0';
}

// if the token is expected, then skip it
Token *tokenSkip(Token *tok, const char *expected) {
  if (!tokenCompare(tok, expected)) {
    char dest[tok->len];
    memcpy(dest, tok->idx, tok->len);
    dest[tok->len] = '\0';
    errorTok(tok, "expected str is %s, but got %s\n", expected, dest);
  }
  return tok->next;
}

static bool ifStartWith(char *str, char *substr) {
  return strncmp(str, substr, strlen(substr)) == 0;
}

static int readPunct(char *str) {
  if (ifStartWith(str, "==") || ifStartWith(str, "!=") ||
      ifStartWith(str, ">=") || ifStartWith(str, "<=")) {
    return 2;
  }
  return ispunct(*str) ? 1 : 0;
}

// rules for determining the initial letter of a ident or a keyword
// [a-zA-Z_]
static bool isIdent1(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// rules for determining the initial letter of a ident
// [a-zA-Z_]
static bool isIdent2(char c) { return isIdent1(c) || ('0' <= c && c <= '9'); }

static bool isKeyword(Token *tok) {
  static char *kws[] = {
    "return",
    "if",
    "else",
    "for",
  };

  for (int i = 0; i < sizeof(kws) / sizeof(*kws); i++)
    if (tokenCompare(tok, kws[i]))
      return true;

  return false;
}

static void markKeyword(Token *tok) {
  for (Token *t = tok; t->type != TK_EOF; t = t->next) {
    // 将名为“return”的终结符转为KEYWORD
    if (isKeyword(t))
      t->type = TK_KEYWORD;
  }
}

// lexical analysis
Token *tokenize(char *p) {
  currentInput = p;
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

    // parse INDET
    if (isIdent1(*p)) {
      char *start = p;
      do {
        ++p;
      } while (isIdent2(*p));
      cur->next = newToken(TK_IDENT, start, p);
      cur = cur->next;
      continue;
    }

    int punctLen = readPunct(p);
    if (punctLen) {
      // I'm not sure, but clang seems to have some inconsistencies with my
      // understanding of arithmetic operations on pointers
      // newToken(TK_PUNCT, p, p + punctLen); is not right when punctLen is 2
      cur->next = newToken(TK_PUNCT, p, p + punctLen);
      cur = cur->next;
      // cur->len = punctLen;
      p += punctLen;
      continue;
    }

    errorAt(p, "invalid token");
  }
  // add eof to represents the end
  cur->next = newToken(TK_EOF, p, p);

  // mark if one token is keyword
  markKeyword(head.next);

  return head.next;
}
