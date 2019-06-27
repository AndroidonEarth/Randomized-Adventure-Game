/*************************************************************************************************************************
 * 
 * NAME
 *    adventure.c - the game program
 * SYNOPSIS
 *    When compiled and run, uses the most recently created files from the room-building program to present an interface
 *       to the player and run the game.
 *    In the game, the player will begin in the "starting room" and will win the game automatically upon entering the
 *       "ending room", which causes the game to exit, displaying the path taken by the player.
 * INSTRUCTIONS
 *    Compile the program using this line:
 *       gcc -o adventure adventure.c -lpthread
 *    Run the game program by executing:
 *       adventure
 * DESCRIPTION
 *    When compiled and run, performs a stat() function call on the rooms directory in the same directory of the game,
 *       and opens the one with the most recent st_mtime component of the returned stat struct.
 *    Then presents the player with an interface that:
 *       > Lists where the player currently is.
 *       > Lists the possible connections that can followed.
 *       > A prompt to the user.
 *    If the user types the exact name of a connection to another room and then hits return, the program writes a new
 *       line and then continues running as before but with the new room that the player entered.
 *    If the user types anything but a valid room name (case sensitive), the game returns an error line and repeats
 *       the current location and prompt.
 *          > Trying to go to an incorrect location does not increment the path history or the step count.
 *    Once the user has reached the "ending room", the game indicates that it has been reached, prints the path the
 *       user has taken to get there, the number of steps taken, a congratulatory message, and then exists with a
 *       status code of 0.
 *    While the game is running, if the player types the command "time" at the prompt and hits enter, utilizing a
 *       second thread and mutex(es), the game writes the current time of day to a file called "currentTime.txt"
 *       in the same directory as the game, and then reads this line and prints it out to the user.
 *          > Using the time command does not increment the path history or step count.
 * AUTHOR
 *    Written by Andrew Swaim
 *
*************************************************************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_OF_ROOMS 7      // Constant to hold the number of rooms that will be created.
#define NUM_OF_NAMES 10     // Constant to hold the total number of room names.
#define MAX_CONNECTIONS 6   // Maximum number of connections a room can have.
#define MAX_CHARS 8         // Maximum number of characters for each name.
#define STR_BUFFER 100      // General purpose buffer for string handling.

// Create bool type for C89/C90 compilation.
typedef enum { false, true } bool;

// Room type enum and string array for conversion.
enum Types { START_ROOM, MID_ROOM, END_ROOM };
char* types[] = {"START_ROOM"
                , "MID_ROOM"
                , "END_ROOM"};

// Room struct.
struct Room
{
    char name[MAX_CHARS+1];
    enum Types type;
    int numConnections;
    char connections[MAX_CONNECTIONS][MAX_CHARS+1];
};

/*************************************************************************************************************************
 * Function Declarations
*************************************************************************************************************************/

int InitRooms(struct Room rooms[]);
//char* GetMostRecentDir();
void GetMostRecentDir(char dirName[]);
void DisplayRoom(struct Room* room);
int GetSelectedRoomIndex(struct Room rooms[], struct Room* room, char* roomName);
void RecordValidChoice(char* roomName, char* filename);
void PrintPlayerPath(char* filename);
void* WriteTime(void* mutex);
void DisplayTime();

/*************************************************************************************************************************
 * Main 
*************************************************************************************************************************/

int main()
{
    // Struct array to hold the information for the rooms.
    struct Room rooms[NUM_OF_ROOMS];

    // Variables to keep track of the current room.
    int currentRoomIndex = -1;
    struct Room* currentRoom;

    // Variables to get the user choice.
    char* userChoice = NULL;
    size_t userChoiceBuffer = 0;

    // Variables to keep track of player stats such as steps and path taken.
    int steps = 0;
    FILE* file;
    char tmpFilename[STR_BUFFER];
    char* tmpFilePrefix = "tmpfile.";
    int pid;
    
    // Mutex variable for handling multithreading.
    pthread_mutex_t myMutex = PTHREAD_MUTEX_INITIALIZER;

    // Lock the mutex and spawn a second thread.
    pthread_mutex_lock(&myMutex);
    pthread_t thread;
    if ((pthread_create(&thread, NULL, WriteTime, (void*) &myMutex)) != 0)
    {
        printf("ERROR: There was a problem creating a second thread\n");
        perror("In main() with pthread_create()");
        exit(1);
    }

    // Get the starting room.
    currentRoomIndex = InitRooms(rooms);
    if (currentRoomIndex < 0 || currentRoomIndex > 6)
    {
        // Error handling.
        printf("ERROR: There was a problem getting the starting room, exiting...\n");
        perror("In main() with GetStartingRoomIndex()");
        exit(1);
    } 
    else
    {
        currentRoom = &rooms[currentRoomIndex];
    }

    // Create the temp filename to store the user path.
    pid = getpid();
    memset(tmpFilename, '\0', STR_BUFFER);
    snprintf(tmpFilename, sizeof(tmpFilename), "%s%d", tmpFilePrefix, pid);

    // Create the temp file and truncate it if it already exists.
    if ((file = fopen(tmpFilename, "w")) == NULL)
    {
        printf("ERROR: Failed to open filename \"%s\"\n", tmpFilename);
        perror("In main()");
        exit(1);
    }
    fclose(file);

    // Start the game.
    while ((strcmp(types[currentRoom->type], "END_ROOM")) != 0)
    {
        // Check if the mutex is already locked by the main thread.
        if ((pthread_mutex_trylock(&myMutex)) == 0)
        {
            // If the (re)lock by the main thread is succesful, create a new thread.
            if ((pthread_create(&thread, NULL, WriteTime, (void*) &myMutex)) != 0)
            {
                printf("ERROR: There was a problem creating a second thread\n");
                perror("In main() with pthread_create()");
                exit(1);
            }
        }

        // Display the current room.
        DisplayRoom(currentRoom);

        // Prompt the user, get the input and remove the newline character.
        printf("WHERE TO? >");
        getline(&userChoice, &userChoiceBuffer, stdin);
        userChoice[strcspn(userChoice, "\n")] = '\0';
        
        // Process user choice.
        if (strcmp(userChoice, "time") == 0)
        {
            // If the user typed "time", unlock the mutex to allow WriteTime to execute in the second thread.
            pthread_mutex_unlock(&myMutex);

            // Wait for the second thread to finish.
            pthread_join(thread, NULL);

            // Display the time that was written by the second thread.
            DisplayTime();
        }
        else
        {
            // Otherwise, try to get the user choice.
            currentRoomIndex = GetSelectedRoomIndex(rooms, currentRoom, userChoice);
            if (currentRoomIndex == -1)
            {
                // If the user choice was invalid, display an error message, don't increment the steps, and loop again.
                printf("\nHUH? I DON'T UNDERSTAND THAT ROOM. TRY AGAIN\n\n");
            }
            else if (currentRoomIndex > 6 || currentRoomIndex < -1)
            {
                // Error handling.
                printf("ERROR: Something went wrong trying to get the selected room index %d\n", currentRoomIndex);
                perror("In main() with GetSelectedRoomIndex()");
                exit(1);
            }
            else
            {
                // Otherwise, if the user choice was valid, get the selected room, record the valid user choice
                // and increment the step count
                currentRoom = &rooms[currentRoomIndex];
                RecordValidChoice(userChoice, tmpFilename);
                steps++;
                printf("\n"); // To match the formatting of the example.
            }
        }
    // Deallocate memory for user choice.
    free(userChoice);
    userChoice = NULL;
    } // End of game loop.

    // If the loop was exited then the player has reached the end room and the game is over.
    // Print a congratulatory message, the number of steps the player took, and the path the player took. 
    printf("YOU HAVE FOUND THE END ROOM. CONGRATULATIONS!");
    printf("\nYOU TOOK %d STEPS. YOUR PATH TO VICTORY WAS:\n", steps);
    PrintPlayerPath(tmpFilename);

    // Delete the temp file.
    remove(tmpFilename);

    // Destroy the mutex.
    pthread_mutex_destroy(&myMutex);

    return 0;
}

/*************************************************************************************************************************
 * Function Definitions 
*************************************************************************************************************************/

/* Initializes the rooms using the room files from the build-rooms program, and returns the index of the "starting room".
   Returns -1 if there was an error initializing or getting the starting room. */
int InitRooms(struct Room rooms[])
{
    // Variable to capture whichever room is the start room.
    int startingIndex = -1;

    // Variables for navigating the rooms directory.
    DIR* dir;
    char dirName[STR_BUFFER];
    struct dirent* dirEntry;

    // Variables to store the names of, and navigate, the files in the room files.
    char filenames[NUM_OF_ROOMS][STR_BUFFER];
    FILE* file;
    char fileLine[STR_BUFFER];
    char* word;

    // Get the most recently created rooms directory.
    memset(dirName, '\0', STR_BUFFER);
    GetMostRecentDir(dirName);

    // Open the directory.
    dir = opendir(dirName);

    // Loop through all the files in the opened rooms directory and get the filenames.
    int i = 0;
    while ((dirEntry = readdir(dir)) != NULL)
    {
        // If the file is a regular file (not a directory).
        if (dirEntry->d_type == DT_REG)
        {
            // Capture the full filepath of the file ("rooms.PID/room-name_room")
            memset(filenames[i], '\0', STR_BUFFER);
            snprintf(filenames[i], sizeof(filenames[i]), "%s/%s", dirName, dirEntry->d_name);
            i++;
        }
    }
    closedir(dir);

    // Open all the files and put the contents in rooms[].
    int j;
    for (i = 0; i < NUM_OF_ROOMS; i++)
    {
        // Open the file for reading.
        if ((file = fopen(filenames[i], "r")) == NULL)
        {
            printf("ERROR: Failed to open filename \"%s\"\n", filenames[i]);
            perror("In InitRooms()");
            exit(1);
        }
        
        // Get the first line from the file.
        memset(fileLine, '\0', STR_BUFFER);
        fgets(fileLine, sizeof(fileLine), file);

        // Remove the trailing newline character.
        fileLine[strcspn(fileLine, "\n")] = '\0';

        // Get the last word from the line and assign it as the room name.
        word = strrchr(fileLine, ' ') + 1;
        strcpy(rooms[i].name, word);

        // Get the first CONNECTION line, minus the newline character.
        memset(fileLine, '\0', STR_BUFFER);
        fgets(fileLine, sizeof(fileLine), file);
        fileLine[strcspn(fileLine, "\n")] = '\0';

        // Loop through the CONNECTION lines.
        j = 0;
        rooms[i].numConnections = 0; // Initialize the number of connections of the room.
        while (fileLine[0] == 'C') // The lines should be only ones in the file that start with 'C'.
        {
            // Get last word of each line (the connection names).
            word = strrchr(fileLine, ' ') + 1;
            strcpy(rooms[i].connections[j], word);

            // Increment the number of connections for the room.
            rooms[i].numConnections++;

            // Prepare for next loop;
            j++;
            memset(fileLine, '\0', STR_BUFFER);
            fgets(fileLine, sizeof(fileLine), file);
            fileLine[strcspn(fileLine, "\n")] = '\0';
        }

        // The last fgets should have been called already for the room type, so simply get the last word.
        word = strrchr(fileLine, ' ') + 1;

        // Assign the room type and capture the START_ROOM index to return and start the game.
        if ((strcmp(word, "START_ROOM")) == 0)
        {
            rooms[i].type = START_ROOM;
            startingIndex = i;
        }
        else if ((strcmp(word, "MID_ROOM")) == 0)
        {
            rooms[i].type = MID_ROOM;
        }
        else if ((strcmp(word, "END_ROOM")) == 0)
        {
            rooms[i].type = END_ROOM;
        }

        // Close the file and loop again to read from the next file.
        fclose(file);
    }

    return startingIndex;
}

// Returns the name of the most recently created directory.
void GetMostRecentDir(char dirName[])
{
    // Directory variables.
    DIR* dir;
    int statRet = 0;
    struct stat dirStat;
    struct dirent* dirEntry;
    time_t mostRecentTime = 0;

    // Open the current directory.
    dir = opendir(".");

    // Loop through all the files in the current directory.
    while ((dirEntry = readdir(dir)) != NULL)
    {
        // (Re)Initialize stat buffer.
        memset(&dirStat, 0, sizeof(dirStat));

        // Get stats of current directory entry.
        statRet = stat(dirEntry->d_name, &dirStat);

        // Error handling.
        if (statRet != 0)
        {
            printf("ERROR: There was an error getting the stats of directory %s\n", dirEntry->d_name);
            perror("In GetMostRecentDir() with stat()");
            exit(1);
        } // If the file path type is a directory that starts with "rooms.".
        else if (S_ISDIR(dirStat.st_mode) && (strstr(dirEntry->d_name, "rooms.") != NULL))
        {
            // Compare its modified time value to find the most recent time.
            if (dirStat.st_mtime > mostRecentTime)
            {
                // Get the directory name and update the modified time comparison variable.
                memset(dirName, '\0', STR_BUFFER);
                strcpy(dirName, dirEntry->d_name);
                mostRecentTime = dirStat.st_mtime;
            }
        } // If the file path was not a rooms directory, simply loop again to the next file.
    }

    if ((strlen(dirName)) == 0)
    {
        printf("ERROR: There is no rooms directory in the current directory.\n");
        perror("In GetMostRecentDir()");
        exit(1);
    }

    // Close the directory.
    closedir(dir);
    
    //return dirName;
}


// Takes a pointer to a room and displays the details of the room.
void DisplayRoom(struct Room* room)
{
    printf("CURRENT ROOM: %s\n", room->name);
    printf("POSSIBLE CONNECTIONS: ");
    int i;
    int numConnections = room->numConnections;
    for(i = 0; i < numConnections-1; i++)
    {
        printf("%s, ", room->connections[i]);
    }
    printf("%s.\n", room->connections[i]);
}

// Checks if the user choice was a valid connection for the current room, and if so gets the new room index.
int GetSelectedRoomIndex(struct Room rooms[], struct Room* currentRoom, char* roomName)
{
    // Check if the user choice is valid.
    int i;
    int numConnections = currentRoom->numConnections;
    for (i = 0; i < numConnections; i++)
    {
        if ((strcmp(currentRoom->connections[i], roomName)) == 0)
        {
            break;
        }
    }

    // If the loop did not exit early, then the user choice was not a valid connection so return -1;
    if (i == numConnections)
    {
        return -1;
    }

    // If it did exit early, get the index of the user choice.
    for (i = 0; i < NUM_OF_ROOMS; i++)
    {
        if ((strcmp(rooms[i].name, roomName)) == 0)
        {
            return i;
        }
    }

    // If this point is reached, something went wrong.
    return -5;
}

// Takes the name of a valid room choice made by the user and writes it to the temp file.
void RecordValidChoice(char* roomName, char* filename)
{
    FILE* file;

    // Open the temp file for appending.
    if ((file = fopen(filename, "a")) == NULL)
    {
        printf("ERROR: Failed to open filename \"%s\"\n", filename);
        perror("In RecordValidChoice()");
        exit(1);
    }

    // Put the room name in the file with a newline character
    fputs(roomName, file);
    fputs("\n", file);

    // Close temp file.
    fclose(file);
}

// Takes the name of temp file and prints the recorded player path to the screen.
void PrintPlayerPath(char* filename)
{
    FILE* file;
    char fileLine[MAX_CHARS+2]; // +2 to take into account the newline and null characters

    // Open the temp file for reading.
    if ((file = fopen(filename, "r")) == NULL)
    {
        printf("ERROR: Failed to open filename \"%s\"\n", filename);
        perror("In PrintPlayerPath()");
        exit(1);
    }

    // Get the lines from the temp file and print them to the screen.
    memset(fileLine, '\0', MAX_CHARS+2);
    while ((fgets(fileLine, sizeof(fileLine), file)) != NULL)
    {
        printf("%s", fileLine);
        memset(fileLine, '\0', MAX_CHARS+2);
    }

    fclose(file);
}

// Runs concurrently with main() but is immediately locked, and only unlocks when the user types "time".
void* WriteTime(void* myMutex)
{
    // Lock this second thread until unlock is called in main().
    pthread_mutex_lock(myMutex);

    // Once unlocked, get current time.
    time_t calendar_tm = time(NULL);
    struct tm* local_tm = localtime(&calendar_tm);

    // Get the formated string.
    char strTime[STR_BUFFER];
    memset(strTime, '\0', STR_BUFFER);
    strftime(strTime, STR_BUFFER, "%l:%M%P, %A, %B %d, %Y", local_tm);

    // Add the formated sring to a file called "currentTime.txt", and truncate if the file already exists.
    FILE* file;
    if ((file = fopen("currentTime.txt", "w")) == NULL)
    {
        printf("ERROR: Failed to open filename \"currentTime.txt\"\n");
        perror("In WriteTime()");
        exit(1);
    }
    fputs(strTime, file);
    fclose(file);

    // Unlock the mutex so that the main thread can lock it again.
    pthread_mutex_unlock(myMutex);
}

// Displays the current time recorded in "currentTime.txt".
void DisplayTime()
{
    // Open the file "currentTime.txt".
    FILE* file;
    if ((file = fopen("currentTime.txt", "r")) == NULL)
    {
        printf("ERROR: Failed to open filename \"currentTime.txt\"\n");
        perror("In DisplayTime()");
        exit(1);
    }
    
    // Print the time that was recorded in the file.
    char strTime[STR_BUFFER];
    memset(strTime, '\0', STR_BUFFER);
    fgets(strTime, sizeof(strTime), file);
    printf("\n%s\n\n", strTime);

    // Close the file.
    fclose(file);
}
