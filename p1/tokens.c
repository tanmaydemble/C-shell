#include "tokens.h"

/*
    Function to duplicate a string and return a pointer to it.
*/
char* my_strdup(const char* s) {
  size_t size = strlen(s) + 1;
  char* p = malloc(size);
  if (p != NULL) {
    memcpy(p, s, size);
  }
  return p;
}

/*
    Function to tokenize a string and store the tokens in an array
*/
void tokenize(char* input, char* tokens[], int* token_count) {
  char* token;
  int in_quote = 0;

  // Initialize an array to store the content within double quotes
  char quote_content[MAX_INPUT_LENGTH];
  int quote_index = 0;

  // Iterate through the input character by character
  for (int i = 0; input[i] != '\0'; i++) {
    if (input[i] == '"') {
      if (in_quote) {
        // If we were inside a quote, we have reached the end
        in_quote = 0;
        quote_content[quote_index] = '\0';  // Null-terminate the content
        tokens[(*token_count)++] =
            my_strdup(quote_content);  // Store the content within quotes as a
                                       // single token
        quote_index = 0;               // Reset the index
      } else {
        // If we were not inside a quote, we are entering one
        in_quote = 1;
      }
    } else if (in_quote) {
      // If inside a quote, add the character to the quote content
      quote_content[quote_index] = input[i];
      quote_index++;
    } else if (strchr(";<>()|", input[i]) != NULL) {
      // If a special char, treat it as a separate token
      char char_token[] = {input[i], '\0'};
      tokens[(*token_count)++] = my_strdup(char_token);
    } else if (input[i] != ' ' && input[i] != '\n' && input[i] != '\t') {
      // If not inside a quote, not whitespace, and not a semicolon, store the
      // character

      char* word = (char*)malloc(MAX_INPUT_LENGTH);
      int word_index = 0;
      while (input[i] != '\0' && input[i] != ' ' && input[i] != '\n' &&
             input[i] != '\t' && (strchr(";<>()|", input[i]) == NULL)) {
        word[word_index++] = input[i];
        i++;
      }

      word[word_index] = '\0';
      tokens[(*token_count)++] = word;

      while (input[i] != '\0' && input[i] != ' ' && input[i] != '\n' &&
             input[i] != '\t') {
        if (strchr(";<>()|", input[i]) != NULL) {
          char char_token[] = {input[i], '\0'};
          tokens[(*token_count)++] = my_strdup(char_token);
          i++;
        } else {
          i--;
          break;
        }
      }

      if (input[i] == '\0') {
        break;
      }
    }
  }
}