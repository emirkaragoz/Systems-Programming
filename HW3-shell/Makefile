FLAGS= -Wall -ansi -pedantic -errors -std=gnu99

all: gtushell lsf pwd cat wc bunedu

gtushell: gtushell.o
	gcc -o gtushell gtushell.o
gtushell.o: gtushell.c 
	gcc -c ${FLAGS} gtushell.c

lsf: lsf.o
	gcc -o lsf lsf.o
lsf.o: lsf.c 
	gcc -c ${FLAGS} lsf.c

pwd: pwd.o
	gcc -o pwd pwd.o
pwd.o: pwd.c 
	gcc -c ${FLAGS} pwd.c

cat: cat.o
	gcc -o cat cat.o
cat.o: cat.c 
	gcc -c ${FLAGS} cat.c

wc: wc.o
	gcc -o wc wc.o
wc.o: wc.c 
	gcc -c ${FLAGS} wc.c

bunedu: buNeDu.o
	gcc -o bunedu buNeDu.o
buNeDu.o: buNeDu.c 
	gcc -c ${FLAGS} buNeDu.c

clean:
	rm -rf *.o gtushell lsf pwd cat wc bunedu