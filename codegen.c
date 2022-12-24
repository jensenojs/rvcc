/*
 *  Semantic analysis and code generation
 */

#include "rvcc.h"

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
  case ND_EQ:
  case ND_NE:
    // first compare the two values to see if they are equal
    printf("  xor a0, a0, a1\n");

    // then base on condition to set 1 or 0 to reg
    if (node->type == ND_EQ)
      // Set if Equal to Zero (rd, rs1)
      // if x[rd] == 0, then write 1 to x[rs1] otherwise 0
      printf("  seqz a0, a0\n");
    else
      // Set if not Equal to Zero (rd, rs2)
      // if x[rd] != 0, then write 1 to x[rs1] otherwise 0
      printf("  snez a0, a0\n");
    return;
  case ND_LT:
    // Set if Less Than (rs, rs1, rs2)
    // compare x[rs1], x[rs2], if x[rs1] < x[rs2], then write 1 to rs, otherwise
    // 0
    printf("  slt a0, a0, a1\n");
    return;
  case ND_LE:
    // a0 <= a1 <==> a0 = a1 < a0, a0 = a1^1

    printf("  slt a0, a1, a0\n");
    printf("  xori a0, a0, 1\n");
    return;
  default:
    break;
  }

  error("invalid expression");
}

void codegen(Node *node) {
  printf("  .global main\n");
  printf("main:\n");

  genExpr(node);
  printf("  ret\n");

  assert(StackDepth == 0);
}