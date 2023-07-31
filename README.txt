Yash Patel - ypp10
Jasmit Singh - js3034

Extensions Implemented:
1.	Home Directory (~/)
2.	Directory Wildcards (*/*.c)

A.	Test Plan: 
		1.	Please refer to requirements.txt for all project requirements that were tested, as well as how our test suite (testSuite) is structured.
		2. 	Note that for every test case in testSuite, there is a doc.txt file that provides additional details about:
				a.	the test case
				b.	the shell commands
				c.	how mysh passes the test case

Implementation Difference:
1.	As discussed with Professor Menendez, our implementation of the pipeline should be accepted despite being different
	than the implementation in the "Additional Notes" announcement.
2.	Our implementation is:
		a.	Look through the pipeline for built-in commands and execute them first.
		b.	After executing all built-in commands in the pipleline, all other executables are then executed.
3. 	Professor Menendez has stated our approach is "reasonable and thus accepted".
4. 	Professor Menendez has explained to us personally after lecture that he "did not specify the additional notes in the pdf
	file. Thus, as long as the program does not crash and meets the original requirements, you should get a 100%".
	
