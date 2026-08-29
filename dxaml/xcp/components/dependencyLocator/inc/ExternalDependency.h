// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// How to use ExternalDependency:
// 1) Use a DependencyProvider<T> as usual in the source Dll
// 2) Add the following to your Dll file:
//  #include <DependencyLocator.h>
//  extern "C" __declspec(dllexport) DependencyLocator::Internal::LocalDependencyStorage* __cdecl GetDependencyLocatorStorage()
//  {
//      return &(DependencyLocator::Internal::GetDependencyLocatorStorage().Get());
//  }
// 3) In the CPP file of the target Dll of the place you wish to consume your dependency declare a static instance of a ExternalDependency:
//    ExternalDependency<MyInterstingType> s_myInterestingType(L"Name Of the Dll that contains MyInterstingType.dll");
//    ...
//    s_myInterestingType->DoSomethingAwesome(); // The dll will be loaded if it was not loaded at this point.
//
// ExternalDependency will load the dll upon first access

#include <DependencyLocator.h>
#include <map>
#include <XcpAllocation.h>

#ifndef E_NOT_FOUND
#define E_NOT_FOUND HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
#endif

typedef void (STDAPICALLTYPE *PFNTELEMETRYPROC)(_In_opt_ const HMODULE, _In_ const wchar_t*);

namespace DependencyLocator
{
    namespace Internal
    {
        class ExternalDependencyStorage : public INotifyLocalDependencyStorageDestroyed
        {
            template<DWORD flags>
            class stringcomparer
            {
            public:
                bool operator()(const wrl_wrappers::HString &szLeft, const wrl_wrappers::HString &szRight) const
                {
                    unsigned int leftLength = 0;
                    unsigned int rightLength = 0;
                    auto leftBuffer = szLeft.GetRawBuffer(&leftLength);
                    auto rightBuffer = szRight.GetRawBuffer(&rightLength);
                    auto iRet = CompareStringEx(
                        nullptr, flags,
                        leftBuffer, static_cast<int>(leftLength),
                        rightBuffer, static_cast<int>(rightLength),
                        nullptr, nullptr, 0);
                    return iRet == CSTR_LESS_THAN;
                }
            };

            using ModulesMap = std::map<
                wrl_wrappers::HString,
                ILocalDependencyStorage*,
                stringcomparer<LINGUISTIC_IGNORECASE | NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH>,
                // We check for leaks while this singleton is still alive, so its safe to ignore them
                XcpAllocation::LeakIgnoringAllocator<std::pair<wrl_wrappers::HString, ILocalDependencyStorage*>>>;

        private:
            ExternalDependencyStorage() = default;
            ExternalDependencyStorage(ExternalDependencyStorage&) = delete;
            ~ExternalDependencyStorage();

        public:
            ILocalDependencyStorage* GetLocalStorageFromModule(_In_ const wchar_t* moduleName, _In_ PFNTELEMETRYPROC pfnTelemetryProc, _In_ bool allowLoadingModule = true);
            static ProtectedResource<ExternalDependencyStorage> Instance();
            void Reset();

            void Destroyed(ILocalDependencyStorage* storage) override;

        private:
            ModulesMap modules;
        };
    }

    template<class T>
    class ExternalDependency: public Dependency<T>
    {
        const wchar_t* moduleName;
        PFNTELEMETRYPROC pfnTelemetryProc;
    protected:
        Internal::ILocalDependencyStorage& GetStorage() const override
        {
            auto externalStorage = Internal::ExternalDependencyStorage::Instance();
            return *(externalStorage.Get().GetLocalStorageFromModule(moduleName, pfnTelemetryProc));
        }

    public:
        explicit ExternalDependency(const wchar_t* moduleName, PFNTELEMETRYPROC pfn) : moduleName(moduleName), pfnTelemetryProc(pfn)
        {
        }

        bool IsInitialized() const
        {
            auto externalStorage = Internal::ExternalDependencyStorage::Instance();
            return externalStorage.Get().GetLocalStorageFromModule(moduleName, pfnTelemetryProc, false /* allowLoadingModule */ ) != nullptr;
        }
    };
}

