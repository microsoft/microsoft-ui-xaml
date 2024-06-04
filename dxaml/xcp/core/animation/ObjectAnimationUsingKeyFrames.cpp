// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ObjectAnimationUsingKeyFrames.h"
#include "ObjectKeyFrame.h"

using namespace DirectUI;

_Check_return_ HRESULT CObjectAnimationUsingKeyFrames::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    // Create type
    xref_ptr<CObjectAnimationUsingKeyFrames> animation;
    animation.init(new CObjectAnimationUsingKeyFrames(pCreate->m_pCore));

    // Create internal members
    xref_ptr<CObjectKeyFrameCollection> keyFrames;
    IFC_RETURN(CObjectKeyFrameCollection::Create(reinterpret_cast<CDependencyObject **>(keyFrames.ReleaseAndGetAddressOf()), pCreate));

    auto restoreTimingOwnerGuard = wil::scope_exit([&keyFrames]
    {
        if (keyFrames)
        {
            keyFrames->SetTimingOwner(nullptr);
        }
    });

    animation->m_pDPValue = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ObjectKeyFrame_Value);
    ASSERT(animation->m_pDPValue != nullptr);

    // Set Defaults
    // Attach inner members

    animation->m_pKeyFrames = keyFrames;

    if (keyFrames)
    {
        keyFrames->SetTimingOwner(animation);

        // Parent the collection as we would if this were part of a SetValue call.
        IFC_RETURN(keyFrames->AddParent(animation, FALSE));
    }

    animation->m_vFrom.SetNull();
    animation->m_vTo.SetNull();
    animation->m_vBaseValue.SetNull();
    animation->m_vNonAnimatedBaseValue.SetNull();
    animation->m_vCurrentValue.SetNull();

    //do base-class initialization and return new object
    IFC_RETURN(ValidateAndInit(animation.detach(), ppObject));

    restoreTimingOwnerGuard.release();
    keyFrames.detach();

    return S_OK;
}

_Check_return_ HRESULT CObjectAnimationUsingKeyFrames::ValueAssign(
    AssignmentOperand destOperand,
    AssignmentOperand sourceOperand)
{
    ASSERT(sourceOperand != destOperand);

    CValue* sourceValue = GetInternalOperand(sourceOperand);
    CValue* destValue = GetInternalOperand(destOperand);
    CValue savedDest = std::move(*destValue);

    auto restoreDestValueGuard = wil::scope_exit([&]
    {
        // Revert the old value.
        *destValue = std::move(savedDest);
    });

    IFC_RETURN(destValue->CopyConverted(*sourceValue));

    restoreDestValueGuard.release();

    return S_OK;
}

_Check_return_ HRESULT CObjectAnimationUsingKeyFrames::ValueAssign(
    _Out_ CValue& destOperand,
    AssignmentOperand sourceOperand)
{
    CValue* sourceValue = GetInternalOperand(sourceOperand);
    IFC_RETURN(destOperand.CopyConverted(*sourceValue));

    return S_OK;
}

_Check_return_ HRESULT CObjectAnimationUsingKeyFrames::ValueAssign(
    AssignmentOperand destOperand,
    _In_ const CValue& sourceOperand)
{
    CValue* destValue = GetInternalOperand(destOperand);
    CValue savedDest = std::move(*destValue);

    auto restoreDestValueGuard = wil::scope_exit([&]
    {
        // Revert the old value.
        *destValue = std::move(savedDest);
    });

    IFC_RETURN(destValue->CopyConverted(sourceOperand));

    restoreDestValueGuard.release();

    return S_OK;
}

// Initializes the base value of the animation from either the
// animation target or from a handoff.
_Check_return_ HRESULT CObjectAnimationUsingKeyFrames::GetAnimationBaseValue()
{
    CValue value;

    // we should have a reference and an object, but if we don't have an actual target, keep current value
    ASSERT(GetTargetObjectWeakRef());
    auto targetObject = GetTargetObjectWeakRef().lock();
    if (targetObject)
    {
        IFC_RETURN(targetObject->GetValueByIndex(m_pTargetDependencyProperty->GetIndex(), &value));

        // Copy new base value.
        IFC_RETURN(ValueAssign(AssignmentOperand::BaseValue, value));
    }

    return S_OK;
}

// Generic interpolation method - this MUST be overridden by derived types
// to provide a way to blend to values of the given animation type
void CObjectAnimationUsingKeyFrames::InterpolateCurrentValue(XFLOAT rPercentEnd)
{
// Internal validation
    ASSERT(rPercentEnd <= 1.0f);
    ASSERT(rPercentEnd >= 0.0f);

// Compute actual interpolation
    // We only deal with discrete keyframe values so the percent should be either 1.0 or 0.0
    if (rPercentEnd == 0.0f)
    {
        // Clone From
        IFCFAILFAST(ValueAssign(AssignmentOperand::CurrentValue, AssignmentOperand::From));
    }
    else if (rPercentEnd == 1.0f)
    {
        // Clone To
        IFCFAILFAST(ValueAssign(AssignmentOperand::CurrentValue, AssignmentOperand::To));
    }
    else
    {
        ASSERT(rPercentEnd <= 1.0f || rPercentEnd >= 0.0f);
        IFCFAILFAST(E_UNEXPECTED);
    }
}

void CObjectAnimationUsingKeyFrames::PostInterpolateValues()
{
    CDependencyObject* pCurrentValue = m_vCurrentValue.AsObject(); // AsObject() does not AddRef()

    // We can't have the object associated with the keyframe and the target:
    // for multiple associated objects that's not an issue, but for the rest
    // we must break that original association.
    // This is safe since we are only holding the value in the keyframe, never
    // actually using it for anything in the visual tree.
    if (pCurrentValue && pCurrentValue->DoesAllowMultipleAssociation() == FALSE)
    {
        // Break association
        pCurrentValue->SetAssociated(false, nullptr);
        // Break parenting
        VERIFYHR(pCurrentValue->RemoveParent(this));
    }
}

KnownTypeIndex CObjectAnimationUsingKeyFrames::GetTypeIndex() const
{
    return DependencyObjectTraits<CObjectAnimationUsingKeyFrames>::Index;
}

CValue* CObjectAnimationUsingKeyFrames::GetInternalOperand(AssignmentOperand operand)
{
    switch (operand)
    {
        case AssignmentOperand::From:
            return &m_vFrom;

        case AssignmentOperand::To:
            return &m_vTo;

        case AssignmentOperand::By:
            return &m_vBy;

        case AssignmentOperand::BaseValue:
            return &m_vBaseValue;

        case AssignmentOperand::NonAnimatedBaseValue:
            return &m_vNonAnimatedBaseValue;

        case AssignmentOperand::CurrentValue:
            return &m_vCurrentValue;

        default:
            XCP_FAULT_ON_FAILURE(false);
            return nullptr;
    }
}
