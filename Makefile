#target: dependencies
#	action

all: myhie coord quicksort custom_mergesort

myhie: myhie.o
	gcc myhie.o -o myhie

myhie.o: myhie.c myhie.h
	gcc -c myhie.c

coord: coord.o coord_func.o file_ops.o
	gcc coord.o coord_func.o file_ops.o -o coord

coord.o: coord.c coord_func.h file_ops.h 
	gcc -c coord.c

coord_func.o: coord_func.c coord_func.h
	gcc -c coord_func.c

file_ops.o: file_ops.c
	gcc -c file_ops.c

custom_mergesort: custom_mergesort.o coord_func.o file_ops.o 
	gcc custom_mergesort.o coord_func.o file_ops.o -o custom_mergesort

custom_mergesort.o: custom_mergesort.c coord_func.h file_ops.h 
	gcc -c custom_mergesort.c

quicksort: quicksort.o coord_func.o file_ops.o 
	gcc quicksort.o coord_func.o file_ops.o -o quicksort

quicksort.o: quicksort.c coord_func.h file_ops.h
	gcc -c quicksort.c

clean:
	rm *.o myhie coord quicksort custom_mergesort