#pragma once

#ifdef _WIN32
#  ifdef COMMON_EXPORTS
#    define COMMON_API __declspec(dllexport)
#  else
#    define COMMON_API __declspec(dllimport)
#  endif
#else
#  define COMMON_API
#endif