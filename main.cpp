/**
 *    ========== Mandelbrot Fractal Renderer ==========
 * A command-line utility for rendering the Mandelbrot set.
 * 
 * Author: bambamboo15
 */
#define CXXOPTS_NO_REGEX
#include "src/cxxopts.hpp"
#include "src/base.hpp"
#include <windows.h>

/*
	Parse command-line options to produce a Mandelbrot render.
			[exe] [format] [output file] [options...]
*/
int main(int argc, char** argv) {
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	cxxopts::Options options("Mandelbrot Fractal Zoomer", "Program to render the Mandelbrot set");
	options.add_options()
		("format", "How the Mandelbrot set should be rendered", cxxopts::value<std::string>())
		("output", "Output file/folder of the render", cxxopts::value<std::string>())
		("w,width", "Width of render in pixels", cxxopts::value<unsigned>())
		("h,height", "Height of render in pixels", cxxopts::value<unsigned>())
		("i,iters", "Iteration count", cxxopts::value<unsigned>())
		("x,real", "Position on real axis", cxxopts::value<std::string>())
		("y,imag", "Position on imaginary axis", cxxopts::value<std::string>())
		("z,zoom", "Magnification", cxxopts::value<std::string>())
		("p,prec", "Precision (in bits) of multiprecision variables", cxxopts::value<unsigned>())
		("Z,ezoom", "Ending magnification", cxxopts::value<std::string>())
		("f,frames", "Number of frames", cxxopts::value<unsigned>())
		("F,framerate", "Framerate", cxxopts::value<unsigned>())
		("l,no-log", "Disable logging");
	options.parse_positional({"format", "output"});
	auto user = options.parse(argc, argv);

	std::string format = user["format"].as<std::string>();
	if (format != "image" && format != "video")
		fatal_error("Unrecognized format '%s', supported formats are ['image', 'video']", format.c_str());
	
	std::string output = user["output"].as<std::string>();
	unsigned width = user.count("width") != 0 ? user["width"].as<unsigned>() : 1920;
	unsigned height = user.count("height") != 0 ? user["height"].as<unsigned>() : 1080;
	unsigned iters = user.count("iters") != 0 ? user["iters"].as<unsigned>() : 1000;
	std::string real = user.count("real") != 0 ? user["real"].as<std::string>() : "-0.75";
	std::string imag = user.count("imag") != 0 ? user["imag"].as<std::string>() : "0.0";
	std::string zoom = user.count("zoom") != 0 ? user["zoom"].as<std::string>() : "1.0";
	unsigned prec = user.count("prec") != 0 ? user["prec"].as<unsigned>() : 200;
	bool log = user.count("no-log") == 0;
	
	if (format == "image")
		mandelbrot_image(
			output, log,
			width, height, iters, real.c_str(), imag.c_str(), zoom.c_str(), prec 
		);
	else if (format == "video") {
		if (user.count("ezoom") == 0)
			fatal_error("Format 'video' requires parameter '--ezoom' or '-Z' but it is missing");
		std::string ezoom = user["ezoom"].as<std::string>();
		if (user.count("frames") == 0)
			fatal_error("Format 'video' requires parameter '--frames' or '-f' but it is missing");
		unsigned frames = user["frames"].as<unsigned>();
		if (user.count("framerate") == 0)
			fatal_error("Format 'video' requires parameter '--framerate' or '-F' but it is missing");
		unsigned framerate = user["framerate"].as<unsigned>();

		mandelbrot_video(
			output, log,
			width, height, iters, real.c_str(), imag.c_str(), zoom.c_str(), prec,
			ezoom.c_str(), frames, framerate 
		);
	}
}