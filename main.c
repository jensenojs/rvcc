#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "%s: invalid number of arguments\n", argv[0]);
    return 1;
  }

  char *p = argv[1];

  printf("  .global main\n");
  printf("main:\n");

  // num (op num) (op num)
  printf("  li a0, %ld\n", strtol(p, &p, 10));

  // resolve (op num)
  while (*p) {

    if (*p == '+') {
      ++p;
      printf("  addi a0, a0, %ld\n", strtol(p, &p, 10));
      continue;
    } else if (*p == '-') {
      // imm in addi is signed, so subtraction represents as rd = rs1 + (-imm)
      // riscv doesn't have subi
      ++p;
      printf("  addi a0, a0, -%ld\n", strtol(p, &p, 10));
      continue;
    }

    fprintf(stderr, "unsupported operator");
    exit(1);
  }

  // ret is alias for "jalr x0, x1, 0"
  printf("  ret\n");

  return 0;
}