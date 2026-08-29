// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// When a new PivotItem is selected, it slides in. Also, every item in that
// PivotItem that has the SlideInAnimationGroup attached property set to
// a value other than the default value will slide in.
// The slide in effect is staggered depending on the value of the attached
// property.
// This file contains the implementation of the pivot slide-in effect.

#include "precomp.h"

#include "Pivot_Partial.h"
#include "PivotItem_Partial.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

#pragma region PivotSlideInThemeAnimation
_Check_return_ HRESULT
PivotSlideInThemeAnimation::RuntimeClassInitialize(
    _In_ DOUBLE offsetX)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
    wrl::ComPtr<xaml_animation::IThemeAnimationBase> spDelegatingInnerInstance;

    if(!s_spThemeAnimationBaseFactory)
    {
        IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ThemeAnimationBase).Get(),
            &s_spThemeAnimationBaseFactory));
    }

    IFC(s_spThemeAnimationBaseFactory->CreateInstance(
                Private::iinspectable_cast(this),
                &spNonDelegatingInnerInspectable,
                &spDelegatingInnerInstance));

    IFC(SetComposableBasePointers(spNonDelegatingInnerInspectable.Get()));

    m_offsetX = offsetX;

Cleanup:
    RRETURN(hr);
}

_Check_return_ IFACEMETHODIMP
PivotSlideInThemeAnimation::CreateTimelinesInternal(
    _In_ BOOLEAN onlyGenerateSteadyState,
    _In_ wfc::IVector<xaml_animation::Timeline*> *pTimelineCollection)
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(onlyGenerateSteadyState);

    wrl::ComPtr<xaml_animation::IDoubleAnimationUsingKeyFrames> spDoubleAnimationUsingKeyFrames;
    wrl::ComPtr<xaml_animation::ITimeline> spDoubleAnimationUsingKeyFramesAsTL;
    wrl::ComPtr<wfc::IVector<xaml_animation::DoubleKeyFrame*>> spKeyFramesCollection;
    wrl::ComPtr<xaml_animation::IExponentialEase> spExponentialEase;
    wrl::ComPtr<xaml_animation::IEasingFunctionBase> spEasingFunction;

    const INT durationInMs = 700;
    const INT millisecondsTo100Nanoseconds = 10000; // 10^6 / 100

    // In Splash, the first half of the slide-in animation is linear and the other half is exponential.
    // When switching from the former to the latter, the output value should be equal to (m_offsetX * linearToExpValueFactor).
    // See the EaseInInterpolation implementation in Splash for more details on how linearToExpValueFactor is calculated.
    // Here is a summary:
    //      double weight = 10.0;
    //      double easeExponent = ln(1 / weight);
    //      double crossOverPointStep = 1.0 - (Math.Pow(weight, 0.99) - 1.0) / (weight - 1.0);
    //      crossOverPoint = crossOverPointStep * linearPortionOfAnimation / (0.01 * (1 - linearPortionOfAnimation) + crossOverPointStep * linearPortionOfAnimation);
    //      linearToExpValueFactor = 1 - crossOverPoint;

    const DOUBLE easeExponent = -2.302585093;     // This comes from  ln(1 / weight).
    const DOUBLE linearPortionOfAnimation = 0.5;
    const DOUBLE linearToExpValueFactor = 0.28335052129660765;

    typedef struct
    {
        BOOLEAN isLinear;
        INT64 keyTime;        // in milliseconds
        DOUBLE value;
    } KeyFrameInfo;

    KeyFrameInfo keyFrames[] = {
        { TRUE, 0, m_offsetX },
        { TRUE, static_cast<INT>(durationInMs * linearPortionOfAnimation), m_offsetX * linearToExpValueFactor},
        { FALSE, durationInMs, 0.0 },
    };

    // Creates exponential ease
    IFC(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_ExponentialEase).Get(),
        &spExponentialEase));
    IFC(spExponentialEase.As(&spEasingFunction));
    IFC(spEasingFunction->put_EasingMode(xaml_animation::EasingMode_EaseIn));
    IFC(spExponentialEase->put_Exponent(easeExponent));

    // Creates the DoubleAnimationUsingKeyFrames animation
    IFC(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_DoubleAnimationUsingKeyFrames).Get(),
        &spDoubleAnimationUsingKeyFrames));
    IFC(spDoubleAnimationUsingKeyFrames.As(&spDoubleAnimationUsingKeyFramesAsTL));
    IFC(spDoubleAnimationUsingKeyFrames->get_KeyFrames(&spKeyFramesCollection));

    // Populates the animation with the key frames.
    for (KeyFrameInfo& keyFrameInfo : keyFrames)
    {
        wrl::ComPtr<xaml_animation::IDoubleKeyFrame> spKeyFrame;

        if (keyFrameInfo.isLinear)
        {
            wrl::ComPtr<xaml_animation::ILinearDoubleKeyFrame> spLinearKeyFrame;

            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_LinearDoubleKeyFrame).Get(),
                &spLinearKeyFrame));
            IFC(spLinearKeyFrame.As(&spKeyFrame));
        }
        else
        {
            wrl::ComPtr<xaml_animation::IEasingDoubleKeyFrame> spEasingKeyFrame;

            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_EasingDoubleKeyFrame).Get(),
                &spEasingKeyFrame));
            IFC(spEasingKeyFrame.As(&spKeyFrame));

            IFC(spEasingKeyFrame->put_EasingFunction(spEasingFunction.Get()));
        }

        xaml_animation::KeyTime keyTime = {};
        keyTime.TimeSpan.Duration = keyFrameInfo.keyTime * millisecondsTo100Nanoseconds;

        IFC(spKeyFrame->put_KeyTime(keyTime));
        IFC(spKeyFrame->put_Value(keyFrameInfo.value));

        IFC(spKeyFramesCollection->Append(spKeyFrame.Get()));
    }

    // Sets the target property of the animation and add it to the timeline collection.
    IFC(Pivot::s_spStoryboardStatics->SetTargetProperty(spDoubleAnimationUsingKeyFramesAsTL.Get(), wrl_wrappers::HStringReference(L"(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateX").Get()));
    IFC(pTimelineCollection->Append(spDoubleAnimationUsingKeyFramesAsTL.Get()));

Cleanup:
    RRETURN(hr);
}
#pragma endregion

#pragma region PivotPropertyChange Functions
_Check_return_ HRESULT
Pivot::OnAttachedPropertyChanged(
    _In_ xaml::IDependencyObject* pSender,
    _In_ xaml::IDependencyPropertyChangedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml::IDependencyProperty> spPropertyInfo;

    IFCPTR(pArgs);
    IFC(pArgs->get_Property(&spPropertyInfo));

    if (spPropertyInfo.Get() == PivotFactory::s_SlideInAnimationGroupProperty)
    {
        wrl::ComPtr<IInspectable> spOldValue;
        wrl::ComPtr<IInspectable> spNewValue;
        wrl::ComPtr<xaml::IFrameworkElement> spSenderAsFE;
        BOOLEAN fromValidValue = FALSE;
        BOOLEAN toValidValue = FALSE;

        IFC(wrl::ComPtr<xaml::IDependencyObject>(pSender).As<xaml::IFrameworkElement>(&spSenderAsFE));
        IFC(pArgs->get_OldValue(&spOldValue));
        IFC(pArgs->get_NewValue(&spNewValue));

        IFC(PivotSlideInManager::IsSlideInAnimationGroupValueValid(spOldValue, &fromValidValue));
        IFC(PivotSlideInManager::IsSlideInAnimationGroupValueValid(spNewValue, &toValidValue));

        // If we go from a value that doesn't enable the slide-in effect (i.e. null, Default or an invalid value)
        // to a value that enable the slide-in effect (GroupOne, GroupTwo, ...), we will register the FrameworkElement.
        // If we go the other way, we will unregister it. If we go from values in the same category, nothing happen.
        // We will also call Prepare\Unprepare in case the FE is already loaded.

        if(fromValidValue != toValidValue)
        {
            if(toValidValue)
            {
                IFC(PivotSlideInManager::RegisterSlideInElement(spSenderAsFE));
                IFC(PivotSlideInManager::PrepareSlideInElement(spSenderAsFE));
            }
            else
            {
                IFC(PivotSlideInManager::UnprepareSlideInElement(spSenderAsFE));
                IFC(PivotSlideInManager::UnregisterSlideInElement(spSenderAsFE));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
Pivot::GetDefaultSlideInAnimationGroup(_Outptr_ IInspectable** ppValue)
{
    RRETURN(Private::ValueBoxer::CreateReference<PivotSlideInAnimationGroup>(PivotSlideInAnimationGroup_Default, ppValue));
}
#pragma endregion

#pragma region PivotSlideInManager
// Returns whether a value is valid or not for slide-in.
// For some invalid values, this will result in an error being returned
// (and, eventually, an exception getting raised).
_Check_return_ HRESULT
PivotSlideInManager::IsSlideInAnimationGroupValueValid(
    _In_ const wrl::ComPtr<IInspectable>& spValue,
    _Out_ BOOLEAN* pIsValid)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<wf::IReference<PivotSlideInAnimationGroup>> spBoxedSlideInAnimationGroup;

    *pIsValid = FALSE;
    IFCPTR(spValue);

    IFC(spValue.As(&spBoxedSlideInAnimationGroup));

    if(spBoxedSlideInAnimationGroup)
    {
        PivotSlideInAnimationGroup slideInAnimationGroupValue;
        IFC(spBoxedSlideInAnimationGroup->get_Value(&slideInAnimationGroupValue));

        // Invalid Enum values raise an error
        if (static_cast<INT32>(slideInAnimationGroupValue) < PivotSlideInAnimationGroup_Default
            || static_cast<INT32>(slideInAnimationGroupValue) > PivotSlideInAnimationGroup_GroupThree)
        {
            wrl_wrappers::HString errString;
            IFC(Private::FindStringResource(
                ERR_INVALID_ENUM_VALUE,
                errString.ReleaseAndGetAddressOf()));
            hr = E_INVALIDARG;
            RoOriginateError(hr, errString.Get());
            IFC(hr);
        }

        if(static_cast<INT32>(slideInAnimationGroupValue) > PivotSlideInAnimationGroup_Default)
        {
            *pIsValid = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotSlideInManager::RegisterSlideInElement(
        _In_ const wrl::ComPtr<IFrameworkElement>& spElement)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<PivotSlideInElementInformation> spElementInformation;

    IFC(wrl::MakeAndInitialize<PivotSlideInElementInformation>(&spElementInformation));

    IFC(spElement->add_Loaded(
        wrl::Callback<xaml::IRoutedEventHandler>(&PivotSlideInManager::OnSlideInElementLoaded).Get(),
        &spElementInformation->m_loadedToken));

    IFC(spElement->add_Unloaded(
        wrl::Callback<xaml::IRoutedEventHandler>(&PivotSlideInManager::OnSlideInElementUnloaded).Get(),
        &spElementInformation->m_unloadedToken));

    // Stores the slide-in animation information on the FE itself.
    IFC(PivotFactory::SetSlideInElementInformationStatic(spElement.Get(), spElementInformation.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotSlideInManager::UnregisterSlideInElement(
    _In_ const wrl::ComPtr<IFrameworkElement>& spElement)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<PivotSlideInElementInformation> spElementInformation;

    IFC(PivotFactory::GetSlideInElementInformationStatic(spElement.Get(), &spElementInformation));
    ASSERT(spElementInformation);

    IFC(spElement->remove_Loaded(spElementInformation->m_loadedToken));
    IFC(spElement->remove_Unloaded(spElementInformation->m_unloadedToken));

    spElementInformation->m_loadedToken.value = 0;
    spElementInformation->m_unloadedToken.value = 0;

    IFC(PivotFactory::SetSlideInElementInformationStatic(spElement.Get(), nullptr));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotSlideInManager::OnSlideInElementLoaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IFrameworkElement> spElement;

    IFC(wrl::ComPtr<IInspectable>(pSender).As(&spElement));
    IFC(PrepareSlideInElement(spElement));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotSlideInManager::OnSlideInElementUnloaded(
    _In_ IInspectable* pSender,
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IFrameworkElement> spElement;

    IFC(wrl::ComPtr<IInspectable>(pSender).As(&spElement));
    IFC(UnprepareSlideInElement(spElement));

Cleanup:
    RRETURN(hr);
}

// Retrieves information on the item that's going to slide-in and add it
// to an internal list in the parent PivotItem.
_Check_return_ HRESULT
PivotSlideInManager::PrepareSlideInElement(
    _In_ const wrl::ComPtr<xaml::IFrameworkElement>& spElement)
{
    HRESULT hr = S_OK;
    BOOLEAN isParentPivotItemFound = FALSE;
    wrl::ComPtr<xaml::IDependencyObject> spCurrent;

    IFC(spElement.As(&spCurrent));

    IFC(Pivot::EnsureStaticsAndFactories());

    // Walk the three and find the pivot item.
    // Register the element with the pivot item if the latter is inside a pivot control.
    while(!isParentPivotItemFound && spCurrent)
    {
        wrl::ComPtr<xaml::IDependencyObject> spParent;
        IFC(Pivot::s_spVisualTreeHelperStatics->GetParent(spCurrent.Get(), &spParent));

        if(spParent)
        {
            isParentPivotItemFound = Private::Is<IPivotItem>(spParent.Get());
        }

        spCurrent = spParent;
    }

    if(isParentPivotItemFound)
    {
        wrl::ComPtr<IPivot> spParentPivot;
        wrl::ComPtr<IPivotItem> spParentPivotItem;

        ASSERT(spCurrent);
        IFC(spCurrent.As(&spParentPivotItem));

        IFC(static_cast<PivotItem*>(spParentPivotItem.Get())->GetParent().As(&spParentPivot));

        if(spParentPivot)
        {
            wrl::ComPtr<PivotSlideInElementInformation> spElementInformation;

            IFC(PivotFactory::GetSlideInElementInformationStatic(spElement.Get(), &spElementInformation));
            ASSERT(spElementInformation);

            IFC(spParentPivotItem.AsWeak(&spElementInformation->m_wrParentPivotItem));

            IFC(static_cast<PivotItem*>(spParentPivotItem.Get())->RegisterSlideInElementNoRef(spElement.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotSlideInManager::UnprepareSlideInElement(
    _In_ const wrl::ComPtr<xaml::IFrameworkElement>& spElement)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<PivotSlideInElementInformation> spElementInformation;

    IFC(PivotFactory::GetSlideInElementInformationStatic(spElement.Get(), &spElementInformation));
    ASSERT(spElementInformation);

    if(spElementInformation->m_wrParentPivotItem)
    {
        wrl::ComPtr<IPivotItem> spParentPivotItem;

        IFC(spElementInformation->m_wrParentPivotItem.As(&spParentPivotItem));

        if(spParentPivotItem)
        {
            IFC(static_cast<PivotItem*>(spParentPivotItem.Get())->UnregisterSlideInElementNoRef(spElement.Get()));
        }

        spElementInformation->m_wrParentPivotItem.Reset();
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PivotSlideInManager::ApplySlideInAnimation(
    _In_ const wrl::ComPtr<IPivotItem>& spPivotItem,
    _In_ PivotAnimationDirection animationDirection)
{
    HRESULT hr = S_OK;
    const std::list<xaml::IFrameworkElement*>& slideInElementsList = static_cast<PivotItem*>(spPivotItem.Get())->GetRegisteredSlideInElementsNoRef();

    if(!slideInElementsList.empty())
    {
        wrl::ComPtr<xaml_animation::IStoryboard> spStoryboard;
        wrl::ComPtr<xaml_animation::ITimeline> spStoryboardAsTL;
        wrl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;
        const INT animationSide = (animationDirection == PivotAnimationDirection_Left) ? -1 : 1;

        IFC(Pivot::EnsureStaticsAndFactories());

        // Creates the storyboard.
        IFC(wf::ActivateInstance(
              wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_Animation_Storyboard).Get(),
              &spStoryboard));
        IFC(spStoryboard.As(&spStoryboardAsTL));
        IFC(spStoryboard->get_Children(spChildren.ReleaseAndGetAddressOf()));

        // For each registered slide-in element in the PivotItem, setup the slide-in animation according
        // to the SlideInAnimationGroup value.
        for(std::list<xaml::IFrameworkElement*>::const_iterator it = slideInElementsList.begin();
            it != slideInElementsList.end();
            ++it)
        {
            wrl::ComPtr<xaml::IFrameworkElement> spElement(*it);
            wrl::ComPtr<xaml::IDependencyObject> spElementAsDO;
            wrl::ComPtr<PivotSlideInElementInformation> spElementInformation;

            wrl::ComPtr<IInspectable> spInnerIgnored;
            wrl::ComPtr<PivotSlideInThemeAnimation> spSlideInAnimation;
            wrl::ComPtr<xaml_animation::ITimeline> spSlideInAnimationAsTL;
            wrl::ComPtr<xaml_animation::IThemeAnimationBase> spThemeAnimationBase;
            wrl::ComPtr<IInspectable> spThemeAnimationBaseInner;

            const DOUBLE slideInGapBetweenGroups = 40.0;

            // Gets the SlideInAnimationGroup value.
            PivotSlideInAnimationGroup slideInAnimationGroup = PivotSlideInAnimationGroup_Default;
            IFC(PivotFactory::GetSlideInAnimationGroupStatic(spElement.Get(), &slideInAnimationGroup));
            ASSERT(static_cast<INT>(slideInAnimationGroup) > 0);

            // Creates and initializes the slide-in theme animation.
            IFC(wrl::MakeAndInitialize<PivotSlideInThemeAnimation>(
                &spSlideInAnimation,
                /* offsetX = */ slideInGapBetweenGroups * static_cast<INT>(slideInAnimationGroup)* animationSide
                ));

            IFC(spSlideInAnimation.As(&spSlideInAnimationAsTL));

            IFC(spElement.As(&spElementAsDO));
            IFC(Pivot::s_spStoryboardStatics->SetTarget(spSlideInAnimationAsTL.Get(), spElementAsDO.Get()));
            IFC(spChildren->Append(spSlideInAnimationAsTL.Get()));
        }

        IFC(spStoryboard->Begin());
    }

Cleanup:
    RRETURN(hr);
}
#pragma endregion

} } } } XAML_ABI_NAMESPACE_END
