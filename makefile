CC = gcc
WINCC = x86_64-w64-mingw32-gcc

CFLAGS += -O3

SRC_DIR = src
INC_DIR = inc
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

OUT = program

CPPFLAGS += -I$(INC_DIR) -Iglad/include

# Linux libraries
LDLIBS += -lOpenCL -lGL -lglfw -ldl -lX11 -lm -lpng -lz

all: $(OUT)

# Compile objects (Linux)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# GLAD (Linux)
GLAD_SRC = glad/src/glad.c
GLAD_OBJ = $(OBJ_DIR)/glad.o

$(GLAD_OBJ): $(GLAD_SRC)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Link (Linux)
$(OUT): $(OBJS) $(GLAD_OBJ)
	$(CC) $(OBJS) $(GLAD_OBJ) -o $@ $(LDLIBS)

clean:
	rm -f $(OBJS) $(GLAD_OBJ) $(OBJS:.o=.d) $(OUT) $(OUT).exe

-include $(OBJS:.o=.d)

.PHONY: all windows clean