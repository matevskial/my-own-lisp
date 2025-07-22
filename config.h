#pragma once

#if defined(_WIN32)
#define _WINDOWS
#endif

#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
#define _UNIX_STYLE_OS
#endif

#if defined(_UNIX_STYLE_OS) && !defined(READ_LINE_STDIN_WITH_STDIO)
#define _UNIX_STYLE_OS_EAD_LINE_STDIN_WITH_EDITLINE
#endif
