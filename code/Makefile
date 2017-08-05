
.PHONY: all release debug clean

all: release

release: CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Release
release: TARGET_DIR=build/release

debug: CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Debug
debug: TARGET_DIR=build/debug

release debug: punter

punter: $(TARGET_DIR)/punter
.PHONY: punter

$(TARGET_DIR)/punter: $(wildcard src/*cpp) $(wildcard src/*hpp) $(TARGET_DIR)/Makefile
	cd $(TARGET_DIR) && $(MAKE)

$(TARGET_DIR)/Makefile:
	echo $0
	mkdir -p $(TARGET_DIR) \
		&& cd $(TARGET_DIR) \
		&& cmake $(CMAKE_ARGS) ../../src


clean:
	[ -d build ] && rm -r build
