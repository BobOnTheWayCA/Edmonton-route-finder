//Name: Shijie Bu
//SID: 1720680
//CCID: sbu1
//COURSE: CMPUT 275 Winter 2022
//PROJECT NAME: Assign 1 Part 2 Client/Server Application

CC = g++
C_FLAGS = -std=c++11

run:
	$(CC) $(C_FLAGS) -c server/server.cpp
	$(CC) $(C_FLAGS) -c server/digraph.cpp
	$(CC) $(C_FLAGS) -c server/dijkstra.cpp
	$(CC) $(C_FLAGS) -o server/server digraph.o dijkstra.o server.o
	./server/server

clean:
	rm -f inpipe
	rm -f outpipe
	rm -f digraph.o dijkstra.o server.o server/server