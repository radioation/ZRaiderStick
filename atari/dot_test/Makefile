# ---- tools ----
CC      := cl65
AS      := ca65
LD      := cl65

# ---- project ----
TARGET 		:= atari
PROGRAM     := dot_test
BUILD_DIR   := build
OUT         := $(BUILD_DIR)/$(PROGRAM).xex


CFG			:= atari-xex.cfg


# ---- sources ----
C_SRCS  := $(wildcard src/*.c)
S_SRCS  := $(wildcard src/*.s)

C_OBJS  := $(patsubst src/%.c,$(BUILD_DIR)/%.o,$(C_SRCS))
S_OBJS  := $(patsubst src/%.s,$(BUILD_DIR)/%.o,$(S_SRCS))
OBJS    := $(C_OBJS) $(S_OBJS)

DEPS    := $(C_OBJS:.o=.d)

# ---- flags ----
# Common include path(s)
INCLUDES := -I src -I include

# cl65 will pass appropriate parts to compiler/assembler/linker.
# -t atari selects platform (libs/headers/startup). Change as needed: c64, apple2, etc.
CFLAGS   := -t $(TARGET) -Oirs $(INCLUDES) --add-source
ASFLAGS  := -t $(TARGET) $(INCLUDES)
LDFLAGS  := -t $(TARGET) -C $(CFG) -m $(BUILD_DIR)/$(PROGRAM).map \
            -Ln $(BUILD_DIR)/$(PROGRAM).lbl

# Optional: define symbols for both C and assembly
# DEFINES := -DDEBUG -DVERSION=1
DEFINES :=

# ---- default ----
.PHONY: all
all: $(OUT)

# ---- link ----
$(OUT): $(OBJS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(DEFINES) -o $@ $(OBJS)

# ---- compile C -> .o (+ .d) ----
# cl65 can emit dependency files via --create-dep
$(BUILD_DIR)/%.o: src/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(DEFINES) --create-dep $(BUILD_DIR)/$*.d -c -o $@ $<

# ---- assemble .s -> .o ----
$(BUILD_DIR)/%.o: src/%.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $(DEFINES) -o $@ $<

# ---- build dir ----
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# ---- deps ----
-include $(DEPS)

# ---- utility targets ----
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: run
run: $(OUT)
	@echo "Built: $(OUT)"
	@echo "Run it in Altirra / your emulator: $(OUT)"

# Example: show symbols/labels file path
.PHONY: symbols
symbols:
	@echo "$(BUILD_DIR)/$(TARGET).lbl"

