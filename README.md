# Platofmer in C

## Build
You need to have the last version [raylib](https//github.com/raysan5/raylib). 
Then put the `.dll`, `.a` or `.dylib` file into the `lib` folder.

# Linux/macOS
```console
gcc main.c -o bin/main -I./include -L./lib -lraylib
```

# Windows
```console
gcc main.c -o bin/main -I./include -L./lib -lraylib -lopengl32 -lgdi32 -lwinmm
```

To link statically on Windows, you can use `-l:libraylib.a` instead of `-lraylib`.