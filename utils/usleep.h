#pragma once

#if defined(_MSC_VER)
void usleep(__int64 usec);
#elif ANDROID || __APPLE__
#include <unistd.h>
#endif // _MSC_VER
