// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DoubleAnimationUsingKeyFrames.h"

class CCoreServices;

// Object created for each component of the PointerDownThemeAnimation:
// rotation in the X axis, rotation in the Y axis, and scale.
class CPointerAnimationUsingKeyFrames : public CDoubleAnimationUsingKeyFrames
{
private:
    CPointerAnimationUsingKeyFrames(_In_ CCoreServices *pCore)
        : CDoubleAnimationUsingKeyFrames(pCore)
        , m_strRelativeToObjectName()
        , m_pointerSource(DirectUI::PointerDirection::PointerDirection_XAxis)
    {
        m_enableDependentAnimation = true;
        m_haveCachedInputDeviceType = FALSE;
        m_inputDeviceType = DirectUI::InputDeviceType::None;
        m_haveCachedPrimaryPointerPosition = false;
        m_cachedPrimaryPointerPosition = {0, 0};
    }

    ~CPointerAnimationUsingKeyFrames() override = default;

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointerAnimationUsingKeyFrames>::Index;
    }

    void GetNaturalDuration(_Out_ DirectUI::DurationType *pDurationType, _Out_ XFLOAT *prDurationValue) final;

    bool IsDurationForProgressDifferent() const override { return true; }

    void GetDurationForProgress(_Out_ DirectUI::DurationType *pDurationType, _Out_ XFLOAT *prDurationValue) override;

    _Check_return_ HRESULT OnBegin() final;

    _Check_return_ HRESULT SetRelativeToObject(
        _In_ CDependencyObject *pRelativeToObject);

    _Check_return_ HRESULT SetRelativeToObjectName(
        _In_ const xstring_ptr_view& strRelativeToObjectName);

    bool IsFinite() final;

protected:
    _Check_return_ HRESULT ResolveLocalTarget(
        _In_ CCoreServices *pCoreServices,
        _In_opt_ CTimeline *pParentTimeline) final;

    bool CanRequestTicksWhileActive() final;

private:
    void ComputeLocalProgressAndTime(
        XDOUBLE rBeginTime,
        XDOUBLE rParentTime,
        DirectUI::DurationType durationType,
        XFLOAT rDurationValue,
        _In_ COptionalDouble *poptExpirationTime,
        bool hadIndependentAnimationLastTick,
        _Out_ bool *pIsInReverseSegment
        ) override;

    _Check_return_ HRESULT FindIndependentAnimationTargetsRecursive(
        _In_ CDependencyObject *pTargetObject,
        _In_opt_ CDependencyObject *pSender,
        _Inout_opt_ IATargetVector *pIATargets,
        _Out_ bool *pIsIndependentAnimation
        ) final;

public:
    xstring_ptr m_strRelativeToObjectName;
    xref::weakref_ptr<CDependencyObject> m_pRelativeToObject;

    // A value indicating whether the source is the x- or y-axis of the pointer, or both.
    DirectUI::PointerDirection m_pointerSource;

private:
    XPOINTF m_cachedPrimaryPointerPosition;
    DirectUI::InputDeviceType m_inputDeviceType;
    bool m_haveCachedPrimaryPointerPosition;
    bool m_haveCachedInputDeviceType;
};