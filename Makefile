CC=g++
CFLAGS= -O2 -msse2 -Wall -Werror
OBJS = main.o fs.o bmp.o convert.o render.o
LIBS = -lpthread
PROG = yuv_and_rgb

all: 		$(PROG)

$(PROG):	$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o yuv_and_rgb

main.o:		fs.h
fs.o:		fs.h
bmp.o:		fs.h bmp.h
convert.o:	convert.h
render.o:	render.h

clean:
		rm -f *~ *.o $(PROG) core a.out
