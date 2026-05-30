// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CValue.h>
#include <CValueUtil.h>
#include <TypeTableStructs.h>
#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <ModifiedValue.h>
#include <ThemeResourceExtension.h>
#include <ThemeResource.h>
#include <corep.h>

CModifiedValue::CModifiedValue(_In_ const CDependencyProperty* dp) :
    m_propertyIndex(dp->GetIndex()),
    m_fullValueSource(0),
    m_modiferValueBeingSet(false),
    m_addedExpectedReference(false),
    m_stopTemplateBinding(false)
{
}

bool CModifiedValue::HasModifiers()
{
    return (m_fullValueSource & fvsModifiersMask) != 0;
}

_Check_return_ HRESULT CModifiedValue::GetAnimatedValue(_Out_ CValue *pValue)
{
    IFCEXPECT_ASSERT_RETURN(IsAnimated());

    IFC_RETURN(pValue->CopyConverted(m_animatedValue));

    // Addref object to be consistent with CDependencyObject::GetValue's
    // resource management rules.
    if (pValue->GetType() == valueThemeResource)
    {
        // If the value is a ThemeResourceExtension, return the resolved value instead.
        CThemeResource* pThemeResourceNoRef = pValue->AsThemeResource();
        if (pThemeResourceNoRef)
        {
            IFC_RETURN(pThemeResourceNoRef->GetLastResolvedThemeValue(pValue));
        }
    }

    return S_OK;
}

xref_ptr<CDependencyObject> CModifiedValue::GetAnimatedValueSource()
{
    ASSERT(IsAnimated());
    return m_sourceObject;
}

_Check_return_ HRESULT CModifiedValue::SetAnimatedValue(_In_ const CValue& value, _In_ const xref_ptr<CDependencyObject>& sourceObject)
{
    // only setting if the new value is not already the animated value
    if (!IsAnimated() || (!CValueUtil::EqualsWithoutUnboxing(m_animatedValue, value)))
    {
        IFC_RETURN(m_animatedValue.CopyConverted(value));
    }

    m_sourceObject = sourceObject;

    m_fullValueSource |= fvsIsAnimated;
    m_fullValueSource &= (~fvsLocalValueNewerThanAnimatedValue);

    return S_OK;
}

_Check_return_ HRESULT CModifiedValue::ClearAnimatedValue()
{
    if (IsAnimated())
    {
        m_animatedValue.SetNull();

        m_sourceObject.reset();
        m_fullValueSource &= (~fvsIsAnimated);
        m_fullValueSource &= (~fvsLocalValueNewerThanAnimatedValue);
    }

    return S_OK;
}

bool CModifiedValue::IsAnimated() const
{
    return (m_fullValueSource & fvsIsAnimated) != 0;
}

bool CModifiedValue::IsEffectiveValueThemeResource()
{
    if (IsAnimated() && (m_fullValueSource & fvsLocalValueNewerThanAnimatedValue) == 0)
    {
        if (m_animatedValue.GetType() == valueThemeResource)
        {
            return m_animatedValue.AsThemeResource() != nullptr;
        }
    }
    else
    {
        if (m_baseValue.GetType() == valueThemeResource)
        {
            return m_baseValue.AsThemeResource() != nullptr;
        }
    }
    return false;
}

bool CModifiedValue::IsBaseValueThemeResource()
{
    if (m_baseValue.GetType() == valueThemeResource)
    {
        return m_baseValue.AsThemeResource() != nullptr;
    }
    return false;
}

// Get base value. Unwraps any theme resource extensions.
_Check_return_ HRESULT CModifiedValue::GetBaseValue(_Out_ CValue *pValue)
{
    IFCEXPECT_ASSERT_RETURN(HasBaseValue());

    IFC_RETURN(pValue->CopyConverted(m_baseValue));

    if (pValue->GetType() == valueThemeResource)
    {
        // If the value is a ThemeResourceExtension, return the resolved value.
        CThemeResource* pThemeResourceNoRef = pValue->AsThemeResource();
        if (pThemeResourceNoRef)
        {
            IFC_RETURN(pThemeResourceNoRef->GetLastResolvedThemeValue(pValue));
        }
    }

    return S_OK;
}

// Get base value, without unwrapping any theme resource extensions.
_Check_return_ HRESULT CModifiedValue::GetBaseValueThemeResource(_Out_ CValue *pValue)
{
    ASSERT(IsBaseValueThemeResource());
    IFCEXPECT_ASSERT_RETURN(HasBaseValue());

    IFC_RETURN(pValue->CopyConverted(m_baseValue));

    if (pValue->GetType() == valueThemeResource)
    {
        CThemeResource* pThemeResource = pValue->AsThemeResource();
        if (pThemeResource)
        {
            return S_OK;
        }
    }

    // This function should only be called when IsBaseValueThemeResource returns TRUE.
    IFC_RETURN(E_FAIL);

    return S_OK;
}

_Check_return_ HRESULT CModifiedValue::SetBaseValue(
    _In_ const CValue& value,
    _In_ BaseValueSource baseValueSource)
{

    // Is the new value already the base value?
    // In the case of boxed values, like an IPropertyValue with a boolean, compare
    // the pointers, not the boolean value, because we preserve the pointers in the property system.
    // (Reference tracking depends on the modified value having the correct reference.)
    if (HasBaseValue() && CValueUtil::EqualsWithoutUnboxing(m_baseValue, value))
    {
        // value has not changed but need to recalculate fullValueSource
        m_fullValueSource &= (~fvsBaseValueSourceMask);
    }
    else
    {
        IFC_RETURN(ClearBaseValue());
        IFC_RETURN(m_baseValue.CopyConverted(value));

        CDependencyObject* valueAsObject = m_baseValue.AsObject();

        if (valueAsObject != nullptr &&
            valueAsObject->HasManagedPeer())
        {
            // When this value is the effective value, CDependencyObject keeps an expected
            // reference count on the peer.  When it's the base value of CModifiedValue, it might stop being the
            // effective value, and consequently lose that expected reference count.  So
            // we need to ensure we keep an expected reference on the peer from here.

            // We don't check for doValue->ParticipatesInManagedTree, we add the expective ref
            // in either case, because it might start participating after this call and at that point depend
            // on the expected ref.

            valueAsObject->GetContext()->AddValueWithExpectedReference(this);

            IFC_RETURN(valueAsObject->AddExtraExpectedReferenceOnPeer());

            m_addedExpectedReference = true;
        }
    }

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

_Check_return_ HRESULT CModifiedValue::ClearBaseValue()
{
    if (HasBaseValue())
    {
        // Remove the expected reference to the peer that we set in SetBaseValue.
        if (m_addedExpectedReference)
        {
            RemoveExpectedReference();
        }

        m_baseValue.SetNull();
        m_fullValueSource &= (~fvsBaseValueSourceMask);
    }
    return S_OK;
}

void CModifiedValue::RemoveExpectedReference()
{
    ASSERT(m_addedExpectedReference);
    ASSERT(HasBaseValue());

    ASSERT(m_baseValue.GetType() == valueObject);
    ASSERT(m_baseValue.AsObject() != nullptr);

    m_baseValue.AsObject()->GetContext()->RemoveValueWithExpectedReference(this);

    IFCFAILFAST(m_baseValue.AsObject()->ClearExtraExpectedReferenceOnPeer());

    m_addedExpectedReference = false;
}

bool CModifiedValue::HasBaseValue()
{
    return (m_fullValueSource & fvsBaseValueSourceMask) != 0;
}

// Get effective value by applying precedence rules.
_Check_return_ HRESULT CModifiedValue::GetEffectiveValue(_Out_ CValue *pValue)
{
    if (IsAnimated())
    {
        // If a local value has been set after an animated value, the local
        // value has precedence. This is different from WPF and is done because
        // some legacy SL apps depend on this and because SL Animation thinks that
        // it is better design for an animation in filling period to be trumped by a
        // local value. In the active period of an animation, the next animated
        // value will take precedence over the old local value.
        if (m_fullValueSource & fvsLocalValueNewerThanAnimatedValue)
        {
            ASSERT(GetBaseValueSource() == BaseValueSourceLocal);
            IFC_RETURN(GetBaseValue(pValue));
        }
        else
        {
            IFC_RETURN(GetAnimatedValue(pValue));
        }
    }
    else
    {
        ASSERT(HasBaseValue());
        IFC_RETURN(GetBaseValue(pValue));
    }

    return S_OK;
}

CThemeResource* CModifiedValue::GetEffectiveValueThemeResourceNoRef()
{
    ASSERT(IsEffectiveValueThemeResource());
    if (IsAnimated() && (m_fullValueSource & fvsLocalValueNewerThanAnimatedValue) == 0)
    {
        return m_animatedValue.AsThemeResource();
    }
    else
    {
        return m_baseValue.AsThemeResource();
    }
}



// During the reference tracker walk, walk the base value if we have one.
void CModifiedValue::ReferenceTrackerWalk(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    if (HasBaseValue() &&
        m_baseValue.AsObject() != nullptr)
    {
        m_baseValue.AsObject()->ReferenceTrackerWalk(
            walkType,
            false,  //isRoot
            true);  //shouldWalkPeer
    }
}
