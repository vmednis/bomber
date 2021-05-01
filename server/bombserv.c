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
#include <sys/wait.h>
#include "../shared/packet.h"
#include "bombserv.h"

#define HOST "127.0.0.1"
#define PORT 3004
unsigned char buffer[PACKET_MAX_BUFFER];

static void CallbackClientId(void* packet, void* data) {
        struct PacketClientId* pcid = packet;
        printf("protocol version = %u, player name = %s, player color = %c", pcid->protoVersion, pcid->playerName, pcid->playerColor);
        fflush(NULL);

}

int client_count = 0;

int process_client(int id, int socket)
{
        struct PacketCallbacks pccbks;
        int len, err;

        pccbks.callback[PACKET_TYPE_CLIENT_IDENTIFY] = &CallbackClientId;

        printf("Processing client id=%d, socket=%d.\n", id, socket);
        printf("CLIENT count %d\n", client_count);
        int i = 0;
        while (1)
        {
                if (i == 0) {
                        i += 1;
                        len = recv(socket, buffer, sizeof(buffer), MSG_DONTWAIT);
                        if (len < 0) {
                                printf("ERROR Can't receive data.");
                                err = errno;
                                printf("ERROR = %d.\n", err);
                                return -1;
                        }
                        printf("Length = %d.\n", len);
                        fflush(NULL);
                        PacketDecode(buffer, len, &pccbks, NULL);
                }
        }
        return 0;
}

int start()
{
        int main_socket;
        int client_socket;
        struct sockaddr_in server_address;
        struct sockaddr_in client_address;
        socklen_t client_address_size = sizeof(client_address);

        if ((main_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
                printf("ERROR opening main server socket\n");
                exit(1);
        };
        printf("Main socket created!\n");

        server_address.sin_family = AF_INET;
        server_address.sin_addr.s_addr = INADDR_ANY;
        server_address.sin_port = htons(PORT);

        if (bind(main_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0)
        {
                printf("ERROR binding the main server socket!\n");
                printf("ERROR %d\n", errno);
                exit(1);
        }
        printf("Main socket binded!\n");

        if (listen(main_socket, MAX_CLIENTS) < 0)
        {
                printf("ERROR listening to socket!\n");
                exit(1);
        }
        printf("Main socket is listening!\n");

        while (1)
        {
                int new_client_id = 0, cpid = 0;

                client_socket = accept(main_socket, (struct sockaddr*)&client_address, &client_address_size);
                if (client_socket < 0)
                {
                        printf("ERROR accepting client connection! ERRNO=%d\n", errno);
                        continue;
                }
                new_client_id = client_count;
                client_count += 1;
                cpid = fork();

                if (cpid == 0) /* Child process */
                {
                        close(main_socket);
                        cpid = fork();
                        if (cpid == 0) /* Grandchild process*/
                        {
                                process_client(new_client_id, client_socket);
                                exit(0);
                        }
                        else
                        {
                                wait(NULL);
                                printf("Successfully orphaned client %d\n", new_client_id);
                                exit(0);
                        }
                }
                else /* Parent process */
                {
                        close(client_socket);
                }
        }
        return 0;
}

int main()
{
        start();
        return 0;
}