
BIN_DIR=./build/bin
LIB_DIR=./build/lib
RES_DIR=./build/res
RESOURCES_DIR=./resources
SRC_DIR=./src

$(BIN_DIR)/LustriousPaint.exe: $(SRC_DIR)/main.rs $(LIB_DIR)/libctypes.rlib $(LIB_DIR)/libwin32.rlib $(RES_DIR)/icon.res $(RES_DIR)/version.res | $(BIN_DIR)
	rustc $< -L $(LIB_DIR) -C link_args="-Wl,--subsystem,windows build/res/icon.res build/res/version.res" -L ./ -o $@

$(LIB_DIR)/libctypes.rlib: $(SRC_DIR)/ctypes.rs | $(LIB_DIR)
	rustc $< -L $(LIB_DIR) --out-dir=$(LIB_DIR)

$(LIB_DIR)/libwin32.rlib: $(SRC_DIR)/win32/lib.rs $(SRC_DIR)/win32/*.rs | $(LIB_DIR)
	rustc $(SRC_DIR)/win32/lib.rs -L $(LIB_DIR) --out-dir=$(LIB_DIR)

$(RES_DIR)/icon.res: $(RESOURCES_DIR)/icon.rc $(RESOURCES_DIR)/icon.ico | $(RES_DIR)
	windres.exe -i $< -O coff -o $@

$(RES_DIR)/version.res: $(RESOURCES_DIR)/version.rc | $(RES_DIR)
	windres.exe -i $< -O coff -o $@

$(RES_DIR):
	mkdir -p $@

$(LIB_DIR):
	mkdir -p $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf ./build
