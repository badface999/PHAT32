/*
  Name: Zane Malacara & Lawrence Wong
  ID: 1001565204 & 1001587603
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
#include <ctype.h>
#include <stdbool.h>
#include <limits.h>

#define MAX_NUM_ARGUMENTS 10

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
						   // so we need to define what delimits our tokens.   \
						   // Inssignment to expression with array type this case  white space                        \
						   // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

FILE *fp;
int16_t BPB_BytesPerSec;
int8_t BPB_SecPerClus;
int16_t BPB_RsvdSecCnt;
int8_t BPB_NumFATS;
int32_t BPB_FATz32;
int32_t Root_Directory_Address;

/*function to print bpb data */
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

//given block, look up into the furst FAT and retrn the block address of teh block in the file. if there's no further blocks -1
int16_t NextLB(uint32_t sector)
{
	uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
	int16_t val;
	fseek(fp, FATAddress, SEEK_SET);
	fread(&val, 2, 1, fp);
	return val;
}

//Parameters - current sector number that points to the block of data
//Returns - The value of the address for that block of data
//Description - finds the starting address of a block of data given the sector number corrosponding to that data block
int LBAToOffset(int32_t sector)
{
	return ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATz32 * BPB_BytesPerSec);
}

bool compare(char *IMG_Name, char *input) //function is supposed to parse and make sure we don't get garbage for the print out
{
	char expanded_name[12];
	char temp[13];		 //temp char array for the copy of the input that we'll have for when we use compare
	strcpy(temp, input); //making copy
	memset(expanded_name, ' ', 12);

	char *token = strtok(temp, ".");
	strncpy(expanded_name, token, strlen(token));
	token = strtok(NULL, ".");

	if (token)
	{
		strncpy((char *)(expanded_name + 8), token, strlen(token));
	}

	expanded_name[11] = '\0';

	int i;
	for (i = 0; i < 11; i++)
	{
		expanded_name[i] = toupper(expanded_name[i]);
	}

	return strncmp(expanded_name, IMG_Name, 11);
}
struct __attribute__((__packed__)) DirectoryEntry
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
	char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);
	int i, j;
	int8_t val;
	char parse[12];
	int next_address, read_address;
	int counter, size;
	/* Made the variables static so that we can keep the data stored in them outside scope*/

	while (1)
	{
		// Print out the mfs promp
		printf("mfs> ");

		// Read the command from the commandline. The
		// maximum command that will be read is MAX_COMMAND_SIZE
		// input something since fgets returns NULL when there
		// is no input
		while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin))
			;

		/* Parse input */
		char *token[MAX_NUM_ARGUMENTS];

		int token_count = 0;

		// pointer to point to the token
		// parsed by strsep

		char *arg_ptr;

		char *working_str = strdup(cmd_str);

		// we are going to move the working_str pointer so
		// keep track of its original value so we can deallocate
		// the correct amount at the end

		char *working_root = working_str;

		// Tokenize the input stringswith whitespace used as the delimiter

		while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
			   (token_count < MAX_NUM_ARGUMENTS))
		{
			token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
			if (strlen(token[token_count]) == 0)
			{
				token[token_count] = NULL;
			}
			token_count++;
		}

		// Now print the tokenized input as a debug check
		// \TODO Remove this code and replace with your FAT32 functinoality
		if (token[0] != NULL)
		{

			if (token[0] != NULL && token[1] != NULL && strcmp(token[0], "open") == 0 && strcmp(token[1], "fat32.img") == 0 && fp == NULL)
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

				/*
				 * Getting the root directory address and then we are fseeking to that address.
				 *
				 * Then we read a directory entry 16 times sequentially store the data into the dir array of structs
				 */
				Root_Directory_Address = (BPB_NumFATS * BPB_FATz32 * BPB_BytesPerSec) + (BPB_RsvdSecCnt * BPB_BytesPerSec);
				fseek(fp, Root_Directory_Address, SEEK_SET);
				fread(dir, 16, sizeof(struct DirectoryEntry), fp);
			}

			/* Checks and tells the user that the img has not been opened yet */
			else if (fp == NULL)
			{
				//in case they want to exit or quit before opening anything at all
				if (strcmp(token[0], "quit") == 0 || strcmp(token[0], "exit") == 0)
				{
					exit(0);
				}
				else
				{
					printf("Error: First system image must be opened first\n");
					continue;
				}
			}
			/* Close File */
			else if (strcmp(token[0], "close") == 0)
			{
				fclose(fp);
				fp = NULL; //set the file pointer to NULL so that we can check later if the user has opened the img
			}

			/* Prints bpb command*/
			else if (token[0] != NULL && strcmp(token[0], "bpb") == 0)
			{
				bpb(BPB_BytesPerSec, BPB_SecPerClus, BPB_RsvdSecCnt, BPB_NumFATS, BPB_FATz32);
			}

			/* 
			 * Statement for when the user wants to list what's in the current directory.
			 *
			 * We will iterate a for loop 16 times becuase that's how many "files" can be 
			 * can be in a directory.
			 *
			 * We then print out the name of the "file" at each iteration. 
			 */
			else if (strcmp(token[0], "ls") == 0)
			{
				for (i = 0; i < 16; i++) //16 since we're reading all the blocks
				{
					//extra garbage in the print and apparently in the folder has junk
					if (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 || dir[i].DIR_Attr == 0x20)
					{
						//printf("This is the orginal thing - %s\n", dir[i].DIR_Name);
						strncpy(parse, dir[i].DIR_Name, 11);
						parse[11] = '\0'; //NULL terminating string to get rid of garbage
						printf("%s\n", parse);
					}
				}
			}
			else if (strcmp(token[0], "cd") == 0) //cd only and that means that we're going back into the home directory
			{
				if (token[1] == NULL) //meaning that we don't have a destination to go to
				{
					fseek(fp, Root_Directory_Address, SEEK_SET);
					fread(dir, 16, sizeof(struct DirectoryEntry), fp);
				}
				else if (token[1] != NULL)
				{
					/*
					 * We check each index of the array of structs and try to find one that is a subdirectory, meaning that the attribute at the index
					 * is 0x10 in hex. If this is true the we compare that with where the user wants to cd into, and if they are the same we find the address
					 * by passing the two values int LBAToOffset to find the starting address of the directory. Then we fseek the address and then read 
					 * and update the array of structs.
					 */
					counter = 0;
					for (i = 0; i < 16; i++) //going through all the blocks
					{
						if (dir[i].DIR_Attr == 0x10) //need to check if the user's input matches the name
						{
							if (strcmp(token[1], "..") == 0)
							{
								/*
								 * Check if the first and second character are 0x2e and if they are, that means that the cluster number
								 * will be that of the parent directory.
								 *
								 * We use that cluster number and pass it into the LBAToOffset to get the address of the parent 
								 * directory and we update the array of structs by fseeking to the address and then reading the contents.
								 */

								if (dir[i].DIR_Name[0] == 0x2e && dir[i].DIR_Name[1] == 0x2e)
								{
									if (dir[i].DIR_FirstClusterLow == 0x0000) //if this is true that means the parent directory is the root directory
									{
										fseek(fp, Root_Directory_Address, SEEK_SET);
										fread(dir, 16, sizeof(struct DirectoryEntry), fp);
									}
									else
									{
										next_address = LBAToOffset(dir[i].DIR_FirstClusterLow);
										fseek(fp, next_address, SEEK_SET);
										fread(dir, 16, sizeof(struct DirectoryEntry), fp);
									}
								}
							}
							else if (compare(dir[i].DIR_Name, token[1]) == 0) //Passes values into compare function which checks to see if what the user enters matches the name
							{
								next_address = LBAToOffset(dir[i].DIR_FirstClusterLow);
								fseek(fp, next_address, SEEK_SET);
								fread(dir, 16, sizeof(struct DirectoryEntry), fp);
							} //set a variable to this since this is a name for the folder
							else if (compare(dir[i].DIR_Name, token[1]) == 1)
							{
								counter += 1;
							}
						}
					}
					if (counter == 1 || counter == 3)
					{
						printf("Error: Unable to find directory ");
						for (i = 1; token[i] != NULL && i < MAX_NUM_ARGUMENTS; i++)
						{
							printf("%s ", token[i]);
						}
						printf("\n");
					}
				}
			}
			else if (strcmp(token[0], "quit") == 0 || strcmp(token[0], "exit") == 0) //these are the commands to exit from the program
			{
				exit(0);
			}
			else if (strcmp(token[0], "stat") == 0) //user wants to get stat of the filename or directory
			{
				if (token[1] != NULL)
				{
					for (i = 0; i < 16; i++)
					{
						if (compare(dir[i].DIR_Name, token[1]) == 0)
						{
							if (dir[i].DIR_Attr == 0x10)
							{
								printf("File Attribute\tSize\tStarting Cluster Number\n");
								printf("%d\t\t0\t%d\n", dir[i].DIR_Attr, dir[i].DIR_FirstClusterLow);
							}
							else //finds a file that matches with what the user typed in
							{
								printf("File Attribute\tSize\tStarting Cluster Number\n");
								printf("%d\t\t%d\t%d\n", dir[i].DIR_Attr, dir[i].DIR_FileSize, dir[i].DIR_FirstClusterLow);
							}
						}
					}
				}
			}
			else if (strcmp(token[0], "read") == 0)
			{
				if (token[2] == NULL || token[3] == NULL || token[1] == NULL) //this should account for when the user doesn't enter anything for the file or where to start and end
				{
					printf("Error: Please use right format\n");
				}
				else if (token[1] != NULL)
				{
					for (i = 0; i < 16; i++)
					{
						if (compare(dir[i].DIR_Name, token[1]) == 0 && dir[i].DIR_Attr != 0x10) //checks if we've have the file and it exists + not a subdirectory
						{
							int startpos = atoi(token[2]);
							int endpos = atoi(token[3]); //end postion
							int cluster = dir[i].DIR_FirstClusterLow;
							int totalprint = endpos - startpos;		 //this will make
							while (cluster != -1 && totalprint != 0) //will look through all the clusters and ensures that we only print out the specified amount and not more
							{
								read_address = LBAToOffset(cluster);		  //gets address
								fseek(fp, read_address + startpos, SEEK_SET); //fseeking to the starting point of where the user wants to see bytes
								for (j = startpos; j < endpos; j++)
								{
									fread(&val, 1, 1, fp);
									printf("%x ", val); //we'll be printing out the hexadecimal of what values we got from
									totalprint--;		//decrementing so that we can make sure that we don't print more than specified
								}
								// 1 = 0x1
								printf("\n");
								cluster = NextLB(cluster); //moves onto the next cluster
							}
							if (compare(dir[i].DIR_Name, token[1]) == 0 && dir[i].DIR_Attr != 0x10)
							{
								continue;
							}
							else if (compare(dir[i].DIR_Name, token[1]) != 0 || dir[i].DIR_Attr == 0x10)
							{
								printf("Error: Please use right format.\n");
								continue;
							}
						}
					}
				}
			}

			/* Prints error message in case user enter an improper command after they open the img */
			else
				printf("Error: command not found\n");
		}

		free(working_root);
	}
	return 0;
}
