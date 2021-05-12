#ifndef NETWORK_H
#define NETWORK_H

struct NetworkState {
        char ip[32];
        unsigned short port;
        int fd;
        char reqName[256];
};

#endif