// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Animation.h"

class CCoreServices;
class CompositionAnimationConversionContext;
class CDependencyProperty;

// Object created for <DoubleAnimation> tag
class CDoubleAnimation : public CAnimation
{
protected:
    CDoubleAnimation(_In_ CCoreServices *pCore)
        : CAnimation(pCore)
    {
        m_pDPTo = GetPropertyByIndexInline(KnownPropertyIndex::DoubleAnimation_To);
        m_pDPFrom = GetPropertyByIndexInline(KnownPropertyIndex::DoubleAnimation_From);
        m_pDPBy = GetPropertyByIndexInline(KnownPropertyIndex::DoubleAnimation_By);

        ASSERT( m_pDPTo && m_pDPFrom && m_pDPBy );
    }

public:
    CDoubleAnimation();

    ~CDoubleAnimation() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override;

    void AttachDCompAnimations() override;

    void AttachDCompAnimationInstancesToTarget() override;
    void DetachDCompAnimationInstancesFromTarget() override;

    void SeekDCompAnimationInstances(double globalSeekTime) override;
    void ResolvePendingDCompAnimationOperations() override;

    // Used for isolated testing
    void SetTargetObjectWeakRef(_In_ const xref::weakref_ptr<CDependencyObject>& value) { m_targetObjectWeakRef = value; }
    // Used for isolated testing
    void SetTargetDependencyProperty(_In_ CDependencyProperty* value) { m_pTargetDependencyProperty = value; }

    void PauseDCompAnimationsOnSuspend() override;
    void ResumeDCompAnimationsOnResume() override;

    void ReleaseDCompResources() final;

protected:
#pragma region DirectComposition

    void ApplyDCompAnimationInstanceToTarget(_In_opt_ IUnknown* pDCompAnimation);

#pragma endregion

    void InterpolateCurrentValue(XFLOAT rPercentEnd) final;

    void ComputeToUsingFromAndBy() final;

    _Check_return_ HRESULT ValueAssign(
        AssignmentOperand destOperand,
        AssignmentOperand sourceOperand) final;

    _Check_return_ HRESULT ValueAssign(
        _Out_ CValue& destOperand,
        AssignmentOperand sourceOperand) final;

    _Check_return_ HRESULT ValueAssign(
        AssignmentOperand destOperand,
        _In_ const CValue& sourceOperand) final;

    _Check_return_ HRESULT GetAnimationBaseValue() final;

private:
    _Check_return_ HRESULT FindIndependentAnimationTargetsRecursive(
        _In_ CDependencyObject *pTargetObject,
        _In_opt_ CDependencyObject *pSender,
        _Inout_opt_ IATargetVector *pIATargets,
        _Out_ bool *pIsIndependentAnimation
        ) override;

protected:
    // Returns reference to class member variable mapping to operand.
    float* GetInternalOperand(AssignmentOperand operand);

#pragma region ::Windows::UI::Composition

    _Check_return_ CompositionAnimationConversionResult MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext) override;

#pragma endregion

public:
    float m_vFrom = 0.0f;
    float m_vTo = 0.0f;
    float m_vBy = 0.0f;
    float m_vBaseValue = 0.0f;
    float m_vNonAnimatedBaseValue = 0.0f;
    float m_vCurrentValue = 0.0f;
};
