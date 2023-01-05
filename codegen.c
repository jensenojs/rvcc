/*
 *  Semantic analysis and code generation
 */

#include "rvcc.h"
#include <stdio.h>

static int StackDepth;

static void genExpr(Node *node);

// count for the number of code block
static int count() {
  static int I = 1;
  return I++;
}

// push the value of a0 onto the stack
static void push() {
  //  sp is the stack pointer, the stack grows downwards, under 64
  //  bits, 8 bytes is a unit, so sp-8
  printf("  # 压栈, 将a0的值存入栈顶\n");
  printf("  addi sp, sp, -8\n");
  // sd rs2, offset(rs1)  M[x[rs1] + sext(offset) = x[rs2][63: 0]
  printf("  sd a0, 0(sp)\n");
  StackDepth++;
}

// pop the value of the address pointed to by sp into the register
static void pop(char *reg) {
  // ld rd, offset(rs1) x[rd] = M[x[rs1] + sext(offset)][63:0]
  printf("  # 弹栈, 将栈顶的值存入%s\n", reg);
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
  switch (node->type) {
  case ND_VAR:
    printf("  # 获取变量%s的栈内地址为%d(fp)\n", node->var->name,
           node->var->offSet);
    printf("  addi a0, fp, %d\n",
           node->var->offSet); // fp is frame pointer, also named as x8, s0
    return;
  case ND_DEREF:
    genExpr(node->left);
    return;
  default:
    break;
  }
  errorTok(node->tok, "not an lvalue");
}

static void genExpr(Node *node) {

  // load data to a0 register
  switch (node->type) {
  case ND_NUM:
    printf("  # 将%d加载到a0中\n", node->val);
    printf("  li a0, %d\n", node->val);
    return;
  case ND_NEG:
    genExpr(node->left);
    printf("  # 对a0值进行取反\n");
    printf("  neg a0, a0\n");
    return;
  case ND_VAR:
    // calculate the address of the variable and store into a0
    genAddr(node);
    // the data stored in the a0 address is accessed and stored in the a0
    // address
    printf("  # 读取a0中存放的地址, 得到的值存入a0\n");
    printf("  ld a0, 0(a0)\n");
    return;
  case ND_ASSIGN:
    // left
    genAddr(node->left);
    push();
    genExpr(node->right);
    pop("a1");
    printf("  # 将a0的值, 写入到a1中存放的地址\n");
    printf("  sd a0, 0(a1)\n"); // assign
    return;
  case ND_DEREF:
    genExpr(node->left);
    printf("  # 读取a0中存放的地址, 得到的值存入a0\n");
    printf("  ld a0, 0(a0)\n");
    return;
  case ND_ADDR:
    genAddr(node->left);
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
    printf("  # a0+a1, 结果写入a0\n");
    printf("  add a0, a0, a1\n");
    return;
  case ND_SUB:
    printf("  # a0-a1, 结果写入a0\n");
    printf("  sub a0, a0, a1\n");
    return;
  case ND_MUL:
    printf("  # a0*a1, 结果写入a0\n");
    printf("  mul a0, a0, a1\n");
    return;
  case ND_DIV:
    printf("  # a0÷a1, 结果写入a0\n");
    printf("  div a0, a0, a1\n");
    return;
  case ND_EQ:
  case ND_NE:
    // first compare the two values to see if they are equal
    printf("  # 判断是否a0%sa1\n", node->type == ND_EQ ? "=" : "≠");
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
    printf("  # 判断a0<a1\n");
    printf("  slt a0, a0, a1\n");
    return;
  case ND_LE:
    // a0 <= a1 <==> a0 = a1 < a0, a0 = a1^1
    printf("  # 判断是否a0≤a1\n");
    printf("  slt a0, a1, a0\n");
    printf("  xori a0, a0, 1\n");
    return;
  default:
    break;
  }

  errorTok(node->tok, "invalid expression");
}

static void genStmt(Node *node) {
  switch (node->type) {
  case ND_RETURN:
    printf("# 返回语句\n");
    genExpr(node->left);
    // no condition jumps : jumps to .L.return segement
    // the way represent "j offset" is jal x0.
    printf("  # 跳转到.L.return段\n");
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
  case ND_IF: {
    int cnt = count();
    printf("\n# =====分支语句%d==============\n", cnt);
    printf("\n# Cond表达式%d\n", cnt);
    genExpr(node->cond);

    // Check whether the result is 0. If it is 0, go to the else tag
    printf("  # 若a0为0, 则跳转到分支%d的.L.else.%d段\n", cnt, cnt);
    printf("  beqz a0, .L.else.%d\n", cnt);

    printf("\n# Then语句%d\n", cnt);
    genStmt(node->then);

    printf("  # 跳转到分支%d的.L.end.%d段\n", cnt, cnt);
    printf("  j .L.end.%d\n", cnt);

    // Generate tag with or without the else statement
    printf("\n# Else语句%d\n", cnt);
    printf("# 分支%d的.L.else.%d段标签\n", cnt, cnt);
    printf(".L.else.%d:\n", cnt);
    if (node->els)
      genStmt(node->els);

    printf("\n# 分支%d的.L.end.%d段标签\n", cnt, cnt);
    printf(".L.end.%d:\n", cnt);

    return;
  }
  case ND_LOOP: { // for or while loop
    int cnt = count();
    printf("\n# ===============循环语句%d===============\n", cnt);

    if (node->init) {
      printf("\n# Init语句%d\n", cnt);
      genStmt(node->init);
    }

    printf("\n# 循环%d的.L.begin.%d段标签\n", cnt, cnt);
    printf(".L.begin.%d:\n", cnt); // printf loop header tag

    printf("# Cond表达式%d\n", cnt);
    if (node->cond) {
      genExpr(node->cond);
      printf("  # 若a0为0, 则跳转到循环%d的.L.end.%d段\n", cnt, cnt);
      printf("  beqz a0, .L.end.%d\n", cnt); // Determine if the result is 0, if
                                             // it is 0 then jump to the end tag
    }

    printf("\n# Then语句%d\n", cnt);
    genStmt(node->then); // Generate loop body statements

    if (node->inc) { // handling loop increment statements
      printf("\n# Inc语句%d\n", cnt);
      genExpr(node->inc);
    }

    printf("  # 跳转到循环%d的.L.begin.%d段\n", cnt, cnt);
    printf("  j .L.begin.%d\n", cnt);
    // 输出循环尾部标签
    printf("\n# 循环%d的.L.end.%d段标签\n", cnt, cnt);
    printf(".L.end.%d:\n", cnt);

    return;
  }
  default:
    break;
  }

  errorTok(node->tok, "invalid Statement");
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

// traversing the AST tree to generate assembly code
// code generation entry function, containing the base information of the code
// block
void codegen(Function *prog) {
  assignLVarOffset(prog);
  printf("  # 定义全局main段\n");
  printf("  .global main\n");
  printf("\n# ===============程序开始===============\n");
  printf("# main段标签, 也是程序入口段\n");
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
  printf("  # 将fp压栈, fp属于“被调用者保存”的寄存器, 需要恢复原值, "
         "这里不适合用push和pop\n");
  printf("  addi sp, sp, -8\n");
  printf("  sd fp, 0(sp)\n");
  // write sp to fp
  printf("  # 将sp的值写入fp\n");
  printf("  mv fp, sp\n");

  // the offset is the size of the stack used by the actual variable
  printf("  # sp腾出StackSize大小的栈空间\n");
  printf("  addi sp, sp, -%d\n", prog->stackSize);

  printf("\n# ===============程序主体===============\n");
  genStmt(prog->body);
  assert(StackDepth == 0);

  // epilogue

  // return segment tag
  printf("\n# ===============程序结束===============\n");
  printf("# return段标签\n");
  printf(".L.return:\n");

  // write fp to sp
  printf("  # 将fp的值写回sp\n");
  printf("  mv sp, fp\n");
  // pop the stack of the earliest fp saved values and restore fp.
  printf("  # 将最早fp保存的值弹栈, 恢复fp和sp\n");
  printf("  ld fp, 0(sp)\n");
  printf("  addi sp, sp, 8\n");
  printf("  # 返回a0值给系统调用\n");
  printf("  ret\n");
}