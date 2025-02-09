/**
 *    ========== Mandelbrot Fractal Renderer ==========
 * A command-line utility for rendering the Mandelbrot set.
 * 
 * Author: bambamboo15
 */
#pragma once
#include "./datatypes.hpp"
#include <string>

/*
	Causes a "fatal error" that exits the program 
	immediately.
*/
template <typename... Args>
void fatal_error(const char* msg, Args&&... args) {
	printf("\033[38;2;255;100;100merror:\033[0m ");
	printf(msg, args...);
	printf("\n");
	exit(-1);
}

/*
	Render the Mandelbrot set to an image.
*/
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
);

/*
	Render the Mandelbrot set to a video.
*/
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
);