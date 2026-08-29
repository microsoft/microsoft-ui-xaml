// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DependencyObject.h" // needed by ExternalObjectReference.g.h
#include "EnumDefs.g.h" // needed by ExternalObjectReference.g.h
#include "ExternalObjectReference.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ExternalObjectReference)
    {
    protected:
        ExternalObjectReference() = default;
        ~ExternalObjectReference() override = default;
        
    public:

        bool IsExternalObjectReference() override
        { return true; }

        void get_Target(_Outptr_result_maybenull_ IInspectable **ppTarget)
        {
            *ppTarget = m_Target.Get();
            AddRefInterface(*ppTarget);
        }

        _Check_return_ HRESULT put_Target(_In_ IInspectable *pTarget)
        {
            HRESULT hr = S_OK;

            IFCEXPECT(m_Target.Get() == NULL);
            SetPtrValue(m_Target, pTarget);


        Cleanup:

            return hr;
        }

        // Calback from core
        static _Check_return_ HRESULT GetTarget(_In_ CDependencyObject* pNativeDO, _Outptr_ IInspectable** ppTarget);

        static _Check_return_ HRESULT ConditionalWrap(_In_ IInspectable *pValue, _Outptr_ DependencyObject **ppWrapped, _Out_opt_ BOOLEAN *pWasWrapped = NULL);
        static _Check_return_ HRESULT ShouldBeWrapped(_In_ IInspectable *pValue, _Out_ BOOLEAN& returnValue);
        static _Check_return_ HRESULT Wrap(_In_ IInspectable *pInspectable, _Outptr_ DependencyObject **ppWrapped);

    private:
        TrackerPtr<IInspectable> m_Target;
    };
};
