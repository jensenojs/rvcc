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
  // sd rs2, offset(rs1)  M[x[rs1] + sext(offset) = x[rs2][63: 0]
  printf("  sd a0, 0(sp)\n");
  StackDepth++;
}

// pop the value of the address pointed to by sp into the register
static void pop(char *reg) {
  // ld rd, offset(rs1) x[rd] = M[x[rs1] + sext(offset)][63:0]
  printf("  ld %s, 0(sp)\n", reg);
  printf("  addi sp, sp, 8\n");
  StackDepth--;
}

static int alighTo(int n, int align) {
  // (0, align] -> align
  return (n + align - 1) / align * align;
}
// offset is relative to fp
static void genAddr(Node *node) {
  if (node->type == ND_VAR) {
    printf("  addi a0, fp, %d\n",
           node->var->offSet); // fp is frame pointer, also named as x8, s0
    return;
  }
  error("not an lvalue");
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
  case ND_VAR:
    // calculate the address of the variable and store into a0
    genAddr(node);
    // the data stored in the a0 address is accessed and stored in the a0
    // address
    printf("  ld a0, 0(a0)\n");
    return;
  case ND_ASSIGN:
    // left
    genAddr(node->left);
    push();
    genExpr(node->right);
    pop("a1");
    printf("  sd a0, 0(a1)\n"); // assign
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

static void genStmt(Node *node) {
  switch (node->type) {
  case ND_RETURN:
    genExpr(node->left);
    // no condition jumps : jumps to .L.return segement
    // the way represent "j offset" is jal x0.
    printf("  j .L.return\n");
    return;
  case ND_EXPR_STMT:
    genExpr(node->left);
    return;
  case ND_BLOCK:
    for (Node *n = node->body; n; n = n->next) {
      genStmt(n);
    }
    return;
  default:
    break;
  }

  error("invalid expression");
}

// Calculate the offset from the variable's linked list
static void assignLVarOffset(Function *prog) {
  int offset = 0;
  for (Obj *var = prog->locals; var; var = var->next) {
    offset += 8;
    var->offSet = -offset;
    prog->stackSize = alighTo(offset, 16); //
  }
}

// code generation entry function, containing the base information of the code
// block
void codegen(Function *prog) {
  assignLVarOffset(prog);
  printf("  .global main\n");
  printf("main:\n");
  // stack layout
  //-------------------------------// sp
  //              fp                  fp = sp-8
  //-------------------------------// fp
  //           variable            //
  //-------------------------------// sp=sp-8-StackSize
  //     Expression evaluation
  //-------------------------------//

  // prologue

  // push fp onto the stack, preserving the value of fp
  printf("  addi sp, sp, -8\n");
  printf("  sd fp, 0(sp)\n");
  // write sp to fp
  printf("  mv fp, sp\n");

  // the offset is the size of the stack used by the actual variable
  printf("  addi sp, sp, -%d\n", prog->stackSize);

  genStmt(prog->body);
  assert(StackDepth == 0);

  // epilogue

  // return segment tag
  printf(".L.return:\n");

  // write fp to sp
  printf("  mv sp, fp\n");
  // pop the stack of the earliest fp saved values and restore fp.
  printf("  ld fp, 0(sp)\n");
  printf("  addi sp, sp, 8\n");

  printf("  ret\n");
}