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

#define WHITESPACE " \t\n"
/* We want to split our command line up into tokens                          
// so we need to define what delimits our tokens. 
// In this case  white space                        
 will separate the tokens on our command line
*/
#define MAX_COMMAND_SIZE 255 // The maximum command-line size

#define MAX_NUM_ARGUMENTS 11 // Mav shell only supports five arguments (changed to 11 to have 10 additional arguments)

struct __attribute__((__packed__)) DirectoryEntry //each record can be represented by
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};
struct DirectoryEntry dir[16];
/*
//given block, look up into the furst FAT and retrn the block address of teh block in the file. if there's no further blocks -1
int16_t NextLB(uint32_t sector)
{g
  uint32_t FATAddress = (BPB_BytesPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}
 
Parameters - current sector number that points to the block of data
Returns - The value of the address for that block of data
Description - finds the starting address of a block of data given the sector number corrosponding to that data block


int LBAToOffset(int32_t sector)
{
  return ((sector - 2) * BPB_BytesPerSec) + (BPB_BytesPerSec * BPB_RsvdSecCnt) + (BPB_NumFATs * BPB_FATSz32 * BPB_BytesPerSec);
}
*/
void displayPIDS(int PIDS[]) //showpids function that takes in the array that holds the pids numbers
{
  int i = 0;
  while (PIDS[i] != 0 && i < 15) //will not print out the entire array if it still has the default
  {
    printf("%d: %d\n", i, PIDS[i]);
    i++;
  }
}
//history to put information into the 2d array, pass in 2d array of history, int of commands we've done and the string that holds the user input
void storecommand(char HistOfCmds[][100], int numofcmd, char *cmd_str)
{
  if (strcmp(HistOfCmds[14], "") == 0) //means last one is empty
  {
    strcpy(HistOfCmds[numofcmd], cmd_str); //copies the whole line into the 2d array
  }
  else //we've maxed out and time to scoot every value over to update
  {
    strcpy(HistOfCmds[15], cmd_str);
    int j = 1;
    for (int i = 0; i < 15; i++)
    {
      strcpy(HistOfCmds[i], HistOfCmds[j]); //copies the value next to it in its place to scoot
      j++;
    }
  }
}
//history function just to print out all the commands used so far
void displayhist(char HistOfCmds[][100])
{
  int i = 0;
  while (strcmp(HistOfCmds[i], "") && i < 15) //will continue to print until it's empty or reaches max
  {
    printf("%d: %s", i, HistOfCmds[i]);
    i++;
  }
}
int main()
{
  char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);
  FILE *fp;
  char HistOfCommands[15][100]; //creating arrays for both the pids and the history
  int PIDS[15];
  for (int i = 0; i < 15; i++)
  {
    PIDS[i] = 0;
    strcpy(HistOfCommands[i], ""); //making the other spots empty so it doesn't hold trash
  }
  int numofcommand = 0; //global like counters to know how many commands and pids we've collected
  int numofpids = 0;
  while (1)
  {
    // Print out the msh prompt
    printf("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin))
      ;

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *argument_ptr;
    if (strstr(cmd_str, "!") != NULL) //will find out if the ! is in the string
    {
      cmd_str = cmd_str + 1;                           //gets rid of the !
      int CommandNum = atoi(cmd_str);                  //casting the char into an int so we can use it for index
      if (CommandNum < 0 || CommandNum > numofcommand) //if it's not in the bounds that we have
      {
        printf("Command not in history\n"); //printout for the requirement
        strcpy(cmd_str, "");                //blanks out so it'll just print it again without printing command not found
      }
      else
      {
        strcpy(cmd_str, HistOfCommands[CommandNum]);
        //takes what we have and puts it into cmd_str so it can parse it and run the command pulled from hist array
      }
    }
    char *working_str = strdup(cmd_str);

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while (((argument_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(argument_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }
    //Beginning of our code.
    if (token[0] != NULL && (!strcmp(token[0], "exit") || !strcmp(token[0], "quit"))) //in order to exit the shell
    {
      return 0;
    }
    else if (token[0] != NULL) //not just an enter typed means it'll probably be a command
    {
      if (strcmp(token[0], "cd") == 0) //if for the cd cmd and using built in function instead of execvp
      {
        storecommand(HistOfCommands, numofcommand, cmd_str); //takes note of the command entered
        chdir(token[1]);                                     //chdir as built in function to do cd command
      }
      else if (strcmp(token[0], "showpids") == 0) //meeting requirement 11 for the showpids
      {
        storecommand(HistOfCommands, numofcommand, cmd_str); //calls store command for the history function
        pid_t pid = fork();                                  //forks since it's a new process
        if (pid == 0)
        {
          return 0;
        }
        else
        {
          PIDS[numofpids] = pid; //takes and stores the pid that's taken from the fork
          int status;
          wait(&status);
        }
        displayPIDS(PIDS); //calls function last since we needed to add the current showpids into the array first
      }
      else if (strcmp(token[0], "history") == 0) //for when the user inputs history it'll show up to the last 15 cmds
      {
        storecommand(HistOfCommands, numofcommand, cmd_str); //takes note of the command entered
        pid_t pid = fork();
        if (pid == 0)
        {
          displayhist(HistOfCommands); //displaying history function
          return 0;
        }
        else
        {
          int status;
          wait(&status);
          PIDS[numofpids] = pid; //storing pid
        }
      }
      else if (strcmp(token[0], "bpb") == 0)
      {
        fp = fopen(token[1], "r"); //opening what the file name is in read only
        int16_t BPB_BytsPerSec;
        int8_t BPB_SecPerClus;
        int16_t BPB_RsvdSecCnt;

        fseek(fp, 11, SEEK_SET);
        fread(&BPB_BytsPerSec, 2, 1, fp);

        fseek(fp, 13, SEEK_SET);
        fread(&BPB_SecPerClus, 1, 1, fp);

        fseek(fp, 14, SEEK_SET);
        fread(&BPB_RsvdSecCnt, 2, 1, fp);

        printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
        printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
        printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);

        fclose(fp);
      }
      else
      {
        storecommand(HistOfCommands, numofcommand, cmd_str); //takes note of the command entered
        pid_t pid = fork();
        if (pid == 0) //child branch
        {
          if (execvp(token[0], token) == -1) //for requirement 2 and 3 in order to catch the wrong commands
          {
            printf("%s Command not found\n", token[0]); //just print the statement to let the user
            return 0;                                   //gets out of child process so that you don't have to press quit or exit twice for it to stop
          }
          else //means the command is all good
          {
            //can't use system and we use execvp instead to execute command and using token to check for the rest of the parameters
            execvp(token[0], token);
          }
        }
        else //parent
        {
          int status;
          wait(&status);
          int pidnumber = pid;
          PIDS[numofpids] = pidnumber; //storing the parent pid that into int array
        }
      }
    }
    free(working_root);
    numofcommand++; //incrementing how many commands we've entered so far
    numofpids++;    //incrementing how many PIDS we've entered so far
  }
  return 0;
}
