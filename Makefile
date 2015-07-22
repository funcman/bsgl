CFLAGS?=-Os -Wall

all: libbsgl.a

libbsgl.a: config.o control.o graphics.o random.o system.o timer.o bsglanim.o bsglrect.o bsglsprite.o bsglwidget.o tinystr.o tinyxml.o tinyxmlerror.o tinyxmlparser.o
	$(AR) -r libbsgl.a *.o

config.o: include/bsgl.h src/core/bsgl_impl.h src/dependencies/tinyxml/tinyxml.h src/core/config.cpp
	$(CC) $(CFLAGS) src/core/config.cpp -c -Iinclude -Isrc/dependencies/tinyxml

control.o: include/bsgl.h src/core/bsgl_impl.h src/core/control.cpp
	$(CC) $(CFLAGS) src/core/control.cpp -c -Iinclude

graphics.o: include/bsgl.h src/core/bsgl_impl.h src/core/graphics.cpp
	$(CC) $(CFLAGS) src/core/graphics.cpp -c -Iinclude

random.o: include/bsgl.h src/core/bsgl_impl.h src/core/random.cpp
	$(CC) $(CFLAGS) src/core/random.cpp -c -Iinclude

system.o: include/bsgl.h src/core/bsgl_impl.h src/core/system.cpp
	$(CC) $(CFLAGS) src/core/system.cpp -c -Iinclude

timer.o: include/bsgl.h src/core/bsgl_impl.h src/core/timer.cpp
	$(CC) $(CFLAGS) src/core/timer.cpp -c -Iinclude

bsglanim.o: include/bsgl.h include/bsglanim.h src/advance/bsglanim.cpp
	$(CC) $(CFLAGS) src/advance/bsglanim.cpp -c -Iinclude

bsglrect.o: include/bsgl.h include/bsglrect.h src/advance/bsglrect.cpp
	$(CC) $(CFLAGS) src/advance/bsglrect.cpp -c -Iinclude

bsglsprite.o: include/bsgl.h include/bsglsprite.h src/advance/bsglsprite.cpp
	$(CC) $(CFLAGS) src/advance/bsglsprite.cpp -c -Iinclude

bsglwidget.o: include/bsgl.h include/bsglwidget.h src/gui/bsglwidget.cpp
	$(CC) $(CFLAGS) src/gui/bsglwidget.cpp -c -Iinclude

tinystr.o: src/dependencies/tinyxml/tinystr.h src/dependencies/tinyxml/tinystr.cpp
	$(CC) $(CFLAGS) src/dependencies/tinyxml/tinystr.cpp -c -Isrc/dependencies/tinyxml

tinyxml.o: src/dependencies/tinyxml/tinyxml.h src/dependencies/tinyxml/tinyxml.cpp
	$(CC) $(CFLAGS) src/dependencies/tinyxml/tinyxml.cpp -c -Isrc/dependencies/tinyxml

tinyxmlerror.o: src/dependencies/tinyxml/tinyxml.h src/dependencies/tinyxml/tinyxmlerror.cpp
	$(CC) $(CFLAGS) src/dependencies/tinyxml/tinyxmlerror.cpp -c -Isrc/dependencies/tinyxml

tinyxmlparser.o: src/dependencies/tinyxml/tinyxml.h src/dependencies/tinyxml/tinyxmlparser.cpp
	$(CC) $(CFLAGS) src/dependencies/tinyxml/tinyxmlparser.cpp -c -Isrc/dependencies/tinyxml

install: all
	cp include/*.h /usr/include
	mv libbsgl.a /usr/lib
	rm -f *.o

uninstall:
	rm /usr/lib/libbsgl.a
	rm /usr/include/bsgl.h
	rm /usr/include/bsglanim.h
	rm /usr/include/bsglrect.h
	rm /usr/include/bsglsprite.h

clean:
	rm -f *.o
	rm -f libbsgl.a
