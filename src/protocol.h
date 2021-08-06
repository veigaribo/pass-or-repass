#ifndef H_PASS_OR_REPASS_PROTOCOL
#define H_PASS_OR_REPASS_PROTOCOL

// Common
#define ACK 0xff
#define DENY 0xfe

// Client -> Server
#define JOIN_QUEUE 0x01
#define QUIT_QUEUE 0x02
#define RESUME_GAME 0x05
#define ASK_AGAIN 0x06

#define ANSWER_A 0x10
#define ANSWER_B 0x11
#define ANSWER_C 0x12

#define PASS 0x1f
#define QUIT_GAME 0x1e

// Server -> Client
#define GAME_FOUND 0x81

// Includes two null terminated strings:
// Match ID and player ID
// Player ID will be either 0x01 or 0x02,
// for player 1 and 2 respectively
#define JOINED_GAME 0x82

// Includes two null terminated strings:
// target player and question description
#define ASK_TO_01 0x83
#define ASK_TO_02 0x84
#define PASS_TO_01 0x85
#define PASS_TO_02 0x86

// Includes one null terminated strings:
// The player's score
#define SCORE_01 0x87
#define SCORE_02 0x88

#define GAME_END 0x89

#define GAME_ABORTED 0x8a

int nparams_get(unsigned char event);

#endif
