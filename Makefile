# Paralleler Build mit gcc und clang (cmake).
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c17 -Ilib3d
CFLAGS_DEBUG = -Wall -Wextra -O0 -g3 -std=c17 -Ilib3d
LDFLAGS = -lglfw -lGL -lm

APPNAME = snake
SRCS = lib3d/glcore.c lib3d/lib3d_util.c lib3d/lib3d.c app/grid.c app/snake.c app/main.c
OBJDIR = obj
BINDIR = bin
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
TARGET = $(BINDIR)/$(APPNAME)

.PHONY: all clean run tidy debug init

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJDIR) $(BINDIR)

run: $(TARGET)
	./$(TARGET)

# Debug-Build mit gcc (-O0 -g3). Nutzt dieselben Verzeichnisse wie der
# Release-Build (obj/, bin/), deshalb wird vorher sauber geloescht:
# Wechsel zwischen Release und Debug immer ueber `make clean`.
debug: clean
	$(MAKE) all CFLAGS="$(CFLAGS_DEBUG)"

init: clean
	bear -- $(MAKE) -B $(OBJS) CFLAGS="$(CFLAGS) -fsyntax-only"
	@sed -i 's/"-fsyntax-only",//g' compile_commands.json
