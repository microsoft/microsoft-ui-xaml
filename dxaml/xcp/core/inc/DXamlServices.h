// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ReferenceTrackerInterfaces.h"

class CDependencyObject;
class CCoreServices;
class CStyle;
class CSetterBaseCollection;
class CResourceDictionary;
class CJupiterWindow;

namespace DirectUI
{

    class DependencyObject;

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Global functions used to get to DXaml layer services.
    //
    //  Notes:
    //      These functions all return interfaces to decouple clients from
    //      concrete implementations. Stubbed out in test code.
    //
    //------------------------------------------------------------------------
    namespace DXamlServices
    {
        __maybenull IDXamlCore* GetDXamlCore();
        __maybenull IPeerTableHost* GetPeerTableHost();
        bool IsDXamlCoreInitializing();
        bool IsDXamlCoreIdle();
        bool IsDXamlCoreInDesignMode();
        bool IsDXamlCoreInitialized();
        bool IsDXamlCoreShutdown();
        bool IsDXamlCoreShuttingDown();
        CCoreServices* GetHandle();
        CCoreServices* GetSafeHandle();
        _Check_return_ HRESULT ActivatePeer(_In_ KnownTypeIndex nTypeIndex, _Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT GetPeer(_In_ CDependencyObject* pDO, _Outptr_ DependencyObject** ppObject);
        
        _Check_return_ HRESULT GetPeer(_In_ CDependencyObject* pDO, _In_ REFIID iid, _Outptr_ void** ppObject);

        _Check_return_ HRESULT TryGetPeer(_In_ CDependencyObject* dobj, _Outptr_result_maybenull_ DependencyObject** peer);
        _Check_return_ HRESULT TryGetPeer(_In_ CDependencyObject* pDO, _In_ REFIID iid, _Outptr_result_maybenull_ void** ppObject);
        
        bool ShouldStoreSourceInformation();
        CJupiterWindow* GetCurrentJupiterWindow();
        wuc::ICoreWindow* GetCurrentCoreWindowNoRef();
        bool HasCompositionTargetRenderedEventHandlers();
        bool IsInBackgroundTask();
    }
}
