OUTPUT = ubus_voip
OBJ = ubus_voip.o
LIBS = -lm -lubus -lubox -lpthread -luci -ljson-c 
all: $(OUTPUT)
 
 
$(OUTPUT): $(OBJ)
	@echo "...................................."
	@echo "CC = " $(CC)
	@echo "CFLAGS = " $(CFLAGS)
	@echo "LDFLAGS = " $(LDFLAGS)
	@echo "...................................."
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS) $(LDLIBS)
 
 
%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $^ $(LDLIBS) $(LIBS)
 
 
clean:
	-rm $(OUTPUT) *.o
