CC=g++
CFLAGS=-I. -std=c++11

all: manager router dijkstra exec

manager:
	$(CC) $(CFLAGS) -c manager.cc
	
router:
	$(CC) $(CFLAGS) -c router.cc

dijkstra:
	$(CC) $(CFLAGS) -c dijkstra.cc
	
exec: manager.o  router.o dijkstra.o
	$(CC) $(CFLAGS) -o manager manager.o router.o dijkstra.o -lpthread
	
clean:
	rm *.o manager 
