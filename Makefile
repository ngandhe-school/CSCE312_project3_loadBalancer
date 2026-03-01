CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror

TARGET = loadbalancer

SRCS = main.cpp WebServer.cpp LoadBalancer.cpp
OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o log.txt


run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run

