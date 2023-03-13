OUTPUT = ubus_demo
OBJ = ubus_demo.o
#LIBS = -lm -lubus -lubox -lpthread -luci -ljson-c
LIBS = 
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
