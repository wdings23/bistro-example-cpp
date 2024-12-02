#if defined(_MSC_VER)
#include <windows.h>
#include "usleep.h"
#include <thread>

void usleep(__int64 usec)
{
	std::this_thread::sleep_for(std::chrono::microseconds(usec));

#if 0
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(1 * usec); // Convert to 100 nanosecond interval, negative value indicates relative time

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);
#endif // #if 0
}
#endif // _MSC_VER