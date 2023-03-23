OUTPUT = ubus_evoip
OBJ = ubus_evoip.o
LIBS = -lubus -lubox -luci -ljson-c -lblobmsg_json 
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
