CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror

TARGET = loadbalancer

SRCS = main.cpp WebServer.cpp LoadBalancer.cpp Logger.cpp Firewall.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o *.log


run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

