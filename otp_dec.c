/*********************************************************************
CS344 Introduction to Operating Systems
One Time Pad Program Created by Matthew Albrecht
August 10, 2019
This is the decoding program that is part of the one-time pad encryption
program.  This program communicates with otp-dec-d, and sends an encrypted
message and a decrypting key, and then receives the decrypted message in
return.  The decrypted message is written to stdout.  This program cannot
communicate with otp-enc-d.
**********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>

void error(const char *msg) { perror(msg); exit(0); } // Error function used for reporting issues

int main(int argc, char *argv[])
{
	//create the constants for the network
	int socketFD, portNumber, charsWritten, charsRead;
	struct sockaddr_in serverAddress;
	struct hostent* serverHostInfo;
	char buffer[1002];//the buffer to hold all input
	ssize_t readSize;
    
	if (argc < 3) { fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); exit(0); } // Check usage & args

	// Set up the server address struct (taken from the example)
	memset((char*)&serverAddress, '\0', sizeof(serverAddress)); // Clear out the address struct
	portNumber = atoi(argv[3]); // Get the port number, convert to an integer from a string
	serverAddress.sin_family = AF_INET; // Create a network-capable socket
	serverAddress.sin_port = htons(portNumber); // Store the port number
	serverHostInfo = gethostbyname(/*argv[1]*/"localhost"); // Convert the machine name into a special form of address
	if (serverHostInfo == NULL) { fprintf(stderr, "CLIENT: ERROR, no such host\n"); exit(0); }
	memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length); // Copy in the address

	// Set up the socket
	socketFD = socket(AF_INET, SOCK_STREAM, 0); // Create the socket
	if (socketFD < 0) error("CLIENT: ERROR opening socket");
	
	// Connect to server
	if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) // Connect socket to address
		error("CLIENT: ERROR connecting");

	//check for otp_dec_d, and exit if connecting to the wrong program
	send(socketFD, "2", 1, 0);//send "2" and check the return value
	char programTest[1] = { '\0' };
	recv(socketFD, programTest, 1000, 0);
	if (programTest[0] == 33)
	{
		fprintf(stderr, "Error, otp_dec cannot connect to otp_enc_d on port %d.\n", portNumber);
		close(socketFD); // Close the socket
		return 2;//return the correct error value
	}

	//int sizeSent = 0;
	// open the encripted file and check if it exists
	int fileDescriptor = open(argv[1], O_RDONLY);
	if (fileDescriptor < 0)
	{
		fprintf(stderr, "Error, file %s does not exist.\n", argv[1]);
		return 1;
	}
	int loopExit = 0;
	//loop until less than 1000 characters are read
	while (loopExit == 0)
	{
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
		//read 1000 characters from the file
		read(fileDescriptor, buffer, 1000);
		buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n that fgets adds
		if (strlen(buffer) < 1000)
		{
			//add a newline character at the end
			buffer[strlen(buffer)] = '\n';
			loopExit = 1;
		}

		// Send message to server
		charsWritten = send(socketFD, buffer, 1000/*strlen(buffer)*/, 0); // Write to the server
		//sizeSent += strlen(buffer);
		//check for errors (from the exmple)
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");
		
	}
	close(fileDescriptor);
	//printf("Total sent: %d\n", sizeSent);
	
	//open the key
	fileDescriptor = open(argv[2], O_RDONLY);
	if (fileDescriptor < 0)
	{
		fprintf(stderr, "Error, key file does not exist.\n");
		return 1;
	}
	loopExit = 0;
	//sizeSent = 0;
	while (loopExit == 0)
	{
		memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer array
		//read 1000 characters from the key
		readSize = read(fileDescriptor, buffer, 1000);
		buffer[strcspn(buffer, "\n")] = '\0'; // Remove the trailing \n
		
		// Send message to server
		charsWritten = send(socketFD, buffer, strlen(buffer), 0); // Write to the server

		//check for errors (from the exmple)
		if (charsWritten < 0) error("CLIENT: ERROR writing to socket");
		if (charsWritten < strlen(buffer)) printf("CLIENT: WARNING: Not all data written to socket!\n");

		//break the loop and signal the end of the transmission
		if (strlen(buffer) < 1000 || readSize == 1)
		{
			//printf("Readsize is 1\n");
			send(socketFD, "!", 1, 0); // Write to the server
			break;
		}
	}
	close(fileDescriptor);

	//Get return message from server
	loopExit = 0;
	//loop until "##" is read
	while (loopExit == 0)
	{
		memset(buffer, '\0', sizeof(buffer));
		charsRead = recv(socketFD, buffer, 1000, 0); // Read the client's message from the socket
		//if the first character is a "!", there was an error decoding
		if (buffer[0] == 33)
		{
			close(socketFD); // Close the socket
			return 1;//return an error
		}
		
		if (charsRead < 0) error("ERROR reading from socket");
		//if the last two characters of the transmission are "##", it is the end of the transmission
		if (buffer[strlen(buffer) - 1] == '#')
		{
			if (buffer[strlen(buffer) - 2] == '#')
			{
				buffer[strlen(buffer) - 2] = '\0';
				loopExit = 1;
			}
		}
		//print out the transmission
		printf("%s", buffer);
	}

	memset(buffer, '\0', sizeof(buffer)); // Clear out the buffer again for reuse
	if (charsRead < 0) error("CLIENT: ERROR reading from socket");
	//add the final newline character
	printf("\n");
	close(socketFD); // Close the socket
	return 0;
}