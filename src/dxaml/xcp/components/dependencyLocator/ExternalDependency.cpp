// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ExternalDependency.h>
#include <minerror.h>
#include <string>
#include "LoadLibraryAbs.h"

namespace DependencyLocator {
    namespace Internal {

        typedef ILocalDependencyStorage* (__cdecl* GetDependencyLocatorStorageFuncPtr)();

        ExternalDependencyStorage::~ExternalDependencyStorage()
        {
            for (auto& module : modules)
            {
                if (module.second)
                {
                    module.second->SetDestroyedCallback(nullptr);
                }
            }
        }

        ILocalDependencyStorage* ExternalDependencyStorage::GetLocalStorageFromModule(_In_ const wchar_t* moduleName, _In_ PFNTELEMETRYPROC pfnTelemetryProc, _In_ bool allowLoadingModule)
        {
            wrl_wrappers::HString module;
            IFCFAILFAST(module.Set(moduleName));

            auto pItModule = modules.find(module);
            if (pItModule == modules.end() || pItModule->second == nullptr)
            {
                if (allowLoadingModule)
                {
                    auto moduleInstance = LoadLibraryExWAbs(moduleName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
                    if (!moduleInstance)
                    {
#pragma prefast( suppress: 26498 "IFCFAILFAST() isn't always being passed a constant everywhere, so the __HR in there can't be constexpr."
                        IFCFAILFAST(E_NOT_FOUND);
                    }

                    pfnTelemetryProc(moduleInstance, moduleName);

                    auto pfnStorage = reinterpret_cast<GetDependencyLocatorStorageFuncPtr>(GetProcAddress(moduleInstance, "GetDependencyLocatorStorage"));
                    if (!pfnStorage)
                    {
                        IFCFAILFAST(E_NOT_FOUND);
                    }

#pragma prefast( suppress: 6011 "PREfast does not understand if pfnStorage is bad we couldn't have passed the check above."
                    auto storage = pfnStorage();
                    if (!storage)
                    {
                        IFCFAILFAST(E_POINTER);
                    }

                    storage->SetDestroyedCallback(this);

                    if (pItModule == modules.end())
                    {
                        modules.insert(std::make_pair(std::move(module), storage));
                    }
                    else
                    {
                        pItModule->second = storage;
                    }

                    return storage;
                }
                else
                {
                    return nullptr;
                }
            }

            return pItModule->second;
        }

        void ExternalDependencyStorage::Reset()
        {
            for (auto& module : modules)
            {
                if (module.second)
                {
                    module.second->SetDestroyedCallback(nullptr);
                }

                auto handle = GetModuleHandle(module.first.GetRawBuffer(nullptr));
                if ((handle != INVALID_HANDLE_VALUE) && (handle != nullptr))
                {
                    FreeLibrary(handle);
                }
            }

            modules.clear();
        }

        void ExternalDependencyStorage::Destroyed(ILocalDependencyStorage* storage)
        {
            auto found = std::find_if(
                modules.begin(),
                modules.end(),
                [storage](const auto& entry)
                {
                    return storage == entry.second;
                }
            );

            if (found != modules.end())
            {
                found->second = nullptr;
            }
        }

        ProtectedResource<ExternalDependencyStorage> ExternalDependencyStorage::Instance()
        {
            static std::recursive_mutex mutex;
            static ExternalDependencyStorage instance;
            return ProtectedResource<ExternalDependencyStorage>(mutex, instance);
        }
    }
}
