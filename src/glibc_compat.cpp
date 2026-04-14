// glibc_compat.cpp
// When libc++.a is compiled on glibc 2.38+ (Ubuntu 24.04+), its objects contain
// unversioned undefined references to __isoc23_strtol, __isoc23_wcstol, etc. —
// C23 function names that glibc 2.38+ headers redirect strtol/wcstol/sscanf to.
// On older hosts (Ubuntu 22.04 / glibc 2.35) those symbols don't exist, so the
// plugin fails to load with "version `GLIBC_2.38' not found".
//
// Fix: provide local definitions for all __isoc23_* functions.  Plain (unversioned)
// definitions satisfy libc++.a's unversioned references, taking priority over
// glibc's versioned __isoc23_strtol@@GLIBC_2.38 dynamic export.  No GLIBC_2.38
// entry is created in the final .so's dynamic symbol table.
//
// Note: system headers must be included BEFORE the __GLIBC_MINOR__ check because
// __GLIBC__ / __GLIBC_MINOR__ are header macros (from <features.h>), not compiler
// predefined macros — they are not available until a glibc header is included.

#include <cstdarg>
#include <cwchar>
#include <locale.h> // pulls in <features.h> → defines __GLIBC__ / __GLIBC_MINOR__
#include <stdlib.h>
#include <wchar.h>
#include <stdio.h>

// Only needed when building on glibc 2.38+; compiles to nothing on older build hosts
// or non-Linux platforms.
#if defined(__linux__) && defined(__GLIBC__) && \
    (__GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 38))

// Forward-declare the pre-C23 (C17) function names, bypassing any header macros
// that would redirect them back to __isoc23_* on this glibc 2.38 build host.
extern "C"
{
    extern long __c17_strtol(const char *, char **, int) __asm__("strtol");
    extern unsigned long __c17_strtoul(const char *, char **, int) __asm__("strtoul");
    extern long long __c17_strtoll(const char *, char **, int) __asm__("strtoll");
    extern unsigned long long __c17_strtoull(const char *, char **, int) __asm__("strtoull");
    extern long long __c17_strtoll_l(const char *, char **, int, locale_t) __asm__("strtoll_l");
    extern unsigned long long __c17_strtoull_l(const char *, char **, int, locale_t) __asm__("strtoull_l");
    extern long __c17_wcstol(const wchar_t *, wchar_t **, int) __asm__("wcstol");
    extern unsigned long __c17_wcstoul(const wchar_t *, wchar_t **, int) __asm__("wcstoul");
    extern long long __c17_wcstoll(const wchar_t *, wchar_t **, int) __asm__("wcstoll");
    extern unsigned long long __c17_wcstoull(const wchar_t *, wchar_t **, int) __asm__("wcstoull");
    extern int __c17_vsscanf(const char *, const char *, va_list) __asm__("vsscanf");
}

// Provide unversioned definitions for every __isoc23_* symbol pulled in by libc++.a.
// Being unversioned they satisfy libc++.a's unversioned undefs locally, so the
// linker never adds a GLIBC_2.38 dynamic requirement.
// visibility("hidden") keeps them out of the .so's exported symbol table.
#define ISOC23(ret, name, params, args, base)        \
    extern "C" __attribute__((visibility("hidden"))) \
    ret name params { return base args; }

ISOC23(long, __isoc23_strtol, (const char *s, char **e, int b), (s, e, b), __c17_strtol)
ISOC23(unsigned long, __isoc23_strtoul, (const char *s, char **e, int b), (s, e, b), __c17_strtoul)
ISOC23(long long, __isoc23_strtoll, (const char *s, char **e, int b), (s, e, b), __c17_strtoll)
ISOC23(unsigned long long, __isoc23_strtoull, (const char *s, char **e, int b), (s, e, b), __c17_strtoull)
ISOC23(long long, __isoc23_strtoll_l, (const char *s, char **e, int b, locale_t l), (s, e, b, l), __c17_strtoll_l)
ISOC23(unsigned long long, __isoc23_strtoull_l, (const char *s, char **e, int b, locale_t l), (s, e, b, l), __c17_strtoull_l)
ISOC23(long, __isoc23_wcstol, (const wchar_t *s, wchar_t **e, int b), (s, e, b), __c17_wcstol)
ISOC23(unsigned long, __isoc23_wcstoul, (const wchar_t *s, wchar_t **e, int b), (s, e, b), __c17_wcstoul)
ISOC23(long long, __isoc23_wcstoll, (const wchar_t *s, wchar_t **e, int b), (s, e, b), __c17_wcstoll)
ISOC23(unsigned long long, __isoc23_wcstoull, (const wchar_t *s, wchar_t **e, int b), (s, e, b), __c17_wcstoull)
ISOC23(int, __isoc23_vsscanf, (const char *s, const char *f, va_list ap), (s, f, ap), __c17_vsscanf)

// Variadic — handle separately.
extern "C" __attribute__((visibility("hidden"))) int __isoc23_sscanf(const char *s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = __c17_vsscanf(s, fmt, ap);
    va_end(ap);
    return r;
}

#endif // linux + glibc 2.38+
