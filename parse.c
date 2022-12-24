#include "rvcc.h"

/*
 * Generate AST (Abstract Syntax Tree), syntax analysis
 * here are some rules for how to generate AST tree to support basic arithmetic
 * like (), *, /
 */

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

// Each function represents a generating rule
// The higher the priority the execution rule is the lower in the AST tree.

// expr = equality
static Node *expr(Token **rest, Token *tok);

// equality = relational ( "==" ralational | "!=" ralational) *
static Node *equality(Token **rest, Token *tok);

// relational = add ( "<" add | "<=" add | ">" add | ">=" add) *
static Node *relational(Token **rest, Token *tok);

// add = mul ("+" mul | "-" mul) *
static Node *add(Token **rest, Token *tok);

// mul = unary ("*" unary | "/" unary) *
static Node *mul(Token **rest, Token *tok);

// unary = ( "+" | "-" ) unary | primary
static Node *unary(Token **rest, Token *tok);

// primary = "(" expr ")" | num
static Node *primary(Token **rest, Token *tok);

// =================================================================

static Node *expr(Token **rest, Token *tok) { return equality(rest, tok); }

static Node *equality(Token **rest, Token *tok) {
  Node *d = relational(&tok, tok);

  while (true) {
    if (tokenCompare(tok, "==")) {
      d = newBinary(ND_EQ, d, relational(&tok, tok->next));
      continue;
    }
    if (tokenCompare(tok, "!=")) {
      d = newBinary(ND_NE, d, relational(&tok, tok->next));
      continue;
    }
    break;
  }

  *rest = tok;
  return d;
}

static Node *relational(Token **rest, Token *tok) {
  Node *d = add(&tok, tok);

  while (true) {
    if (tokenCompare(tok, "<")) {
      d = newBinary(ND_LT, d, relational(&tok, tok->next));
      continue;
    }
    if (tokenCompare(tok, "<=")) {
      d = newBinary(ND_LE, d, relational(&tok, tok->next));
      continue;
    }
    if (tokenCompare(tok, ">")) {
      d = newBinary(ND_LT, relational(&tok, tok->next), d);
      continue;
    }
    if (tokenCompare(tok, ">=")) {
      d = newBinary(ND_LE, relational(&tok, tok->next), d);
      continue;
    }
    break;
  }

  *rest = tok;
  return d;
}

static Node *add(Token **rest, Token *tok) {
  Node *d = mul(&tok, tok);

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
  Node *d = unary(&tok, tok);

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

static Node *unary(Token **rest, Token *tok) {
  if (tokenCompare(tok, "+"))
    return unary(rest, tok->next);
  if (tokenCompare(tok, "-"))
    return newUnary(ND_NEG, unary(rest, tok->next));
  return primary(rest, tok);
}

static Node *primary(Token **rest, Token *tok) {
  if (tokenCompare(tok, "(")) {
    Node *d = expr(&tok, tok->next);
    *rest = tokenSkip(tok, ")");
    return d;
  }

  if (tok->type == TK_NUM) {
    Node *d = newNum(tok->val);
    *rest = tok->next;
    return d;
  }

  errorTok(tok, "invalid expression");
  return NULL;
}

Node *parse(Token *tok) {
  Node *d = expr(&tok, tok);
  if (tok->type != TK_EOF)
    errorTok(tok, "extra token!");
  return d;
}