#pragma once

template <typename T>
inline void escape(const T& value) {
    volatile T v = value;  // Prevent optimization
    (void)v;
}

inline void clobber() {
    asm volatile("" : : : "memory");
}
