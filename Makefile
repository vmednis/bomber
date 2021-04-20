CC = gcc
CFLAGS = -std=c89 -pedantic -pedantic-errors -Wall -Wextra -g

ODIR = obj
SHROBJS = shared/shrtest.o
CLIOBJS = $(SHROBJS) client/bombcli.o
SRVOBJS = $(SHROBJS) server/bombsrv.o

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
