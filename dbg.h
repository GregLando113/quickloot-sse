#pragma once

#define BLD_DEBUG 1

#ifdef BLD_DEBUG
#define D_MSG(msg,...) _MESSAGE("[DBG] %s(%d): " ## msg,__FILE__, __LINE__, __VA_ARGS__)
#define D_VAR(var, fmt) D_MSG(#var " = " fmt, var)
#else
#define D_MSG(msg,...)
#define D_VAR(var, fmt)
#endif