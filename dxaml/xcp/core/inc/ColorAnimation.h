// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Animation.h"

class CCoreServices;

// Object created for <ColorAnimation> tag
class CColorAnimation : public CAnimation
{
public:
    CColorAnimation()
        : CAnimation(nullptr)
    {
    }

    ~CColorAnimation() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CColorAnimation>::Index;
    }

    void AttachDCompAnimations() override;

    void AttachDCompAnimationInstancesToTarget() override;
    void DetachDCompAnimationInstancesFromTarget() override;

    void SeekDCompAnimationInstances(double globalSeekTime) override;
    void ResolvePendingDCompAnimationOperations() override;

    void PauseDCompAnimationsOnSuspend() override;
    void ResumeDCompAnimationsOnResume() override;

    void ReleaseDCompResources() final;

    // Used for isolated testing
    void SetTargetObjectWeakRef(_In_ const xref::weakref_ptr<CDependencyObject>& value) { m_targetObjectWeakRef = value; }
    // Used for isolated testing
    void SetTargetDependencyProperty(_In_ CDependencyProperty* value) { m_pTargetDependencyProperty = value; }

protected:
    CColorAnimation(_In_ CCoreServices *pCore)
        : CAnimation(pCore)
    {
        m_pDPTo = GetPropertyByIndexInline(KnownPropertyIndex::ColorAnimation_To);
        m_pDPFrom = GetPropertyByIndexInline(KnownPropertyIndex::ColorAnimation_From);
        m_pDPBy = GetPropertyByIndexInline(KnownPropertyIndex::ColorAnimation_By);
    }

    // For WUC animations, which uses a single color animation
    void ApplyDCompAnimationInstanceToTarget(_In_opt_ IUnknown* pDCompAnimation);

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

#pragma region ::Windows::UI::Composition

    _Check_return_ CompositionAnimationConversionResult MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext) override;

#pragma endregion

private:
    _Check_return_ HRESULT FindIndependentAnimationTargetsRecursive(
        _In_ CDependencyObject *pTargetObject,
        _In_opt_ CDependencyObject *pSender,
        _Inout_opt_ IATargetVector *pIATargets,
        _Out_ bool *pIsIndependentAnimation
        ) final;

    _Check_return_ HRESULT FindInheritedIndependentAnimationTargets(
        _In_ CUIElement *pElement,
        _In_ const CBrush *pAnimatedBrush,
        _Inout_opt_ IATargetVector *pIATargets
        );

protected:
    // Returns reference to class member variable mapping to operand.
    XUINT32* GetInternalOperand(AssignmentOperand operand);

public:
    XUINT32 m_vFrom = 0;
    XUINT32 m_vTo = 0;
    XUINT32 m_vBy = 0;
    XUINT32 m_vBaseValue = 0;
    XUINT32 m_vNonAnimatedBaseValue = 0;
    XUINT32 m_vCurrentValue = 0;
};
