// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ElementCompositionPreview.g.h"
#include "ScrollViewer.g.h"
#include "ManipulationTransform.h"
#include "PointerSourceWrapper.h"
#include "DCompTreeHost.h"
#include "host.h"
#include "windowsgraphicsdevicemanager.h"

using namespace DirectUI;
using namespace wrl::Wrappers;

// Returns the WinRT Composition Visual handoff visual for the provided UI element.
// The same visual is returned if GetElementVisualImpl is called multiple times in a row.
_Check_return_ HRESULT
ElementCompositionPreviewFactory::GetElementVisualImpl(
    _In_ xaml::IUIElement* pElement,
    _Outptr_ WUComp::IVisual** ppResult)
{
    *ppResult = nullptr;

    UIElement* pUIE = static_cast<UIElement*>(pElement);

    // Fail if this UIElement is a strict type or has opted into strict mode.
    IFC_RETURN(NonStrictOnlyApiCheck(pUIE, L"ElementCompositionPreview.GetElementVisual"));

    IFC_RETURN(pUIE->CheckThread());

    IFC_RETURN(pUIE->GetElementVisual(ppResult));

    return S_OK;
}

// Returns the WinRT Composition hand-in visual for the provided UI element, if any was previously set.
// Returns null otherwise. The same visual is returned if GetElementChildVisual is called multiple times in a row.
_Check_return_ HRESULT
ElementCompositionPreviewFactory::GetElementChildVisualImpl(
    _In_ xaml::IUIElement* pElement,
    _Outptr_ WUComp::IVisual** ppResult)
{
    *ppResult = nullptr;

    UIElement* pUIE = static_cast<UIElement*>(pElement);

    IFC_RETURN(pUIE->CheckThread());

    CUIElement* pUIElement = static_cast<CUIElement*>(pUIE->GetHandle());
    IFC_RETURN(pUIElement->GetHandInVisual(ppResult));

    return S_OK;
}

// Sets the provided WinRT Composition Visual as the hand-in visual for the provided UI element. Overrides any previously set visual.
_Check_return_ HRESULT
ElementCompositionPreviewFactory::SetElementChildVisualImpl(
    _In_ xaml::IUIElement* pElement,
    _In_opt_ WUComp::IVisual* pVisual)
{
    UIElement* pUIE = static_cast<UIElement*>(pElement);

    IFC_RETURN(pUIE->CheckThread());

    CUIElement* pUIElement = static_cast<CUIElement*>(pUIE->GetHandle());
    IFC_RETURN(pUIElement->SetHandInVisual(pVisual));

    return S_OK;
}

_Check_return_ HRESULT
ElementCompositionPreviewFactory::GetScrollViewerManipulationPropertySetImpl(
    _In_ xaml_controls::IScrollViewer* pScrollViewer,
    _Outptr_ WUComp::ICompositionPropertySet** ppResult)
{
    *ppResult = nullptr;

    ScrollViewer* pSV = static_cast<ScrollViewer*>(pScrollViewer);

    IFC_RETURN(pSV->CheckThread());

    CScrollViewer* coreScrollViewer = static_cast<CScrollViewer*>(pSV->GetHandle());
    IFC_RETURN(coreScrollViewer->EnsureManipulationTransformPropertySet(ppResult));

    return S_OK;
}

_Check_return_ HRESULT
ElementCompositionPreviewFactory::SetImplicitShowAnimationImpl(
    _In_ xaml::IUIElement* pElement,
    _In_opt_ WUComp::ICompositionAnimationBase* pAnimation)
{
    UIElement* uie = static_cast<UIElement*>(pElement);
    IFC_RETURN(uie->CheckThread());

    CUIElement* uielement = static_cast<CUIElement*>(uie->GetHandle());

    if (uie->HasShownHiddenHandlers())
    {
        IFC_RETURN(uielement->SetAndOriginateError(E_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SHOWN_HIDDEN_MIXED_WITH_ECP));
    }

    uielement->SetImplicitShowHideAnimation(ImplicitAnimationType::Show, pAnimation);

    return S_OK;
}

_Check_return_ HRESULT
ElementCompositionPreviewFactory::SetImplicitHideAnimationImpl(
    _In_ xaml::IUIElement* pElement,
    _In_opt_ WUComp::ICompositionAnimationBase* pAnimation)
{
    UIElement* uie = static_cast<UIElement*>(pElement);
    IFC_RETURN(uie->CheckThread());

    CUIElement* uielement = static_cast<CUIElement*>(uie->GetHandle());

    if (uie->HasShownHiddenHandlers())
    {
        IFC_RETURN(uielement->SetAndOriginateError(E_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SHOWN_HIDDEN_MIXED_WITH_ECP));
    }

    uielement->SetImplicitShowHideAnimation(ImplicitAnimationType::Hide, pAnimation);

    return S_OK;
}

_Check_return_ HRESULT
ElementCompositionPreviewFactory::SetIsTranslationEnabledImpl(
    _In_ xaml::IUIElement* pElement,
    _In_ BOOLEAN value)
{
    UIElement* uie = static_cast<UIElement*>(pElement);
    IFC_RETURN(uie->CheckThread());

    // Fail if this UIElement is a strict type or has opted into strict mode.
    IFC_RETURN(NonStrictOnlyApiCheck(uie, L"ElementCompositionPreview.SetIsTranslationEnabled"));

    CUIElement* uielement = static_cast<CUIElement*>(uie->GetHandle());
    uielement->SetIsTranslationEnabled(value == TRUE);

    return S_OK;
}

_Check_return_ HRESULT
ElementCompositionPreviewFactory::GetPointerPositionPropertySetImpl(
    _In_ xaml::IUIElement* pTargetElement,
    _Outptr_ WUComp::ICompositionPropertySet** ppResult)
{
    *ppResult = nullptr;

    auto uie = static_cast<UIElement*>(pTargetElement);
    IFC_RETURN(uie->CheckThread());

    const auto& coreServices = uie->GetHandle()->GetContext();

    auto graphicsDeviceManager = coreServices->GetBrowserHost()->GetGraphicsDeviceManager();
    IFCEXPECT_RETURN(graphicsDeviceManager);
    IFC_RETURN(graphicsDeviceManager->EnsureDCompDevice());
    DCompTreeHost *dcompTreeHost = graphicsDeviceManager->GetDCompTreeHost();

    CUIElement* pointerSourceElement = uie->GetHandle();

    // Lights have a feature where lights set on the root scroll viewer are lifted up to the root visual. We do this because
    // we assume that the app put them on the root scroll viewer with the intention of applying them to the entire tree, and
    // only placed them there because that's as high as the app can reach in the public UIElement tree.
    //
    // This policy affects the pointer position property set as well. In these cases the app passes in the root scroll viewer
    // and asks for its pointer position property set. In these cases we need to return the property set that's been created
    // from the root visual instead. If we return a hover pointer source on the root scroll viewer instead, then it will not
    // capture input when sibling roots like the full window media root are covering the root scroll viewer, which means
    // reveal lighting will still not work correctly.

    CRootVisual* rootVisualNoRef = coreServices->GetMainRootVisual();
    CUIElement* rootScrollViewOrCanvas = (rootVisualNoRef != nullptr) ? rootVisualNoRef->GetRootScrollViewerOrCanvas() : nullptr;

    if (rootScrollViewOrCanvas != nullptr
        && pointerSourceElement == rootScrollViewOrCanvas)
    {
        // If the app asks for the pointer position property set on the root scroll viewer, return one for the root visual instead.
        pointerSourceElement = rootVisualNoRef;
    }

    std::shared_ptr<PointerSourceWrapper> pointerSourceWrapper;
    {
        std::shared_ptr<void> pointerSourceWrapperAlias;
        IFC_RETURN(dcompTreeHost->GetPointerSourceForElement(pointerSourceElement, &pointerSourceWrapperAlias));

        if (pointerSourceWrapperAlias)
        {
            pointerSourceWrapper = reinterpret_cast<PointerSourceWrapper*>(pointerSourceWrapperAlias.get())->shared_from_this();
        }
        else
        {
            pointerSourceWrapper = std::make_shared<PointerSourceWrapper>();
            IFC_RETURN(pointerSourceWrapper->Initialize(pointerSourceElement));
            dcompTreeHost->StorePointerSourceForElement(
                pointerSourceElement,
                std::shared_ptr<void>(pointerSourceWrapper, pointerSourceWrapper.get()));
        }
    }

    wrl::ComPtr<WUComp::ICompositionAnimation> compositionAnimation;
    {
        wrl::ComPtr<WUComp::IExpressionAnimation> expressionAnimation;
        IFC_RETURN(dcompTreeHost->GetCompositor()->CreateExpressionAnimationWithExpression(
            HStringReference(L"Vector3(hover.Point.x, hover.Point.y, 0)").Get(),
            &expressionAnimation));
        IFC_RETURN(expressionAnimation.As(&compositionAnimation));

        IFC_RETURN(compositionAnimation->SetReferenceParameter(
            HStringReference(L"hover").Get(),
            pointerSourceWrapper->GetPointerSourceProxy()));
    }

    // Return new PropertySet each time in case the app decides to close or change others
    wrl::ComPtr<WUComp::ICompositionPropertySet> pointerPositionPropertySet;
    {
        HStringReference pointerPositionPropName(L"Position");

        IFC_RETURN(dcompTreeHost->GetCompositor()->CreatePropertySet(&pointerPositionPropertySet));
        IFC_RETURN(pointerPositionPropertySet->InsertVector3(pointerPositionPropName.Get(), { 0 }));

        wrl::ComPtr<WUComp::ICompositionObject> pointerPositionPropertySetCO;
        IFC_RETURN(pointerPositionPropertySet.As(&pointerPositionPropertySetCO));

        IFC_RETURN(pointerPositionPropertySetCO->StartAnimation(pointerPositionPropName.Get(), compositionAnimation.Get()));
    }

    *ppResult = pointerPositionPropertySet.Detach();

    return S_OK;
}
