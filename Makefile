CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = 

# Object files
OBJS = storage_mgr.o dberror.o

# Test executables
TEST_EXEC = test_assign1
ADDITIONAL_TEST_EXEC = test_additional

# Source files
SRCS = storage_mgr.c dberror.c test_assign1_1.c test_additional.c

# Header files
HEADERS = storage_mgr.h dberror.h test_helper.h

# Default target
all: $(TEST_EXEC) $(ADDITIONAL_TEST_EXEC)

# Build the main test executable
$(TEST_EXEC): $(OBJS) test_assign1_1.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build the additional test executable
$(ADDITIONAL_TEST_EXEC): $(OBJS) test_additional.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Build object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run basic tests
test: $(TEST_EXEC)
	./$(TEST_EXEC)

# Run all tests including additional ones
test-all: $(TEST_EXEC) $(ADDITIONAL_TEST_EXEC)
	@echo "Running basic tests..."
	./$(TEST_EXEC)
	@echo "\nRunning additional tests..."
	./$(ADDITIONAL_TEST_EXEC)

# Clean build artifacts
clean:
	rm -f *.o $(TEST_EXEC) $(ADDITIONAL_TEST_EXEC) *.bin

# Clean and rebuild
rebuild: clean all

# Show help
help:
	@echo "Available targets:"
	@echo "  all       - Build all test executables"
	@echo "  test      - Build and run basic tests"
	@echo "  test-all  - Build and run all tests"
	@echo "  clean     - Remove build artifacts"
	@echo "  rebuild   - Clean and rebuild"
	@echo "  help      - Show this help message"

.PHONY: all test test-all clean rebuild help