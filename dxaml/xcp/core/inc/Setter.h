// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <AssociativeStorage.h>
#include "SetterBase.h"

class CStyle;

namespace Diagnostics { class DiagnosticsInterop; }

class CSetter final : public CSetterBase
{
    friend class Diagnostics::DiagnosticsInterop;
private:
    CSetter(_In_ CCoreServices *pCore) : CSetterBase( pCore )
        , m_pDependencyPropertyProxy(nullptr)
        , m_valueResolved(false)
        , m_target(nullptr)
        , m_themeChanging(false)
        , m_isValueMutable(false)
    {
    }

   ~CSetter() override;

public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSetter>::Index;
    }

    // Creation method
    DECLARE_CREATE(CSetter);

    // Getters that allow us to bypass/shortcut GetValue()
    _Check_return_ HRESULT GetProperty(_In_ KnownTypeIndex typeIndex, _Out_ KnownPropertyIndex *puiPropertyId);
    _Check_return_ HRESULT GetSetterValue(_Out_ CValue *pValue);
    xref_ptr<CTargetPropertyPath> GetTargetPropertyPath();

    // Like ResolveValue(), but resolves the value using the passed in DP instead of the
    // Setter's Property/Target properties. This is primarily intended for scenarios
    // where the DP might've already been resolved separately and we don't want to redo the work
    _Check_return_ HRESULT ResolveValueUsingProperty(_In_ const CDependencyProperty* dp);

    bool IsStyleSetter() const;
    bool IsValueValid() const;
    bool IsValueValidDiagnostics();

// CDependencyObject overrides
public:
    // Override so that Setter_Value can be resolved if needed
    _Check_return_ HRESULT GetValue(
        _In_ const CDependencyProperty *pdp,
        _Out_ CValue *pValue) override;

    // Override to prevent modification if the Setter is final
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

     _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

     _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

     void SubscribeToValueChangedNotification(_In_ CStyle* const subscriber);
     void UnsubscribeFromValueChangedNotification(_In_ CStyle* const subscriber);

    CDependencyObject* GetTemplatedParent() override;

    void SetIsValueMutable(bool value)
    {
        m_isValueMutable = value;
    }

    bool GetIsValueMutable() const
    {
        return m_isValueMutable;
    }

    // properties.
    CDependencyPropertyProxy *m_pDependencyPropertyProxy;
    CValue m_vValue;
    CTargetPropertyPath *m_target;

protected:
    void SetTemplatedParentImpl(_In_ CDependencyObject* parent) final;

private:
    _Check_return_ HRESULT ResolveDependencyProperty(_Outptr_result_maybenull_ const CDependencyProperty** ppDP);

     _Check_return_ HRESULT OnSetterPropertyChanged(_In_ const PropertyChangedParams& args);
     _Check_return_ HRESULT OnSetterTargetChanged(_In_ const PropertyChangedParams& args);
     _Check_return_ HRESULT OnSetterValueChanged(_In_ const PropertyChangedParams& args);

    xref::weakref_ptr<CDependencyObject> m_templatedParent;
    bool m_valueResolved : 1;
    bool m_themeChanging : 1;
    bool m_isValueMutable : 1;
};
