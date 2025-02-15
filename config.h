#pragma once

#if defined(_WIN32)
#define _WINDOWS
#endif

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#define _UNIX_STYLE_OS
#endif
