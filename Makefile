CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Isrc
SRC      := src/main.cpp src/lexer/lexer.cpp
TARGET   := lumen

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe *.o
