
BUILD_DIR ?= ./build
BIN_DIR = $(BUILD_DIR)/bin
LIB_DIR = $(BUILD_DIR)/lib
SRC_DIR = ./

$(LIB_DIR)/libcbpad.a: $(SRC_DIR)/cbpad_nasm.o $(SRC_DIR)/cbpad_c.o | $(LIB_DIR)
	ar rcs $@ $+

$(SRC_DIR)/cbpad_c.o: $(SRC_DIR)/cbpad.c
	gcc -c $< -o $@

$(SRC_DIR)/cbpad_c_test.o: $(SRC_DIR)/cbpad.c
	gcc -std=c99 -DTEST -c $< -o $@

$(SRC_DIR)/cbpad_nasm.o: $(SRC_DIR)/cbpad.nasm
	nasm -f elf $< -o $@

.PHONY: clean
clean:
	rm -f $(LIB_DIR)/libcbpad.a
	rm -f $(SRC_DIR)/cbpad_c.o
	rm -f $(SRC_DIR)/cbpad_nasm.o

TEST_TGT=$(BIN_DIR)/cbpad_test

.PHONY: test
test: $(TEST_TGT)
	$(TEST_TGT)

$(TEST_TGT): $(SRC_DIR)/cbpad_nasm.o $(SRC_DIR)/cbpad_c_test.o | $(BIN_DIR)
	gcc -std=c99 $+ -o $@

$(LIB_DIR):
	mkdir -p ./build/lib

$(BIN_DIR):
	mkdir -p ./build/bin