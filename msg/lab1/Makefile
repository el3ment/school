# Makefile for echo client and server

CXX=			g++ $(CCFLAGS)

DEAMON=		msgd.o inet-server.o server.o
CLIENT=		msg.o inet-client.o client.o
OBJS =			$(DEAMON) $(CLIENT)

LIBS=

CCFLAGS= -g

all:	$(DEAMON) $(CLIENT)

deamon:$(DEAMON)
	$(CXX) -o server $(DEAMON) $(LIBS)

client:$(CLIENT)
	$(CXX) -o client $(CLIENT) $(LIBS)

clean:
	rm -f $(OBJS) $(OBJS:.o=.d)

realclean:
	rm -f $(OBJS) $(OBJS:.o=.d) server client


# These lines ensure that dependencies are handled automatically.
%.d:	%.cc
	$(SHELL) -ec '$(CC) -M $(CPPFLAGS) $< \
		| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
		[ -s $@ ] || rm -f $@'

include	$(OBJS:.o=.d)
