#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <glob.h>
#include "helper.c"

// prototypes of all functions
void setHomeDir();
void checkArgs();
void setStdIn(char **argv);
void greet();
void inputLoop();
void parseCommand(char *command);
void exitCommand();
void exitCommandWrap(char **tokens, size_t numOfTokens);
void pwdCommand(char **tokens, size_t numOfTokens);
void cdCommand(char **tokens, size_t numOfTokens);
void executeCommand(char **tokens, size_t numOfTokens);
char* findProgramPath(const char *program);
void executeProgram(const char *programPath, char **args, const int *stdInFd, const int *stdOutFd, bool isFinal, int *pipeFd, bool *pipeSet);
ssize_t replaceWithHomeDir(char **tokens, size_t numOfTokens);
char** getFilenames(const char *filePath, size_t *numOfFilenames);
char** wildcardFilenames(char **tokens, size_t *numOfTokens);
bool isRegularFile(const char *path);
bool isExecutableFile(const char *path);
ssize_t replaceWithProgramPath(char **tokens, size_t numOfTokens);
ssize_t builtIn(char **tokens, size_t numOfTokens);
void singleProgram(char **tokens, size_t numOfTokens);
void multiProgram(char **tokens, size_t numOfTokens);
ssize_t checkCommandSyntax(char **tokens, size_t numOfTokens);
ssize_t checkProgramSyntax(char **tokens, size_t numOfTokens);
char** getProgramArgs(char **tokens, size_t numOfTokens, size_t *numOfArgs);
char** getFilenamesExt(const char *filePath, size_t *pnumOfFilenames);

// define enumeration for the mode of the shell
typedef enum mode {
	INTERACTIVE = 1,
	BATCH
} mode;

// define global variable for the mode of the shell
mode shellMode = INTERACTIVE;

// define global variable for exit_status of last command
ssize_t exit_status = 0;

// define global variable for the home directory
char *homeDir = NULL;

// this program accepts either 0 or 1 arguments
// if no arguments are given, then the program will run in interactive mode
// if 1 argument is given (file name for stdin), then the program will run in batch mode
int main(int argc, char **argv) {
	// set shellMode to argc
	shellMode = argc;

	// set stdout buffer to NULL
	setbuf(stdout, NULL);

	// check the arguments of this program
	checkArgs();

	// set stdin to either the default or the input file
	setStdIn(argv);
	
	// if INTERACTIVE, greet the user
	greet();

	// set homeDir to the home directory
	setHomeDir();
	
	// at this point, stdin is set correctly
	// so we can use the same input loop for both interactive and batch modes
	inputLoop();
}

// function that sets the homeDir variable to the home directory
void setHomeDir() {
	// if the homeDir variable is not NULL, then free it
	if (homeDir != NULL) {
		homeDir = Free(homeDir);
	}

	// get the home directory from the environment variable HOME
	char *home = getenv("HOME");

	// if home is NULL, then there was an error
	if (home == NULL || strlen(home) == 0) {
		perror("getenv");
		exit(EXIT_FAILURE);
	}

	// copy the home directory to homeDir using strdup()
	homeDir = strdup(home);

	// if homeDir is NULL, then there was an error
	if (homeDir == NULL) {
		perror("strdup");
		exit(EXIT_FAILURE);
	}

	// if homeDir has a "/" at the end, then remove it
	size_t homeDirLen = strlen(homeDir);
	if (homeDir[homeDirLen - 1] == '/') {
		homeDir[homeDirLen - 1] = '\0';
	}
}

// input loop for both interactive and batch modes
void inputLoop() {
	// while loop that runs until the program exits
	// dynamically allocate the buffer if it is too small if the user inputs a long line
	// use read() to read character by character from stdin and save each character to a dynamic buffer
	// if the user inputs a newline, then the buffer is complete and the command can be executed
	// if EOF is read, then call exitCommand() to exit the program
	while (true) {
		// if exit_status is 0 and it is INTERACTIVE, print "mysh> " otherwise print "!mysh> "
		if (shellMode == INTERACTIVE) {
			if (exit_status == 0) {
				write(STDOUT_FILENO, "mysh> ", 6);
			} else {
				write(STDOUT_FILENO, "!mysh> ", 7);
			}
		}
		// read input
		char *buffer = malloc(sizeof(char) * 101);
		size_t size = 101;
		size_t i = 0;
		char c;
		ssize_t readStatus;
		while ((readStatus = read(STDIN_FILENO, &c, sizeof(char))) == sizeof(char)) {
			// if buffer is too small, increase size
			if (i == size) {
				size += 100;
				buffer = realloc(buffer, sizeof(char) * size);
			}
			// if newline is read, then the buffer is complete
			if (c == '\n') {
				buffer[i] = '\0';
				break;
			}
			// increase i and add character to buffer
			buffer[i] = c;
			i++;
		}

		// if EOF is read, then check if the buffer is empty
		if (readStatus == 0) {
			// if buffer is full, increase size
			if (i == size) {
				size += 1;
				buffer = realloc(buffer, sizeof(char) * size);
			}

			// EOF is read, so the buffer is complete
			buffer[i] = '\0';

			// if buffer is empty, then exit the program successfully
			if (strlen(buffer) == 0) {
				buffer = Free(buffer);
				exitCommand();
			}
		}
		// if readStatus is -1, then there was an error
		if (readStatus == -1) {
			buffer = Free(buffer);
			perror("read");
			exit(EXIT_FAILURE);
		}

		// now the command is complete and can be parsed
		parseCommand(buffer);
	}
}

// function that parses command
void parseCommand(char *command) {
	// if command is NULL or empty, then return
	if (command == NULL || strlen(command) == 0) {
		command = Free(command);
		exit_status = 0;
		return;
	}

	// tokenize the command with whitespace as the delimiter and special tokens
	size_t numOfTokens;
	char **tokens = strTokenize(command, " \t\n\v\f\r", &numOfTokens, "|><");

	// free the command buffer because it is no longer needed
	command = Free(command);

	// if tokens is NULL, then set exit status to 0 and return
	if (tokens == NULL) {
		exit_status = 0;
		return;
	}

	// replace the "~/" with home directory
	ssize_t result = replaceWithHomeDir(tokens, numOfTokens);

	// if result is -1, then set exit status to 1
	if (result == -1) {
		exit_status = 1;
		tokens = freeStrTokens(tokens, numOfTokens);
		return;
	}

	// call wildcardFilenames() to replace any wildcard file paths with sequence of filenames
	tokens = wildcardFilenames(tokens, &numOfTokens);

	// if tokens is NULL, then set exit status to 1 and return
	if (tokens == NULL) {
		exit_status = 1;
		return;
	}

	// check for syntax errors
	result = checkCommandSyntax(tokens, numOfTokens);

	// if result is -1, then set exit status to 1
	if (result == -1) {
		exit_status = 1;
		tokens = freeStrTokens(tokens, numOfTokens);
		return;
	}

	// replace the program name with the program path
	result = replaceWithProgramPath(tokens, numOfTokens);

	// if result is -1, then set exit status to 1
	if (result == -1) {
		exit_status = 1;
		tokens = freeStrTokens(tokens, numOfTokens);
		return;
	}

	// at this point, the command is parsed and ready to be executed
	// call executeCommand() to execute the command
	executeCommand(tokens, numOfTokens);

	// free the memory allocated
	tokens = freeStrTokens(tokens, numOfTokens);
}

// function that executes the command
void executeCommand(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, then set exit status to 0 and return
	if (tokens == NULL || numOfTokens == 0) {
		exit_status = 0;
		return;
	}

	// if the command does not contain a pipe, then call singleProgram()
	// iterate over tokens and use strcmp to check if the token is a pipe
	bool pipeFound = false;
	for (size_t i = 0; i < numOfTokens; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			pipeFound = true;
			break;
		}
	}

	// if pipe is found, then call multiProgram()
	if (pipeFound) {
		multiProgram(tokens, numOfTokens);
	} else {
		singleProgram(tokens, numOfTokens);
	}
}

// function that checks the arguments of this program
void checkArgs() {
	// if more than 1 argument is given, then the program will exit with an error
	if (shellMode != INTERACTIVE && shellMode != BATCH) {
		write(STDERR_FILENO, "Usage: './mysh' or './mysh myscript.sh'\n", 40);
		exit(EXIT_FAILURE);
	}
}

// function that sets stdin correctly to either the terminal or the input file
void setStdIn(char **argv) {
	// if 1 argument is given, set the file as stdin
	// if no arguments are given, set stdin to the terminal
	// use posix functions open, read, write, close
	if (shellMode == BATCH) {
		// close stdin
		if (close(STDIN_FILENO) == -1) {
			perror("close");
			exit(EXIT_FAILURE);
		}
		
		// open file
		int fd = open(argv[1], O_RDONLY);
		if (fd == -1) {
			perror("open");
			exit(EXIT_FAILURE);
		}
	}
}

// function that greets the user in interactive mode
void greet() {
	if (shellMode == INTERACTIVE) {
		write(STDOUT_FILENO, "Welcome to MySH 3.0\n", 20);
	}
}

// function that exits the program successfully
void exitCommand() {
	// free all global variables
	homeDir = Free(homeDir);

	// if INTERACTIVE, prints "mysh: exiting" to stdout and exits successfully
	if (shellMode == INTERACTIVE) {
		write(STDOUT_FILENO, "mysh: exiting\n", 14);
	}
	exit(EXIT_SUCCESS);
}

// wrapper function for exitCommand that takes in arguments
void exitCommandWrap(char **tokens, size_t numOfTokens) {
	// extract program args
	size_t numOfArgs = 0;
	char **args = getProgramArgs(tokens, numOfTokens, &numOfArgs);

	// if any arguments are given, then print an error message to stderr and set exit status to 1
	if (numOfArgs > 1) {
		write(STDERR_FILENO, "exit: too many arguments\n", 25);
		exit_status = 1;
		args = freeArrayOfStrings(args, numOfArgs);
		return;
	}

	// otherwise call exitCommand()
	args = freeArrayOfStrings(args, numOfArgs);
	tokens = freeStrTokens(tokens, numOfTokens);
	exitCommand();
}

// function that prints the current working directory
// example: /current/path/subdir/subsubdir
void pwdCommand(char **tokens, size_t numOfTokens) {
	// extract program args
	size_t numOfArgs = 0;
	char **args = getProgramArgs(tokens, numOfTokens, &numOfArgs);

	// if any arguments are given, then print an error message to stderr and set exit status to 1
	if (numOfArgs > 1) {
		write(STDERR_FILENO, "pwd: too many arguments\n", 24);
		exit_status = 1;
		args = freeArrayOfStrings(args, numOfArgs);
		return;
	}

	// free the args because they are not needed
	args = freeArrayOfStrings(args, numOfArgs);

	// extract stdout redirection file path if it exists from the tokens
	const char *stdOutFile = NULL;
	for (size_t i = 0; i < numOfTokens; i++) {
		if (strcmp(tokens[i], ">") == 0) {
			stdOutFile = tokens[i + 1];
			break;
		}
	}

	// declare a file descriptor for stdout
	int stdOutFd = STDOUT_FILENO;

	if (stdOutFile != NULL) {
		// if the file exists, then open it with O_WRONLY | O_CREAT | O_TRUNC
		// mode is 0640
		// if the file does not exist, then create it
		// if the file exists, then truncate it
		// if the file cannot be opened, then print an error message to stderr and set exit status to 1
		stdOutFd = open(stdOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);
		if (stdOutFd == -1) {
			perror("open");
			exit_status = 1;
			return;
		}
	}

	// using getcwd() to get the current working directory
	// use a dynamic buffer to store the path
	// if getcwd returns NULL, increase buffer size and try again until it works
	// print the path
	char *buffer = malloc(sizeof(char) * 101);
	size_t size = 101;
	while (getcwd(buffer, size) == NULL) {
		size += 100;
		buffer = Free(buffer);
		buffer = malloc(sizeof(char) * size);
	}

	// increase buffer size by 1 if the buffer is full
	if (strlen(buffer) + 1 == size) {
		buffer = realloc(buffer, sizeof(char) * (size + 1));
	}

	// concat a newline character to the end of the buffer
	strcat(buffer, "\n");

	// print to stdout and free the buffer
	write(stdOutFd, buffer, strlen(buffer));
	buffer = Free(buffer);

	// close the file descriptor if it was opened
	if (stdOutFile != NULL && close(stdOutFd) == -1) {
		perror("close");
		exit_status = 1;
		return;
	}

	// set exit status to 0 on success
	exit_status = 0;
}

// function that changes the current directory
void cdCommand(char **tokens, size_t numOfTokens) {
	// extract program args
	size_t numOfArgs = 0;
	char **args = getProgramArgs(tokens, numOfTokens, &numOfArgs);

	// if more than 1 argument is given, then print an error message to stderr and set exit status to 1
	if (numOfArgs > 2) {
		write(STDERR_FILENO, "cd: too many arguments\n", 23);
		exit_status = 1;
		args = freeArrayOfStrings(args, numOfArgs);
		return;
	}

	// if no arguments are given, then change to home directory
	if (numOfArgs == 1) {
		if (chdir(homeDir) == -1) {
			perror("chdir");
			exit_status = 1;
		} else {
			exit_status = 0;
		}
	}

	// if 1 argument is given, then change to the given directory
	else {
		if (chdir(args[1]) == -1) {
			perror("chdir");
			exit_status = 1;
		} else {
			exit_status = 0;
		}
	}

	// free the args because they are not needed
	args = freeArrayOfStrings(args, numOfArgs);
}

// function that returns the full path of a given program or it returns the same program if it is already a path
char* findProgramPath(const char *program) {
	// if program is NULL, then return NULL
	if (program == NULL) {
		return NULL;
	}

	// if program is a path, then return a new allocated program
	if (strchr(program, '/') != NULL) {
		if (!isExecutableFile(program)) {
			return NULL;
		}
		return strdup(program);
	}

	// otherwise, we know program is just a file name so traverse the list of directories
	// define list of directories to search
	char *dirs[] = {
		"/usr/local/sbin/", 
		"/usr/local/bin/", 
		"/usr/sbin/", 
		"/usr/bin/", 
		"/sbin/", 
		"/bin/"
	};
	size_t numOfDirs = 6;

	// check each directory in the list from beginning to end in order to find the file
	// if the file is found, then return the full path of the file
	// if the file is not found, then return NULL
	// use stat() to check if the file exists in the directory
	// use a dynamic buffer to store the full path of the file if it is found
	// if stat returns -1, then the file does not exist in the directory
	// if stat returns 0, then the file exists in the directory
	for (size_t i = 0; i < numOfDirs; i++) {
		// concatenate the directory and the program
		char *fullPath = malloc(sizeof(char) * (strlen(dirs[i]) + strlen(program) + 1));
		strcpy(fullPath, dirs[i]);
		strcat(fullPath, program);

		// check if the file exists in the directory
		if (isExecutableFile(fullPath)) {
			return fullPath;
		}

		// free the buffer
		fullPath = Free(fullPath);
	}

	// if the file is not found, then return NULL
	return NULL;
}

// function that executes a program and collects its exit status. Args must be NULL terminated
void executeProgram(const char *programPath, char **args, const int *stdInFd, const int *stdOutFd, bool isFinal, int *pipeFd, bool *pipeSet) {
	// initialize variables for wait()
	int status = 0;
	pid_t gotPid = 0;
	bool abnormalExit = false;

	// fork a child process
	pid_t pid = fork();
	switch (pid) {
		case -1:
			// if fork returns -1, then perror and set exit status to 1
			perror("fork");
			exit_status = 1;
			break;

		default:
			// if fork returns a positive number, then we are in the parent process
			// if this is the final child process, then wait for all child processes to finish
			// set the global exit_status to the exit status of the last child process that was forked
			if (!isFinal) {
				break;
			}
			// close both ends of the pipe because the parent process will not be writing or reading from the pipe
			if (pipeFd != NULL && pipeSet != NULL) {
				if (close(pipeFd[0]) == -1 || close(pipeFd[1]) == -1) {
					perror("close");
					exit_status = 1;
				}
			}
			while (true) {
				// get the pid of the child process that finished
				gotPid = wait(&status);

				// if wait returns -1 or 0, there is an error or there are no more child processes
				if (gotPid == -1 || gotPid == 0) {
					if (errno == ECHILD) {
						break;
					}
					// if wait returns -1 and errno is not ECHILD, then perror and set exit status to 1
					perror("wait");
					write(STDERR_FILENO, "Please restart the shell\n", 25);
					exit_status = 1;
					break;
				}

				// if wait returns a positive number, then there is a child process that finished
				// so check if the child process exited normally
				// if at least 1 child process did not exit normally, then the exit status is 1 regardless if other child processes exited normally
				if (abnormalExit == false && WIFEXITED(status) == false) {
					abnormalExit = true;
					write(STDERR_FILENO, "child process did not exit normally\n", 36);
					exit_status = 1;
				}

				// if all child processes exited normally, then the final exit status is the exit status of the last child process that was forked
				if (abnormalExit == false && gotPid == pid && WIFEXITED(status) == true) {
					exit_status = WEXITSTATUS(status);
				}
			}
			break;

		case 0:
			// if fork returns 0, then we are in the child process
			// if stdInFd is not NULL, then redirect stdin to the file descriptor
			if (stdInFd != NULL) {
				if (dup2(*stdInFd, STDIN_FILENO) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}

			// if stdOutFd is not NULL, then redirect stdout to the file descriptor
			if (stdOutFd != NULL) {
				if (dup2(*stdOutFd, STDOUT_FILENO) == -1) {
					perror("dup2");
					exit(EXIT_FAILURE);
				}
			}

			// whenever pipeSet is false, close the pipe file descriptor that matches the index
			// if pipeSet is true, then the pipe file descriptor is used by the child process
			// so do not close the pipe file descriptor
			if (pipeFd != NULL && pipeSet != NULL && pipeSet[0] == false) {
				if (close(pipeFd[0]) == -1) {
					perror("close");
					exit(EXIT_FAILURE);
				}
			}
			if (pipeFd != NULL && pipeSet != NULL && pipeSet[1] == false) {
				if (close(pipeFd[1]) == -1) {
					perror("close");
					exit(EXIT_FAILURE);
				}
			}
			// use execv() to execute the program
			// execv only returns when there is an error
			// so then perror and exit the child process with exit status 1
			execv(programPath, args);
			perror("execv");
			exit(EXIT_FAILURE);
	}
}

// function that replaces tokens that begin with "~/" with the home directory
ssize_t replaceWithHomeDir(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, then return -1
	if (tokens == NULL || numOfTokens == 0) {
		return -1;
	}

	// iterate through the tokens and if a token begins with "~/" then replace the first occurrence of "~" with the home directory
	// make sure you check if the token is NULL or if the token is not long enough to contain "~/" and other checks
	// if the token is not NULL and it is long enough to contain "~/" then check if the first 2 characters are "~/"
	// if the first 2 characters are "~/", then replace the first occurrence of "~" using strReplace()
	for (size_t i = 0; i < numOfTokens; i++) {
		if (tokens[i] != NULL && strlen(tokens[i]) >= 2 && strncmp(tokens[i], "~/", 2) == 0) {
			char *temp = strReplace(tokens[i], "~", homeDir, 1);
			if (temp == NULL) {
				exit_status = 1;
				return -1;
			}
			tokens[i] = Free(tokens[i]);
			tokens[i] = temp;
		}
	}

	// return 0 if successful
	return 0;
}

// function that returns a list of filenames that match the pattern in the file path
char** getFilenames(const char *filePath, size_t *numOfFilenames) {
	// if filePath is NULL, then return NULL
	if (filePath == NULL || strlen(filePath) == 0 || numOfFilenames == NULL) {
		return NULL;
	}

	// initialize numOfFilenames to 0
	*numOfFilenames = 0;

	// if the filePath does not contain a "*" then return NULL
	if (strchr(filePath, '*') == NULL) {
		return NULL;
	}

	// if the filePath contains more than 1 "*" in the whole filePath then return NULL
	if (strchr(filePath, '*') != strrchr(filePath, '*')) {
		return NULL;
	}
	
	// the filePath is like "/dir1/dir2/dir3/a*t.txt" or "/dir1/a*" or "/dir1/*a" or "/dir1/*" or "/dir1/a*t" or "*a" or "*"
	// if it is just the filename without "/", then search the current directory
	// first check whether the path is a directory or a file. If it is a directory, then return NULL
	// if the last character is a "/" that means the path is a directory so return NULL
	size_t filePathLen = strlen(filePath);
	if (filePath[filePathLen - 1] == '/') {
		return NULL;
	}

	// extract the directory path (dirPath) and the file pattern (filePattern) from the filePath
	// if there is no directory path, then set dirPath to "./" so that the full path becomes "./a*t.txt"
	// otherwise if filePath is "/dir1/dir2/dir3/a*" then dirPath is "/dir1/dir2/dir3/" and filePattern is "a*"
	char *dirPath = NULL;
	char *filePattern = NULL;
	if (strchr(filePath, '/') == NULL) {
		dirPath = strdup("./");
		filePattern = strdup(filePath);
	}
	else {
		// if there is a directory path, then extract the directory path and the file pattern
		// first find the last occurrence of "/". No need to check for NULL because we already checked for "/" above
		char *lastSlash = strrchr(filePath, '/');

		// if filePath is "/dir1/a*" then copy "/dir1/" to dirPath and "a*" to filePattern
		// allocate memory for dirPath and copy the directory path to dirPath
		dirPath = malloc(lastSlash - filePath + 2);
		if (dirPath == NULL) {
			perror("malloc");
			exit_status = 1;
			return NULL;
		}

		// copy the directory path to dirPath
		strncpy(dirPath, filePath, lastSlash - filePath + 1);
		dirPath[lastSlash - filePath + 1] = '\0';

		// allocate memory for filePattern and copy the file pattern to filePattern
		filePattern = strdup(lastSlash + 1);
		if (filePattern == NULL) {
			perror("strdup");
			exit_status = 1;
			dirPath = Free(dirPath);
			return NULL;
		}
	}

	// at this point dirPath and filePattern are set
	// if file pattern does not contain a "*" then return NULL
	if (strchr(filePattern, '*') == NULL) {
		dirPath = Free(dirPath);
		filePattern = Free(filePattern);
		return NULL;
	}

	// open the directory for reading filenames
	DIR *dir = opendir(dirPath);

	if (dir == NULL) {
		dirPath = Free(dirPath);
		filePattern = Free(filePattern);
		return NULL;
	}

	// first count the number of filenames that match the file pattern
	// if the file pattern is "*" then all filenames match
	// if the file pattern is "a*" then the filename must start with "a"
	// if the file pattern is "*a" then the filename must end with "a"
	// if the file pattern is "a*t" then the filename must start with "a" and end with "t"
	// if the file pattern is "ab*t.txt" then the filename must start with "ab" and end with "t.txt"
	// initialize variables to count the number of filenames that match the file pattern
	size_t numOfCharsBeforeAst = strcspn(filePattern, "*");
	size_t numOfCharsAfterAst = strlen(filePattern) - numOfCharsBeforeAst - 1;

	// initialize a variable to store a pointer to the current entry in the directory
	struct dirent *entry = NULL;

	// set errno to 0 before calling readdir() to check for errors
	errno = 0;
	while ((entry = readdir(dir)) != NULL) {
		// extract filename from the current entry
		char *filename = entry->d_name;

		// if filename is NULL or filename is empty or begins with ".", then skip to the next filename
		if (filename == NULL || strlen(filename) == 0 || filename[0] == '.') {
			continue;
		}

		// concatenate the directory path and the filename to get the full path
		char *fullFilePath = malloc(strlen(dirPath) + strlen(filename) + 1);

		if (fullFilePath == NULL) {
			perror("malloc");
			exit_status = 1;
			dirPath = Free(dirPath);
			filePattern = Free(filePattern);
			closedir(dir);
			return NULL;
		}

		// copy the directory path to fullFilePath and then concatenate the filename
		strcpy(fullFilePath, dirPath);
		strcat(fullFilePath, filename);

		// check if the full path is a directory or a file
		// if it is not a regular file, then skip to the next filename
		if (isRegularFile(fullFilePath) == false) {
			fullFilePath = Free(fullFilePath);
			continue;
		}

		// free the memory allocated for fullFilePath
		fullFilePath = Free(fullFilePath);

		// check if filename starts with the sequence of characters before the "*" in the file pattern
		if (strncmp(filename, filePattern, numOfCharsBeforeAst) == 0) {
			// check if filename ends with the sequence of characters after the "*" in the file pattern
			if (strncmp(filename + strlen(filename) - numOfCharsAfterAst, filePattern + numOfCharsBeforeAst + 1, numOfCharsAfterAst) == 0) {
				(*numOfFilenames)++;
			}
		}
	}

	// close the directory
	closedir(dir);

	// check if errno was changed by readdir() to indicate an error
	if (errno != 0) {
		perror("readdir");
		exit_status = 1;
		dirPath = Free(dirPath);
		filePattern = Free(filePattern);
		return NULL;
	}

	// if there are no filenames that match the file pattern, then return NULL and set numOfFilenames to 0
	if (*numOfFilenames == 0) {
		dirPath = Free(dirPath);
		filePattern = Free(filePattern);
		return NULL;
	}

	// at this point, we know that there are numOfFilenames that match the file pattern
	// allocate memory for the array of filenames, which will be terminated with a NULL pointer
	char **filenames = malloc((*numOfFilenames + 1) * sizeof(char *));

	if (filenames == NULL) {
		perror("malloc");
		exit_status = 1;
		dirPath = Free(dirPath);
		filePattern = Free(filePattern);
		return NULL;
	}

	// set the last element of the array to NULL
	filenames[*numOfFilenames] = NULL;

	// repeat the process above and allocate each filename to the array
	// make sure to calculate size of the filename before allocating memory for it
	// set the last element of the array to NULL
	// open the directory for reading filenames
	dir = opendir(dirPath);

	if (dir == NULL) {
		*numOfFilenames = 0;
		dirPath = Free(dirPath);
		filePattern = Free(filePattern);
		filenames = freeArrayOfStrings(filenames, 0);
		return NULL;
	}

	// initialize a variable to store the index of the current filename in the array
	size_t i = 0;

	// set errno to 0 before calling readdir() to check for errors
	errno = 0;

	while ((entry = readdir(dir)) != NULL) {
		// extract filename from the current entry
		char *filename = entry->d_name;

		// if filename is NULL or filename is empty or begins with ".", then skip to the next filename
		if (filename == NULL || strlen(filename) == 0 || filename[0] == '.') {
			continue;
		}

		// concatenate the directory path and the filename to get the full path
		char *fullFilePath = malloc(strlen(dirPath) + strlen(filename) + 1);

		if (fullFilePath == NULL) {
			perror("malloc");
			exit_status = 1;
			dirPath = Free(dirPath);
			filePattern = Free(filePattern);
			closedir(dir);
			return NULL;
		}

		// copy the directory path to fullFilePath and then concatenate the filename
		strcpy(fullFilePath, dirPath);
		strcat(fullFilePath, filename);

		// check if the full path is a directory or a file
		// if it is not a regular file, then skip to the next filename
		if (isRegularFile(fullFilePath) == false) {
			fullFilePath = Free(fullFilePath);
			continue;
		}

		// free the memory allocated for fullFilePath
		fullFilePath = Free(fullFilePath);

		// check if filename starts with the sequence of characters before the "*" in the file pattern
		if (strncmp(filename, filePattern, numOfCharsBeforeAst) == 0) {
			// check if filename ends with the sequence of characters after the "*" in the file pattern
			if (strncmp(filename + strlen(filename) - numOfCharsAfterAst, filePattern + numOfCharsBeforeAst + 1, numOfCharsAfterAst) == 0) {
				// allocate memory for the filename and copy the filename to the array
				filenames[i] = strdup(filename);

				if (filenames[i] == NULL) {
					perror("strdup");
					exit_status = 1;
					dirPath = Free(dirPath);
					filePattern = Free(filePattern);
					filenames = freeArrayOfStrings(filenames, i);
					closedir(dir);
					return NULL;
				}

				// increment the index of the current filename in the array
				i++;
			}
		}
	}

	// close the directory
	closedir(dir);

	// check if errno was changed by readdir() to indicate an error
	if (errno != 0) {
		perror("readdir");
		exit_status = 1;
		dirPath = Free(dirPath);
		filePattern = Free(filePattern);
		filenames = freeArrayOfStrings(filenames, i);
		return NULL;
	}

	// free memory allocated for dirPath and filePattern
	dirPath = Free(dirPath);
	filePattern = Free(filePattern);

	// return the array of filenames
	return filenames;
}

// function that expands tokens that contain wildcards for a sequence of file names
// returns new allocated tokens array and updates numOfTokens.
// note: the input tokens array is freed, so it should not be used after calling this function
char** wildcardFilenames(char **tokens, size_t *numOfTokens) {
	// if tokens is NULL or numOfTokens is NULL, then return NULL
	if (tokens == NULL || numOfTokens == NULL) {
		return NULL;
	}

	// if there are no tokens, then return NULL
	if (*numOfTokens == 0) {
		return NULL;
	}

	// tokens can be like ["./foo", "*a", "b*", "*"] or 
	// ["./baz", "*", "<", "/usr/bin/file.txt", ">", "file.txt", "|", "grep", "a*", "*b"]
	// basically we need to expand the tokens that contain wildcards and replace them with the filenames that match the wildcard pattern
	// in ["./foo*", "*", "<", "/usr/bin/file.txt", ">", "file.txt*", "|", "grep", "a*", "*b"], we need to expand only
	// the tokens that contain wildcards, which are ["./foo*", "*", "file.txt*", "a*", "*b"]
	size_t numOfTokensCopy = *numOfTokens;
	size_t numOfFilenames = 0;
	for (size_t i = 0; i < numOfTokensCopy; i++) {
		// call getFilenames() to get the filenames that match the wildcard pattern if there is one
		char **filenames = getFilenamesExt(tokens[i], &numOfFilenames);

		// if there are no filenames that match the wildcard pattern, then skip to the next token
		if (filenames == NULL) {
			continue;
		}

		// at this point, we know that there are filenames that match the wildcard pattern
		// save old token and set the returned filenames to the token
		char *oldToken = tokens[i];

		// call strCombineTokens() to combine the filenames into a single string
		// and set the token to the returned string
		tokens[i] = strCombineTokens(filenames, numOfFilenames, " ");

		// free the array of filenames
		filenames = freeArrayOfStrings(filenames, numOfFilenames);

		// if the returned string is NULL, then set it back to the old token
		if (tokens[i] == NULL) {
			tokens[i] = oldToken;
			continue;
		}

		// free the old token
		oldToken = Free(oldToken);
	}

	// combine the tokens array into a single string separated by a single space as the delimiter
	// then tokenize the string using the same delimiter to get the new tokens array
	// free the old tokens array
	char *tokensStr = strCombineTokens(tokens, numOfTokensCopy, " ");
	tokens = freeStrTokens(tokens, numOfTokensCopy);
	tokens = strTokenize(tokensStr, " ", numOfTokens, "");

	// free the string
	tokensStr = Free(tokensStr);

	// return the new tokens array
	return tokens;
}

// function that returns whether given path points to a regular file
bool isRegularFile(const char *path) {
	// if path is NULL, then return false
	if (path == NULL || strlen(path) == 0) {
		return false;
	}

	// use stat() to check if the path points to a regular file
	struct stat st;

	// if stat() returns -1, then return false
	if (stat(path, &st) == -1) {
		return false;
	}

	// use S_ISREG() to check if the path points to a regular file
	return S_ISREG(st.st_mode);
}

// function that returns whether given path points to an executable file
bool isExecutableFile(const char *path) {
	// if path is NULL or empty, then return false
	if (path == NULL || strlen(path) == 0) {
		return false;
	}

	// first, use isRegularFile() to check if the path points to a regular file
	// if not, return false
	if (!isRegularFile(path)) {
		return false;
	}

	// use stat() to check if the path points to an executable file
	struct stat st;

	// if stat() returns -1, then return false
	if (stat(path, &st) == -1) {
		return false;
	}

	// use S_IXUSR, S_IXGRP, S_IXOTH to check if the path points to an executable file
	return (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH);
}

// function that replaces bare names with full path of the program
ssize_t replaceWithProgramPath(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, then return
	if (tokens == NULL || numOfTokens == 0) {
		return -1;
	}

	// tokens are like ["ls", ">", "file.txt", "|", "grep", "a*", "*b"]
	// the command names are the first token in each subcommand separated by pipes
	// so including the first token, the token following a pipe is a command name
	// if the command name is a built-in command using strcasecmp ("cd", "pwd", "exit"), then continue to the next command name
	// if the command name is not a built-in command, then call findProgramPath() to get the full path of the program
	// if the full path of the program is NULL, then perror() and set exit status to 1 and return -1
	// if the full path is not NULL, then use isExecutableFile() to check if the full path points to an executable file
	// if it does not, then perror() and set exit status to 1 and return -1
	// if it does, then replace the command name with the full path that is returned by findProgramPath()
	bool commandNotFound = false;

	// iterate over the tokens array
	for (size_t i = 0; i < numOfTokens; i++) {
		// if this is not the first token and it is not following a pipe, then continue to the next token
		// because the first token is a command name, and the token following a pipe is a command name
		if (i > 0 && strcmp(tokens[i - 1], "|") != 0) {
			continue;
		}

		// if this is a built-in command, then continue to the next token
		if (strcasecmp(tokens[i], "cd") == 0 || strcasecmp(tokens[i], "pwd") == 0 || strcasecmp(tokens[i], "exit") == 0) {
			continue;
		}

		// at this point, we know that this is not a built-in command so it must be a program name
		// call findProgramPath() to get the full path of the program
		char *fullPath = findProgramPath(tokens[i]);

		// if the full path is NULL, then print error and set exit status to 1 and return -1
		if (fullPath == NULL) {
			commandNotFound = true;
			// malloc space for the error message that is format "command not found: %s\n"
			char *error = malloc(sizeof(char) * (19 + strlen(tokens[i]) + 2));
			strcpy(error, "command not found: ");
			strcat(error, tokens[i]);
			strcat(error, "\n");
			write(STDERR_FILENO, error, strlen(error));
			error = Free(error);
			continue;
		}

		// at this point, we know that the full path is not NULL and it points to an executable file
		// free the old token and set the full path to the token
		tokens[i] = Free(tokens[i]);
		tokens[i] = fullPath;
	}

	// if commandNotFound is true, then set exit status to 1 and return -1
	if (commandNotFound) {
		exit_status = 1;
		return -1;
	}

	// return 0 on success
	return 0;
}

// function that deals with built in commands
// returns -1 if the command is not a built-in command and 0 otherwise
ssize_t builtIn(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, then return 0
	if (tokens == NULL || numOfTokens == 0) {
		exit_status = 0;
		return 0;
	}

	// if command is "exit", then call exitCommand() to exit the program
	if (strcasecmp(tokens[0], "exit") == 0) {
		exitCommandWrap(tokens, numOfTokens);
	}

	// if command is "pwd", then call pwdCommand() to print the current working directory to stdout
	else if (strcasecmp(tokens[0], "pwd") == 0) {
		pwdCommand(tokens, numOfTokens);
	}

	// if command is "cd", then call cdCommand() to change the current working directory
	else if (strcasecmp(tokens[0], "cd") == 0) {
		cdCommand(tokens, numOfTokens);
	}

	// otherwise this is not a built-in command so return -1
	else {
		return -1;
	}

	// return 0 because this is a built-in command
	return 0;
}

// function that deals with a command that contains a single program
void singleProgram(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, then return
	if (tokens == NULL || numOfTokens == 0) {
		exit_status = 0;
		return;
	}

	// check program syntax
	if (checkProgramSyntax(tokens, numOfTokens) == -1) {
		exit_status = 1;
		return;
	}

	// call builtIn() to check if the command is a built-in command
	// if it is, then return
	if (builtIn(tokens, numOfTokens) == 0) {
		return;
	}

	// call getProgramArgs() to get the arguments of the program
	size_t numOfArgs = 0;
	char **args = getProgramArgs(tokens, numOfTokens, &numOfArgs);

	// at this point we know that the command does not contain a pipe so it is a single program
	// iterate over tokens, and save the stdin and stdout file paths specified by the redirection operators, if there are any
	// there could be at most 2 redirection operators, one for stdin and one for stdout, "<" and ">" respectively
	// the token following a redirection operator is the file path
	// if there are more than 1 of each type of redirection operator, then print error and set exit status to 1 and return
	// so "<" or ">" can only appear once in the command because there can be at most 1 stdin and 1 stdout redirection for a single program
	// if there is no stdin redirection, then set stdInFile to NULL
	// if there is no stdout redirection, then set stdOutFile to NULL
	// tokens can be like ["ls", "cool", "*", ">", "file.txt", "<", "file2.txt"] or ["ls", "<", "file.txt"] or ["ls"]
	const char *stdInFile = NULL;
	const char *stdOutFile = NULL;
	const char *programPath = tokens[0];

	// iterate over tokens and use strcmp to check if the command contains a redirection operator
	// if it does contain a redirection operator, then set the corresponding stdin or stdout file path
	for (size_t i = 0; i < numOfTokens; i++) {
		if (strcmp(tokens[i], "<") == 0) {
			stdInFile = tokens[i + 1];
		} else if (strcmp(tokens[i], ">") == 0) {
			stdOutFile = tokens[i + 1];
		}
	}

	// initialize input variables to call executeProgram()
	int stdInFdValue = -1;
	int stdOutFdValue = -1;
	const int *stdInFd = NULL;
	const int *stdOutFd = NULL;
	bool isStdInFdOpen = false;
	bool isStdOutFdOpen = false;

	if (stdInFile != NULL) {
		// get the file descriptor of the file specified by stdInFile
		stdInFdValue = open(stdInFile, O_RDONLY);

		// if the file descriptor is -1, then print error and set exit status to 1 and return
		if (stdInFdValue == -1) {
			exit_status = 1;
			perror("open");
			args = freeArrayOfStrings(args, numOfArgs);
			return;
		}

		// set stdInFd to the file descriptor
		stdInFd = &stdInFdValue;
		isStdInFdOpen = true;
	}

	// When redirecting output, the file should be created if it does not exist or truncated if it does
	// exist. Use mode 0640 (S_IRUSR|S_IWUSR|S_IRGRP) when creating
	if (stdOutFile != NULL) {
		// get the file descriptor of the file specified by stdOutFile
		stdOutFdValue = open(stdOutFile, O_WRONLY | O_CREAT | O_TRUNC, 0640);

		// if the file descriptor is -1, then print error and set exit status to 1 and return
		if (stdOutFdValue == -1) {
			exit_status = 1;
			perror("open");
			args = freeArrayOfStrings(args, numOfArgs);
			if (isStdInFdOpen && close(stdInFdValue) == -1) {
				perror("close");
			}
			return;
		}

		// set stdOutFd to the file descriptor
		stdOutFd = &stdOutFdValue;
		isStdOutFdOpen = true;
	}

	// call executeProgram() to execute the program
	executeProgram(programPath, args, stdInFd, stdOutFd, true, NULL, NULL);

	// free the memory allocated for args
	args = freeArrayOfStrings(args, numOfArgs);

	// close the file descriptors if they are open
	if (isStdInFdOpen && close(stdInFdValue) == -1) {
		exit_status = 1;
		perror("close");
	}
	if (isStdOutFdOpen && close(stdOutFdValue) == -1) {
		exit_status = 1;
		perror("close");
	}
}

// function that deals with multiple programs separated by pipes
void multiProgram(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, set exit status to 0 then return
	if (tokens == NULL || numOfTokens == 0) {
		exit_status = 0;
		return;
	}

	// we can assume that there is only one pipe in the command

	// tokenize the tokens into programs
	size_t numOfPrograms = 0;
	char *combinedStr = strCombineTokens(tokens, numOfTokens, " ");
	char **programs = strTokenize(combinedStr, "|", &numOfPrograms, "");
	combinedStr = Free(combinedStr);

	// there are exactly 2 programs so make variables to store each programs tokens and number of program tokens
	char **program1Tokens = NULL;
	char **program2Tokens = NULL;
	size_t numOfProgram1Tokens = 0;
	size_t numOfProgram2Tokens = 0;

	// tokenize each program and put them into their respective variables
	program1Tokens = strTokenize(programs[0], " ", &numOfProgram1Tokens, "");
	program2Tokens = strTokenize(programs[1], " ", &numOfProgram2Tokens, "");

	// now we can free the memory allocated for programs because we have the tokens for each program
	programs = freeArrayOfStrings(programs, numOfPrograms);

	// check program syntax for each program
	// if either program has invalid syntax, then print error and set exit status to 1 and return
	// make sure to free the memory allocated for the programs and their tokens before returning
	if (checkProgramSyntax(program1Tokens, numOfProgram1Tokens) == -1) {
		exit_status = 1;
		program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
		program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
		return;
	}

	if (checkProgramSyntax(program2Tokens, numOfProgram2Tokens) == -1) {
		exit_status = 1;
		program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
		program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
		return;
	}

	// run builtIn() on each program
	ssize_t builtInResult1 = builtIn(program1Tokens, numOfProgram1Tokens);
	ssize_t builtInResult2 = builtIn(program2Tokens, numOfProgram2Tokens);

	// if at least 1 program is a built in command, then call singleProgram() on the one that is not a built in command
	// if both programs are built in commands, then return
	if (builtInResult1 != -1) {
		if (builtInResult2 != -1) {
			program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
			program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
			return;
		} else {
			singleProgram(program2Tokens, numOfProgram2Tokens);
			program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
			program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
			return;
		}
	} else if (builtInResult2 != -1) {
		singleProgram(program1Tokens, numOfProgram1Tokens);
		program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
		program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
		return;
	}

	// at this point we know that the command is valid and we have the tokens for each program
	// so now we have to set up stdin and stdout for each program
	// and then call executeProgram() to execute each program

	// set up variables for program1
	const char *program1Path = program1Tokens[0];
	char **program1Args = NULL;
	size_t numOfProgram1Args = 0;
	program1Args = getProgramArgs(program1Tokens, numOfProgram1Tokens, &numOfProgram1Args);

	// set up variables for program2
	const char *program2Path = program2Tokens[0];
	char **program2Args = NULL;
	size_t numOfProgram2Args = 0;
	program2Args = getProgramArgs(program2Tokens, numOfProgram2Tokens, &numOfProgram2Args);

	// this is a piped program so the stdin for the first program is either the default stdin NULL or what is specified in file redirection
	// the stdout for the first program is the write end of the pipe
	// the stdin for the second program is the read end of the pipe
	// the stdout for the second program is either the default stdout NULL or what is specified in file redirection

	// initialize variables for stdin and stdout for program1
	int stdInFdValue1 = -1;
	int stdOutFdValue1 = -1;
	const int *stdInFd1 = NULL;
	const int *stdOutFd1 = NULL;
	bool isStdInFdOpen1 = false;
	bool isStdOutFdOpen1 = false;
	bool isPipeWriteSet = false; 

	// initialize variables for stdin and stdout for program2
	int stdInFdValue2 = -1;
	int stdOutFdValue2 = -1;
	const int *stdInFd2 = NULL;
	const int *stdOutFd2 = NULL;
	bool isStdInFdOpen2 = false;
	bool isStdOutFdOpen2 = false;
	bool isPipeReadSet = false;

	// iterate over program1Tokens and set stdin and stdout for program1
	for (size_t i = 0; i < numOfProgram1Tokens; i++) {
		// if the token is "<", then the next token is the file to redirect stdin from
		// so set stdInFd1 to the file descriptor of the file to redirect stdin from
		// and set isStdInFdOpen1 to true
		if (strcmp(program1Tokens[i], "<") == 0) {
			stdInFdValue1 = open(program1Tokens[i + 1], O_RDONLY);
			if (stdInFdValue1 == -1) {
				exit_status = 1;
				perror("open");
				program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
				program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
				program1Args = freeArrayOfStrings(program1Args, numOfProgram1Args);
				program2Args = freeArrayOfStrings(program2Args, numOfProgram2Args);
				if (isStdOutFdOpen1 && close(stdOutFdValue1) == -1) {
					perror("close");
				}
				return;
			}
			stdInFd1 = &stdInFdValue1;
			isStdInFdOpen1 = true;
		}

		// if the token is ">", then the next token is the file to redirect stdout to
		// so set stdOutFd1 to the file descriptor of the file to redirect stdout to
		// and set isStdOutFdOpen1 to true
		if (strcmp(program1Tokens[i], ">") == 0) {
			stdOutFdValue1 = open(program1Tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0640);
			if (stdOutFdValue1 == -1) {
				exit_status = 1;
				perror("open");
				program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
				program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
				program1Args = freeArrayOfStrings(program1Args, numOfProgram1Args);
				program2Args = freeArrayOfStrings(program2Args, numOfProgram2Args);
				if (isStdInFdOpen1 && close(stdInFdValue1) == -1) {
					perror("close");
				}
				return;
			}
			stdOutFd1 = &stdOutFdValue1;
			isStdOutFdOpen1 = true;
		}
	}

	// iterate over program2Tokens and set stdin and stdout for program2
	// make sure to close program1's file descriptors if they are open before returning from error
	for (size_t i = 0; i < numOfProgram2Tokens; i++) {
		// if the token is "<", then the next token is the file to redirect stdin from
		// so set stdInFd2 to the file descriptor of the file to redirect stdin from
		// and set isStdInFdOpen2 to true
		if (strcmp(program2Tokens[i], "<") == 0) {
			stdInFdValue2 = open(program2Tokens[i + 1], O_RDONLY);
			if (stdInFdValue2 == -1) {
				exit_status = 1;
				perror("open");
				program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
				program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
				program1Args = freeArrayOfStrings(program1Args, numOfProgram1Args);
				program2Args = freeArrayOfStrings(program2Args, numOfProgram2Args);
				if (isStdOutFdOpen1 && close(stdOutFdValue1) == -1) {
					perror("close");
				}
				if (isStdInFdOpen1 && close(stdInFdValue1) == -1) {
					perror("close");
				}
				if (isStdOutFdOpen2 && close(stdOutFdValue2) == -1) {
					perror("close");
				}
				return;
			}
			stdInFd2 = &stdInFdValue2;
			isStdInFdOpen2 = true;
		}

		// if the token is ">", then the next token is the file to redirect stdout to
		// so set stdOutFd2 to the file descriptor of the file to redirect stdout to
		// and set isStdOutFdOpen2 to true
		if (strcmp(program2Tokens[i], ">") == 0) {
			stdOutFdValue2 = open(program2Tokens[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0640);
			if (stdOutFdValue2 == -1) {
				exit_status = 1;
				perror("open");
				program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
				program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
				program1Args = freeArrayOfStrings(program1Args, numOfProgram1Args);
				program2Args = freeArrayOfStrings(program2Args, numOfProgram2Args);
				if (isStdInFdOpen1 && close(stdInFdValue1) == -1) {
					perror("close");
				}
				if (isStdOutFdOpen1 && close(stdOutFdValue1) == -1) {
					perror("close");
				}
				if (isStdInFdOpen2 && close(stdInFdValue2) == -1) {
					perror("close");
				}
				return;
			}
			stdOutFd2 = &stdOutFdValue2;
			isStdOutFdOpen2 = true;
		}
	}

	// at this point, we have the file descriptors for program1 and program2
	// so create a pipe
	// if pipe fails, then set exit_status to 1 and print the error message
	// and free all the memory and close all the file descriptors before returning
	int pipeFd[2];
	if (pipe(pipeFd) == -1) {
		exit_status = 1;
		perror("pipe");
		program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
		program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
		program1Args = freeArrayOfStrings(program1Args, numOfProgram1Args);
		program2Args = freeArrayOfStrings(program2Args, numOfProgram2Args);
		if (isStdInFdOpen1 && close(stdInFdValue1) == -1) {
			perror("close");
		}
		if (isStdOutFdOpen1 && close(stdOutFdValue1) == -1) {
			perror("close");
		}
		if (isStdInFdOpen2 && close(stdInFdValue2) == -1) {
			perror("close");
		}
		if (isStdOutFdOpen2 && close(stdOutFdValue2) == -1) {
			perror("close");
		}
		return;
	}

	// if program1 is missing a stdout redirection, then set it to the write end of the pipe
	if (isStdOutFdOpen1 == false) {
		stdOutFd1 = &pipeFd[1];
		isPipeWriteSet = true;
	}

	// if program2 is missing a stdin redirection, then set it to the read end of the pipe
	if (isStdInFdOpen2 == false) {
		stdInFd2 = &pipeFd[0];
		isPipeReadSet = true;
	}

	// create array to store read and write
	bool pipeSet1[2];
	bool pipeSet2[2];
	pipeSet1[0] = false;
	pipeSet1[1] = isPipeWriteSet;
	pipeSet2[0] = isPipeReadSet;
	pipeSet2[1] = false;

	// now call executeProgram twice
	// once for program1 and once for program2
	executeProgram(program1Path, program1Args, stdInFd1, stdOutFd1, false, pipeFd, pipeSet1);
	executeProgram(program2Path, program2Args, stdInFd2, stdOutFd2, true, pipeFd, pipeSet2);

	// free all the memory and close all the file descriptors and pipes if they are open
	program1Tokens = freeArrayOfStrings(program1Tokens, numOfProgram1Tokens);
	program2Tokens = freeArrayOfStrings(program2Tokens, numOfProgram2Tokens);
	program1Args = freeArrayOfStrings(program1Args, numOfProgram1Args);
	program2Args = freeArrayOfStrings(program2Args, numOfProgram2Args);
	if (isStdInFdOpen1 && close(stdInFdValue1) == -1) {
		exit_status = 1;
		perror("close");
	}
	if (isStdOutFdOpen1 && close(stdOutFdValue1) == -1) {
		exit_status = 1;
		perror("close");
	}
	if (isStdInFdOpen2 && close(stdInFdValue2) == -1) {
		exit_status = 1;
		perror("close");
	}
	if (isStdOutFdOpen2 && close(stdOutFdValue2) == -1) {
		exit_status = 1;
		perror("close");
	}
}

// function that returns a new allocated list of arguments for a program that is terminated by a NULL pointer
char** getProgramArgs(char **tokens, size_t numOfTokens, size_t *numOfArgs) {
	// if tokens is NULL or numOfTokens is 0, then return NULL
	if (tokens == NULL || numOfTokens == 0 || numOfArgs == NULL) {
		return NULL;
	}

	// iterate over tokens and as soon as you see a "|" or end of tokens, that means the end of the argument list for the program
	// so tokens are like ["ls", "lol", "cool", "<", "file.txt", "fine"]
	// the argument list is ["ls", "lol", "cool", "fine"]
	// set numOfArgs
	*numOfArgs = 0;
	for (size_t i = 0; i < numOfTokens; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			break;
		}
		if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0) {
			// if the token is "<" or ">", then skip the next token
			i++;
			continue;
		}
		(*numOfArgs)++;
	}

	// allocate memory for the argument list
	char **args = malloc(sizeof(char *) * (*numOfArgs + 1));
	if (args == NULL) {
		exit_status = 1;
		perror("malloc");
		return NULL;
	}

	// set the last element of the argument list to NULL
	args[*numOfArgs] = NULL;

	// copy the tokens into the argument list
	// use same process as above to determine if the current token is an argument or not
	// if it is an argument, then copy it into the argument list using strdup
	// otherwise skip it
	size_t j = 0;
	for (size_t i = 0; i < numOfTokens; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			break;
		}
		if (strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0) {
			// if the token is "<" or ">", then skip the next token
			i++;
			continue;
		}
		args[j] = strdup(tokens[i]);
		if (args[j] == NULL) {
			exit_status = 1;
			perror("strdup");
			args = freeArrayOfStrings(args, *numOfArgs);
			return NULL;
		}
		j++;
	}

	// return the argument list
	return args;
}

// function that checks for command syntax errors
// returns -1 on syntax error and 0 on success
ssize_t checkCommandSyntax(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, then return 0
	if (tokens == NULL || numOfTokens == 0) {
		exit_status = 0;
		return 0;
	}

	// check syntax for both single and multiple programs
	// if the first token is a pipe or redirection operator, print error and set exit status to 1 and return
	if (strcmp(tokens[0], "|") == 0 || strcmp(tokens[0], "<") == 0 || strcmp(tokens[0], ">") == 0) {
		exit_status = 1;
		write(STDERR_FILENO, "command has invalid syntax\n", 27);
		return -1;
	}

	// check the rest of the tokens
	for (size_t i = 1; i < numOfTokens; i++) {
		// if the token following a pipe is a pipe or redirection operator, print error and set exit status to 1 and return
		if (strcmp(tokens[i], "|") == 0 || strcmp(tokens[i], "<") == 0 || strcmp(tokens[i], ">") == 0) {
			if (i + 1 >= numOfTokens || strcmp(tokens[i + 1], "|") == 0 || strcmp(tokens[i + 1], "<") == 0 || strcmp(tokens[i + 1], ">") == 0) {
				exit_status = 1;
				write(STDERR_FILENO, "command has invalid syntax\n", 27);
				return -1;
			}
		}
	}

	// also if the command contains more than 1 pipe, then exit status 1 and return -1
	// iterate over tokens and if you see more than 1 pipe, then return -1
	bool seenPipe = false;
	for (size_t i = 0; i < numOfTokens; i++) {
		if (strcmp(tokens[i], "|") == 0) {
			if (seenPipe) {
				exit_status = 1;
				write(STDERR_FILENO, "command has invalid syntax\n", 27);
				return -1;
			}
			seenPipe = true;
		}
	}

	// return 0 on success
	return 0;
}

// function that checks syntax of a single program
// returns -1 on syntax error and 0 on success
ssize_t checkProgramSyntax(char **tokens, size_t numOfTokens) {
	// if tokens is NULL or numOfTokens is 0, then return 0
	if (tokens == NULL || numOfTokens == 0) {
		exit_status = 0;
		return 0;
	}

	// initialize stdInFile and numOfArgs and stdOutFile
	const char *stdInFile = NULL;
	const char *stdOutFile = NULL;

	// iterate over the tokens
	for (size_t i = 0; i < numOfTokens; i++) {
		if (strcmp(tokens[i], "<") == 0) {
			if (stdInFile != NULL) {
				exit_status = 1;
				write(STDERR_FILENO, "command can not have multiple stdin redirections\n", 49);
				return -1;
			}
			if (i + 1 >= numOfTokens || strcmp(tokens[i + 1], "<") == 0 || strcmp(tokens[i + 1], ">") == 0) {
				exit_status = 1;
				write(STDERR_FILENO, "command is missing stdin redirection file path\n", 47);
				return -1;
			}
			stdInFile = tokens[i + 1];
		} else if (strcmp(tokens[i], ">") == 0) {
			if (stdOutFile != NULL) {
				exit_status = 1;
				write(STDERR_FILENO, "command can not have multiple stdout redirections\n", 50);
				return -1;
			}
			if (i + 1 >= numOfTokens || strcmp(tokens[i + 1], "<") == 0 || strcmp(tokens[i + 1], ">") == 0) {
				exit_status = 1;
				write(STDERR_FILENO, "command is missing stdout redirection file path\n", 48);
				return -1;
			}
			stdOutFile = tokens[i + 1];
		}
	}

	// return 0 on success
	return 0;
}

// function that returns a list of filenames that match a given pattern with wildcard directories and files
// returns NULL on error
char** getFilenamesExt(const char *filePath, size_t *pnumOfFilenames) {
	// if filePath is NULL, then return NULL
	if (filePath == NULL || strlen(filePath) == 0 || pnumOfFilenames == NULL) {
		return NULL;
	}

	// initialize pnumOfFilenames to 0
	*pnumOfFilenames = 0;

	// if the filePath does not contain a "*" then return NULL
	if (strchr(filePath, '*') == NULL) {
		return NULL;
	}
	
	// the filePath is like "/dir1/dir2/dir3/a*t.txt" or "/dir1/a*" or "/dir1/*a" or "/dir1/*" or "/dir1/a*t" or "*a" or "*"
	// if it is just the filename without "/", then search the current directory
	// first check whether the path is a directory or a file. If it is a directory, then return NULL
	// if the last character is a "/" that means the path is a directory so return NULL
	size_t filePathLen = strlen(filePath);
	if (filePath[filePathLen - 1] == '/') {
		return NULL;
	}

	// at this point you know that the filePath points to a file, not a directory (theoretically)
	// use glob() to get the list of filenames that match the pattern in filePath
	// do not include hidden files that start with a "."
	// do not include directories or anything that is not a file in the list of filenames
	// do not sort the list of filenames
	// do not use any escape characters
	// if no matches are found or an error occurs, then return NULL
	// do not expand the pattern with tilda or anything else, keep it as it is
	// otherwise return the list of filenames
	glob_t globbuf;
	int globStatus = glob(filePath, GLOB_MARK | GLOB_NOSORT | GLOB_NOESCAPE, NULL, &globbuf);

	// if an error occurs, then return NULL
	if (globStatus != 0) {
		return NULL;
	}

	// if no matches are found, then return NULL
	if (globbuf.gl_pathc == 0) {
		globfree(&globbuf);
		return NULL;
	}

	// copy the list of filenames to a new array of strings
	// use strDupArrayOfStrings() or strdup() to duplicate the filenames
	size_t numOfFilenames = globbuf.gl_pathc;
	char **filenames = strDupArrayOfStrings(globbuf.gl_pathv, numOfFilenames);

	// free the globbuf
	globfree(&globbuf);

	// if you get here, then you have a list of filenames that match the pattern in filePath
	// just make sure that none of the filenames are directories or hidden files
	// if you find a directory or a hidden file, then remove it from the list
	// if you remove a filename, then decrement the numOfFilenames
	// if you remove all the filenames, then return NULL
	// first iterate over filenames and count the number of directories and hidden files
	size_t numOfWrongFiles = 0;

	// use isRegularFile() to check whether a filename is a regular file or not
	// make sure to extract last part of the filename before checking whether it is a regular file or not and "."
	for (size_t i = 0; i < numOfFilenames; i++) {
		// first extract the last part of the filename after checking whether filename contains a "/"
		// use strdup and strchr and strrchr
		char *lastPart = NULL;
		if (strchr(filenames[i], '/') == NULL || filenames[i][strlen(filenames[i]) - 1] == '/') {
			lastPart = strdup(filenames[i]);
		} else {
			lastPart = strdup(strrchr(filenames[i], '/') + 1);
		}
		
		// now check isRegularFile() passing in the entire path and "." is the first character of the last part of the filename
		if (isRegularFile(filenames[i]) == false || lastPart[0] == '.') {
			numOfWrongFiles++;
		}

		// free the last part of the filename
		lastPart = Free(lastPart);
	}

	// calculate the final number of filenames
	size_t finalNumOfFilenames = numOfFilenames - numOfWrongFiles;

	// if the final number of filenames is 0, then return NULL
	if (finalNumOfFilenames == 0) {
		filenames = freeArrayOfStrings(filenames, numOfFilenames);
		return NULL;
	}

	// allocate memory for the final list of filenames
	char **finalFilenames = malloc(finalNumOfFilenames * sizeof(char *));

	// iterate over the filenames and copy the filenames that are not directories or hidden files to the final list of filenames
	// a filename is of the form "/dir1/dir2/dir3/a.txt", so just copy the last part of the filename because we just want the filename
	// extract the last part of the filename before isRegularFile() and "." is checked
	size_t j = 0;
	for (size_t i = 0; i < numOfFilenames; i++) {
		// first extract the last part of the filename after checking whether filename contains a "/"
		// use strdup and strchr and strrchr
		char *lastPart = NULL;
		if (strchr(filenames[i], '/') == NULL || filenames[i][strlen(filenames[i]) - 1] == '/') {
			lastPart = strdup(filenames[i]);
		} else {
			lastPart = strdup(strrchr(filenames[i], '/') + 1);
		}
		
		// now check isRegularFile() passing in the entire path and "." is not the first character of the last part of the filename
		if (isRegularFile(filenames[i]) == true && lastPart[0] != '.') {
			finalFilenames[j] = strdup(filenames[i]);
			j++;
		}

		// free the last part of the filename
		lastPart = Free(lastPart);
	}

	// free the filenames
	filenames = freeArrayOfStrings(filenames, numOfFilenames);

	// set the number of filenames
	*pnumOfFilenames = finalNumOfFilenames;

	// return the final list of filenames
	return finalFilenames;
}
