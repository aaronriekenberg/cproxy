#if (!defined(PROXY_DISABLE_EPOLL)) && defined(__linux__)
#include "epoll_pollutil.c"
#elif (!defined(PROXY_DISABLE_KQUEUE)) && defined(__FreeBSD__)
#include "kqueue_pollutil.c"
#else
#include "poll_pollutil.c"
#endif
