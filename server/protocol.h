#ifndef H_PASS_OR_REPASS_PROTOCOL
#define H_PASS_OR_REPASS_PROTOCOL

// # Tags

// ## Common
#define DENY_ABORT 0xfe
#define DENY_RETRY 0xfd

// ## Client -> Server

// ### Misc

// Includes two parameters: a 1 byte length and a length bytes
// sized string.
// String (name) should use UTF-8 and not exceed 20 characters
#define SET_NAME 0x20
#define GET_NAME 0x21
#define GET_OPPONENT_NAME 0x22

// ### Queue
#define JOIN_QUEUE 0x01
#define QUIT_QUEUE 0x02
#define ACCEPT_GAME 0x03
#define DENY_GAME 0x04

// ### Game
#define ASK_AGAIN 0x06
#define ANSWER_A 0x10
#define ANSWER_B 0x11
#define ANSWER_C 0x12
#define PASS 0x1f
#define QUIT_GAME 0x1e

// ## Server -> Client

// ### Queue
#define GAME_FOUND 0x81

// Includes two parameters:
// An 8 bytes Match ID and a 1 byte player ID
// Player ID will be either 0x01 or 0x02,
// for player 1 or 2 respectively
#define JOINED_GAME 0x82

// ### Game

// Includes two parameters: a 4 byte length (little endian) and a
// length bytes sized string.
#define ASK_TO_01 0x83
#define ASK_TO_02 0x84

#define PASS_TO_01 0x85
#define PASS_TO_02 0x86

// Includes one parameter, a 4 bytes little-endian player score
#define SCORE_01 0x87
#define SCORE_02 0x88

#define GAME_END 0x89
#define GAME_ABORTED 0x8a

// # Data

#define SET_NAME_MAX_SIZE 20

#endif
