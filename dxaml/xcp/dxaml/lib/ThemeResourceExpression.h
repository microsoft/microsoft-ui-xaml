// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Expression used to evaluate theme resource bindings.

#pragma once

#include "BindingExpressionBase_Partial.h"

class CThemeResource;

namespace DirectUI
{
    MIDL_INTERFACE("a02195b1-b4ba-42d8-84c1-71a43717d34d")
    IThemeResourceExpression: IInspectable
    {
    };

    // Expression used to evaluate theme resource bindings.
    class __declspec(uuid("75d53b20-a232-49f1-987f-579add2f6e5a")) ThemeResourceExpression :
        public BindingExpressionBase,
        public IThemeResourceExpression
    {
    protected:
        ThemeResourceExpression();
        ~ThemeResourceExpression() override;

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override
        {
            if (InlineIsEqualGUID(riid, __uuidof(ThemeResourceExpression)))
            {
                *ppObject = static_cast<ThemeResourceExpression*>(this);
            }
            else if (InlineIsEqualGUID(riid, __uuidof(IThemeResourceExpression)))
            {
                *ppObject = static_cast<IThemeResourceExpression*>(this);
            }
            else
            {
                return BindingExpressionBase::QueryInterfaceImpl(riid, ppObject);
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    private:

        // ThemeResourceExtension corresponding to this ThemeResourceExpression
        CThemeResource* m_pCoreThemeResource;

    public:
        static _Check_return_ HRESULT Create(
            _In_ CThemeResource* pCoreThemeResource,
            _Out_ ThemeResourceExpression** ppExpression);

    // BindingExpressionBase members
        // Gets a value indicating whether the value can be set.
        // ThemeResource bindings are always removed if a new value is set, so this
        // always returns FALSE.
        _Check_return_ HRESULT GetCanSetValue(_Out_ bool *pValue) override;

        // Gets a value indicating whether the expression has been associated
        // with a target.
        bool GetIsAssociated() override;

        // Attach the expression to a specific property on the target object.
        _Check_return_ HRESULT OnAttach(
            _In_ DependencyObject* pTarget,
            _In_ const CDependencyProperty* pTargetProperty) override;

        // Detach the expression from the target object.
        _Check_return_ HRESULT OnDetach() override;

        // Get the value of the target object's target property.
        _Check_return_ HRESULT GetValue(
            _In_ DependencyObject* pObject,
            _In_ const CDependencyProperty* pProperty,
            _Out_ IInspectable** ppValue) override;

        // Get the last resolved value of the target object's target property.
        _Check_return_ HRESULT GetLastResolvedThemeValue(
            _Out_ IInspectable** ppValue);

        // This is a callback from the core, responsible for setting up a theme resource
        // binding that gets re-evaluated when switching themes at runtime.
        static _Check_return_ HRESULT SetThemeResourceBinding2(
            _In_ CDependencyObject* pdo,
            _In_ KnownPropertyIndex nPropertyID,
            _In_::BaseValueSource baseValueSource,
            _In_opt_ CThemeResource* pCoreThemeResource);

        // Return the CThemeResource from a ThemeResourceExpression if one exists. Otherwise
        // return NULL.
        static _Check_return_ HRESULT GetThemeResourceNoRef(
            _In_ CDependencyObject* pdo,
            _In_ KnownPropertyIndex nPropertyID,
            _Out_ CThemeResource** ppThemeResourceNoRef);

        static _Check_return_ HRESULT GetThemeValueFromCore(
            _In_ CThemeResource* pCoreThemeResource,
            _In_ const CDependencyProperty* pProperty,
           _Out_ IInspectable** ppValue);

        // Get the last resolved theme value of the target object's target property.
         static _Check_return_ HRESULT GetLastResolvedThemeValueFromCore(
             _In_ CThemeResource* pCoreThemeResource,
            _Out_ IInspectable** ppValue);
    };
}
