#ifndef NETCALLBACKS_H
#define NETCALLBACKS_H

void CallbackServerId(void * packet, void * passthrough);
void CallbackGameArea(void * packet, void * passthrough);
void CallbackMovableObj(void * packet, void * passthrough);
void CallbackMessage(void * packet, void * passthrough);
void CallbackPlayerInfo(void * packet, void * passthrough);
void CallbackPing(void * packet, void * passthrough);

#endif