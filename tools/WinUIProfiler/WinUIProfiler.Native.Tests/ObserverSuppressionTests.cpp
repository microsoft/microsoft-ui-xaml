#include <Windows.h>
#include <cstdio>
#if __has_include("../../../dxaml/xcp/components/base/inc/WinUIProfilerObserverSuppression.h")
#include "../../../dxaml/xcp/components/base/inc/WinUIProfilerObserverSuppression.h"
#include "../../../dxaml/xcp/components/base/inc/WinUIProfilerTracing.h"
#define HAS_SUPPRESSION 1
#endif
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
#ifdef HAS_SUPPRESSION
static DWORD WINAPI OtherThread(void*)
{
    return WinUIProfiler::ObserverSuppression::IsSuppressed() ? 1 : 0;
}
#endif
int main()
{
#ifndef HAS_SUPPRESSION
    CHECK(false && "shared observer suppression not implemented");
#else
    using namespace WinUIProfiler;
    CHECK(!ObserverSuppression::IsSuppressed());
    SetLastError(1234);
    {
        ObserverSuppression::Scope outer;
        CHECK(ObserverSuppression::IsSuppressed());
        CHECK(GetLastError() == 1234);
        {
            ObserverSuppression::Scope inner;
            CHECK(ObserverSuppression::IsSuppressed());
        }
        CHECK(ObserverSuppression::IsSuppressed());
        CHECK(EnterInstrumentation());
        CHECK(!EnterInstrumentation());
        LeaveInstrumentation();
        CHECK(ObserverSuppression::IsSuppressed());
        HANDLE thread = CreateThread(nullptr, 0, OtherThread, nullptr, 0, nullptr);
        CHECK(thread != nullptr);
        CHECK(WaitForSingleObject(thread, 2000) == WAIT_OBJECT_0);
        DWORD code = 1;
        CHECK(GetExitCodeThread(thread, &code) && code == 0);
        CloseHandle(thread);
    }
    CHECK(!ObserverSuppression::IsSuppressed());
    CHECK(EnterInstrumentation());
    CHECK(ObserverSuppression::IsSuppressed());
    {
        ObserverSuppression::Scope inner;
        CHECK(ObserverSuppression::IsSuppressed());
    }
    LeaveInstrumentation();
    CHECK(!ObserverSuppression::IsSuppressed());
    {
        InstrumentationScope scope;
        CHECK(ObserverSuppression::IsSuppressed());
        CHECK(scope.Detach());
    }
    CHECK(ObserverSuppression::IsSuppressed());
    LeaveInstrumentation();
    CHECK(!ObserverSuppression::IsSuppressed());
    bool factoryCalled = false;
    CHECK(Trace<Event::Allocation>([&]() noexcept { factoryCalled = true; return Allocation{}; }) == S_FALSE);
    CHECK(!factoryCalled && !ObserverSuppression::IsSuppressed());
    SetLastError(4321);
    { ObserverSuppression::Scope scope; }
    CHECK(GetLastError() == 4321);
#endif
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
