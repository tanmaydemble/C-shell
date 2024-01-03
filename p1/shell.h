#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <assert.h>
#define MAX_INPUT_LENGTH 255
#define MAX_TOKENS 100

#ifndef SHELL_H
#define SHELL_H

int executePipeCommand(char* leftCommand[],
                       int leftCommandLength,
                       char* rightCommand[],
                       int rightCommandLength);

char** addNullTerminator(char* array[], int size);

void freeStringArray(char** array);

void executeSimpleCommand(char* command[], int command_length);

void executeRedirCommand(char* command[],
                         int command_length,
                         char* filename[],
                         int is_output_redirection);

bool isArrayEmpty(char** array);

int findStringInArray( char* searchString,
                       char* stringArray[],
                      int arraySize);

void splitStringArray(char** stringArray,
                      int arraySize,
                      int splitIndex,
                      char*** leftArray,
                      int leftSize,
                      char*** rightArray,
                      int rightSize);

int executeCommand(char* currentCommand[], int commandLength);

int getCurrentCommand(char* tokens[],
                       int startIdx,
                       int endIdx,
                       int* commandLength,
                       char*** currentCommand);

int* findSemicolonIndices(char* tokens[],
                          int token_count,
                          int* semicolonCount);

int findIndex( char* input,  char* array[], int size);

#endif