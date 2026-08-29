#include "pch.h"

#include <appmodel.h>
#include <detours.h>

HRESULT WINAPI MockGetCurrentPackageFullName(UINT32*, PWSTR)
{
    return APPMODEL_ERROR_NO_PACKAGE;
}

typedef HRESULT(WINAPI* GetCurrentPackageFullNameFunc)(UINT32*, PWSTR);

void SetAsUnpackaged()
{
    GetCurrentPackageFullNameFunc* trueGetCurrentPackageFullName = static_cast<GetCurrentPackageFullNameFunc*>(DetourFindFunction("Kernel32.dll", "GetCurrentPackageFullName"));

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)trueGetCurrentPackageFullName, MockGetCurrentPackageFullName);
    DetourTransactionCommit();
}