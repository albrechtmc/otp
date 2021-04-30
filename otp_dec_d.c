/*********************************************************************
CS344 Introduction to Operating Systems
One Time Pad Program Created by Matthew Albrecht
August 10, 2019
This is the decoding program that is part of the one-time pad encryption
program.  This program communicates with otp-dec.  It receives an encrypted 
message and a key from otp-dec.  Then it decrypts the message using the key
and sends the decrypted message back to otp-dec.  This program cannot
communicate with otp-enc.
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

#define ARSIZE 100000//maximum size of the message
/******************************************************************
This is the decrypting function.  It decrypts the requested file
using a key passed to it.  It stores the output in an array.  The
maximum size of the transmission is set by the macro above.  The
decryption method is shown on the following website:
https://en.wikipedia.org/wiki/One-time_pad
******************************************************************/
char* decriptText(char* text, char* key, char* output)
{
	int i;
	//char keyBuffer[1000];
	memset(output, '\0', ARSIZE);
	//loop thorugh the whole message replaceing spaces
	for (i = 0; i < strlen(text); i++)
	{
		if (text[i] == ' ')//replace space characters with @
			text[i] = '@';
		if (text[i] == '\n')//replace the newline with a teminator symbol
			text[i] = '\0';
	}
	//loop through the message encripting it with the key
	for (i = 0; i < strlen(text); i++)
	{
		output[i] = (((text[i] - 64) - (key[i] - 64) + 27) % 27) + 64;
		if (output[i] == 64)//replace the @ synbol with a space
			output[i] = ' ';
	}
	return output;
}

void error(const char *msg) { perror(msg); exit(1); } // Error function used for reporting issues
//main function
int main(int argc, char *argv[])
{
	int pid = getpid();//pid of the parent function
	//socket variables
	int listenSocketFD, establishedConnectionFD, portNumber, charsRead;
	socklen_t sizeOfClientInfo;
	char buffer[1001];//buffer to hold incoming text
	char text[ARSIZE];//string to hold the encripted message
	char key[ARSIZE];//string to hold the key
	int i, j, keyFlag = 0;//counters
	memset(key, '\0', ARSIZE);//initialize the arrays
	memset(text, '\0', ARSIZE);
	struct sockaddr_in serverAddress, clientAddress;
	pid_t spawnpid = -5;//child process variable
	int childExitMethod = -5;

	if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); } // Check usage & args

	// Set up the address struct for this process (the server)
	memset((char *)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[1]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverAddress.sin_addr.s_addr = INADDR_ANY; // Any address is allowed for connection to this process

	// Set up the socket
	listenSocketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (listenSocketFD < 0) error("ERROR opening socket");

	// Enable the socket to begin listening
	if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to port
		error("ERROR on binding");
	listen(listenSocketFD, 5); // Flip the socket on - it can now receive up to 5 connections
	//loop to hold the parent process
	while (1)
	{
		//reset the arrays
		memset(key, '\0', ARSIZE);
		memset(text, '\0', ARSIZE);
		keyFlag = 0;

		// Accept a connection, blocking if one is not available until one connects
		sizeOfClientInfo = sizeof(clientAddress); // Get the size of the address for the client that will connect
		establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); // Accept
		//check if connection established
		if (establishedConnectionFD < 0) error("ERROR on accept");
		else
		{
			//create a single array to hold the test variable from the client
			char programTest[1] = { '\0' };
			recv(establishedConnectionFD, programTest, 1, 0); // Read the client's message from the socket
			//if the client sent 1, it is the encoding program
			if (programTest[0] == '2')
			{
				send(establishedConnectionFD, "2", 1, 0);
				spawnpid = fork();//spawn a child
			}
			else//encoding program attempted to connect (return an error)
			{
				send(establishedConnectionFD, "!", 1, 0);
				spawnpid = -5;//reject the connection
			}
		}
		switch (spawnpid)
		{
		case -1://error
			perror("Error!");
			exit(1);
			break;
		case 0://this is the child
		{
			int loopExit = 0;
			//int totalSize = 0;
			j = 0;
			//child process ID
			pid = getpid();
			//loop until the entire message is read
			while (loopExit == 0)
			{
				//reset the buffer
				memset(buffer, '\0', 1001);
				charsRead = recv(establishedConnectionFD, buffer, 1000, 0); // Read the client's message from the socket
				//if the initial character is a "!" exit the loop
				if (buffer[0] == 33)
				{
					loopExit = 1;
					//totalSize--;
				}
				//totalSize += strlen(buffer);

				if (charsRead < 0) error("ERROR reading from socket");
				//if keyflag is 0, the message is getting sent
				if (keyFlag == 0)
				{
					for (i = 0; i < strlen(buffer); i++)
					{
						text[j] = buffer[i];
						j++;
					}
				}
				else//if keyflag is 1, the key is getting sent
					for (i = 0; i < strlen(buffer); i++)
					{
						key[j] = buffer[i];
						j++;
					}
				//exit the loop when there is nothing more to read or the message is less than 1000 characters
				if (strlen(buffer) < 1000 || charsRead == 1)
				{
					if (keyFlag == 1)//once the key is sent exit the loop
						loopExit = 1;
					else//first set the keyflag to 1 to receive the key
					{
						keyFlag = 1;
						j = 0;
					}
				}
			}
			char output[ARSIZE];//array to hold the encripted message
			//if the key is smaller than the text, send an error
			if (strlen(key) < strlen(text))
			{
				fprintf(stderr, "Error, key must be longer than plaintext file.\n");
				send(establishedConnectionFD, "!", 1, 0); // Write to the server
				//exit process
				close(establishedConnectionFD);
				exit(1);
				break;
			}
			decriptText(text, key, output);//call the decripting function
			//if the encripting function found bad input, send an error
			if (output[0] == 33)
			{
				fprintf(stderr, "Error, bad character input.\n");
				send(establishedConnectionFD, "!", 1, 0); // Write to the server
				close(establishedConnectionFD);
				exit(1);
				break;
			}
			//send the encripted message to the server
			charsRead = send(establishedConnectionFD, output, strlen(output), 0); // Write to the server

			if (charsRead < 0) error("CLIENT: ERROR writing to socket");
			if (charsRead < strlen(output)) printf("CLIENT: WARNING: Not all data written to socket!\n");
			charsRead = send(establishedConnectionFD, "##", 2, 0); // Write to the server

			if (charsRead < 0) error("ERROR writing to socket");
			close(establishedConnectionFD); // Close the existing socket which is connected to the client
			exit(0);//exit child process
			break;
		}
		default://parent process
			wait(&childExitMethod);
			break;
		}
	}
	close(listenSocketFD); // Close the listening socket
	return 0; 
}
