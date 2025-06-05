# Executable and source directories
BIN=bin
SRC=src

# Compiler to use
CC=g++

# Compiler flags
CFLAGS=-c 

# Source files
SOURCES=$(SRC)/dijkstra_algorithm.cpp $(SRC)/geo_position_to_node.cpp $(SRC)/main.cpp

# Object files (move them to BIN)
OBJECTS=$(SOURCES:$(SRC)/%.cpp=$(BIN)/%.o)

# Executable name
EXECUTABLE=$(BIN)/specification_dijkstra

# Default target
all: $(EXECUTABLE)

# Build target
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

# Compile target (store .o files in BIN)
$(BIN)/%.o: $(SRC)/%.cpp | $(BIN)
	$(CC) $(CFLAGS) $< -o $@

# Ensure BIN folder exists
$(BIN):
	mkdir -p $(BIN)

# Clean target
clean:
	rm -f $(BIN)/*.o $(EXECUTABLE)

