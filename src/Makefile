CC = gcc
# -pipe			speeds up compilation, using pipes instead of files
# -Wall 		all warnings
#
# -g 			debug info for gdb
#
# -O3 			speed>space optimization
# -funroll-loops	doesn't seem to do all that much, might be more useful when I move OR_SUMMATION to a loop
# -fomit-frame-pointer	makes debugging impossible but reduces run time by 7%!
# -march=pentium4	compiles for a specific processor architecture without backward compatibility
#  			(ssh-linux.ece.ubc.ca is a dual core genuine Pentium 4)
#
# -pg 			builds information for profiler (gprof)
#
#
# http://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/i386-and-x86-64-Options.html
#
#
CFLAGS = -pipe -Wall -O3 -march=pentium4 -funroll-loops -fomit-frame-pointer  
#CFLAGS = -pipe -Wall -O3 -march=pentium4 -funroll-loops -g

all:	bin strip

bin:	main log diskrw solve9 timer solve16
	$(CC) -o ../bin/sudoku-solve ../build/main.o ../build/log.o ../build/diskrw.o ../build/solve9.o ../build/dump.o ../build/timer.o ../build/solve16.o

main: main.c log.h global.h datum.h diskrw.h solve9.h solve16.h
	$(CC) $(CFLAGS) -c main.c -o ../build/main.o

log:	log.c log.h global.h
	$(CC) $(CFLAGS) -c log.c -o ../build/log.o

diskrw:	diskrw.c diskrw.h datum.h global.h
	$(CC) $(CFLAGS) -c diskrw.c -o ../build/diskrw.o

solve9:	solve9.c log.h datum.h single.h
	$(CC) $(CFLAGS) -c solve9.c -o ../build/solve9.o

solve16:	solve16.c log.h datum.h lookup16.h
	$(CC) $(CFLAGS) -c solve16.c -o ../build/solve16.o

# debug-only function that shouldn't get used much
dump:	dump.c log.h dump.h
	$(CC) $(CFLAGS) -c dump.c -o ../build/dump.o

timer:	timer.c timer.h log.h global.h
	$(CC) $(CFLAGS) -c timer.c -o ../build/timer.o


clean:
	rm -f build/*.o bin/sudoku-solve

strip:
	strip -s ../bin/sudoku-solve
# No speedup here, but eh.

