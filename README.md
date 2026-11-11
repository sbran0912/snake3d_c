# Build

Voraussetzungen: CMake, Ninja, GLFW, OpenGL sowie beide Compiler (clang und gcc).

Es gibt zwei parallele Builds, die sich nicht gegenseitig stören:

| Build       | Compiler | Ausgabe         | Artefakt-Verzeichnis |
| ----------- | -------- | --------------- | -------------------- |
| CMake/Ninja | clang    | `build/path3d`  | `build/`             |
| Make        | gcc      | `bin/path3d`    | `bin/`, `obj/`       |

## CMake + Ninja (clang)

Aus dem Projektverzeichnis `path3d_c` heraus:

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Das Binary liegt danach unter `build/path3d`.

Nach Code-Änderungen genügt:

```sh
cmake --build build
```

Neu konfigurieren (`cmake -S . -B build -G Ninja`) muss man nur, wenn sich
`CMakeLists.txt` ändert, neue Dateien hinzukommen oder `build/` gelöscht wurde.

> Hinweis: Der Compiler ist in `CMakeLists.txt` fest auf `clang` gesetzt. Ein
> bereits mit einem anderen Compiler konfiguriertes `build/` muss vorher
> gelöscht werden (`rm -rf build`). Überschreiben lässt sich der Compiler mit
> `-DCMAKE_C_COMPILER=...`.

## Make (gcc)

```sh
make              # baut bin/path3d
make run          # baut bei Bedarf und startet bin/path3d
make clean        # entfernt bin/ und obj/
```

## Make (Targets für den CMake-Build)

Alternativ lässt sich der CMake/Ninja-Build (clang) auch über das Makefile anstoßen:

```sh
make cmake        # konfiguriert und baut build/path3d
make cmake-init   # konfiguriert (cmake -S . -B build -G Ninja)
make cmake-run    # baut und startet build/path3d
make cmake-clean  # entfernt build/
```

`make` bleibt dabei weiterhin der gcc-Build nach `bin/path3d`.

## Starten

```sh
./build/path3d     # clang-Build
./bin/path3d       # gcc-Build
```

Beide öffnen ein GLFW/OpenGL-Fenster und brauchen daher eine grafische Umgebung.

## Editor / clangd

Der CMake-Build erzeugt `build/compile_commands.json` (clang). Damit clangd
die Compile-Flags automatisch findet, verweist die Datei `.clangd` im
Projektwurzelverzeichnis auf dieses Verzeichnis:

```yaml
CompileFlags:
  CompilationDatabase: build
```
