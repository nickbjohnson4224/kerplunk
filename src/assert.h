#ifndef KERPLUNK_ASSERT_H_
#define KERPLUNK_ASSERT_H_

#include <assert.h>

#ifdef NDEBUG
#undef assert
#define assert(x) if (x) __builtin_unreachable()
#endif

#endif//KERPLUNK_ASSERT_H_
