// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Timeline.h"
#include <CValue.h>
#include <EnumDefs.g.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <DOCollection.h>
#include <IATarget.h>
#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>

class CCoreServices;
class CKeyFrameCollection;
struct IFrameScheduler;
class CStoryboard;
class CDOCollection;
class CompositionAnimationConversionContext;
class CKeyFrame;

// WPF defaults animations to 0:0:1, including Automatic, and Xaml will do the same.
#define NULL_DURATION_DEFAULT 1.0f

class CAnimation : public CTimeline
{
public:
    enum AnimationValueOperation
    {
        AnimationValueSet,
        AnimationValueClear
    };

    bool IsAnimationValueNull(
        _In_ const CDependencyProperty *pdp,
        _In_ const CValue& value)
    {
        return (( value.GetType() == valueNull ||
                ( value.GetType() == valueObject && value.AsObject() == nullptr ))
                &&
                ( pdp == m_pDPTo ||
                  pdp == m_pDPBy ||
                  pdp == m_pDPFrom
                ));
    }

protected:
    CAnimation(_In_ CCoreServices *pCore);
    ~CAnimation() override;

public:
    DECLARE_CREATE_RETURN(CAnimation, E_UNEXPECTED);

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) final;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CAnimation>::Index;
    }

    _Check_return_ HRESULT GetValue(
        _In_ const CDependencyProperty *pdp,
        _Inout_ CValue *pValue) final;

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) final;

    _Check_return_ HRESULT UpdateAnimation(
        _In_ const ComputeStateParams &parentParams,
        _Inout_ ComputeStateParams &myParams,
        XDOUBLE beginTime,
        DirectUI::DurationType durationType,
        XFLOAT durationValue,
        _In_ const COptionalDouble &expirationTime,
        _Out_ bool *pIsIndependentAnimation
        ) final;

    void GetNaturalDuration(
        _Out_ DirectUI::DurationType* pDurationType,
        _Out_ XFLOAT* pDurationValue
        ) override;

    _Check_return_ HRESULT FinalizeIteration() final;

    _Check_return_ HRESULT OnRemoveFromTimeManager() override;
    _Check_return_ HRESULT OnBegin() override;

    _Check_return_ HRESULT ReadBaseValuesFromTargetOrHandoff(
        _In_opt_ CAnimation *pPrecedingAnimation,
        _In_ const xref::weakref_ptr<CDependencyObject>& pPreviouslyAnimatedDOWeakRef,
        _In_opt_ const KnownPropertyIndex *pPreviouslyAnimatedPropertyIndex
        );

    _Check_return_ HRESULT RegisterAnimationOnTarget();

    void TakeControlOfTarget(_In_ CAnimation *pPreviousAnimation);
    void ReleaseControlOfTarget();

    bool IsAnimation() final { return true; }

    _Check_return_ HRESULT GetAnimationTargets(
        _Out_ xref::weakref_ptr<CDependencyObject>* ppTargetRef,
        _Outptr_ const CDependencyProperty** ppProperty);

    CDOCollection *GetKeyFrameCollection();

    // Beware: This will return TRUE for some animations in a RD which might be incorrect.
    _Check_return_ HRESULT IsIndependentAnimation(
        _In_ CDependencyObject* pTargetObject,
        _Out_ bool* pIsIndependentAnimation)
    {
        // calls into method that has no sideeffects (because of not passing IATargets)
        RRETURN(FindIndependentAnimationTargets(pTargetObject, NULL, NULL, pIsIndependentAnimation));
    }

    _Check_return_ HRESULT FireCompletedEvent() final;

    _Check_return_ HRESULT FindIndependentAnimationTargets(
        _In_ CDependencyObject *pTargetObject,
        _In_opt_ CDependencyObject *pSender,
        _Inout_opt_ IATargetVector *pIATargets,
        _Out_ bool *pIsIndependentAnimation
        );

    bool ShouldDiscardDCompAnimationOnDetach() override;

    void ReleaseDCompResources() override;

    static void GetNaturalDurationFromKeyFrames(
        const CDOCollection::storage_type& sortedKeyFrames,
        _Out_ DirectUI::DurationType* pDurationType,
        _Out_ XFLOAT* pDurationValue
        );

    static KnownPropertyIndex MapToDCompAnimationProperty(KnownPropertyIndex xamlAnimatedProperty);

    bool IsZeroDuration() const;

protected:
    enum class AssignmentOperand
    {
        From,
        To,
        By,
        BaseValue,
        NonAnimatedBaseValue,
        CurrentValue
    };

    _Check_return_ HRESULT ResolveLocalTarget(
        _In_ CCoreServices *pCoreServices,
        _In_opt_ CTimeline *pParentTimeline) override;

    virtual void InterpolateCurrentValue(XFLOAT rPercentEnd) = 0;
    // Used by ObjectAnimationUsingKeyFrames to remove parents from DOs. Not a const method.
    virtual void PostInterpolateValues() { }

    virtual void ComputeToUsingFromAndBy() = 0;

    virtual _Check_return_ HRESULT ValueAssign(
        AssignmentOperand destOperand,
        AssignmentOperand sourceOperand) = 0;

    virtual _Check_return_ HRESULT ValueAssign(
        _Out_ CValue& destOperand,
        AssignmentOperand sourceOperand) = 0;

    virtual _Check_return_ HRESULT ValueAssign(
        AssignmentOperand destOperand,
        _In_ const CValue& sourceOperand) = 0;

    virtual _Check_return_ HRESULT GetAnimationBaseValue() = 0;

    _Check_return_ HRESULT CheckElementUsingAnimatedTarget(
        _In_ CDependencyObject *pTargetObject,
        _Inout_ IATargetVector *pIATargets,
        _Inout_ bool *pIsIndependentAnimation
        );

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext) final;

    virtual _Check_return_ CompositionAnimationConversionResult MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext);

    void UpdateWUCAnimationTelemetryString(_In_ CompositionAnimationConversionContext* myContext);

#pragma endregion

private:
    _Check_return_ HRESULT ShouldTickOnUIThread(
        bool isStoryboardPaused,
        bool cannotConvertToCompositionAnimation,
        _Out_ bool *pTickOnUIThread,
        _Out_ bool *pIsIndependentAnimation);

    virtual _Check_return_ HRESULT FindIndependentAnimationTargetsRecursive(
        _In_ CDependencyObject *pTargetObject,
        _In_opt_ CDependencyObject *pSender,
        _Inout_opt_ IATargetVector *pIATargets,
        _Out_ bool *pIsIndependentAnimation
        );

    _Check_return_ HRESULT UpdateAnimationUsingKeyFrames(
        bool isIndependentAnimation
        );

    _Check_return_ HRESULT RequestNextTickForKeyFrames(
        _In_ const ComputeStateParams &myParams,
        DirectUI::DurationType durationType,
        float durationValue
        );

    _Check_return_ HRESULT DoAnimationValueOperation(
        _In_ CValue& value,
        AnimationValueOperation operation,
        bool isValueChangeIndependent
        );

    bool IsAllowedDependentAnimation();

    void TraceAnimation(_In_ const AnimationValueOperation &operation, _In_ CDependencyObject * pDO, _In_ bool startOfTrace);

    void TraceTickPausedAnimationHelper();

public:
    // C<*>UsingKeyFrames properties
    CKeyFrameCollection *m_pKeyFrames;  // Mutually exclusive with use of m_pBy.

    // Easing function for the animation.
    // Native easing functions all derive from CEasingFunctionBase. Managed ones must implement IEasingFunction.
    CDependencyObject *m_pEasingFunction;

    // OM DP cached lookups
    const CDependencyProperty *m_pDPTo;
    const CDependencyProperty *m_pDPFrom;
    const CDependencyProperty *m_pDPBy;
    const CDependencyProperty *m_pDPValue;

    static const long long s_minWUCKeyFrameAnimationDurationInTicks;
    static const float s_maxWUCKeyFrameAnimationDurationInSeconds;
    static const UINT64 s_maxWUCKeyFrameAnimationDurationInTicks;

#pragma region ::Windows::UI::Composition

    wrl::ComPtr<WUComp::IKeyFrameAnimation> m_spWUCAnimation;

#pragma endregion

    bool m_enableDependentAnimation; // Cannot be a bit, since it is set by property system

    bool m_fUsesKeyFrames : 1;
    bool m_fFiredDependentAnimationWarning : 1;
    bool m_hasControlOfTarget : 1;    // Whether ticking this animation will affect the value of the object+target

    bool m_hasHandoff : 1;
    bool m_isToAnimation : 1;  // Used for DComp animation handoff; the other option is a By animation

    // Considers the durations of the ancestor timelines as well. Used to determine whether this animation can hand off
    // to another. Zero-duration animations aren't handled with DComp animations, and should not hand off to another
    // DComp animation.
    bool m_isZeroDuration : 1;

    bool m_fAnimationApplied : 1;
};
