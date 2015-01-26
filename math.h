#ifndef MATH_H
#define MATH_H


using std::abs;
using std::min;
using std::max;

struct v2 {
    f32 x, y;

    f32 &operator[](u32 i) { return (&x)[i]; }
};

inline v2 operator-(v2 a) { return v2 { -a.x, -a.y }; }

inline v2 operator+(v2 a, v2 b) { return v2 { a.x + b.x, a.y + b.y }; }
inline v2 operator-(v2 a, v2 b) { return v2 { a.x - b.x, a.y - b.y }; }

inline v2 &operator+=(v2 &a, v2 b) { a.x += b.x; a.y += b.y; return a; }
inline v2 &operator-=(v2 &a, v2 b) { a.x -= b.x; a.y -= b.y; return a; }

inline v2 operator*(v2 a, f32 s) { return v2 { a.x * s, a.y * s}; }
inline v2 operator*(f32 s, v2 a) { return v2 { a.x * s, a.y * s}; }

inline f32 dot(v2 a, v2 b) { return a.x * b.x + a.y * b.y; }
inline f32 sqrlen(v2 a) { return dot(a, a); }
inline f32 len(v2 a) { return sqrtf(sqrlen(a)); }

#endif
