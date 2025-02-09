/**
 *    ========== Mandelbrot Fractal Zoomer ==========
 * Render Mandelbrot fractal zooms like the ones you see on 
 * YouTube! Just give rendering parameters to the program,
 * and it will start rendering your Mandelbrot movie!
 */
#include "./mandelbrot.hpp"
#include <cstdlib>
#include <cstring>

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
) {
	// For arbitrary precision reasons, some parameters are given 
	// as strings.
	globals.pixels = pixels;
	globals.width = width;
	globals.height = height;
	globals.iterations = iterations;
	globals.precision = prec;
	globals.radius = 100.0;
	mpfr_inits2(globals.precision,
		globals.real, globals.imag,
		globals.multiplier,
		globals.start_multiplier, globals.end_multiplier,
		globals.keyframe_multiplier, globals.half_keyframe_multiplier,
		(mpfr_ptr)0);
	mpfr_set_str(globals.real, real, 10, MPFR_RNDN);
	mpfr_set_str(globals.imag, imag, 10, MPFR_RNDN);

	// For a 1080x720 screen, the initial multiplier should be 0.00375.
	// For different sized screens, adjust the multiplier to mimic a 
	// fixed resolution effect.
	mpfr_set_d(globals.multiplier,
		(globals.width < globals.height) ?
			(0.00375 * 1080.0 / globals.width) :
			(0.00375 * 720.0 / globals.height),
		MPFR_RNDN);
	
	// Set starting and ending multipliers for zooms 
	mpfr_set_str(globals.start_multiplier, zoom, 10, MPFR_RNDN);
	mpfr_div(globals.start_multiplier, globals.multiplier, globals.start_multiplier, MPFR_RNDN);
	mpfr_set_str(globals.end_multiplier, ezoom, 10, MPFR_RNDN);
	mpfr_div(globals.end_multiplier, globals.multiplier, globals.end_multiplier, MPFR_RNDN);

	// Set keyframe multipliers for zooms 
	mpfr_set(globals.keyframe_multiplier, globals.start_multiplier, MPFR_RNDN);
	mpfr_mul_d(globals.half_keyframe_multiplier, globals.keyframe_multiplier, 0.5, MPFR_RNDN);

	// Set multiplier that changes every frame rendered 
	mpfr_set(globals.multiplier, globals.start_multiplier, MPFR_RNDN);

	// Calculate all perturbation iterations 
	mpfr_t z_re, z_im, z2_re, z2_im, temp;
	mpfr_inits2(globals.precision, z_re, z_im, z2_re, z2_im, temp, (mpfr_ptr)0);
	mpfr_set_zero(z_re, 0);
	mpfr_set_zero(z_im, 0);
	mpfr_set_zero(z2_re, 0);
	mpfr_set_zero(z2_im, 0);
	globals.perturbation = new Complex[globals.iterations];
	globals.perturbation_iters = globals.iterations;
	for (int i = 0; i < globals.iterations; ++i) {
		globals.perturbation[i] = Complex{
			mpfr_get_float128(z_re, MPFR_RNDN),
			mpfr_get_float128(z_im, MPFR_RNDN)
		};
		mpfr_add(temp, z_re, z_re, MPFR_RNDN);
		mpfr_fma(z_im, z_im, temp, globals.imag, MPFR_RNDN);
		mpfr_sub(z_re, z2_re, z2_im, MPFR_RNDN);
		mpfr_add(z_re, z_re, globals.real, MPFR_RNDN);
		mpfr_sqr(z2_re, z_re, MPFR_RNDN);
		mpfr_sqr(z2_im, z_im, MPFR_RNDN);
		mpfr_add(temp, z2_re, z2_im, MPFR_RNDN);
		if (mpfr_cmp_d(temp, globals.radius * globals.radius) > 0) {
			globals.perturbation_iters = i + 1;
			break;
		}
	}
	mpfr_clears(z_re, z_im, z2_re, z2_im, temp, (mpfr_ptr)0);
}

MANDELBROT_INLINE static void color(unsigned char& r, unsigned char& g, unsigned char& b, unsigned i, const Complex z) {
	// Color a point depending on its iteration value and escape coordinate.
	// There are two ingredients to do so smoothly:
	//    - Increase escape radius 
	//    - Turn the discrete iteration value into a continuous one 
	// We index the color palette with the smoothed iteration count with 
	// linear interpolation.
	const double palette[] = {
		255,	0,	 	0,
		0,		0,		0,
		255,	255,	0,
		255,	255,	255,
		0,		255,	0,
		0,		0,		0,
		0,		255,	255,
		255,	255,	255,
		0, 		0,		255,
		0,		0,		0,
		255,	0,		255,
		255,	255,	255,
	};
	const size_t count = sizeof(palette) / sizeof(double) / 3;
	double smooth = i + 2.0 - std::log2(std::log(z.re * z.re + z.im * z.im));
	smooth /= 40.0;
	const size_t integer = (size_t)smooth;
	const double lerp = smooth - integer;
	const size_t lookup0 = 3 * (integer % count), lookup1 = 3 * ((integer + 1) % count);
	r = palette[lookup0 + 0] + (palette[lookup1 + 0] - palette[lookup0 + 0]) * lerp;
	g = palette[lookup0 + 1] + (palette[lookup1 + 1] - palette[lookup0 + 1]) * lerp;
	b = palette[lookup0 + 2] + (palette[lookup1 + 2] - palette[lookup0 + 2]) * lerp;
}

void mandelbrot(const MandelbrotGlobals& globals) {
	// Run on many threads as Mandelbrot set rendering is extremely parallel 
	#pragma omp parallel num_threads(64)
	{
		// Allocate multiprecision values 
		mpfr_t c_re, c_im, z_re, z_im, z2_re, z2_im, temp;
		mpfr_inits2(globals.precision, c_re, c_im, z_re, z_im, z2_re, z2_im, temp, (mpfr_ptr)0);
	
		// Loop through each pixel of the image and apply the Mandelbrot set formula 
		#pragma omp for
		for (unsigned p = 0; p != globals.width * globals.height; ++p) {
			// Calculate the delta with full precision 
			mpfr_mul_d(c_re, globals.multiplier, (p % globals.width) - (globals.width * 0.5) + 0.5, MPFR_RNDN);
			mpfr_mul_d(c_im, globals.multiplier, -((p / globals.width) - (globals.height * 0.5) + 0.5), MPFR_RNDN);

			// Round it off to normal precision 
			Complex dc{
				mpfr_get_float128(c_re, MPFR_RNDN),
				mpfr_get_float128(c_im, MPFR_RNDN)
			}, dz{0, 0}, z{0, 0};

			// Perform all iterations 
			unsigned iteration = 0, ref_iteration = 0;
			while (iteration < globals.iterations) {
				const Complex ref = globals.perturbation[ref_iteration];
				dz *= dz + ref + ref;
				dz += dc;
				++ref_iteration;

				z = globals.perturbation[ref_iteration] + dz;
				if (Real sqrlen = z.norm(); sqrlen > globals.radius * globals.radius)
					goto explode;
				else if (sqrlen < dz.norm() || ref_iteration >= globals.perturbation_iters) {
					dz = z;
					ref_iteration = 0;
				}
				++iteration;
			}

			// If the point does "not explode", that is, in the Mandelbrot set,
			// color it black 
			noexplode: {
				globals.pixels[3 * p + 0] = 0x00;
				globals.pixels[3 * p + 1] = 0x00;
				globals.pixels[3 * p + 2] = 0x00;
				continue;
			}

			// If the point does "explode", that is, it is not in the Mandelbrot set,
			// color it dependent on the "color" function 
			explode: {
				unsigned char r, g, b;
				color(r, g, b, iteration, z);
				globals.pixels[3 * p + 0] = r;
				globals.pixels[3 * p + 1] = g;
				globals.pixels[3 * p + 2] = b;
			}
		}

		// Free cache and all multiprecision variables 
		mpfr_clears(c_re, c_im, z_re, z_im, z2_re, z2_im, temp, (mpfr_ptr)0);
		mpfr_free_cache();
	}
}