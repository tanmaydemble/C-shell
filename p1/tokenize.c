#include "tokens.h"

int main() {
  char input[MAX_INPUT_LENGTH];
  char* tokens[MAX_TOKENS];
  int token_count = 0;

  // Read a single line from standard input
  if (fgets(input, MAX_INPUT_LENGTH, stdin) != NULL) {
    // Call the tokenize function to extract and store tokens in the array
    tokenize(input, tokens, &token_count);
  }

  for (int i = 0; i < token_count; i++) {
    printf("%s\n", tokens[i]);
    free(tokens[i]);
  }

  return 0;
}