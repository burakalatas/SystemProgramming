all:create combine extract

create:
	gcc tarsau.c -o tarsau

combine:
	./tarsau -b t1 t2 t3 t4.txt t5.dat -o s1.sau

extract:
	./tarsau -a s1.sau d1