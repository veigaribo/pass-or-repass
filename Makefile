CC=gcc
CFLAGS=
LDFLAGS=-pthread
CUSTOMCFLAGS=
CUSTOMLDFLAGS=

# To avoid having to clean when changing parameters
# Maybe there is another way but I'm not aware
OBJSUFFIX=

# Use `DEBUG=1` in the invocation of `make` to enable
ifdef DEBUG
	CUSTOMCFLAGS := $(CUSTOMCFLAGS) -g -DDEBUG
	OBJSUFFIX := $(OBJSUFFIX)-debug
else
	CUSTOMCFLAGS := $(CUSTOMCFLAGS) -O3
endif

SRCDIR=server
ODIR=obj

SOURCES=client.c event_loop.c event_handlers.c game.c hashmap.c id.c \
	net_sender.c net_parser.c list.c main.c message.c mm.c \
	question.c queue.c ready_check.c state.c thr_hashmap.c \
	thr_queue.c thr_vector.c vector.c

# TODO: Detect header dependencies
OBJS=$(patsubst %.c,%$(OBJSUFFIX).o,$(SOURCES))

$(ODIR)/%$(OBJSUFFIX).o: $(SRCDIR)/%.c
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $(CFLAGS) $(CUSTOMCFLAGS) $<

por: $(addprefix $(ODIR)/,$(OBJS))
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS) $(CUSTOMLDFLAGS)

.PHONY: clean

clean:
	rm $(ODIR)/*.o por
