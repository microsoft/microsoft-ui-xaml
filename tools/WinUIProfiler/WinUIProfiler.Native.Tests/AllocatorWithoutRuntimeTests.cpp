// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include <Windows.h>
#include <cstdio>
#include "WinUIProfilerAllocatorRuntime.h"

int main()
{
    using namespace WinUIProfiler::AllocatorObservation;
    int checks = 0, failures = 0;
    auto check = [&](bool value)
    {
        ++checks;
        if (!value) { ++failures; std::printf("FAIL check %d\n", checks); }
    };
    // This executable deliberately does not link the XAML runtime bridge.
    void* address = ObservedHeapAllocate(GetProcessHeap(), 0, 32);
    check(address != nullptr);
    if (address)
    {
        static_cast<unsigned char*>(address)[0] = 42;
        void* resized = ObservedHeapResize(GetProcessHeap(), 0, address, 64);
        check(resized != nullptr);
        if (resized) address = resized;
        check(static_cast<unsigned char*>(address)[0] == 42);
        check(ObservedHeapResize(GetProcessHeap(), 0, address, SIZE_MAX) == nullptr);
        check(static_cast<unsigned char*>(address)[0] == 42);
        check(ObservedHeapFree(GetProcessHeap(), 0, address) != FALSE);
    }
    check(!WinUIProfiler::ObserverSuppression::IsSuppressed());
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
