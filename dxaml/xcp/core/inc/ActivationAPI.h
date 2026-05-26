// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Type Activation API. Used to dynamically activate types at runtime.

#pragma once

#include "MetadataAPI.h"

class CClassInfo;

namespace DirectUI
{
    class ActivationAPI
    {
    public:
        // Activates a type.
        static _Check_return_ HRESULT ActivateInstance(_In_ const CClassInfo* pType, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance);
        // ActivateInstance2 is more robust and does better error handling and checking for certain cases.
        static _Check_return_ HRESULT ActivateInstance2(_In_ const CClassInfo* pType, _Outptr_ IInspectable** ppInstance);
        static _Check_return_ HRESULT ActivateCoreInstance(_In_ const CClassInfo* pType, _Outptr_ CDependencyObject** ppInstance);
        static inline _Check_return_ HRESULT ActivateInstance(_In_ const CClassInfo* pType, _Outptr_ IInspectable** ppInstance)
        {
            RRETURN(ActivateInstance(pType, /* pOuter */ nullptr, ppInstance));
        }
        static inline _Check_return_ HRESULT ActivateInstance(_In_ KnownTypeIndex eTypeIndex, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance)
        {
            RRETURN(ActivateInstance(MetadataAPI::GetClassInfoByIndex(eTypeIndex), pOuter, ppInstance));
        }
        static inline _Check_return_ HRESULT ActivateInstance(_In_ KnownTypeIndex eTypeIndex, _Outptr_ IInspectable** ppInstance)
        {
            RRETURN(ActivateInstance(MetadataAPI::GetClassInfoByIndex(eTypeIndex), ppInstance));
        }

        // Activates a type from a string.
        static _Check_return_ HRESULT ActivateInstanceFromString(_In_ const CClassInfo* pType, _In_ const xstring_ptr_view& strValue, _Outptr_ IInspectable** ppInstance);

        // Activates an automation peer type.
        static _Check_return_ HRESULT ActivateAutomationInstance(_In_ const CClassInfo* pType, _In_ CDependencyObject* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance);
        static inline _Check_return_ HRESULT ActivateAutomationInstance(_In_ const CClassInfo* pType, _In_ CDependencyObject* pOwner, _Outptr_ IInspectable** ppInstance)
        {
            RRETURN(ActivateAutomationInstance(pType, pOwner, /* pOuter */ nullptr, ppInstance));
        }
        static inline _Check_return_ HRESULT ActivateAutomationInstance(_In_ KnownTypeIndex eTypeIndex, _In_ CDependencyObject* pOwner, _In_opt_ IInspectable* pOuter, _Outptr_ IInspectable** ppInstance)
        {
            RRETURN(ActivateAutomationInstance(MetadataAPI::GetClassInfoByIndex(eTypeIndex), pOwner, pOuter, ppInstance));
        }
        static inline _Check_return_ HRESULT ActivateAutomationInstance(_In_ KnownTypeIndex eTypeIndex, _In_ CDependencyObject* pOwner, _Outptr_ IInspectable** ppInstance)
        {
            RRETURN(ActivateAutomationInstance(MetadataAPI::GetClassInfoByIndex(eTypeIndex), pOwner, ppInstance));
        }
        template<class T>
        static _Check_return_ HRESULT ActivateAutomationInstance(_In_ KnownTypeIndex eTypeIndex, _In_ CDependencyObject* pOwner, _Outptr_ T** ppInstance)
        {
            RRETURN(ActivateAutomationInstance(eTypeIndex, pOwner, __uuidof(T), reinterpret_cast<void**>(ppInstance)));
        }
        static _Check_return_ HRESULT ActivateAutomationInstance(_In_ KnownTypeIndex eTypeIndex, _In_ CDependencyObject* pOwner, _In_ REFIID iid, _Outptr_ void** ppInstance);

    private:
        static _Check_return_ HRESULT ActivateMediaPlaybackItemInstanceFromUri(_In_ const xstring_ptr_view& uriString, _Outptr_ IInspectable** ppInstance);
    };
}
