#pragma once

#ifdef _WIN32
#  ifdef DATAACCESS_EXPORTS
#    define DAL_API __declspec(dllexport)
#  else
#    define DAL_API __declspec(dllimport)
#  endif
#else
#  define DAL_API
#endif