.PHONY: all install uninstall clean

name = tuxfishing

all:
	gcc ../main.c -I ../inc -lSDL2 -lGLESv2 -lEGL -Ofast -lm -o $(name)

install:
	cp $(name) $(DESTDIR)

uninstall:
	rm $(DESTDIR)/$(name)

clean:
	rm $(name)