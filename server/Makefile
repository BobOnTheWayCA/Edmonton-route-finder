//Name: Shijie Bu
//SID: 1720680
//CCID: sbu1
//COURSE: CMPUT 275 Winter 2022
//PROJECT NAME: Assign 1 Part 2 Client/Server Application

CC = g++
C_FLAGS = -std=c++11

server.o:
	$(CC) $(C_FLAGS) -c server.cpp

digraph.o:
	$(CC) $(C_FLAGS) -c digraph.cpp
 
dijkstra.o:
	$(CC) $(C_FLAGS) -c dijkstra.cpp

server:
	$(CC) $(C_FLAGS) -o server digraph.o dijkstra.o server.o

clean:
	rm -f digraph.o dijkstra.o server.o server