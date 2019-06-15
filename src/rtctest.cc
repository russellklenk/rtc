#ifndef RTC_NO_TESTS
#   include "rtctest.h"
#   include "rtcmath.h"
#endif

#ifndef RTC_TEST_MODULE
#define RTC_TEST_MODULE(_func, ...)                                            \
    {                                                                          \
        int32_t _test_result = _func(__VA_ARGS__);                             \
        if (_test_result != 0) {                                               \
            OutputDebugStringA("TEST MODULE FAILURE: " #_func "\n");           \
            result = _test_result;                                             \
        }                                                                      \
    }
#endif

RTC_API(int32_t)
rtcRunUnitTests
(
    void
)
{
    int32_t result = 0;
#   ifndef RTC_NO_TESTS
    RTC_TEST_MODULE(rtcMathTest);
#   endif
    return result;
}
