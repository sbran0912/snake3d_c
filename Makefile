# Paralleler Build mit gcc und clang (cmake).
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c17 -Ilib3d
LDFLAGS = -lglfw -lGL -lm

APPNAME = snake
SRCS = lib3d/glcore.c lib3d/lib3d_util.c lib3d/lib3d.c app/grid.c app/snake.c app/main.c
OBJDIR = obj
BINDIR = bin
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
TARGET = $(BINDIR)/$(APPNAME)

# CMake/Ninja-Build (clang) - parallel zum gcc-Build oben.
BUILDDIR = build

.PHONY: all clean run cmake cmake-debug cmake-clean cmake-init cmake-run cmake-tidy

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

# cmake builds
cmake:
	cmake -S . -B $(BUILDDIR) -G Ninja -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILDDIR)

# Debug-Build (-O0 -g3), Binary unter build/snake.
cmake-debug:
	cmake -S . -B $(BUILDDIR) -G Ninja -DCMAKE_BUILD_TYPE=Debug
	cmake --build $(BUILDDIR)

cmake-clean:
	rm -rf $(BUILDDIR)

cmake-run: cmake
	./$(BUILDDIR)/$(APPNAME)

cmake-init:
	cmake -S . -B $(BUILDDIR) -G Ninja

# clang-tidy ueber die Compile-Datenbank aus build/.
# cmake-init stellt sicher, dass build/compile_commands.json existiert.
cmake-tidy: cmake-init
	@echo "----------------------------"
	@echo "Prüfen auf Speicherfehler..."
	@echo "----------------------------"
	clang-tidy -p $(BUILDDIR) app/*.c
