#include "rvcc.h"
#include <assert.h>
#include <stdlib.h>

Type *TyInt = &(Type){TY_INT};

bool isInteger(Type *ty) {
  assert(ty != NULL);
  return ty->kind == TY_INT;
}

Type *pointerTo(Type *base) {
  assert(base != NULL);
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_POINTER;
  ty->base = base;
  return ty;
}

Type *funcType(Type *returnType) {
  Type *ty = calloc(1, sizeof(Type));
  ty->kind = TY_FUNCTION;
  ty->returnType = returnType;
  return ty;
}

void addType(Node *node) {
  if (!node || node->dataType)
    return;

  addType(node->left);
  addType(node->right);
  addType(node->cond);
  addType(node->then);
  addType(node->els);
  addType(node->init);
  addType(node->inc);

  // Access all nodes and args in the linked-list to increase the type
  for (Node *n = node->body; n; n = n->next)
    addType(n);
  for (Node *n = node->args; n; n = n->next)
    addType(n);

  switch (node->nodeType) {
  // set the dataType of the node to the left child's
  case ND_ADD:
  case ND_SUB:
  case ND_MUL:
  case ND_DIV:
  case ND_NEG:
  case ND_ASSIGN:
    node->dataType = node->left->dataType;
    return;
  // set the dataType of the node to TyInt
  case ND_EQ:
  case ND_NE:
  case ND_LT:
  case ND_LE:
  case ND_NUM:
  case ND_FUNCALL:
    node->dataType = TyInt;
    return;
  case ND_VAR:
    node->dataType = node->var->dataType;
    return;
  // set the dataType of the node to pointer to left child
  case ND_ADDR:
    node->dataType = pointerTo(node->left->dataType);
    return;
  // If derefer is to pointer, then dataType is baseType, else ERROR
  case ND_DEREF:
    if (node->left->dataType->kind != TY_POINTER)
      errorTok(node->tok, "invalid pointer dereference");
    node->dataType = node->left->dataType->base;
    return;
  default:
    break;
  }
}

Type *copyType(Type *Ty) {
  Type *Ret = calloc(1, sizeof(Type));
  *Ret = *Ty;
  return Ret;
}
