#pragma once
#ifndef __BITFUNC_H_
#define __BITFUNC_H_
template<typename T>
constexpr void SET_BIT(T& x,int idx) {
	x |= 1u << idx;
}
template<typename T>
constexpr int GET_BIT(T x, int idx) {
	return (x >> idx) & 1u;
}

template<typename T>
constexpr int get_bit_count(T x) {
	int s = 0;
	while (x) {
		s += x & 1;
		x >>= 1;
	}
	return s;
}
#endif
