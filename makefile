.PHONY: all web test deps clean

name = TuxFishing

all:
	mkdir -p release
	cc main.c -I inc -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o release/$(name)_linux
	strip --strip-unneeded release/$(name)_linux
	upx --lzma --best release/$(name)_linux

web:
	emcc main.c -DWEB -O3 --closure 0 -s FILESYSTEM=0 -s USE_SDL=2 -s ENVIRONMENT=web -s TOTAL_MEMORY=256MB -I inc -o web/index.html --shell-file t.html

test:
	gcc main.c -I inc -Ofast -lSDL2 -lGLESv2 -lEGL -lm -o /tmp/$(name)_test
	/tmp/$(name)_test
	rm /tmp/$(name)_test

deps:
	@echo https://emscripten.org/docs/getting_started/downloads.html
	@echo https://github.com/upx/upx/releases/tag/v4.2.4
	sudo apt install libsdl2-dev libsdl2-2.0-0
	sudo apt install upx-ucl

clean:
	rm -rf release
	rm -f web/index.html
	rm -f web/index.js
	rm -f web/index.wasm