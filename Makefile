CC=gcc
CFLAGS=
LDFLAGS=-pthread
CUSTOMCFLAGS=

SRCDIR=src
ODIR=obj

SOURCES=$(notdir $(wildcard $(SRCDIR)/*.c))
OBJS=$(patsubst %.c,%.o,$(SOURCES))

$(ODIR)/%.o: $(SRCDIR)/%.c
	mkdir -p $(ODIR)
	$(CC) -c -o $@ $(CFLAGS) $(CUSTOMCFLAGS) $<

por: $(addprefix $(ODIR)/,$(OBJS))
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

.PHONY: clean

clean:
	rm $(ODIR)/*.o por
