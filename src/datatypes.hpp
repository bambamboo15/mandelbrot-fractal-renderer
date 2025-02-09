/**
 *    ========== Mandelbrot Fractal Zoomer ==========
 * Render Mandelbrot fractal zooms like the ones you see on 
 * YouTube! Just give rendering parameters to the program,
 * and it will start rendering your Mandelbrot movie!
 */
#pragma once
#include <cstdint>
#include <cmath>
#define MPFR_WANT_FLOAT128
#include <mpfr.h>

/*
	Real number datatype. For higher-precision uses, use __float80, and 
	for even more intensive uses, use __float128, and for the most extreme 
	cases (where you should use a designated tool like KF), it should be 
	possible to replace with some __float_exp class.

	This is not supposed to be a multiprecision datatype, and it should only 
	be able to represent multiprecision deltas.

	For now, only the `double` datatype is supported, and converting it to 
	another floating-point type may lead to precision issues.
*/
#if defined(__INTELLISENSE__)
using Real = double;
#else
using Real = double;
#endif

/*
	Complex number datatype, which depends on Real. Does not implement 
	division nor reciprocal.
*/
struct Complex {
	Real re;
	Real im;

	constexpr Complex() : re{0}, im{0} {}
	constexpr Complex(Real _re) : re{_re}, im{0} {}
	constexpr Complex(Real _re, Real _im) : re{_re}, im{_im} {}

	constexpr Complex& operator+=(const Complex& other) {
		re += other.re;
		im += other.im;
		return *this;
	}

	constexpr Complex& operator-=(const Complex& other) {
		re -= other.re;
		im -= other.im;
		return *this;
	}

	constexpr Complex& operator*=(const Complex& other) {
		Real old = re;
		re = re * other.re - im * other.im;
		im = old * other.im + im * other.re;
		return *this;
	}

	constexpr Complex& operator*=(const Real scalar) {
		re *= scalar;
		im *= scalar;
		return *this;
	}

	friend constexpr Complex operator+(Complex a, const Complex& b) {
		return a += b;
	}

	friend constexpr Complex operator-(Complex a, const Complex& b) {
		return a -= b;
	}

	friend constexpr Complex operator*(Complex a, const Complex& b) {
		return a *= b;
	}

	friend constexpr Complex operator*(Complex a, const Real b) {
		return a *= b;
	}

	friend constexpr Complex operator*(const Real a, Complex b) {
		return b *= a;
	}

	constexpr Real len() const noexcept {
		return std::hypot(re, im);
	}

	constexpr Real norm() const noexcept {
		return re * re + im * im;
	}
};

// TODO: Abstract mpfr_t in a similar way, providing operator overloads 
// TODO: Support arbitrary datatypes for Real (double only allowed now)