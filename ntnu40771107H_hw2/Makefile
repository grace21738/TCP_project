CC = gcc
CXX = g++
INCLUDE_OPENCV = `pkg-config --cflags --libs opencv`
LINK_PTHREAD = -lpthread

CLIENT = client.cpp
SERVER = server.cpp
OPEN_CV = openCV.cpp
PTHREAD = pthread.c
CLI = client
SER = server
CV = openCV
PTH = pthread

all: server client opencv pthread
  
server: $(SERVER)
	$(CXX) $(SERVER) $(INCLUDE_OPENCV) $(LINK_PTHREAD) -o $(SER)  
client: $(CLIENT)
	$(CXX) $(CLIENT) $(INCLUDE_OPENCV) $(LINK_PTHREAD) -Wall -o $(CLI)
opencv: $(OPEN_CV)
	$(CXX) $(OPEN_CV) -o $(CV) $(INCLUDE_OPENCV)
pthread: $(PTHREAD)
	$(CC) $(PTHREAD) -o $(PTH) $(LINK_PTHREAD)

.PHONY: clean

clean:
	rm $(CLI) $(SER) $(CV) $(PTH)
