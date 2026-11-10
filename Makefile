# Paralleler Build mit gcc (der CMake/Ninja-Build nutzt clang).
CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c17 -Ilib3d
LDFLAGS = -lglfw -lGL -lm

SRCS = lib3d/glcore.c lib3d/lib3d_util.c lib3d/lib3d_mesh.c lib3d/lib3d.c src/main.c
OBJDIR = obj
BINDIR = bin
OBJS = $(patsubst %.c,$(OBJDIR)/%.o,$(SRCS))
TARGET = $(BINDIR)/path3d

# CMake/Ninja-Build (clang) - parallel zum gcc-Build oben.
BUILDDIR = build

.PHONY: all clean run cmake cmake-clean cmake-run

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Startet den gcc-Build (baut ihn vorher bei Bedarf).
run: $(TARGET)
	./$(TARGET)

# Konfiguriert neu und baut den clang-Build nach $(BUILDDIR)/path3d.
cmake:
	cmake -S . -B $(BUILDDIR) -G Ninja
	cmake --build $(BUILDDIR)

# Entfernt den CMake-Build.
cmake-clean:
	rm -rf $(BUILDDIR)

# Baut und startet den clang-Build ($(BUILDDIR)/path3d).
cmake-run: cmake
	./$(BUILDDIR)/path3d
