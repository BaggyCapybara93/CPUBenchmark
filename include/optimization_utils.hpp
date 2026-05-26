#pragma once

template <typename T>
inline void escape(const T& value) {
    asm volatile("" : : "g"(value) : "memory");
}

inline void clobber() {
    asm volatile("" : : : "memory");
}
