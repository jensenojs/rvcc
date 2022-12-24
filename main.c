#include "rvcc.h"

int main(int argc, char **argv) {
  if (argc != 2) {
    error("%s: Invalid number of arguments %d", argv[0], argc);
  }

  // parse argv[1] to generate a stream of tokens
  Token *tok = tokenize(argv[1]);

  // parse the stream of tokens
  Node *node = parse(tok);

  // codegen
  codegen(node);

  return 0;
}