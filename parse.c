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
  Node *node = newNode(ND_NUM);
  node->val = val;
  return node;
}

static Node *newUnary(NodeType type, Node *expr) {
  Node *node = newNode(type);
  node->left = expr;
  return node;
}

static Node *newVarNode(char Name) {
  Node *node = newNode(ND_VAR);
  node->name = Name;
  return node;
}

static Node *newBinary(NodeType type, Node *left, Node *right) {
  Node *node = newNode(type);
  node->left = left;
  node->right = right;
  return node;
}

// Each function represents a generating rule
// The higher the priority the execution rule is the lower in the AST tree.

// program = stmt*

// stmt = exprStmt

// exprStmt = expr ";"
static Node *exprStmt(Token **rest, Token *tok);

// expr = assign
static Node *expr(Token **rest, Token *tok);

// assign = equality ("=" assign)?
static Node *assign(Token **rest, Token *tok);

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

// primary = "(" expr ")" | num | ident(variable)
static Node *primary(Token **rest, Token *tok);

// =================================================================

Node *stmt(Token **rest, Token *tok) { return exprStmt(rest, tok); }

Node *exprStmt(Token **rest, Token *tok) {
  Node *node = newUnary(ND_EXPR_STMT, expr(&tok, tok));
  *rest = tokenSkip(tok, ";");
  return node;
}

Node *expr(Token **rest, Token *tok) { return assign(rest, tok); }

Node *assign(Token **rest, Token *tok) {
  Node *node = equality(&tok, tok);

  if (tokenCompare(tok, "="))
    node = newBinary(ND_ASSIGN, node, assign(&tok, tok->next));

  *rest = tok;
  return node;
}

Node *equality(Token **rest, Token *tok) {
  Node *node = relational(&tok, tok);

  while (true) {
    if (tokenCompare(tok, "==")) {
      node = newBinary(ND_EQ, node, relational(&tok, tok->next));
      continue;
    }
    if (tokenCompare(tok, "!=")) {
      node = newBinary(ND_NE, node, relational(&tok, tok->next));
      continue;
    }
    break;
  }

  *rest = tok;
  return node;
}

Node *relational(Token **rest, Token *tok) {
  Node *node = add(&tok, tok);

  while (true) {
    if (tokenCompare(tok, "<")) {
      node = newBinary(ND_LT, node, relational(&tok, tok->next));
      continue;
    }
    if (tokenCompare(tok, "<=")) {
      node = newBinary(ND_LE, node, relational(&tok, tok->next));
      continue;
    }
    if (tokenCompare(tok, ">")) {
      node = newBinary(ND_LT, relational(&tok, tok->next), node);
      continue;
    }
    if (tokenCompare(tok, ">=")) {
      node = newBinary(ND_LE, relational(&tok, tok->next), node);
      continue;
    }
    break;
  }

  *rest = tok;
  return node;
}

Node *add(Token **rest, Token *tok) {
  Node *node = mul(&tok, tok);

  while (true) {
    if (tokenCompare(tok, "+")) {
      node = newBinary(ND_ADD, node, mul(&tok, tok->next));
      continue;
    }
    if (tokenCompare(tok, "-")) {
      node = newBinary(ND_SUB, node, mul(&tok, tok->next));
      continue;
    }
    break;
  }
  *rest = tok;
  return node;
}

Node *mul(Token **rest, Token *tok) {
  Node *node = unary(&tok, tok);

  while (true) {
    if (tokenCompare(tok, "*")) {
      node = newBinary(ND_MUL, node, unary(&tok, tok->next));
      continue;
    }

    if (tokenCompare(tok, "/")) {
      node = newBinary(ND_DIV, node, unary(&tok, tok->next));
      continue;
    }

    break;
  }

  *rest = tok;
  return node;
}

Node *unary(Token **rest, Token *tok) {
  if (tokenCompare(tok, "+"))
    return unary(rest, tok->next);
  if (tokenCompare(tok, "-"))
    return newUnary(ND_NEG, unary(rest, tok->next));
  return primary(rest, tok);
}

Node *primary(Token **rest, Token *tok) {
  if (tokenCompare(tok, "(")) {
    Node *node = expr(&tok, tok->next);
    *rest = tokenSkip(tok, ")");
    return node;
  }

  if (tok->type == TK_INDET) {
    Node *node = newVarNode(*tok->idx);
    *rest = tok->next;
    return node;
  }

  if (tok->type == TK_NUM) {
    Node *node = newNum(tok->val);
    *rest = tok->next;
    return node;
  }

  errorTok(tok, "invalid expression");
  return NULL;
}

Node *parse(Token *tok) {
  Node head = {};
  Node *cur = &head;
  while (tok->type != TK_EOF) {
    cur->next = stmt(&tok, tok);
    cur = cur->next;
  }
  return head.next;
}