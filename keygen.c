/*********************************************************************
CS344 Introduction to Operating Systems
One Time Pad Program Created by Matthew Albrecht
August 10, 2019
This is the key generating program that is part of the one-time pad 
encryption program.  It generates a key of random letters and spaces 
of the requested length.  It outputs the key to stdout.
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[])
{
	int numChars, i;
	char c;
	//seed the random number generator
	srand(time(0));
	//check for the correct number of arguments
	if (argc != 2)
	{
		printf("Error: Incorrect number of arguments.");
		return 1;
	}
	//convert the string to an integer
	numChars = atoi(argv[1]);
	//check for valid input
	if (numChars == 0)
	{
		printf("Error: Please enter an integer.");
		return 1;
	}
	//print the output to standard out
	for (i = 0; i < numChars; i++)
	{
		//generatre a random number as the ASCII value of a character
		//64 != @ it equals " " for this program
		c = rand() % 27 + 64;
		if (c == 64)
			printf(" ");
		else
			printf("%c", c);
	}
	//end the file with a newline character
	printf("\n");
	return 0;
}