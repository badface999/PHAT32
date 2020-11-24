/*
  Name: Zane Malacara & Lawrence Wong
  ID:  & 1001587603
*/





#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>

#define MAX_NUM_ARGUMENTS 3

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
								// In this case  white space
							    // will separate the tokens on our command line
								 
#define MAX_COMMAND_SIZE 255    // The maximum command-line size

void bpb(int16_t BytesPerSec, int8_t SecPerClus, int16_t RsvdSecCnt, int8_t NumFats, int32_t BPB_FATz32)
{
	printf("BPB_BytesPerSec: %d\n", BytesPerSec);
	printf("BPB_BytesPerSec: %x\n\n", BytesPerSec);

	printf("BPB_SecPerClus: %d\n", SecPerClus);
	printf("BPB_SecPerClus: %x\n\n", SecPerClus);
	
	printf("BPB_RsvdSecCnt: %d\n", RsvdSecCnt);
	printf("BPB_RsvdSecCnt: %x\n\n", RsvdSecCnt);

	printf("BPB_NumFATS: %d\n", NumFats);
	printf("BPB_NumFATS: %x\n\n", NumFats);

	printf("BPB_FATz32: %d\n", BPB_FATz32);
	printf("BPB_FATz32: %x\n\n", BPB_FATz32);
}

struct __attribute__ ((__packed__)) DirectoryEntry 
{
	char DIR_Name[11];
	uint8_t DIR_Attr;
	uint8_t Unused[8];
	uint16_t DIR_FirstClusterHigh;
	uint8_t Unused2[4];
	uint16_t DIR_FirstClusterLow;
	uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

int main()
{
	char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );
	
	/* Made the variables static so that we can keep the data stored in them outside scope*/
	static FILE *fp;
	static int16_t BPB_BytesPerSec;
	static int8_t BPB_SecPerClus;
	static int16_t BPB_RsvdSecCnt;
	static int8_t BPB_NumFATS;
	static int32_t BPB_FATz32;
	while ( 1 )
	{
		// Print out the mfs promp
		printf("mfs> ");

		// Read the command from the commandline. The 
		// maximum command that will be read is MAX_COMMAND_SIZE
		// input something since fgets returns NULL when there
		// is no input
		while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

		/* Parse input */
		char *token[MAX_NUM_ARGUMENTS];

		int token_count = 0;

		// pointer to point to the token
		// parsed by strsep

		char *arg_ptr;

		char *working_str = strdup( cmd_str );

		// we are going to move the working_str pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end 

		char *working_root = working_str;

		// Tokenize the input stringswith whitespace used as the delimiter

		while( ( ( arg_ptr = strsep(&working_str, WHITESPACE) ) != NULL ) &&
					(token_count<MAX_NUM_ARGUMENTS))
		{
			token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
			if(strlen (token[token_count]) == 0)
			{
				token[token_count] = NULL;
			}
			token_count++;
		}

		// Now print the tokenized input as a debug check
		// \TODO Remove this code and replace with your FAT32 functinoality



		if(token[0] != NULL && token[1] != NULL && strcmp(token[0], "open" )  == 0 && strcmp(token[1], "fat32.img" ) == 0)
		{
			fp = fopen("fat32.img", "r+");


			fseek(fp, 11, SEEK_SET);
			fread(&BPB_BytesPerSec, 2, 1, fp);

			fseek(fp, 13, SEEK_SET);
			fread(&BPB_SecPerClus, 1, 1, fp);

			fseek(fp, 14, SEEK_SET);
			fread(&BPB_RsvdSecCnt, 2, 1, fp);

			fseek(fp, 16, SEEK_SET);
			fread(&BPB_NumFATS, 1, 1, fp);

			fseek(fp, 36, SEEK_SET);
			fread(&BPB_FATz32, 4, 1, fp);
			
		}

		/* Close File */
		else if(strcmp(token[0], "close") == 0)
		{
			fclose(fp);
			fp = NULL; //set the file pointer to NULL so that we can check later if the user has opened the img 

		}

		/* Prints bpb command*/
		else if(token[0] != NULL && strcmp(token[0], "bpb") == 0)
		{
			bpb(BPB_BytesPerSec, BPB_SecPerClus, BPB_RsvdSecCnt, BPB_NumFATS, BPB_FATz32);
		}
		
		/* Checks and tells the user that the img has not been opened yet */
		else if(fp == NULL && cmd_str != NULL)
		{
			printf("Error: First system image must be opened first\n");
		}
		
		/* Prints error message in case user enter an improper command after they open the img */ 
		else printf("Error: command not found\n");
		free(working_root);		
	}
	return 0;
}
