CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -g -lraylib -lm

ODIR = obj
SHROBJS = shared/packet.o
CLIOBJS = $(SHROBJS) client/bombcli.o client/tests.o client/netcallbacks.o client/hashmap.o
SRVOBJS = $(SHROBJS) server/bombserv.o server/vec2.o server/collision.o

all: bombcli bombsrv

bombcli: $(addprefix $(ODIR)/, $(CLIOBJS))
	$(CC) $^ -o $@ $(CFLAGS)

bombsrv: $(addprefix $(ODIR)/, $(SRVOBJS))
	$(CC) $^ -o $@ $(CFLAGS)

$(ODIR)/%.o: %.c $(ODIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(ODIR):
	mkdir $(ODIR)
	mkdir $(ODIR)/client
	mkdir $(ODIR)/shared
	mkdir $(ODIR)/server

clean:
	rm -r $(ODIR)
	rm bombcli bombsrv
