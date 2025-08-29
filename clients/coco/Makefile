TARGET_EXEC ?= news

BUILD_DIR ?= ./build
DIST_DIR ?= ./dist
SRC_DIRS ?= ./src

CC=cmoc

SRCS := $(shell find $(SRC_DIRS) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

CFLAGS=
LDFLAGS=-o$(TARGET_EXEC).bin

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

$(DIST_DIR)/$(TARGET_EXEC).dsk: $(OBJS) | $(DIST_DIR)
	$(CC) $(LDFLAGS) $(OBJS)
	$(RM) $(TARGET_EXEC)
	decb dskini $@
	writecocofile $@ $(TARGET_EXEC).bin
#	cp $(TARGET_EXEC).dsk ~/tnfs/

# c source
$(BUILD_DIR)/%.c.o: %.c
	$(MKDIR_P) $(dir $@)
	$(CC) $(CFLAGS) -o $@ -c $<

$(DIST_DIR):
	$(MKDIR_P) $@

.PHONY: clean disk

disk: $(DIST_DIR)/$(TARGET_EXEC).dsk

clean:
	$(RM) -rf $(TARGET_EXEC)* build

-include $(DEPS)

MKDIR_P ?= mkdir -p
