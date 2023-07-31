#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv) {
    write(STDOUT_FILENO, "this is written to stdout from pipesOUT.c\n", 42);
    if (argc == 1) {
		 return EXIT_SUCCESS;
	}
    return EXIT_FAILURE;
}