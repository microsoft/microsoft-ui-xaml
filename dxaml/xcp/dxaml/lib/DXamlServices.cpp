// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DXamlServices.h"

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <DesignMode.h>

#include "JupiterControl.h"
#include "JupiterWindow.h"

#include <IHwndComponentHost.h>

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

namespace DirectUI
{
    namespace DXamlServices
    {
        //------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Returns the DXamlCore for the calling thread.
        //
        //------------------------------------------------------------------------
        __maybenull IDXamlCore* GetDXamlCore()
        {
            return DXamlCore::GetCurrent();
        }

        //------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Returns the DXamlCore for the calling thread.
        //
        //------------------------------------------------------------------------
        __maybenull IPeerTableHost* GetPeerTableHost()
        {
            return DXamlCore::GetCurrent();
        }

        //------------------------------------------------------------------------
        //
        //------------------------------------------------------------------------
        bool IsDXamlCoreInitializing()
        {
            return DXamlCore::IsInitializingStatic();
        }

        //------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Returns whether or not a DXamlCore instance has been initialized
        //      on the calling thread.
        //
        //------------------------------------------------------------------------
        bool IsDXamlCoreInitialized()
        {
            return DXamlCore::IsInitializedStatic();
        }

        //------------------------------------------------------------------------
        //
        //  Synopsis:
        //      Returns whether or not a DXamlCore instance has been shut down
        //      on the calling thread.
        //
        //------------------------------------------------------------------------
        bool IsDXamlCoreShutdown()
        {
            return DXamlCore::IsShutdownStatic();
        }

        bool IsDXamlCoreShuttingDown()
        {
            return DXamlCore::IsShuttingDownStatic();
        }

        CCoreServices* GetHandle()
        {
            return DXamlCore::GetCurrent()->GetHandle();
        }

        CCoreServices* GetSafeHandle()
        {
            DXamlCore* pCore = DXamlCore::GetCurrent();
            if (pCore)
            {
                return pCore->GetHandle();
            }
            return nullptr;
        }

        _Check_return_ HRESULT ActivatePeer(
            _In_ KnownTypeIndex nTypeIndex,
            _Outptr_ DependencyObject** ppObject)
        {
            return DXamlCore::GetCurrent()->ActivatePeer(nTypeIndex, ppObject);
        }

        _Check_return_ HRESULT GetPeer(
            _In_ CDependencyObject* pDO,
            _Outptr_ DependencyObject** ppObject)
        {
            return DXamlCore::GetCurrent()->GetPeer(pDO, ppObject);
        }

        _Check_return_ HRESULT GetPeer(
            _In_ CDependencyObject* pDO,
            _In_ REFIID iid,
            _Outptr_ void** ppObject)
        {
            ctl::ComPtr<DependencyObject> peer;
            IFC_RETURN(GetPeer(pDO, peer.ReleaseAndGetAddressOf()));
            return peer.CopyTo(iid, ppObject);
        }

        _Check_return_ HRESULT TryGetPeer(
            _In_ CDependencyObject* dobj,
            _Outptr_result_maybenull_ DependencyObject** object)
        {
            return DXamlCore::GetCurrent()->TryGetPeer(dobj, object);
        }

        _Check_return_ HRESULT TryGetPeer(
            _In_ CDependencyObject* pDO,
            _In_ REFIID iid,
            _Outptr_result_maybenull_ void** ppObject)
        {
            ctl::ComPtr<DependencyObject> peer;
            IFC_RETURN(TryGetPeer(pDO, peer.ReleaseAndGetAddressOf()));
            if (peer)
            {
                IFC_RETURN(peer.CopyTo(iid, ppObject));
            }

            return S_OK;
        }

        bool ShouldStoreSourceInformation()
        {
            //Cache the check on ENABLE_XAML_DIAGNOSTICS_SOURCE_INFO once since GetEnvironmentVariable is expensive.
            static bool hasCheckedEnvironmentVariable = false;
            static bool environmentVariableResult = false;

            // Check if in the V2 designer
            if (DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
            {
                return true;
            }

            // Check runtime enabled feature.
            static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
            if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics))
            {
                return true;
            }

            // Check for ETW flag.
            if (EventEnabledElementCreatedWithSourceInfo())
            {
                return true;
            }

            //Use the cached environment variable result if we've already checked it.
            if (hasCheckedEnvironmentVariable)
            {
                return environmentVariableResult;
            }
            else
            {
                hasCheckedEnvironmentVariable = true;
                // Check environment variable.
                LPCWSTR var = L"ENABLE_XAML_DIAGNOSTICS_SOURCE_INFO";
                std::array<wchar_t, MAX_PATH> buffer;
                unsigned long varSize = GetEnvironmentVariable(var, nullptr, 0);
                if ((varSize > 0) && (GetEnvironmentVariable(var, &buffer[0], buffer.size()) == varSize - 1))
                {
                    std::wstring varValue = buffer.data();
                    std::transform(varValue.begin(), varValue.end(), varValue.begin(), ::towlower);

                    if (!varValue.empty() && varValue.compare(L"0") != 0 && varValue.compare(L"false") != 0)
                    {
                        environmentVariableResult = true;
                        return true;
                    }
                }

                environmentVariableResult = false;
                return false;
            }
        }

        CJupiterWindow* GetCurrentJupiterWindow()
        {
            return DXamlCore::GetCurrent()->GetControl()->GetJupiterWindow();
        }

        wuc::ICoreWindow* GetCurrentCoreWindowNoRef()
        {
            return GetCurrentJupiterWindow()->GetCoreWindowNoRef();
        }

        bool HasCompositionTargetRenderedEventHandlers()
        {
            ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>> eventSource;
            IFCFAILFAST(DXamlCore::GetCurrent()->GetRenderedEventSource(&eventSource));

            return eventSource->HasHandlers();
        }

        bool IsInBackgroundTask()
        {
            return !!GetHandle()->IsInBackgroundTask();
        }

        
        HWND GetComponentHwndForPeer(_In_ CDependencyObject* pDO)
        {
            ctl::ComPtr<DirectUI::DependencyObject> peer;
            DXamlCore::GetCurrent()->TryGetPeer(pDO, &peer);
            if (peer)
            {
                ctl::ComPtr<IHwndComponentHost> host;
                HRESULT qiResult = ctl::iinspectable_cast(peer.Get())->QueryInterface(__uuidof(IHwndComponentHost), reinterpret_cast<void**>(host.ReleaseAndGetAddressOf()));
                if (SUCCEEDED(qiResult) && host != nullptr)
                {
                    return host->GetComponentHwnd(); 
                }
            }
            return nullptr;
        }
    }
}
