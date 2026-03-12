#pragma once

#include <UtilClass.hpp>

template<Floating T> T TAGUtil::randomFloat() {
	static std::uniform_real_distribution<T> rngf((T)0, (T)1);
	return rngf(gen);
}

template<Integral T> T TAGUtil::randomRangeInt(const T& min, const T& max) {
	static std::uniform_int_distribution<T> rngi(min, max);
	return rngi(gen);
}

template<Floating T> T TAGUtil::randomRangeFloat(const T& min, const T& max) {
	static std::uniform_real_distribution<T> rngf(min, max);
	return rngf(gen);
}