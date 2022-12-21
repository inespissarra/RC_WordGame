#ifndef CONSTANTS_H
#define CONSTANTS_H

#define MAX_WORD_LENGTH 31
#define MAX_FILENAME_SIZE 40
#define MAX_FSIZE_SIZE 10
#define MAX_READ_SIZE 128
#define MAX_PLID_SIZE 6
#define MAX_COMMAND_SIZE 3
#define MAX_PLAYER_STATE_SIZE 37
#define MAX_HOSTNAME_SIZE 253
#define TIMEOUT 5000

#define HINT 0
#define SCOREBOARD 1
#define STATE 2

#define INVALID_PLID "Invalid PLID\n"
#define INVALID_LETTER "Invalid letter\n"
#define INVALID_TRIAL "Invalid trial number\n"
#define INVALID_WORD "Invalid word. The word cannot contain numbers\n"
#define FORMAT_ERROR "Format error\n"

#define NEW_GAME "New game started (max %d errors): %s\n"
#define GAME_ALREADY_STARTED "Error: Game already started\n"
#define GOOD_PLAY "Yes, \"%c\" is part of the word (max %d errors): %s\n"
#define BAD_PLAY "No, \"%c\" is not part of the word (max %d errors): %s\n"
#define DUP_PLAY "The letter has been inserted before\n"
#define GOOD_GUESS "WELL DONE! You guessed: %s\n"
#define BAD_GUESS "No, \"%s\" is not the word (max %d errors)\n"
#define GAME_OVER "You have reached the maximum error limit.\nGame over!\n"

#define NO_SCORES "No game was yet won by any player\n"
#define NO_HINT "No hint available\n"
#define NO_STATE "No games active or finished for you :/\n"

#define QUIT "Game over!\n"
#define NO_GAME "There is no game in progress\n"


#endif