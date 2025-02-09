/**
 *    ========== Mandelbrot Fractal Zoomer ==========
 * Render Mandelbrot fractal zooms like the ones you see on 
 * YouTube! Just give rendering parameters to the program,
 * and it will start rendering your Mandelbrot movie!
 */
#pragma once
#include "datatypes.hpp"

/*
	Some useful optimization macros 
*/
#define MANDELBROT_INLINE __attribute__((always_inline)) inline 

/*
	Struct that holds all arguments of the Mandelbrot renderer.
	The file "base.cpp" is allowed to interact with the specifics 
	of this struct, as long as it is rendering-related.
*/
struct MandelbrotGlobals {
	unsigned char* pixels;				/* the pixel array */
	unsigned width;						/* width in pixels */
	unsigned height;					/* height in pixels */
	unsigned iterations;				/* iteration count */
	unsigned precision;					/* precision in bits */
	Real radius;						/* escape radius */
	mpfr_t real;						/* starting real position */
	mpfr_t imag;						/* starting imag position */
	mpfr_t multiplier;					/* multiplier (inverse magnification) */
	Complex* perturbation;				/* perturbation iterations */
	unsigned perturbation_iters;		/* number of perturbation iterations */

	mpfr_t start_multiplier;			/* starting multiplier */
	mpfr_t end_multiplier;				/* ending multiplier */
	mpfr_t keyframe_multiplier;			/* keyframe multiplier */
	mpfr_t half_keyframe_multiplier;	/* half keyframe multiplier */
};

/*
	Initialize the MandelbrotGlobals struct.
*/
void mandelbrot_start(
	MandelbrotGlobals& globals,
	unsigned char* pixels,
	unsigned width,
	unsigned height,
	unsigned iterations,
	const char* real,
	const char* imag,
	const char* zoom,
	unsigned prec,
	const char* ezoom 
);

/*
	Given the arguments below, and any other information, render 
	the Mandelbrot set to a pixel array.
*/
void mandelbrot(const MandelbrotGlobals& globals);