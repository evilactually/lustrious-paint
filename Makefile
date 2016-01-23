
BIN_DIR=./build/bin
LIB_DIR=./build/lib
SRC_DIR=./src

NASM="C:\Users\geo\AppData\Local\nasm\nasm.exe"

all: $(LIB_DIR)/libctypes.rlib $(LIB_DIR)/libwin32.rlib | $(BIN_DIR)
	rustc main.rs -L $(LIB_DIR) -L ./ -C link-args='adder.o' --out-dir=$(BIN_DIR)

$(LIB_DIR)/libctypes.rlib: $(SRC_DIR)/ctypes.rs | $(LIB_DIR)
	rustc $< -L $(LIB_DIR) --out-dir=$(LIB_DIR)

$(LIB_DIR)/libwin32.rlib: $(SRC_DIR)/win32/lib.rs | $(LIB_DIR)
	rustc $(SRC_DIR)/win32/lib.rs -L $(LIB_DIR) --out-dir=$(LIB_DIR)

$(LIB_DIR):
	mkdir -p ./build/lib

$(BIN_DIR):
	mkdir -p ./build/bin

callbacks:
	$(NASM) -f elf adder.nasm
	gcc main.c adder.o -o cb.exe