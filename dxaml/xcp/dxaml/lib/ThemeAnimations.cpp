// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "vsanimation.h"
#include "Timeline.g.h"
#include "FadeInThemeAnimation.g.h"
#include "FadeOutThemeAnimation.g.h"
#include "PopInThemeAnimation.g.h"
#include "PopOutThemeAnimation.g.h"
#include "PointerDownThemeAnimation.g.h"
#include "PlaneProjection.g.h"
#include "CompositeTransform.g.h"
#include "TranslateTransform.g.h"
#include "TransformGroup.g.h"
#include "PointerUpThemeAnimation.g.h"
#include "SplitOpenThemeAnimation.g.h"
#include "SplitCloseThemeAnimation.g.h"
#include "SwipeHintThemeAnimation.g.h"
#include "SwipeBackThemeAnimation.g.h"
#include "RepositionThemeAnimation.g.h"
#include "DragItemThemeAnimation.g.h"
#include "DropTargetItemThemeAnimation.g.h"
#include "DragOverThemeAnimation.g.h"
#include "DrillInThemeAnimation.g.h"
#include "DrillOutThemeAnimation.g.h"
#include "ToolTip.g.h"
#include "Storyboard.g.h"
#include "PointerAnimationUsingKeyFrames.h"
#include "TransformCollection.g.h"
#include "ThemeAnimationsHelper.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

std::default_random_engine DropTargetItemThemeAnimation::m_generator;

_Check_return_ HRESULT ThemeAnimation_GetStoryboardTargetName(
        _In_ Timeline *pThemeAnimation,
        _Out_ HSTRING *pStoryboardTargetName)
{
    HRESULT hr = S_OK;
    IActivationFactory* pActivationFactory = NULL;
    xaml_animation::IStoryboardStatics* pStoryboardStatics = NULL;

    *pStoryboardTargetName = NULL;

    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::StoryboardFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pStoryboardStatics, pActivationFactory));

    IFC(pStoryboardStatics->GetTargetName(pThemeAnimation, pStoryboardTargetName));

Cleanup:
    ReleaseInterface(pStoryboardStatics);
    ReleaseInterface(pActivationFactory);
    RRETURN(hr);
}

_Check_return_ HRESULT FadeInThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point offset = {0,0};
    wrl_wrappers::HString strTargetName;

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_FADEIN, TA_FADEIN_SHOWN, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, offset, offset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FadeOutThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point offset = {0,0};
    wrl_wrappers::HString strTargetName;

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_FADEOUT, TA_FADEOUT_HIDDEN, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, offset, offset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PopInThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point destinationOffset = {0,0};
    wf::Point startOffset;
    wrl_wrappers::HString strTargetName;
    DOUBLE x;
    DOUBLE y;
    IFC(get_FromHorizontalOffset(&x));
    IFC(get_FromVerticalOffset(&y));

    startOffset.X = (FLOAT)x;
    startOffset.Y = (FLOAT)y;

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_SHOWPOPUP, TA_SHOWPOPUP_TARGET, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PopOutThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point startOffset = {0,0};
    wf::Point destinationOffset = {0, 0};
    wrl_wrappers::HString strTargetName;

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_HIDEPOPUP, TA_HIDEPOPUP_TARGET, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PointerDownThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    wf::Point nullPoint = {0,0};
    wrl_wrappers::HString strTargetName;
    ctl::ComPtr<xaml::IDependencyObject> spTarget = m_tpAnimationTarget.AsOrNull<xaml::IDependencyObject>();

    if (!spTarget)
    {
        IFC_RETURN(get_TargetName(strTargetName.GetAddressOf()));

        if (!strTargetName.Get())
        {
            // If the value of the TargetName property of the ThemeAnimation is null
            // that means that we will be fall back and use the value of the attached
            // property Storyboard.TargetName instead.
            IFC_RETURN(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
        }
    }

    // The tilt animtion conflicts with the way we register for gesture processing.
    // In ElementGestureTracker::ProcessPointerInformation, when we get a POINTER_FLAG_DOWN,
    // we call ::AddPointerInteractionContext if and only if the target element is visually located
    // under the pointer. In the case of something that tilts though, it is possible that the animation
    // pushes this element away from your finger, so this condition is not met anymore.
    // At the end, under these circumstances we don't get registered for gesture processing and all input
    // events like Tapped or Click get blocked.
    // To fix this, we will add a 10 millisecond delay before we start the animation.
    // This won't have any noticeable visual repercussion, but still give us time to register for gestures.
    static const INT64 beginTime = 10;
    wrl_wrappers::HStringReference strRotateXPropertyName(L"(UIElement.Projection).(PlaneProjection.RotationX)");
    wrl_wrappers::HStringReference strRotateYPropertyName(L"(UIElement.Projection).(PlaneProjection.RotationY)");
    wrl_wrappers::HStringReference strGlobalOffsetZPropertyName(L"(UIElement.Projection).(PlaneProjection.GlobalOffsetZ)");
    wrl_wrappers::HStringReference strScaleXPropertyName(L"(UIElement.RenderTransform).(ScaleTransform.ScaleX)");
    wrl_wrappers::HStringReference strScaleYPropertyName(L"(UIElement.RenderTransform).(ScaleTransform.ScaleY)");
    CUIElement *pUIElementNoRef = nullptr;
    UIElement *pUIENoRef = nullptr;
    xref_ptr<CDependencyObject> spCoreDO;
    ctl::ComPtr<DependencyObject> spDO;
    ctl::ComPtr<PlaneProjection> spProjection;
    ctl::ComPtr<CompositeTransform> spCompositeTransform;
    ctl::ComPtr<TransformGroup> spTransformGroup;
    float maxAngleXAxis = 0.0f;
    float maxAngleYAxis = 0.0f;

    if (spTarget)
    {
        IFC_RETURN(spTarget.As(&spDO));
        spCoreDO = spDO->GetHandle();
    }
    else
    {
        xstring_ptr strName;

        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(strTargetName.Get(), &strName));

        spCoreDO = CoreImports::Timeline_ResolveName(
            checked_cast<CTimeline>(GetHandle()),
            strName);

        if (spCoreDO)
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spCoreDO, &spDO));
        }
        else if (!strName.IsNullOrEmpty())
        {
            HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
            xephemeral_string_ptr parameters[1];

            strName.Demote(&parameters[0]);

            // Don't fail fast - VSM can catch this error and handle it
            IGNOREHR(GetHandle()->SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_SB_BEGIN_INVALID_TARGET, 1, parameters));
            IFC_RETURN(hrToOriginate);
        }
        else
        {
            HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
            // Don't fail fast - VSM can catch this error and handle it
            IGNOREHR(GetHandle()->SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_SB_BEGIN_NO_TARGET));
            IFC_RETURN(hrToOriginate);
        }
    }

    pUIElementNoRef = static_cast<CUIElement*>(spCoreDO.get());
    pUIENoRef = spDO.Cast<UIElement>();

    IFC_RETURN(ctl::make<PlaneProjection>(&spProjection));
    IFC_RETURN(ctl::make<CompositeTransform>(&spCompositeTransform));

    // Several changes in Tilt Animation behavior starting with Threshold (aka Windows 10):
    // 1) The amount of tilt angle was tweaked.
    // 2) Instead of translating back in the Z direction, a Scale Transform is applied. The scale factor is
    //    clamped between 97% and 100%, depending on how far from the center the pointer landed (100% at
    //    the edges, 97% at the center).
    // 3) The "simulated global camera" effect is disabled (see CPointerAnimationUsingKeyFrames::OnBegin()).
    // 4) The PointerUpThemeAnimation has different hold/duration (see PointerUpThemeAnimation::CreateTimelines()).
    // 5) The "scrub" effect of continuing to tilt the control as you drag the pointer is disabled
    //    (see CPointerAnimationUsingKeyFrames::ComputeLocalProgressAndTime()).
    static const float maxScaleFactor = 0.97f;
    float controlWidth = pUIElementNoRef->GetActualWidth();
    float controlHeight = pUIElementNoRef->GetActualHeight();

    // If this is a child of an item presenter, we get the dimensions of the parent and take the minimum size of the two.
    // This is when the content is larger than the item itself (in one or both dimensions).
    ctl::ComPtr<xaml::IDependencyObject> spParentAsIDO;

    IFC_RETURN(VisualTreeHelper::GetParentStatic(pUIENoRef, &spParentAsIDO));

    if (spParentAsIDO.AsOrNull<xaml_primitives::IListViewItemPresenter>())
    {
        ctl::ComPtr<DependencyObject> spParentAsDO;
        xref_ptr<CDependencyObject> spParentAsCDO;

        IFC_RETURN(spParentAsIDO.As(&spParentAsDO));
        spParentAsCDO = spParentAsDO->GetHandle();

        // when using a ListViewItemPresenter, we apply the PointerDownThemeAnimation to the child rather than presenter itself
        // this means that the child's size could be bigger than the presenter (which is the same size as the ListViewItem)
        // in that case, we want to offset the projection (tilt) and the scale (pressed)
        // this is only needed in the dimension (could be both x & y) where the size is larger than the item
        if (spParentAsCDO)
        {
            bool needsTransformGroup = false;
            CListViewBaseItemChrome* pLVBICNoRef = static_cast<CListViewBaseItemChrome*>(spParentAsCDO.get());
            float containerWidth = pLVBICNoRef->GetActualWidth();
            float containerHeight = pLVBICNoRef->GetActualHeight();

            // this is the translation offset for the projection
            float halfDifferenceX = 0.0f;
            float halfDifferenceY = 0.0f;

            needsTransformGroup = DirectUI::Components::ThemeAnimationsHelper::DoesLVIPNeedTransformGroupForPointerDownThemeAnimation(containerWidth,
                containerHeight,
                controlWidth,
                controlHeight,
                halfDifferenceX,
                halfDifferenceY);

            // we need a TransformGroup because we want to apply the Translation first
            if (needsTransformGroup)
            {
                ctl::ComPtr<TransformCollection> spTransforms;
                ctl::ComPtr<TranslateTransform> spTranslateTransform;

                IFC_RETURN(ctl::make<TransformCollection>(&spTransforms));
                IFC_RETURN(ctl::make<TranslateTransform>(&spTranslateTransform));
                IFC_RETURN(ctl::make<TransformGroup>(&spTransformGroup));

                // set the PlaneProjection's LocalOffset(s)
                IFC_RETURN(spProjection->put_LocalOffsetX(halfDifferenceX));
                IFC_RETURN(spProjection->put_LocalOffsetY(halfDifferenceY));

                // set the TranslateTransforms's coordinates
                IFC_RETURN(spTranslateTransform->put_X(-halfDifferenceX));
                IFC_RETURN(spTranslateTransform->put_Y(-halfDifferenceY));

                // add the translate transform first, then the composite transform
                IFC_RETURN(spTransforms->Append(spTranslateTransform.Get()));
                IFC_RETURN(spTransforms->Append(spCompositeTransform.Get()));

                // set the transform group children
                IFC_RETURN(spTransformGroup->put_Children(spTransforms.Get()));

                // set the animation property path names
                strScaleXPropertyName = wrl_wrappers::HStringReference(L"(UIElement.RenderTransform).(TransformGroup.Children)[1].(ScaleTransform.ScaleX)");
                strScaleYPropertyName = wrl_wrappers::HStringReference(L"(UIElement.RenderTransform).(TransformGroup.Children)[1].(ScaleTransform.ScaleY)");
            }
        }
    }

    // In order to get the right behavior here, first we had designers identify "points of
    // interest", i.e. designers chose what they considered important element sizes and
    // defined the max angle that would fit each size appropriately.
    // Currently, the points of interest we have from them are the following:
    //
    // Size         Max angle
    //-------------------------
    // 48px         30deg
    // 320px        9deg
    // 600px        2deg
    //
    // Following this approach, we need a max angle for elements that are theoretically 0px
    // in size and we also need to define a size for elements where the angle will ultimately
    // cap at 0 degrees. Designers are not particularly interested in elements smaller than
    // 48px or bigger than 600px, so long as the behavior is reasonable. So we can make something
    // up like this:
    //
    // Size         Max angle
    //-------------------------
    // 0px          ~35deg
    // 800px        ~0deg
    //
    // Using these five points, we can now plot them and obtain the following trendline by using
    // a polynomial regression of third order:
    // y = (-0.00000007 * x^3) + (0.00015904 * x^2) - (0.12506463 * x) + 35.27311191
    // NOTE: We're missing a little bit of precision here, so the trendline does not match the points
    // exactly, but that's alright. The only things that are important are: 1) the value of the max angle
    // always decreases as the element gets larger and 2) the value eventually reaches zero.
    // For the case above, the angle will reach zero at ~950px, but for the sake of it we will cap it
    // at 925px.
    static const float controlSizeThreshold = 925.0f;
    maxAngleXAxis = static_cast<float>((controlWidth > controlSizeThreshold) ? 0.0f : -0.00000007f * pow(controlWidth, 3) + 0.00015904f * pow(controlWidth, 2) - 0.12506463f * controlWidth + 35.27311191f);
    maxAngleYAxis = static_cast<float>((controlHeight > controlSizeThreshold) ? 0.0f : -0.00000007f * pow(controlHeight, 3) + 0.00015904f * pow(controlHeight, 2) - 0.12506463f * controlHeight + 35.27311191f);

    // Set the center for the Scale Transform to the center of the control to keep the scale symmetric about the center
    spCompositeTransform->put_CenterX(controlWidth / 2);
    spCompositeTransform->put_CenterY(controlHeight / 2);

    // Generate horizontal depression via ScaleX.
    ThemeGeneratorHelper scaleXSupplier(nullPoint, nullPoint, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
    IFC_RETURN(scaleXSupplier.Initialize());
    IFC_RETURN(scaleXSupplier.RegisterPointerKeyFrame(strScaleXPropertyName.Get(), PointerDirection::PointerDirection_BothAxes, 0.0, 1.0, beginTime));
    IFC_RETURN(scaleXSupplier.RegisterPointerKeyFrame(strScaleXPropertyName.Get(), PointerDirection::PointerDirection_BothAxes, 1.0, maxScaleFactor, beginTime));

    // Generate vertical depression via ScaleY.
    ThemeGeneratorHelper scaleYSupplier(nullPoint, nullPoint, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
    IFC_RETURN(scaleYSupplier.Initialize());
    IFC_RETURN(scaleYSupplier.RegisterPointerKeyFrame(strScaleYPropertyName.Get(), PointerDirection::PointerDirection_BothAxes, 0.0, 1.0, beginTime));
    IFC_RETURN(scaleYSupplier.RegisterPointerKeyFrame(strScaleYPropertyName.Get(), PointerDirection::PointerDirection_BothAxes, 1.0, maxScaleFactor, beginTime));

    IFC_RETURN(pUIENoRef->put_Projection(spProjection.Get()));

    if (spTransformGroup)
    {
        IFC_RETURN(pUIENoRef->put_RenderTransform(spTransformGroup.Get()));
    }
    else
    {
        IFC_RETURN(pUIENoRef->put_RenderTransform(spCompositeTransform.Get()));
    }

    // Generate horizontal tilting.
    ThemeGeneratorHelper tiltXSupplier(nullPoint, nullPoint, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
    IFC_RETURN(tiltXSupplier.Initialize());
    IFC_RETURN(tiltXSupplier.RegisterPointerKeyFrame(strRotateYPropertyName.Get(), PointerDirection::PointerDirection_XAxis, 0.0, maxAngleXAxis, beginTime));
    IFC_RETURN(tiltXSupplier.RegisterPointerKeyFrame(strRotateYPropertyName.Get(), PointerDirection::PointerDirection_XAxis, 1.0, -maxAngleXAxis, beginTime));

    // Generate vertical tilting.
    ThemeGeneratorHelper tiltYSupplier(nullPoint, nullPoint, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
    IFC_RETURN(tiltYSupplier.Initialize());
    IFC_RETURN(tiltYSupplier.RegisterPointerKeyFrame(strRotateXPropertyName.Get(), PointerDirection::PointerDirection_YAxis, 0.0, -maxAngleYAxis, beginTime));
    IFC_RETURN(tiltYSupplier.RegisterPointerKeyFrame(strRotateXPropertyName.Get(), PointerDirection::PointerDirection_YAxis, 1.0, maxAngleYAxis, beginTime));

    return S_OK;
}

PointerUpThemeAnimation::PointerUpThemeAnimation()
    : PointerUpThemeAnimationGenerated()
{
    m_completedToken = EventRegistrationToken();
}

_Check_return_ HRESULT PointerUpThemeAnimation::OnTimelineCompleted(_In_ IInspectable* /*pSender*/, _In_ IInspectable* /*pArgs*/)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spDO;
    ctl::ComPtr<xaml::IDependencyObject> spTarget = m_tpAnimationTarget.AsOrNull<xaml::IDependencyObject>();

    if (spTarget)
    {
        IFC(spTarget.As(&spDO));
    }
    else
    {
        wrl_wrappers::HString strTargetName;
        xstring_ptr strName;

        IFC(get_TargetName(strTargetName.GetAddressOf()));

        if (!strTargetName.Get())
        {
            // If the value of the TargetName property of the ThemeAnimation is null
            // that means that we will be fall back and use the value of the attached
            // property Storyboard.TargetName instead.
            IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
        }

        IFC(xstring_ptr::CloneRuntimeStringHandle(strTargetName.Get(), &strName));

        if (auto spCoreDO = CoreImports::Timeline_ResolveName(checked_cast<CTimeline>(GetHandle()), strName))
        {
            // Even if we got a core object, the peer might not exist (might be GC'd but
            // not finalized).  So *Try* to get it, we do a null check later.
            IFC(DXamlCore::GetCurrent()->TryGetPeer(spCoreDO, &spDO));
        }
    }

    if (spDO)
    {
        // Remove the projection and transform used by the tilt animation.
        UIElement *pUIENoRef = spDO.Cast<UIElement>();
        IFC(pUIENoRef->put_Projection(nullptr));
        IFC(pUIENoRef->put_RenderTransform(nullptr));
    }

    IFC(remove_Completed(m_completedToken));
    m_completedToken = EventRegistrationToken();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PointerUpThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    wf::Point offset = {0,0};
    wrl_wrappers::HString strTargetName;
    ctl::ComPtr<xaml::IDependencyObject> spTarget = m_tpAnimationTarget.AsOrNull<xaml::IDependencyObject>();

    if (!spTarget)
    {
        IFC_RETURN(get_TargetName(strTargetName.GetAddressOf()));

        if (!strTargetName.Get())
        {
            // If the value of the TargetName property of the ThemeAnimation is null
            // that means that we will be fall back and use the value of the attached
            // property Storyboard.TargetName instead.
            IFC_RETURN(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
        }
    }

    wrl_wrappers::HStringReference strRotateXPropertyName(L"(UIElement.Projection).(PlaneProjection.RotationX)");
    wrl_wrappers::HStringReference strRotateYPropertyName(L"(UIElement.Projection).(PlaneProjection.RotationY)");
    wrl_wrappers::HStringReference strGlobalOffsetZPropertyName(L"(UIElement.Projection).(PlaneProjection.GlobalOffsetZ)");
    wrl_wrappers::HStringReference strScaleXPropertyName(L"(UIElement.RenderTransform).(ScaleTransform.ScaleX)");
    wrl_wrappers::HStringReference strScaleYPropertyName(L"(UIElement.RenderTransform).(ScaleTransform.ScaleY)");
    UIElement *pUIENoRef = NULL;
    ctl::ComPtr<DependencyObject> spDO;
    ctl::ComPtr<IProjection> spProjection;
    ctl::ComPtr<ITransform> spTransform;

    TimingFunctionDescription easing = TimingFunctionDescription();
    easing.cp1.X = 0.0f;    // Exponential easing.
    easing.cp1.Y = 0.0f;
    easing.cp2.X = 0.1f;
    easing.cp2.Y = 0.25f;
    easing.cp3.X = 0.75f;
    easing.cp3.Y = 0.9f;
    easing.cp4.X = 1.0f;
    easing.cp4.Y = 1.0f;

    if (spTarget)
    {
        IFC_RETURN(spTarget.As(&spDO));
    }
    else
    {
        xstring_ptr strName;

        IFC_RETURN(xstring_ptr::CloneRuntimeStringHandle(strTargetName.Get(), &strName));

        if (auto spCoreDO = CoreImports::Timeline_ResolveName(checked_cast<CTimeline>(GetHandle()), strName))
        {
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(spCoreDO, &spDO));
        }
        else if (!strName.IsNullOrEmpty())
        {
            HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
            xephemeral_string_ptr parameters[1];

            strName.Demote(&parameters[0]);

            // Don't fail fast - VSM can catch this error and handle it
            IGNOREHR(GetHandle()->SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_SB_BEGIN_INVALID_TARGET, 1, parameters));
            IFC_RETURN(hrToOriginate);
        }
        else
        {
            HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
            // Don't fail fast - VSM can catch this error and handle it
            IGNOREHR(GetHandle()->SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_RUNTIME_SB_BEGIN_NO_TARGET));
            IFC_RETURN(hrToOriginate);
        }
    }

    pUIENoRef = spDO.Cast<UIElement>();

    // This theme animation is used to return an element tilted by the
    // PointerDownThemeAnimation back to its original position.
    // If an element is tilted, it should have a projection and a transform
    // attached to it already., since these are necessary for the animation.
    // If it does not, we infer that this animation is not needed.
    IFC_RETURN(pUIENoRef->get_Projection(&spProjection));
    IFC_RETURN(pUIENoRef->get_RenderTransform(&spTransform));

    if (spProjection != NULL && spTransform != NULL)
    {
        XINT64 beginTime = 0;
        XINT64 duration = 0;
        ctl::ComPtr<wf::IEventHandler<IInspectable*>> spTimelineCompletedHandler;

        // in case of a TransformGroup, our animation should target the second child of the group
        if (spTransform.AsOrNull<TransformGroup>())
        {
            // set the property path names
            strScaleXPropertyName = wrl_wrappers::HStringReference(L"(UIElement.RenderTransform).(TransformGroup.Children)[1].(ScaleTransform.ScaleX)");
            strScaleYPropertyName = wrl_wrappers::HStringReference(L"(UIElement.RenderTransform).(TransformGroup.Children)[1].(ScaleTransform.ScaleY)");
        }

        // Several changes in Tilt Animation behavior starting with Threshold (aka Windows 10),
        // see PointerDownThemeAnimation::CreateTimelines for description.
        beginTime = 150;
        duration = 200;

        // Generate horizontal depression via ScaleX.
        ThemeGeneratorHelper scaleXSupplier(offset, offset, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
        IFC_RETURN(scaleXSupplier.Initialize());
        IFC_RETURN(scaleXSupplier.RegisterKeyFrame(strScaleXPropertyName.Get(), 1.0, beginTime, duration, &easing));

        // Generate vertical depression via ScaleY.
        ThemeGeneratorHelper scaleYSupplier(offset, offset, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
        IFC_RETURN(scaleYSupplier.Initialize());
        IFC_RETURN(scaleYSupplier.RegisterKeyFrame(strScaleYPropertyName.Get(), 1.0, beginTime, duration, &easing));

        // Generate horizontal tilting.
        ThemeGeneratorHelper tiltXSupplier(offset, offset, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
        IFC_RETURN(tiltXSupplier.Initialize());
        IFC_RETURN(tiltXSupplier.RegisterKeyFrame(strRotateYPropertyName.Get(), 0, beginTime, duration, &easing));

        // Generate vertical tilting.
        ThemeGeneratorHelper tiltYSupplier(offset, offset, strTargetName.Get(), spTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
        IFC_RETURN(tiltYSupplier.Initialize());
        IFC_RETURN(tiltYSupplier.RegisterKeyFrame(strRotateXPropertyName.Get(), 0, beginTime, duration, &easing));

        if (m_completedToken.value != NULL)
        {
            IFC_RETURN(remove_Completed(m_completedToken));
            m_completedToken = EventRegistrationToken();
        }

        spTimelineCompletedHandler.Attach(
            new ClassMemberEventHandler<
                PointerUpThemeAnimation,
                IPointerUpThemeAnimation,
                wf::IEventHandler<IInspectable*>,
                IInspectable,
                IInspectable>(this, &PointerUpThemeAnimation::OnTimelineCompleted, true /* subscribingToSelf */ ));


        IFC_RETURN(add_Completed(spTimelineCompletedHandler.Get(), &m_completedToken));
    }

    return S_OK;
}

_Check_return_ HRESULT SplitOpenThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    const DOUBLE closedRatio = 0.50;
    const wf::Point nullPoint = {};
    const wf::Point splitOrigin = {0, 0.5};
    TimingFunctionDescription easing = TimingFunctionDescription();
    easing.cp3.X = 0.0f; // Cubic-bezier (0, 0, 0, 1). Default TimingFunctionDescription() constructor creates a Linear curve (0,0,0,0,1,1,1,1).
    ThemeGeneratorHelper* pBackgroundSupplier = NULL;
    ThemeGeneratorHelper* pPanelSupplier = NULL;
    ThemeGeneratorHelper* pFaceplateSupplier = NULL;
    ctl::ComPtr<IInspectable> spClosedLength;
    BOOLEAN isUnsetValue = FALSE;

    // Parts and rects.
    wrl_wrappers::HString strOpenName;
    xaml::IDependencyObject* pOpenTarget = NULL;
    wrl_wrappers::HString strContentName;
    xaml::IDependencyObject* pContentTarget = NULL;
    wrl_wrappers::HString strCloseName;
    xaml::IDependencyObject* pCloseTarget = NULL;
    DOUBLE closedLength = 0;
    DOUBLE openedLength = 0;
    DOUBLE offsetFromCenter = 0;
    DOUBLE contentTranslateOffset = 0;
    DOUBLE initialClipScaleY = 0;
    DOUBLE finalClipScaleY = 0;
    xaml_primitives::AnimationDirection direction = xaml_primitives::AnimationDirection_Top;

    IFC(get_OpenedTargetName(strOpenName.GetAddressOf()));
    IFC(get_OpenedTarget(&pOpenTarget));
    IFC(get_ContentTargetName(strContentName.GetAddressOf()));
    IFC(get_ContentTarget(&pContentTarget));
    IFC(get_ClosedTargetName(strCloseName.GetAddressOf()));
    IFC(get_ClosedTarget(&pCloseTarget));
    IFC(get_ClosedLength(&closedLength));
    IFC(get_OpenedLength(&openedLength));
    IFC(get_OffsetFromCenter(&offsetFromCenter));
    IFC(get_ContentTranslationOffset(&contentTranslateOffset));
    IFC(get_ContentTranslationDirection(&direction));

    if ((!strOpenName.Get() && !pOpenTarget) || openedLength == 0)   // dividing by openlength
    {
        goto Cleanup;
    }

    // Since we will be translating the clip, that means, for example, that
    // a clip scale of 1.0 would not cover the element entirely. Thus we
    // need to adjust the scale accordingly. In other words, if the area
    // covered by the clip is partially translated off the element, then
    // we need to make the clip bigger in order to compensate.

    IFC(ReadLocalValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::SplitOpenThemeAnimation_ClosedLength),
        &spClosedLength));
    IFC(DependencyPropertyFactory::IsUnsetValue(spClosedLength.Get(), isUnsetValue));

    // If the ClosedLength property was not set, by default we will instead use
    // a specific proportion of the OpenedLength.
    if (isUnsetValue)
    {
        DOUBLE clipLength = openedLength * closedRatio;
        DOUBLE maxOffset = openedLength * (1 - closedRatio) / 2.0;   // Max offset possible before the clip is partially off the element.
        if (DoubleUtil::Abs(offsetFromCenter) > maxOffset)
        {
            DOUBLE pixelsOff = (clipLength / 2.0) - (openedLength / 2.0 - DoubleUtil::Abs(offsetFromCenter));
            initialClipScaleY = pixelsOff / openedLength * 2.0 + closedRatio;
        }
        else
        {
            initialClipScaleY = closedRatio;
        }
    }
    else
    {
        initialClipScaleY = closedLength / openedLength;
    }

    finalClipScaleY = (0.5 + DoubleUtil::Abs(offsetFromCenter / openedLength)) * 2;

    // ********** background, clip and opacity ************
    pBackgroundSupplier = new ThemeGeneratorHelper(nullPoint, nullPoint, strOpenName.Get(), pOpenTarget, bOnlyGenerateSteadyState, pTimelineCollection);
    IFC(pBackgroundSupplier->Initialize());
    IFC(pBackgroundSupplier->SetClipOriginValues(splitOrigin));   // to get the same speed going up and down, we always use 0.5 for this animation
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetClipScaleYPropertyName(), initialClipScaleY, 0, 0, &easing));
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetClipScaleYPropertyName(), finalClipScaleY, 0, SplitOpenThemeAnimation::s_OpenDuration, &easing));
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetClipTranslateYPropertyName(), offsetFromCenter, 0, 0, &easing));  // immediately go there
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetOpacityPropertyName(), 1.0, 0, 0, &easing));    // be fully opaque

    // ********* content, opacity and translation *********
    if (strContentName.Get() || pContentTarget)
    {
        pPanelSupplier = new ThemeGeneratorHelper(nullPoint, nullPoint, strContentName.Get(), pContentTarget, bOnlyGenerateSteadyState, pTimelineCollection);
        IFC(pPanelSupplier->Initialize());
        if (direction == xaml_primitives::AnimationDirection_Top || direction == xaml_primitives::AnimationDirection_Bottom)
        {
            IFC(pPanelSupplier->RegisterKeyFrame(
                pPanelSupplier->GetTranslateYPropertyName(),
                direction == xaml_primitives::AnimationDirection_Bottom ? contentTranslateOffset : -contentTranslateOffset,
                0,
                0,
                &easing));    // start off with offset so we can come to rest at 0
            IFC(pPanelSupplier->RegisterKeyFrame(pPanelSupplier->GetTranslateYPropertyName(), 0, 0, SplitOpenThemeAnimation::s_OpenDuration, &easing));
        }
        else
        {
            IFC(pPanelSupplier->RegisterKeyFrame(
                pPanelSupplier->GetTranslateXPropertyName(),
                direction == xaml_primitives::AnimationDirection_Right ? contentTranslateOffset : -contentTranslateOffset,
                0,
                0,
                &easing));    // start off with offset so we can come to rest at 0
            IFC(pPanelSupplier->RegisterKeyFrame(pPanelSupplier->GetTranslateXPropertyName(), 0, 0, SplitOpenThemeAnimation::s_OpenDuration, &easing));
        }
    }

    // *************** faceplate, opacity *****************
    if (strCloseName.Get() || pCloseTarget)
    {
        TimingFunctionDescription linear = TimingFunctionDescription();

        pFaceplateSupplier = new ThemeGeneratorHelper(nullPoint, nullPoint, strCloseName.Get(), pCloseTarget, bOnlyGenerateSteadyState, pTimelineCollection);
        IFC(pFaceplateSupplier->Initialize());
        IFC(pFaceplateSupplier->RegisterKeyFrame(pFaceplateSupplier->GetOpacityPropertyName(), 1.0, 0, 0, &linear));
        IFC(pFaceplateSupplier->RegisterKeyFrame(pFaceplateSupplier->GetOpacityPropertyName(), 0.5, 0, SplitOpenThemeAnimation::s_OpacityChangeDuration, &linear));
    }

Cleanup:
    delete pBackgroundSupplier;
    delete pPanelSupplier;
    delete pFaceplateSupplier;
    ReleaseInterface(pOpenTarget);
    ReleaseInterface(pContentTarget);
    ReleaseInterface(pCloseTarget);
    RRETURN(hr);
}

_Check_return_ HRESULT SplitCloseThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    // unfortunately the split open is different enough from split close that I don't see that much value in moving code to a helper method.
    // also, we need to have the flexibility to change these independently, even though they use the same primitives at the moment.
    HRESULT hr = S_OK;
    const DOUBLE closedRatio = 0.15;
    const wf::Point nullPoint = {};
    const wf::Point splitOrigin = {0, 0.5};
    TimingFunctionDescription linear = TimingFunctionDescription();
    TimingFunctionDescription easing = TimingFunctionDescription();
    easing.cp3.X = 0.0f; // Cubic-bezier (0, 0, 0, 1). Default TimingFunctionDescription() constructor creates a Linear curve (0,0,0,0,1,1,1,1).

    ThemeGeneratorHelper* pBackgroundSupplier = NULL;
    ThemeGeneratorHelper* pPanelSupplier = NULL;
    ThemeGeneratorHelper* pFaceplateSupplier = NULL;
    ctl::ComPtr<IInspectable> spClosedLength;
    BOOLEAN isUnsetValue = FALSE;

    // Parts and rects.
    wrl_wrappers::HString strOpenName;
    xaml::IDependencyObject* pOpenTarget = NULL;
    wrl_wrappers::HString strContentName;
    xaml::IDependencyObject* pContentTarget = NULL;
    wrl_wrappers::HString strCloseName;
    xaml::IDependencyObject* pCloseTarget = NULL;
    DOUBLE closedLength = 0;
    DOUBLE openedLength = 0;
    DOUBLE offsetFromCenter = 0;
    DOUBLE contentTranslateOffset = 0;
    DOUBLE initialClipScaleY = 0;
    DOUBLE finalClipScaleY = 0;
    xaml_primitives::AnimationDirection direction = xaml_primitives::AnimationDirection_Top;

    IFC(get_OpenedTargetName(strOpenName.GetAddressOf()));
    IFC(get_OpenedTarget(&pOpenTarget));
    IFC(get_ContentTargetName(strContentName.GetAddressOf()));
    IFC(get_ContentTarget(&pContentTarget));
    IFC(get_ClosedTargetName(strCloseName.GetAddressOf()));
    IFC(get_ClosedTarget(&pCloseTarget));
    IFC(get_ClosedLength(&closedLength));
    IFC(get_OpenedLength(&openedLength));
    IFC(get_OffsetFromCenter(&offsetFromCenter));
    IFC(get_ContentTranslationOffset(&contentTranslateOffset));
    IFC(get_ContentTranslationDirection(&direction));

    if ((!strOpenName.Get() && !pOpenTarget) || openedLength == 0)   // dividing by openlength
    {
        goto Cleanup;
    }

    // Since we will be translating the clip, that means, for example, that
    // a clip scale of 1.0 would not cover the element entirely. Thus we
    // need to adjust the scale accordingly. In other words, if the area
    // covered by the clip is partially translated off the element, then
    // we need to make the clip bigger in order to compensate.

    initialClipScaleY = (0.5 + DoubleUtil::Abs(offsetFromCenter / openedLength)) * 2;

    IFC(ReadLocalValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::SplitCloseThemeAnimation_ClosedLength),
        &spClosedLength));
    IFC(DependencyPropertyFactory::IsUnsetValue(spClosedLength.Get(), isUnsetValue));

    // If the ClosedLength property was not set, by default we will instead use
    // a specific proportion of the OpenedLength.
    if (isUnsetValue)
    {
        DOUBLE clipLength = openedLength * closedRatio;
        DOUBLE maxOffset = openedLength * (1 - closedRatio) / 2.0;   // Max offset possible before the clip is partially off the element.
        if (DoubleUtil::Abs(offsetFromCenter) > maxOffset)
        {
            DOUBLE pixelsOff = (clipLength / 2.0) - (openedLength / 2.0 - DoubleUtil::Abs(offsetFromCenter));
            finalClipScaleY = pixelsOff / openedLength * 2.0 + closedRatio;
        }
        else
        {
            finalClipScaleY = closedRatio;
        }
    }
    else
    {
        finalClipScaleY = closedLength / openedLength;
    }

    // ********** background, clip and opacity ************
    pBackgroundSupplier = new ThemeGeneratorHelper(nullPoint, nullPoint, strOpenName.Get(), pOpenTarget, bOnlyGenerateSteadyState, pTimelineCollection);
    IFC(pBackgroundSupplier->Initialize());
    IFC(pBackgroundSupplier->SetClipOriginValues(splitOrigin));
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetClipScaleYPropertyName(), initialClipScaleY, 0, 0, &easing));
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetClipScaleYPropertyName(), finalClipScaleY, 0, SplitCloseThemeAnimation::s_CloseDuration, &easing));
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetClipTranslateYPropertyName(), offsetFromCenter, 0, 0, &easing));  // immediately go there

    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetOpacityPropertyName(), 1.0, 0, 0, &linear)); 
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetOpacityPropertyName(), 1.0, 0, SplitCloseThemeAnimation::s_OpacityChangeBeginTime, &linear));
    IFC(pBackgroundSupplier->RegisterKeyFrame(pBackgroundSupplier->GetOpacityPropertyName(), 0.0, SplitCloseThemeAnimation::s_OpacityChangeBeginTime, SplitCloseThemeAnimation::s_OpacityChangeDuration, &linear));

    // ********* content, opacity and translation *********
    if (strContentName.Get() || pContentTarget)
    {
        pPanelSupplier = new ThemeGeneratorHelper(nullPoint, nullPoint, strContentName.Get(), pContentTarget, bOnlyGenerateSteadyState, pTimelineCollection);
        IFC(pPanelSupplier->Initialize());
        IFC(pPanelSupplier->RegisterKeyFrame(pPanelSupplier->GetOpacityPropertyName(), 1, 0, 0, &linear));    // start off fully opaque
        IFC(pPanelSupplier->RegisterKeyFrame(pPanelSupplier->GetOpacityPropertyName(), 1.0, 0, SplitCloseThemeAnimation::s_OpacityChangeBeginTime, &linear));
        IFC(pPanelSupplier->RegisterKeyFrame(pPanelSupplier->GetOpacityPropertyName(), 0.0, SplitCloseThemeAnimation::s_OpacityChangeBeginTime, SplitCloseThemeAnimation::s_OpacityChangeDuration, &linear));

        if (direction == xaml_primitives::AnimationDirection_Top || direction == xaml_primitives::AnimationDirection_Bottom)
        {
            IFC(pPanelSupplier->RegisterKeyFrame(pPanelSupplier->GetTranslateYPropertyName(), 0, 0, 0, &easing));    // start off with offset so we can come to rest at 0
            IFC(pPanelSupplier->RegisterKeyFrame(
                pPanelSupplier->GetTranslateYPropertyName(),
                direction == xaml_primitives::AnimationDirection_Bottom ? contentTranslateOffset : -contentTranslateOffset, 
                0,
                SplitCloseThemeAnimation::s_CloseDuration, 
                &easing));
        }
        else
        {
            IFC(pPanelSupplier->RegisterKeyFrame(pPanelSupplier->GetTranslateXPropertyName(), 0, 0, 0, &easing));    // start off with offset so we can come to rest at 0
            IFC(pPanelSupplier->RegisterKeyFrame(
                pPanelSupplier->GetTranslateXPropertyName(),
                direction == xaml_primitives::AnimationDirection_Right ? contentTranslateOffset : -contentTranslateOffset,  
                0,
                SplitCloseThemeAnimation::s_CloseDuration, 
                &easing));
        }
    }

    // *************** faceplate, opacity *****************
    if (strCloseName.Get() || pCloseTarget)
    {
        pFaceplateSupplier = new ThemeGeneratorHelper(nullPoint, nullPoint, strCloseName.Get(), pCloseTarget, bOnlyGenerateSteadyState, pTimelineCollection);
        IFC(pFaceplateSupplier->Initialize());
        IFC(pFaceplateSupplier->RegisterKeyFrame(pFaceplateSupplier->GetOpacityPropertyName(), 0.0, 0, 0, &linear));
        IFC(pFaceplateSupplier->RegisterKeyFrame(pFaceplateSupplier->GetOpacityPropertyName(), 0.0, 0, SplitCloseThemeAnimation::s_OpacityChangeBeginTime, &linear));
        IFC(pFaceplateSupplier->RegisterKeyFrame(pFaceplateSupplier->GetOpacityPropertyName(), 1.0, SplitCloseThemeAnimation::s_OpacityChangeBeginTime, SplitCloseThemeAnimation::s_OpacityChangeDuration, &linear));
    }

Cleanup:
    delete pFaceplateSupplier;
    delete pBackgroundSupplier;
    delete pPanelSupplier;
    ReleaseInterface(pOpenTarget);
    ReleaseInterface(pCloseTarget);
    ReleaseInterface(pContentTarget);
    RRETURN(hr);
}

_Check_return_ HRESULT SwipeHintThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point destinationOffset = {0,0};
    wf::Point startOffset = {0, 0};
    ThemeGeneratorHelper* pSupplier = NULL;
    wrl_wrappers::HString strTargetName;
    SwipeHintTimingFunctionDescription easing = SwipeHintTimingFunctionDescription();

    DOUBLE x;
    DOUBLE y;
    IFC(get_ToHorizontalOffset(&x));
    IFC(get_ToVerticalOffset(&y));

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    destinationOffset.X = (FLOAT)x;
    destinationOffset.Y = (FLOAT)y;

    // if we only want to generate steady state, the animation does nothing
    // since it is an animation down and up again.
    if (!bOnlyGenerateSteadyState)
    {

        // XDR has decided that it wants the following animation for the checkmark reveal:
        // 1. Animate item down 25px: 300ms (pillow landing)
        // 2. Animate item back up to original position: 300ms (pillow landing)

        // However, they have also decided not to put that information into the swipereveal definition.
        // That means that we cannot use PVL for this, and we'll build up our own animation.

        // the architecture of ThemeGenerator was never meant to have to composite animations this way
        // so we'll have to build up all the components by hand.
        pSupplier = new ThemeGeneratorHelper(startOffset, destinationOffset, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, pTimelineCollection);
        IFC(pSupplier->Initialize());

        // 1. animate down
        IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), destinationOffset.X, 0, 300, &easing));
        IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateYPropertyName(), destinationOffset.Y, 0, 300, &easing));


        // there is no easy way to know how long the previous animation took, but if PVL remains
        // unchanged, this is defined as 300ms (as defined in fbl_uex(dev) on 12/22/2011)
        // so the animation up should start after 300ms.
        // We have an override for AddTimelinesForThemeAnimation that takes a parameter for
        // additional time.

        // 2. animate back up
        IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateXPropertyName(), 0, 300, 300, &easing));
        IFC(pSupplier->RegisterKeyFrame(pSupplier->GetTranslateYPropertyName(), 0, 300, 300, &easing));
    }

Cleanup:
    delete pSupplier;
    RRETURN(hr);
}

_Check_return_ HRESULT SwipeBackThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point destinationOffset = {0,0};
    wf::Point startOffset = {0, 0};
    wrl_wrappers::HString strTargetName;

    DOUBLE x;
    DOUBLE y;
    IFC(get_FromHorizontalOffset(&x));
    IFC(get_FromVerticalOffset(&y));

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    startOffset.X = (FLOAT)x;
    startOffset.Y = (FLOAT)y;

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_SWIPEDESELECT, TA_SWIPEDESELECT_DESELECTED, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT RepositionThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point destinationOffset = {0,0};
    wf::Point startOffset = {0, 0};
    wrl_wrappers::HString strTargetName;

    DOUBLE x;
    DOUBLE y;
    IFC(get_FromHorizontalOffset(&x));
    IFC(get_FromVerticalOffset(&y));

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    startOffset.X = (FLOAT)x;
    startOffset.Y = (FLOAT)y;

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_REPOSITION, TA_REPOSITION_TARGET, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

// the item being dragged
_Check_return_ HRESULT DragItemThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point destinationOffset = {0,0};
    wf::Point startOffset = {0, 0};
    wrl_wrappers::HString strTargetName;

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_DRAGSOURCESTART, TA_DRAGSOURCESTART_DRAGSOURCE, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

// items not being dragged, during drag/drop
_Check_return_ HRESULT DropTargetItemThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point destinationOffset = {0,0};
    wf::Point startOffset = {0, 0};
    wrl_wrappers::HString strTargetName;

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_DRAGSOURCESTART, TA_DRAGSOURCESTART_AFFECTED, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

// items below the dragged item
_Check_return_ HRESULT DragOverThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    HRESULT hr = S_OK;
    wf::Point destinationOffset = {0,0};
    wf::Point startOffset = {0, 0};
    xaml_primitives::AnimationDirection direction = xaml_primitives::AnimationDirection_Top;
    wrl_wrappers::HString strTargetName;
    DOUBLE offset;

    // Get Target Name. If ThemeAnimation's TargetName is not set, get its
    // Storyboard.TargetName core attached property.
    IFC(get_TargetName(strTargetName.GetAddressOf()));
    if (!strTargetName.Get())
    {
        IFC(ThemeAnimation_GetStoryboardTargetName(this, strTargetName.GetAddressOf()));
    }

    IFC(get_ToOffset(&offset));
    IFC(get_Direction(&direction));

    if (direction == xaml_primitives::AnimationDirection_Top || direction == xaml_primitives::AnimationDirection_Bottom)
    {
        destinationOffset.Y = direction == xaml_primitives::AnimationDirection_Bottom ? (FLOAT)offset : (FLOAT)-offset;
    }
    else
    {
        destinationOffset.X = direction == xaml_primitives::AnimationDirection_Right ? (FLOAT)offset : (FLOAT)-offset;
    }

    __if_exists(TAS_DRAGENTER)
    {
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_DRAGENTER, TA_DRAGENTER_TARGET, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));
    }
    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_DRAGBETWEENENTER, TA_DRAGBETWEENENTER_AFFECTED, strTargetName.Get(), NULL, bOnlyGenerateSteadyState, startOffset, destinationOffset, pTimelineCollection));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DrillInThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    wf::Point nullPoint = { 0, 0 };
    wf::Point transformOrigin = { 0.5, 0.5 };

    TimingFunctionDescription scaleEasing = TimingFunctionDescription();
    scaleEasing.cp2.X = 0.1f;   // Cubic-Bezier (0.1, 0.9, 0.2, 1.0).
    scaleEasing.cp2.Y = 0.9f;
    scaleEasing.cp3.X = 0.2f;
    scaleEasing.cp3.Y = 1.0f;

    TimingFunctionDescription opacityEasing = TimingFunctionDescription();
    opacityEasing.cp2.X = 0.17f;   // Cubic-Bezier (0.17, 0.17, 0.0, 1.0).
    opacityEasing.cp2.Y = 0.17f;
    opacityEasing.cp3.X = 0.0f;
    opacityEasing.cp3.Y = 1.0f;

    wrl_wrappers::HString strEntranceTargetName;
    ctl::ComPtr<xaml::IDependencyObject> spEntranceTarget;
    wrl_wrappers::HString strExitTargetName;
    ctl::ComPtr<xaml::IDependencyObject> spExitTarget;

    IFC_RETURN(get_EntranceTargetName(strEntranceTargetName.GetAddressOf()));
    IFC_RETURN(get_EntranceTarget(&spEntranceTarget));
    IFC_RETURN(get_ExitTargetName(strExitTargetName.GetAddressOf()));
    IFC_RETURN(get_ExitTarget(&spExitTarget));

    if ((strEntranceTargetName.Get() || spEntranceTarget.Get()) && (strExitTargetName.Get() || spExitTarget.Get()))
    {
        // Exit.
        {
            const DOUBLE scaleFactor = 1.04;
            ThemeGeneratorHelper exitSupplier(nullPoint, nullPoint, strExitTargetName.Get(), spExitTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
            IFC_RETURN(exitSupplier.Initialize());

            // 2D Transform origin.
            IFC_RETURN(exitSupplier.Set2DTransformOriginValues(transformOrigin));

            // Scale X.
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleXPropertyName(), 1.0, 0, 0, &scaleEasing));
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleXPropertyName(), scaleFactor, 0, DrillInThemeAnimation::s_NavigatingAwayScaleDuration, &scaleEasing));

            // Scale Y.
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleYPropertyName(), 1.0, 0, 0, &scaleEasing));
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleYPropertyName(), scaleFactor, 0, DrillInThemeAnimation::s_NavigatingAwayScaleDuration, &scaleEasing));

            // Opacity.
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetOpacityPropertyName(), 1.0, 0, 0, &opacityEasing));
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetOpacityPropertyName(), 0.0, 0, DrillInThemeAnimation::s_NavigatingAwayOpacityDuration, &opacityEasing));
        }

        // Entrance.
        {
            const DOUBLE scaleFactor = 0.94;
            ThemeGeneratorHelper entranceSupplier(nullPoint, nullPoint, strEntranceTargetName.Get(), spEntranceTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
            IFC_RETURN(entranceSupplier.Initialize());

            // 2D Transform origin.
            IFC_RETURN(entranceSupplier.Set2DTransformOriginValues(transformOrigin));

            // Scale X.
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleXPropertyName(), scaleFactor, 0, 0, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleXPropertyName(), scaleFactor, 0, DrillInThemeAnimation::s_NavigatingToBeginTime, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleXPropertyName(), 1.0, DrillInThemeAnimation::s_NavigatingToBeginTime, DrillInThemeAnimation::s_NavigatingToScaleDuration, &scaleEasing));

            // Scale Y.
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleYPropertyName(), scaleFactor, 0, 0, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleYPropertyName(), scaleFactor, 0, DrillInThemeAnimation::s_NavigatingToBeginTime, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleYPropertyName(), 1.0, DrillInThemeAnimation::s_NavigatingToBeginTime, DrillInThemeAnimation::s_NavigatingToScaleDuration, &scaleEasing));

            // Opacity.
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetOpacityPropertyName(), 0.0, 0, 0, &opacityEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetOpacityPropertyName(), 0.0, 0, DrillInThemeAnimation::s_NavigatingToBeginTime, &opacityEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetOpacityPropertyName(), 1.0, DrillInThemeAnimation::s_NavigatingToBeginTime, DrillInThemeAnimation::s_NavigatingToOpacityDuration, &opacityEasing));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DrillOutThemeAnimation::CreateTimelines(_In_ BOOLEAN bOnlyGenerateSteadyState, _In_ wfc::IVector<xaml_animation::Timeline*>* pTimelineCollection)
{
    wf::Point nullPoint = { 0, 0 };
    wf::Point transformOrigin = { 0.5, 0.5 };

    wrl_wrappers::HString strEntranceTargetName;
    ctl::ComPtr<xaml::IDependencyObject> spEntranceTarget;
    wrl_wrappers::HString strExitTargetName;
    ctl::ComPtr<xaml::IDependencyObject> spExitTarget;

    IFC_RETURN(get_EntranceTargetName(strEntranceTargetName.GetAddressOf()));
    IFC_RETURN(get_EntranceTarget(&spEntranceTarget));
    IFC_RETURN(get_ExitTargetName(strExitTargetName.GetAddressOf()));
    IFC_RETURN(get_ExitTarget(&spExitTarget));

    if ((strEntranceTargetName.Get() || spEntranceTarget.Get()) && (strExitTargetName.Get() || spExitTarget.Get()))
    {
        // Exit.
        {
            const DOUBLE scaleFactor = 0.96;

            TimingFunctionDescription scaleEasing = TimingFunctionDescription();
            scaleEasing.cp2.X = 0.1f;   // Cubic-Bezier (0.1, 0.9, 0.2, 1.0).
            scaleEasing.cp2.Y = 0.9f;
            scaleEasing.cp3.X = 0.2f;
            scaleEasing.cp3.Y = 1.0f;

            TimingFunctionDescription opacityEasing = TimingFunctionDescription();
            opacityEasing.cp2.X = 0.17f;   // Cubic-Bezier (0.17, 0.17, 0.0, 1.0).
            opacityEasing.cp2.Y = 0.17f;
            opacityEasing.cp3.X = 0.0f;
            opacityEasing.cp3.Y = 1.0f;

            ThemeGeneratorHelper exitSupplier(nullPoint, nullPoint, strExitTargetName.Get(), spExitTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
            IFC_RETURN(exitSupplier.Initialize());

            // 2D Transform origin.
            IFC_RETURN(exitSupplier.Set2DTransformOriginValues(transformOrigin));

            // Scale X.
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleXPropertyName(), 1.0, 0, 0, &scaleEasing));
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleXPropertyName(), scaleFactor, 0, DrillOutThemeAnimation::s_BackNavigatingAwayScaleDuration, &scaleEasing));

            // Scale Y.
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleYPropertyName(), 1.0, 0, 0, &scaleEasing));
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetScaleYPropertyName(), scaleFactor, 0, DrillOutThemeAnimation::s_BackNavigatingAwayScaleDuration, &scaleEasing));

            // Opacity.
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetOpacityPropertyName(), 1.0, 0, 0, &opacityEasing));
            IFC_RETURN(exitSupplier.RegisterKeyFrame(exitSupplier.GetOpacityPropertyName(), 0.0, 0, DrillOutThemeAnimation::s_BackNavigatingAwayOpacityDuration, &opacityEasing));
        }

        // Entrance.
        {
            const DOUBLE scaleFactor = 1.06;

            TimingFunctionDescription scaleEasing = TimingFunctionDescription();
            scaleEasing.cp2.X = 0.12f;   // Cubic-Bezier (0.12, 0.0, 0.0, 1.0).
            scaleEasing.cp2.Y = 0.0f;
            scaleEasing.cp3.X = 0.0f;
            scaleEasing.cp3.Y = 1.0f;

            TimingFunctionDescription opacityEasing = TimingFunctionDescription();
            opacityEasing.cp2.X = 0.17f;   // Cubic-Bezier (0.17, 0.17, 0.0, 1.0).
            opacityEasing.cp2.Y = 0.17f;
            opacityEasing.cp3.X = 0.0f;
            opacityEasing.cp3.Y = 1.0f;


            ThemeGeneratorHelper entranceSupplier(nullPoint, nullPoint, strEntranceTargetName.Get(), spEntranceTarget.Get(), bOnlyGenerateSteadyState, pTimelineCollection);
            IFC_RETURN(entranceSupplier.Initialize());

            // 2D transform origin.
            IFC_RETURN(entranceSupplier.Set2DTransformOriginValues(transformOrigin));

            // Scale X.
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleXPropertyName(), scaleFactor, 0, 0, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleXPropertyName(), scaleFactor, 0, DrillOutThemeAnimation::s_BackNavigatingToBeginTime, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleXPropertyName(), 1.0, DrillOutThemeAnimation::s_BackNavigatingToBeginTime, DrillOutThemeAnimation::s_BackNavigatingToScaleDuration, &scaleEasing));

            // Scale Y.
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleYPropertyName(), scaleFactor, 0, 0, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleYPropertyName(), scaleFactor, 0, DrillOutThemeAnimation::s_BackNavigatingToBeginTime, &scaleEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetScaleYPropertyName(), 1.0, DrillOutThemeAnimation::s_BackNavigatingToBeginTime, DrillOutThemeAnimation::s_BackNavigatingToScaleDuration, &scaleEasing));

            // Opacity.
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetOpacityPropertyName(), 0.0, 0, 0, &opacityEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetOpacityPropertyName(), 0.0, 0, DrillOutThemeAnimation::s_BackNavigatingToBeginTime, &opacityEasing));
            IFC_RETURN(entranceSupplier.RegisterKeyFrame(entranceSupplier.GetOpacityPropertyName(), 1.0, DrillOutThemeAnimation::s_BackNavigatingToBeginTime, DrillOutThemeAnimation::s_BackNavigatingToOpacityDuration, &opacityEasing));
        }
    }

    return S_OK;
}
