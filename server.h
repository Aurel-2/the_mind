#ifndef SERVER_H
#define SERVER_H
#include <pthread.h>



void *client_handler(void *p);
void wait_players();

#endif // SERVER_H
