#ifndef INCLUDE_PRIORITIZEDGOALSPLANNER_EXPORTSYMBOLS_MACRO_HPP
#define INCLUDE_PRIORITIZEDGOALSPLANNER_EXPORTSYMBOLS_MACRO_HPP

#if defined _WIN32 || defined __CYGWIN__
#  define PRIORITIZEDGOALSPLANNER_LIB_API_EXPORTS(LIBRARY_NAME) __declspec(dllexport)
#  define PRIORITIZEDGOALSPLANNER_LIB_API(LIBRARY_NAME)         __declspec(dllimport)
#elif __GNUC__ >= 4
#  define PRIORITIZEDGOALSPLANNER_LIB_API_EXPORTS(LIBRARY_NAME) __attribute__ ((visibility("default")))
#  define PRIORITIZEDGOALSPLANNER_LIB_API(LIBRARY_NAME)        __attribute__ ((visibility("default")))
#else
#  define PRIORITIZEDGOALSPLANNER_LIB_API_EXPORTS(LIBRARY_NAME)
#  define PRIORITIZEDGOALSPLANNER_LIB_API(LIBRARY_NAME)
#endif

#endif // INCLUDE_PRIORITIZEDGOALSPLANNER_EXPORTSYMBOLS_MACRO_HPP
