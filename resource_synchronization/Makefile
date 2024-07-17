CXX = g++
CXXFLAGS = -std=c++11 -Wall -g
TARGET = main
SOURCES = src/*.cpp 
INCLUDE = include/*.h
OUTPUT_DIR = data/output

all: main

$(TARGET): $(SOURCES) $(INCLUDE)
	$(CXX) $(CXXFLAGS) -pthread  $(SOURCES) -o $@

clean:
	-rm -rf $(TARGET)

run: $(TARGET)
	./$(TARGET) data/main-tests.dat 575 2

test: $(TARGET)
	@echo "Running Tests:"
	@echo "Main Test"
	./$(TARGET) data/main-tests.dat 750 3 > $(OUTPUT_DIR)/edge-cases.txt
	@echo "Basic Deadlock Scenario"
	./$(TARGET) data/deadlock-scenario.dat 500 3 > $(OUTPUT_DIR)/deadlock-scenario.txt
	@echo "Dining Philosophers Problem"
	./$(TARGET) data/dining-philosophers.dat 500 3 > $(OUTPUT_DIR)/dining-philosophers.txt
	@echo "Tests complete."

clean_test:
	-rm -rf $(OUTPUT_DIR)/*.txt