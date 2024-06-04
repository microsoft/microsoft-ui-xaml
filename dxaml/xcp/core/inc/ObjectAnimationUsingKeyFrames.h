// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Animation.h"

class CCoreServices;

// Object created for <ObjectAnimationUsingKeyFrames> tag
class CObjectAnimationUsingKeyFrames final : public CAnimation
{
private:
    CObjectAnimationUsingKeyFrames(_In_ CCoreServices *pCore)
        : CAnimation(pCore)
    {
        m_fUsesKeyFrames = true;
    }

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override;

    bool IsInterestingForAnimationTracking() override { return false; }

protected:
    void InterpolateCurrentValue(XFLOAT rPercentEnd) final;
    void PostInterpolateValues() final;
    void ComputeToUsingFromAndBy() final { } ;

    _Check_return_ HRESULT ValueAssign(
        AssignmentOperand destOperand,
        AssignmentOperand sourceOperand) final;

    _Check_return_ HRESULT ValueAssign(
        _Out_ CValue& destOperand,
        AssignmentOperand sourceOperand) final;

    _Check_return_ HRESULT ValueAssign(
        AssignmentOperand destOperand,
        _In_ const CValue& sourceOperand) final;

    _Check_return_ HRESULT GetAnimationBaseValue() override;

protected:
    // Returns reference to class member variable mapping to operand.
    CValue* GetInternalOperand(AssignmentOperand operand);

    CValue m_vFrom;
    CValue m_vTo;
    CValue m_vBy;
    CValue m_vBaseValue;
    CValue m_vNonAnimatedBaseValue;
    CValue m_vCurrentValue;
};
