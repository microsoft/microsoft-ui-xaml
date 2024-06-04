// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Transition.g.h"
#include "DoubleAnimation.g.h"
#include "DoubleAnimationUsingKeyFrames.g.h"
#include "SplineDoubleKeyFrame.g.h"
#include "KeySpline.g.h"
#include "ObjectAnimationUsingKeyFrames.g.h"
#include "DiscreteObjectKeyFrame.g.h"
#include "vsanimation.h"
#include "ItemContainerGenerator.g.h"
#include "RepositionThemeTransition.g.h"
#include "Storyboard.g.h"
#include "InputPaneThemeTransition.g.h"
#include "MenuPopupThemeTransition.g.h"
#include "PickerFlyoutThemeTransition.g.h"
#include "ContentThemeTransition.g.h"
#include "PopupThemeTransition.g.h"
#include "EdgeUIThemeTransition.g.h"
#include "PaneThemeTransition.g.h"
#include "EntranceThemeTransition.g.h"
#include "StaggerFunctionBase.g.h"
#include "Panel.g.h"
#include "ReorderThemeTransition.g.h"
#include "AddDeleteThemeTransition.g.h"
#include "ITransitionContextProvider.g.h"
#include "IChildTransitionContextProvider.g.h"
#include "ListViewBaseHeaderItem.g.h"
#include "Window.g.h"
#include "ContentDialog.g.h"
#include "ContentDialogOpenCloseThemeTransition.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "ThemeGenerator.h"
#include "VisualTreeHelper.h"
#include "RootScale.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Uncomment to output debugging information
// #define MPTT_DEBUG

// static
_Check_return_ HRESULT Transition::ParticipateInTransition(
    _In_ CDependencyObject* nativeTransition,
    _In_ CDependencyObject* nativeUIElement,
    _In_ XINT32 transitionTrigger,
    _Out_ bool* DoesParticipate)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    ctl::ComPtr<DependencyObject> spTransitionDO;
    ctl::ComPtr<DependencyObject> spElementDO;
    ctl::ComPtr<IUIElement> spElementAsI;
    BOOLEAN participate = FALSE;
    ctl::ComPtr<IInspectable> spIsRecycling;
    BOOLEAN isRecycling = FALSE;
    BOOLEAN isUnsetValue = FALSE;
    ctl::ComPtr<ITransitionPrivate> spTransitionAsITransitionPrivate;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC(pCore->GetPeer(nativeTransition, &spTransitionDO));
    IFC(spTransitionDO.As(&spTransitionAsITransitionPrivate));
    IFC(pCore->GetPeer(nativeUIElement, &spElementDO));
    IFC(spElementDO.As(&spElementAsI));

    // check if this container is actually already placed in the recycle queue
    // if so, never setup a transition (it will probably try to do an unload during disconnecting from the visual tree)
    IFC(spElementDO->ReadLocalValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ItemContainerGenerator_IsRecycledContainer), &spIsRecycling));
    IFC(DependencyPropertyFactory::IsUnsetValue(spIsRecycling.Get(), isUnsetValue));

    if (!isUnsetValue)
    {
        IFC(ctl::do_get_value(isRecycling, spIsRecycling.Get()));
    }

    if (!isRecycling)
    {
        IFC(spTransitionAsITransitionPrivate->ParticipatesInTransition(spElementAsI.Get(), static_cast<xaml::TransitionTrigger>(transitionTrigger), &participate));
    }

    *DoesParticipate = !!participate;
Cleanup:
    RRETURN(S_OK);
}

// static
_Check_return_ HRESULT Transition::CreateStoryboardsForTransition(
    _In_ CDependencyObject* nativeTransition,
    _In_ CDependencyObject* nativeUIElement,
    _In_ XRECTF startBounds,
    _In_ XRECTF destinationBounds,
    _In_ XINT32 transitionTrigger,
    _Out_ XINT32* cStoryboardCount,
    _Outptr_result_buffer_(*cStoryboardCount) CStoryboard*** pppStoryboardArray,
    _Out_ DirectUI::TransitionParent* parentToTransitionEnum)
{
    HRESULT hr = S_OK;
    UINT cReturnedStoryboardCount = 0;
    CStoryboard** ppStoryboardArray = nullptr;
    ctl::ComPtr<IUIElement> spElementAsI;
    ctl::ComPtr<ITransitionPrivate> spTransitionAsITransitionPrivate;
    xaml::TransitionTrigger trigger = static_cast<xaml::TransitionTrigger>(transitionTrigger);
    xaml::TransitionParent transitionToParent = xaml::TransitionParent_ParentToCommonParent;    // default to clipping to parent
    ctl::ComPtr<TrackerCollection<xaml_animation::Storyboard*>> spStoryboards;
    IFC(ctl::make(&spStoryboards));

    wf::Rect start;
    start.Height = startBounds.Height;
    start.Width = startBounds.Width;
    start.X = startBounds.X;
    start.Y = startBounds.Y;

    wf::Rect destination;
    destination.Height = destinationBounds.Height;
    destination.Width = destinationBounds.Width;
    destination.X = destinationBounds.X;
    destination.Y = destinationBounds.Y;

    {
        ctl::ComPtr<DependencyObject> spTransitionDO;
        ctl::ComPtr<DependencyObject> spElementDO;
        DXamlCore* pCore = DXamlCore::GetCurrent();
        HRESULT getPeerHR = pCore->GetPeer(nativeTransition, &spTransitionDO);
        if (getPeerHR == E_FAIL)
        {
            // We're seeing crashes in Todo and Sticky Notes caused by a failure to get the managed peer for the
            // transition. For now ignore these transitions just like we would if the element's peer was not

            // available.
            // Bug 29565659: [Watson Failure] caused by STOWED_EXCEPTION_8000ffff_Windows.UI.Xaml.dll!Microsoft::WRL::Details::DelegateArgTraits_long_
            goto Cleanup;
        }
        else
        {
            IFC(getPeerHR);
        }
        IFC(spTransitionDO.As(&spTransitionAsITransitionPrivate));
        IFC(pCore->TryGetPeer(nativeUIElement, &spElementDO));
        if (!spElementDO)
        {
            // Peer may not be reachable if element has been removed from the visual tree
            goto Cleanup;
        }
        IFC(spElementDO.As(&spElementAsI));
    }

    IFCFAILFAST(spTransitionAsITransitionPrivate->CreateStoryboard(spElementAsI.Get(), start, destination, trigger, spStoryboards.Get(), &transitionToParent));

    IFC(spStoryboards->get_Size(&cReturnedStoryboardCount));
    ppStoryboardArray = new(ZERO_MEM_FAIL_FAST) CStoryboard*[cReturnedStoryboardCount]();
    for (UINT index = 0; index < cReturnedStoryboardCount; index++)
    {
        ctl::ComPtr<xaml_animation::IStoryboard> spStoryboardAsI;
        IFC(spStoryboards->GetAt(index, &spStoryboardAsI));
        ppStoryboardArray[index] = static_cast<CStoryboard*>(spStoryboardAsI.Cast<Storyboard>()->GetHandleAddRef());
    }
    *cStoryboardCount = cReturnedStoryboardCount;
    *pppStoryboardArray = ppStoryboardArray;  // they are responsible for cleaning up.
    *parentToTransitionEnum = static_cast<DirectUI::TransitionParent>(transitionToParent);
    ppStoryboardArray = nullptr;

    #ifdef LT_DEBUG
    WCHAR szTrace[256];
    IFCEXPECT(swprintf_s(szTrace, 256, L"==== Ended creation of storyboards for %p, transitionparent = %d, nr. of storyboards = %d", spElementAsI.Get(), transitionToParent, cReturnedStoryboardCount) >= 0);
    Trace(szTrace);
    #endif

Cleanup:
    if (FAILED(hr) && ppStoryboardArray)
    {
        for (UINT index = 0; index < cReturnedStoryboardCount; index++)
        {
            ReleaseInterfaceNoNULL(ppStoryboardArray[index]);
        }
        delete[] ppStoryboardArray;
    }
    RRETURN(hr);
}

// static
_Check_return_ HRESULT Transition::NotifyLayoutTransitionStart(
    _In_ CDependencyObject* nativeUIElement)
{
    ctl::ComPtr<IUIElement> spElementAsI;
    ctl::ComPtr<DependencyObject> spElementDO;

    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFC_RETURN(pCore->GetPeer(nativeUIElement, &spElementDO));
    IFC_RETURN(spElementDO.As(&spElementAsI));

    auto spNotification = spElementAsI.AsOrNull<ILayoutTransitionStoryboardNotification>();
    if (spNotification)
    {
        IFC_RETURN(spNotification->NotifyLayoutTransitionStart());
    }

    return S_OK;
}

// static
_Check_return_ HRESULT Transition::NotifyLayoutTransitionEnd(
    _In_ CDependencyObject* nativeUIElement)
{
    ctl::ComPtr<IUIElement> spElementAsI;
    ctl::ComPtr<DependencyObject> spElementDO;

    DXamlCore* pCore = DXamlCore::GetCurrent();

    IFC_RETURN(pCore->GetPeer(nativeUIElement, &spElementDO));
    IFC_RETURN(spElementDO.As(&spElementAsI));

    auto spNotification = spElementAsI.AsOrNull<ILayoutTransitionStoryboardNotification>();
    if (spNotification)
    {
        IFC_RETURN(spNotification->NotifyLayoutTransitionEnd());
    }

    return S_OK;
}

namespace DirectUI
{
// helper that checks for the contextprovider interface having been implemented and if so, calling it.
_Check_return_ HRESULT GetTransitionContext(
    _In_ xaml::IUIElement* element,
    _Out_ BOOLEAN* pRelevantInformation,
    _Out_ ThemeTransitionContext* pContext)
{
    HRESULT hr = S_OK;
    ITransitionContextProvider* pElementAsProvider = ctl::query_interface<ITransitionContextProvider>(element);
    *pContext = ThemeTransitionContext::None;
    *pRelevantInformation = FALSE;
    INT layoutTick = 0;

    IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(static_cast<UIElement*>(element)->GetHandle()), (XINT16*)&layoutTick));

    if (pElementAsProvider)
    {
        IFC(pElementAsProvider->GetCurrentTransitionContext(layoutTick, pContext));
        *pRelevantInformation = TRUE;
    }

Cleanup:
    ReleaseInterface(pElementAsProvider);
    RRETURN(hr);
}
}
_Check_return_ HRESULT GetSpeedOfChanges(
    _In_ xaml::IUIElement* element,
    _Out_ BOOLEAN* pFast)
{
    HRESULT hr = S_OK;
    ITransitionContextProvider* pElementAsProvider = ctl::query_interface<ITransitionContextProvider>(element);

    *pFast = FALSE;

    if (pElementAsProvider)
    {
        IFC(pElementAsProvider->IsCollectionMutatingFast(pFast));
    }

Cleanup:
    ReleaseInterface(pElementAsProvider);
    RRETURN(hr);
}

// helper that allow extra processing of the transition parent
static void SetTransitionParentForStickyHeaders(
    _In_ xaml::IUIElement* element,
    _Inout_ xaml::TransitionParent* parentForTransition)
{
    // transitions should not be aware of things like stickyheaders, however, this code already crept into addDeleteThemeTransition and
    // we need to escape the sticky header clip that is put on the panel. To limit the impact, i'm only changing the parent for
    // these headers.
    UIElement::VirtualizationInformation *pVirtualizationInformation = static_cast<UIElement*>(element)->GetVirtualizationInformation();
    if (pVirtualizationInformation != nullptr)
    {
        if (pVirtualizationInformation->GetIsHeader())
        {
            *parentForTransition = xaml::TransitionParent_ParentToGrandParent;
        }
    }
}

_Check_return_ HRESULT RepositionThemeTransition::PrepareState()
{
    HRESULT hr = S_OK;

    StaggerFunctionBase* pStagger = NULL;
    IFC(RepositionThemeTransitionGenerated::PrepareState());
    IFC(ThemeGenerator::GetStaggerFunction(TAS_REPOSITION, TA_REPOSITION_TARGET, &pStagger));

    IFC(put_GeneratedStaggerFunction(pStagger));

    // Force use of staggering.
    IFC(CoreImports::Transition_SetIsStaggeringEnabled(static_cast<CTransition*>(GetHandle()), true));

Cleanup:
    ctl::release_interface(pStagger);
    RRETURN(hr);
}

_Check_return_ HRESULT RepositionThemeTransition::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    IFC_RETURN(RepositionThemeTransitionGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::RepositionThemeTransition_IsStaggeringEnabled:
        IFC_RETURN(CoreImports::Transition_SetIsStaggeringEnabled(static_cast<CTransition*>(GetHandle()), args.m_pNewValue->AsBool()));
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT RepositionThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = transitionTrigger == xaml::TransitionTrigger_Layout || transitionTrigger == xaml::TransitionTrigger_Reparent;

    RRETURN(hr);
}

_Check_return_ HRESULT RepositionThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    wf::Point sourceoffset = {start.X - destination.X, start.Y - destination.Y};
    wf::Point destoffset = {0,0};
    ctl::ComPtr<Storyboard> spSB;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;

    IFC(ctl::make(&spSB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spSB->get_Children(&spChildren));

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_REPOSITION, TA_REPOSITION_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));

    IFC(storyboards->Append(spSB.Get()));

    *parentForTransition = transitionTrigger == xaml::TransitionTrigger_Reparent ? xaml::TransitionParent_ParentToRoot : xaml::TransitionParent_ParentToCommonParent;

Cleanup:
    RRETURN(hr);
}

// InputPaneThemeTransition
_Check_return_ HRESULT InputPaneThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = transitionTrigger == xaml::TransitionTrigger_Layout && IsInputPaneTransitionEnabled();

    return S_OK;
}

_Check_return_ HRESULT InputPaneThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    wf::Point sourceoffset = {start.X - destination.X, start.Y - destination.Y};
    wf::Point destoffset = {0,0};
    ctl::ComPtr<Storyboard> spSB;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;

    IFC(ctl::make(&spSB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spSB->get_Children(&spChildren));

    if (IsInputPaneShow())
    {
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_SHOWPANEL, TA_SHOWPANEL_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));
    }
    else
    {
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_HIDEPANEL, TA_HIDEPANEL_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));
    }

    IFC(storyboards->Append(spSB.Get()));

    *parentForTransition = xaml::TransitionParent_ParentToRoot;

    // InputPane transition is already kicked off
    m_isInputPaneTransitionEnabled = FALSE;

Cleanup:
    RRETURN(hr);
}

// MenuPopupThemeTransition
_Check_return_ HRESULT MenuPopupThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload;

    return S_OK;
}

_Check_return_ HRESULT MenuPopupThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    const wf::Point nullPoint = { 0, 0 };
    TimingFunctionDescription linear = TimingFunctionDescription();
    ctl::ComPtr<Storyboard> spStoryboard;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelines;

    IFC(ctl::make(&spStoryboard));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spStoryboard.Get()->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spStoryboard->get_Children(&spTimelines));

    if (transitionTrigger == xaml::TransitionTrigger_Load)
    {
        TimingFunctionDescription easing = TimingFunctionDescription();
        easing.cp3.X = 0.0f; // Cubic-bezier (0, 0, 0, 1). Default TimingFunctionDescription() constructor creates a Linear curve (0,0,0,0,1,1,1,1).
        double openedLength = 0.0;
        double closedRatio = 0.0;
        xaml_primitives::AnimationDirection direction = xaml_primitives::AnimationDirection_Top;
        ctl::ComPtr<IActivationFactory> spActivationFactory;
        ctl::ComPtr<xaml_animation::IStoryboardStatics> spStoryboardStatics;
        ctl::ComPtr<IUIElement> spElement(element);

        ThemeGeneratorHelper themeSupplier(nullPoint, nullPoint, nullptr, nullptr, FALSE, spTimelines.Get());
        IFC(themeSupplier.Initialize());

        spActivationFactory.Attach(ctl::ActivationFactoryCreator<StoryboardFactory>::CreateActivationFactory());
        IFC(spActivationFactory.As(&spStoryboardStatics));

        IFC(get_OpenedLength(&openedLength));
        IFC(get_Direction(&direction));
        IFC(get_ClosedRatio(&closedRatio));

        ASSERT(direction == xaml_primitives::AnimationDirection::AnimationDirection_Top ||
               direction == xaml_primitives::AnimationDirection::AnimationDirection_Bottom);

        const double initialTranslateY =
            openedLength * closedRatio *
            ((direction == xaml_primitives::AnimationDirection::AnimationDirection_Bottom) ? 1.0 : -1.0);

        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipTranslateYPropertyName(), -initialTranslateY, 0, 0, &easing));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipTranslateYPropertyName(), 0.0, 0, MenuPopupThemeTransition::s_OpenDuration, &easing));

        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetTranslateYPropertyName(), initialTranslateY, 0, 0, &easing));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetTranslateYPropertyName(), 0.0, 0, MenuPopupThemeTransition::s_OpenDuration, &easing));

        // Border animation
        {
            ctl::ComPtr<MenuFlyoutPresenter> presenter = spElement.AsOrNull<MenuFlyoutPresenter>();

            // For windowed MenuFlyout popups, the transition targets the grand child of the popup.
            // In that case, the parent of spElement in the MenuFlyoutPresenter instance.
            if (!presenter)
            {
                ctl::ComPtr<IDependencyObject> presenterAsDO;
                IFC(VisualTreeHelper::GetParentStatic(static_cast<UIElement*>(spElement.Get()), &presenterAsDO));
                presenter = presenterAsDO.AsOrNull<MenuFlyoutPresenter>();
            }

            if (presenter)
            {
                ctl::ComPtr<xaml::IDependencyObject> spBorderAsDO;
                IFC(presenter->GetTemplateChild(wrl_wrappers::HStringReference(L"MenuFlyoutPresenterBorder").Get(), &spBorderAsDO));
                if (spBorderAsDO)
                {
                    ThemeGeneratorHelper borderThemeSupplier(nullPoint, nullPoint, nullptr, spBorderAsDO.Get(), false, spTimelines.Get());
                    IFC(borderThemeSupplier.Initialize());

                    IFC(borderThemeSupplier.RegisterKeyFrame(borderThemeSupplier.GetScaleYPropertyName(), 1.0 - closedRatio, 0, 0, &easing));
                    IFC(borderThemeSupplier.RegisterKeyFrame(borderThemeSupplier.GetScaleYPropertyName(), 1.0, 0, MenuPopupThemeTransition::s_OpenDuration, &easing));

                    if (direction == xaml_primitives::AnimationDirection::AnimationDirection_Top)
                    {
                        IFC(borderThemeSupplier.RegisterKeyFrame(borderThemeSupplier.GetCenterYPropertyName(), openedLength, 0, 0, nullptr /* pEasing */));
                    }
                }
            }
        }

        // If this transition has an associated overlay, then animate that as well.
        if (m_tpOverlayElement)
        {
            ctl::ComPtr<xaml::IDependencyObject> overlayElementDO;
            IFC(m_tpOverlayElement.As(&overlayElementDO));

            ThemeGeneratorHelper themeSupplier2(nullPoint /*startOffset*/, nullPoint /*destinationOffset*/, nullptr /*targetName*/, overlayElementDO.Get(), FALSE /*onlyGenerateSteadyState*/, spTimelines.Get());
            IFC(themeSupplier2.Initialize());

            IFC(themeSupplier2.RegisterKeyFrame(themeSupplier2.GetOpacityPropertyName(), 0.0 /*value*/, 0 /*begintime*/, 0 /*duration*/, &linear));
            IFC(themeSupplier2.RegisterKeyFrame(themeSupplier2.GetOpacityPropertyName(), 1.0 /*value*/, 0 /*begintime*/, MenuPopupThemeTransition::s_OpacityChangeDuration, &linear));
        }
        
#ifdef MPTT_DEBUG
        IGNOREHR(DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MPTT[0x%p]: CreateStoryboardImpl for TransitionTrigger_Load - direction:%d, openedLength:%f.", this, direction, openedLength));
#endif // MPTT_DEBUG
    }
    else if (transitionTrigger == xaml::TransitionTrigger_Unload)
    {
        ThemeGeneratorHelper themeSupplier(nullPoint, nullPoint, nullptr, nullptr, FALSE, spTimelines.Get());
        IFC(themeSupplier.Initialize());

        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, 0, &linear));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0, 0, MenuPopupThemeTransition::s_OpacityChangeDuration, &linear));

        // If the unload transition interrupts the load transition, the transition system knows how to start the new transition at the location
        // of where it got interrupted. It doesn't handle the clipping though. We need to do that ourselves.
        const LayoutTransitionStorage* storage = static_cast<CUIElement*>(static_cast<UIElement*>(element)->GetHandle())->GetLayoutTransitionStorage();
        if (storage)
        {
            const float clipOffset = (storage->m_arrangeOutput.Y - storage->m_transformStart.GetDy());
            if (clipOffset != 0.0f)
            {
                IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipTranslateYPropertyName(), clipOffset, 0, 0, nullptr /* pEasing */));
                IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipTranslateYPropertyName(), clipOffset, 0, MenuPopupThemeTransition::s_OpacityChangeDuration, nullptr /* pEasing */));
            }
        }

        // If this transition has an associated overlay, then animate that as well.
        if (m_tpOverlayElement)
        {
            ctl::ComPtr<xaml::IDependencyObject> overlayElementDO;
            IFC(m_tpOverlayElement.As(&overlayElementDO));

            ThemeGeneratorHelper themeSupplier2(nullPoint /*startOffset*/, nullPoint /*destinationOffset*/, nullptr /*targetName*/, overlayElementDO.Get(), FALSE /*onlyGenerateSteadyState*/, spTimelines.Get());
            IFC(themeSupplier2.Initialize());

            IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0 /*value*/, 0 /*begintime*/, 0 /*duration*/, &linear));
            IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0 /*value*/, 0 /*begintime*/, MenuPopupThemeTransition::s_OpacityChangeDuration, &linear));
        }
#ifdef MPTT_DEBUG
        IGNOREHR(DebugTrace(XCP_TRACE_OUTPUT_MSG /*traceType*/, L"MPTT[0x%p]: CreateStoryboardImpl for TransitionTrigger_Unload.", this));
#endif // MPTT_DEBUG
    }

    IFC(storyboards->Append(spStoryboard.Get()));

Cleanup:
    RRETURN(hr);
}

// PickerFlyoutThemeTransition
_Check_return_ HRESULT PickerFlyoutThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload;

    return S_OK;
}

_Check_return_ HRESULT PickerFlyoutThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    const wf::Point nullPoint = { 0, 0 };
    const wf::Point splitOrigin = { 0, 0.5 };
    DOUBLE openedLength = 0.0;
    DOUBLE offsetFromCenter = 0.0;
    TimingFunctionDescription easing = TimingFunctionDescription();
    easing.cp3.X = 0.0f; // Cubic-bezier (0, 0, 0, 1). Default TimingFunctionDescription() constructor creates a Linear curve (0,0,0,0,1,1,1,1).

    ctl::ComPtr<Storyboard> spStoryboard;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelines;

    IFC_RETURN(ctl::make(&spStoryboard));
    IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spStoryboard.Get()->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC_RETURN(spStoryboard->get_Children(&spTimelines));

    IFC_RETURN(get_OpenedLength(&openedLength));
    IFC_RETURN(get_OffsetFromCenter(&offsetFromCenter));

    if (openedLength == 0)
    {
        return S_OK;
    }

    DOUBLE initialClipScaleY = 0.0;
    DOUBLE finalClipScaleY = 0.0;

    if (transitionTrigger == xaml::TransitionTrigger_Load)
    {
        const DOUBLE closedRatio = 0.50;
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

        finalClipScaleY = (0.5 + DoubleUtil::Abs(offsetFromCenter / openedLength)) * 2;

        ThemeGeneratorHelper themeSupplier(nullPoint, nullPoint, nullptr, nullptr, FALSE, spTimelines.Get());
        IFC_RETURN(themeSupplier.Initialize());

        IFC_RETURN(themeSupplier.SetClipOriginValues(splitOrigin));   // to get the same speed going up and down, we always use 0.5 for this animation

        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipScaleYPropertyName(), initialClipScaleY, 0, 0, &easing));
        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipScaleYPropertyName(), finalClipScaleY, 0, PickerFlyoutThemeTransition::s_OpenDuration, &easing));
        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipTranslateYPropertyName(), offsetFromCenter, 0, 0, &easing));  // immediately go there

        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, 0, &easing));    // be fully opaque
    }
    else if (transitionTrigger == xaml::TransitionTrigger_Unload)
    {
        const DOUBLE closedRatio = 0.15;
        DOUBLE clipLength = openedLength * closedRatio;
        DOUBLE maxOffset = openedLength * (1 - closedRatio) / 2.0;   // Max offset possible before the clip is partially off the element.
        TimingFunctionDescription linear = TimingFunctionDescription();

        initialClipScaleY = (0.5 + DoubleUtil::Abs(offsetFromCenter / openedLength)) * 2;

        if (DoubleUtil::Abs(offsetFromCenter) > maxOffset)
        {
            DOUBLE pixelsOff = (clipLength / 2.0) - (openedLength / 2.0 - DoubleUtil::Abs(offsetFromCenter));
            finalClipScaleY = pixelsOff / openedLength * 2.0 + closedRatio;
        }
        else
        {
            finalClipScaleY = closedRatio;
        }

        ThemeGeneratorHelper themeSupplier(nullPoint, nullPoint, nullptr, nullptr, FALSE, spTimelines.Get());
        IFC_RETURN(themeSupplier.Initialize());

        IFC_RETURN(themeSupplier.SetClipOriginValues(splitOrigin));

        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipScaleYPropertyName(), initialClipScaleY, 0, 0, &easing));
        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipScaleYPropertyName(), finalClipScaleY, 0, PickerFlyoutThemeTransition::s_CloseDuration, &easing));
        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipTranslateYPropertyName(), offsetFromCenter, 0, 0, &easing));  // immediately go there

        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, 0, &linear));
        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, PickerFlyoutThemeTransition::s_OpacityChangeBeginTime, &linear));
        IFC_RETURN(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0, PickerFlyoutThemeTransition::s_OpacityChangeBeginTime, PickerFlyoutThemeTransition::s_OpacityChangeDuration, &linear));
    }

    IFC_RETURN(storyboards->Append(spStoryboard.Get()));

    return S_OK;
}

_Check_return_ HRESULT ContentThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = TRUE;
    return S_OK;
}

_Check_return_ HRESULT ContentThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldRun = FALSE;
    wf::Point startLocation = {start.X - destination.X, start.Y - destination.Y};
    wf::Point destinationLocation = {0,0};
    BOOLEAN didCheck = FALSE;
    ThemeTransitionContext context = ThemeTransitionContext::None;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spPrimaryChildren;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spSecondaryChildren;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;

    ctl::ComPtr<Storyboard> spPrimarySB;
    ctl::ComPtr<Storyboard> spSecondarySB;

    wf::Point destinationLocationOffset;
    DOUBLE x;
    DOUBLE y;
    IFC(get_HorizontalOffset(&x));
    IFC(get_VerticalOffset(&y));

    destinationLocationOffset.X = (FLOAT)x;
    destinationLocationOffset.Y = (FLOAT)y;

    shouldRun = TRUE;

    if (shouldRun)
    {
        IFC(GetTransitionContext(element, &didCheck, &context));
    }
    if (didCheck)
    {
        shouldRun &= context == ThemeTransitionContext::ContentTransition;
    }

    if (!shouldRun)
    {
        goto Cleanup;
    }

    IFC(ctl::make(&spPrimarySB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spPrimarySB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spPrimarySB->get_Children(&spPrimaryChildren));
    IFC(storyboards->Append(spPrimarySB.Get()));

    IFC(ctl::make(&spSecondarySB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSecondarySB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spSecondarySB->get_Children(&spSecondaryChildren));

    spChildren = spPrimaryChildren;

    if (transitionTrigger != xaml::TransitionTrigger_Load)
    {
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_TRANSITIONCONTENT, TA_TRANSITIONCONTENT_OUTGOING, NULL, NULL, FALSE, startLocation, startLocation, spChildren.Get()));
        spChildren = spSecondaryChildren;
    }

    if (transitionTrigger != xaml::TransitionTrigger_Unload)
    {
        if (spChildren == spSecondaryChildren)
        {
            IFC(storyboards->Append(spSecondarySB.Get()));
        }
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_TRANSITIONCONTENT, TA_TRANSITIONCONTENT_INCOMING, NULL, NULL, FALSE, destinationLocationOffset, destinationLocation, spChildren.Get()));
    }

    SetTransitionParentForStickyHeaders(element, parentForTransition);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PopupThemeTransition::CreateReaderboardStoryboards(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
{
    // DEAD_CODE_REMOVAL: This should never execute anymore
    XAML_FAIL_FAST();

    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spElement (element);

    if (ctl::is<IFrameworkElement>(spElement))
    {
        // The proposed segment heights are designed based on a 480x800 screen.
        // These numbers must be adjusted accordingly on different resolutions.
        const XFLOAT targetScreenHeight = 800.0;
        const XFLOAT defaultSegmentHeight = 112.0;
        const XFLOAT segmentHeightForDatePickerAndTimePicker = 162.0;
        const XFLOAT segmentHeightForListPickerAndContentDialog = 72.0;
        const XFLOAT pixelPaddingForSegments = 1.0;

        wrl_wrappers::HString className;
        wrl_wrappers::HStringReference datePickerFlyoutPresenterClassName(L"Microsoft.UI.Xaml.Controls.DatePickerFlyoutPresenter");
        wrl_wrappers::HStringReference timePickerFlyoutPresenterClassName(L"Microsoft.UI.Xaml.Controls.TimePickerFlyoutPresenter");
        wrl_wrappers::HStringReference listPickerFlyoutPresenterClassName(L"Microsoft.UI.Xaml.Controls.ListPickerFlyoutPresenter");
        wrl_wrappers::HStringReference contentDialogClassName(L"Microsoft.UI.Xaml.Controls.ContentDialog");
        wrl_wrappers::HStringReference centerOfRotationYPropertyName(L"(UIElement.Projection).(PlaneProjection.CenterOfRotationY)");
        wrl_wrappers::HStringReference rotationXPropertyName(L"(UIElement.Projection).(PlaneProjection.RotationX)");
        ctl::ComPtr<DependencyObject> spTemplatedParentAsDO;
        ctl::ComPtr<FrameworkElement> spElementAsFE = spElement.AsOrNull<IFrameworkElement>().Cast<FrameworkElement>();
        wf::Point offset = {0, 0};
        wf::Rect segmentRectInPhysicalPixels = { 0, 0, 0, defaultSegmentHeight };
        wf::Rect segmentRectInLogicalPixels = { 0, 0 };
        BOOLEAN isPartialScreenDialog = FALSE;
        DOUBLE dialogHeight = 0.0;

        // Used for measurement.
        DOUBLE elementHeight;
        UINT32 numberOfSegments;

        // Used for the generation of timelines.
        DOUBLE clipScaleYFactor;
        DOUBLE clipTranslation;
        DOUBLE centerOfRotation;

        spElementAsFE->get_ActualHeight(&elementHeight);
        elementHeight = (elementHeight > 0) ? elementHeight : 1; // Avoid possible divisions by zero.

        // TODO: PopupThemeTransition should have a public API that will let us
        // specify the height of each segment per control. Since this API does not
        // exist, we manually modify this number for our first-party controls. Some of 
        // these exist in MUXP, though, so we will check using the runtime class name. 
        // Note that using this method, we cannot check for SemanticZoom, since the 
        // "ZoomedOutContent" can be anything.
        // Ideally, the segment height for a JumpGrid would be 112px and for a JumpList
        // it would be 75px. To make up for that as best as we can, the default
        // segment height for the readerboard animation will be JumpGrid's 112px.
        IFC(spElement->GetRuntimeClassName(className.GetAddressOf()));

        if (wrl_wrappers::HStringReference(datePickerFlyoutPresenterClassName) == className || wrl_wrappers::HStringReference(timePickerFlyoutPresenterClassName) == className)
        {
            segmentRectInPhysicalPixels.Height = segmentHeightForDatePickerAndTimePicker;
        }
        else if (wrl_wrappers::HStringReference(listPickerFlyoutPresenterClassName) == className || wrl_wrappers::HStringReference(contentDialogClassName) == className)
        {
            segmentRectInPhysicalPixels.Height = segmentHeightForListPickerAndContentDialog;

            if (ctl::is<IContentDialog>(spElement))
            {
                ctl::ComPtr<ContentDialog> spContentDialog = spElement.AsOrNull<IContentDialog>().Cast<ContentDialog>();

                BOOLEAN fullSizeDesired = FALSE;
                IFC(spContentDialog->get_FullSizeDesired(&fullSizeDesired));

                isPartialScreenDialog = !fullSizeDesired;

                IFC(spContentDialog->GetDialogHeight(&dialogHeight));
            }
        }

        // If the ContentDialog is defined in XAML, the element to which readerboard will
        // be applied is a template part of the ContentDialog instead of the ContentDialog
        // itself. We will now get the templated parent to corroborate if this belongs to
        // a ContentDialog or not.
        IFC(spElementAsFE->get_TemplatedParent(&spTemplatedParentAsDO));

        if (ctl::is<IContentDialog>(spTemplatedParentAsDO))
        {
            ctl::ComPtr<ContentDialog> spTemplatedParentAsContentDialog = spTemplatedParentAsDO.AsOrNull<IContentDialog>().Cast<ContentDialog>();

            BOOLEAN fullSizeDesired = FALSE;
            IFC(spTemplatedParentAsContentDialog->get_FullSizeDesired(&fullSizeDesired));

            isPartialScreenDialog = !fullSizeDesired;

            IFC(spTemplatedParentAsContentDialog->GetDialogHeight(&dialogHeight))
            segmentRectInPhysicalPixels.Height = segmentHeightForListPickerAndContentDialog;
        }

        // If this is a partial screen ContentDialog, we must play the
        // swivel animation. Otherwise, we will play the readerboard animation.
        if (isPartialScreenDialog)
        {
            ctl::ComPtr<Storyboard> spStoryboard;
            ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelines;

            IFC(ctl::make(&spStoryboard));
            IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spStoryboard.Get()->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
            IFC(spStoryboard->get_Children(&spTimelines));

            ThemeGeneratorHelper themeSupplier(offset, offset, nullptr, nullptr, FALSE, spTimelines.Get());
            IFC(themeSupplier.Initialize());

            centerOfRotation = (dialogHeight / 2.0) / elementHeight;

             if (transitionTrigger == xaml::TransitionTrigger_Load)
             {
                const DOUBLE initialAngleForSwivelIn = -45;
                const INT64 durationForSwivelIn = 167;
                TimingFunctionDescription logarithmicEase;

                // Logarithmic easing, 20.
                logarithmicEase.cp1.X = 0.00f;
                logarithmicEase.cp1.Y = 0.00f;
                logarithmicEase.cp2.X = 0.24f;
                logarithmicEase.cp2.Y = 0.57f;
                logarithmicEase.cp3.X = 0.50f;
                logarithmicEase.cp3.Y = 0.92f;
                logarithmicEase.cp4.X = 1.00f;
                logarithmicEase.cp4.Y = 1.00f;

                IFC(themeSupplier.RegisterKeyFrame(centerOfRotationYPropertyName.Get(), centerOfRotation, 0, 0, NULL));

                IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, 0, NULL));

                IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), initialAngleForSwivelIn, 0, 0, &logarithmicEase));
                IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), 0.0, 0, durationForSwivelIn, &logarithmicEase));
             }
             else if (transitionTrigger == xaml::TransitionTrigger_Unload)
             {
                const DOUBLE finalAngleForSwivelOut = 60;
                const INT64 durationForSwivelOut = 100;
                TimingFunctionDescription exponentialEase;

                // Exponential easing, 15.
                exponentialEase.cp1.X = 0.00f;
                exponentialEase.cp1.Y = 0.00f;
                exponentialEase.cp2.X = 0.65f;
                exponentialEase.cp2.Y = 0.15f;
                exponentialEase.cp3.X = 0.85f;
                exponentialEase.cp3.Y = 0.40f;
                exponentialEase.cp4.X = 1.00f;
                exponentialEase.cp4.Y = 1.00f;

                IFC(themeSupplier.RegisterKeyFrame(centerOfRotationYPropertyName.Get(), centerOfRotation, 0, 0, NULL));

                IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, 0, NULL));
                IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0, durationForSwivelOut, 0, NULL));

                IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), 0.0, 0, 0, &exponentialEase));
                IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), finalAngleForSwivelOut, 0, durationForSwivelOut, &exponentialEase));
             }

             IFC(storyboards->Append(spStoryboard.Get()));
        }
        else
        {
            TimingFunctionDescription linearEase;
            XFLOAT physicalScreenHeight;
            XFLOAT scaleFactor = 1.0;
            wf::Rect windowBounds = {};
            const float zoomScale = RootScale::GetRasterizationScaleForElement(GetHandle());

            // Adjust the height of the segment based on the physical screen size.

            IFC(DXamlCore::GetCurrent()->GetContentBoundsForElement(GetHandle(), &windowBounds));
            physicalScreenHeight = static_cast<XFLOAT>(floor(DoubleUtil::Max(windowBounds.Height, windowBounds.Width) * zoomScale));
            scaleFactor = physicalScreenHeight / targetScreenHeight;
            segmentRectInPhysicalPixels.Height *= scaleFactor;

            // Convert the height of the segment to logical pixels and calculate
            // the number of segments and clip scale based on this number.
            // We're adding some padding to the clip scale in order to prevent
            // rounding errors from generating small gaps between the segments.
            const float scale = RootScale::GetRasterizationScaleForElement(GetHandle());
            DXamlCore::GetCurrent()->PhysicalPixelsToDips(scale, &segmentRectInPhysicalPixels, &segmentRectInLogicalPixels);
            numberOfSegments = static_cast<UINT32>(ceil(elementHeight / segmentRectInLogicalPixels.Height));
            clipScaleYFactor = (segmentRectInLogicalPixels.Height + pixelPaddingForSegments) / elementHeight;

            for (UINT32 i = 0; i < numberOfSegments; i++)
            {
                ctl::ComPtr<Storyboard> spStoryboard;
                ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelines;

                // Every storyboard we append to the collection will be registered as a layout transition.
                // As part of the layout run, CLayoutManager::RealizeRegisteredLayoutTransitions will get called to
                // create all these transitions that have been deferred until that moment.
                // As part of this, CTransition::SetupTranstion will get called and create an LTE based on the target
                // of every storyboard. We need one LTE for each segment in the readerboard, thus we will create a new
                // storyboard for each segment and set the original UIElement as the target.
                IFC(ctl::make(&spStoryboard));
                IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spStoryboard.Get()->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
                IFC(spStoryboard->get_Children(&spTimelines));

                ThemeGeneratorHelper themeSupplier(offset, offset, nullptr, nullptr, FALSE, spTimelines.Get());
                IFC(themeSupplier.Initialize());

                clipTranslation = segmentRectInLogicalPixels.Height * i;
                centerOfRotation = (0.5 + i * 1.0) / numberOfSegments;

                // Generate and attach the corresponding clip to the LTE.
                IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipScaleYPropertyName(), clipScaleYFactor, 0, 0, NULL));
                IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetClipTranslateYPropertyName(), clipTranslation, 0, 0, NULL));

                // The center of rotation must be placed at the center of each individual segment.
                IFC(themeSupplier.RegisterKeyFrame(centerOfRotationYPropertyName.Get(), centerOfRotation, 0, 0, NULL));

                if (transitionTrigger == xaml::TransitionTrigger_Load)
                {
                    const DOUBLE initialAngleForReaderboardIn = -80;
                    const INT64 beginTimeForReaderboardIn = 30;
                    const INT64 durationPerSegmentForReaderboardIn = 100;
                    INT64 staggerTimeForReaderboardIn = i * beginTimeForReaderboardIn;

                    IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0, 0, 0, NULL));
                    IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, staggerTimeForReaderboardIn, 0, NULL));

                    IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), initialAngleForReaderboardIn, 0, 0, &linearEase));
                    IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), 0.0, staggerTimeForReaderboardIn, durationPerSegmentForReaderboardIn, &linearEase));

                    // Make the element invisible for hit-testing until it appears on the screen.
                    IFC(themeSupplier.PreventHitTestingWhileAnimating(staggerTimeForReaderboardIn + 1));
                }
                else if (transitionTrigger == xaml::TransitionTrigger_Unload)
                {
                    const DOUBLE finalAngleForReaderboardOut = 70;
                    const INT64 beginTimeForReaderboardOut = 30;
                    const INT64 durationPerSegmentForReaderboardOut = 100;
                    INT64 staggerTimeForReaderboardOut = i * beginTimeForReaderboardOut;
                    INT64 finishTimeForReaderboardOut = staggerTimeForReaderboardOut + durationPerSegmentForReaderboardOut;

                    IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, 0, NULL));
                    IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0, finishTimeForReaderboardOut, 0, NULL));

                    IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), 0.0, staggerTimeForReaderboardOut, 0, &linearEase));
                    IFC(themeSupplier.RegisterKeyFrame(rotationXPropertyName.Get(), finalAngleForReaderboardOut, staggerTimeForReaderboardOut, durationPerSegmentForReaderboardOut, &linearEase));
                }

                IFC(storyboards->Append(spStoryboard.Get()));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PopupThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload;
    return S_OK;
}

_Check_return_ HRESULT PopupThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    wf::Point sourceoffset = {0,0};
    wf::Point destoffset = {0,0};
    ctl::ComPtr<Storyboard> spSB;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;

    DOUBLE x;
    DOUBLE y;

    if (transitionTrigger != xaml::TransitionTrigger_Load && transitionTrigger != xaml::TransitionTrigger_Unload)
    {
        goto Cleanup;
    }

    IFC(get_FromHorizontalOffset(&x));
    IFC(get_FromVerticalOffset(&y));

    IFC(ctl::make(&spSB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spSB->get_Children(&spChildren));

    if (transitionTrigger == xaml::TransitionTrigger_Load)
    {
        sourceoffset.X = (FLOAT)x;
        sourceoffset.Y = (FLOAT)y;

        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_SHOWPOPUP, TA_SHOWPOPUP_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));
    }

    if (transitionTrigger == xaml::TransitionTrigger_Unload)
    {
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_HIDEPOPUP, TA_HIDEPOPUP_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));
    }

    // If this transition has an associated overlay, then animate that as well.
    if (m_tpOverlayElement)
    {
        ctl::ComPtr<xaml::IDependencyObject> overlayElementDO;
        IFC(m_tpOverlayElement.As(&overlayElementDO));

        if (transitionTrigger == xaml::TransitionTrigger_Load)
        {
            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_SHOWPOPUP, TA_SHOWPOPUP_TARGET, NULL, overlayElementDO.Get(), FALSE, { 0, 0 }, { 0, 0 }, spChildren.Get()));
        }

        if (transitionTrigger == xaml::TransitionTrigger_Unload)
        {
            IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_HIDEPOPUP, TA_HIDEPOPUP_TARGET, NULL, overlayElementDO.Get(), FALSE, { 0, 0 }, { 0, 0 }, spChildren.Get()));
        }
    }

    IFC(storyboards->Append(spSB.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT EdgeUIThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload || transitionTrigger == xaml::TransitionTrigger_Layout;
    if (*returnValue && UsePanelAnimationInsteadOfEdgeAnimation())
    {
        INT16 tick = 0;
        IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(static_cast<UIElement*>(element)->GetHandle()), (XINT16*)&tick));
        *returnValue = tick == m_iNextTickToReactTo;
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT EdgeUIThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    wf::Point sourceoffset = {0,0};
    wf::Point destoffset = {0,0};
    ctl::ComPtr<Storyboard> spSB;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;
    xaml_primitives::EdgeTransitionLocation direction = xaml_primitives::EdgeTransitionLocation_Top;

    if (!(transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload || transitionTrigger == xaml::TransitionTrigger_Layout))
    {
        goto Cleanup;
    }
    if (UsePanelAnimationInsteadOfEdgeAnimation())
    {
        INT16 tick = 0;
        IFC(CoreImports::LayoutManager_GetLayoutTickForTransition(static_cast<CUIElement*>(static_cast<UIElement*>(element)->GetHandle()), (XINT16*)&tick));
        if (tick != m_iNextTickToReactTo)
        {
            goto Cleanup;
        }
    }

    #ifdef LT_DEBUG
    WCHAR szTrace[64];
    IFCEXPECT(swprintf_s(szTrace, 64, L"==== Creating EdgeUITransition for %p", element) >= 0);
    Trace(szTrace);
    #endif

    IFC(ctl::make(&spSB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spSB->get_Children(&spChildren));

    IFC(get_Edge(&direction))

    if (transitionTrigger == xaml::TransitionTrigger_Load)
    {
        switch (direction)
        {
        case xaml_primitives::EdgeTransitionLocation_Top:
            sourceoffset.Y -= destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Bottom:
            sourceoffset.Y += destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Right:
            sourceoffset.X += destination.Width;
            break;
        case xaml_primitives::EdgeTransitionLocation_Left:
            sourceoffset.X -= destination.Width;
            break;
        }
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_SHOWEDGEUI, TA_SHOWEDGEUI_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));
    }
    else if (transitionTrigger == xaml::TransitionTrigger_Unload)
    {
        switch (direction)
        {
        case xaml_primitives::EdgeTransitionLocation_Top:
            destoffset.Y -= destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Bottom:
            destoffset.Y += destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Right:
            destoffset.X += destination.Width;
            break;
        case xaml_primitives::EdgeTransitionLocation_Left:
            destoffset.X -= destination.Width;
            break;
        }
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_HIDEEDGEUI, TA_HIDEEDGEUI_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));
    }
    else if (transitionTrigger == xaml::TransitionTrigger_Layout)
    {
        sourceoffset.X = start.X - destination.X;
        sourceoffset.Y = start.Y - destination.Y;
        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
            UsePanelAnimationInsteadOfEdgeAnimation() ? TAS_SHOWPANEL : TAS_SHOWEDGEUI,
            UsePanelAnimationInsteadOfEdgeAnimation() ? TA_SHOWPANEL_TARGET : TA_SHOWEDGEUI_TARGET,
            NULL,
            NULL,
            FALSE,
            sourceoffset,
            destoffset,
            spChildren.Get()));
    }

    IFC(storyboards->Append(spSB.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PaneThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;

    *returnValue = transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload || transitionTrigger == xaml::TransitionTrigger_Layout;

    RRETURN(hr);
}

_Check_return_ HRESULT PaneThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    wf::Point sourceoffset = {0,0};
    wf::Point destoffset = {0,0};
    ctl::ComPtr<Storyboard> spSB;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;
    xaml_primitives::EdgeTransitionLocation direction = xaml_primitives::EdgeTransitionLocation_Left;
    ThemeGeneratorHelper* pClipHelper = NULL;

    if (!(transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload || transitionTrigger == xaml::TransitionTrigger_Layout))
    {
        goto Cleanup;
    }

    #ifdef LT_DEBUG
    WCHAR szTrace[64];
    IFCEXPECT(swprintf_s(szTrace, 64, L"==== Creating PaneThemeTransition for %p", element) >= 0);
    Trace(szTrace);
    #endif

    IFC(ctl::make(&spSB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spSB->get_Children(&spChildren));

    IFC(get_Edge(&direction))

    if (transitionTrigger == xaml::TransitionTrigger_Layout)
    {
        sourceoffset.X = start.X - destination.X;
        sourceoffset.Y = start.Y - destination.Y;
    }
    else if (transitionTrigger == xaml::TransitionTrigger_Load)
    {
        switch (direction)
        {
        case xaml_primitives::EdgeTransitionLocation_Top:
            sourceoffset.Y -= destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Bottom:
            sourceoffset.Y += destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Right:
            sourceoffset.X += destination.Width;
            break;
        case xaml_primitives::EdgeTransitionLocation_Left:
            sourceoffset.X -= destination.Width;
            break;
        }
    }
    else if (transitionTrigger == xaml::TransitionTrigger_Unload)
    {
        switch (direction)
        {
        case xaml_primitives::EdgeTransitionLocation_Top:
            destoffset.Y -= destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Bottom:
            destoffset.Y += destination.Height;
            break;
        case xaml_primitives::EdgeTransitionLocation_Right:
            destoffset.X += destination.Width;
            break;
        case xaml_primitives::EdgeTransitionLocation_Left:
            destoffset.X -= destination.Width;
            break;
        }
    }
    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
        transitionTrigger == xaml::TransitionTrigger_Unload ? TAS_HIDEPANEL : TAS_SHOWPANEL,
        transitionTrigger == xaml::TransitionTrigger_Unload ? TA_HIDEPANEL_TARGET : TA_SHOWPANEL_TARGET,
        NULL,
        NULL,
        FALSE,
        sourceoffset,
        destoffset,
        spChildren.Get()));

    // clipping
    // we are setting up a clip here (that is always relative to the element) with the opposite
    // movement as the element itself. This will make it appear to be 'sliding' from behind.
    //
    // ThemeGenerator is not truly setup for this (there is no PVL with the clip here) so I need
    // to override the targetted properties.
    if (transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload)
    {
        wf::Point clipSourceoffset = sourceoffset;
        wf::Point clipDestoffset = destoffset;
        clipSourceoffset.X *= -1;
        clipSourceoffset.Y *= -1;
        clipDestoffset.X *= -1;
        clipDestoffset.Y *= -1;

        pClipHelper = new ThemeGeneratorHelper(clipSourceoffset, clipDestoffset, NULL, NULL, /*bOnlyGenerateSteadyState*/ FALSE, spChildren.Get());
        IFC(pClipHelper->Initialize());
        IFC(pClipHelper->SetOverrideTranslateXPropertyName(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateX")));
        IFC(pClipHelper->SetOverrideTranslateYPropertyName(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateY")));

        IFC(ThemeGenerator::AddTimelinesForThemeAnimation(
            transitionTrigger == xaml::TransitionTrigger_Load ? TAS_SHOWPANEL : TAS_HIDEPANEL,
            transitionTrigger == xaml::TransitionTrigger_Load ? TA_SHOWPANEL_TARGET : TA_HIDEPANEL_TARGET,
            pClipHelper));
    }

    *parentForTransition = xaml::TransitionParent_ParentToCommonParent;
    IFC(storyboards->Append(spSB.Get()));

Cleanup:
    delete pClipHelper;
    RRETURN(hr);
}

_Check_return_ HRESULT EntranceThemeTransition::PrepareState()
{
    HRESULT hr = S_OK;

    StaggerFunctionBase* pStagger = NULL;
    IFC(EntranceThemeTransitionGenerated::PrepareState());
    IFC(ThemeGenerator::GetStaggerFunction(TAS_ENTERPAGE, TA_ENTERPAGE_TARGET, &pStagger));

    IFC(put_GeneratedStaggerFunction(pStagger));

Cleanup:
    ctl::release_interface(pStagger);
    RRETURN(hr);
}

_Check_return_ HRESULT EntranceThemeTransition::OnPropertyChanged2(
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(EntranceThemeTransitionGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::EntranceThemeTransition_IsStaggeringEnabled:
            IFC(CoreImports::Transition_SetIsStaggeringEnabled(static_cast<CTransition*>(GetHandle()), args.m_pNewValue->AsBool()));
            break;
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT EntranceThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    *returnValue = transitionTrigger == xaml::TransitionTrigger_Load;
    return S_OK;
}

_Check_return_ HRESULT EntranceThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldRun = FALSE;
    wf::Point sourceoffset = {0,0};
    wf::Point destoffset = {0,0};
    ctl::ComPtr<Storyboard> spSB;
    DOUBLE x;
    DOUBLE y;
    BOOLEAN didCheck = FALSE;
    ThemeTransitionContext context = ThemeTransitionContext::None;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spChildren;

    IFC(get_FromHorizontalOffset(&x));
    IFC(get_FromVerticalOffset(&y));

    sourceoffset.X += (FLOAT)x;
    sourceoffset.Y += (FLOAT)y;

    shouldRun = transitionTrigger == xaml::TransitionTrigger_Load;
    if (shouldRun)
    {
        IFC(GetTransitionContext(element, &didCheck, &context));
    }
    if (didCheck)
    {
        shouldRun &= context == ThemeTransitionContext::Entrance;
    }
    else
    {
        // if there was no context, we will only react to a true xaml::TransitionTrigger_Load
        shouldRun = transitionTrigger == xaml::TransitionTrigger_Load;
    }

    if (!shouldRun)
    {
        goto Cleanup;
    }

    #ifdef LT_DEBUG
    WCHAR szTrace[64];
    IFCEXPECT(swprintf_s(szTrace, 64, L"==== Creating EntranceTransition for %p", element) >= 0);
    Trace(szTrace);
    #endif

    IFC(ctl::make(&spSB));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSB->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC(spSB->get_Children(&spChildren));

    IFC(ThemeGenerator::AddTimelinesForThemeAnimation(TAS_ENTERPAGE, TA_ENTERPAGE_TARGET, NULL, NULL, FALSE, sourceoffset, destoffset, spChildren.Get()));

    IFC(storyboards->Append(spSB.Get()));

    SetTransitionParentForStickyHeaders(element, parentForTransition);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AddDeleteRepositionHelperLoad(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
{
    wf::Point sourceoffset = { start.X - destination.X, start.Y - destination.Y };
    wf::Point destoffset = { 0, 0 };
    ctl::ComPtr<Storyboard> spPrimaryStoryboard;

    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*> > spPrimaryChildren;

    ThemeTransitionContext context = ThemeTransitionContext::None;
    BOOLEAN didContextCheck = FALSE;

    // configuration of this animation
    BOOLEAN allowLoadUnload = TRUE;
    BOOLEAN scheduleAffectedFirst = FALSE;   // pvl should define a begintime on the affected here
    BOOLEAN reorderingContext = FALSE;
    INT64   affectedDuration = 300; // todo: pvl might change, do I want to make yet another call to pvl to find out these times?
    bool mixedContext = false;

    IFC_RETURN(GetTransitionContext(element, &didContextCheck, &context));

    UIElement::VirtualizationInformation *pVirtualizationInformation = static_cast<UIElement*>(element)->GetVirtualizationInformation();
    if (pVirtualizationInformation != nullptr)
    {
        DOUBLE offsets[] = { sourceoffset.Y, destoffset.Y };
        if (pVirtualizationInformation->GetIsHeader())
        {
            // We may have to correct the offsets
            ctl::ComPtr<IListViewBaseHeaderItem> spHeader = ctl::query_interface_cast<IListViewBaseHeaderItem>(element);
            if (spHeader)
            {
                IFC_RETURN(spHeader.Cast<ListViewBaseHeaderItem>()->CoerceStickyHeaderOffsets(ARRAYSIZE(offsets), offsets));
                sourceoffset.Y = static_cast<FLOAT>(offsets[0]);
                destoffset.Y = static_cast<FLOAT>(offsets[1]);
            }
        }
    }

    if (didContextCheck)
    {
        allowLoadUnload =
            (context == ThemeTransitionContext::SingleAddList ||
            context == ThemeTransitionContext::SingleDeleteList ||
            context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::SingleDeleteGrid ||
            context == ThemeTransitionContext::MixedOperationsList ||
            context == ThemeTransitionContext::MixedOperationsGrid ||
            context == ThemeTransitionContext::MultipleAddList ||
            context == ThemeTransitionContext::MultipleDeleteList ||
            context == ThemeTransitionContext::MultipleAddGrid ||
            context == ThemeTransitionContext::MultipleDeleteGrid);

        mixedContext =
            (context == ThemeTransitionContext::MixedOperationsList ||
            context == ThemeTransitionContext::MixedOperationsGrid);

        scheduleAffectedFirst =
            (context == ThemeTransitionContext::SingleAddList ||
            context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::MultipleAddList ||
            context == ThemeTransitionContext::MultipleReorderGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleAddGrid);

        reorderingContext =
            (context == ThemeTransitionContext::SingleReorderList ||
            context == ThemeTransitionContext::SingleReorderGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleReorderGrid);
    }
    else
    {
        // no context could be gotten, so we need sane defaults.
        // this is expected for non-moco scenarios, such as a regular stackpanel
        // in practise this means that these transitions should never be used outside of MoCo
        allowLoadUnload = TRUE;
        scheduleAffectedFirst = TRUE;
    }

    // prepare primary storyboard. We hold off on secondary storyboard since it is not always needed
    IFC_RETURN(ctl::make(&spPrimaryStoryboard));
    IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spPrimaryStoryboard->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC_RETURN(spPrimaryStoryboard->get_Children(&spPrimaryChildren));
    IFC_RETURN(storyboards->Append(spPrimaryStoryboard.Get()));

    if (allowLoadUnload)
    {
        INT64 startOffset = scheduleAffectedFirst ? affectedDuration : 0;
        if (mixedContext)
        {
            ASSERT(!scheduleAffectedFirst);
            // first need to work through deletes, then moves. Only then should adds occur
            startOffset = 2 * affectedDuration;
        }

        IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                    TAS_ADDTOGRID,
                    TA_ADDTOGRID_ADDED,
                    FALSE,
                    sourceoffset,
                    destoffset,
                    startOffset,
                    spPrimaryChildren.Get()));
    }
    else if (reorderingContext)
    {
        // set the current transition location for this element
        BOOLEAN bRelevantInformation = FALSE;
        const int targetID = TA_ADDTOGRID_ADDED;

        sourceoffset = destoffset;
        bRelevantInformation = TRUE;

        // setting this variable to true indicates that we have
        // sufficient information to execute the animation
        if (bRelevantInformation)
        {
            IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                        TAS_ADDTOGRID,
                        targetID,
                        FALSE,
                        sourceoffset,
                        destoffset,
                        0,
                        spPrimaryChildren.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT AddDeleteRepositionHelperUnload(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
{
    wf::Point sourceoffset = { start.X - destination.X, start.Y - destination.Y };
    wf::Point destoffset = { 0, 0 };
    ctl::ComPtr<Storyboard> spPrimaryStoryboard;

    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*> > spPrimaryChildren;

    ThemeTransitionContext context = ThemeTransitionContext::None;
    BOOLEAN didContextCheck = FALSE;

    // configuration of this animation
    BOOLEAN allowLoadUnload = TRUE;
    BOOLEAN scheduleAffectedFirst = FALSE;   // pvl should define a begintime on the affected here
    INT64   affectedDuration = 300; // todo: pvl might change, do I want to make yet another call to pvl to find out these times?

    IFC_RETURN(GetTransitionContext(element, &didContextCheck, &context));

    UIElement::VirtualizationInformation *pVirtualizationInformation = static_cast<UIElement*>(element)->GetVirtualizationInformation();
    if (pVirtualizationInformation != nullptr)
    {
        DOUBLE offsets[] = { sourceoffset.Y, destoffset.Y };
        if (pVirtualizationInformation->GetIsHeader())
        {
            // We may have to correct the offsets
            ctl::ComPtr<IListViewBaseHeaderItem> spHeader = ctl::query_interface_cast<IListViewBaseHeaderItem>(element);
            if (spHeader)
            {
                IFC_RETURN(spHeader.Cast<ListViewBaseHeaderItem>()->CoerceStickyHeaderOffsets(ARRAYSIZE(offsets), offsets));
                sourceoffset.Y = static_cast<FLOAT>(offsets[0]);
                destoffset.Y = static_cast<FLOAT>(offsets[1]);
            }
        }
    }

    if (didContextCheck)
    {
        allowLoadUnload =
            (context == ThemeTransitionContext::SingleAddList ||
            context == ThemeTransitionContext::SingleDeleteList ||
            context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::SingleDeleteGrid ||
            context == ThemeTransitionContext::MixedOperationsList ||
            context == ThemeTransitionContext::MixedOperationsGrid ||
            context == ThemeTransitionContext::MultipleAddList ||
            context == ThemeTransitionContext::MultipleDeleteList ||
            context == ThemeTransitionContext::MultipleAddGrid ||
            context == ThemeTransitionContext::MultipleDeleteGrid);

        scheduleAffectedFirst =
            (context == ThemeTransitionContext::SingleAddList ||
            context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::MultipleAddList ||
            context == ThemeTransitionContext::MultipleReorderGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleAddGrid);
    }
    else
    {
        // no context could be gotten, so we need sane defaults.
        // this is expected for non-moco scenarios, such as a regular stackpanel
        // in practise this means that these transitions should never be used outside of MoCo
        allowLoadUnload = TRUE;
        scheduleAffectedFirst = TRUE;
    }

    // prepare primary storyboard. We hold off on secondary storyboard since it is not always needed
    IFC_RETURN(ctl::make(&spPrimaryStoryboard));
    IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spPrimaryStoryboard->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC_RETURN(spPrimaryStoryboard->get_Children(&spPrimaryChildren));
    IFC_RETURN(storyboards->Append(spPrimaryStoryboard.Get()));

    if (allowLoadUnload)    // the reorder transition does no animation here
    {
        IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                    TAS_DELETEFROMGRID,
                    TA_DELETEFROMGRID_DELETED,
                    FALSE,
                    sourceoffset,
                    destoffset,
                    scheduleAffectedFirst ? affectedDuration : 0,
                    spPrimaryChildren.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT AddDeleteRepositionHelperReparentAndLayout(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards)
{
    wf::Point sourceoffset = { start.X - destination.X, start.Y - destination.Y };
    TimingFunctionDescription linear = TimingFunctionDescription();
    wf::Point destoffset = { 0, 0 };

    ctl::ComPtr<Storyboard> spPrimaryStoryboard;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*> > spPrimaryChildren;

    ThemeTransitionContext context = ThemeTransitionContext::None;
    BOOLEAN didContextCheck = FALSE;

    // configuration of this animation
    BOOLEAN allowPortalling = FALSE;
    BOOLEAN scheduleAffectedFirst = FALSE;   // pvl should define a begintime on the affected here
    BOOLEAN addingContext = FALSE;
    BOOLEAN deletingContext = FALSE;
    BOOLEAN reorderingContext = FALSE;
    BOOLEAN multiple = FALSE;
    BOOLEAN grid = FALSE;
    bool mixedContext = false;
    INT64   loadDuration = 220;

    IFC_RETURN(GetTransitionContext(element, &didContextCheck, &context));

    UIElement::VirtualizationInformation *pVirtualizationInformation = static_cast<UIElement*>(element)->GetVirtualizationInformation();
    if (pVirtualizationInformation != nullptr)
    {
        DOUBLE offsets[] = { sourceoffset.Y, destoffset.Y };
        if (pVirtualizationInformation->GetIsHeader())
        {
            // We may have to correct the offsets
            ctl::ComPtr<IListViewBaseHeaderItem> spHeader = ctl::query_interface_cast<IListViewBaseHeaderItem>(element);
            if (spHeader)
            {
                IFC_RETURN(spHeader.Cast<ListViewBaseHeaderItem>()->CoerceStickyHeaderOffsets(ARRAYSIZE(offsets), offsets));
                sourceoffset.Y = static_cast<FLOAT>(offsets[0]);
                destoffset.Y = static_cast<FLOAT>(offsets[1]);
            }
        }
    }

    if (didContextCheck)
    {
        allowPortalling =
            (context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::SingleDeleteGrid ||
            context == ThemeTransitionContext::SingleReorderGrid);

        mixedContext =
            (context == ThemeTransitionContext::MixedOperationsGrid ||
            context == ThemeTransitionContext::MixedOperationsList);

        scheduleAffectedFirst =
            (context == ThemeTransitionContext::SingleAddList ||
            context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::MultipleAddList ||
            context == ThemeTransitionContext::MultipleReorderGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleAddGrid);

        addingContext =
            (context == ThemeTransitionContext::SingleAddList ||
            context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::MultipleAddList ||
            context == ThemeTransitionContext::MultipleAddGrid);

        deletingContext =
            (context == ThemeTransitionContext::SingleDeleteList ||
            context == ThemeTransitionContext::SingleDeleteGrid ||
            context == ThemeTransitionContext::MultipleDeleteList ||
            context == ThemeTransitionContext::MultipleDeleteGrid);

        reorderingContext =
            (context == ThemeTransitionContext::SingleReorderList ||
            context == ThemeTransitionContext::SingleReorderGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleReorderGrid);

        multiple =
            (context == ThemeTransitionContext::MultipleAddList ||
            context == ThemeTransitionContext::MultipleDeleteList ||
            context == ThemeTransitionContext::MixedOperationsList ||
            context == ThemeTransitionContext::MixedOperationsGrid ||
            context == ThemeTransitionContext::MultipleAddGrid ||
            context == ThemeTransitionContext::MultipleDeleteGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleReorderGrid);

        grid =
            (context == ThemeTransitionContext::SingleAddGrid ||
            context == ThemeTransitionContext::SingleDeleteGrid ||
            context == ThemeTransitionContext::SingleReorderGrid ||
            context == ThemeTransitionContext::MixedOperationsGrid ||
            context == ThemeTransitionContext::MultipleAddGrid ||
            context == ThemeTransitionContext::MultipleDeleteGrid ||
            context == ThemeTransitionContext::MultipleReorderGrid);
    }
    else
    {
        // no context could be gotten, so we need sane defaults.
        // this is expected for non-moco scenarios, such as a regular stackpanel
        // in practise this means that these transitions should never be used outside of MoCo
        allowPortalling = FALSE;
        scheduleAffectedFirst = TRUE;
        addingContext = TRUE;
        multiple = TRUE;
        grid = FALSE;
        mixedContext = FALSE;
    }

    // prepare primary storyboard. We hold off on secondary storyboard since it is not always needed
    IFC_RETURN(ctl::make(&spPrimaryStoryboard));
    IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spPrimaryStoryboard->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
    IFC_RETURN(spPrimaryStoryboard->get_Children(&spPrimaryChildren));
    IFC_RETURN(storyboards->Append(spPrimaryStoryboard.Get()));

    BOOLEAN inTheSameLine = DoubleUtil::Abs(sourceoffset.X) < start.Width || DoubleUtil::Abs(sourceoffset.Y) < start.Height;

    if (!grid || (grid && !multiple && inTheSameLine))
    {
        // straight move when:
        // 1. we are not in a grid
        // 2. we are in a grid, but we are not in mulitple mode _and_ we have not moved columns

        INT storyboardID = 0;
        INT targetID = 0;
        if (addingContext)
        {
            storyboardID = TAS_ADDTOGRID;
            targetID = TA_ADDTOGRID_AFFECTED;
        }
        else if (deletingContext)
        {
            storyboardID = TAS_DELETEFROMGRID;
            targetID = TA_DELETEFROMGRID_REMAINING;
        }
        else if (reorderingContext)
        {
            storyboardID = TAS_ADDTOGRID;
            targetID = TA_ADDTOGRID_AFFECTED; // when this changes, make sure to match up with the storyboards used for the portalling!
        }
        else if (mixedContext)
        {
            // there is no PVL definition for how the affected should move in this case
            storyboardID = TAS_DELETEFROMGRID;
            targetID = TA_DELETEFROMGRID_REMAINING;
        }
        else
        {
            // we have defined sane behavior above. So if we have a different context, this is truly unsupported.
            IFC_RETURN(E_NOT_SUPPORTED);
        }

        // we are setting up storyboards for the affected elements. If they are caused by an add or a reorder, we want them to
        // first move (and then we will fade in the added elements). If they are caused by a delete, we want them to wait with the
        // move. If they are moved because of a mix of add/deletes, we want them to wait.
        IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                    storyboardID,
                    targetID,
                    FALSE /* onlyGenerateSteadyState */,
                    sourceoffset,
                    destoffset,
                    (scheduleAffectedFirst || reorderingContext) && !mixedContext ? 0 : loadDuration,
                    spPrimaryChildren.Get()));
    }
    else
    {
        // this was either a straight move in a multiple situation ( we want crossfade)
        // or a column switch in a single/multiple situation (we want portal/crossfade)
        ctl::ComPtr<Storyboard> spSecondaryStoryboard;
        ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*> > spSecondaryChildren;

        std::unique_ptr<ThemeGeneratorHelper> spPrimaryClipHelper;
        std::unique_ptr<ThemeGeneratorHelper> spSecondaryClipHelper;
        std::unique_ptr<ThemeGeneratorHelper> spHelper;

        ctl::ComPtr<xaml::IUIElement> spPanelAsUIE;
        ctl::ComPtr<xaml_controls::IPanel> spPanelAsPanel;
        ctl::ComPtr<xaml::IDependencyObject> spElementAsDO;
        ctl::ComPtr<xaml::IDependencyObject> spParentAsDO;
        ctl::ComPtr<IRectangleGeometry> spClipGeometry;

        // need two animations, one for the source column, one for the destination column (portalling) or for the crossfade
        IFC_RETURN(ctl::make(&spSecondaryStoryboard));
        IFC_RETURN(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spSecondaryStoryboard->GetHandle()), static_cast<UIElement*>(element)->GetHandle()));
        IFC_RETURN(spSecondaryStoryboard->get_Children(&spSecondaryChildren));
        IFC_RETURN(storyboards->Append(spSecondaryStoryboard.Get()));

        BOOLEAN handledTransition = FALSE;
        if (!multiple && allowPortalling)
        {
            // we will possibly do portal animation

            // we shall use the clip on the parent panel as an indication how far to move.
            // if no clip is available we can use the size of the parent itself
            wf::Rect bounds = { 0, 0, 0, 0 };
            spElementAsDO = ctl::query_interface_cast<xaml::IDependencyObject>(element);
            IFC_RETURN(VisualTreeHelper::GetParentStatic(spElementAsDO.Get(), &spParentAsDO));
            spPanelAsUIE = ctl::query_interface_cast<xaml::IUIElement>(spParentAsDO.Get());
            IFC_RETURN(spPanelAsUIE->get_Clip(spClipGeometry.ReleaseAndGetAddressOf()));
            spPanelAsPanel = ctl::query_interface_cast<xaml_controls::IPanel>(spParentAsDO.Get());
            if (spPanelAsPanel)
            {
                allowPortalling = static_cast<Panel*>(spPanelAsPanel.Get())->IsPortallingSupported();
            }
            else
            {
                allowPortalling = FALSE;    // parent is not a panel, lets not do portalling
            }

            if (allowPortalling)
            {
                if (spClipGeometry)
                {
                    IFC_RETURN(spClipGeometry->get_Rect(&bounds));
                }
                else
                {
                    ctl::ComPtr<IChildTransitionContextProvider> spContextProviderPanel = ctl::query_interface_cast<IChildTransitionContextProvider>(spPanelAsPanel.Get());
                    if (spContextProviderPanel)
                    {
                        IFC_RETURN(spContextProviderPanel->GetChildTransitionBounds(element, &bounds));
                    }
                }

                if (bounds.Width > 0 || bounds.Height > 0)
                {
                    // on the old panels bounds.Height will never be 0;
                    BOOLEAN verticalMove = bounds.Height != 0;

                    // lets determine if we are portalling down or up (left or right).
                    BOOLEAN primaryMovesOut = verticalMove ? start.X < destination.X : start.Y < destination.Y;

                    wf::Point primaryStart = sourceoffset;
                    wf::Point primaryDestination = sourceoffset;
                    wf::Point secondaryStart = destoffset;
                    wf::Point secondaryDestination = destoffset;

                    // the total amount that we are going to move is the larger of two distances:
                    // a. the distance of the top of the element to the bottom of the bounds
                    //    --> the distance it needs to end completely clipped at the bottom
                    // b. the distance of the top of the bounds to the bottom of the element
                    //    --> the distance it needs to start completely clipped at the top

                    // (reverse when moving up)

                    DOUBLE a = 0;
                    DOUBLE b = 0;

                    if (primaryMovesOut)
                    {
                        a = verticalMove ? (bounds.Y + bounds.Height) - start.Y : (bounds.X + bounds.Width) - start.X;
                        b = verticalMove ? (destination.Y + destination.Height) - bounds.Y : (destination.X + destination.Width) - bounds.X;
                    }
                    else
                    {
                        a = verticalMove ? (start.Y + start.Height) - bounds.Y : (start.X + start.Width) - bounds.X;
                        b = verticalMove ? (bounds.Y + bounds.Height) - destination.Y : (bounds.X + bounds.Width) - destination.X;
                    }

                    DOUBLE dist = DoubleUtil::Max(a, b);
                    if (primaryMovesOut)
                    {
                        if (verticalMove)
                        {
                            primaryDestination.Y += static_cast<FLOAT>(dist);
                            secondaryStart.Y -= static_cast<FLOAT>(dist);
                        }
                        else
                        {
                            primaryDestination.X += static_cast<FLOAT>(dist);
                            secondaryStart.X -= static_cast<FLOAT>(dist);
                        }
                    }
                    else
                    {
                        if (verticalMove)
                        {
                            primaryDestination.Y -= static_cast<FLOAT>(dist);
                            secondaryStart.Y += static_cast<FLOAT>(dist);
                        }
                        else
                        {
                            primaryDestination.X -= static_cast<FLOAT>(dist);
                            secondaryStart.X += static_cast<FLOAT>(dist);
                        }
                    }

                    // old panels will have clip around elements. No special clip needed.
                    // for the new panels we don't have per panel clip thus we should create clip
                    // which moves with the same animation but in opposite direction as LTE. It will create effect of boundary.
                    if (!spClipGeometry)
                    {
                        wf::Point primaryClipOrigin = { 0, 0 };
                        wf::Point primaryClipDestination = { primaryStart.X - primaryDestination.X, primaryStart.Y - primaryDestination.Y };
                        wf::Point secondaryClipOrigin = { -secondaryStart.X, -secondaryStart.Y };
                        wf::Point secondaryClipDestination = { 0, 0 };

                        spPrimaryClipHelper.reset(new ThemeGeneratorHelper(primaryClipOrigin, primaryClipDestination, NULL, NULL, FALSE, spPrimaryChildren.Get()));
                        IFC_RETURN(spPrimaryClipHelper->Initialize());
                        spPrimaryClipHelper->SetAdditionalTime(scheduleAffectedFirst || reorderingContext ? 0 : loadDuration);
                        IFC_RETURN(spPrimaryClipHelper->SetOverrideTranslateXPropertyName(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateX")));
                        IFC_RETURN(spPrimaryClipHelper->SetOverrideTranslateYPropertyName(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateY")));

                        // Clip for primary LTE
                        IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                                    TAS_ADDTOGRID,
                                    // clip moves to the opposite direction of LTE
                                    primaryMovesOut ? TA_ADDTOGRID_ROWIN : TA_ADDTOGRID_ROWOUT,
                                    spPrimaryClipHelper.get()));

                        spSecondaryClipHelper.reset(new ThemeGeneratorHelper(secondaryClipOrigin, secondaryClipDestination, NULL, NULL, FALSE, spSecondaryChildren.Get()));
                        IFC_RETURN(spSecondaryClipHelper->Initialize());
                        spSecondaryClipHelper->SetAdditionalTime(scheduleAffectedFirst || reorderingContext ? 0 : loadDuration);
                        IFC_RETURN(spSecondaryClipHelper->SetOverrideTranslateXPropertyName(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateX")));
                        IFC_RETURN(spSecondaryClipHelper->SetOverrideTranslateYPropertyName(STR_LEN_PAIR(L"(UIElement.TransitionTarget).(TransitionTarget.ClipTransform).TranslateY")));

                        // Clip for secondary LTE
                        IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                                    TAS_ADDTOGRID,
                                    // clip moves to the opposite direction of LTE
                                    primaryMovesOut ? TA_ADDTOGRID_ROWIN : TA_ADDTOGRID_ROWOUT,
                                    spSecondaryClipHelper.get()));
                    }

                    // LTE for the primary
                    IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_ADDTOGRID,
                                primaryMovesOut ? TA_ADDTOGRID_ROWOUT : TA_ADDTOGRID_ROWIN,
                                FALSE,
                                primaryStart,
                                primaryDestination,
                                scheduleAffectedFirst || reorderingContext ? 0 : loadDuration,
                                spPrimaryChildren.Get()));

                    // LTE for secondary
                    IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                                TAS_ADDTOGRID,
                                primaryMovesOut ? TA_ADDTOGRID_ROWOUT : TA_ADDTOGRID_ROWIN,
                                FALSE,
                                secondaryStart,
                                secondaryDestination,
                                scheduleAffectedFirst || reorderingContext ? 0 : loadDuration,
                                spSecondaryChildren.Get()));

                    handledTransition = TRUE;
                }
            }
        }

        if (!handledTransition) // if we did not want portalling, or couldn't (no clip to portal against)
        {
            // we will do crossfade

            // the transition will keep the element in the old location, so we need to animate this guy to the new location immediately.
            spHelper.reset(new ThemeGeneratorHelper(destoffset, destoffset, NULL, NULL, FALSE, spPrimaryChildren.Get()));
            IFC_RETURN(spHelper->Initialize());
            IFC_RETURN(spHelper->RegisterKeyFrame(spHelper->GetTranslateXPropertyName(), 0, 0, 0, &linear));
            IFC_RETURN(spHelper->RegisterKeyFrame(spHelper->GetTranslateYPropertyName(), 0, 0, 0, &linear));
            spHelper->SetOverrideInitialOpacity(0); // also, pvl no longer starts with a 0 opacity
            spHelper->SetAdditionalTime(scheduleAffectedFirst || reorderingContext ? 0 : loadDuration);

            IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                        TAS_CROSSFADE,
                        TA_CROSSFADE_INCOMING,
                        spHelper.get()));

            // LTE for secondary
            IFC_RETURN(ThemeGenerator::AddTimelinesForThemeAnimation(
                        TAS_CROSSFADE,
                        TA_CROSSFADE_OUTGOING,
                        FALSE,
                        sourceoffset,
                        sourceoffset,
                        scheduleAffectedFirst || reorderingContext ? 0 : loadDuration,
                        spSecondaryChildren.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT AddDeleteRepositionStoryboardCreator(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    *parentForTransition =
        transitionTrigger == xaml::TransitionTrigger_Reparent ?
        xaml::TransitionParent_ParentToRoot :
        xaml::TransitionParent_ParentToCommonParent;

    switch (transitionTrigger)
    {
        case xaml::TransitionTrigger_Load:
            IFC_RETURN(AddDeleteRepositionHelperLoad(element, start, destination, storyboards));
            break;

        case xaml::TransitionTrigger_Unload:
            IFC_RETURN(AddDeleteRepositionHelperUnload(element, start, destination, storyboards));
            break;

        case xaml::TransitionTrigger_Reparent:
        case xaml::TransitionTrigger_Layout:
            IFC_RETURN(AddDeleteRepositionHelperReparentAndLayout(element, start, destination, storyboards));
            break;

        default:
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT ReorderThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldRun = transitionTrigger == xaml::TransitionTrigger_Layout || transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Reparent;
    BOOLEAN didCheck = FALSE;
    ThemeTransitionContext context = ThemeTransitionContext::None;

    *returnValue = FALSE;

    if (shouldRun)
    {
        IFC(GetTransitionContext(element, &didCheck, &context));
    }
    if (didCheck)
    {
        shouldRun =
            (context == ThemeTransitionContext::ReorderedItem ||
            context == ThemeTransitionContext::SingleReorderList ||
            context == ThemeTransitionContext::SingleReorderGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleReorderGrid);
    }

Cleanup:
    *returnValue = shouldRun;
    RRETURN(hr);
}

_Check_return_ HRESULT ReorderThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    BOOLEAN shouldRun = transitionTrigger == xaml::TransitionTrigger_Layout || transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Reparent;
    BOOLEAN didCheck = FALSE;
    ThemeTransitionContext context = ThemeTransitionContext::None;

    if (shouldRun)
    {
        IFC(GetTransitionContext(element, &didCheck, &context));
    }
    if (didCheck)
    {
        shouldRun &=
            (context == ThemeTransitionContext::ReorderedItem ||
            context == ThemeTransitionContext::SingleReorderList ||
            context == ThemeTransitionContext::SingleReorderGrid ||
            context == ThemeTransitionContext::MultipleReorderList ||
            context == ThemeTransitionContext::MultipleReorderGrid);
    }

    if (shouldRun)
    {
        IFC(AddDeleteRepositionStoryboardCreator(element, start, destination, transitionTrigger, storyboards, parentForTransition));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AddDeleteThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN fastMutations = FALSE;

    // we do not know how to properly express animations with lots of interruptions. We do not have any independent handoff
    // for transitions and they jump back to hold their location (as known to the uithread). This looks very bad, so
    // we fall back to no animations.
    // Note how i'm doing this both for grids and non grid layouts, since even in list scenarios, this looks very jarring.
    IFC(GetSpeedOfChanges(element, &fastMutations));
    *returnValue = !fastMutations;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT AddDeleteThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;
    BOOLEAN didCheck = FALSE;
    ThemeTransitionContext context = ThemeTransitionContext::None;

    BOOLEAN shouldRun = TRUE;
    BOOLEAN fastMutations = FALSE;

    // we do not know how to properly express animations with lots of interruptions. We do not have any independent handoff
    // for transitions and they jump back to hold their location (as known to the uithread). This looks very bad, so
    // we fall back to no animations.
    // Note how i'm doing this both for grids and non grid layouts, since even in list scenarios, this looks very jarring.
    IFC(GetSpeedOfChanges(element, &fastMutations));
    shouldRun = !fastMutations;

    if (shouldRun)
    {
        IFC(GetTransitionContext(element, &didCheck, &context));
    }
    if (didCheck)
    {
        shouldRun &=
            (context == ThemeTransitionContext::SingleAddList ||
             context == ThemeTransitionContext::SingleDeleteList ||
             context == ThemeTransitionContext::SingleAddGrid ||
             context == ThemeTransitionContext::SingleDeleteGrid ||
             context == ThemeTransitionContext::MixedOperationsList ||
             context == ThemeTransitionContext::MixedOperationsGrid ||
             context == ThemeTransitionContext::MultipleAddList ||
             context == ThemeTransitionContext::MultipleDeleteList ||
             context == ThemeTransitionContext::MultipleAddGrid ||
             context == ThemeTransitionContext::MultipleDeleteGrid);
    }

    if (shouldRun)
    {
        IFC(AddDeleteRepositionStoryboardCreator(element, start, destination, transitionTrigger, storyboards, parentForTransition));
    }

    SetTransitionParentForStickyHeaders(element, parentForTransition);

Cleanup:
    RRETURN(hr);
}

// ContentDialogOpenCloseThemeTransition
_Check_return_ HRESULT ContentDialogOpenCloseThemeTransition::ParticipatesInTransitionImpl(
    _In_ xaml::IUIElement* element,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _Out_ BOOLEAN* returnValue)
{
    // This transition will only play when the target element is a ContentDialog.
    *returnValue = ctl::is<xaml_controls::IContentDialog>(element) &&
        (transitionTrigger == xaml::TransitionTrigger_Load || transitionTrigger == xaml::TransitionTrigger_Unload);

    return S_OK;
}

_Check_return_ HRESULT ContentDialogOpenCloseThemeTransition::CreateStoryboardImpl(
    _In_ xaml::IUIElement* element,
    _In_ wf::Rect start,
    _In_ wf::Rect destination,
    _In_ xaml::TransitionTrigger transitionTrigger,
    _In_ wfc::IVector<xaml_animation::Storyboard*>* storyboards,
    _Out_ xaml::TransitionParent* parentForTransition)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<IUIElement> spElement(element);
    ctl::ComPtr<IUIElement> smokeLayer;

    // This transition will only play when the target element is a ContentDialog.
    ASSERT(ctl::is<xaml_controls::IContentDialog>(element));

    GetSmokeLayer(&smokeLayer);

    ctl::ComPtr<Storyboard> spStoryboard;
    ctl::ComPtr<wfc::IVector<xaml_animation::Timeline*>> spTimelines;
    TimingFunctionDescription linear = TimingFunctionDescription();
    TimingFunctionDescription easing = TimingFunctionDescription();
    easing.cp3.X = 0.0f; // Cubic-bezier (0, 0, 0, 1). Default TimingFunctionDescription() constructor creates a Linear curve (0,0,0,0,1,1,1,1).
    const wf::Point nullPoint = { 0, 0 };
    const DOUBLE scaledFactor = 1.05;
    const DOUBLE defaultScaleFactor = 1.0;
    const wf::Point scaleOrigin = { 0.5f, 0.5f };

    // Create the storyboard for ContentDialog Open or Close theme transition
    IFC(ctl::make(&spStoryboard));
    IFC(CoreImports::Storyboard_SetTarget(static_cast<CTimeline*>(spStoryboard.Get()->GetHandle()), spElement.Cast<UIElement>()->GetHandle()));

    IFC(spStoryboard->get_Children(&spTimelines));

    if (transitionTrigger == xaml::TransitionTrigger_Load)
    {
        ThemeGeneratorHelper themeSupplier(nullPoint, nullPoint, nullptr, nullptr, FALSE, spTimelines.Get());

        IFC(themeSupplier.Initialize());
        IFC(themeSupplier.Set2DTransformOriginValues(scaleOrigin));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleXPropertyName(), scaledFactor, 0, 0, &linear));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleYPropertyName(), scaledFactor, 0, 0, &linear));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleXPropertyName(), defaultScaleFactor, 0, s_OpenScaleDuration, &easing));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleYPropertyName(), defaultScaleFactor, 0, s_OpenScaleDuration, &easing));

        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0, 0, 0, &linear));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, s_OpacityChangeDuration, &linear));

        if (smokeLayer)
        {
            ThemeGeneratorHelper themeSupplierOverlay(nullPoint, nullPoint, nullptr, smokeLayer.Cast<UIElement>(), FALSE, spTimelines.Get());

            IFC(themeSupplierOverlay.Initialize());
            IFC(themeSupplierOverlay.RegisterKeyFrame(themeSupplierOverlay.GetOpacityPropertyName(), 0.0, 0, 0, &linear));
            IFC(themeSupplierOverlay.RegisterKeyFrame(themeSupplierOverlay.GetOpacityPropertyName(), 1.0, 0, s_OpacityChangeDuration, &linear));
        }
    }
    else if (transitionTrigger == xaml::TransitionTrigger_Unload)
    {
        ThemeGeneratorHelper themeSupplier(nullPoint, nullPoint, nullptr, nullptr, FALSE, spTimelines.Get());

        IFC(themeSupplier.Initialize());
        IFC(themeSupplier.Set2DTransformOriginValues(scaleOrigin));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleXPropertyName(), defaultScaleFactor, 0, 0, &linear));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleYPropertyName(), defaultScaleFactor, 0, 0, &linear));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleXPropertyName(), scaledFactor, 0, s_CloseScaleDuration, &easing));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetScaleYPropertyName(), scaledFactor, 0, s_CloseScaleDuration, &easing));

        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 1.0, 0, 0, &linear));
        IFC(themeSupplier.RegisterKeyFrame(themeSupplier.GetOpacityPropertyName(), 0.0, 0, s_OpacityChangeDuration, &linear));

        if (smokeLayer)
        {
            ThemeGeneratorHelper themeSupplierOverlay(nullPoint, nullPoint, nullptr, smokeLayer.Cast<UIElement>(), FALSE, spTimelines.Get());

            IFC(themeSupplierOverlay.Initialize());
            IFC(themeSupplierOverlay.RegisterKeyFrame(themeSupplierOverlay.GetOpacityPropertyName(), 1.0, 0, 0, &linear));
            IFC(themeSupplierOverlay.RegisterKeyFrame(themeSupplierOverlay.GetOpacityPropertyName(), 0.0, 0, s_OpacityChangeDuration, &linear));
        }
    }

    IFC(storyboards->Append(spStoryboard.Get()));

Cleanup:
    RRETURN(hr);
}
