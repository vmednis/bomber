#include "../shared/packet.h"

static unsigned char tmpBuffer[PACKET_MAX_BUFFER];
static int BufferEscape(unsigned char* buffer, int len);
static int BufferUnscape(unsigned char* buffer, int len);

int PacketEncode(unsigned char* buffer, unsigned char type, void* packet) {
        return -1;
}

void PacketDecode(unsigned char* buffer, int len, struct PacketCallbacks* callbacks) {
        return;
}

/*
** Escapes 0xffs in a buffer and returns the new length of the escaped data
*/
static int BufferEscape(unsigned char* buffer, int len) {
        int srcPtr = 0;
        int trgPtr = 0;

        while(srcPtr < len) {
                if(buffer[srcPtr] == 0xff) {
                        tmpBuffer[trgPtr] = 0xff;
                        tmpBuffer[trgPtr + 1] = 0xff;
                        trgPtr += 2;
                } else {
                        tmpBuffer[trgPtr] = buffer[srcPtr];
                        trgPtr++;
                }
                srcPtr++;
        }

        srcPtr = 0;
        while(srcPtr < trgPtr) {
                buffer[srcPtr] = tmpBuffer[srcPtr];
                srcPtr++;
        }

        buffer[trgPtr] = '\0'; /*Just to make debugging easier*/
        return trgPtr;
}

/*
** Unescapes 0xff 0xff sequences in the buffer and retuns the length of the
** unescaped data
*/
static int BufferUnscape(unsigned char* buffer, int len) {
        int srcPtr = 0;
        int trgPtr = 0;
        unsigned char lastByte = 0;

        while(srcPtr < len) {
                if(lastByte == 0xff && buffer[srcPtr] == 0xff) {
                        buffer[srcPtr] = 0;
                } else {
                        /*This will accept invalid escapes as just byte sequences*/
                        tmpBuffer[trgPtr] = buffer[srcPtr];
                        trgPtr++;
                }
                lastByte = buffer[srcPtr];
                srcPtr++;
        }

        srcPtr = 0;
        while(srcPtr < trgPtr) {
                buffer[srcPtr] = tmpBuffer[srcPtr];
                srcPtr++;
        }

        buffer[trgPtr] = '\0'; /*Just to make debugging easier*/
        return trgPtr;
}