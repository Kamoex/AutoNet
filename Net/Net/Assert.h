#pragma once
#include "stdio.h"
#include <assert.h>

namespace AutoNet
{
#define ASSERT(expr) if(!(expr)) {assert(NULL);}
#define ASSERTV(expr) if(!(expr)) {assert(NULL); return;}
#define ASSERTN(expr, N) if(!(expr)) {assert(NULL); return (N);}

#define ASSERTOP(expr, OP) if(!(expr)) {assert(NULL); OP;}
#define ASSERTVOP(expr, OP) if(!(expr)) {assert(NULL); OP; return;}
#define ASSERTNOP(expr, N, OP) if(!(expr)) {assert(NULL); OP; return (N);}

#define ASSERTLOG(expr, fmt, ...) if(!(expr)) {printf(fmt, ##__VA_ARGS__); printf("\n"); assert(NULL);}
#define ASSERTVLOG(expr, fmt, ...) if(!(expr)) {printf(fmt, ##__VA_ARGS__); printf("\n"); assert(NULL); return;}
#define ASSERTNLOG(expr, N, fmt, ...) if(!(expr)) {printf(fmt, ##__VA_ARGS__); printf("\n"); assert(NULL); return (N);}

#define LOGERROR(fmt, ...) printf(fmt, ##__VA_ARGS__); printf("\n");
}