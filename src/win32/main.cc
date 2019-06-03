// main.cc: Give our application a well-behaved window and backbuffer to render into 
// and handle user input and presentation.

#include <stddef.h>
#include <stdint.h>
#include <Windows.h>
#include <shellapi.h>
#include <ShellScalingApi.h>

static WCHAR const *WSI_WndTitle     = L"The Ray Tracer Challenge";
static WCHAR const *WSI_WndClassName = L"WSI_WndClass_rtc";

/* @summary Convert from physical to logical pixels.
 * @param _dim The value to convert, specified in physical pixels.
 * @param _dpi The DPI value of the display.
 * @return The corresponding dimention specified in logical pixels.
 */
#ifndef PhysicalToLogicalPixels
#define PhysicalToLogicalPixels(_dim, _dpi)                                    \
    (((_dim) * USER_DEFAULT_SCREEN_DPI) / (_dpi))
#endif

/* @summary Convert from logical to physical pixels.
 * @param _dim The value to convert, specified in logical pixels.
 * @param _dpi The DPI value of the display.
 * @return The corresponding dimension specified in physical pixels.
 */
#ifndef LogicalToPhysicalPixels
#define LogicalToPhysicalPixels(_dim, _dpi)                                    \
    (((_dim) * (_dpi)) / USER_DEFAULT_SCREEN_DPI)
#endif

/**
 * Function pointer types.
 * Some Win32 API functions are runtime loaded into the process address space.
 */
typedef HRESULT (WINAPI *PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
typedef HRESULT (WINAPI *PFN_GetDpiForMonitor      )(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);

typedef enum WSI_WINDOW_STATUS_FLAGS {
    WSI_WINDOW_STATUS_FLAGS_NONE           = (0UL <<  0),  /* The window has been destroyed. */
    WSI_WINDOW_STATUS_FLAG_CREATED         = (1UL <<  0),  /* The window has been created. */
    WSI_WINDOW_STATUS_FLAG_ACTIVE          = (1UL <<  1),  /* The window is active and has input focus. */
    WSI_WINDOW_STATUS_FLAG_VISIBLE         = (1UL <<  2),  /* The window is currently visible on some display output. */
    WSI_WINDOW_STATUS_FLAG_FULLSCREEN      = (1UL <<  3),  /* The window is currently in fullscreen mode. */
} WSI_WINDOW_STATUS_FLAGS;

typedef enum WSI_WINDOW_EVENT_FLAGS {
    WSI_WINDOW_EVENT_FLAGS_NONE            = (0UL <<  0),  /* No events were received by the window. */
    WSI_WINDOW_EVENT_FLAG_CREATED          = (1UL <<  0),  /* The window was just created. */
    WSI_WINDOW_EVENT_FLAG_DESTROYED        = (1UL <<  1),  /* The window was just destroyed. */
    WSI_WINDOW_EVENT_FLAG_SHOWN            = (1UL <<  2),  /* The window was just made visible. */
    WSI_WINDOW_EVENT_FLAG_HIDDEN           = (1UL <<  3),  /* The window was just hidden. */
    WSI_WINDOW_EVENT_FLAG_ACTIVATED        = (1UL <<  4),  /* The window just received input focus. */
    WSI_WINDOW_EVENT_FLAG_DEACTIVATED      = (1UL <<  5),  /* The window just lost input focus. */
    WSI_WINDOW_EVENT_FLAG_SIZE_CHANGED     = (1UL <<  6),  /* The window size was just changed. */
    WSI_WINDOW_EVENT_FLAG_POSITION_CHANGED = (1UL <<  7),  /* The window position was just changed. */
} WSI_WINDOW_EVENT_FLAGS;

/* @summary The dispatch table used to call runtime-resolved Windows APIs.
 */
typedef struct WIN32API_DISPATCH {
    PFN_SetProcessDpiAwareness SetProcessDpiAwareness;     /* The SetProcessDpiAwareness entry point. */
    PFN_GetDpiForMonitor       GetDpiForMonitor;           /* The GetDpiForMonitor entry point. */
    HMODULE                    ModuleHandle_Shcore;        /* The module handle for Shcore.dll. */
} WIN32API_DISPATCH;

typedef struct WSI_WINDOW_STATE {
    HWND                       Win32Handle;
    uint32_t                   StatusFlags;                /* One or more bitwise OR'd values of the WSI_WINDOW_STATUS_FLAGS enumeration. */
    uint32_t                   EventFlags;                 /* One or more bitwise OR'd values of the WSI_WINDOW_EVENT_FLAGS enumeration. */
    uint32_t                   OutputDpiX;                 /* The horizontal dots-per-inch setting of the output containing the window. */
    uint32_t                   OutputDpiY;                 /* The vertical dots-per-inch setting of the output containing the window. */
    int32_t                    PositionX;                  /* The x-coordinate of the upper-left corner of the window, in virtual display space. */
    int32_t                    PositionY;                  /* The y-coordinate of the upper-left corner of the window, in virtual display space. */
    uint32_t                   WindowSizeX;                /* The horizontal size of the window, including borders and chrome, in logical pixels. */
    uint32_t                   WindowSizeY;                /* The vertical size of the window, including borders and chrome, in logical pixels. */
    uint32_t                   ClientSizeX;                /* The horizontal size of the window client area, in logical pixels. */
    uint32_t                   ClientSizeY;                /* The vertical size of the window client area, in logical pixels. */
    RECT                       RestoreRect;                /* The RECT describing the window position and size prior to entering fullscreen mode, in physical pixels. */
    DWORD                      RestoreStyle;               /* The window style used for the window prior to entering fullscreen mode. */
    DWORD                      RestoreStyleEx;             /* The extended window style used for the window prior to entering fullscreen mode. */
    uint8_t                   *BackBufferMemory;
    uint32_t                   BackBufferWidth;
    uint32_t                   BackBufferHeight;
    uint32_t                   BackBufferStride;
    BITMAPINFO                 BackBufferInfo;
    WIN32API_DISPATCH          Win32Api;
} WSI_WINDOW_STATE;

static HRESULT WINAPI
SetProcessDpiAwareness_Stub
(
    PROCESS_DPI_AWARENESS level
)
{
    UNREFERENCED_PARAMETER(level);
    if (SetProcessDPIAware()) {
        return S_OK;
    } else {
        return E_ACCESSDENIED;
    }
}

static HRESULT WINAPI
GetDpiForMonitor_Stub
(
    HMONITOR      monitor, 
    MONITOR_DPI_TYPE type, 
    UINT           *dpi_x, 
    UINT           *dpi_y
)
{
    if (type == MDT_EFFECTIVE_DPI) {
        HDC screen_dc = GetDC(NULL);
        DWORD   h_dpi = GetDeviceCaps(screen_dc, LOGPIXELSX);
        DWORD   v_dpi = GetDeviceCaps(screen_dc, LOGPIXELSY);
        ReleaseDC(NULL, screen_dc);
        *dpi_x = h_dpi;
        *dpi_y = v_dpi;
        UNREFERENCED_PARAMETER(monitor);
        return S_OK;
    } else {
        *dpi_x = USER_DEFAULT_SCREEN_DPI;
        *dpi_y = USER_DEFAULT_SCREEN_DPI;
        return E_INVALIDARG;
    }
}

static void
QueryMonitorGeometry
(
    MONITORINFO* o_monitorinfo,
    HMONITOR           monitor
)
{
    POINT pt = {0, 0};
    if (monitor == nullptr) {
        monitor  = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    } o_monitorinfo->cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, o_monitorinfo);
}

static int
ResizeBackBuffer
(
    struct WSI_WINDOW_STATE* window,
    uint32_t          physical_x_px,
    uint32_t          physical_y_px
)
{
    BITMAPINFOHEADER *hdr =&window->BackBufferInfo.bmiHeader;
    uint8_t     *prev_mem = window->BackBufferMemory;
    uint8_t     *bmap_mem = nullptr;
    uint32_t    prev_x_px = window->BackBufferWidth;
    uint32_t    prev_y_px = window->BackBufferHeight;
    size_t      num_bytes =(size_t) physical_x_px * (size_t) physical_y_px * 4;

    if (physical_x_px == prev_x_px && physical_y_px == prev_y_px && prev_mem) {
        // There's no need to resize.
        return 0;
    }
    ZeroMemory(&window->BackBufferInfo, sizeof(BITMAPINFO));
    hdr->biSize       = sizeof(window->BackBufferInfo.bmiHeader);
    hdr->biWidth      = (LONG )physical_x_px;
    hdr->biHeight     =-(LONG )physical_y_px;
    hdr->biPlanes     = 1;
    hdr->biBitCount   = 32;
    hdr->biCompression= BI_RGB;

    if ((bmap_mem = (uint8_t*)VirtualAlloc(nullptr, num_bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) == nullptr) {
        // Keep the prior backbuffer since memory allocation failed.
        return -1;
    }
    window->BackBufferMemory = bmap_mem;
    window->BackBufferWidth  = physical_x_px;
    window->BackBufferHeight = physical_y_px;
    window->BackBufferStride = physical_x_px * 4;
    if (prev_mem != nullptr) {
        VirtualFree(prev_mem, 0, MEM_RELEASE);
    }
    return 0;
}

static void
PresentBackBuffer
(
    struct WSI_WINDOW_STATE* window,
    HDC                          dc
)
{
    int32_t dst_x = 0;
    int32_t dst_y = 0;
    int32_t src_x = 0;
    int32_t src_y = 0;
    int32_t dst_w = LogicalToPhysicalPixels(window->ClientSizeX, window->OutputDpiX);
    int32_t dst_h = LogicalToPhysicalPixels(window->ClientSizeY, window->OutputDpiY);
    int32_t src_w = window->BackBufferWidth;  // Already in physical pixels.
    int32_t src_h = window->BackBufferHeight; // Already in physical pixels.
    StretchDIBits(dc, dst_x, dst_y, dst_w, dst_h, src_x, src_y, src_w, src_h, window->BackBufferMemory, &window->BackBufferInfo, DIB_RGB_COLORS, SRCCOPY);
}

/* @summary Handle the WM_CREATE message.
 * This routine retrieves the properties of the display associated with the window, and resizes the window to account for borders and chrome.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_CREATE
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    WIN32API_DISPATCH      *winapi =&window->Win32Api;
    HMONITOR               monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    UINT                     dpi_x = USER_DEFAULT_SCREEN_DPI;
    UINT                     dpi_y = USER_DEFAULT_SCREEN_DPI;
    DWORD                    style =(DWORD) GetWindowLongPtr(hwnd, GWL_STYLE);
    DWORD                 ex_style =(DWORD) GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    uint32_t             phys_x_px;
    uint32_t             phys_y_px;
    RECT                        rc;

    UNREFERENCED_PARAMETER(wparam);
    UNREFERENCED_PARAMETER(lparam);
    winapi->GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
    phys_x_px = LogicalToPhysicalPixels(window->ClientSizeX, dpi_x);
    phys_y_px = LogicalToPhysicalPixels(window->ClientSizeY, dpi_y);

    rc.left   = rc.top = 0;
    rc.right  =(LONG) phys_x_px;
    rc.bottom =(LONG) phys_y_px;
    AdjustWindowRectEx(&rc, style, FALSE, ex_style);
    SetWindowPos(hwnd, nullptr, 0, 0, rc.right-rc.left, rc.bottom-rc.top, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);

    window->StatusFlags = WSI_WINDOW_STATUS_FLAG_CREATED;
    window->EventFlags  = WSI_WINDOW_EVENT_FLAG_CREATED;
    window->EventFlags |= WSI_WINDOW_EVENT_FLAG_SIZE_CHANGED;
    window->EventFlags |= WSI_WINDOW_EVENT_FLAG_POSITION_CHANGED;
    window->OutputDpiX  = dpi_x;
    window->OutputDpiY  = dpi_y;
    window->WindowSizeX = PhysicalToLogicalPixels((uint32_t)(rc.right-rc.left), dpi_x);
    window->WindowSizeY = PhysicalToLogicalPixels((uint32_t)(rc.bottom-rc.top), dpi_y);
    ResizeBackBuffer(window, phys_x_px, phys_y_px);
    return 0;
}

/* @summary Handle the WM_CLOSE message.
 * This routine hides the window and marks it as destroyed, but does not actually destroy the window.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_CLOSE
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    UNREFERENCED_PARAMETER(wparam);
    UNREFERENCED_PARAMETER(lparam);
    ShowWindow(hwnd, SW_HIDE);
    window->StatusFlags = WSI_WINDOW_STATUS_FLAGS_NONE;
    window->EventFlags  = WSI_WINDOW_EVENT_FLAG_DESTROYED;
    return 0;
}

/* @summary Handle the WM_ACTIVATE message.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_ACTIVATE
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    int    active = LOWORD(wparam);
    int minimized = HIWORD(wparam);

    if (active) {
        window->StatusFlags |=  WSI_WINDOW_STATUS_FLAG_ACTIVE | WSI_WINDOW_STATUS_FLAG_VISIBLE;
        window->EventFlags  |=  WSI_WINDOW_EVENT_FLAG_ACTIVATED;
    } else { 
        window->StatusFlags &= ~WSI_WINDOW_STATUS_FLAG_ACTIVE;
        window->EventFlags  |=  WSI_WINDOW_EVENT_FLAG_DEACTIVATED;
    }
    if (minimized) {
        window->StatusFlags &= ~WSI_WINDOW_STATUS_FLAG_VISIBLE;
        window->EventFlags  |=  WSI_WINDOW_EVENT_FLAG_HIDDEN;
    }
    return DefWindowProc(hwnd, WM_ACTIVATE, wparam, lparam);
}

/* @summary Handle the WM_DPICHANGED message.
 * This routine updates the position and size of the window based on the suggestion made by the operating system.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_DPICHANGED
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    HMONITOR    monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    uint32_t      flags = WSI_WINDOW_EVENT_FLAGS_NONE;
    UINT          dpi_x = LOWORD(wparam);
    UINT          dpi_y = HIWORD(wparam);
    DWORD         style =(DWORD) GetWindowLong(hwnd, GWL_STYLE);
    DWORD      ex_style =(DWORD) GetWindowLong(hwnd, GWL_EXSTYLE);
    uint32_t  phys_x_px = LogicalToPhysicalPixels(window->ClientSizeX, dpi_x);
    uint32_t  phys_y_px = LogicalToPhysicalPixels(window->ClientSizeY, dpi_y);
    RECT     *suggested =(RECT*)(lparam);
    RECT             rc;
    MONITORINFO moninfo;
    
    if ((style & WS_POPUP) == 0) {
        /* resize the window to account for chrome and borders.
         * position the window at the location suggested by the OS.
         */
        if (suggested->left != window->PositionX || suggested->top != window->PositionY) {
            flags |= WSI_WINDOW_EVENT_FLAG_POSITION_CHANGED;
        }
        rc.left    = suggested->left;
        rc.top     = suggested->top;
        rc.right   = suggested->left + phys_x_px;
        rc.bottom  = suggested->top  + phys_y_px;
        AdjustWindowRectEx(&rc, style, FALSE, ex_style);
        SetWindowPos(hwnd, nullptr, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, SWP_NOACTIVATE|SWP_NOZORDER);
    } else { /* fullscreen borderless */
        QueryMonitorGeometry(&moninfo, monitor);
        rc = moninfo.rcMonitor;
    }
    window->OutputDpiX  = dpi_x;
    window->OutputDpiY  = dpi_y;
    window->PositionX   = rc.left;
    window->PositionY   = rc.top;
    window->WindowSizeX = PhysicalToLogicalPixels((uint32_t)(rc.right-rc.left), dpi_x);
    window->WindowSizeY = PhysicalToLogicalPixels((uint32_t)(rc.bottom-rc.top), dpi_y);
    window->EventFlags |= WSI_WINDOW_EVENT_FLAG_SIZE_CHANGED | flags;
    ResizeBackBuffer(window, phys_x_px, phys_y_px);
    return 0;
}

/* @summary Handle the WM_MOVE message.
 * This routine updates the position of the window and detects any DPI changes resulting from moving to a different monitor.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_MOVE
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    WIN32API_DISPATCH *winapi =&window->Win32Api;
    HMONITOR          monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    UINT                dpi_x = USER_DEFAULT_SCREEN_DPI;
    UINT                dpi_y = USER_DEFAULT_SCREEN_DPI;
    RECT                   rc;

    GetWindowRect(hwnd, &rc);
    winapi->GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
    window->EventFlags |= WSI_WINDOW_EVENT_FLAG_POSITION_CHANGED;
    window->PositionX   = rc.left;
    window->PositionY   = rc.top;
    window->WindowSizeX = PhysicalToLogicalPixels((uint32_t)(rc.right-rc.left), dpi_x);
    window->WindowSizeY = PhysicalToLogicalPixels((uint32_t)(rc.bottom-rc.top), dpi_y);
    window->OutputDpiX  = dpi_x;
    window->OutputDpiY  = dpi_y;
    UNREFERENCED_PARAMETER(wparam);
    UNREFERENCED_PARAMETER(lparam);
    return 0;
}

/* @summary Handle the WM_SIZE message.
 * This routine updates the position and size of the window and detects any DPI changes resulting from moving to a different monitor.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_SIZE
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    WIN32API_DISPATCH *winapi =&window->Win32Api;
    HMONITOR          monitor = MonitorFromWindow(hwnd , MONITOR_DEFAULTTONEAREST);
    uint32_t     phy_client_w =(uint32_t)LOWORD(lparam);
    uint32_t     phy_client_h =(uint32_t)HIWORD(lparam);
    UINT                dpi_x = USER_DEFAULT_SCREEN_DPI;
    UINT                dpi_y = USER_DEFAULT_SCREEN_DPI;
    uint32_t            flags = WSI_WINDOW_EVENT_FLAG_SIZE_CHANGED;
    uint32_t           status = window->StatusFlags;
    uint32_t     log_client_w;
    uint32_t     log_client_h;
    BOOL        is_fullscreen;
    BOOL           is_visible;
    BOOL             did_size;
    RECT                   rc;

    /* we'll need to access the swapchain and per-frame resources */
    winapi->GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI , &dpi_x, &dpi_y);
    log_client_w = PhysicalToLogicalPixels(phy_client_w, dpi_x);
    log_client_h = PhysicalToLogicalPixels(phy_client_h, dpi_y);

    /* Determine the visibility status and whether the size changed */
    if (wparam == SIZE_MINIMIZED || wparam == SIZE_MAXHIDE) {
        flags     |= WSI_WINDOW_EVENT_FLAG_HIDDEN;
        status    &=~WSI_WINDOW_STATUS_FLAG_VISIBLE;
        is_visible = FALSE;
    } else if (wparam == SIZE_RESTORED) {
        flags     |= WSI_WINDOW_EVENT_FLAG_SHOWN;
        status    |= WSI_WINDOW_STATUS_FLAG_VISIBLE;
        is_visible = TRUE;
    } else {
        status    |= WSI_WINDOW_STATUS_FLAG_VISIBLE;
        is_visible = TRUE;
    }
    if (log_client_w == window->ClientSizeX && log_client_h == window->ClientSizeY) {
        did_size = FALSE;
    } else {
        did_size = TRUE;
    }
    if (is_visible == FALSE || did_size == FALSE) {
        window->StatusFlags = status;
        return 0;
    }

    if (window->StatusFlags & WSI_WINDOW_STATUS_FLAG_FULLSCREEN) {
        is_fullscreen = TRUE;
    } else {
        is_fullscreen = FALSE;
    }

    /* The window is visible, and the size did change.
     */
    ResizeBackBuffer(window, phy_client_w, phy_client_h);

    if (is_fullscreen) {
        status |= WSI_WINDOW_STATUS_FLAG_FULLSCREEN;
    } else {
        status &=~WSI_WINDOW_STATUS_FLAG_FULLSCREEN;
    }

    GetWindowRect(hwnd, &rc);
    window->StatusFlags = status;
    window->EventFlags |= flags;
    window->PositionX   = rc.left;
    window->PositionY   = rc.top;
    window->WindowSizeX = PhysicalToLogicalPixels((uint32_t)(rc.right-rc.left), dpi_x);
    window->WindowSizeY = PhysicalToLogicalPixels((uint32_t)(rc.bottom-rc.top), dpi_y);
    window->ClientSizeX = log_client_w;
    window->ClientSizeY = log_client_h;
    window->OutputDpiX  = dpi_x;
    window->OutputDpiY  = dpi_y;
    return 0;
}

/* @summary Handle the WM_SHOWWINDOW message.
 * This routine updates the visibility status of the window.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_SHOWWINDOW
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    if (wparam) { /* window is being shown */
        WIN32API_DISPATCH *winapi =&window->Win32Api;
        HMONITOR          monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
        UINT                dpi_x = USER_DEFAULT_SCREEN_DPI;
        UINT                dpi_y = USER_DEFAULT_SCREEN_DPI;
        uint32_t        phys_x_px;
        uint32_t        phys_y_px;
        RECT                   rc;

        GetWindowRect(hwnd, &rc);
        winapi->GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
        window->StatusFlags |= WSI_WINDOW_STATUS_FLAG_VISIBLE;
        window->EventFlags  |= WSI_WINDOW_EVENT_FLAG_SHOWN;
        window->PositionX    = rc.left;
        window->PositionY    = rc.top;
        window->WindowSizeX  = PhysicalToLogicalPixels((uint32_t)(rc.right-rc.left), dpi_x);
        window->WindowSizeY  = PhysicalToLogicalPixels((uint32_t)(rc.bottom-rc.top), dpi_y);
        GetClientRect(hwnd , &rc);
        phys_x_px            =(uint32_t)(rc.right - rc.left);
        phys_y_px            =(uint32_t)(rc.bottom - rc.top);
        window->ClientSizeX  = PhysicalToLogicalPixels((uint32_t)(rc.right-rc.left), dpi_x);
        window->ClientSizeY  = PhysicalToLogicalPixels((uint32_t)(rc.bottom-rc.top), dpi_y);
        window->OutputDpiX   = dpi_x;
        window->OutputDpiY   = dpi_y;
        ResizeBackBuffer(window, phys_x_px, phys_y_px);
    } else { /* window is being hidden */
        window->StatusFlags &=~WSI_WINDOW_STATUS_FLAG_VISIBLE;
        window->StatusFlags &=~WSI_WINDOW_STATUS_FLAG_ACTIVE;
        window->EventFlags  |=(WSI_WINDOW_EVENT_FLAG_HIDDEN | WSI_WINDOW_EVENT_FLAG_DEACTIVATED);
    }
    return DefWindowProc(hwnd, WM_SHOWWINDOW, wparam, lparam);
}

/* @summary Handle the WM_SYSCOMMAND message.
 * This routine processes the Alt+Enter key combination to toggle between windowed and fullscreen.
 * @param window Data associated with the window.
 * @param hwnd The window handle.
 * @param wparam Additional data associated with the message.
 * @param lparam Additional data associated with the message.
 * @return A message-specific result code.
 */
static LRESULT CALLBACK
WSI_WndProc_WM_SYSCOMMAND
(
    struct WSI_WINDOW_STATE *window, 
    HWND                       hwnd, 
    WPARAM                   wparam, 
    LPARAM                   lparam
)
{
    if ((wparam & 0xFFF0) == SC_KEYMENU) {
        if (lparam == VK_RETURN) { /* Alt+Enter */
            WIN32API_DISPATCH *winapi =&window->Win32Api;
            HMONITOR          monitor = MonitorFromWindow(hwnd , MONITOR_DEFAULTTONEAREST);
            UINT                dpi_x = USER_DEFAULT_SCREEN_DPI;
            UINT                dpi_y = USER_DEFAULT_SCREEN_DPI;
            MONITORINFO       moninfo;

            winapi->GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI , &dpi_x, &dpi_y);
            if (window->StatusFlags & WSI_WINDOW_STATUS_FLAG_FULLSCREEN) {
                /* Toggle back to windowed mode */
                RECT     rc = window->RestoreRect;
                SetWindowLong(hwnd, GWL_STYLE  , window->RestoreStyle);
                SetWindowLong(hwnd, GWL_EXSTYLE, window->RestoreStyleEx);
                SetWindowPos (hwnd, HWND_TOP   , rc.left, rc.top, (rc.right-rc.left), (rc.bottom-rc.top), SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
                window->StatusFlags &=~WSI_WINDOW_STATUS_FLAG_FULLSCREEN;
                ShowWindow(hwnd, SW_NORMAL);
            } else {
                /* Toggle to fullscreen mode */
                QueryMonitorGeometry(&moninfo, monitor);
                GetWindowRect(hwnd,  &window->RestoreRect);
                window->RestoreStyle   = (DWORD)GetWindowLong(hwnd, GWL_STYLE);
                window->RestoreStyleEx = (DWORD)GetWindowLong(hwnd, GWL_EXSTYLE);
                RECT     rc = moninfo.rcMonitor;
                SetWindowLong(hwnd, GWL_STYLE, WS_POPUP);
                SetWindowPos (hwnd, HWND_TOP , rc.left, rc.top, (rc.right-rc.left), (rc.bottom-rc.top), SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
                window->StatusFlags |= WSI_WINDOW_STATUS_FLAG_FULLSCREEN;
                ShowWindow(hwnd, SW_MAXIMIZE);
            }
            return 0; /* Handled Alt+Enter */
        }
    }
    return DefWindowProc(hwnd, WM_SYSCOMMAND, wparam, lparam);
}

static LRESULT CALLBACK
WSI_WndProc_WM_PAINT
(
    struct WSI_WINDOW_STATE* window,
    HWND                       hwnd,
    WPARAM                   wparam,
    LPARAM                   lparam
)
{
    PAINTSTRUCT ps;
    HDC         dc;
    
    if (window->BackBufferMemory != nullptr) {
        dc = BeginPaint(window->Win32Handle, &ps);
        PresentBackBuffer(window, dc);
        EndPaint(window->Win32Handle, &ps);
        return 0;
    } else {
        return DefWindowProc(hwnd, WM_PAINT, wparam, lparam);
    }
}

static LRESULT CALLBACK
WSI_WndProc
(
    HWND     hwnd,
    UINT      msg,
    WPARAM wparam,
    LPARAM lparam
)
{
    LRESULT          result = 0;
    WSI_WINDOW_STATE *state = nullptr;

    // WM_NCCREATE performs special handling to store the state data associated with the window.
    // The handler for WM_NCCREATE executes before the call to CreateWindowEx returns.
    if (msg == WM_NCCREATE) {
        CREATESTRUCT     *cs =(CREATESTRUCT    *) lparam;
        WSI_WINDOW_STATE *ws =(WSI_WINDOW_STATE*) cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) ws);
        ws->Win32Handle= hwnd;
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    // WndProc may receive several messages prior to WM_NCCREATE.
    // Send these off to the default handler.
    if ((state = (WSI_WINDOW_STATE*)GetWindowLongPtr(hwnd, GWLP_USERDATA)) == nullptr) {
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }
    // The state pointer is guaranteed to be valid.
    // Perform our own message processing.
    switch (msg) {
        case WM_MOVE:
            { // Update position and detect monitor/DPI changes.
              result = WSI_WndProc_WM_MOVE(state, hwnd, wparam, lparam);
            } break;

        case WM_SIZE:
            { // Update size and detect monitor/DPI changes.
              result = WSI_WndProc_WM_SIZE(state, hwnd, wparam, lparam);
            } break;

        case WM_PAINT:
            { // Present the back buffer contents to the window client area.
              result = WSI_WndProc_WM_PAINT(state, hwnd, wparam, lparam);
            } break;

        case WM_ACTIVATE:
            { // Update the active status so app can save power when inactive.
              result = WSI_WndProc_WM_ACTIVATE(state, hwnd, wparam, lparam);
            } break;

        case WM_SHOWWINDOW:
            { // Update visibility status so app can save power when hidden.
              result = WSI_WndProc_WM_SHOWWINDOW(state, hwnd, wparam, lparam);
            } break;

        case WM_DPICHANGED:
            { // Respond to DPI setting changes.
              result = WSI_WndProc_WM_DPICHANGED(state, hwnd, wparam, lparam);
            } break;

        case WM_SYSCOMMAND:
            { // Handle Alt+Enter to toggle between fullscreen and windowed mode.
              result = WSI_WndProc_WM_SYSCOMMAND(state, hwnd, wparam, lparam);
            } break;

        case WM_CREATE:
            { // Update and resize the window client area if necessary.
              result = WSI_WndProc_WM_CREATE(state, hwnd, wparam, lparam);
            } break;

        case WM_CLOSE:
            { // Close the window and mark it as being destroyed.
              result = WSI_WndProc_WM_CLOSE(state, hwnd, wparam, lparam);
            } break;

        default:
            { // Pass the message off to the default handler.
              result = DefWindowProc(hwnd, msg, wparam, lparam);
            } break;
    } return result;
}

int WINAPI
wWinMain
(
    HINSTANCE curr_instance,
    HINSTANCE prev_instance,
    LPWSTR          cmdline,
    int             showcmd
)
{
    HWND               hwnd = nullptr;
    HMODULE          shcore = nullptr;
    DWORD        error_code = ERROR_SUCCESS;
    DWORD             style = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
    DWORD          style_ex = 0;
    int32_t       virtual_x = 0;
    int32_t       virtual_y = 0;
    uint32_t       dim_x_px = 800; // Width of the window client area, in logical pixels.
    uint32_t       dim_y_px = 600; // Height of the window client area, in logical pixels.
    RECT                 rc;
    MONITORINFO     moninfo;
    WNDCLASSEX     wndclass;
    WSI_WINDOW_STATE     ws;
    WIN32API_DISPATCH win32;
    BOOL                ret;
    MSG                 msg;

    UNREFERENCED_PARAMETER(prev_instance);
    UNREFERENCED_PARAMETER(cmdline);
    UNREFERENCED_PARAMETER(showcmd);

    // Resolve dynamically-loaded Win32 API functions.
    if ((shcore = LoadLibraryW(L"Shcore.dll")) != nullptr) {
        if((win32.GetDpiForMonitor       =(PFN_GetDpiForMonitor      ) GetProcAddress(shcore, "GetDpiForMonitor")) == nullptr) {
            win32.GetDpiForMonitor       = GetDpiForMonitor_Stub;
        }
        if((win32.SetProcessDpiAwareness =(PFN_SetProcessDpiAwareness) GetProcAddress(shcore, "SetProcessDpiAwareness")) == nullptr) {
            win32.SetProcessDpiAwareness = SetProcessDpiAwareness_Stub;
        }
        win32.ModuleHandle_Shcore        = shcore;
    } else {
        // Shcore.dll is not available on the host.
        win32.GetDpiForMonitor           = GetDpiForMonitor_Stub;
        win32.SetProcessDpiAwareness     = SetProcessDpiAwareness_Stub;
        win32.ModuleHandle_Shcore        = nullptr;
    }

    // SetProcessDpiAwareness must be called prior to calling -any- other graphics functions.
    win32.SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);

    if (GetClassInfoEx(curr_instance, WSI_WndClassName, &wndclass) == FALSE) {
        ZeroMemory(&wndclass   , sizeof(WNDCLASSEX));
        wndclass.cbSize        = sizeof(WNDCLASSEX);
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = sizeof(WSI_WINDOW_STATE*);
        wndclass.hInstance     = curr_instance;
        wndclass.lpszClassName = WSI_WndClassName;
        wndclass.lpszMenuName  = nullptr;
        wndclass.lpfnWndProc   = WSI_WndProc;
        wndclass.hIcon         = LoadIcon  (0, IDI_APPLICATION);
        wndclass.hIconSm       = LoadIcon  (0, IDI_APPLICATION);
        wndclass.hCursor       = LoadCursor(0, IDC_ARROW);
        wndclass.style         = CS_HREDRAW | CS_VREDRAW;
        wndclass.hbrBackground =(HBRUSH) GetStockObject(WHITE_BRUSH);
        if (RegisterClassEx(&wndclass) == FALSE) {
            // 
        }
    }
    // See display_window_d3d12.cc for a decent window starting point.
    // See handmade hero day 003 for backbuffer creation.

    // The main window is always created on the primary display, centered 
    // and with chrome. The user can use Alt+Enter to toggle fullscreen 
    // mode, and can drag the window to the desired display.
    QueryMonitorGeometry(&moninfo, nullptr);
    rc        = moninfo.rcWork;
    virtual_x = rc.left + (rc.right - rc.left - dim_x_px) / 2;
    virtual_y = rc.top  + (rc.bottom - rc.top - dim_y_px) / 2;
    if ((int32_t)(virtual_x + dim_x_px) > (int32_t)(rc.right - rc.left)) {
        dim_x_px = (rc.right - rc.left) - virtual_x;
    }
    if ((int32_t)(virtual_y + dim_y_px) > (int32_t)(rc.bottom - rc.top)) {
        dim_y_px = (rc.bottom - rc.top) - virtual_y;
    }
    ZeroMemory(&ws, sizeof(WSI_WINDOW_STATE));
    ws.StatusFlags    = WSI_WINDOW_STATUS_FLAGS_NONE;
    ws.EventFlags     = WSI_WINDOW_EVENT_FLAGS_NONE;
    ws.OutputDpiX     = USER_DEFAULT_SCREEN_DPI;
    ws.OutputDpiY     = USER_DEFAULT_SCREEN_DPI;
    ws.PositionX      = virtual_x;
    ws.PositionY      = virtual_y;
    ws.WindowSizeX    = dim_x_px;
    ws.WindowSizeY    = dim_y_px;
    ws.ClientSizeX    = dim_x_px;
    ws.ClientSizeY    = dim_y_px;
    ws.RestoreStyle   = style;
    ws.RestoreStyleEx = style_ex;
    ws.Win32Api       = win32;
    if ((hwnd = CreateWindowEx(style_ex, WSI_WndClassName, WSI_WndTitle, style, virtual_x, virtual_y, dim_x_px, dim_y_px, nullptr, nullptr, curr_instance, (LPVOID)& ws)) == nullptr) {
        // Window creation failed.
        return (int) GetLastError();
    } ShowWindow(hwnd, SW_SHOW);

    // Update the main window at a set interval.
    for ( ; ; ) {
        // Run the main message dispatch loop.
        while ((ret = PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) != 0) {
            if (ret < 0 || msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (ws.EventFlags & WSI_WINDOW_EVENT_FLAG_DESTROYED) {
            break;
        }
        if (ws.StatusFlags & WSI_WINDOW_STATUS_FLAG_VISIBLE) {
            if (ws.BackBufferMemory != nullptr) {
                HDC dc = GetDC(ws.Win32Handle);
                PresentBackBuffer(&ws, dc);
                ReleaseDC(ws.Win32Handle, dc);
            }
        }
        Sleep(16);
    }

    // Destroy the main window.
    if (DestroyWindow(hwnd) != FALSE) {
        while ((ret = GetMessage(&msg, hwnd, 0, 0)) != 0) {
            if (ret == -1 || msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}
