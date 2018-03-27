#include "..\ScriptHookV.h"

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(X)                                      \
do {												   \
	if (!(X))                                          \
	    LOG_DEBUG("ASSERT: %s %s::%s:%d",              \
				#X,                                    \
				__FUNCTION__,                          \
				__FILE__,                              \
				__LINE__);                             \
} while (0)
	        
#define TASSERT(X) \
	if (!(X)) throw new std::exception();

#define RASSERT(X, R)                                  \
	if (!(X))                                          \
		return LOG_DEBUG("RASSERT: %s %s::%s:%d",      \
				#X,                                    \
				__FUNCTION__,                          \
				__FILE__,                              \
				__LINE__                              \
		), (R)

#define DASSERT(X, XFMT, ...)                          \
	if (!(X))                                          \
		LOG_DEBUG("DASSERT: %s %s::%s:%d - " #XFMT,    \
				#X,                                    \
				__FUNCTION__,                          \
				__FILE__,                              \
				__LINE__,                              \
				##__VA_ARGS__                          \
		)      

#define QASSERT(X, R)                                  \
	if (!(X))                                          \
		return (R)
