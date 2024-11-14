#ifndef GAME_H
#define GAME_H

#define MAX_CARDS 100

#define MAX_PLAYERS 4

typedef enum
{
    WAITING,
    PLAYING,
    DISTRIBUTION,
    BAD_ROUND,
    SCORING,
    END
} GameStatus;

typedef struct
{
    int socket;
    char name[50];
    int card[MAX_CARDS];
} Player;

typedef struct
{
    int cards[MAX_CARDS];
    int lives;
    int level;
    int current_card;
    int played_cards[MAX_PLAYERS];
    int played_count;
    GameStatus status;
} TheMind;

#endif
