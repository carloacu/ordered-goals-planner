#ifndef INCLUDE_ORDEREDGOALSPLANNER_EXPORTSYMBOLS_MACRO_HPP
#define INCLUDE_ORDEREDGOALSPLANNER_EXPORTSYMBOLS_MACRO_HPP

#if defined _WIN32 || defined __CYGWIN__
#  define ORDEREDGOALSPLANNER_LIB_API_EXPORTS(LIBRARY_NAME) __declspec(dllexport)
#  define ORDEREDGOALSPLANNER_LIB_API(LIBRARY_NAME)         __declspec(dllimport)
#elif __GNUC__ >= 4
#  define ORDEREDGOALSPLANNER_LIB_API_EXPORTS(LIBRARY_NAME) __attribute__ ((visibility("default")))
#  define ORDEREDGOALSPLANNER_LIB_API(LIBRARY_NAME)        __attribute__ ((visibility("default")))
#else
#  define ORDEREDGOALSPLANNER_LIB_API_EXPORTS(LIBRARY_NAME)
#  define ORDEREDGOALSPLANNER_LIB_API(LIBRARY_NAME)
#endif

#endif // INCLUDE_ORDEREDGOALSPLANNER_EXPORTSYMBOLS_MACRO_HPP
