// rtctest.h: Define a very simple unit testing framework. Test failures are 
// written to stderr (and should also be picked up by any attached debugger).
// Define preprocessor symbol RTC_NO_TESTS to skip running unit tests.
#ifndef __RTC_RTCTEST_H__
#define __RTC_RTCTEST_H__

#pragma once

#ifndef RTC_NO_INCLUDES
#   define  WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include "rtc.h"
#   undef   WIN32_LEAN_AND_MEAN
#endif

/* @summary Mark a function as being a unit test.
 * @param _return_type The return type of the function, usually int32_t.
 */
#ifndef RTC_TEST_API
#define RTC_TEST_API(_return_type)                                             \
    static _return_type
#endif

/* @summary Execute a single unit test routine.
 * The parent frame must define an int32_t value named 'result'.
 * @param _func The unit test routine to execute.
 * @param ... A variable-length list of arguments to pass to the unit test routine.
 */
#ifndef RTC_TEST
#define RTC_TEST(_func, ...)                                                   \
    {                                                                          \
        int32_t _test_result = _func(__VA_ARGS__);                             \
        if (_test_result != 0) {                                               \
            OutputDebugStringA("TEST FAILURE: " #_func "\n");                  \
            result = _test_result;                                             \
        }                                                                      \
    }
#endif

/* @summary Assert that two values are equal.
 * @param _lval A value.
 * @param _rval A value.
 */
#ifndef RTC_TEST_ASSERT_EQL
#define RTC_TEST_ASSERT_EQL(_lval, _rval)                                      \
    {                                                                          \
        if ((_lval) != (_rval)) {                                              \
            OutputDebugStringA("ASSERT_EQL FAILED: " #_lval " != " #_rval "\n");\
            return -1;                                                         \
        }                                                                      \
    }
#endif

/* @summary Assert that two values are not equal.
 * @param _lval A value.
 * @param _rval A value.
 */
#ifndef RTC_TEST_ASSERT_NEQ
#define RTC_TEST_ASSERT_NEQ(_lval, _rval)                                      \
    {                                                                          \
        if ((_lval) != (_rval)) {                                              \
            OutputDebugStringA("ASSERT_NEQ FAILED: " #_lval " == " #_rval "\n");\
            return -1;                                                         \
        }                                                                      \
    }
#endif

/* @summary Assert that two floating point values are equal.
 * @param _lval A floating point value.
 * @param _rval A floating point value.
 */
#ifndef RTC_TEST_ASSERT_EQL_FLT
#define RTC_TEST_ASSERT_EQL_FLT(_lval, _rval)                                  \
    {                                                                          \
        if (fleq((_lval), (_rval)) == false) {                                 \
            OutputDebugStringA("ASSERT_EQL FAILED: " #_lval " != " #_rval "\n");\
            return -1;                                                         \
        }                                                                      \
    }
#endif

/* @summary Assert that two floating point values are not equal.
 * @param _lval A floating point value.
 * @param _rval A floating point value.
 */
#ifndef RTC_TEST_ASSERT_NEQ_FLT
#define RTC_TEST_ASSERT_NEQ_FLT(_lval, _rval)                                  \
    {                                                                          \
        if (fleq((_lval), (_rval)) == true) {                                  \
            OutputDebugStringA("ASSERT_NEQ FAILED: " #_lval " == " #_rval "\n");\
            return -1;                                                         \
        }                                                                      \
    }
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* @summary Execute all defined unit tests.
 * Define preprocessor symbol RTC_NO_TESTS to skip running all unit tests.
 * @return Zero if all unit tests execute successfully, or non-zero if one or more tests failed.
 */
RTC_API(int32_t)
rtcRunUnitTests
(
    void
);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* __RTC_RTCTEST_H__ */
