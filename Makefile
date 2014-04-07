CFLAGS= -O6 `sdl-config --cflags` -DEXECZ80 -DDEBUG -DLSB_FIRST
LDFLAGS= `sdl-config --libs` -lSDL_image -lm

OBJS= app.o Debug.o Z80.o fm.o sn76496.o snd.o sndp.o main.o vt100.o async.o fifo.o y.tab.o lex.yy.o emu.o link.o 

z80: ${OBJS}
	gcc ${OBJS} ${LDFLAGS} -o z80

	
%.o: %.c
	gcc -c $< ${CFLAGS} 

y.tab.c y.tab.h: lang.y
	yacc -d lang.y

lex.yy.c: lang.l y.tab.h
	lex lang.l

clean:
	rm -f ${OBJS} z80 z80.core y.tab.c y.tab.h lex.yy.c

