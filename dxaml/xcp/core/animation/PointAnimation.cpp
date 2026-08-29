// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PointAnimation.h"
#include "PointKeyFrame.h"
#include <CValueConvert.h>

using namespace DirectUI;

_Check_return_ HRESULT CPointAnimation::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;

// Create type
    CPointAnimation * pAnimation = new CPointAnimation(pCreate->m_pCore);

//do base-class initialization and return new object
    IFC(ValidateAndInit(pAnimation, ppObject));

// Cleanup
    pAnimation = NULL;

Cleanup:
    ReleaseInterface(pAnimation);
    RRETURN(hr);
}

// Generic interpolation method - this MUST be overridden by derived types
// to provide a way to blend to values of the given animation type
void CPointAnimation::InterpolateCurrentValue(XFLOAT rPercentEnd)
{
// Easing functions can give an unnormalized value outside of the range 0 to 1 so
// don't validate that rPercentEnd is from 0 to 1 but handle the cases where is is.

    XFLOAT rPercentStart = 1.0f - rPercentEnd;

// Compute actual interpolation
    m_ptCurrentValue.x = m_ptFrom.x * rPercentStart + rPercentEnd * m_ptTo.x;
    m_ptCurrentValue.y = m_ptFrom.y * rPercentStart + rPercentEnd * m_ptTo.y;
}

// Computes the m_pTo field from the m_pBy and m_pFrom fields using a type-specific addition;
void CPointAnimation::ComputeToUsingFromAndBy()
{
    // Compute type-specific add
    m_ptTo.x = m_ptFrom.x + m_ptBy.x;
    m_ptTo.y = m_ptFrom.y + m_ptBy.y;
}

_Check_return_ HRESULT CPointAnimation::ValueAssign(
    AssignmentOperand destOperand,
    AssignmentOperand sourceOperand)
{
    ASSERT(sourceOperand != destOperand);

    XPOINTF* sourceValue = GetInternalOperand(sourceOperand);
    XPOINTF* destValue = GetInternalOperand(destOperand);
    *destValue = *sourceValue;

    return S_OK;
}

_Check_return_ HRESULT CPointAnimation::ValueAssign(
    _Out_ CValue& destOperand,
    AssignmentOperand sourceOperand)
{
    XPOINTF* sourceValue = GetInternalOperand(sourceOperand);
    XPOINTF* newPoint = new XPOINTF();
    *newPoint = *sourceValue;
    destOperand.SetPoint(newPoint);

    return S_OK;
}

_Check_return_ HRESULT CPointAnimation::ValueAssign(
    AssignmentOperand destOperand,
    _In_ const CValue& sourceOperand)
{
    XPOINTF* destValue = GetInternalOperand(destOperand);

    if (sourceOperand.AsPoint() != nullptr)
    {
        *destValue = *sourceOperand.AsPoint();
    }
    else
    {
        *destValue = XPOINTF();
    }

    return S_OK;
}

_Check_return_ HRESULT CPointAnimation::GetAnimationBaseValue()
{
    // we should have a reference, but if we don't have an actual target, keep current value

    ASSERT(GetTargetObjectWeakRef());

    auto targetObject = GetTargetObjectWeakRef().lock();

    if (targetObject)
    {
        CValue value;
        XPOINTF point = {};

        value.WrapPoint(&point);

        IFC_RETURN(targetObject->GetValueByIndex(m_pTargetDependencyProperty->GetIndex(), &value));

        CValue rawValue;
        IFC_RETURN(CValueConvert::EnsurePropertyValueUnboxed(value, rawValue));

        if (rawValue.GetType() == valuePoint)
        {
            m_ptBaseValue = *rawValue.AsPoint();
        }
        else if (rawValue.GetType() == valueObject)
        {
            // We are getting one of the DO animation properties ( To/From/By )
            // which are not stored as value types.
            // GetValue may return null if we are trying to query an animation property
            // that has never been set - if that's the case then both it and the basevalue
            // have been initialized to the same default in the animation constructor.

            CDependencyObject* rawValueAsObject = rawValue.AsObject();

            if (rawValueAsObject)
            {
                m_ptBaseValue = static_cast<CPoint*>(rawValueAsObject)->m_pt;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CPointAnimationUsingKeyFrames::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    // Create type
    xref_ptr<CPointAnimationUsingKeyFrames> animation;
    animation.init(new CPointAnimationUsingKeyFrames(pCreate->m_pCore));

    // Create internal members
    xref_ptr<CPointKeyFrameCollection> keyFrames;
    IFC_RETURN(CPointKeyFrameCollection::Create(reinterpret_cast<CDependencyObject **>(keyFrames.ReleaseAndGetAddressOf()), pCreate));
    IFC_RETURN(animation->SetValueByKnownIndex(KnownPropertyIndex::PointAnimationUsingKeyFrames_KeyFrames, keyFrames.get()));

    animation->m_pDPValue = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::PointKeyFrame_Value);
    ASSERT(animation->m_pDPValue != nullptr);

    //do base-class initialization and return new object
    IFC_RETURN(ValidateAndInit(animation.detach(), ppObject));

    return S_OK;
}

XPOINTF* CPointAnimation::GetInternalOperand(AssignmentOperand operand)
{
    switch (operand)
    {
        case AssignmentOperand::From:
            return &m_ptFrom;

        case AssignmentOperand::To:
            return &m_ptTo;

        case AssignmentOperand::By:
            return &m_ptBy;

        case AssignmentOperand::BaseValue:
            return &m_ptBaseValue;

        case AssignmentOperand::NonAnimatedBaseValue:
            return &m_ptNonAnimatedBaseValue;

        case AssignmentOperand::CurrentValue:
            return &m_ptCurrentValue;

        default:
            XCP_FAULT_ON_FAILURE(false);
            return nullptr;
    }
}
