#include "rtcmath.h"

#ifndef RTC_NO_TESTS
#include "rtctest.h"
#endif

#ifndef RTC_NO_TESTS

RTC_TEST_API(int32_t)
tuple_with_w_eq_1_is_point
(
    void
)
{
    tuple4_t a = tuple4(4.3f, -4.2f, 3.1f, 1.0f);
    RTC_TEST_ASSERT_EQL_FLT(a.pnt.x,  4.3f);
    RTC_TEST_ASSERT_EQL_FLT(a.pnt.y, -4.2f);
    RTC_TEST_ASSERT_EQL_FLT(a.pnt.z,  3.1f);
    RTC_TEST_ASSERT_EQL_FLT(a.pnt.w,  1.0f);
    RTC_TEST_ASSERT_EQL(is_point(a) , true);
    RTC_TEST_ASSERT_EQL(is_vector(a), false);
    return 0;
}

RTC_TEST_API(int32_t)
tuple_with_w_eq_0_is_vector
(
    void
)
{
    tuple4_t a = tuple4(4.3f, -4.2f, 3.1f, 0.0f);
    RTC_TEST_ASSERT_EQL_FLT(a.vec.x,  4.3f);
    RTC_TEST_ASSERT_EQL_FLT(a.vec.y, -4.2f);
    RTC_TEST_ASSERT_EQL_FLT(a.vec.z,  3.1f);
    RTC_TEST_ASSERT_EQL_FLT(a.vec.w,  0.0f);
    RTC_TEST_ASSERT_EQL(is_point(a) , false);
    RTC_TEST_ASSERT_EQL(is_vector(a), true);
    return 0;
}

#endif /* !defined(RTC_NO_TESTS) */

RTC_API(int32_t)
rtcMathTest
(
    void
)
{
    int32_t result = 0;
    RTC_TEST(tuple_with_w_eq_1_is_point);
    RTC_TEST(tuple_with_w_eq_0_is_vector);
    return result;
}
