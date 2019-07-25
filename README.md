# Randomized-Adventure-Game
A point-A-to-point-B adventure game where the rooms are randomized. Uses Raw C I/O, file and directory management, and mutex/thread manipulation. 

# Instructions
Compile the two programs using the following lines:

    gcc -o adventure adventure.c -lpthread
    gcc -o buildrooms buildrooms.c
    
Then to start the game, first run the buildrooms program to generate the room files, before running the adventure program to use the most recently created room files to present an interface to the player and run the game.

# buildrooms Room Generator
The first program, buildrooms, creates a directory called rooms. In this directory, 7 different room files are created from 10 possible rooms:

    Basement_room
    Attic_room
    Ballroom_room
    Dining_room
    Kichen_room
    Library_room
    Bathroom_room
    Bedroom_room
    Trophy_room
    Study_room
    
Elements that make up an actual room defined inside a room file are:

    A room name (max 8 characters)
    A room type (START_ROOM, END_ROOM, or MID_ROOM
    And outbound connections
    
Each room will have between 3 to 6 outbound connections to other rooms, as well as a matching connection coming back. A room will not have an outbound connection to itself, and cannot have more than one outbound connection to the same room.

# adventure Game
In the game, the player will begin in the "starting room" and will win the game automatically upon entering the "ending room", which causes the game to exit, displaying the path taken by the player.

When the program is initially run, the program will look for the most recently created rooms directory in the current directory of the game and reads the files. Then it presents the player with an interface that:

    Lists where the player currently is
    Lists the possible connections that can be followed
    Prompts the user for input
    
If the user types the exact name of a connection to another room and then hits return, the player will move into that room (the program writes a new line and continues running as before, but from the new room the player entered). If the user types anything but a valid room name (case sensitive), the game returns an error lines and repeats the current location and prompt (typing an incorrect location does not increment the path history or the step count).

Once the user has reached the "ending room", the game indicates that it has been reached, prints the path the player has taken to get there, the number of steps taken, a congratulatory message, and then exits with a status code of 0.

One additional feature is that while the game is running, if the player types the command "time" at the prompt and hits return, utilizing a second thread and mutexes the game writes the current tim of day to a file called currentTime.txt in the same directory of the game, and then reads this line and prints it out to the user (using the time command does not affect gameplay/does not increment the path history or the step count).
