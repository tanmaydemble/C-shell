#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define MAX_INPUT_LENGTH 255
#define MAX_TOKENS 100

#ifndef TOKENS_H
#define TOKENS_H

char *my_strdup(const char *s);
extern void tokenize(char *input, char *tokens[], int *token_count);

#endif