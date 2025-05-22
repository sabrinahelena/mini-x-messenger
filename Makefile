CXX = g++
CXXFLAGS = -std=c++20 -Iinclude -Wall
LDFLAGS = -lws2_32

SRC_DIR = src

all: server client_sender client_receiver

server: $(SRC_DIR)/server.cpp
	$(CXX) $(CXXFLAGS) -o server $(SRC_DIR)/server.cpp $(LDFLAGS)

client_sender: $(SRC_DIR)/client_sender.cpp
	$(CXX) $(CXXFLAGS) -o client_sender $(SRC_DIR)/client_sender.cpp $(LDFLAGS)

client_receiver: $(SRC_DIR)/client_receiver.cpp
	$(CXX) $(CXXFLAGS) -o client_receiver $(SRC_DIR)/client_receiver.cpp $(LDFLAGS)

clean:
	del /Q server.exe client_sender.exe client_receiver.exe 2>nul || exit 0

