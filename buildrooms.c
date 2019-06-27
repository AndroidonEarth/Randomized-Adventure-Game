/*************************************************************************************************************************
 * 
 * NAME
 *    buildrooms.c - the room-building program
 * SYNOPSIS
 *    When compiled and run, creates a new directory and a series of files that hold descriptions of the in game rooms 
 *    and how the rooms are connected.
 * INSTRUCTIONS
 *    Compile the program using this line:
 *       gcc -o buildrooms buildrooms.c
 *    Run the room building program by executing:
 *       buildrooms
 *    NOTE: No output should be returned.
 * DESCRIPTION
 *    Creates a directory called rooms, and in that directory creates 7 different room files from 10 possible rooms:
 *       Basement_room, Attic_room, Ballroom_room, Dining_room, Kitchen_room, Library_room, Bathroom_room, Bedroom_room,
 *       Trophy_room, Study_room.
 *    Elements that make up an actual room defined inside a room file are:
 *       A Room Name (max 8 characters)
 *       A Room Type (START_ROOM, END_ROOM, or MID_ROOM)
 *       Outbound Connections
 *          > Between 3 to 6 outbound connections from this room to other rooms.
 *          > Outbound connections have matching connections coming back.
 *          > A room does not have an outbound connection to itself.
 *          > A room does not have more than one outbound connection to the same room.
 * AUTHOR
 *    Written by Andrew Swaim
 *
*************************************************************************************************************************/

#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
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

// Room type enum and strings.
enum Types { START_ROOM, MID_ROOM, END_ROOM };
char* types[] = {"START_ROOM"
                , "MID_ROOM"
                , "END_ROOM"};

// Room struct.
struct Room
{
    char name[MAX_CHARS+1]; // +1 for the null character.
    enum Types type;
    int numConnections;
    char connections[MAX_CONNECTIONS][MAX_CHARS+1];
};

// Room names.
char* names[] = {"Basement"
                , "Attic"
                , "Ballroom"
                , "Dining"
                , "Kitchen"
                , "Library"
                , "Bathroom"
                , "Bedroom"
                , "Trophy"
                , "Study"};

/*************************************************************************************************************************
 * Function Declarations
*************************************************************************************************************************/

void InitRooms(struct Room rooms[]);
void Shuffle(int arr[], int n);
void MakeRoomFile(struct Room* room, char* dir);
bool IsGraphFull();
void AddRandomConnection(struct Room rooms[]);
bool ConnectionAlreadyExists(struct Room* roomA, struct Room* roomB);

/*************************************************************************************************************************
 * Main 
*************************************************************************************************************************/

int main()
{
    // Seed the random number generator.
    unsigned seed = time(0);
    srand(seed);

    // Create the rooms.
    struct Room rooms[NUM_OF_ROOMS];
    
    // Initialize the rooms
    InitRooms(rooms);
    
    // Create all connections in graph.
    while (IsGraphFull(rooms) == false)
    {
      AddRandomConnection(rooms);
    }

    // Get the current process id.
    int pid = getpid();
    
    // Variables for creating the directory.
    char dirName[STR_BUFFER];

    // Initialize the buffer and concat the prefix and proccess id into the buffer.
    memset(dirName, '\0', STR_BUFFER);
    snprintf(dirName, sizeof(dirName), "rooms.%d", pid); 

    // Create the directory.
    mkdir(dirName, 0755);

    int i;
    for (i = 0; i < NUM_OF_ROOMS; i++)
    {
        MakeRoomFile(&rooms[i], dirName);
    }    

    return 0;
}

/*************************************************************************************************************************
 * Function Definitions 
*************************************************************************************************************************/

// Initializes the array of rooms.
void InitRooms(struct Room rooms[])
{
    // Indexes for shuffling in order to randomly select room names.
    int indexes[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    
    // Shuffle array of indexes.
    Shuffle(indexes, NUM_OF_NAMES);

    /* Assign names randomly using the shuffled indexes, 
       initialize numConnections, and assign room types */
    int i;
    for (i = 0; i < NUM_OF_ROOMS; i++)
    {
        strcpy(rooms[i].name, names[indexes[i]]);
        rooms[i].numConnections = 0;
        rooms[i].type = MID_ROOM;
    }

    // Re-assign the room types for the first and last room.
    rooms[0].type = START_ROOM;
    rooms[6].type = END_ROOM;
}

// Shuffles an array of integers using the Fisher-Yates shuffle algorithm.
void Shuffle(int arr[], int n)
{
   int i, j, tmp;   // Index variables.
   
   for (i = n-1 ; i > 0; i--)
   {
        // Get a random index.
        j = rand() % i;

        // Swap the current index with the random index.
        tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
   }
}

// Creates a room file.
void MakeRoomFile(struct Room* room, char* dir)
{
    FILE* file;

    // Buffer to hold the filename.
    char filename[STR_BUFFER];
    memset(filename, '\0', STR_BUFFER);

    // Create the filename.
    snprintf(filename, sizeof(filename), "%s/%s_room", dir, room->name);

    // Open the file for writing.
    if ((file = fopen(filename, "w")) == NULL)
    {
        printf("ERROR: Failed to open filename \"%s\"\n", filename);
        perror("In MakeRoomFile()");
        exit(1);
    }

    // Create a line to write.
    char fileLine[STR_BUFFER];
    memset(fileLine, '\0', STR_BUFFER);
    snprintf(fileLine, sizeof(fileLine), "ROOM NAME: %s\n", room->name);

    // Write room name line.
    fputs(fileLine, file);

    // Write the room connections.
    int i;
    char* text = "CONNECTION";
    for (i = 1; i <= room->numConnections; i++)
    {
        // Reset the write buffer.
        memset(fileLine, '\0', STR_BUFFER);

        // Create a line for writing.
        snprintf(fileLine, sizeof(fileLine), "%s %d: %s\n", text, i, room->connections[i-1]);

        // Write to file.
        fputs(fileLine, file);
    }

    // Write the room type to a file.
    memset(fileLine, '\0', STR_BUFFER);
    snprintf(fileLine, sizeof(fileLine), "ROOM TYPE: %s\n", types[room->type]);
    fputs(fileLine, file);

    fclose(file);
}

// Returns true if all rooms have 3 to 6 outbound connections, false otherwise.
bool IsGraphFull(struct Room rooms[])
{
    int i;
    for (i = 0; i < NUM_OF_ROOMS; i++) 
    {
        if (rooms[i].numConnections < 3)
        {
            return false;
        }
    }
    return true;
}

// Adds a random, valid outbound connection from a Room to another Room.
void AddRandomConnection(struct Room rooms[])
{
    // To hold the indexes and pointers to the rooms.
    int indexA, indexB;
    struct Room *roomA, *roomB;

    while(1)
    {
        // Keep selecting a random room index until one that is not full is selected.
        indexA = rand() % NUM_OF_ROOMS;
        roomA = &rooms[indexA];
        if (roomA->numConnections < 6) 
        {
            break;
        }
    }

    do
    {
        // Keep selecting another random room until a valid selection is made.
        indexB = rand() % NUM_OF_ROOMS;
        roomB = &rooms[indexB];
    }
    while(roomB->numConnections >= 6 || indexA == indexB || ConnectionAlreadyExists(roomA, roomB) == true);

    // Connect the rooms to each other.
    strcpy(roomA->connections[roomA->numConnections], roomB->name);
    roomA->numConnections++;

    strcpy(roomB->connections[roomB->numConnections], roomA->name);
    roomB->numConnections++;
}

// Returns true if a connection from room A to room B already exists, false otherwise..
bool ConnectionAlreadyExists(struct Room* roomA, struct Room* roomB)
{
    // Get the number of outbound connections for room A.
    int numConnections = roomA->numConnections;
    
    int i;
    for (i = 0; i < numConnections; i++)
    {
        if ((strcmp(roomA->connections[i], roomB->name)) == 0)
            return true;
    }
    return false;
}
