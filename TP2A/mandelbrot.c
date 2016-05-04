#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lodepng.h"
#include "dccthread.h"

#define DIM 2048

/*#define SIZE 0.000002925302
 #define X_COORD -0.7545735446455
 #define Y_COORD	-0.0551496797864*/

#define SIZE 4.0
#define X_COORD 0.0
#define Y_COORD	0.0

#define n_iter 512

#define RES_MUL 1

#define squared 1

#define multiplier_set 0

#if multiplier_set == 1
#define multiplier 0.15
#else
#define multiplier 1.0
#endif

#if squared == 1
#define X_SIZE SIZE
#define Y_SIZE SIZE
#define X_DIM DIM
#define Y_DIM DIM
#else
#define X_SIZE (SIZE*16/9)
#define Y_SIZE SIZE
#define X_DIM (1920*RES_MUL)
#define Y_DIM (1080*RES_MUL)
#endif

#define R_CONST 16.0*multiplier			//default 16.0
#define G_CONST 32.0*multiplier			//default 32.0
#define B_CONST 64.0*multiplier			//default 64.0

#define SET_CENTER 1

#if SET_CENTER == 1
#define X_REND (X_COORD - (X_SIZE / 2.0))
#define Y_REND (Y_COORD - (Y_SIZE / 2.0))
#else
#define X_REND X_COORD
#define Y_REND Y_COORD
#endif

#define T_TYPE double 

#define fi (T_TYPE)i
#define fj (T_TYPE)j

#define a ((X_SIZE*fi / (T_TYPE)(X_DIM-1)) + X_REND)
#define b ((Y_SIZE*fj / (T_TYPE)(Y_DIM-1)) + Y_REND)
#define iterate() {T_TYPE tmp=x*x-y*y +a;y=2*x*y+b;x=tmp;}

#define colouring_function() (n + 1 - log(log(sqrt(x2 + y2))) / log(2))
#define palette_function(RES,COLOUR_CONST) (unsigned char)(RES != 0.0 ?(128 + 128 * sin(RES / COLOUR_CONST)):0)

//unsigned long n;
//T_TYPE x, y;

//n - log((log(sqrt((x*x) + (y*y))) / log(2)))

double diverges(int i, int j) {
	T_TYPE x = 0.0, y = 0.0;
	x = 0.0, y = 0.0;
	unsigned long n;
	T_TYPE x2, y2;						//code optimization
	T_TYPE tmp;
	T_TYPE real = a;
	T_TYPE imaginaria = b;
	for (n = 0; n < n_iter; n++) {
		x2 = x * x;
		y2 = y * y;
		tmp = x2 - y2 + real;
		y = 2 * x * y + imaginaria;
		x = tmp;
		if (x2 + y2 > 4.0) {
			return colouring_function();
		}
	}
	return 0.0;
}

unsigned char *image;

void draw_line(int j) {
	int i;
	for (i = 0; i < X_DIM; i++) {
		double result = diverges(i, (Y_DIM - j));
		image[3 * (X_DIM * j + i)] = palette_function(result, R_CONST);
		image[3 * (X_DIM * j + i) + 1] = palette_function(result, G_CONST);
		image[3 * (X_DIM * j + i) + 2] = palette_function(result, B_CONST);
	}
	dccthread_exit();
}

void mandelbrot(int nthreads) {
	int i, j;
	image = (unsigned char *) malloc(3 * X_DIM * Y_DIM);
	dccthread_t *t[nthreads];
	char state[nthreads];
	for (i = 0; i < nthreads; i++)
		state[0];

	for (j = 0; j < Y_DIM; j++) {
		if (j >= nthreads) {
			dccthread_wait(t[j % nthreads]);
			state[j % nthreads] = 0;
		}
		t[j % nthreads] = dccthread_create("thread", draw_line, j);
		state[j % nthreads] = 1;
	}

	for (j = 0; j < nthreads; j++) {
		dccthread_wait(t[j]);
	}
	//exit(1);
	lodepng_encode24_file("mandelbrot_output.png", image, X_DIM, Y_DIM);
	free(image);
	dccthread_exit();
}

int main(int argc, char* argv[]) {
	dccthread_init(mandelbrot, 8);
	return 0;
}

