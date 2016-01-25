include config.mk

BIN_DIR = $(CFG_BUILD_DIR)/bin
LIB_DIR = $(CFG_BUILD_DIR)/lib
SRC_DIR = ./

ifeq ($(CFG_CPUTYPE), x86)
	NASM_FLAGS := $(NASM_FLAGS) -f elf
	GCC_ARCH := -m32
else
	NASM_FLAGS := $(NASM_FLAGS) -f elf64
 	GCC_ARCH := -m64
endif

ifeq ($(CFG_OSTYPE), windows)
	NASM_PREFIX := _
endif

ifdef NASM_PREFIX
	NASM_FLAGS := $(NASM_FLAGS) --prefix $(NASM_PREFIX)
endif 

LIB_TGT=$(LIB_DIR)/libcbpad_$(CFG_CPUTYPE).a
TEST_TGT=$(BIN_DIR)/cbpad_test_$(CFG_CPUTYPE)
CBPAD_C_OBJ_TGT=$(SRC_DIR)/cbpad_c_$(CFG_CPUTYPE).o
CBPAD_C_TEST_OBJ_TGT=$(SRC_DIR)/cbpad_c_test_$(CFG_CPUTYPE).o
CBPAD_NASM_OBJ_TGT=$(SRC_DIR)/cbpad_nasm_$(CFG_CPUTYPE).o
CONFIG_MK=$(SRC_DIR)/config.mk


.PHONY: libcbpad
libcbpad: $(LIB_TGT)

$(LIB_TGT): $(CBPAD_OBJ_TGT) $(CBPAD_NASM_OBJ_TGT) $(CONFIG_MK) | $(LIB_DIR)
	ar rcs $@ $+

$(CBPAD_C_OBJ_TGT): $(SRC_DIR)/cbpad.c $(SRC_DIR)/cbpad.h $(CONFIG_MK)
	gcc -DCPU_TYPE=$(CFG_CPUTYPE) $(GCC_ARCH) -c $< -o $@

$(CBPAD_C_TEST_OBJ_TGT): $(SRC_DIR)/cbpad.c $(SRC_DIR)/cbpad.h $(CONFIG_MK)
	gcc -DCPU_TYPE=$(CFG_CPUTYPE) $(GCC_ARCH) -std=c99 -DTEST -c $< -o $@

$(CBPAD_NASM_OBJ_TGT): $(SRC_DIR)/cbpad_$(CFG_CPUTYPE).nasm $(CONFIG_MK)
	nasm $(NASM_FLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(LIB_DIR)/libcbpad_{x86,x86-64}.a
	rm -f $(SRC_DIR)/*.o
	rm -f $(BIN_DIR)/cbpad_test*

.PHONY: test
test: $(TEST_TGT)
	$(TEST_TGT)

$(TEST_TGT): $(CBPAD_NASM_OBJ_TGT) $(CBPAD_C_TEST_OBJ_TGT) | $(BIN_DIR)
	gcc -DCPU_TYPE=$(CFG_CPUTYPE) $(GCC_ARCH) -std=c99 $+ -o $@

$(LIB_DIR):
	mkdir -p ./build/lib

$(BIN_DIR):
	mkdir -p ./build/bin