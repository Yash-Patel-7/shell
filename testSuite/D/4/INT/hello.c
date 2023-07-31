#include <stdio.h>
#include <stdlib.h>
#include "../../../../helper.c"

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

int main() {
    printf("you have successfully executed hello.c\n");
    return EXIT_SUCCESS;
}