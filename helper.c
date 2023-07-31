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

// prototypes of all functions
void* Free(void *ptr);
char** strTokenize(const char *str, const char *delimiters, size_t *numOfTokens, const char *specialTokens);
char* strStrip(const char *str, const char* delimiters);
void* freeStrTokens(char **tokens, size_t numOfTokens);
void printStrTokens(char **tokens, size_t numOfTokens, const char *delimiter);
char* strCombineTokens(char **tokens, size_t numOfTokens, const char *delimiter);
char* strReplace(const char *str, const char *oldSubStr, const char *newSubStr, ssize_t numOfOccurrences);
void* freeArrayOfStrings(char **array, size_t numOfStrings);
char* strdup(const char *str);
char** strDupArrayOfStrings(char **array, size_t numOfStrings);

// define free function that changes the pointer to NULL after freeing
void* Free(void *ptr) {
	free(ptr);
	return NULL;
}

// function that returns a list of tokens from a string using a list of delimiter characters
// for example: if string is "   hel lo wor ld   " and delimiters are " l" then the list of tokens will be ["he", "o", "wor", "d"]
// special tokens are characters that are considered tokens by themselves
// so if string is "hello" and special tokens are "l" with delimiters " " then list of tokens will be ["he", "l", "l", "o"]
char** strTokenize(const char *str, const char *delimiters, size_t *numOfTokens, const char *specialTokens) {
	// if str, delimiters, numOfTokens, or specialTokens is NULL, then return NULL
	if (str == NULL || delimiters == NULL || numOfTokens == NULL || specialTokens == NULL) {
		return NULL;
	}

	// if any character in specialTokens is also in delimiters, then return NULL
	// use strchr to check if any character in specialTokens is also in delimiters
	size_t specialTokensLen = strlen(specialTokens);
	for (size_t i = 0; i < specialTokensLen; i++) {
		if (strchr(delimiters, specialTokens[i]) != NULL) {
			return NULL;
		}
	}

	// initialize numOfTokens to 0
	*numOfTokens = 0;

	// if str is empty, then return NULL and numOfTokens is 0
	if (strlen(str) == 0) {
		return NULL;
	}
	
	// strip leading and trailing delimiters from str and save it to a new string
	char *newStr = strStrip(str, delimiters);

	// if newStr is NULL, then return NULL and numOfTokens is 0
	if (newStr == NULL) {
		return NULL;
	}

	// get the length of newStr
	size_t newStrLen = strlen(newStr);

	// if newStr is empty, then return NULL and numOfTokens is 0
	if (newStrLen == 0) {
		newStr = Free(newStr);
		return NULL;
	}

	// at this point newStr is stripped of leading and trailing delimiters
	// so now we can tokenize newStr
	// iterate through newStr and count the number of tokens
	// a token is considered the non-delimiter string between 2 delimiters so 
	// a string "hello       world" with delimiters " " will have 2 tokens, "hello" and "world"
	// a string "hello world" with delimiters "l" will have 3 tokens, "he", "o wor", "d"
	// consider special tokens as tokens by themselves
	// the number of special tokens in the string is obtained by counting the number of special tokens in newStr
	// so newStr is "hello" with special "lb" and delimiters " " then the number of total tokens is 4 because the "l" is considered a token by itself
	// so you skip non-delimiter AND non-special tokens so that you can handle special tokens as tokens by themselves
	for (size_t i = 0; i < newStrLen; i++) {
		// if current character is a delimiter, then skip it
		if (strchr(delimiters, newStr[i]) != NULL) {
			continue;
		}

		// if current character is a special token, then increment numOfTokens and skip it
		if (strchr(specialTokens, newStr[i]) != NULL) {
			(*numOfTokens)++;
			continue;
		}

		// if current character is not a delimiter or special token, then increment numOfTokens
		// and skip all non-delimiter and non-special tokens
		(*numOfTokens)++;
		while (i < newStrLen && strchr(delimiters, newStr[i]) == NULL && strchr(specialTokens, newStr[i]) == NULL) {
			i++;
		}
		i--;
	}
	
	// if numOfTokens is 0, then return NULL
	if (*numOfTokens == 0) {
		newStr = Free(newStr);
		return NULL;
	}

	// allocate memory for tokens
	char **tokens = malloc(sizeof(char*) * (*numOfTokens + 1));

	// make tokens a NULL terminated list of strings
	tokens[*numOfTokens] = NULL;

	// iterate through newStr and save each token substring to tokens
	// using same process as above but this time also calculate size of each token before allocating memory for it
	size_t tokenIndex = 0;
	for (size_t i = 0; i < newStrLen; i++) {
		// if current character is a delimiter, then skip it
		if (strchr(delimiters, newStr[i]) != NULL) {
			continue;
		}

		// if current character is a special token, then save it to tokens and skip it
		if (strchr(specialTokens, newStr[i]) != NULL) {
			tokens[tokenIndex] = malloc(sizeof(char) * 2);
			tokens[tokenIndex][0] = newStr[i];
			tokens[tokenIndex][1] = '\0';
			tokenIndex++;
			continue;
		}

		// if current character is not a delimiter or special token, then save it to tokens
		// and skip all non-delimiter and non-special tokens
		size_t tokenLen = 0;
		while (i < newStrLen && strchr(delimiters, newStr[i]) == NULL && strchr(specialTokens, newStr[i]) == NULL) {
			tokenLen++;
			i++;
		}
		i--;
		tokens[tokenIndex] = malloc(sizeof(char) * (tokenLen + 1));
		strncpy(tokens[tokenIndex], newStr + i - tokenLen + 1, tokenLen);
		tokens[tokenIndex][tokenLen] = '\0';
		tokenIndex++;
	}

	// free newStr
	newStr = Free(newStr);

	// return tokens
	return tokens;
}

// function that strips leading and trailing delimiters from a string
char* strStrip(const char *str, const char *delimiters) {
	// if str is NULL, then return NULL
	if (str == NULL) {
		return NULL;
	}

	// if delimiters is NULL or empty, then return a copy of str
	if (delimiters == NULL || strlen(delimiters) == 0 || strlen(str) == 0) {
		char *newStr = malloc(sizeof(char) * (strlen(str) + 1));
		strcpy(newStr, str);
		return newStr;
	}

	// get number of leading delimiters
	size_t leadingDelimiters = 0;
	size_t strLen = strlen(str);
	for (size_t i = 0; i < strLen; i++) {
		if (strchr(delimiters, str[i]) != NULL) {
			leadingDelimiters++;
		} else {
			break;
		}
	}

	// get number of trailing delimiters
	size_t trailingDelimiters = 0;
	for (ssize_t i = strLen - 1; i >= 0; i--) {
		if (strchr(delimiters, str[i]) != NULL) {
			trailingDelimiters++;
		} else {
			break;
		}
	}

	// if leadingDelimiters + trailingDelimiters >= strLen, then return new allocated empty string
	if (leadingDelimiters + trailingDelimiters >= strLen) {
		char *newStr = malloc(sizeof(char));
		newStr[0] = '\0';
		return newStr;
	}

	// calculate newStrLen
	size_t newStrLen = strLen - leadingDelimiters - trailingDelimiters;

	// allocate memory for newStr
	char *newStr = malloc(sizeof(char) * (newStrLen + 1));

	// copy str to newStr
	for (size_t i = 0; i < newStrLen; i++) {
		newStr[i] = str[i + leadingDelimiters];
	}

	// add null terminator to newStr
	newStr[newStrLen] = '\0';

	// return newStr
	return newStr;
}

// function that frees tokens returned by strTokenize
void* freeStrTokens(char **tokens, size_t numOfTokens) {
	// iterate through tokens and free each token
	for (size_t i = 0; i < numOfTokens; i++) {
		tokens[i] = Free(tokens[i]);
	}
	// free tokens
	tokens = Free(tokens);
	// return NULL
	return NULL;
}

// function that prints tokens using a delimiter given
void printStrTokens(char **tokens, size_t numOfTokens, const char *delimiter) {
	// iterate through tokens and print each token
	for (size_t i = 0; i < numOfTokens; i++) {
		printf("%s", tokens[i]);
		// if i is not the last token, then print delimiter
		if (i != numOfTokens - 1) {
			printf("%s", delimiter);
		}
	}
	// print new line
	printf("\n");
}

// function that combines tokens into a new allocated string using a delimiter given
char* strCombineTokens(char **tokens, size_t numOfTokens, const char *delimiter) {
	// if tokens is NULL or numOfTokens is 0, then return NULL
	if (tokens == NULL || numOfTokens == 0) {
		return NULL;
	}

	// if delimiter is NULL, then set it to empty string
	if (delimiter == NULL) {
		return strCombineTokens(tokens, numOfTokens, "");
	}

	// calculate size of newStr
	size_t newStrSize = 0;
	for (size_t i = 0; i < numOfTokens; i++) {
		newStrSize += strlen(tokens[i]);
	}

	// add size of delimiters
	newStrSize += strlen(delimiter) * (numOfTokens - 1);

	// allocate memory for newStr
	char *newStr = malloc(sizeof(char) * (newStrSize + 1));

	// copy tokens to newStr with delimiters
	// make sure to create variable for strlen() for efficiency
	size_t newStrIndex = 0;
	size_t delimiterLen = strlen(delimiter);
	for (size_t i = 0; i < numOfTokens; i++) {
		size_t tokenLen = strlen(tokens[i]);
		for (size_t j = 0; j < tokenLen; j++) {
			newStr[newStrIndex] = tokens[i][j];
			newStrIndex++;
		}
		// if i is not the last token, then add delimiter
		if (i != numOfTokens - 1) {
			for (size_t j = 0; j < delimiterLen; j++) {
				newStr[newStrIndex] = delimiter[j];
				newStrIndex++;
			}
		}
	}

	// add null terminator to newStr
	newStr[newStrSize] = '\0';

	// return newStr
	return newStr;
}

// function that replaces a substring with another substring up to a given number of occurrences and returns new allocated string
// if numOfOccurrences is -1, then replace all occurrences
char* strReplace(const char *str, const char *oldSubStr, const char *newSubStr, ssize_t numOfOccurrences) {
	// if str is NULL, then return NULL
	if (str == NULL) {
		return NULL;
	}

	// if oldSubStr is NULL or empty or numOfOccurrences is 0, then return a copy of str
	if (oldSubStr == NULL || strlen(oldSubStr) == 0 || numOfOccurrences == 0) {
		char *newStr = malloc(sizeof(char) * (strlen(str) + 1));
		strcpy(newStr, str);
		return newStr;
	}

	// if newSubStr is NULL, then set it to empty string
	if (newSubStr == NULL) {
		return strReplace(str, oldSubStr, "", numOfOccurrences);
	}

	// calculate size of newStr
	size_t newStrSize = 0;
	size_t oldSubStrLen = strlen(oldSubStr);
	size_t newSubStrLen = strlen(newSubStr);
	size_t strLen = strlen(str);

	// save numOfOccurrences in a variable to avoid modifying it
	ssize_t numOfOccurrencesCopy = numOfOccurrences;
	
	// if numOfOccurrences is -1, then replace all occurrences, otherwise replace numOfOccurrences occurrences
	// when old substring is found, then add newSubStrLen to newStrSize and skip oldSubStrLen characters in str
	// when old substring is not found, then add 1 to newStrSize and skip 1 character in str
	if (numOfOccurrences == -1) {
		for (size_t i = 0; i < strLen; i++) {
			if (strncmp(str + i, oldSubStr, oldSubStrLen) == 0) {
				newStrSize += newSubStrLen;
				i += oldSubStrLen - 1;
			} else {
				newStrSize++;
			}
		}
	} else {
		for (size_t i = 0; i < strLen; i++) {
			if (strncmp(str + i, oldSubStr, oldSubStrLen) == 0) {
				newStrSize += newSubStrLen;
				i += oldSubStrLen - 1;
				numOfOccurrencesCopy--;
				if (numOfOccurrencesCopy == 0) {
					newStrSize += strLen - i - 1;
					break;
				}
			} else {
				newStrSize++;
			}
		}
	}

	// reset numOfOccurrencesCopy
	numOfOccurrencesCopy = numOfOccurrences;

	// allocate memory for newStr
	char *newStr = malloc(sizeof(char) * (newStrSize + 1));

	// copy str to newStr replacing oldSubStr with newSubStr
	// use same process as above
	size_t newStrIndex = 0;
	if (numOfOccurrences == -1) {
		for (size_t i = 0; i < strLen; i++) {
			if (strncmp(str + i, oldSubStr, oldSubStrLen) == 0) {
				for (size_t j = 0; j < newSubStrLen; j++) {
					newStr[newStrIndex] = newSubStr[j];
					newStrIndex++;
				}
				i += oldSubStrLen - 1;
			} else {
				newStr[newStrIndex] = str[i];
				newStrIndex++;
			}
		}
	} else {
		for (size_t i = 0; i < strLen; i++) {
			if (strncmp(str + i, oldSubStr, oldSubStrLen) == 0) {
				for (size_t j = 0; j < newSubStrLen; j++) {
					newStr[newStrIndex] = newSubStr[j];
					newStrIndex++;
				}
				i += oldSubStrLen - 1;
				numOfOccurrencesCopy--;
				if (numOfOccurrencesCopy == 0) {
					for (size_t j = i + 1; j < strLen; j++) {
						newStr[newStrIndex] = str[j];
						newStrIndex++;
					}
					break;
				}
			} else {
				newStr[newStrIndex] = str[i];
				newStrIndex++;
			}
		}
	}

	// add null terminator to newStr
	newStr[newStrSize] = '\0';

	// return newStr
	return newStr;
}

// function that frees an array of strings
void* freeArrayOfStrings(char **array, size_t numOfStrings) {
	// free each string in array
	for (size_t i = 0; i < numOfStrings; i++) {
		array[i] = Free(array[i]);
	}

	// free array
	array = Free(array);

	// return NULL
	return NULL;
}

// function that duplicates a string
char* strdup(const char *str) {
	// if str is NULL, then return NULL
	if (str == NULL) {
		return NULL;
	}

	// allocate memory for newStr
	char *newStr = malloc(sizeof(char) * (strlen(str) + 1));

	// if newStr is NULL, then return NULL
	if (newStr == NULL) {
		return NULL;
	}

	// copy str to newStr
	strcpy(newStr, str);

	// return newStr
	return newStr;
}

// function that duplicates an array of strings
char** strDupArrayOfStrings(char **array, size_t numOfStrings) {
	// if array is NULL or numOfStrings is 0, then return NULL
	if (array == NULL || numOfStrings == 0) {
		return NULL;
	}

	// allocate memory for newArray
	char **newArray = malloc(sizeof(char*) * numOfStrings);

	// if newArray is NULL, then return NULL
	if (newArray == NULL) {
		return NULL;
	}

	// copy each string in array to newArray
	for (size_t i = 0; i < numOfStrings; i++) {
		newArray[i] = strdup(array[i]);
	}

	// return newArray
	return newArray;
}
