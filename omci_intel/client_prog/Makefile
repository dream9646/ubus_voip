include ../makefile.inc
CFLAGS+= -I../util -I./editline -DUSE_READLINE -DDIAG_HELPER

all: omci_client fencrypt cpul

omci_client: omci_client.o diag.o clitree.o
#	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o $@ $< -lpthread 
	make -C ./editline
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o $@ omci_client.o diag.o clitree.o ./editline/editline.a -lpthread
#	$(STRIP) $@

fencrypt: fencrypt.c 
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -DMAKE_FENCRYPT -o $@ $<
#	$(STRIP) $@

cpul: cpul.c 
	$(CC) $(LOCAL_CFLAGS) $(CFLAGS) -o $@ $<
#	$(STRIP) $@

update_cli:
	gcc -I../util -DCLITREE_UPDATE -DCLITREE_DEBUG -o clitree_update clitree.c
	./clitree_update ../etc/omci/omci.cli ../etc/omci/omci.help

clean:
	make -C ./editline clean
	rm -f *.o omci_client fencrypt cpul clitree_update
