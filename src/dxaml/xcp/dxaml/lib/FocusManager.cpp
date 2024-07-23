// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FocusManager.g.h"
#include "focusmgr.h"
#include "xyfocus.h"
#include "FindNextElementOptions.g.h"
#include "FocusMovement.h"
#include "FocusAsyncOperation.h"
#include "Control.g.h"
#include <FocusProperties.h>
#include <ContentRoot.h>
#include <XamlRoot_Partial.h>

using namespace DirectUI;

template <typename T> ULONG FocusAsyncActionBase<T>::m_uniqueAsyncActionId = 1;

typedef FocusAsyncActionBase<wrl::AsyncCausalityOptions<FocusManagerTryMoveFocusAsyncOperationName>> TryMoveFocusAsyncAction;
typedef FocusAsyncActionBase<wrl::AsyncCausalityOptions<FocusManagerTryFocusAsyncOperationName>> TryFocusAsyncAction;

static XRECTF_RB RectToXRectFRB(
    _In_ wf::Rect& rect)
{
    XRECTF_RB coreRect;

    coreRect.left = rect.X;
    coreRect.top = rect.Y;
    coreRect.right = rect.X + rect.Width;
    coreRect.bottom = rect.Y + rect.Height;

    return coreRect;
}

static wf::Rect XRectFRBToRect(
    _In_ XRECTF_RB& rect)
{
    wf::Rect out;

    out.X = rect.left;
    out.Y = rect.top;
    out.Width = rect.right - rect.left;
    out.Height = rect.bottom - rect.top;

    return out;
}

static void ConvertOptionsRectsToPhysical(
     _In_ float scale,
     _Inout_ Focus::XYFocusOptions& xyFocusOptions)
 {
     if (xyFocusOptions.exclusionRect)
     {
         wf::Rect exclusionRect = XRectFRBToRect(*(xyFocusOptions.exclusionRect));
         DXamlCore::GetCurrent()->DipsToPhysicalPixels(scale, &exclusionRect, &exclusionRect);
         *xyFocusOptions.exclusionRect = RectToXRectFRB(exclusionRect);
     }

     if (xyFocusOptions.focusHintRectangle)
     {
         wf::Rect hintRect = XRectFRBToRect(*(xyFocusOptions.focusHintRectangle));
         DXamlCore::GetCurrent()->DipsToPhysicalPixels(scale, &hintRect, &hintRect);
         *xyFocusOptions.focusHintRectangle = RectToXRectFRB(hintRect);
     }
 }

 static bool InIslandsMode()
 {
     DXamlCore* pCore = DXamlCore::GetCurrent();
     ASSERT(pCore);

     return pCore ? (pCore->GetHandle()->GetInitializationType() == InitializationType::IslandsOnly) : false;
 }

 static _Check_return_ HRESULT FindNextFocus(
     _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
     _In_ Focus::XYFocusOptions& xyFocusOptions,
     _Outptr_ IInspectable** ppCandidate)
 {
    if (focusNavigationDirection == xaml_input::FocusNavigationDirection_None)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR_RETURN(pCore);
    IFCEXPECT_RETURN(pCore->GetHandle());

    *ppCandidate = nullptr;

    CFocusManager* focusManager = nullptr;

    if (CDependencyObject* searchRoot = xyFocusOptions.searchRoot)
    {
        focusManager = VisualTree::GetFocusManagerForElement(searchRoot);
    }
    else if (pCore->GetHandle()->GetInitializationType() == InitializationType::IslandsOnly)
    {
        // Return error if searchRoot is not valid in islands/ desktop mode
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_INVALID_SEARCHROOT_WIN32));
    }
    else
    {
        // For compat reasons, these FocusManager static APIs need to always use the CoreWindow as the
        // ContentRoot, so explicitly return the CoreWindow content root.
        const auto contentRootCoordinator = pCore->GetHandle()->GetContentRootCoordinator();
        CContentRoot* contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();

        if (contentRoot == nullptr)
        {
            return S_OK;
        }

        focusManager = contentRoot->GetFocusManagerNoRef();
    }

    const auto root = focusManager->GetContentRootNoRef();
    const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);
    ConvertOptionsRectsToPhysical(scale, xyFocusOptions);

    CDependencyObject* candidate = focusManager->FindNextFocus((FocusNavigationDirection)focusNavigationDirection, xyFocusOptions);

    if (candidate)
    {
        ctl::ComPtr<DependencyObject> peer;
        IFC_RETURN(pCore->GetPeer(candidate, &peer));
        *ppCandidate = peer.AsOrNull<IInspectable>().Detach();
    }

    return S_OK;
 }

_Check_return_ HRESULT
FocusManagerFactory::GetFocusedElementImpl(
    _Outptr_ IInspectable** ppFocusedObject)
{
    IFCPTR_RETURN(ppFocusedObject);
    *ppFocusedObject = NULL;

    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR_RETURN(pCore);

    // For compat reasons, these FocusManager static APIs need to always use the CoreWindow as the
    // ContentRoot, so explicitly return the CoreWindow content root.
    const auto contentRootCoordinator = pCore->GetHandle()->GetContentRootCoordinator();
    CContentRoot* contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();

    if (contentRoot == nullptr)
    {
        return S_OK;
    }

    CDependencyObject* pDO = contentRoot->GetFocusManagerNoRef()->GetFocusedElementNoRef();

    if (pDO != nullptr)
    {
        ctl::ComPtr<DependencyObject> pFocusedElement;
        IFC_RETURN(pCore->GetPeer(pDO, &pFocusedElement));
        IFC_RETURN(ctl::do_query_interface<IInspectable>(*ppFocusedObject, pFocusedElement.Get()));
    }

    return S_OK;
}

// Gets the XamlRoot.Content object as seen by the app, so we can use it to compare against a parameter.
// This is because some of our APIs (FocusManager TryNextFocus/FindNextElement) specifically allow an app
// to scope a search to the XamlRoot level (by passing in XamlRoot.Content) but not for arbitrary elements.
// This function calls XamlRoot.Content the way the app would.
ctl::ComPtr<DependencyObject> GetAppVisibleXamlRootContent(_In_ CContentRoot* contentRoot)
{
    IInspectable* xamlRootInsp = contentRoot->GetOrCreateXamlRootNoRef();

    ctl::ComPtr<XamlRoot> xamlRoot;
    IFCFAILFAST(xamlRootInsp->QueryInterface(IID_PPV_ARGS(&xamlRoot)));

    ctl::ComPtr<xaml::IUIElement> uiElement;
    if (SUCCEEDED(xamlRoot->get_Content(&uiElement)))
    {
        ctl::ComPtr<DependencyObject> obj;
        if (SUCCEEDED(ctl::do_query_interface(obj, uiElement.Get())))
        {
            return obj;
        }
    }

    return ctl::ComPtr<DependencyObject> {};
}

static _Check_return_ HRESULT TryMoveFocusStatic(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_opt_ xaml_input::IFindNextElementOptions* pFocusNavigationOverride,
    _Out_ BOOLEAN* pIsFocusMoved,
    _Outptr_opt_result_maybenull_ wf::IAsyncOperation<xaml_input::FocusMovementResult*>** asyncOperation)
{
    ctl::ComPtr<DependencyObject> searchRootAsDO;
    wf::Rect hintRect;
    wf::Rect exclusionRect;
    xaml_input::XYFocusNavigationStrategyOverride navigationStrategyOverride;
    Focus::XYFocusOptions xyFocusOptions;
    IFCPTR_RETURN(pIsFocusMoved);
    *pIsFocusMoved = FALSE;

    DirectUI::FindNextElementOptions* options = static_cast<DirectUI::FindNextElementOptions*>(pFocusNavigationOverride);

    BOOLEAN ignoreOcclusivity;
    wrl::ComPtr<TryMoveFocusAsyncAction> spFocusAsyncOperation;

    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR_RETURN(pCore);

    CFocusManager* focusManager = nullptr;
    CContentRoot* contentRoot = nullptr;
    const auto contentRootCoordinator = pCore->GetHandle()->GetContentRootCoordinator();

    if (options != nullptr)
    {
        ctl::ComPtr<xaml::IDependencyObject> searchRoot;
        IFCFAILFAST(options->get_SearchRoot(&searchRoot));
        IFCFAILFAST(options->get_ExclusionRect(&exclusionRect));
        IFCFAILFAST(options->get_HintRect(&hintRect));
        IFCFAILFAST(options->get_XYFocusNavigationStrategyOverride(&navigationStrategyOverride));
        IFCFAILFAST(options->get_IgnoreOcclusivity(&ignoreOcclusivity));
        searchRootAsDO = searchRoot.Cast<DependencyObject>();

        if (searchRootAsDO)
        {
            contentRoot = VisualTree::GetContentRootForElement(searchRootAsDO->GetHandle());
            focusManager = contentRoot->GetFocusManagerNoRef();
            IFCPTR_RETURN(focusManager);
        }
        else if (pCore->GetHandle()->GetInitializationType() == InitializationType::IslandsOnly)
        {
            // SearchRoot must exist for islands/ desktop
            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_TRYMOVEFOCUS_WITHOUT_FINDNEXTELEMENTOPTIONS_WIN32));
        }
        else
        {
            contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        }

        // If we are being passed in the public root visual of a XamlRoot as the SearchRoot, then override the SearchRoot to be the RootVisual.
        // This will enable searching through both the public root visual and the popup root. We will also allow Next/Prev.
        const bool shouldOverrideSearchRoot =
            contentRoot != nullptr
            && ctl::are_equal(GetAppVisibleXamlRootContent(contentRoot).Get(), searchRootAsDO.Get());
        if (shouldOverrideSearchRoot)
        {
            IFC_RETURN(pCore->TryGetPeer(contentRoot->GetVisualTreeNoRef()->GetRootElementNoRef(), &searchRootAsDO));
        }
        else
        {
            if (focusNavigationDirection != xaml_input::FocusNavigationDirection_Up &&
                focusNavigationDirection != xaml_input::FocusNavigationDirection_Down &&
                focusNavigationDirection != xaml_input::FocusNavigationDirection_Left &&
                focusNavigationDirection != xaml_input::FocusNavigationDirection_Right)
            {
                IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_FINDNEXTELEMENT_OPTION_DIRECTION));
            }
        }

        xyFocusOptions.navigationStrategyOverride = static_cast<DirectUI::XYFocusNavigationStrategyOverride>(navigationStrategyOverride);
        xyFocusOptions.ignoreOcclusivity = ignoreOcclusivity;

        XRECTF_RB exclusionRectNative = RectToXRectFRB(exclusionRect);
        XRECTF_RB hintRectNative = RectToXRectFRB(hintRect);

        if (searchRootAsDO)
        {
            xyFocusOptions.searchRoot = searchRootAsDO->GetHandle();
        }

        if (!exclusionRectNative.IsUniform())
        {
            xyFocusOptions.exclusionRect = &exclusionRectNative;
        }

        if (!hintRectNative.IsUniform())
        {
            xyFocusOptions.focusHintRectangle = &hintRectNative;
        }

        if (contentRoot)
        {
            const auto scale = RootScale::GetRasterizationScaleForContentRoot(contentRoot);
            ConvertOptionsRectsToPhysical(scale, xyFocusOptions);
        }
    }

    if (focusManager == nullptr)
    {
        // Return error if call is without focus navigation option in islands/ desktop
        if (pCore->GetHandle()->GetInitializationType() == InitializationType::IslandsOnly)
        {
            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_TRYMOVEFOCUS_WITHOUT_FINDNEXTELEMENTOPTIONS_WIN32));
        }

        // For compat reasons, these FocusManager static APIs need to always use the CoreWindow as the
        // ContentRoot, so explicitly return the CoreWindow content root.
        if (contentRoot == nullptr)
        {
            contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        }
        if (contentRoot == nullptr)
        {
            return S_OK;
        }

        focusManager = contentRoot->GetFocusManagerNoRef();
    }

    Focus::FocusMovement movement = Focus::FocusMovement(xyFocusOptions, (FocusNavigationDirection)focusNavigationDirection, nullptr);

    if (asyncOperation != nullptr)
    {
        spFocusAsyncOperation = TryMoveFocusAsyncAction::CreateFocusAsyncOperation(movement.GetCorrelationId());
        IFCFAILFAST(spFocusAsyncOperation.CopyTo(asyncOperation));
        AddRefInterface(spFocusAsyncOperation.Get());

        // Async operation is not guaranteed to be released synchronously.
        // Therefore, we let UpdateFocus to handle the responsibility of releasing it.
        movement.shouldCompleteAsyncOperation = focusManager->TrySetAsyncOperation(spFocusAsyncOperation.Get());

        if (movement.shouldCompleteAsyncOperation)
        {
            spFocusAsyncOperation->StartOperation();
        }
    }

    const ::Focus::FocusMovementResult result = focusManager->FindAndSetNextFocus(movement);
    // We ignore result.GetHResult() here because this is a "Try" function
    *pIsFocusMoved = result.WasMoved();

    // Async operation is not guaranteed to be released synchronously.
    // Therefore, we let UpdateFocus to handle the responsibility of releasing it.
    spFocusAsyncOperation.Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::TryMoveFocusAsyncImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _Outptr_ wf::IAsyncOperation<xaml_input::FocusMovementResult*>** ppReturnValue)
{
    BOOLEAN focusMoved = FALSE;

    IFC_RETURN(TryMoveFocusStatic(focusNavigationDirection, nullptr, &focusMoved, ppReturnValue));
    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::TryMoveFocusWithOptionsAsyncImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_ xaml_input::IFindNextElementOptions* focusNavigationOptions,
    _Outptr_ wf::IAsyncOperation<xaml_input::FocusMovementResult*>** ppReturnValue)
{
    BOOLEAN focusMoved = FALSE;

    IFC_RETURN(TryMoveFocusStatic(focusNavigationDirection, focusNavigationOptions, &focusMoved, ppReturnValue));
    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::TryMoveFocusImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _Out_ BOOLEAN* pIsFocusMoved)
{
    IFC_RETURN(TryMoveFocusStatic(focusNavigationDirection, nullptr, pIsFocusMoved, nullptr));
    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::TryMoveFocusWithOptionsImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_ xaml_input::IFindNextElementOptions* pFocusNavigationOverride,
    _Out_ BOOLEAN* pIsFocusMoved)
{
    IFC_RETURN(TryMoveFocusStatic(focusNavigationDirection, pFocusNavigationOverride, pIsFocusMoved, nullptr));
    return S_OK;
}

_Check_return_ HRESULT FocusManagerFactory::TryFocusAsyncImpl(
    _In_ xaml::IDependencyObject* pElement,
    _In_ xaml::FocusState focusState,
    _Outptr_ wf::IAsyncOperation<xaml_input::FocusMovementResult*>** asyncOperation)
{
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(static_cast<DirectUI::DependencyObject*>(pElement)->GetHandle());
    IFCEXPECT_RETURN(focusManager);

    Focus::FocusMovement movement = Focus::FocusMovement(static_cast<DirectUI::DependencyObject*>(pElement)->GetHandle(),
        DirectUI::FocusNavigationDirection::None,
        static_cast<DirectUI::FocusState>(focusState));

    wrl::ComPtr<TryFocusAsyncAction> spFocusAsyncOperation = TryFocusAsyncAction::CreateFocusAsyncOperation(movement.GetCorrelationId());
    IFCFAILFAST(spFocusAsyncOperation.CopyTo(asyncOperation));

    if (FocusProperties::IsFocusable(static_cast<DirectUI::DependencyObject*>(pElement)->GetHandle(), false /*ignoreOffScreenPosition*/) == false)
    {
        // We need to start and complete the async operation since this is a no-op
        spFocusAsyncOperation->StartOperation();
        static_cast<ICoreAsyncOperation<Focus::FocusMovementResult>*>(spFocusAsyncOperation.Get())->CoreSetResults(
            ::Focus::FocusMovementResult());
        static_cast<ICoreAsyncOperation<Focus::FocusMovementResult>*>(spFocusAsyncOperation.Get())->CoreFireCompletion();
        return S_OK;
    }

    movement.shouldCompleteAsyncOperation = focusManager->TrySetAsyncOperation(spFocusAsyncOperation.Get());
    if (movement.shouldCompleteAsyncOperation)
    {
        spFocusAsyncOperation->StartOperation();
    }

    // We ignore the result here because CFocusManager::UpdateFocus will take care of completing the async operation
    // and propagating the result.
    const auto ignored = focusManager->SetFocusedElement(movement);    

    // Async operation is not guaranteed to be released synchronously.
    // Therefore, we let UpdateFocus to handle the responsibility of releasing it.
    spFocusAsyncOperation.Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindNextFocusableElementImpl(_In_ xaml_input::FocusNavigationDirection focusNavigationDirection, _Outptr_ xaml::IUIElement** ppReturnValue)
{
    if (InIslandsMode())
    {
        // This api is not supported in islands/ desktop mode.
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_API_NOT_SUPPORTED_WIN32));
    }

    ctl::ComPtr<IInspectable> candidate;

    Focus::XYFocusOptions xyFocusOptions;
    xyFocusOptions.updateManifold = false;

    IFC_RETURN(FindNextFocus(focusNavigationDirection, xyFocusOptions, &candidate));
    *ppReturnValue = candidate.AsOrNull<xaml::IUIElement>().Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindNextFocusableElementWithHintImpl(_In_ xaml_input::FocusNavigationDirection focusNavigationDirection, _In_ wf::Rect focusHintRectangle, _Outptr_ xaml::IUIElement** ppReturnValue)
{
    if (InIslandsMode())
    {
        // This api is not supported in islands/ desktop mode.
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_API_NOT_SUPPORTED_WIN32));
    }

    ctl::ComPtr<IInspectable> candidate;
    XRECTF_RB hintRect = RectToXRectFRB(focusHintRectangle);

    Focus::XYFocusOptions xyFocusOptions;
    xyFocusOptions.updateManifold = false;
    xyFocusOptions.focusHintRectangle = &hintRect;

    IFC_RETURN(FindNextFocus(focusNavigationDirection, xyFocusOptions, &candidate));
    *ppReturnValue = candidate.AsOrNull<xaml::IUIElement>().Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindNextFocusWithSearchRootIgnoreEngagementImpl(_In_ xaml_input::FocusNavigationDirection focusNavigationDirection, _In_ IInspectable* pSearchRoot, _Outptr_ IInspectable** ppReturnValue)
{
    ctl::ComPtr<IInspectable> spScope(pSearchRoot);
    ctl::ComPtr<xaml::IDependencyObject> spScopeDO;
    IFC_RETURN(spScope.As(&spScopeDO));

    ctl::ComPtr<IInspectable> candidate;

    auto cDOSearchRoot = spScopeDO.Cast<DependencyObject>()->GetHandle();
    if (!cDOSearchRoot && InIslandsMode())
    {
        // Return error if cDOSearchRoot is not valid in islands/ desktop mode
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_INVALID_SEARCHROOT_WIN32));
    }

    Focus::XYFocusOptions xyFocusOptions;
    xyFocusOptions.searchRoot = cDOSearchRoot;
    xyFocusOptions.considerEngagement = false;

    IFC_RETURN(FindNextFocus(focusNavigationDirection, xyFocusOptions, &candidate));
    *ppReturnValue = candidate.AsOrNull<IInspectable>().Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::SetEngagedControlImpl(_In_ IInspectable* pEngagedControl)
{
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR_RETURN(pCore);
    IFCEXPECT_RETURN(pCore->GetHandle());

    ctl::ComPtr<DependencyObject> controlAsDO;
    IFC_RETURN(ctl::do_query_interface(controlAsDO, pEngagedControl));

    CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(controlAsDO->GetHandle());

    if (pFocusManager->GetEngagedControlNoRef() != nullptr)
    {
        // We should never engage a control when there is already
        // an engaged control.
        IFCFAILFAST(E_FAIL);
    }

    if (pEngagedControl)
    {
        CControl* ccontrol = static_cast<CControl*>(controlAsDO->GetHandle());
        IFC_RETURN(ccontrol->SetValueByKnownIndex(KnownPropertyIndex::Control_IsFocusEngaged, true));
    }

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::SetFocusedElementWithDirectionImpl(
    _In_ xaml::IDependencyObject* pElement,
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN animateIfBringIntoView,
    _In_ BOOLEAN forceBringIntoView,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_ BOOLEAN requestInputActivation,
    _Out_ BOOLEAN* pFocusUpdated)
{
    ctl::ComPtr<xaml::IDependencyObject> spElementToFocus(pElement);
    ctl::ComPtr<xaml_controls::IControl> spControlToFocus;
    *pFocusUpdated = FALSE;
    IFCPTR_RETURN(pElement);

    spControlToFocus = spElementToFocus.AsOrNull<xaml_controls::IControl>();
    if (spControlToFocus)
    {
        // For control, use IControl.Focus, for safer backward compat
        if (animateIfBringIntoView)
        {
            // Set the flag that indicates that the Focus change operation
            // needs to use an animation if the element is brouhgt into view.
            spControlToFocus.Cast<Control>()->SetAnimateIfBringIntoView();
        }

        InputActivationBehavior inputActivationBehavior = requestInputActivation ? InputActivationBehavior::RequestActivation : InputActivationBehavior::NoActivate;
        IFC_RETURN(spControlToFocus.Cast<Control>()->FocusWithDirection(focusState, static_cast<DirectUI::FocusNavigationDirection>(focusNavigationDirection), inputActivationBehavior, pFocusUpdated));
    }
    else
    {
        const bool isShiftPressed = (focusNavigationDirection == xaml_input::FocusNavigationDirection_Previous);
        const bool isProcessingTab = (focusNavigationDirection == xaml_input::FocusNavigationDirection_Next) || isShiftPressed;

        // Set focus on non-controls, like Hyperlink.
        ctl::ComPtr<DependencyObject> spElementToFocusDO;
        IFC_RETURN(spElementToFocus.As(&spElementToFocusDO));
        CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(spElementToFocusDO->GetHandle());
        Focus::FocusMovement movement(
            spElementToFocusDO->GetHandle(),
            static_cast<DirectUI::FocusNavigationDirection>(focusNavigationDirection),
            static_cast<FocusState>(focusState));
        movement.forceBringIntoView = forceBringIntoView;
        movement.animateIfBringIntoView = animateIfBringIntoView;
        movement.isProcessingTab = isProcessingTab;
        movement.isShiftPressed = isShiftPressed;
        movement.requestInputActivation = !!requestInputActivation;
        const ::Focus::FocusMovementResult result = focusManager->SetFocusedElement(movement);
        IFC_RETURN(result.GetHResult());
        *pFocusUpdated = result.WasMoved();
    }

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindFirstFocusableElementImpl(_In_opt_ xaml::IDependencyObject* pSearchScope, _Outptr_ xaml::IDependencyObject** ppReturnValue)
{
    ctl::ComPtr<xaml::IDependencyObject> searchStartDO(pSearchScope);

    *ppReturnValue = nullptr;

    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR_RETURN(pCore);
    IFCEXPECT_RETURN(pCore->GetHandle());

    CDependencyObject* element = nullptr;

    if (searchStartDO)
    {
        CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(searchStartDO.Cast<DependencyObject>()->GetHandle());
        element = pFocusManager->GetFirstFocusableElement(searchStartDO.Cast<DependencyObject>()->GetHandle());
    }
    else
    {
        // For compat reasons, these FocusManager static APIs need to always use the CoreWindow as the
        // ContentRoot, so explicitly return the CoreWindow content root.
        const auto contentRootCoordinator = pCore->GetHandle()->GetContentRootCoordinator();
        CContentRoot* contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();

        if (contentRoot == nullptr)
        {
            return S_OK;
        }

        element = contentRoot->GetFocusManagerNoRef()->GetFirstFocusableElementFromRoot(false /* bReverse */);
    }

    if (element != nullptr)
    {
        ctl::ComPtr<DependencyObject> spPeer;
        IFC_RETURN(pCore->GetPeer(element, &spPeer));
        *ppReturnValue = static_cast<xaml::IDependencyObject*>(spPeer.Detach());
    }

    return S_OK;
}

_Check_return_ HRESULT
DirectUI::FocusManagerFactory::FindLastFocusableElementImpl(_In_opt_ xaml::IDependencyObject* pSearchScope, _Outptr_ xaml::IDependencyObject** ppReturnValue)
{
    ctl::ComPtr<xaml::IDependencyObject> searchStartDO(pSearchScope);

    *ppReturnValue = nullptr;

    IFCEXPECT_RETURN(DXamlServices::IsDXamlCoreInitialized());
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFCPTR_RETURN(pCore);
    IFCEXPECT_RETURN(pCore->GetHandle());

    CDependencyObject* element = nullptr;

    if (searchStartDO)
    {
        CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(searchStartDO.Cast<DependencyObject>()->GetHandle());
        element = pFocusManager->GetLastFocusableElement(searchStartDO.Cast<DependencyObject>()->GetHandle());
    }
    else
    {
        // For compat reasons, these FocusManager static APIs need to always use the CoreWindow as the
        // ContentRoot, so explicitly return the CoreWindow content root.
        const auto contentRootCoordinator = pCore->GetHandle()->GetContentRootCoordinator();
        CContentRoot* contentRoot = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();

        if (contentRoot == nullptr)
        {
            return S_OK;
        }

        element = contentRoot->GetFocusManagerNoRef()->GetFirstFocusableElementFromRoot(true /* bReverse */);
    }

    if (element != nullptr)
    {
        ctl::ComPtr<DependencyObject> spPeer;
        IFC_RETURN(pCore->GetPeer(element, &spPeer));
        *ppReturnValue = static_cast<xaml::IDependencyObject*>(spPeer.Detach());
    }

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::add_GotFocus(
    _In_ wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>* pValue,
    _Out_ EventRegistrationToken* pToken)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>> spEventSource;

    ARG_VALIDRETURNPOINTER(pToken);
    ARG_NOTNULL_RETURN(pValue, "value");

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerGotFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->AddHandler(pValue));

    pToken->value = (INT64)pValue;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::remove_GotFocus(
    _In_ EventRegistrationToken token)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>> spEventSource;
    wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>* pValue = (wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>*)token.value;

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    // removal of event handlers can occur during core shutdown
    // through ShutdownAllPeers(), we don't need to explicitly
    // clean up the handlers from the core in those cases.
    if (pdxc && DXamlCore::IsShuttingDownStatic())
    {
        return S_OK;
    }

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerGotFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->RemoveHandler(pValue));

    token.value = 0;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::add_LostFocus(
    _In_ wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>* pValue,
    _Out_ EventRegistrationToken* pToken)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>> spEventSource;

    ARG_VALIDRETURNPOINTER(pToken);
    ARG_NOTNULL_RETURN(pValue, "value");

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerLostFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->AddHandler(pValue));

    pToken->value = (INT64)pValue;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::remove_LostFocus(
    _In_ EventRegistrationToken token)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>> spEventSource;
    wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>* pValue = (wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>*)token.value;

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    // removal of event handlers can occur during core shutdown
    // through ShutdownAllPeers(), we don't need to explicitly
    // clean up the handlers from the core in those cases.
    if (pdxc && DXamlCore::IsShuttingDownStatic())
    {
        return S_OK;
    }

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerLostFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->RemoveHandler(pValue));

    token.value = 0;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::add_GettingFocus(
    _In_ wf::IEventHandler<xaml_input::GettingFocusEventArgs*>* pValue,
    _Out_ EventRegistrationToken* pToken)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>> spEventSource;

    ARG_VALIDRETURNPOINTER(pToken);
    ARG_NOTNULL_RETURN(pValue, "value");

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerGettingFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->AddHandler(pValue));

    pToken->value = (INT64)pValue;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::remove_GettingFocus(
    _In_ EventRegistrationToken token)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>> spEventSource;
    wf::IEventHandler<xaml_input::GettingFocusEventArgs*>* pValue = (wf::IEventHandler<xaml_input::GettingFocusEventArgs*>*)token.value;

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    // removal of event handlers can occur during core shutdown
    // through ShutdownAllPeers(), we don't need to explicitly
    // clean up the handlers from the core in those cases.
    if (pdxc && DXamlCore::IsShuttingDownStatic())
    {
        return S_OK;
    }

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerGettingFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->RemoveHandler(pValue));

    token.value = 0;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::add_LosingFocus(
    _In_ wf::IEventHandler<xaml_input::LosingFocusEventArgs*>* pValue,
    _Out_ EventRegistrationToken* pToken)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>> spEventSource;

    ARG_VALIDRETURNPOINTER(pToken);
    ARG_NOTNULL_RETURN(pValue, "value");

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerLosingFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->AddHandler(pValue));

    pToken->value = (INT64)pValue;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::remove_LosingFocus(
    _In_ EventRegistrationToken token)
{
    ctl::ComPtr<CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>> spEventSource;
    wf::IEventHandler<xaml_input::LosingFocusEventArgs*>* pValue = (wf::IEventHandler<xaml_input::LosingFocusEventArgs*>*)token.value;

    DXamlCore *pdxc = DXamlCore::GetCurrent();

    // removal of event handlers can occur during core shutdown
    // through ShutdownAllPeers(), we don't need to explicitly
    // clean up the handlers from the core in those cases.
    if (pdxc && DXamlCore::IsShuttingDownStatic())
    {
        return S_OK;
    }

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(pdxc->GetFocusManagerLosingFocusEventSource(&spEventSource));
    IFC_RETURN(spEventSource->RemoveHandler(pValue));

    token.value = 0;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::add_FocusedElementRemoved(_In_ xaml_input::IFocusedElementRemovedEventHandler* value, _Out_ EventRegistrationToken* token)
{
    ctl::ComPtr<CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>> eventSource;

    ARG_VALIDRETURNPOINTER(token);
    ARG_NOTNULL_RETURN(value, "value");

    DXamlCore* core = DXamlCore::GetCurrent();

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(core->GetFocusedElementRemovedEventSource(&eventSource));
    IFC_RETURN(eventSource->AddHandler(value));

    token->value = (INT64)value;

    return S_OK;
}

IFACEMETHODIMP DirectUI::FocusManagerFactory::remove_FocusedElementRemoved(_In_ EventRegistrationToken token)
{
    ctl::ComPtr<CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>> eventSource;
    xaml_input::IFocusedElementRemovedEventHandler* value = (xaml_input::IFocusedElementRemovedEventHandler*)token.value;

    DXamlCore* core = DXamlCore::GetCurrent();

    // removal of event handlers can occur during core shutdown
    // through ShutdownAllPeers(), we don't need to explicitly
    // clean up the handlers from the core in those cases.
    if (core && DXamlCore::IsShuttingDownStatic())
    {
        return S_OK;
    }
    IFC_RETURN(CheckActivationAllowed());
    IFC_RETURN(core->GetFocusedElementRemovedEventSource(&eventSource));
    IFC_RETURN(eventSource->RemoveHandler(value));

    token.value = 0;

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindNextFocusWithSearchRootIgnoreEngagementWithHintRectImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_ IInspectable* pSearchRoot,
    _In_ wf::Rect focusHintRectangle,
    _In_ wf::Rect focusExclusionRectangle,
    _Outptr_ IInspectable** ppReturnValue)
{
    ctl::ComPtr<IInspectable> spScope(pSearchRoot);
    ctl::ComPtr<xaml::IDependencyObject> spScopeDO;
    IFC_RETURN(spScope.As(&spScopeDO));

    ctl::ComPtr<IInspectable> candidate;

    XRECTF_RB hintRect = RectToXRectFRB(focusHintRectangle);
    XRECTF_RB exRect = RectToXRectFRB(focusExclusionRectangle);

    auto cDOSearchRoot = spScopeDO.Cast<DependencyObject>()->GetHandle();
    if (!cDOSearchRoot && InIslandsMode())
    {
        // Return error if cDOSearchRoot is not valid in islands/ desktop mode
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_INVALID_SEARCHROOT_WIN32));
    }

    Focus::XYFocusOptions xyFocusOptions;
    xyFocusOptions.searchRoot = cDOSearchRoot;
    xyFocusOptions.considerEngagement = false;
    xyFocusOptions.focusHintRectangle = &hintRect;
    xyFocusOptions.exclusionRect = &exRect;

    IFC_RETURN(FindNextFocus(focusNavigationDirection, xyFocusOptions, &candidate));
    *ppReturnValue = candidate.AsOrNull<IInspectable>().Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindNextFocusWithSearchRootIgnoreEngagementWithClipImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_ IInspectable* pSearchRoot,
    _In_ BOOLEAN ignoreClipping,
    _In_ BOOLEAN ignoreCone,
    _Outptr_ IInspectable** ppReturnValue)
{
    ctl::ComPtr<IInspectable> spScope(pSearchRoot);
    ctl::ComPtr<xaml::IDependencyObject> spScopeDO;
    IFC_RETURN(spScope.As(&spScopeDO));

    ctl::ComPtr<IInspectable> candidate;

    auto cDOSearchRoot = spScopeDO.Cast<DependencyObject>()->GetHandle();
    if (!cDOSearchRoot && InIslandsMode())
    {
        // Return error if cDOSearchRoot is not valid in islands/ desktop mode
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_INVALID_SEARCHROOT_WIN32));
    }

    Focus::XYFocusOptions xyFocusOptions;
    xyFocusOptions.searchRoot = cDOSearchRoot;
    xyFocusOptions.considerEngagement = false;
    xyFocusOptions.ignoreClipping = ignoreClipping == TRUE;
    xyFocusOptions.ignoreCone = ignoreCone == TRUE;

    IFC_RETURN(FindNextFocus(focusNavigationDirection, xyFocusOptions, &candidate));
    *ppReturnValue = candidate.AsOrNull<IInspectable>().Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindNextElementImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _Outptr_ xaml::IDependencyObject** ppReturnValue)
{
    // Return error if FindNextElement is called without focus navigation option in islands/ desktop
    if (InIslandsMode())
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_FINDNEXTELEMENT_WITHOUT_FINDNEXTELEMENTOPTIONS_WIN32));
    }

    ctl::ComPtr<IInspectable> candidate;

    Focus::XYFocusOptions xyFocusOptions;
    xyFocusOptions.updateManifold = false;

    IFC_RETURN(FindNextFocus(focusNavigationDirection, xyFocusOptions, &candidate));
    *ppReturnValue = candidate.AsOrNull<xaml::IDependencyObject>().Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::FindNextElementWithOptionsImpl(
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_ xaml_input::IFindNextElementOptions* pFocusNavigationOverride,
    _Outptr_ xaml::IDependencyObject** ppReturnValue)
{
    ctl::ComPtr<IInspectable> candidate;
    ctl::ComPtr<xaml::IDependencyObject> searchRoot;
    ctl::ComPtr<DependencyObject> searchRootAsDO;
    wf::Rect hintRect;
    wf::Rect exclusionRect;
    xaml_input::XYFocusNavigationStrategyOverride navigationStrategyOverride;

    DirectUI::FindNextElementOptions* options = static_cast<DirectUI::FindNextElementOptions*>(pFocusNavigationOverride);
    BOOLEAN ignoreOcclusivity;

    Focus::XYFocusOptions xyFocusOptions;
    xyFocusOptions.updateManifold = false;

    IFCFAILFAST(options->get_SearchRoot(&searchRoot));
    IFCFAILFAST(options->get_ExclusionRect(&exclusionRect));
    IFCFAILFAST(options->get_HintRect(&hintRect));
    IFCFAILFAST(options->get_XYFocusNavigationStrategyOverride(&navigationStrategyOverride));
    IFCFAILFAST(options->get_IgnoreOcclusivity(&ignoreOcclusivity));

    searchRootAsDO = searchRoot.Cast<DependencyObject>();

    CContentRoot* contentRoot = nullptr;

    if (searchRootAsDO)
    {
        contentRoot = VisualTree::GetContentRootForElement(searchRootAsDO->GetHandle());
    }
    else if (InIslandsMode())
    {
        // Return error if searchRootAsDO is not valid in islands/ desktop mode
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_INVALID_SEARCHROOT_WIN32));
    }
    else
    {
        contentRoot = DXamlCore::GetCurrent()->GetHandle()->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
    }

    // If we are being passed in the public root visual of the XamlRoot as the SearchRoot, then override the SearchRoot to be the RootVisual.
    // This will enable searching through both the public root visual and the popup root. We will also allow Next/Prev.
    const bool shouldOverrideSearchRoot =
        contentRoot != nullptr
        && ctl::are_equal(GetAppVisibleXamlRootContent(contentRoot).Get(), searchRootAsDO.Get());
    if (shouldOverrideSearchRoot)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(contentRoot->GetVisualTreeNoRef()->GetRootElementNoRef(), &searchRootAsDO));
    }
    else
    {
        if (focusNavigationDirection != xaml_input::FocusNavigationDirection_Up &&
            focusNavigationDirection != xaml_input::FocusNavigationDirection_Down &&
            focusNavigationDirection != xaml_input::FocusNavigationDirection_Left &&
            focusNavigationDirection != xaml_input::FocusNavigationDirection_Right)
        {
            IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_INVALID_FINDNEXTELEMENT_OPTION_DIRECTION));
        }
    }

    XRECTF_RB exclusionRectNative = RectToXRectFRB(exclusionRect);
    XRECTF_RB hintRectNative = RectToXRectFRB(hintRect);

    if (searchRootAsDO)
    {
        xyFocusOptions.searchRoot = searchRootAsDO->GetHandle();
    }

    if (!exclusionRectNative.IsUniform())
    {
        xyFocusOptions.exclusionRect = &exclusionRectNative;
    }

    if (!hintRectNative.IsUniform())
    {
        xyFocusOptions.focusHintRectangle = &hintRectNative;
    }

    xyFocusOptions.navigationStrategyOverride = static_cast<DirectUI::XYFocusNavigationStrategyOverride>(navigationStrategyOverride);
    xyFocusOptions.ignoreOcclusivity = ignoreOcclusivity;

    IFC_RETURN(FindNextFocus(focusNavigationDirection, xyFocusOptions, &candidate));
    *ppReturnValue = candidate.AsOrNull<xaml::IDependencyObject>().Detach();

    return S_OK;
}

_Check_return_ HRESULT
FocusManagerFactory::GetFocusedElementWithRootImpl(_In_ xaml::IXamlRoot* pXamlRoot, _Outptr_result_maybenull_ IInspectable** ppReturnValue)
{
    *ppReturnValue = nullptr;

    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(pXamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    CFocusManager* focusManager = spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef()->GetFocusManagerNoRef();
    CDependencyObject* pDO = focusManager->GetFocusedElementNoRef();

    if (pDO != nullptr)
    {
        ctl::ComPtr<DependencyObject> pFocusedElement;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pDO, &pFocusedElement));
        IFC_RETURN(pFocusedElement.CopyTo(ppReturnValue));
    }

    return S_OK;
}
