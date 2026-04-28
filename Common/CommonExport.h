#pragma once

#ifdef _MSC_VER
#pragma warning(disable: 4251)
#endif

#ifdef _WIN32
#  ifdef COMMON_EXPORTS
#    define COMMON_API __declspec(dllexport)
#  else
#    define COMMON_API __declspec(dllimport)
#  endif
#else
#  define COMMON_API
#endif