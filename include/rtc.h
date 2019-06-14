#ifndef __RTC_RTC_H__
#define __RTC_RTC_H__

#pragma once

#ifndef RTC_NO_INCLUDES
#   include <stddef.h>
#   include <stdint.h>
#endif

#ifndef RTC_API
#   define RTC_API(_return_type)                                               \
        extern _return_type
#endif

#endif /* __RTC_RTC_H__ */
