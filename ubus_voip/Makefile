CC=gcc
CFLAGS=-I.
DEPS =
OBJ = ubus_voip.o
LIBS =

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

ubus_voip: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)
