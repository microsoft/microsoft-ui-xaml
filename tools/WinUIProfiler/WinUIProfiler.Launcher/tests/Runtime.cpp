#include <windows.h>

extern "C" __declspec(dllexport) int ProfilerBuildId()
{
    return PROFILER_BUILD_ID;
}
