#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include "../shared/packet.h"
#include "../client/tests.h"

#define HOST "127.0.0.1"
#define PORT 3000

int main()
{
        int my_socket = 0;
        struct sockaddr_in remote_address;
        remote_address.sin_family = AF_INET;
        remote_address.sin_port = htons(PORT);
        unsigned char buffer[PACKET_MAX_BUFFER];
        struct PacketClientId pcid;
        struct PacketClientInput pci;
        /* struct PacketClientMessage pcm; */
        unsigned int len;

        pcid.protoVersion = 0x00;
        strcpy(pcid.playerName, "Valters");
        pcid.playerColor = 'x';

        pci.movementX = 7;
        pci.movementY = 4;
        pci.action = 1;

        inet_pton(AF_INET, HOST, &remote_address.sin_addr);

        if ((my_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) /* new socket fd is returned */
        {
                printf("SOCKET ERROR\n");
                return -1;
        }

        if (connect(my_socket, (struct sockaddr*)&remote_address, sizeof(remote_address)) < 0)
        {
                printf("connection error");
                fprintf(stderr, "%d\n", errno);
                return -1;
        }
        else
        {
                /* Client ID */
                len = PacketEncode(buffer, PACKET_TYPE_CLIENT_IDENTIFY, &pcid);
                printf("%u", buffer[len - 1]);
                fflush(NULL);
                if (send(my_socket, buffer, len, 0) < 0) {
                        printf("error sending message");
                        return -1;
                }
                /* Client input */
                len = PacketEncode(buffer, PACKET_TYPE_CLIENT_INPUT, &pci);
                printf("%u", buffer[len - 1]);
                fflush(NULL);
                if (send(my_socket, buffer, len, 0) < 0) {
                        printf("error sending message");
                        return -1;
                }
        }
        return 0;
}