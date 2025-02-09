/**
 *    ========== Mandelbrot Fractal Zoomer ==========
 * Render Mandelbrot fractal zooms like the ones you see on 
 * YouTube! Just give rendering parameters to the program,
 * and it will start rendering your Mandelbrot movie!
 */
#include "./mandelbrot.hpp"
#include "./base.hpp"
#include <limits>
#include <sstream>
#include <chrono>
#include <cstring>
#include <cmath>
#include <omp.h>
#include <immintrin.h>

void mandelbrot_image(
	std::string output,
	bool log,
	unsigned width,
	unsigned height,
	unsigned iterations,
	const char* real,
	const char* imag,
	const char* zoom,
	unsigned prec 
) {
	// We make a pipe to use with the ffmpeg command-line utility,
	// and we pipe to it in binary mode.
	std::stringstream formed;
	formed << "ffmpeg -f rawvideo -pix_fmt argb -s " << width << "x" << height << " -c:v ppm -i - "
		   << output << " -s " << width << "x" << height << " -update true -y > NUL 2>&1";
	auto pipe = _popen(formed.str().c_str(), "wb");

	// If Windows failed to open the pipe, report that error 
	if (pipe == NULL)
		fatal_error("Failed opening pipe (Windows error)\n");

	// Initialize the MandelbrotGlobals 
	MandelbrotGlobals globals;
	unsigned char* pixels = new unsigned char[width * height * 3];
	mandelbrot_start(globals, pixels, width, height, iterations, real, imag, zoom, prec, zoom);

	// Generate the Mandelbrot image and time it 
	auto start = std::chrono::high_resolution_clock::now();
	mandelbrot(globals);
	auto end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = end - start;

	// Log the data 
	printf("\033[2J\033[HTime taken for image to render: %.4fs\n", elapsed);

	// Relay all of the frame data to ffmpeg 
	fprintf(pipe, "P6 %d %d 255 ", width, height);
	fwrite(pixels, 1, width * height * 3, pipe);

	// Close the pipe 
	_pclose(pipe);
}

MANDELBROT_INLINE static void calculate_frame_part_1_xy(
	const double s,
	const double dx,
	const double dy,
	const unsigned x,
	const unsigned y,
	const unsigned width,
	const unsigned height,
	double* frame,
	const unsigned char* keyframe0 
) {
	const unsigned keyframe_index = 3 * (x + y * width * 2);
	const double px = s * (x + dx), py = s * (y + dy);
	const int ix0 = (int)px, ix1 = (int)(px + s),
				iy0 = (int)py, iy1 = (int)(py + s);
	
	#define CHECK(X, Y) if (X >= 0 && Y >= 0 && X < width && Y < height)
	#define PERFORM(X, Y)													\
		const double a = ax * ay;											\
		const unsigned frame_index = 3 * (X + Y * width);					\
		frame[frame_index + 0] += keyframe0[keyframe_index + 0] * a;		\
		frame[frame_index + 1] += keyframe0[keyframe_index + 1] * a;		\
		frame[frame_index + 2] += keyframe0[keyframe_index + 2] * a;

	if (ix0 != ix1) {
		if (iy0 != iy1) {
			CHECK(ix0, iy0) {
				const double ax = ix1 - px,
								ay = iy1 - py;
				PERFORM(ix0, iy0)
			}
			CHECK(ix1, iy0) {
				const double ax = px + s - ix1,
								ay = iy1 - py;
				PERFORM(ix1, iy0)
			}
			CHECK(ix1, iy1) {
				const double ax = px + s - ix1,
								ay = py + s - iy1;
				PERFORM(ix1, iy1)
			}
		} else {
			CHECK(ix0, iy0) {
				const double ax = ix1 - px,
								ay = s;
				PERFORM(ix0, iy0)
			}
			CHECK(ix1, iy0) {
				const double ax = px + s - ix1,
								ay = s;
				PERFORM(ix1, iy0)
			}
		}
	} else {
		CHECK(ix0, iy0) {
			const double ax = s,
							ay = (iy0 == iy1) ? s : (iy1 - py);
			PERFORM(ix0, iy0)
		}
	}
	if (iy0 != iy1) {
		CHECK(ix0, iy1) {
			const double ax = (ix0 == ix1) ? s : (ix1 - px),
							ay = py + s - iy1;
			PERFORM(ix0, iy1)
		}
	}
}

MANDELBROT_INLINE static void calculate_frame_part_2_xy(
	const double s,
	const double t,
	const unsigned x,
	const unsigned y,
	const unsigned width,
	const unsigned height,
	double* frame,
	const unsigned char* keyframe1 
) {
	const double px = s * ((int)x - (int)width) + (width / 2.0),
					py = s * ((int)y - (int)height) + (height / 2.0);
	const int ix0 = (int)px, ix1 = (int)(px + s),
				iy0 = (int)py, iy1 = (int)(py + s);
	const unsigned keyframe_index = 3 * (x + y * width * 2);
	const double ax0 = (ix0 == ix1) ? s : (px + s - ix1), ax1 = ix1 - px,
				ay0 = (iy0 == iy1) ? s : (py + s - iy1), ay1 = iy1 - py;
	for (int ix = ix0; ix <= ix1; ++ix) {
		for (int iy = iy0; iy <= iy1; ++iy) {
			const double ax = (ix == ix1) ? ax0 : ax1,
						ay = (iy == iy1) ? ay0 : ay1;
			const double a = ax * ay;
			const unsigned frame_index = 3 * (ix + iy * width);
			frame[frame_index + 0] += (double)keyframe1[keyframe_index + 0] * a * t;
			frame[frame_index + 1] += (double)keyframe1[keyframe_index + 1] * a * t;
			frame[frame_index + 2] += (double)keyframe1[keyframe_index + 2] * a * t;
		}
	}
}

MANDELBROT_INLINE static void calculate_frame(
	const double Z0,
	const unsigned width,
	const unsigned height,
	double* frame,
	const unsigned char* keyframe0,
	const unsigned char* keyframe1 
) {
	// Set the entire frame to black 
	if constexpr (std::numeric_limits<double>::is_iec559)
		memset(frame, 0, width * height * 3 * sizeof(double));
	else 
		for (unsigned i = 0; i != width * height * 3; ++i)
			frame[i] = 0.0;
	
	// PART 1: Zoom into the current keyframe 
	{
		const double s = 0.5 / Z0;
		const double dx = (Z0 - 1.0) * width, dy = (Z0 - 1.0) * height;
		const unsigned xlo = width * (1.0 - Z0), xhi = width * (1.0 + Z0),
					   ylo = height * (1.0 - Z0), yhi = height * (1.0 + Z0);
		const int num_threads = 16;

		#pragma omp parallel num_threads(num_threads)
		{
			const int thread_num = omp_get_thread_num();

			for (unsigned y = ylo + ((yhi - ylo) * thread_num) / num_threads; y < ylo + ((yhi - ylo) * (thread_num + 1)) / num_threads - 3; ++y)
				for (unsigned x = xlo; x < xhi; ++x)
					calculate_frame_part_1_xy(s, dx, dy, x, y, width, height, frame, keyframe0);
		}

		for (unsigned d = 1; d < num_threads; ++d)
			for (unsigned y = ylo + ((yhi - ylo) * d) / num_threads - 3; y < ylo + ((yhi - ylo) * d) / num_threads; ++y)
				for (unsigned x = xlo; x < xhi; ++x)
					calculate_frame_part_1_xy(s, dx, dy, x, y, width, height, frame, keyframe0);
	}

	// PART 2: Blend the next keyframe with a portion of the current image 
	{
		const double t = 2.0 - 2.0 * Z0;
		const double s = 0.25 / Z0;
		unsigned xlo, xhi, ylo, yhi;
		int ixlo = 0, ixhi = 0, iylo = 0, iyhi = 0;
		double ixlo_d = 0.0, ixhi_d = 0.0, iylo_d = 0.0, iyhi_d = 0.0;
		for (xlo = 0; xlo != width * 2; ++xlo)
			if ((ixlo = (ixlo_d = s * ((int)xlo - (int)width) + (width / 2.0))) >= 0)
				break;
		for (xhi = width * 2 - 1; xhi != 0; --xhi)
			if ((ixhi = (ixhi_d = s * ((int)xhi - (int)width + 1) + (width / 2.0))) < width)
				break;
		for (ylo = 0; ylo != height * 2; ++ylo)
			if ((iylo = (iylo_d = s * ((int)ylo - (int)height) + (height / 2.0))) >= 0)
				break;
		for (yhi = height * 2 - 1; yhi != 0; --yhi)
			if ((iyhi = (iyhi_d = s * ((int)yhi - (int)height + 1) + (height / 2.0))) < height)
				break;
		for (int iy = iylo; iy <= iyhi; ++iy) {
			for (int ix = ixlo; ix <= ixhi; ++ix) {
				const unsigned frame_index = 3 * (ix + iy * width);
				double m = 1.0;
				if (ix == ixlo)
					m *= ixlo + 1.0 - ixlo_d;
				else if (ix == ixhi)
					m *= ixhi_d - ixhi;
				if (iy == iylo)
					m *= iylo + 1.0 - iylo_d;
				else if (iy == iyhi)
					m *= iyhi_d - iyhi;
				
				m = 1.0 - m * t;
				frame[frame_index + 0] *= m;
				frame[frame_index + 1] *= m;
				frame[frame_index + 2] *= m;
			}
		}

		{
			const int num_threads = 16;

			#pragma omp parallel num_threads(num_threads)
			{
				const int thread_num = omp_get_thread_num();

				for (unsigned y = ylo + ((yhi - ylo) * thread_num) / num_threads; y < ylo + ((yhi - ylo) * (thread_num + 1)) / num_threads - 3; ++y)
					for (unsigned x = xlo; x <= xhi; ++x)
						calculate_frame_part_2_xy(s, t, x, y, width, height, frame, keyframe1);
			}

			for (unsigned d = 1; d <= num_threads; ++d)
				for (unsigned y = ylo + ((yhi - ylo) * d) / num_threads - 3; y < ylo + ((yhi - ylo) * d) / num_threads; ++y)
					for (unsigned x = xlo; x <= xhi; ++x)
						calculate_frame_part_2_xy(s, t, x, y, width, height, frame, keyframe1);
			for (unsigned x = xlo; x <= xhi; ++x)
				calculate_frame_part_2_xy(s, t, x, yhi, width, height, frame, keyframe1);
		}
	}
}

// The below passage is about optimizations for rendering Mandelbrot zooms.
//
// Now, rendering Mandelbrot fractals can take time. However, there are 
// two rendering-based methods that can be done:
//
// [PIXEL REUSE]
//   This method reuses pixels from one frame to another, causing 
//   massive speedups every frame.
//
// [RENDERING KEYFRAMES]
//   This renders the Mandelbrot zoom with keyframes, which makes 
//   the cumulative time spent depend more on the final magnification 
//   instead of frames needed to generate. In this case,
//   a 2x higher-resolution image will be generated, then all of the 
//   frames up until it gets to 2x more magnification will just be 
//   downscales of that one.
//
// The optimization implemented right now is rendering keyframes. We 
// continuously double the magnification until it exceeds the actual 
// final magnification. We double the magnification when the current 
// frame exceeds double the current magnification.
//
// NOTE: Magnification is inverse multiplier.
//
// NOTE: 0 < Z0 ≤ 1 is the multiplier of a frame divided by the multiplier 
//       of its corresponding keyframe.
//
// To calculate the image from the keyframe image with respect to Z0,
// we have to crop the center of the keyframe image to Z0 and scale to 
// the frame resolution.
//
// For each keyframe pixel, we calculate the resulting floating pixel 
// after the abforementioned translation, and check which frame pixels 
// it intersects. For each of those pixels, we blend the keyframe pixel 
// color with it multiplied by how much the keyframe pixel intersects 
// with that pixel.
//
// Since pixels will only strictly get smaller, we do not assume that a 
// transformed scaled keyframe pixel does not intersect more than two 
// frame pixels.
//
// NOTE: We do use OpenMP to parallelize this image processing.
void mandelbrot_video(
	std::string output,
	bool log,
	unsigned width,
	unsigned height,
	unsigned iterations,
	const char* real,
	const char* imag,
	const char* zoom,
	unsigned prec,
	const char* ezoom,
	unsigned frames,
	unsigned framerate 
) {
	// We do the same thing as the image function, but use a different 
	// ffmpeg command.
	std::stringstream formed;
	formed << "ffmpeg -f image2pipe -framerate " << framerate << " -c:v ppm -i - "
		   << "-c:v libx264 -crf 18 -vf \"scale=" << width << ":" << height << ",format=yuv420p\" "
		   << "-movflags +faststart " << output << " -y > NUL 2>&1";
	auto pipe = _popen(formed.str().c_str(), "wb");

	// If Windows failed to open the pipe, report that error 
	if (pipe == NULL)
		fatal_error("Failed opening pipe (Windows error)\n");

	// Initialize the MandelbrotGlobals (and both keyframe buffers)
	MandelbrotGlobals globals;
	unsigned char* keyframe0 = new unsigned char[width * height * 12];
	unsigned char* keyframe1 = new unsigned char[width * height * 12];
	mandelbrot_start(globals, keyframe1, width * 2, height * 2, iterations, real, imag, zoom, prec, ezoom);

	// Form a normal-resolution copy of pixels for normal frame generation 
	__attribute__((aligned(16))) double* frame_raw = new double[width * height * 3];
	__attribute__((aligned(16))) unsigned char* frame = new unsigned char[width * height * 3];

	// Initialize the first keyframe buffer and time it 
	{
		auto start = std::chrono::high_resolution_clock::now();

		mandelbrot(globals);

		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration = end - start;
		printf("\033[2J\033[HKeyframe 1 done rendering! %.3fs\n", duration.count());
	}

	// Temporary multiprecision variables 
	mpfr_t temp0, temp1;
	mpfr_inits2(globals.precision, temp0, temp1, 0);

	// Generate keyframes and zoom into them until half multiplier is reached,
	// then generate yet another one 
	unsigned frameno = 0, keyframeno = 1, oldframeno = 0;
	while (frameno < frames) {
		// Render keyframe image and time it 
		auto start0 = std::chrono::high_resolution_clock::now();
		{
			memcpy(keyframe0, keyframe1, width * height * 12);
			mpfr_set(temp0, globals.multiplier, MPFR_RNDN);
			mpfr_set(globals.multiplier, globals.half_keyframe_multiplier, MPFR_RNDN);
			mandelbrot(globals);
			mpfr_set(globals.multiplier, temp0, MPFR_RNDN);
		}
		auto end0 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> duration0 = end0 - start0;
		printf("\033[2J\033[HKeyframe %d done rendering! %.3fs\n", ++keyframeno, duration0.count());

		// Generate frames and time them 
		auto start1 = std::chrono::high_resolution_clock::now();
		{
			// While the frame can be scaled down from the keyframe 
			for (oldframeno = frameno; mpfr_cmp(globals.multiplier, globals.half_keyframe_multiplier) > 0 && frameno < frames; ++frameno) {
				// Calculate the zoom-in amount, 0 < Z0 ≤ 1, with respect to the keyframe 
				mpfr_div(temp0, globals.multiplier, globals.keyframe_multiplier, MPFR_RNDN);
				const double Z0 = mpfr_get_d(temp0, MPFR_RNDN);

				// Run frame calculations 
				calculate_frame(Z0, width, height, frame_raw, keyframe0, keyframe1);

				// Adjust multiplier 
				const double ratio = (double)(frameno + 1) / (frames - 1);
				mpfr_div(temp0, globals.end_multiplier, globals.start_multiplier, MPFR_RNDN);
				mpfr_set_d(temp1, ratio, MPFR_RNDN);
				mpfr_pow(temp0, temp0, temp1, MPFR_RNDN);
				mpfr_mul(globals.multiplier, temp0, globals.start_multiplier, MPFR_RNDN);

				// Transfer all precise pixel data to the frame buffer 
				unsigned i = 0;
				for (; i < width * height * 3; i += 16) {
					__m128i xmm0 = _mm256_cvtpd_epi32(_mm256_load_pd(frame_raw + (i + 0)));
					__m128i xmm1 = _mm256_cvtpd_epi32(_mm256_load_pd(frame_raw + (i + 4)));
					__m128i xmm2 = _mm256_cvtpd_epi32(_mm256_load_pd(frame_raw + (i + 8)));
					__m128i xmm3 = _mm256_cvtpd_epi32(_mm256_load_pd(frame_raw + (i + 12)));
					__m128i xmm01 = _mm_packus_epi32(xmm0, xmm1);
					__m128i xmm23 = _mm_packus_epi32(xmm2, xmm3);
					__m128i xmm0123 = _mm_packus_epi16(xmm01, xmm23);
					_mm_store_si128((__m128i*)(frame + i), xmm0123);
				}
				for (i -= 16; i < width * height * 3; ++i)
					frame[i] = (unsigned char)frame_raw[i];
				
				// Relay all of the absorbed pixel data to ffmpeg 
				fprintf(pipe, "P6 %d %d 255 ", width, height);
				fwrite(frame, 1, width * height * 3, pipe);
			}

			// Adjust keyframe multipliers 
			mpfr_mul_d(globals.keyframe_multiplier, globals.keyframe_multiplier, 0.5, MPFR_RNDN);
			mpfr_mul_d(globals.half_keyframe_multiplier, globals.half_keyframe_multiplier, 0.5, MPFR_RNDN);
		}
		auto end1 = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double, std::ratio<1, 1>> duration1 = end1 - start1;
		printf("Frames %d-%d done rendering! %.3fs\n", oldframeno, frameno, duration1.count());
	}

	// Close the pipe 
	_pclose(pipe);
}