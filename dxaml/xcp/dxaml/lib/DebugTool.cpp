// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InternalDebugInteropModel.h"
#include "InternalDebugInterop.h"
#include "DiagnosticsInterop.h"
#include "DiagnosticsInteropModel.h"
#include "RuntimeEnabledFeatures.h"
#include "DependencyLocator.h"
#include <XamlDiagnostics.h>
#include "JupiterControl.h"
#include "Callback.h"
#include "MetadataIterator.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

using namespace DirectUI;
// Creates a debug tool instance, if the tool dll is installed.
namespace DebugTool
{
    static LPCWSTR g_wszVisualDiagDMPath = L"XAML_DM_PATH";
    static LPCWSTR g_wszVisualXamlClsid = L"XAML_DM_CLSID";

    static HRESULT
        TryGetEnv(
        _In_ LPCWSTR var,
        _In_ const EnvironmentMap& env,
        _Out_opt_ std::wstring* pValue)
    {
        auto iter = env.find(var);
        if (iter != env.end())
        {
            if (pValue)
            {
                *pValue = iter->second;
            }

            return S_OK;
        }

        std::array<wchar_t, MAX_PATH> buffer;
        unsigned long varSize = GetEnvironmentVariable(var, nullptr, 0);
        if ((varSize > 0) && (GetEnvironmentVariable(var, &buffer[0], buffer.size()) == varSize - 1))
        {
            if (pValue)
            {
                *pValue = buffer.data();
            }

            return S_OK;
        }

        return E_FAIL;
    }


    // NOTE: This is the code path used by the Core Messsaging IPC infrastructure
    HRESULT
        CreateDiagnostics(
        _In_ CJupiterControl * pControl,
        _In_ const Msg_ConnectToVisualTree * pMsg)
    {
        EnvironmentMap env;
        env[g_wszVisualDiagDMPath] = pMsg->wszTAPDllName;
        env[g_wszInitializationData] = pMsg->wszInitializationData;

        LPOLESTR wszClsId = nullptr;
        IFC_RETURN(StringFromCLSID(pMsg->tapClsid, &wszClsId));

        auto Xbfv2Guard = wil::scope_exit([&]()
        {
            CoTaskMemFree(wszClsId);
        });

        env[g_wszVisualXamlClsid] = wszClsId;


        std::vector<LPCWSTR> vars;

        // Set the runtime flag as soon as possible.
        if (SUCCEEDED(TryGetEnv(g_wszVisualDiagDMPath, env, nullptr)))
        {
            auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
            runtimeEnabledFeatureDetector->SetFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics, true);
        }

        vars.push_back(g_wszVisualDiagDMPath);

        IFC_RETURN(Create(pControl, vars, env));

        return S_OK;
    }

    typedef std::tuple<std::vector<LPCWSTR>, EnvironmentMap> CallbackParams;

    static HRESULT callback(CJupiterControl* pControl, CallbackParams callbackParams)
    {
        std::vector<HMODULE> modules;

        std::vector<LPCWSTR> variables;
        EnvironmentMap env;
        std::tie(variables, env) = std::move(callbackParams);

        auto moduleCleanup = wil::scope_exit([&modules]()
        {
            for (auto module : modules)
            {
                FreeLibrary(module);
            }
        });

        for (auto &elem : variables)
        {
            std::wstring moduleName;

            if (SUCCEEDED(TryGetEnv(elem, env, &moduleName)))
            {
                // Only allow absolute paths
                FAIL_FAST_ASSERT(!PathIsRelativeW(moduleName.c_str()));
                HMODULE handle = LoadLibraryEx(moduleName.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);

                if (!handle)
                {
                // All of the modules must load successfully for create to succeed.
                return E_FAIL;
                }

                modules.push_back(handle);
            }
        }

        if (modules.size() == 0)
        {
            return S_OK;
        }

        ctl::ComPtr<IXamlDiagnostics> xamlDiagnostics;
        if (SUCCEEDED(XamlDiagnostics::Create(&xamlDiagnostics)) && xamlDiagnostics)
        {
            const auto interop = Diagnostics::GetDiagnosticsInterop(true);
            interop->Launch(xamlDiagnostics, env);

            // This is really weird, but it is what we've always done. Theoretically, launch should
            // have succeeded and the tap now has a reference to xamldiagnostics and is keeping it alive.
            // however, this isn't the case if the core window hasn't been setup yet and we don't
            // have a dispatcher. there is a weird limbo state between now and then where if something fails
            // we would leak, but this probably also result in the app failing to load so i think we are ok.
            xamlDiagnostics.Detach();
        }

        moduleCleanup.release();

        return S_OK;
    }

    HRESULT
        Create(
        _In_ CJupiterControl* pControl,
        _In_ const std::vector<LPCWSTR>& variables,
        _In_ const EnvironmentMap& env)
    {
        ctl::ComPtr<ICallback> spCallback;
        ctl::ComPtr<IDispatcher> spDispatcher;

        // MakeCallback can only take 2 pass-through parameters,
        CallbackParams callbackPair(variables, env);

        spCallback = MakeCallback<CJupiterControl*, CallbackParams>(
            &callback,
            pControl,
            callbackPair);

        IFC_RETURN(DXamlCore::GetCurrent()->GetXamlDispatcher(&spDispatcher));
        IFC_RETURN(spDispatcher->RunAsync(spCallback));

        return S_OK;
    }

    // Get all of the properties on the given DO and add them to the given
    // ICollection.  This takes a vector so it can be called internally,
    // rather than by a debug tool.
    HRESULT
        GetPropertiesForDO(
        _In_ xaml::IDependencyObject* pReference,
        _In_ bool bGetDefaultValues,
        _In_ std::function<HRESULT(IInspectable*, HSTRING*)> valueToString,
        _Inout_ std::vector<DebugTool::DebugPropertyInfo>& propInfoList)
    {
        std::vector<DebugTool::DebugPropertyInfo> localPropInfoList;

        ASSERT(pReference);

        CDependencyObject *pDO = static_cast<CDependencyObject*>(static_cast<DependencyObject*>(pReference)->GetHandle());

        // First iterate over all of the built-in properties.
        // CDependencyObject::GetPropertyByIndexInline can't handle custom properties, so only iterate over known property counts
        auto iterator = Diagnostics::EnumIterator<KnownPropertyIndex>(Diagnostics::EnumIterator<KnownPropertyIndex>::Begin());

        while (*iterator != static_cast<KnownPropertyIndex>(KnownDependencyPropertyCount))
        {
            const CDependencyProperty* pProperty = pDO->GetPropertyByIndexInline(*iterator);

            if (pProperty && pProperty->GetIndex() != KnownPropertyIndex::UIElement_ChildrenInternal)
            {
                bool didPopulate = false;
                DebugTool::DebugPropertyInfo debugPropertyInfo;

                IFC_RETURN(InternalDebugInterop::PopulateBaseValueDebugPropertyInfo(
                    DXamlCore::GetCurrent()->GetHandle(),       // pCore
                    valueToString,                              // valueToString
                    static_cast<DependencyObject*>(pReference), // pObject
                    pProperty,                                  // pProperty
                    pProperty->GetIndex(),                      // propertyIndex
                    bGetDefaultValues,                          // pGetDefaultValue
                    didPopulate,                                // didPopulate
                    &debugPropertyInfo)                         // pDebugPropertyInfo
                    );

                if (didPopulate)
                {
                    localPropInfoList.push_back(std::move(debugPropertyInfo));
                }
            }

            iterator++;
        }

        // TODO:  Need to handle default-value non-core properties, plus custom attached properties

        InternalDebugInterop::PopulateDebugPropertyInfoCollection(
            static_cast<DependencyObject*>(pReference),
            valueToString,
            localPropInfoList,
            bGetDefaultValues);

        propInfoList = std::move(localPropInfoList);

        return S_OK;
    }
}
