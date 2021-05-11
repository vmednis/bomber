#ifndef BOMBSERV_H
#define BOMBSERV_H

typedef struct server {
        int address;
        int port;
        int fd;
} server;

typedef struct client
{
        int clientID;
        int fd;
        char name[32];
        char color;
        int inGame;
        int readyForGame;
        float x;
        float y;
} client;

typedef struct allClients
{
        client* client;
        struct allClients* next;
} allClients;

/* Default game arena */
unsigned char blocks[169] = {
                                1,1,1,1,1,1,1,1,1,1,1,1,1,
                                1,0,0,2,2,2,2,2,2,2,2,2,1,
                                1,0,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,2,1,
                                1,2,2,2,2,2,2,2,2,2,2,2,1,
                                1,2,1,2,1,2,1,2,1,2,1,0,1,
                                1,2,2,2,2,2,2,2,2,2,0,0,1,
                                1,1,1,1,1,1,1,1,1,1,1,1,1
};

#define PACKET_BUFFER_PICK(buffer, offset, type, value, converter) do { \
        value = converter(*(type *)((void *) (buffer + offset))); \
} while(0);

#endif