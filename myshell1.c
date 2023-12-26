/********************************************************************************************
This is a template for an assignment on writing a custom Shell.
Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.
Though the use of any extra functions is not recommended, students may use new functions if they need to,
but that should not make the code unnecessarily complex to read.
Students should keep names of declared variables (and any new functions) self-explanatory,
and add proper comments for every logical step.
Students need to be careful while forking a new process (no unnecessary process creations)
or while inserting the single handler code (should be added at the correct places).
Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp,
as you do not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_COMMANDS 100

int parseInput(char* Input) {
    // This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).

    int del; // stands for delimiter
    char* Inputcpy;
    Inputcpy = strdup(Input); // duplicating the command so that Input doesn't change

    if (strstr(Inputcpy, "&&") != NULL) {
        del = 1;
    } else if (strstr(Inputcpy, "##") != NULL) {
        del = 2;
    } else if (strstr(Inputcpy, ">") != NULL) {
        del = 3;
    } else if (strstr(Inputcpy, "|") != NULL) {
        del = 4;
    } else {
        del = 5;
    }

    return del;
}
// Function to split the input command into individual arguments and trim spaces
char** separate_and_trim_spaces(char* Input) {

    size_t size = 50;
    char** commands = (char**)malloc(size * sizeof(char*));
    char delimiters[] = " ";
    char* token = NULL;
    int i = 0;
    char* rest = strdup(Input); // duplicating the command so that Input doesn't change

    while ((token = strsep(&rest, delimiters)) != NULL) {
        // Trim leading and trailing spaces from the token
        char* trimmed_token = token;
        while (*trimmed_token == ' ' || *trimmed_token == '\t') {
            trimmed_token++;
        }
        int len = strlen(trimmed_token);
        while (len > 0 && (trimmed_token[len - 1] == ' ' || trimmed_token[len - 1] == '\t')) {
            trimmed_token[--len] = '\0';
        }
        if (strlen(trimmed_token) > 0) {
            commands[i] = trimmed_token;
            i++;
        }
    }
    commands[i] = NULL; // Null-terminate the command array
    return commands;
}

// Function to handle the 'cd' command
void execute_cd(char** command) {
    int ans;
    if (strcmp(command[0], "cd") == 0) {
        if (command[1] == NULL) {
            // No directory specified, go to the home directory
            ans = chdir(getenv("HOME"));
        } else if (command[2] == NULL) {
            // Change to the specified directory
            ans = chdir(command[1]);
        } else {
            printf("Shell: Incorrect command\n");
        }
    } else if (strcmp(command[0], "cd ..") == 0) {
        // Handle the 'cd ..' command
        if (command[1] == NULL) {
            ans = chdir("..");
        } else {
            printf("Shell: Incorrect command\n");
        }
    }
    if (ans == -1) {
        printf("Shell: Incorrect command\n");
    }
    return;
}

void executeCommand(char* Input) {
    // Function to execute a command
    char** command = separate_and_trim_spaces(Input);
    if (strcmp(command[0], "cd") == 0) {
        execute_cd(command);
    } else {
        // This function will fork a new process to execute a command
        int rc = fork();
        if (rc < 0) {
            // fork failed
            perror("Fork failed");
            exit(1);
        } else if (rc == 0) {
            // Child Process
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            if (command[1] == NULL) {
                char* argsToExec[] = {command[0], command[1]};
                int status = execvp(argsToExec[0], argsToExec);
                if (status == -1) {
                    perror("Command execution failed");
                    exit(1);
                }
            } else {
                char* argsToExec[] = {command[0], command[1], NULL};
                int status = execvp(argsToExec[0], argsToExec);
                if (status == -1) {
                    perror("Command execution failed");
                    exit(1);
                }
            }
            exit(1);
        } else {
            // Parent process
            int rc_wait = wait(NULL); // Makes Deterministic that Child will run before parent
            if (rc_wait == -1) {
                perror("Wait failed");
                exit(1);
            }
        }
    }
}


void executeParallelCommands(char* commandString) {
    int numCommands = 0, i = 0;

    // Count the number of parallel commands separated by &&
    while (commandString[i] != '\0') {
        if (commandString[i] == '&' && commandString[i + 1] == '&') {
            numCommands++;
            i++;
        }
        i++;
    }

    char* delimiter = "&&";
    char* delimiterPointer = strstr(commandString, delimiter); // Points to the first occurrence of the delimiter
    char** commands = (char**)malloc((numCommands + 2) * sizeof(char*));

    i = 0;
    while (i <= numCommands) {
        commands[i] = (char*)malloc(150 * sizeof(char));
        i++;
    }

    i = 0;
    char* cmdPtr = commandString;

    while (delimiterPointer != NULL) {
        delimiterPointer[0] = '\0'; // Replace & with \0 to get the first command before the delimiter
        char* tempCommand = strdup(cmdPtr); // Store the individual command
        commands[i] = tempCommand; // Store that command in commands array
        cmdPtr[0] = ' '; // Delete the '\0' so we can move forward in the string
        commandString = cmdPtr + 2; // Move to the next command after the delimiter
        delimiterPointer = strstr(commandString, delimiter);
        i++;
    }

    commands[i] = commandString;

    for (i = 0; i <= numCommands; i++) {
        int childPid = fork(); // Create a child process

        if (childPid < 0) {
            // Fork failed; exit
            perror("Fork failed");
            exit(1);
        } else if (childPid == 0) {
            // Child process
            // Re-enable signals for child processes
            signal(SIGINT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);

            char** commandArgs = separate_and_trim_spaces(commands[i]);

            if (commandArgs == NULL) {
                perror("Invalid command");
                exit(0);
            }

            int execResult = execvp(commandArgs[0], commandArgs);

            if (execResult < 0) {
                // execvp error
                perror("Command execution failed");
                exit(1);
            }
        } else {
            // Parent process
            int child_status;
            waitpid(childPid, &child_status, 0);
            
            // Check if the child process exited with an error
            if (WIFEXITED(child_status) && WEXITSTATUS(child_status) != 0) {
                // If the child process exited with a non-zero status, don't execute the next command
                break;
            }
        }
    }
}


void executeSequentialCommands(char* cmd) {
    int i, cmd_count = 0;
    // Counting the number of sequential commands to be executed separated by ##
    for (i = 0; cmd[i] != '\0'; i++) {
        if (cmd[i] == '#' && cmd[i + 1] == '#') {
            cmd_count++;
            i++;
        }
    }

    // Now separate all individual commands by delimiter ## and store it in cmd_container for future execution
    char** cmd_container = (char**)malloc((cmd_count + 2) * sizeof(char*));

    i = 0;
    while (i <= cmd_count) {
        cmd_container[i] = (char*)malloc((50) * sizeof(char));
        i++;
    }

    i = 0;
    char* delimeter = "##";
    char* cmd_ptr = strstr(cmd, delimeter); // return the pointer to the presence of the first delimiter

    while (cmd_ptr != NULL) {
        cmd_ptr[0] = '\0'; // replace ## with \0 to get the first command before the delimiter
        char* temp = strdup(cmd); // temp stores the individual command
        cmd_container[i] = temp; // stores that command in temp in command_container
        i++;
        cmd_ptr[0] = ' '; // Replace the '\0' so we can move forward in the string
        cmd = cmd_ptr + 2;
        cmd_ptr = strstr(cmd, delimeter); // move to next command after delimiter
    }

    cmd_container[i] = cmd;

    i = 0;
    // Now Execute each command present in cmd_container as a single command by passing it to executeCommand(copy_cmd);
    // executeCommand(copy_cmd) this will fork a child process and make the parent wait until the child is over, after that
    // the parent also ends and control is back to this function and 'i' is incremented for the next command to be executed
    while (i <= cmd_count) {
        char* copy_cmd = strdup(cmd_container[i]);
        executeCommand(copy_cmd);
        i++;
    }
}

void executeCommandRedirection(char* cmd) {
    // This function will run a single command with output redirected to an output file specified by the user
    char* command = strsep(&cmd, ">");
    /*strsep will return a pointer to the 1st part of the command before > , which is our actual command to
    be executed, and now, cmd will be pointing to the part which is just after delimiter '>', which is the output file name*/

    char* filename = cmd;
    while (*filename == ' ') // Removing whitespaces if present
    {
        filename++;
    }

    int rc = fork(); // Forking child process
    if (rc < 0) // In case fork fails due to a lack of memory
    {
        exit(0);
    } else if (rc == 0) // Child process
    {
        close(STDOUT_FILENO); // Closing the STDOUT

        // Opening the file where we want to write the output
        open(strsep(&filename, "\n"), O_CREAT | O_RDWR, S_IRWXU);

        char** arguments = separate_and_trim_spaces(command);

        int retval = execvp(arguments[0], arguments);

        if (retval < 0) { // execvp error code
            printf("Shell: Incorrect command\n");
            exit(1);
        }
    } else {
        int waiting_rc = wait(NULL); // Waiting for the child to be over
    }
}

void executePipelines(char* commands) {
    char* command_tokens[MAX_COMMANDS];
    int command_count = 0;

    // Split the input into commands using a pipe as the delimiter
    char* command = strtok(commands, "|");
    while (command != NULL && command_count < MAX_COMMANDS) {
        command_tokens[command_count++] = command;
        command = strtok(NULL, "|");
    }

    if (command_count == 0) {
        // No commands found after the pipe
        fprintf(stderr, "Shell: Incorrect command\n");
        return;
    }

    int pipes[2]; // File descriptors for the pipe
    int input_fd = 0; // File descriptor for input from the previous command

    for (int i = 0; i < command_count; i++) {
        if (i < command_count - 1) {
            // Create a pipe for commands in the middle of the pipeline
            if (pipe(pipes) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }

        pid_t pid = fork();

        if (pid == 0) { // Child process
            if (i < command_count - 1) {
                // Redirect stdout to the write end of the pipe
                dup2(pipes[1], STDOUT_FILENO);
            }

            if (i > 0) {
                // Redirect stdin to the read end of the previous pipe
                dup2(input_fd, STDIN_FILENO);
                close(input_fd); // Close the previous pipe's read end
            }

            // Close all pipe file descriptors in the child
            close(pipes[0]);
            close(pipes[1]);

            // Execute the command
            executeCommand(command_tokens[i]);
            exit(EXIT_SUCCESS);
        } else if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else { // Parent process
            // Close the write end of the pipe if it's not the last command
            if (i < command_count - 1) {
                close(pipes[1]);
            }

            // Close the previous pipe's read end
            if (i > 0) {
                close(input_fd);
            }

            // Update the input file descriptor for the next command
            input_fd = pipes[0];

            // Wait for the child process to complete
            waitpid(pid, NULL, 0);
        }
    }
}

int main() {

    while (1) {

        signal(SIGINT, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);

        // Print the prompt in the format - currentWorkingDirectory$
        char currentWorkingDirectory[1024];
        if (getcwd(currentWorkingDirectory, sizeof(currentWorkingDirectory)) != NULL) {
            printf("%s$", currentWorkingDirectory);
        } else {
            printf("ERROR!!! in calling getcwd()\n");
        }

        // Accept input with 'getline()'
        char* input = NULL;
        size_t len = 0;
        ssize_t read;
        read = getline(&input, &len, stdin);

        if (read == -1) {
            // If getline fails to read, print error message and exit
            perror("getline");
            exit(EXIT_FAILURE);
        }

        // Remove the newline character from the input
        input[strcspn(input, "\n")] = '\0';

        // Determine the type of command based on delimiters (&&, ##, >, |, or none)
        int del = parseInput(input);

        if (del == 5) {
            // If no delimiter is found, execute a single command
            executeCommand(input);
        } else if (del == 1) {
            // If '&&' is found, execute commands in parallel
            executeParallelCommands(input);
        } else if (del == 2) {
            // If '##' is found, execute commands sequentially
            executeSequentialCommands(input);
        } else if (del == 3) {
            // If '>' is found, execute a command with output redirection
            executeCommandRedirection(input);
        } else if (del == 4) {
            // If '|' is found, execute commands in a pipeline
            executePipelines(input);
        }

        free(input); // Free the dynamically allocated input buffer
    }

    return 0;
}

