#include "shell.h"
#include "tokens.h"

int pipe_fds[2];

// to execute a pipe command
int executePipeCommand(char* leftCommand[],
                       int leftCommandLength,
                       char* rightCommand[],
                       int rightCommandLength) {
  int pipe_fds[2];  // the pipe system call creates two file descriptors in the
                    // 2-element array given as argument

  pipe(pipe_fds);  // returns 0 on success

  int read_fd = pipe_fds[0];   // index 0 is the "read end" of the pipe
  int write_fd = pipe_fds[1];  // index 1 is the "write end" of the pipt

  pid_t childA_pid = fork();

  if (childA_pid == 0) {  // in child A
    close(read_fd);       // close the other end of the pipe

    // replace stdout with the write end of the pipe
    if (close(1) == -1) {
      perror("Error closing stdout");
      exit(1);
    }

    dup(write_fd);
    executeCommand(leftCommand, leftCommandLength);
      
    // exit(0);
  } else if (childA_pid == -1) {
    perror("Error - fork failed A");
    exit(1);
  } else {
    close(write_fd);
    int A_status;
    waitpid(childA_pid, &A_status, WNOHANG);
  }

  pid_t childB_pid = fork();
  if (childB_pid == 0) {  // in child B
    close(write_fd);      // close the other end of the pipe

    // replace stdin with the read end of the pipe
    if (close(0) == -1) {
      perror("Error closing stdin");
      exit(1);
    }
    dup(read_fd);

    exit(0);
  } else if (childB_pid == -1) {
    perror("Error - fork failed B");
    exit(1);
  } else {
    close(read_fd);
    int B_status;
    waitpid(childB_pid, &B_status, WNOHANG);
  }
  close(write_fd);
  close(read_fd);

  return 0;
}

// to add a null terminator at the end of an array of command and its arguments
char** addNullTerminator(char* array[], int size) {
  char** newArray = (char**)malloc((size + 1) * sizeof(char*));
  if (newArray == NULL) {
    fprintf(stderr, "Memory allocation failed.\n");
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < size; i++) {
    newArray[i] = (char*)malloc(strlen(array[i]) + 1);
    if (newArray[i] == NULL) {
      fprintf(stderr, "Memory allocation failed.\n");
      exit(EXIT_FAILURE);
    }
    strcpy(newArray[i], array[i]);
  }

  newArray[size] = NULL;

  return newArray;
}

// to free the memory taken up by a string array
void freeStringArray(char** array) {
  if (array == NULL) {
    return;
  }
  for (int i = 0; array[i] != NULL; i++) {
    free(array[i]);
  }
  free(array);
}

// to execute a simple command
void executeSimpleCommand(char* command[], int command_length) {
  char** nullTerminatedCommand = addNullTerminator(command, command_length);
  if (strcmp(nullTerminatedCommand[0], "help") == 0) {
    printf("The built in commands are as follows:\n");
    printf("1. exit   : helps exit from the  shell\n");
    printf(
        "2. cd     : changes the current working directory of the shell to the "
        "path specified as the argument\n");
    printf("3. source : helps execute a script\n");
    printf(
        "4. prev   : prints the previous command line and executes it again, "
        "without becoming the new command line\n");
    printf(
        "5. help   : explains all the built-in commands available in the "
        "shell\n");
  } else if (strcmp(nullTerminatedCommand[0], "prev") == 0) {
    // status_code = 2;
    // use_prev = true;
    // status_char = 'p';
  } else if (strcmp(nullTerminatedCommand[0], "source") == 0) {
    FILE* file;
    char line[MAX_INPUT_LENGTH];
    char* filename = nullTerminatedCommand[1];
    file = fopen(filename, "r");
    if (file == NULL) {
      perror("File open failed");
      // return 1;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
      char* tokens[MAX_TOKENS];
      int token_count = 0;
      int semicolonCount = 0;
      tokenize(line, tokens, &token_count);
      int* semicolonIndices =
          findSemicolonIndices(tokens, token_count, &semicolonCount);
      int startIdx = 0;
      int endIdx = semicolonIndices[0];
      char** currentCommand = NULL;
      int commandLength = 0;
      for (int i = 0; i < semicolonCount; i++) {
        getCurrentCommand(tokens, startIdx, endIdx, &commandLength,
                          &currentCommand);
        executeCommand(currentCommand, commandLength);

        if ((i + 1) != semicolonCount) {
          startIdx = semicolonIndices[i] + 1;
          endIdx = semicolonIndices[i + 1];
        }

        for (int i = 0; i < commandLength; i++) {
          free(currentCommand[i]);
        }
      }
      free(currentCommand);
      free(semicolonIndices);
      for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
      }
    }
    fclose(file);
  } else if (strcmp(nullTerminatedCommand[0], "cd") == 0) {
    // change the cd of this child
    chdir(nullTerminatedCommand[1]);
    char cwd[MAX_INPUT_LENGTH];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
      close(pipe_fds[0]);  // close the read end of the pipe
      write(pipe_fds[1], cwd, strlen(cwd));
      close(pipe_fds[1]);  // close the write end of the pipe
    } else {
      perror("getcwd");
    }
  } else if (execvp(nullTerminatedCommand[0], nullTerminatedCommand) == -1) {
    char error_message[100];
    snprintf(error_message, sizeof(error_message), "[%s]: command not found",
             command[0]);
    perror(error_message);
    exit(EXIT_FAILURE);
  } else {
    // printf("Exited simple commdn\n");
    // status_char = 'c';
  }
  freeStringArray(nullTerminatedCommand);
  // return status_char;
}

// to execute a redirection command
void executeRedirCommand(char* command[],
                         int command_length,
                         char* filename[],
                         int is_output_redirection) {
  pid_t child_pid;
  int status;

  // Create a child process.
  if ((child_pid = fork()) == 0) {
    // Child process

    if (is_output_redirection) {
      // Output redirection
      int fd = open(filename[0], O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd == -1) {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDOUT_FILENO);
      close(fd);
    } else {
      // Input redirection
      int fd = open(filename[0], O_RDONLY);
      if (fd == -1) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
      }
      dup2(fd, STDIN_FILENO);
      close(fd);
    }
    //  Execute the command in the child process.
    executeSimpleCommand(command, command_length);
  } else if (child_pid > 0) {
    // Parent process
    wait(NULL);
  } else {
    perror("Fork failed");
    exit(EXIT_FAILURE);
  }
}

// to check whether an array of strings is empty
bool isArrayEmpty(char** array) {
  bool isEmpty = true;
  for (int i = 0; i < (sizeof(array) / sizeof(array[0])); i++) {
    if (strlen(array[i]) > 0) {
      isEmpty = false;
      break;
    }
  }
  return isEmpty;
}

// returns the position of the first occurance of the given string in an array
// of strings if the element is not found then it returns -1
int findStringInArray(char* searchString, char* stringArray[], int arraySize) {
  for (int i = 0; i < arraySize; i++) {
    if (strcmp(searchString, stringArray[i]) == 0) {
      return i;
    }
  }
  return -1;
}

// to split the array of strings into two at a given location
void splitStringArray(char** stringArray,
                      int arraySize,
                      int splitIndex,
                      char*** leftArray,
                      int leftSize,
                      char*** rightArray,
                      int rightSize) {
  if (splitIndex < 0 || splitIndex > arraySize) {
    // Invalid split index
    leftSize = 0;
    rightSize = 0;
    *leftArray = NULL;
    *rightArray = NULL;
    return;
  }

  *leftArray = (char**)malloc((leftSize) * sizeof(char*));
  *rightArray = (char**)malloc((rightSize) * sizeof(char*));

  for (int i = 0; i < splitIndex; i++) {
    (*leftArray)[i] = my_strdup(stringArray[i]);
  }

  for (int i = 0; i < rightSize; i++) {
    (*rightArray)[i] = my_strdup(stringArray[splitIndex + i + 1]);
  }
}

// to fork a child and execute a command
int executeCommand(char* currentCommand[], int commandLength) {
  // int fd[2];
  // pipe(fd);
  pipe(pipe_fds);
  pid_t child_pid;
  child_pid = fork();

  if (child_pid == -1) {
    perror("Fork failed");
    return 0;
  }

  if (child_pid == 0) {
    if (isArrayEmpty(currentCommand)) {
    } else if (findStringInArray("|", currentCommand, commandLength) > -1) {
      int pipeLocation = findStringInArray("|", currentCommand, commandLength);
      char** leftCommand;
      int leftCommandLength = pipeLocation;
      char** rightCommand;
      int rightCommandLength = commandLength - pipeLocation - 1;
      splitStringArray(currentCommand, commandLength, pipeLocation,
                       &leftCommand, leftCommandLength, &rightCommand,
                       rightCommandLength);
      executePipeCommand(leftCommand, leftCommandLength, rightCommand,
                         rightCommandLength);

      for (int i = 0; i < leftCommandLength; i++) {
        free(leftCommand[i]);
      }
      free(leftCommand);
      for (int i = 0; i < rightCommandLength; i++) {
        free(rightCommand[i]);
      }
      free(rightCommand);
    } else if (findStringInArray(">", currentCommand, commandLength) > -1) {
      int redirectionLocation =
          findStringInArray(">", currentCommand, commandLength);
      if (redirectionLocation == 0 ||
          redirectionLocation == commandLength - 1) {
        fprintf(stderr, "Invalid redirection location.\n");
        exit(EXIT_FAILURE);
      } else {
        char** leftCommand;
        char** filename;
        int leftCommandLength = redirectionLocation;
        int filenameLength =
            1;  // will always be one as the file is a single token
        splitStringArray(currentCommand, commandLength, redirectionLocation,
                         &leftCommand, leftCommandLength, &filename,
                         filenameLength);
        executeRedirCommand(leftCommand, leftCommandLength, filename, 1);
        for (int i = 0; i < leftCommandLength + 1; i++) {
          free(leftCommand[i]);
        }
        free(leftCommand);
        for (int i = 0; i < filenameLength; i++) {
          free(filename[i]);
        }
        free(filename);
      }
    } else if (findStringInArray("<", currentCommand, commandLength) > -1) {
      int redirectionLocation =
          findStringInArray("<", currentCommand, commandLength);
      if (redirectionLocation == 0 ||
          redirectionLocation == commandLength - 1) {
        fprintf(stderr, "Invalid redirection location.\n");
        exit(EXIT_FAILURE);
      } else {
        char** leftCommand;
        char** filename;
        int leftCommandLength = redirectionLocation;
        int filenameLength =
            1;  // will always be one as the file is a single token
        splitStringArray(currentCommand, commandLength, redirectionLocation,
                         &leftCommand, leftCommandLength, &filename,
                         filenameLength);
        executeRedirCommand(leftCommand, leftCommandLength, filename, 0);
        for (int i = 0; i < leftCommandLength; i++) {
          free(leftCommand[i]);
        }
        free(leftCommand);
        for (int i = 0; i < filenameLength; i++) {
          free(filename[i]);
        }
        free(filename);
      }
    } else {
      executeSimpleCommand(currentCommand, commandLength);
      // exit(0);
    }
  } else {
    wait(NULL);
    if (strcmp(currentCommand[0], "cd") == 0) {
      close(pipe_fds[1]); //close the write end of the pipe
      char cwd[MAX_INPUT_LENGTH];
      int readLength = read(pipe_fds[0], cwd, MAX_INPUT_LENGTH);
      cwd[readLength] = 0;
      close(pipe_fds[0]);
      chdir(cwd);
    }
    return 0;
  }
}

// to create a subarray from the tokens array which contains the current command
// to process
int getCurrentCommand(char* tokens[],
                      int startIdx,
                      int endIdx,
                      int* commandLength,
                      char*** currentCommand) {
  *commandLength = endIdx - startIdx;
  if (*(commandLength) == 0)
    return 0;
  *currentCommand = (char**)malloc((*commandLength) * sizeof(char*));
  if (startIdx == 0) {
    for (int i = startIdx; i < (*commandLength); i++) {
      (*currentCommand)[i] =
          (char*)malloc(sizeof(char) * (strlen(tokens[i]) + 1));
      assert((*currentCommand)[i] != NULL);

      strcpy((*currentCommand)[i], tokens[i]);
    }
  } else {
    for (int i = startIdx; i < ((*commandLength) + startIdx); i++) {
      (*currentCommand)[i - startIdx] =
          (char*)malloc(sizeof(char) * (strlen(tokens[i]) + 1));
      assert((*currentCommand)[i - startIdx] != NULL);

      strcpy((*currentCommand)[i - startIdx], tokens[i]);
    }
  }
  return (*(commandLength));
}

// to identify the elements in the tokenize array that are ; and return an array
// of such indices
int* findSemicolonIndices(char* tokens[],
                          int token_count,
                          int* semicolonCount) {
  int* semicolonIndices = (int*)malloc(sizeof(int));
  *semicolonCount = 0;
  for (int i = 0; i < token_count; i++) {
    if (strcmp(tokens[i], ";") == 0) {
      semicolonIndices[(*semicolonCount)] = i;
      (*semicolonCount)++;
      semicolonIndices = (int*)realloc(semicolonIndices,
                                       ((*semicolonCount) + 1) * sizeof(int));
    }
  }
  // we also need to assume that the tokens array has a semicolon at the very
  // end of the array as an additional element so that we can properly get our
  // last command
  (*semicolonCount)++;
  semicolonIndices[(*semicolonCount) - 1] = token_count;

  return semicolonIndices;
}

// to find the first occurance of a particular string in a array of strings
// if no such occurance exists then return -1
int findIndex(char* input, char* array[], int size) {
  for (int i = 0; i < size; i++) {
    if (strcmp(input, array[i]) == 0) {
      return i;  // Found a match; return the index
    }
  }
  return -1;  // No match found; return -1
}

int main() {
  printf("Welcome to mini-shell.\n");
  char* prev_tokens[MAX_TOKENS];
  int prev_token_count = 0;

  bool use_prev = false;
  int status_code = 0;
  char status_char = 'c';
  while (1) {
    char input[MAX_INPUT_LENGTH];
    char* tokens[MAX_TOKENS];
    int token_count = 0;
    int semicolonCount = 0;

    if (status_char == 'p') {
      use_prev = true;
    }

    // Read a single line from standard input
    printf("shell $ ");
    fflush(stdout);
    if (fgets(input, MAX_INPUT_LENGTH, stdin) != NULL) {
      // Call the tokenize function to extract and store tokens in the arra
      tokenize(input, tokens, &token_count);
    } else {
      fflush(stdout);
      printf("Bye bye.");
      for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
      }
      for (int i = 0; i < prev_token_count; i++) {
        free(prev_tokens[i]);
      }
      return 0;
    }

    if (strcmp(tokens[0], "prev") == 0) {
      int* semicolonIndices =
          findSemicolonIndices(prev_tokens, prev_token_count, &semicolonCount);

      int startIdx = 0;
      int endIdx = semicolonIndices[0];
      char** currentCommand = NULL;
      int commandLength = 0;
      for (int i = 0; i < semicolonCount; i++) {
        getCurrentCommand(prev_tokens, startIdx, endIdx, &commandLength,
                          &currentCommand);
        executeCommand(currentCommand, commandLength);

        for (int i = 0; i < commandLength; i++) {
          free(currentCommand[i]);
        }
        free(currentCommand);
      }
      free(semicolonIndices);

      for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
      }
    } else if (strcmp(tokens[0], "exit") == 0) {
      fflush(stdout);
      printf("Bye bye.");
      for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
      }
      for (int i = 0; i < prev_token_count; i++) {
        free(prev_tokens[i]);
      }
      return 0;
    } else {
      prev_token_count = token_count;
      for (int i = 0; i < token_count; i++) {
        prev_tokens[i] = my_strdup(tokens[i]);
      }

      int* semicolonIndices =
          findSemicolonIndices(tokens, token_count, &semicolonCount);

      int startIdx = 0;
      int endIdx = semicolonIndices[0];
      char** currentCommand = NULL;
      int commandLength = 0;
      for (int i = 0; i < semicolonCount; i++) {
        if (getCurrentCommand(tokens, startIdx, endIdx, &commandLength,
                              &currentCommand) == 0) {
          for (int i = 0; i < commandLength; i++) {
            free(currentCommand[i]);
          }
          break;
        } else {
          executeCommand(currentCommand, commandLength);

          if ((i + 1) != semicolonCount) {
            startIdx = semicolonIndices[i] + 1;
            endIdx = semicolonIndices[i + 1];
          }

          for (int i = 0; i < commandLength; i++) {
            free(currentCommand[i]);
          }
          free(currentCommand);
        }
      }
      for (int i = 0; i < token_count; i++) {
        free(tokens[i]);
      }
      free(semicolonIndices);
    }
  }

  for (int i = 0; i < MAX_TOKENS; i++) {
    free(prev_tokens[i]);
  }

  return 0;
}