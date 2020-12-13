CC=gcc
CFLAGS=-Wall -pedantic
LDFLAGS=-pthread -lm
OBJ_BLT = iccpa_fopaes_blt.o calculate_collisions_blt_float.o calculate_collisions_blt_double.o calculations_float.o calculations_double.o
OBJ_BI = iccpa_fopaes_bi.o calculate_collisions_bi_float.o calculate_collisions_bi_double.o calculations_float.o calculations_double.o

default: iccpa_fopaes

iccpa_fopaes: iccpa_fopaes_blt iccpa_fopaes_bi

iccpa_fopaes_blt: iccpa_fopaes_blt.o
	$(CC) $(CFLAGS) -o iccpa_fopaes_blt iccpa_fopaes_blt.o $(LDFLAGS)

iccpa_fopaes_blt.o: main_blt.c iccpa.h
	$(CC) $(CFLAGS) -o iccpa_fopaes_blt.o -c main_blt.c

iccpa_fopaes_bi: iccpa_fopaes_bi.o
	$(CC) $(CFLAGS) -o iccpa_fopaes_bi iccpa_fopaes_bi.o $(LDFLAGS)

iccpa_fopaes_bi.o: main_bi.c iccpa.h
	$(CC) $(CFLAGS) -o iccpa_fopaes_bi.o -c main_bi.c

#iccpa_fopaes: iccpa_fopaes_blt iccpa_fopaes_bi
#
#iccpa_fopaes_blt: $(OBJ_BLT)
#	$(CC) $(CFLAGS) -o iccpa_fopaes_blt $(OBJ_BLT) $(LDFLAGS)
#	
#iccpa_fopaes_blt.o: main_blt.c iccpa.h
#	$(CC) $(CFLAGS) -o iccpa_fopaes_blt.o -c main_blt.c
#	
#calculate_collisions_blt_float.o: calculate_collisions_blt.c iccpa.h
#	$(CC) $(CFLAGS) -o calculate_collisions_blt_float.o -c -DTYPE_SUFFIX_f calculate_collisions_blt.c
#	
#calculate_collisions_blt_double.o: calculate_collisions_blt.c iccpa.h
#	$(CC) $(CFLAGS) -o calculate_collisions_blt_double.o -c -DTYPE_SUFFIX calculate_collisions_blt.c
#	
#calculations_float.o: calculations.c iccpa.h
#	$(CC) $(CFLAGS) -o calculations_float.o -c -DTYPE_SUFFIX_f calculations.c
#
#calculations_double.o: calculations.c iccpa.h
#	$(CC) $(CFLAGS) -o calculations_double.o -c -DTYPE_SUFFIX calculations.c
#	
#iccpa_fopaes_bi: $(OBJ_BI)
#	$(CC) $(CFLAGS) -o iccpa_fopaes_bi $(OBJ_BI) $(LDFLAGS)
#	
#iccpa_fopaes_bi.o: main_bi.c iccpa.h
#	$(CC) $(CFLAGS) -o iccpa_fopaes_bi.o -c main_bi.c
#	
#calculate_collisions_bi_float.o: calculate_collisions_bi.c iccpa.h
#	$(CC) $(CFLAGS) -o calculate_collisions_bi_float.o -c -DTYPE_SUFFIX_f calculate_collisions_bi.c
#	
#calculate_collisions_bi_double.o: calculate_collisions_bi.c iccpa.h
#	$(CC) $(CFLAGS) -o calculate_collisions_bi_double.o -c -DTYPE_SUFFIX calculate_collisions_bi.c

clean: 
	$(RM) iccpa_fopaes_blt iccpa_fopaes_blt *.o *~
