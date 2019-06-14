// display.h: Define the interface to the display system. The display system 
// implementation is platform-dependent, but outside of the platform-dependent
// module all the rest of the system needs to know about is the framebuffer.
#ifndef __RTC_DISPLAY_H__
#define __RTC_DISPLAY_H__

#pragma once

#ifndef RTC_NO_INCLUDES
#   include "rtc.h"
#endif

typedef struct RTC_FRAMEBUFFER {    /* Describes a 32-bit RGBA framebuffer. */
    uint8_t               *Base;    /* The address of the first pixel in the first row of the upper-left corner of the frame buffer. */
    uint32_t              Width;    /* The width of the framebuffer in physical pixels. */
    uint32_t             Height;    /* The height of the framebuffer in physical pixels. */
    uint32_t             Stride;    /* The number of bytes between rows in the framebuffer. */
} RTC_FRAMEBUFFER;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @summary Retrieve a description of the current framebuffer.
 * @param o_framebuffer Pointer to an RTC_FRAMEBUFFER structure to populate.
 * @return Zero if the framebuffer description was retrieved, or non-zero if an error occurred.
 */
RTC_API(int32_t)
rtcGetFrameBuffer
(
    struct RTC_FRAMEBUFFER *o_framebuffer
);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif /* __RTC_DISPLAY_H__ */
