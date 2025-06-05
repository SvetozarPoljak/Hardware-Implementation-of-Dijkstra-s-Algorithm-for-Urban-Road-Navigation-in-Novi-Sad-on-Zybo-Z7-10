# SystemC installation directory
SYSTEMC_HOME=/usr/local/systemc

# Executable and source directories
BIN=bin
SRC=src

# Compiler to use
CC=g++

# Compiler flags
CFLAGS=-c -I$(SYSTEMC_HOME)/include

# Linker flags
LDFLAGS=-L$(SYSTEMC_HOME)/lib-linux64 -lsystemc -lpthread

# Source files
SOURCES=$(SRC)/bram_mem.cpp $(SRC)/dma.cpp $(SRC)/ic.cpp $(SRC)/ip.cpp $(SRC)/scmain.cpp $(SRC)/vp.cpp $(SRC)/ddr3.cpp $(SRC)/tb_vp.cpp 

# Object files (move them to BIN)
OBJECTS=$(SOURCES:$(SRC)/%.cpp=$(BIN)/%.o)

# Executable name
EXECUTABLE=$(BIN)/vp_dijkstra

# Default target
all: $(EXECUTABLE)

# Build target
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# Compile target (store .o files in BIN)
$(BIN)/%.o: $(SRC)/%.cpp | $(BIN)
	$(CC) $(CFLAGS) $< -o $@

# Ensure BIN folder exists
$(BIN):
	mkdir -p $(BIN)

# Clean target
clean:
	rm -f $(BIN)/*.o $(EXECUTABLE)

