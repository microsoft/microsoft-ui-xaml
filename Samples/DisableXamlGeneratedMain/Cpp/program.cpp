#include "pch.h"

// Forward declare wXamlGeneratedMain since it is generated/defined elsewhere
extern int __stdcall wXamlGeneratedMain();

std::wstring g_launchMarker;

int __stdcall wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
    g_launchMarker = L"CustomMain";

    // Delegate to generated main method
    return wXamlGeneratedMain();
}