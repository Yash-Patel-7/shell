#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
	printf("This is the current directory's echo file.\n");
	for (int i = 0; i < argc; i++) {
		printf("%s ", argv[i]);
	}
	printf("\n");
	return EXIT_SUCCESS;
}
