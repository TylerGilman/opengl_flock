CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c11
LIBS = -lglfw -lGL -lGLU -lm

EMCC = emcc
EMFLAGS = -O3 -s USE_WEBGL2=1 -s FULL_ES3=1 -s WASM=1 \
          -s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=1 \
          -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
          -s EXPORTED_FUNCTIONS='["_main","_resize_canvas"]'

TARGET = flock
WASM_JS = flock_wasm.js
SOURCES = main.c vector3d.c particle.c spatial_grid.c
WASM_SOURCES = main_wasm.c vector3d.c particle.c spatial_grid.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

wasm: $(WASM_SOURCES)
	$(EMCC) $(WASM_SOURCES) $(EMFLAGS) -o $(WASM_JS)
	@echo "Build complete! Open index.html in a browser (serve with: make serve)"

clean:
	rm -f $(OBJECTS) $(TARGET) flock_wasm.js flock_wasm.wasm

run: $(TARGET)
	./$(TARGET)

serve:
	python3 -m http.server 8080

.PHONY: all clean run wasm serve
