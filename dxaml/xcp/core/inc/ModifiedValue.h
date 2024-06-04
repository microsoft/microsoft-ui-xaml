// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <BaseValueSource.h>

class CThemeResourceExtension;

enum FullValueSource
{
    // BaseValueSource stored in bits 0-3
    fvsBaseValueSourceMask = 0x000F,
    fvsModifiersMask = 0x0070,
    fvsIsAnimated = 0x0010,
    fvsIsExpression = 0x0020,

    // Is LocalValue newer than animated value?
    fvsLocalValueNewerThanAnimatedValue = 0x1000
};

// When a modifier (e.g. animation) is present CModifiedValue holds the modifier value and source and the base value and source.
class CModifiedValue
{

public:
    CModifiedValue(_In_ const CDependencyProperty *pdp);
    CModifiedValue& operator=(const CModifiedValue &other) = delete;
    
    ~CModifiedValue()
    {
        // Ensure the base value's expected reference is cleared.
        VERIFYHR(ClearBaseValue());
    }

public:
    bool HasModifiers();

    KnownPropertyIndex GetPropertyIndex()
    {
        return m_propertyIndex;
    }

    _Check_return_ HRESULT SetAnimatedValue(_In_ const CValue& value, _In_ const xref_ptr<CDependencyObject>& sourceObject = nullptr);
    _Check_return_ HRESULT GetAnimatedValue(_Out_ CValue* pValue);
    xref_ptr<CDependencyObject> GetAnimatedValueSource();
    _Check_return_ HRESULT ClearAnimatedValue();
    bool IsAnimated() const;

    bool IsEffectiveValueThemeResource();
    bool IsBaseValueThemeResource();

    _Check_return_ HRESULT SetBaseValue(_In_ const CValue& value, _In_ BaseValueSource baseValueSource);
    _Check_return_ HRESULT GetBaseValue(_Out_ CValue* pValue);
    _Check_return_ HRESULT GetBaseValueThemeResource(_Out_ CValue* pValue);
    BaseValueSource GetBaseValueSource() { return static_cast<BaseValueSource>(m_fullValueSource & fvsBaseValueSourceMask); }

    _Check_return_ HRESULT GetEffectiveValue(_Out_ CValue* pValue);

    CThemeResource* GetEffectiveValueThemeResourceNoRef();

    bool IsModifierValueBeingSet() { return m_modiferValueBeingSet; }
    void SetModifierValueBeingSet(_In_ bool value) { m_modiferValueBeingSet = value; }

    bool ShouldTemplateBindingBeStopped() { return m_stopTemplateBinding; }
    void StopTemplateBinding(_In_ bool value) { m_stopTemplateBinding = value; }

    void ReferenceTrackerWalk(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer);

    bool IsAnimatedValueOverwritten() const { return IsAnimated() && ((m_fullValueSource & fvsLocalValueNewerThanAnimatedValue) == fvsLocalValueNewerThanAnimatedValue); }

    void RemoveExpectedReference();

private:
    _Check_return_ HRESULT ClearBaseValue();
    bool HasBaseValue();

private:
    CValue m_animatedValue;

    CValue m_baseValue;

    UINT32 m_fullValueSource;

    xref_ptr<CDependencyObject> m_sourceObject;

    // Property for which this is the modified value
    KnownPropertyIndex m_propertyIndex;

    // Is modifier value being set for this property?
    bool m_modiferValueBeingSet : 1;

    // If local value is set during modifying, we register that we want to stop templatebinding.
    bool m_stopTemplateBinding : 1;

    // Did SetBaseValue call AddExtraExpectedReferenceOnPeer
    bool m_addedExpectedReference : 1;
};
