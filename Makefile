all: cleanExec mysh test echo hello pipesIN pipesOUT cleanDSYM

clean: cleanExec cleanDSYM

mysh: mysh.c
	gcc -g -Wall -Werror -fsanitize=address -std=c99 mysh.c -o mysh

test: test.c
	gcc -g -Wall -Werror -fsanitize=address -std=c99 test.c -o test

echo: testSuite/D/6/INT/echo.c
	gcc -g -Wall -Werror -fsanitize=address -std=c99 testSuite/D/6/INT/echo.c -o testSuite/D/6/INT/echo

hello: testSuite/D/4/INT/hello.c
	gcc -g -Wall -Werror -fsanitize=address -std=c99 testSuite/D/4/INT/hello.c -o testSuite/D/4/INT/hello

pipesIN: testSuite/D/8/pipesIN.c
	gcc -g -Wall -Werror -fsanitize=address -std=c99 testSuite/D/8/pipesIN.c -o testSuite/D/8/pipesIN

pipesOUT: testSuite/D/8/pipesOUT.c
	gcc -g -Wall -Werror -fsanitize=address -std=c99 testSuite/D/8/pipesOUT.c -o testSuite/D/8/pipesOUT

cleanExec:
	rm -rf mysh && rm -rf test && rm -rf testSuite/D/6/INT/echo && rm -rf testSuite/D/4/INT/hello && rm -rf testSuite/D/8/pipesIN && rm -rf testSuite/D/8/pipesOUT

cleanDSYM:
	rm -rf mysh.dSYM && rm -rf test.dSYM && rm -rf testSuite/D/6/INT/echo.dSYM && rm -rf testSuite/D/4/INT/hello.dSYM && rm -rf testSuite/D/8/pipesIN.dSYM && rm -rf testSuite/D/8/pipesOUT.dSYM

