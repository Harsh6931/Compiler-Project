CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Iinclude
SRC      := src/main.cpp
TARGET   := lumen

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe
