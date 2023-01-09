/*
 *  Semantic analysis and code generation
 */

#include "rvcc.h"

static int StackDepth;
// 用于函数参数的寄存器们
static char *ArgReg[] = {"a0", "a1", "a2", "a3", "a4", "a5"};
static Function *CurrentFn;

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
  switch (node->nodeType) {
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
  switch (node->nodeType) {
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
  case ND_FUNCALL: {
    int nargs = 0;

    // Calculate the values of all parameters and push to stack
    for (Node *arg = node->args; arg; arg = arg->next) {
      genExpr(arg);
      push();
      nargs++;
    }

    assert(nargs <=
           6); // up to 6 registers to store the parameters of the function

    // pop the arguments to register, a0 -> args1, a1 -> args2 and so on
    for (int i = nargs - 1; i >= 0; i--) {
      pop(ArgReg[i]);
    }

    printf("\n  # 调用函数%s\n", node->funcName);
    printf("  call %s\n", node->funcName);
    return;
  }
  default:
    break;
  }

  // consider the priority, recurse to the right node first
  genExpr(node->right);
  push();
  genExpr(node->left);
  pop("a1");

  // generate what each binary tree node does in assembly code
  switch (node->nodeType) {
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
    printf("  # 判断是否a0%sa1\n", node->nodeType == ND_EQ ? "=" : "≠");
    printf("  xor a0, a0, a1\n");

    // then base on condition to set 1 or 0 to reg
    if (node->nodeType == ND_EQ)
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
  switch (node->nodeType) {
  case ND_RETURN:
    printf("# 返回语句\n");
    genExpr(node->left);
    // no condition jumps : jumps to .L.return segement
    // the way represent "j offset" is jal x0.
    printf("  # 跳转到.L.return.%s段\n", CurrentFn->name);
    printf("  j .L.return.%s\n", CurrentFn->name);
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
  // Calculate the stack space used by its variables for each function
  for (Function *fn = prog; fn; fn = fn->next) {
    int offSet = 0;

    // fetch all the variables
    for (Obj *var = fn->locals; var; var = var->next) {
      offSet += 8;
      var->offSet = -offSet;
    }

    fn->stackSize = alighTo(offSet, 16);
  }
}

// traversing the AST tree to generate assembly code
// code generation entry function, containing the base information of the code
// block
void codegen(Function *prog) {
  assignLVarOffset(prog);

  // Generate separate code for each function
  for (Function *fn = prog; fn; fn = fn->next) {
    printf("  # 定义全局%s段\n", fn->name);
    printf("  .global %s\n", fn->name);
    printf("\n# ===============%s程序开始===============\n", fn->name);
    printf("# %s段标签, 也是程序入口段\n", fn->name);
    printf("%s:\n", fn->name);
    CurrentFn = fn;
    // stack layout
    //-------------------------------// sp
    //              ra
    //-------------------------------// ra = sp-8
    //              fp
    //-------------------------------// fp = sp-16
    //           variable            //
    //-------------------------------// sp = sp-16-StackSize
    //     Expression evaluation
    //-------------------------------//

    // prologue

    // 将ra寄存器压栈,保存ra的值
    printf("  # 将ra寄存器压栈,保存ra的值, ra寄存器保存的是返回地址\n");
    printf("  addi sp, sp, -16\n");
    printf("  sd ra, 8(sp)\n");
    printf("  # 将fp压栈, fp属于“被调用者保存”的寄存器, 需要恢复原值\n");
    printf("  sd fp, 0(sp)\n");
    // write sp to fp
    printf("  # 将sp的值写入fp\n");
    printf("  mv fp, sp\n");

    // the offset is the size of the stack used by the actual variable
    printf("  # sp腾出StackSize大小的栈空间\n");
    printf("  addi sp, sp, -%d\n", fn->stackSize);

    printf("\n# ===============%s段主体===============\n", fn->name);
    genStmt(fn->body);
    assert(StackDepth == 0);

    // epilogue

    // return segment tag
    printf("\n# ===============%s段结束===============\n", fn->name);
    printf("# return段标签\n");
    printf(".L.return.%s:\n", fn->name);

    // write fp to sp
    printf("  # 将fp的值写回sp\n");
    printf("  mv sp, fp\n");
    // pop the stack of the earliest fp saved values and restore fp.
    printf("  # 将最早fp保存的值弹栈, 恢复fp和sp\n");
    printf("  ld fp, 0(sp)\n");
    // 将ra寄存器弹栈,恢复ra的值
    printf("  # 将ra寄存器弹栈,恢复ra的值\n");
    printf("  ld ra, 8(sp)\n");
    printf("  addi sp, sp, 16\n");
    printf("  # 返回a0值给系统调用\n");
    printf("  ret\n");
  }
}