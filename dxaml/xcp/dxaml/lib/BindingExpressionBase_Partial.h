// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WeakReferenceSource.h" // needed by BindingExpressionBase.g.h
#include "comTemplateLibrary.h" // needed by BindingExpressionBase.g.h
#include "BindingExpressionBase.g.h"
#include "JoltInternalInterfaces.h"

class CDependencyProperty;

namespace DirectUI
{
    PARTIAL_CLASS(BindingExpressionBase),
        public IExpressionBase
    {
    public:
        virtual _Check_return_ HRESULT GetValue(_In_ DependencyObject *pObject, _In_ const CDependencyProperty *pdp, _Out_ IInspectable **ppValue) { return E_UNEXPECTED; }

        virtual _Check_return_ HRESULT OnAttach(_In_ DependencyObject *pTarget, _In_ const CDependencyProperty *pdp) { return E_UNEXPECTED; }
        virtual _Check_return_ HRESULT OnDetach() { return E_UNEXPECTED; }

        virtual _Check_return_ HRESULT GetCanSetValue(_Out_ bool *pValue) { return E_UNEXPECTED; }
        virtual bool GetIsAssociated() { return true; }

        virtual void NotifyThemeChanged(
            _In_ Theming::Theme theme, 
            _In_ bool forceRefresh,
            _Out_ bool& valueChanged)
        {
            valueChanged = false;
        }

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override
        {
            if (InlineIsEqualGUID(riid, __uuidof(IExpressionBase)))
            {
                *ppObject = static_cast<IExpressionBase*>(this);
            }
            else 
            {
                return BindingExpressionBaseGenerated::QueryInterfaceImpl(riid, ppObject);
            }

            AddRefOuter();
            RRETURN(S_OK);
        }
    };
}
