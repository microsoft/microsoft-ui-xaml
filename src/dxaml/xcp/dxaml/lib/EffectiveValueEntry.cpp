// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EffectiveValueEntry.h"
#include "BindingExpressionBase_Partial.h"

using namespace DirectUI;

EffectiveValueEntry::~EffectiveValueEntry()
{
    if (IsExpression())
    {
        ctl::ComPtr<IExpressionBase> spIExpressionBase;
        VERIFYHR(m_BaseValue.As(&spIExpressionBase));
        VERIFYHR(spIExpressionBase.Cast<BindingExpressionBase>()->OnDetach());
    }
}

ctl::ComPtr<IInspectable> EffectiveValueEntry::GetBaseValue() const
{
    return ctl::ComPtr<IInspectable>(m_BaseValue.Get());
}

_Check_return_ HRESULT EffectiveValueEntry::SetBaseValue(
    _In_ IInspectable *pValue,
    _In_ BaseValueSource baseValueSource)
{
    //
    // TODO
    // Replace with a more complex check that
    // can handle the BindingExpressionBase and IInspectable object type comparisons.
    //

    m_BaseValue.Set(pValue);

    m_fullValueSource &= ~fvsBaseValueSourceMask;
    m_fullValueSource |= baseValueSource;

    if (baseValueSource == BaseValueSourceLocal)
    {
        m_fullValueSource |= fvsLocalValueNewerThanAnimatedValue;
    }
    else
    {
        m_fullValueSource &= ~fvsLocalValueNewerThanAnimatedValue;
    }

    return S_OK;
}

void EffectiveValueEntry::ClearBaseValue()
{
    if (HasBaseValue())
    {
        m_BaseValue.Clear();
        m_fullValueSource &= ~fvsBaseValueSourceMask;
    }
}

ctl::ComPtr<IInspectable> EffectiveValueEntry::GetEffectiveValue() const
{
    if (IsExpression())
    {
        return ctl::ComPtr<IInspectable>(m_ExpressionValue.Get());
    }
    else
    {
        ASSERT(HasBaseValue());
        return ctl::ComPtr<IInspectable>(m_BaseValue.Get());
    }
}

_Check_return_ HRESULT EffectiveValueEntry::GetLocalValue(
    _Out_ IInspectable** ppValue)
{
    if (IsPropertyLocal())
    {
        if (!HasModifiers())
        {
            IFC_RETURN(GetEffectiveValue().MoveTo(ppValue));
        }
        else
        {
            IGNOREHR(m_BaseValue.CopyTo(ppValue));
        }
    }
    else
    {
        IFC_RETURN(DependencyPropertyFactory::GetUnsetValue(ppValue));
    }

    return S_OK;
}

_Check_return_ HRESULT EffectiveValueEntry::SetExpressionValue(
    _In_ IInspectable *pValue)
{
    bool areEqual = false;

    IFC_RETURN(PropertyValue::AreEqual(
        m_ExpressionValue.Get(),
        pValue,
        &areEqual));

    // only setting if the new value is not already the expression value
    if (!IsExpression() || !areEqual)
    {
        if (m_ExpressionValue)
        {
            ClearExpressionValue();
        }

        m_ExpressionValue.Set(pValue);
    }

    m_fullValueSource |= fvsIsExpression;
    m_fullValueSource |= fvsLocalValueNewerThanAnimatedValue;

    return S_OK;
}

void EffectiveValueEntry::ClearExpressionValue()
{
    if (IsExpression())
    {
        m_ExpressionValue.Clear();
        m_fullValueSource &= ~fvsIsExpression;
    }
}

bool EffectiveValueEntry::ReferenceTrackerWalk(
    _In_ EReferenceTrackerWalkType walkType,
    _In_ bool fIsRoot)
{
    m_BaseValue.ReferenceTrackerWalk(walkType);
    m_ExpressionValue.ReferenceTrackerWalk(walkType);
    return true;
}

_Check_return_ HRESULT 
EffectiveValueEntry::NotifyThemeChanged(
    _In_ Theming::Theme theme, 
    _In_ bool forceRefresh,
    _Out_ bool& valueChanged)
{
    if (   GetBaseValueSource() == BaseValueSourceLocal
        && IsExpression())
    {
        ctl::ComPtr<IInspectable> baseValue = GetBaseValue();

        ctl::ComPtr<IExpressionBase> baseValueAsIExpressionBase;
        IFC_RETURN(baseValue.As(&baseValueAsIExpressionBase));
        // Binding errors are ignored
        baseValueAsIExpressionBase.Cast<BindingExpressionBase>()->NotifyThemeChanged(theme, forceRefresh, valueChanged);
    }
    return S_OK;
}