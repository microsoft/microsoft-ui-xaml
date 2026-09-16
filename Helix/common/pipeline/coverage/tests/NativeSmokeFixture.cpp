// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// Builds the tiny DLL probes and loader used by the real native coverage smoke test.

#ifdef COVERAGE_TEST_DLL
extern "C" __declspec(dllexport) __declspec(noinline) int CoverageProbe(int value)
{
    if (value > 0)
    {
        return value + 7;
    }
    return 7;
}
#else
#include <windows.h>

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2)
    {
        return 2;
    }
    const int value = argv[1][0] == L'1' ? 1 : 0;
    const wchar_t* modules[] = { L"Microsoft.ui.xaml.dll", L"Microsoft.UI.Xaml.Controls.dll" };
    for (const auto name : modules)
    {
        const auto module = LoadLibraryW(name);
        if (!module)
        {
            return 3;
        }
        const auto probe = reinterpret_cast<int (*)(int)>(GetProcAddress(module, "CoverageProbe"));
        const int result = probe ? probe(value) : -1;
        FreeLibrary(module);
        if (result != value + 7)
        {
            return 4;
        }
    }
    return 0;
}
#endif
