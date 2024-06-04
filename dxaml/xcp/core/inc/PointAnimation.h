// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Animation.h"

class CCoreServices;

// Object created for <PointAnimation> tag
class CPointAnimation : public CAnimation
{
protected:
    CPointAnimation(_In_ CCoreServices *pCore)
        : CAnimation(pCore)
    {
        m_pDPTo = GetPropertyByIndexInline(KnownPropertyIndex::PointAnimation_To);
        m_pDPFrom = GetPropertyByIndexInline(KnownPropertyIndex::PointAnimation_From);
        m_pDPBy = GetPropertyByIndexInline(KnownPropertyIndex::PointAnimation_By);

        ASSERT( m_pDPTo && m_pDPFrom && m_pDPBy );
    }

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointAnimation>::Index;
    }

protected:
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

    // Returns reference to class member variable mapping to operand.
    XPOINTF* GetInternalOperand(AssignmentOperand operand);

public:
    XPOINTF m_ptFrom = {};
    XPOINTF m_ptTo = {};
    XPOINTF m_ptBy = {};
    XPOINTF m_ptBaseValue = {};
    XPOINTF m_ptNonAnimatedBaseValue = {};
    XPOINTF m_ptCurrentValue = {};
};

// Object created for <PointAnimationUsingKeyFrames> tag
class CPointAnimationUsingKeyFrames final : public CPointAnimation
{
private:
    CPointAnimationUsingKeyFrames(_In_ CCoreServices *pCore)
        : CPointAnimation(pCore)
    {
        m_fUsesKeyFrames = true;
    }

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointAnimationUsingKeyFrames>::Index;
    }
};

