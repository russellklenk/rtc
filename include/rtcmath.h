// rtcmath.h: Define data types and functions for working with tuples (points 
// vectors and colors), matrices and scalars. Light operator overloading is 
// used to make some code easier to read, but operations such as dot and 
// cross products are provided as normal functions.
#ifndef __RTC_RTCMATH_H__
#define __RTC_RTCMATH_H__

#pragma once

#ifndef RTC_NO_INCLUDES
#   include <cmath>
#   include "rtc.h"
#endif

/* @summary Define the epsilon value used for comparing single-precision floating point values for equality.
 */
#ifndef RTC_EPSILON
#   define RTC_EPSILON    0.0001f
#endif

typedef union alignas(16) tuple4_t {    // A tuple of four single-precision floating point values used to represent points, colors and vectors.
    struct color {                      // Access the components as an RGBA color value.
        float r;                        // The red channel of the color.
        float g;                        // The green channel of the color.
        float b;                        // The blue channel of the color.
        float a;                        // The alpha channel of the color.
    } clr;

    struct point {                      // Access the components as a point value (w = 1).
        float x;                        // The x-axis coordinate of the point.
        float y;                        // The y-axis coordinate of the point.
        float z;                        // The z-axis coordinate of the point.
        float w;                        // The homogenous w component of the point (=1).
    } pnt;

    struct vector {                     // Access the components as a vector value (w = 0).
        float x;                        // The x-axis component of the vector.
        float y;                        // The y-axis component of the vector.
        float z;                        // The z-axis component of the vector.
        float w;                        // The homogenous w component of the vector (=0).
    } vec;

    float arr[4];                     // Access the components as an array.
} tuple4_t;

/* @summary Return the maximum of two single-precision floating point values.
 * Implemented here to avoid pulling in <algorithm>.
 * @param a A single-precision floating point value.
 * @param b A single-precision floating point value.
 * @return The larger of the two values a and b.
 */
static inline float
fmax2
(
    float a, 
    float b
)
{
    return (a > b) ? a : b;
}

/* @summary Return the maximum of three single-precision floating point values.
 * Implemented here to avoid pulling in <algorithm>.
 * @param a A single-precision floating point value.
 * @param b A single-precision floating point value.
 * @param c A single-prevision floating point value.
 * @return The larger of the three values a, b and c.
 */
static inline float
fmax3
(
    float a, 
    float b, 
    float c
)
{
    float   max_ab = a > b ? a : b;
    float   max_bc = b > c ? b : c;
    return (max_ab > max_bc) ? max_ab : max_bc;
}

/* @summary Compare to floating-point values for equality, using RTC_EPSILON as both the relative and absolute tolerance.
 * See http://realtimecollisiondetection.net/blog/?p=89
 * @param a A single-precision floating point value.
 * @param b A single-precision floating point value.
 * @return true if values a and b can be considered equal.
 */
static inline bool 
fleq
(
    float a, 
    float b
)
{
    return (fabsf(a - b) <= RTC_EPSILON * fmax3(1.0f, fabsf(a), fabsf(b)));
}

/* @summary Compare two floating-point values for equality, specifying both an absolute and a relative tolerance.
 * See http://realtimecollisiondetection.net/blog/?p=89
 * @param a A single-precision floating point value.
 * @param b A single-precision floating point value.
 * @param abs_tolerance An absolute tolerance value, used when a and b become small.
 * @param rel_tolerance A relative tolerance value, used when a and b become large.
 * @return true if values a and b can be considered equal.
 */
static inline bool
fleq
(
    float a, 
    float b, 
    float abs_tolerance, 
    float rel_tolerance
)
{
    return (fabsf(a - b) <= fmax2(abs_tolerance, rel_tolerance * fmax2(fabsf(a), fabsf(b))));
}

/* @summary Initialize a tuple from individual components.
 * @param a The first component of the tuple.
 * @param b The second component of the tuple.
 * @param c The third component of the tuple.
 * @param d The fourth component of the tuple.
 * @return A new tuple initialized with the given components.
 */
static inline tuple4_t
tuple4
(
    float a, 
    float b, 
    float c, 
    float d
)
{
    return tuple4_t{a, b, c, d};
}

/* @summary Initialize a tuple representing an RGB color value.
 * The alpha channel component is set to 1.0f.
 * @param r The red channel value, typically in [0, 1.0f].
 * @param g The green channel value, typically in [0, 1.0f].
 * @param b The blue channel value, typically in [0, 1.0f].
 * @return A new tuple value representing the color.
 */
static inline tuple4_t
color3
(
    float r, 
    float g, 
    float b
)
{
    return tuple4_t{r, g, b, 1.0f};
}

/* @summary Initialize a tuple representing an RGBA color value.
 * @param r The red channel value, typically in [0, 1.0f].
 * @param g The green channel value, typically in [0, 1.0f].
 * @param b The blue channel value, typically in [0, 1.0f].
 * @param a The alpha channel value, in [0, 1.0f].
 * @return A new tuple value representing the color.
 */
static inline tuple4_t
color4
(
    float r, 
    float g, 
    float b, 
    float a
)
{
    return tuple4_t{r, g, b, a};
}

/* @summary Initialize a tuple representing a 3-dimensional point.
 * @param x The x-axis coordinate of the point.
 * @param y The y-axis coordinate of the point.
 * @param z The z-axis coordinate of the point.
 * @return A new tuple representing the point.
 */
static inline tuple4_t
point
(
    float x, 
    float y, 
    float z
)
{
    return tuple4_t{x, y, z, 1.0f};
}

/* @summary Initialize a tuple representing a 3-dimensional vector.
 * @param x The x-axis component of the vector.
 * @param y The y-axis component of the vector.
 * @param z The z-axis component of the vector.
 * @return A new tuple representing the vector.
 */
static inline tuple4_t
vec3
(
    float x, 
    float y, 
    float z
)
{
    return tuple4_t{x, y, z, 0.0f};
}

/* @summary Initialize a tuple representing a 3-dimensional vector.
 * @param x The x-axis component of the vector.
 * @param y The y-axis component of the vector.
 * @param z The z-axis component of the vector.
 * @param w The homogenous w component of the vector.
 * @return A new tuple representing the vector.
 */
static inline tuple4_t
vec4
(
    float x, 
    float y, 
    float z, 
    float w
)
{
    return tuple4_t{x, y, z, w};
}

/* @summary Determine if a tuple represents a point.
 * @param t The tuple to inspect.
 * @return true if the tuple appears to represent a point (w = 1.0f).
 */
static inline bool
is_point
(
    tuple4_t const &t
)
{
    return fleq(t.arr[3], 1.0f);
}

/* @summary Determine if a tuple represents a vector.
 * @param t The tuple to inspect.
 * @return true if the tuple appears to represent a vector (w = 0.0f).
 */
static inline bool
is_vector
(
    tuple4_t const &t
)
{
    return fleq(t.arr[3], 0.0f);
}

/* @summary Compare two tuples for equality.
 * @param a A tuple value.
 * @param b A tuple value.
 * @return true if the tuples are considered equal.
 */
static inline bool
operator ==
(
    tuple4_t const &a, 
    tuple4_t const &b
)
{
    return fleq(a.arr[0], b.arr[0]) && fleq(a.arr[1], b.arr[1]) && 
           fleq(a.arr[2], b.arr[2]) && fleq(a.arr[3], b.arr[3]);
}

/* @summary Compare two tuples for inequality.
 * @param a A tuple value.
 * @param b A tuple value.
 * @return true if the tuples are not considered equal.
 */
static inline bool
operator !=
(
    tuple4_t const &a, 
    tuple4_t const &b
)
{
    return !fleq(a.arr[0], b.arr[0]) || !fleq(a.arr[1], b.arr[1]) || 
           !fleq(a.arr[2], b.arr[2]) || !fleq(a.arr[3], b.arr[3]);
}

/* @summary Implement unary negation for a tuple.
 * @param t A tuple value.
 * @return The negated tuple.
 */
static inline tuple4_t
operator -
(
    tuple4_t const &t
)
{
    return tuple4_t{-t.arr[0], -t.arr[1], -t.arr[2], -t.arr[3]};
}

/* @summary Implement tuple addition.
 * @param a A tuple value.
 * @param b A tuple value.
 * @return The resulting tuple.
 */
static inline tuple4_t
operator +
(
    tuple4_t const &a, 
    tuple4_t const &b
)
{
    return tuple4_t {
        a.arr[0] + b.arr[0], 
        a.arr[1] + b.arr[1], 
        a.arr[2] + b.arr[2], 
        a.arr[3] + b.arr[3]
    };
}

/* @summary Implement tuple subtraction.
 * @param a A tuple value.
 * @param b A tuple value.
 * @return The resulting tuple.
 */
static inline tuple4_t
operator - 
(
    tuple4_t const &a, 
    tuple4_t const &b
)
{
    return tuple4_t {
        a.arr[0] - b.arr[0], 
        a.arr[1] - b.arr[1], 
        a.arr[2] - b.arr[2], 
        a.arr[3] - b.arr[3]
    };
}

/* @summary Implement tuple multiplication by a scalar.
 * @param t A tuple value.
 * @param s The scale factor.
 * @return The resulting tuple.
 */
static inline tuple4_t
operator * 
(
    tuple4_t const &t, 
    float           s
)
{
    return tuple4_t{t.arr[0] * s, t.arr[1] * s, t.arr[2] * s, t.arr[3] * s};
}

/* @summary Implement tuple multiplication by a scalar.
 * @param s The scale factor.
 * @param t A tuple value.
 * @return The resulting tuple.
 */
static inline tuple4_t
operator * 
(
    float           s, 
    tuple4_t const &t
)
{
    return tuple4_t{t.arr[0] * s, t.arr[1] * s, t.arr[2] * s, t.arr[3] * s};
}

#ifdef __cplusplus
extern "C" {
#endif

/* @summary Execute unit tests for the rtcmath module.
 * @return Zero if all tests executed successfully, or non-zero if one or more tests failed.
 */
RTC_API(int32_t)
rtcMathTest
(
    void
);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* __RTC_RTCMATH_H__ */
