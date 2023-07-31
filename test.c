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


char* readOutput(int fd) {
	// read input
	char *buffer = malloc(sizeof(char) * 101);
	size_t size = 101;
	size_t i = 0;
	char c;
	ssize_t readStatus;
	while ((readStatus = read(fd, &c, sizeof(char))) == sizeof(char)) {
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
			return NULL;
		}
	}
	// if readStatus is -1, then there was an error
	if (readStatus == -1) {
		buffer = Free(buffer);
		perror("read");
		exit(EXIT_FAILURE);
	}
	// now the buffer is complete and can be executed
	return buffer;
}

// function that prints the current working directory
// example: /current/path/subdir/subsubdir
void pwdCommand(int fd) {
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
	write(fd, buffer, strlen(buffer));
	write(fd, "\n", 1);
	buffer = Free(buffer);
}

// Test Case A_1: mysh only takes up to one argument
void program_A_1() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the arguments passed into mysh
	// exp.txt will contain the expected output of the arguments passed into mysh
	int fdO = open("testSuite/A/1/out.txt", O_RDONLY);
	int fdE = open("testSuite/A/1/exp.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with arguments "echo" and "testSuite/A/1/myscript.sh"
	// the stdout and stderr of the two arguments is redirected to "testSuite/A/1/out.txt"
    system("./mysh echo testSuite/A/1/myscript.sh 2> testSuite/A/1/out.txt");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case A_1 failed.
		// Or if both files are not NULL, but the contents of the output file do not equal the contents of the 
		// expected file, then Test Case A_1 failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case A_1_ALL failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case A_1 passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case A_1_ALL passed\n");
}

// Test Case A_2_BAT: mysh will enter batch mode when given one argument
void program_A_2_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/A/2/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/A/2/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/A/2/myscript.sh"
	// the stdout and stderr of the argument is redirected to "testSuite/A/2/outBAT.txt"
    system("./mysh testSuite/A/2/myscript.sh > testSuite/A/2/outBAT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case A_2_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case A_2_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case A_2_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case A_2_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case A_2_BAT passed\n");
}

// Test Case A_2_INT tests if mysh will enter interactive mode when given no arguments
void program_A_2_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/A/2/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/A/2/myscript.sh"
	int fdO = open("testSuite/A/2/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/A/2/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/A/2/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/A/2/myscript.sh"
	// stderr is set to stdout
    system("./mysh > testSuite/A/2/outINT.txt < testSuite/A/2/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case A_2_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case A_2_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case A_2_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case A_2_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case A_2_INT passed\n");
}

// Test Case B_1: mysh will execute the commands sequentially (execute command, wait for completion, then execute next command)
void program_B_1() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/B/1/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/B/1/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/B/1/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/B/1/outBAT.txt"
	// stderr is redirected to stdout
    system("./mysh testSuite/B/1/myscript.sh > testSuite/B/1/outBAT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case B_1_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case B_1 failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case B_1_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case B_1_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case B_1_BAT passed\n");
}

// Test Case B_2 tests if mysh terminates once it reaches end of input file
void program_B_2() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/B/2/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/B/2/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/B/2/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/B/2/outBAT.txt"
	// stdout is redirected to stdout
    system("./mysh testSuite/B/2/myscript.sh > testSuite/B/2/outBAT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case B_2_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case B_2_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case B_2_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case B_2_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case B_2_BAT passed\n");
}

// Test Case B_3: mysh terminates when it encounters shell command exit
void program_B_3() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/B/3/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/B/3/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/B/3/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/B/3/outBAT.txt"
	// stderr is redirected to stdout
    system("./mysh testSuite/B/3/myscript.sh > testSuite/B/3/outBAT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case B_3_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case B_3_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case B_3_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case B_3_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case B_3_BAT passed\n");
}

// Test Case C_1 tests:
// if mysh prints a greeting before the first prompt in Interaction Mode
// Before reading a command, mysh will write a prompt to stdout to indicate that it is ready to read input (“mysh> ”)
// mysh reads commands from stdin
void program_C_1() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/C/1/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/C/1/myscript.sh"
	int fdO = open("testSuite/C/1/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/C/1/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/C/1/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/C/1/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/C/1/outINT.txt < testSuite/C/1/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case C_1_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case C_1_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case C_1_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case C_1_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case C_1_INT passed\n");
}

// Test Case C_2: After executing a command, mysh will print a new prompt:
// a. If the last command fails, print prompt “!mysh> ” and read the next command
// b. Else print prompt “mysh> ” and read the next command
void program_C_2() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/C/2/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/C/2/myscript.sh"
	int fdO = open("testSuite/C/2/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/C/2/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/C/2/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/C/2/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/C/2/outINT.txt < testSuite/C/2/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case C_2_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case C_2_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case C_2_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case C_2_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case C_2_INT passed\n");
}

//Test Case C_3: mysh terminates once it reaches the end of the input file
void program_C_3() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/C/3/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/C/3/myscript.sh"
	int fdO = open("testSuite/C/3/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/C/3/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/C/3/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/C/3/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/C/3/outINT.txt < testSuite/C/3/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case C_3_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case C_3_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case C_3_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case C_3 passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case C_3_INT passed\n");
}

//Test Case C_4: mysh terminates once it encounters shell command exit
void program_C_4() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/C/4/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/C/4/myscript.sh"
	int fdO = open("testSuite/C/4/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/C/4/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/C/4/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/C/4/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/C/4/outINT.txt < testSuite/C/4/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case C_4_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case C_4_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case C_4_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case C_4_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case C_4_INT passed\n");
}

// Test Case D_1_BAT: In batch mode, there is no intrinsic limit to the length of a command that will cause mysh to fail.
void program_D_1_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/1/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/D/1/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/1/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/1/outBAT.txt"
	// stderr is redirected to stdout
    system("./mysh testSuite/D/1/myscript.sh > testSuite/D/1/outBAT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_1_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_1_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_1_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_1_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_1_BAT passed\n");
}

// Test Case D_1_INT: In interactive mode, there is no intrinsic limit to the length of a command that will cause 
// mysh to fail.
void program_D_1_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/1/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/1/myscript.sh"
	int fdO = open("testSuite/D/1/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/1/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/D/1/outINT.txt" and will read commands from stdin,
	// stdin is set to file "testSuite/D/1/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/1/outINT.txt < testSuite/D/1/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_1_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_1_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_1_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_1_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_1_INT passed\n");
}

// Test Case D_2_BAT: In batch mode, the code interprets the first token of a command or sub-command as the 
// program/built-in operation to execute 
void program_D_2_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/2/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/D/2/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/2/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/2/outBAT.txt"
	// stderr is redirected to stdout
    system("./mysh testSuite/D/2/myscript.sh > testSuite/D/2/outBAT.txt 2>&1");
	// another file descriptor with a seperate open call to the same file is used to write the present working directory.
	// the present working directory is dependent on the machine this code runs on, thus the pwdCommand() is used to 
	// manually write the present working directory into "testSuite/D/2/expBAT.txt", and chdir() is used to manually 
	// change directories.  
	int fdE2 = open("testSuite/D/2/expBAT.txt", O_RDWR | O_TRUNC);
	pwdCommand(fdE2);
	chdir("..");
	pwdCommand(fdE2);
	chdir("p2");
	pwdCommand(fdE2);
	write(fdE2, "this should work\n", 17);
	write(fdE2, "pwd\n", 4);
	write(fdE2, "cd\n", 3);
	write(fdE2, "exit\n", 5);
	close(fdE2);
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_2_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_2_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_2_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_2_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_2_BAT passed\n");
}

// Test Case D_2_INT: In interactive mode, the code interprets the first token of a command or sub-command as the 
// program/built-in operation to execute
void program_D_2_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/2/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/2/myscript.sh"
	int fdO = open("testSuite/D/2/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/2/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/D/2/outINT.txt" and will read commands from stdin,
	// where stdin is set to file "testSuite/D/2/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/2/outINT.txt < testSuite/D/2/myscript.sh 2>&1");
	// another file descriptor with a seperate open call to the same file is used to write the present working directory.
	// the present working directory is dependent on the machine this code runs on, thus the pwdCommand() is used to 
	// manually write the present working directory into "testSuite/D/2/expINT.txt", and chdir() is used to manually 
	// change directories.  
	int fdE2 = open("testSuite/D/2/expINT.txt", O_RDWR | O_TRUNC);
	write(fdE2, "Welcome to MySH 3.0\n", 20);
	write(fdE2, "mysh> ", 6);
	pwdCommand(fdE2);
	write(fdE2, "mysh> ", 6);
	chdir("..");
	write(fdE2, "mysh> ", 6);
	pwdCommand(fdE2);
	write(fdE2, "mysh> ", 6);
	chdir("p2");
	write(fdE2, "mysh> ", 6);
	pwdCommand(fdE2);
	// to maintain the dynamic state of "testSuite/D/2/expINT.txt", the rest of the commands after calling pwd and cd are
	// are also outputted and written into "testSuite/D/2/expINT.txt"
	char msg[74] = "mysh> this should work\n"
					"mysh> pwd\n"
					"mysh> cd\n"
					"mysh> exit\n"
					"mysh> mysh: exiting\n";
	write(fdE2, msg, 73);
	close(fdE2);
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_2_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_2_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_2_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_2_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_2_INT passed\n");
}

// Test Case D_3_BAT:
// If an error is caused using built-in commands (ex: changing to a non-existent or inaccessible directory), mysh prints 
// an error message to stderr, and the last exit status is set to 1.
void program_D_3_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/3/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/D/3/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/3/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/3/outBAT.txt"
	// stderr is redirected to stdout
    system("./mysh testSuite/D/3/myscript.sh > testSuite/D/3/outBAT.txt 2>&1");
	
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_3_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_3_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_3_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_3_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_3_BAT passed\n");
}

// Test Case D_3_INT: In interactive mode, if an error is caused using built-in commands (ex: changing to a non-existent 
// or inaccessible directory), mysh prints an error message to stderr, and the last exit status is set to 1.
void program_D_3_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/3/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/3/myscript.sh"
	int fdO = open("testSuite/D/3/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/3/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/D/3/outINT.txt" and will read commands from stdin,
	// stdin is set to file "testSuite/D/3/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/3/outINT.txt < testSuite/D/3/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_3_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_3_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_3_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_3_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_3_INT passed\n");
}

// Test Case D_4_BAT: In batch mode, If the specified program cannot be executed, mysh prints an error message, 
// and the last exit status is set to 1.
void program_D_4_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/4/BAT/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/D/4/BAT/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/4/BAT/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/4/outBAT.txt"
	// the stderr of the argument is redirected to stdout
	system("./mysh testSuite/D/4/BAT/myscript.sh > testSuite/D/4/BAT/outBAT.txt 2>&1");
	// mysh is called with argument "testSuite/D/4"
	// the stdout of the argument is redirected to "testSuite/D/4/BAT/outBAT.txt"
	// a double arrow (>>) is used to append to the output file
	// the stderr of the argument is redirected to stdout
    system("./mysh testSuite/D/4 >> testSuite/D/4/BAT/outBAT.txt 2>&1");
	// mysh is called with argument "testSuite/D/4/random.sh"
	// the stdout of the argument is redirected to "testSuite/D/4/BAT/outBAT.txt"
	// a double arrow (>>) is used to append to the output file
	// the stderr of the argument is redirected to stdout
	system("./mysh testSuite/D/4/random.sh >> testSuite/D/4/BAT/outBAT.txt 2>&1");
	// mysh is called with argument "testSuite/D/100"
	// the stdout of the argument is redirected to "testSuite/D/4/BAT/outBAT.txt"
	// a double arrow (>>) is used to append to the output file
	// the stderr of the argument is redirected to stdout
	system("./mysh testSuite/D/100 >> testSuite/D/4/BAT/outBAT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_4_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_4_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_4_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_4_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_4_BAT passed\n");
}

// Test Case D_4_INT: In interactive mode, if the specified program cannot be executed, mysh prints an error message, 
// and the last exit status is set to 1.
void program_D_4_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/4/INT/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/4/INT/myscript.sh"
	int fdO = open("testSuite/D/4/INT/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/4/INT/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/D/4/INT/outINT.txt" and will read commands from stdin,
	// stdin is set to file "testSuite/D/4/INT/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/4/INT/outINT.txt < testSuite/D/4/INT/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_4_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_4_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_4_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_4_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_4_INT passed\n");
}

// Test Case D_5: 
// if the command is not built-in and not a path, mysh checks the following directories in the same order for a file
// with the specified name:
// 1. /usr/local/sbin
// 2. /usr/local/bin
// 3. /usr/sbin
// 4. /usr/bin
// 5. /sbin
// 6. /bin 
// If no file is found, mysh prints an error message, and the last exit status is set to 1.
// If a file is found but cannot be executed, mysh prints an error message, and the last exit status is set to 1
void program_D_5_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/5/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/5/myscript.sh"
	int fdO = open("testSuite/D/5/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/5/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/D/5/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/D/5/INT/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/5/outINT.txt < testSuite/D/5/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_5_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_5_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_5_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_5_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_5_INT passed\n");
}

// Test Case D_6_BAT: In batch mode, 
// mysh matches any file in the specified directory whose name begins with the characters before the asterisk AND ends 
// with the characters after the asterisk.
// When a command includes a wildcard token, it is replaced in the argument list by the list of names matching the pattern.
// If no names match the pattern, mysh passes the token to the command unchanged.
// Asterisks may occur in any segment of a path. For example, */*.c references files ending with .c in any subdirectory
// of the working directory (excluding files and subdirectories that begin with a period).
// You may allow more than one asterisk within a path segment, but this is not required.
void program_D_6_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/6/BAT/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/D/6/BAT/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/4/BAT/*.sh"
	// the stdout of the argument is redirected to "testSuite/D/6/outBAT.txt and stderr is redirected to stdout"
	system("./mysh testSuite/D/4/BAT/*.sh > testSuite/D/6/BAT/outBAT.txt 2>&1");
	// mysh is called with argument "testSuite/B/2/my*"
	// the stdout of the argument is redirected to "testSuite/D/6/outBAT.txt and stderr is redirected to stdout"
	system("./mysh testSuite/B/2/my* >> testSuite/D/6/BAT/outBAT.txt 2>&1");
	// mysh is called with argument "testSuite/D/1/m*h"
	// the stdout of the argument is redirected to "testSuite/D/6/outBAT.txt and stderr is redirected to stdout"
	system("./mysh testSuite/D/1/m*h >> testSuite/D/6/BAT/outBAT.txt 2>&1");
	// mysh is called with argument "*"
	// the stdout of the argument is redirected to "testSuite/D/6/outBAT.txt and stderr is redirected to stdout"
	system("./mysh * >> testSuite/D/6/BAT/outBAT.txt 2>&1");
	// mysh is called with argument "testSuite/D/6/BAT/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/6/outBAT.txt and stderr is redirected to stdout"
	system("./mysh testSuite/D/6/BAT/myscript.sh >> testSuite/D/6/BAT/outBAT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_6_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_6_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_6_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_6_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_6_BAT passed\n");
}

// Test Case D_6_INT: In interactive mode,
// mysh matches any file in the specified directory whose name begins with the characters before the asterisk AND ends 
// with the characters after the asterisk.
// When a command includes a wildcard token, it is replaced in the argument list by the list of names matching the pattern.
// If no names match the pattern, mysh passes the token to the command unchanged.
// Asterisks may occur in any segment of a path. For example, */*.c references files ending with .c in any subdirectory
// of the working directory (excluding files and subdirectories that begin with a period).
// You may allow more than one asterisk within a path segment, but this is not required.
void program_D_6_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/6/INT/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/6/INT/myscript.sh"
	int fdO = open("testSuite/D/6/INT/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/6/INT/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/D/6/INT/outINT.txt" and will read commands from stdin,
	// where stdin is set to file "testSuite/D/6/INT/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/6/INT/outINT.txt < testSuite/D/6/INT/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	size_t lineCount = 0;
	char** tokensE = NULL;
	char** tokensO = NULL; 
	size_t numOfTokensE = 0;
	size_t numOfTokensO = 0;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		lineCount++;
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_6_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_6_INT failed
		if (((lineO == NULL) ^ (lineE == NULL))) {
			close(fdO);
			close(fdE);
			printf("Test Case D_6_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		numOfTokensE = 0;
		numOfTokensO = 0;
		tokensE = strTokenize(lineE, " ", &numOfTokensE, "");
		tokensO = strTokenize(lineO, " ", &numOfTokensO, "");
		if (numOfTokensE != numOfTokensO) {
			close(fdO);
			close(fdE);
			printf("Test Case D_6_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE);
			tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
			tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
			return;
		}
		bool found = false;
		for (size_t i = 0; i < numOfTokensE; i++) {
			for (size_t j = 0; j < numOfTokensO; j++) {
				if (strcmp(tokensE[i], tokensO[j]) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				close(fdO);
				close(fdE);
				printf("Test Case D_6_INT failed\n");
				lineO = Free(lineO);
				lineE = Free(lineE);
				tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
				tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
				return;
			}
		}
		if (lineCount != 2 && lineCount != 3 && lineCount != 4 && lineCount != 5) {
			if (strcmp(lineE, lineO) != 0) {
				close(fdO);
				close(fdE);
				printf("Test Case D_6_INT failed\n");
				lineO = Free(lineO);
				lineE = Free(lineE);
				tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
				tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
				return;
			}
		}
		lineO = Free(lineO);
		lineE = Free(lineE);
		tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
		tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_6_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE);
	printf("Test Case D_6_INT passed\n");
}

// Test Case D_7_BAT: In batch mode, 
// When redirecting the output, if the file does not exist, the file should be created using mode 0640 
// (S_IRUSR|S_IWUSR|S_IRGRP)
// When redirecting the output, if the file already exists, the file should be truncated.
// If mysh is unable to open the file in the requested mode, mysh reports an error, and the last exit status is set to 1.
void program_D_7_BAT() {
	// A system call is used to ensure the output file is deleted before redirecting stdout to that file
	system("rm -rf testSuite/D/7/BAT/outBAT1.txt");
	// mysh is called with argument "testSuite/D/6/INT/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/7/outBAT1.txt and stderr is redirected to stdout"
	system("./mysh testSuite/D/6/INT/myscript.sh > testSuite/D/7/BAT/outBAT1.txt 2>&1");
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/7/BAT/outBAT1.txt", O_RDONLY);
	int fdE = open("testSuite/D/7/BAT/expBAT1.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	char *lineO = NULL;
	char *lineE = NULL;

	size_t lineCount = 0;
	char** tokensE = NULL;
	char** tokensO = NULL; 
	size_t numOfTokensE = 0;
	size_t numOfTokensO = 0;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		lineCount++;
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_7_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_7_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL))) {
			close(fdO);
			close(fdE);
			printf("Test Case D_7_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		numOfTokensE = 0;
		numOfTokensO = 0;
		tokensE = strTokenize(lineE, " ", &numOfTokensE, "");
		tokensO = strTokenize(lineO, " ", &numOfTokensO, "");
		if (numOfTokensE != numOfTokensO) {
			close(fdO);
			close(fdE);
			printf("Test Case D_7_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE);
			tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
			tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
			return;
		}
		bool found = false;
		for (size_t i = 0; i < numOfTokensE; i++) {
			for (size_t j = 0; j < numOfTokensO; j++) {
				if (strcmp(tokensE[i], tokensO[j]) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				close(fdO);
				close(fdE);
				printf("Test Case D_7_BAT failed\n");
				lineO = Free(lineO);
				lineE = Free(lineE);
				tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
				tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
				return;
			}
		}
		if (lineCount != 1 && lineCount != 2 && lineCount != 3 && lineCount != 4) {
			if (strcmp(lineE, lineO) != 0) {
				close(fdO);
				close(fdE);
				printf("Test Case D_7_BAT failed\n");
				lineO = Free(lineO);
				lineE = Free(lineE);
				tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
				tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
				return;
			}
		}
		lineO = Free(lineO);
		lineE = Free(lineE);
		tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
		tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_7_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE);
	printf("Test Case D_7_BAT passed\n");
	// A system call is used to delete the output file after redirecting stdout to that file
	system("rm -rf testSuite/D/7/BAT/outBAT1.txt");
}

// This is Part 2 of Test Case D_7_BAT, where the output file will not be deleted and the argument called to mysh is
// invalid
void program_D_7_BA2() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/7/BAT/outBAT2.txt", O_RDONLY);
	int fdE = open("testSuite/D/7/BAT/expBAT2.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/100/INT/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/7/outBAT2.txt and stderr is redirected to stdout"
	system("./mysh testSuite/D/100/INT/myscript.sh > testSuite/D/7/BAT/outBAT2.txt 2>&1");

	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_7_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_7_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_7_BA2 failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_7_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_7_BA2 passed\n");
}

// Test Case D_7_INT: In interactive mode,
// When redirecting the output, if the file does not exist, the file should be created using mode 0640 
// (S_IRUSR|S_IWUSR|S_IRGRP)
// When redirecting the output, if the file already exists, the file should be truncated.
// If mysh is unable to open the file in the requested mode, mysh reports an error, and the last exit status is set to 1.
void program_D_7_INT() {
	// A system call is used to delete the output file before redirecting stdout to that file
	system("rm -rf testSuite/D/7/INT/OUT1.txt");
	// stderr is redirected to stdout
    system("./mysh >> testSuite/D/7/INT/OUT1.txt < testSuite/D/7/INT/myscript.sh 2>&1");

	// open the expected output and the actual output file in read only mode
	int fdO = open("testSuite/D/7/INT/OUT1.txt", O_RDONLY);
	int fdE = open("testSuite/D/7/INT/expOUT1.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	char *lineO = NULL;
	char *lineE = NULL;
	size_t lineCount = 0;
	char** tokensE = NULL;
	char** tokensO = NULL; 
	size_t numOfTokensE = 0;
	size_t numOfTokensO = 0;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		lineCount++;
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_7_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_7_INT failed
		if (((lineO == NULL) ^ (lineE == NULL))) {
			close(fdO);
			close(fdE);
			printf("Test Case D_7_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		numOfTokensE = 0;
		numOfTokensO = 0;
		tokensE = strTokenize(lineE, " ", &numOfTokensE, "");
		tokensO = strTokenize(lineO, " ", &numOfTokensO, "");
		if (numOfTokensE != numOfTokensO) {
			close(fdO);
			close(fdE);
			printf("Test Case D_7_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE);
			tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
			tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
			return;
		}
		bool found = false;
		for (size_t i = 0; i < numOfTokensE; i++) {
			for (size_t j = 0; j < numOfTokensO; j++) {
				if (strcmp(tokensE[i], tokensO[j]) == 0) {
					found = true;
					break;
				}
			}
			if (!found) {
				close(fdO);
				close(fdE);
				printf("Test Case D_7_INT failed\n");
				lineO = Free(lineO);
				lineE = Free(lineE);
				tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
				tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
				return;
			}
		}
		if (lineCount != 3 && lineCount != 4 && lineCount != 5 && lineCount != 6) {
			if (strcmp(lineE, lineO) != 0) {
				close(fdO);
				close(fdE);
				printf("Test Case D_7_INT failed\n");
				lineO = Free(lineO);
				lineE = Free(lineE);
				tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
				tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
				return;
			}
		}
		lineO = Free(lineO);
		lineE = Free(lineE);
		tokensE = freeArrayOfStrings(tokensE, numOfTokensE);
		tokensO = freeArrayOfStrings(tokensO, numOfTokensO); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_7_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE);
	printf("Test Case D_7_INT passed\n");
	// A system call is used to delete the output file after redirecting stdout to that file
	system("rm -rf testSuite/D/7/INT/OUT1.txt");
}

// This is Part 2 to Test Case D_7_INT, where the output file is not deleted and the stdin of mysh in the file
// testSuite/D/7/INT/myscript2.sh is invalid
void program_D_7_IN2() {
	// mysh is called with no argument
	// the stdout of mysh is redirected to "testSuite/D/7/INT/OUT2.txt" and will read commands from stdin,
	// where stdin is set to file "testSuite/D/7/INT/myscript2.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/7/INT/OUT2.txt < testSuite/D/7/INT/myscript2.sh 2>&1");

	// open the expected output and the actual output file in read only mode
	int fdO = open("testSuite/D/7/INT/OUT2.txt", O_RDONLY);
	int fdE = open("testSuite/D/7/INT/expOUT2.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_7_IN2 failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_7_IN2 failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_7_IN2 failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_7_IN2 passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_7_IN2 passed\n");
}

// Test Case D_8_BAT: In Batch Mode, mysh allows for a single pipe to connect two processes
void program_D_8_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/8/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/D/8/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/8/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/D/8/outBAT.txt"
	// stderr is redirected to stdout
    system("./mysh testSuite/D/8/myscript.sh > testSuite/D/8/outBAT.txt 2>&1");
	
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_8_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_8_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_8_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_8_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_8_BAT passed\n");
}

// Test Case D_8_INT: In Interactive Mode, mysh allows for a single pipe to connect two processes
void program_D_8_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/8/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/8/myscript.sh"
	int fdO = open("testSuite/D/8/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/8/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout is redirected to "testSuite/D/8/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/D/8/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh > testSuite/D/8/outINT.txt < testSuite/D/8/myscript.sh 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_8_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_8_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_8_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_8_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_8_INT passed\n");
}

// Test Case D_9_BAT: In batch mode, Input/output redirection can occur anywhere in a command string. All words not 
// appearing after a < or > are passed to the specified program as arguments.
void program_D_9_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/D/9/BAT/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/D/9/BAT/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/D/9/BAT/myscript1.sh"
	// the stdout of the argument is redirected to "testSuite/D/9/outBAT.txt"
	// the stderr of the argument is redirected to stdout
	system("./mysh testSuite/D/9/BAT/myscript.sh >> testSuite/D/9/BAT/outBAT.txt 2>&1");

	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_9_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_9_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_9_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_9_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_9_BAT passed\n");
}

// Test Case D_9_INT: In Interactive Mode, Input/output redirection can occur anywhere in a command string. All 
// words not appearing after a < or > are passed to the specified program as arguments.
void program_D_9_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/D/9/INT/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/D/9/INT/myscript.sh"
	int fdO = open("testSuite/D/9/INT/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/D/9/INT/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout is redirected to "testSuite/D/9/INT/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/D/9/INT/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh << testSuite/D/9/INT/myscript.sh > testSuite/D/9/INT/myshOUT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case D_9_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case D_9_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case D_9_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case D_9_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case D_9_INT passed\n");
}

// Test Case E_1_BAT: In batch mode:
// The user’s home directory is obtained using getenv(“HOME”).
// When cd is called with no arguments, it changes the working directory to the home directory.
// Tokens beginning with ~/ are interpreted as relative to the home directory.
// The ~ is replaced with the home directory.
void program_E_1_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/E/1/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/E/1/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/E/1/myscript.sh"
	// the stdout and stderr of the argument is redirected to "testSuite/E/1/outBAT.txt"
    system("./mysh testSuite/E/1/myscript.sh > testSuite/E/1/outBAT.txt 2>&1");
	// another file descriptor with a seperate open call to the same file is used to write the present working directory.
	// the present working directory is dependent on the machine this code runs on, thus the pwdCommand() is used to 
	// manually write the present working directory into "testSuite/E/1/expBAT.txt", and chdir() is used to manually 
	// change directories.  
	int fdE2 = open("testSuite/E/1/expBAT.txt", O_RDWR | O_TRUNC);
	char *buffer = malloc(sizeof(char) * 101);
	size_t size = 101;
	while (getcwd(buffer, size) == NULL) {
		size += 100;
		buffer = Free(buffer);
		buffer = malloc(sizeof(char) * size);
	}
	write(fdE2, getenv("HOME"), strlen(getenv("HOME")));
	write(fdE2, "/random/~/hello/~ ~\n", 20);
	chdir(getenv("HOME"));
	pwdCommand(fdE2);
	close(fdE2);
	chdir(buffer);
	buffer = Free(buffer);
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case E_1_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case E_1_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case E_1_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case E_1_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case E_1_BAT passed\n");
}

// Test Case E_1_INT: In interactive mode, the code interprets the first token of a command or sub-command as the 
// program/built-in operation to execute
void program_E_1_INT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/E/1/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/E/1/myscript.sh"
	int fdO = open("testSuite/E/1/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/E/1/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout and stderr of mysh is redirected to "testSuite/E/1/outINT.txt" and will read commands from stdin,
	// where stdin is set to file "testSuite/E/1/myscript.sh"
    system("./mysh > testSuite/E/1/outINT.txt < testSuite/E/1/myscript.sh 2>&1");
	// another file descriptor with a seperate open call to the same file is used to write the present working directory.
	// the present working directory is dependent on the machine this code runs on, thus the pwdCommand() is used to 
	// manually write the present working directory into "testSuite/E/1/expINT.txt", and chdir() is used to manually 
	// change directories.  
	int fdE2 = open("testSuite/E/1/expINT.txt", O_RDWR | O_TRUNC);
	char *buffer = malloc(sizeof(char) * 101);
	size_t size = 101;
	while (getcwd(buffer, size) == NULL) {
		size += 100;
		buffer = Free(buffer);
		buffer = malloc(sizeof(char) * size);
	}
	write(fdE2, "Welcome to MySH 3.0\n", 20);
	write(fdE2, "mysh> ", 6);
	write(fdE2, getenv("HOME"), strlen(getenv("HOME")));
	write(fdE2, "/random/~/hello/~ ~\n", 20);
	write(fdE2, "mysh> ", 6);
	chdir(getenv("HOME"));
	write(fdE2, "mysh> ", 6);
	pwdCommand(fdE2);
	chdir(buffer);
	buffer = Free(buffer);
	char msg[21] = "mysh> mysh: exiting\n";
	write(fdE2, msg, 20);
	close(fdE2);

	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case E_1_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case E_1_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case E_1_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case E_1_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case E_1_INT passed\n");
}

// Test Case F_1_BAT: In batch mode:
// A process has exactly one STDIN and STDOUT. This can be inherited from the parent (mysh) or explicitly changed to a pipe 
// or opened file using dup2(). 
// Commands that specify more than one input or output source cannot be performed as requested, so mysh must either reject
// the command or choose which input or output source will be used.
// We will test commands like these only to make sure your project does not crash or do something unreasonable.
void program_F_1_BAT() {
	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the argument passed into mysh
	// exp.txt will contain the expected output of the argument passed into mysh
	int fdO = open("testSuite/F/1/BAT/outBAT.txt", O_RDONLY);
	int fdE = open("testSuite/F/1/BAT/expBAT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		if (fdO == -1) {
			printf("here\n");
		}
		if (fdE == -1) {
			printf("here\n");
		}
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with argument "testSuite/F/1/BAT/myscript.sh"
	// the stdout of the argument is redirected to "testSuite/F/1/outBAT.txt"
	// the stderr of the argument is redirected to stdout
	system("./mysh testSuite/F/1/BAT/myscript.sh >> testSuite/F/1/BAT/outBAT.txt 2>&1");

	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case F_1_BAT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case F_1_BAT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case F_1_BAT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case F_1_BAT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case F_1_BAT passed\n");
}

// Test Case F_1_INT: In Interactive Mode, 
// A process has exactly one STDIN and STDOUT. This can be inherited from the parent (mysh) or explicitly changed to a pipe 
// or opened file using dup2(). 
// Commands that specify more than one input or output source cannot be performed as requested, so mysh must either reject
// the command or choose which input or output source will be used.
// We will test commands like these only to make sure your project does not crash or do something unreasonable.
void program_F_1_INT() {
	// The bar.txt output file is deleted before the commands are executed
	remove("testSuite/F/1/INT/bar.txt");

	// open the out.txt file in read only mode and exp.txt file in read only mode
	// out.txt will contain the output of the shell commands inputted in "testSuite/F/1/INT/myscript.sh"
	// exp.txt will contain the expected output of the shell commands inputted in "testSuite/F/1/INT/myscript.sh"
	int fdO = open("testSuite/F/1/INT/outINT.txt", O_RDONLY);
	int fdE = open("testSuite/F/1/INT/expINT.txt", O_RDONLY);
	if (fdO == -1 || fdE == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}
	// mysh is called with no argument
	// the stdout is redirected to "testSuite/F/1/INT/outINT.txt" and will read commands from stdin
	// stdin is set to file "testSuite/F/1/INT/myscript.sh"
	// stderr is redirected to stdout
    system("./mysh < testSuite/F/1/INT/myscript.sh > testSuite/F/1/INT/outINT.txt 2>&1");
	char *lineO = NULL;
	char *lineE = NULL;
	while (true) {
		lineO = readOutput(fdO);
		lineE = readOutput(fdE);
		// if the output file (lineO) and expected output (lineE) are both NULL, then break out of the loop 
		// because they are both empty, thus are equal to each other.
		if (lineO == NULL && lineE == NULL) {
			break;
		}
		// if only one of the files is NULL, then the files are not equal to each other, thus Test Case F_1_INT failed.
		// Or if both files are not NULL, but the contents of the output file does not equal the contents of the 
		// expected file, then Test Case F_1_INT failed
		if (((lineO == NULL) ^ (lineE == NULL)) || (strcmp(lineO, lineE) != 0)) {
			close(fdO);
			close(fdE);
			printf("Test Case F_1_INT failed\n");
			lineO = Free(lineO);
			lineE = Free(lineE); 
			return;
		}
		lineO = Free(lineO);
		lineE = Free(lineE); 
	}
	// if the contents of the output and expected file are equal to each other, then Test Case F_1_INT passed.
	close(fdO);
	close(fdE);
	lineO = Free(lineO);
	lineE = Free(lineE); 
	printf("Test Case F_1_INT passed\n");

	// The bar.txt output file is deleted after the commands are executed
	remove("testSuite/F/1/INT/bar.txt");
}

int main() {
	setbuf(stdout, NULL);

	program_A_1();
	program_A_2_BAT();
	program_A_2_INT();

	program_B_1();
	program_B_2();
	program_B_3();

	program_C_1();
	program_C_2();
	program_C_3();
	program_C_4();

	program_D_1_BAT();
	program_D_1_INT();
	program_D_2_BAT();
	program_D_2_INT();
	program_D_3_BAT();
	program_D_3_INT();
	program_D_4_BAT();
	program_D_4_INT();
	program_D_5_INT();
	program_D_6_BAT();
	program_D_6_INT();
	program_D_7_BAT();
	program_D_7_BA2();
	program_D_7_INT();
	program_D_7_IN2();
	program_D_8_BAT();
	program_D_8_INT();
	program_D_9_BAT();
	program_D_9_INT();

	program_E_1_BAT();
	program_E_1_INT();

	program_F_1_BAT();
	program_F_1_INT();

    return 0;
}
