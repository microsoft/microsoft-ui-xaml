// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "UIElement.g.h"
#include "AutomationPeer.g.h"
#include "ContainerContentChangingEventArgs.g.h"
#include "MatrixTransform.g.h"
#include "Pointer.g.h"
#include "StackPanel.g.h"
#include "Popup.g.h"
#include "RootScrollViewer.g.h"
#include "RoutedEvent.g.h"
#include "StickyHeaderWrapper.h"
#include <XamlDiagnostics.h>
#include "VisualTreeHelper.h"
#include "AKCommon.h"
#include <AutomationProperty.g.h>
#include "StaticStore.h"
#include <LayoutTransition_Partial.h>
#include <XamlLightCollection.g.h>
#include <Microsoft.UI.Xaml.coretypes.h>
#include "EventMgr.h"
#include <FocusProperties.h>
#include <DefaultFocusChildrenIterable.h>
#include "KeyboardAcceleratorUtility.h"
#include "ProcessKeyboardAcceleratorEventArgs.g.h"
#include "KeyboardAcceleratorInvokedEventArgs.g.h"
#include "focusmgr.h"
#include "Button.g.h"
#include "RoutedEventArgs.h"
#include "InteractionCollection.h"
#include "IApplicationBarService.h"
#include <FeatureFlags.h>
#include "ElevationHelper.h"
#include "XamlRoot_partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

// Uncomment for handoff visual debug traces
// #define HOVE_DBG

// Initializes a new instance of the UIElement class.
UIElement::UIElement()
    : m_IsLocationValid(false)
    , m_isGeneratedContainer(false)
    , m_animateIfBringIntoView(false)
    , m_pVirtualizationInformation()
{

}

// Destroys an instance of the UIElement class.
UIElement::~UIElement()
{
    if (DXamlCore::GetCurrent() != nullptr)
    {
        DXamlCore::GetCurrent()->SetAutomaticDragHelper(this, nullptr);
    }

    ResetAutomationPeer();
}

CUIElement* UIElement::GetHandle() const
{
    auto result = DependencyObject::GetHandle();
    ASSERT(!result || result->OfTypeByIndex<KnownTypeIndex::UIElement>());
    return static_cast<CUIElement*>(result);
}

// Reset AutomationPeer when it is no longer needed.
void UIElement::ResetAutomationPeer()
{
    if (auto peg = m_tpAP.TryMakeAutoPeg())
    {
        m_tpAP.Cast<AutomationPeer>()->NotifyManagedUIElementIsDead();
        m_tpAP.Clear();
    }
}

IFACEMETHODIMP UIElement::get_RenderTransform(
    _Outptr_ xaml_media::ITransform** pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_media::ITransform> spTransform;

    IFC(UIElementGenerated::get_RenderTransform(&spTransform));
    if (!spTransform)
    {
        // If the property is null, create an identity transform on the fly.
        ctl::ComPtr<MatrixTransform> spNewTransform;
        IFC(ctl::make<MatrixTransform>(&spNewTransform));
        spTransform = spNewTransform;
    }

    IFC(spTransform.CopyTo(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::get_TranslationImpl(_Out_ wfn::Vector3* translation)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *translation = uie->GetTranslation();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_TranslationImpl(const wfn::Vector3& translation)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    uie->SetTranslation(translation);
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_RotationImpl(_Out_ FLOAT* rotation)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *rotation = uie->GetRotation();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_RotationImpl(FLOAT rotation)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    uie->SetRotation(rotation);
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_ScaleImpl(_Out_ wfn::Vector3* scale)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *scale = uie->GetScale();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_ScaleImpl(const wfn::Vector3& scale)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    uie->SetScale(scale);
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_TransformMatrixImpl(_Out_ wfn::Matrix4x4* transformMatrix)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *transformMatrix = uie->GetTransformMatrix();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_TransformMatrixImpl(const wfn::Matrix4x4& transformMatrix)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    uie->SetTransformMatrix(transformMatrix);
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_CenterPointImpl(_Out_ wfn::Vector3* centerPoint)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *centerPoint = uie->GetCenterPoint();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_CenterPointImpl(const wfn::Vector3& centerPoint)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    uie->SetCenterPoint(centerPoint);
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_RotationAxisImpl(_Out_ wfn::Vector3* rotationAxis)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *rotationAxis = uie->GetRotationAxis();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_RotationAxisImpl(const wfn::Vector3& rotationAxis)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    uie->SetRotationAxis(rotationAxis);
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_RasterizationScaleImpl(_Out_ DOUBLE* rasterizationScale)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *rasterizationScale = uie->GetRasterizationScale();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_RasterizationScaleImpl(const DOUBLE rasterizationScale)
{
    if (rasterizationScale <= 0)
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_RASTERIZATIONSCALE_MUST_BE_POSITIVE));
    }

    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    uie->SetRasterizationScale(rasterizationScale);
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_ActualOffsetImpl(_Out_ wfn::Vector3* layoutOffset)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *layoutOffset = uie->GetActualOffset();
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_ActualSizeImpl(_Out_ wfn::Vector2* layoutSize)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    *layoutSize = uie->GetActualSize();
    return S_OK;
}

_Check_return_ HRESULT UIElement::StartAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    return uie->StartAnimation(animation);
}

_Check_return_ HRESULT UIElement::StopAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    return uie->StopAnimation(animation);
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::PopulatePropertyInfo(
    _In_ HSTRING propertyName,
    _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
    )
{
    return PopulatePropertyInfoOverrideProtected(propertyName, animationPropertyInfo);
}

_Check_return_ HRESULT UIElement::PopulatePropertyInfoOverrideImpl(
    _In_ HSTRING propertyName,
    _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
    )
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    return uie->PopulatePropertyInfo(propertyName, animationPropertyInfo);
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::GetVisualInternal(_Outptr_ WUComp::IVisual** visual)
{
    return GetElementVisual(visual);
}

// Gets the size that this UIElement computed during the measure pass of the
// layout process.
_Check_return_ HRESULT UIElement::get_DesiredSizeImpl(
    _Out_ wf::Size* pValue)
{
    HRESULT hr = S_OK;
    FLOAT width = 0.0;
    FLOAT height = 0.0;

    IFCPTR(pValue);

    IFC(CoreImports::UIElement_GetDesiredSize(
        static_cast<CUIElement*>(GetHandle()),
        &width,
        &height));

    pValue->Width = width;
    pValue->Height = height;

Cleanup:
    RRETURN(hr);
}

// Updates the DesiredSize of a UIElement. Typically, objects that implement
// custom layout for their layout children call this method from their own
// MeasureOverride implementations to form a recursive layout update.
_Check_return_ HRESULT UIElement::MeasureImpl(
    _In_ wf::Size availableSize)
{
    HRESULT hr = S_OK;

    // availableSize should not have NaN values
    if (DoubleUtil::IsNaN(availableSize.Width) ||
        DoubleUtil::IsNaN(availableSize.Height))
    {
        IFC(CoreImports::LayoutInformation_SetLayoutExceptionElement(
            DXamlCore::GetCurrent()->GetHandle(),
            static_cast<CUIElement*>(GetHandle())));
        // TODO: Throw exception
        IFC(E_FAIL);
    }

    IFC(CoreImports::UIElement_Measure(
        static_cast<CUIElement*>(GetHandle()),
        static_cast<FLOAT>(availableSize.Width),
        static_cast<FLOAT>(availableSize.Height)));

Cleanup:
    RRETURN(hr);
}

// Returns True when this element is about to be re-arranged.
bool UIElement::IsArrangeDirty()
{
    CUIElement* pElement = static_cast<CUIElement*>(GetHandle());
    return pElement->GetIsArrangeDirty() || pElement->GetIsOnArrangeDirtyPath();
}

// Positions child objects and determines a size for a UIElement.
// Parent objects that implement custom layout for their child
// elements should call this method from their layout override
// implementations to form a recursive layout update.
_Check_return_ HRESULT UIElement::ArrangeImpl(
    _In_ wf::Rect finalRect)
{
    HRESULT hr = S_OK;

    // finalRect should not have NaN or Infinite values
    if (DoubleUtil::IsPositiveInfinity(finalRect.Width) ||
        DoubleUtil::IsPositiveInfinity(finalRect.Height) ||
        DoubleUtil::IsNaN(finalRect.Width) ||
        DoubleUtil::IsNaN(finalRect.Height) ||
        DoubleUtil::IsInfinity(finalRect.X) ||
        DoubleUtil::IsInfinity(finalRect.Y) ||
        DoubleUtil::IsNaN(finalRect.X) ||
        DoubleUtil::IsNaN(finalRect.Y))
    {
        IFC(CoreImports::LayoutInformation_SetLayoutExceptionElement(
            DXamlCore::GetCurrent()->GetHandle(),
            static_cast<CUIElement*>(GetHandle())));
        // TODO: Throw exception
        IFC(E_FAIL);
    }

    IFC(CoreImports::UIElement_Arrange(
        static_cast<CUIElement*>(GetHandle()),
        static_cast<FLOAT>(finalRect.X),
        static_cast<FLOAT>(finalRect.Y),
        static_cast<FLOAT>(finalRect.Width),
        static_cast<FLOAT>(finalRect.Height)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::InvalidateArrangeImpl()
{
    // If we're in the process of shutting down, this UIElement might have
    // already been disconnected from the core. If that is the case, GetHandle
    // will return null, and the subsequent call to InvalidateArrange will
    // result in an access violation. In order to prevent this, we're
    // explicitly checking for null here. Note: This is not intended to
    // establish a precedent and, if similar issues arise in the future, we
    // will need a full-fledged solution that would work across the board.
    // We're opting for the current approach because InvalidateArrange
    // in particular does not have any externally visible consequences.
    // (See RS Bug #1563794).
    CUIElement* pElement = static_cast<CUIElement*>(GetHandle());

    if (pElement)
    {
        pElement->InvalidateArrange();
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::InvalidateMeasureImpl()
{
    // If we're in the process of shutting down, this UIElement might have
    // already been disconnected from the core. If that is the case, GetHandle
    // will return null, and the subsequent call to InvalidateMeasure will
    // result in an access violation. In order to prevent this, we're
    // explicitly checking for null here. Note: This is not intended to
    // establish a precedent and, if similar issues arise in the future, we
    // will probably need a full-fledged solution that would work across the board.
    // We're opting for the current approach because InvalidateMeasure
    // in particular does not have any externally visible consequences.
    // (See RS Bug #1563794).
    CUIElement* pElement = static_cast<CUIElement*>(GetHandle());

    if (pElement)
    {
        pElement->InvalidateMeasure();
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::CapturePointerImpl(
    _In_ xaml_input::IPointer* value, _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    bool returnValueNative;
    CPointer* pValueNative = static_cast<CPointer*>(value ? static_cast<Pointer*>(value)->GetHandle() : NULL);

    IFC(CoreImports::CUIElement_CapturePointer(static_cast<CUIElement*>(GetHandle()), pValueNative, &returnValueNative));

    *returnValue = static_cast<BOOLEAN>(returnValueNative);

    if (*returnValue)
    {
        IFC(OnPointerCaptured());
    }

Cleanup:
    RRETURN(hr);
}

// Attempts to bring the given rectangle of this element into view by
// originating a RequestBringIntoView event.
void UIElement::BringIntoView(
    wf::Rect bounds,
    BOOLEAN forceIntoView,
    BOOLEAN useAnimation,
    BOOLEAN skipDuringManipulation,
    double horizontalAlignmentRatio,
    double verticalAlignmentRatio,
    double offsetX,
    double offsetY)
{
    XRECTF coreBounds;

    coreBounds.X = bounds.X;
    coreBounds.Y = bounds.Y;
    coreBounds.Width = bounds.Width;
    coreBounds.Height = bounds.Height;

    static_cast<CUIElement*>(GetHandle())->BringIntoView(coreBounds, forceIntoView, useAnimation, skipDuringManipulation, horizontalAlignmentRatio, verticalAlignmentRatio, offsetX, offsetY);
}

// Called by non-core elements that declare themselves as an IDirectManipulationContainer
// implementation
_Check_return_ HRESULT UIElement::put_IsDirectManipulationContainer(
    _In_ BOOLEAN isDirectManipulationContainer)
{
    CInputServices* inputServices = static_cast<CUIElement*>(GetHandle())->GetContext()->GetInputServices();
    IFC_RETURN(inputServices->RegisterDirectManipulationContainer(static_cast<CUIElement*>(GetHandle()), static_cast<bool>(isDirectManipulationContainer)));

    return S_OK;
}

// Used to register or unregister this element as a dummy DM container.
_Check_return_ HRESULT UIElement::put_IsDirectManipulationCrossSlideContainer(
    _In_ BOOLEAN isDirectManipulationCrossSlideContainer)
{
    CInputServices* inputServices = static_cast<CUIElement*>(GetHandle())->GetContext()->GetInputServices();
    IFC_RETURN(inputServices->RegisterDirectManipulationCrossSlideContainer(static_cast<CUIElement*>(GetHandle()), static_cast<bool>(isDirectManipulationCrossSlideContainer)));

    return S_OK;
}

// Used to declare that the cross-slide viewport for this element can be discarded.
_Check_return_ HRESULT UIElement::DirectManipulationCrossSlideContainerCompleted()
{
    CInputServices* inputServices = static_cast<CUIElement*>(GetHandle())->GetContext()->GetInputServices();
    IFC_RETURN(inputServices->DirectManipulationCrossSlideContainerCompleted(static_cast<CUIElement*>(GetHandle()), nullptr /*pCrossSlideViewport*/));

    return S_OK;
}

// Used to determine if an inner element can be manipulated.
_Check_return_ HRESULT UIElement::GetCanManipulateElements(
    _In_ CUIElement* nativeTarget,
    _Out_ bool* pfCanManipulateElementsByTouch,
    _Out_ bool* pfCanManipulateElementsNonTouch,
    _Out_ bool* pfCanManipulateElementsWithBringIntoViewport)
{
    HRESULT hr = S_OK;
    BOOLEAN canManipulateElementsByTouch;
    BOOLEAN canManipulateElementsNonTouch;
    BOOLEAN canManipulateElementsWithBringIntoViewport;
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR(nativeTarget);
    IFCPTR(pfCanManipulateElementsByTouch);
    *pfCanManipulateElementsByTouch = FALSE;
    IFCPTR(pfCanManipulateElementsNonTouch);
    *pfCanManipulateElementsNonTouch = FALSE;
    IFCPTR(pfCanManipulateElementsWithBringIntoViewport);
    *pfCanManipulateElementsWithBringIntoViewport = FALSE;

    // During Window deactivate, the target may already be partially destructed (its controlling unknown may be
    // gone already).  So get the peer "WithInternalRef".
    IFC(DXamlCore::GetCurrent()->GetPeerWithInternalRef(nativeTarget, &spTarget));

    IFC(spTarget.Cast<UIElement>()->get_CanManipulateElements(&canManipulateElementsByTouch, &canManipulateElementsNonTouch, &canManipulateElementsWithBringIntoViewport));
    *pfCanManipulateElementsByTouch = static_cast<bool>(canManipulateElementsByTouch);
    *pfCanManipulateElementsNonTouch = static_cast<bool>(canManipulateElementsNonTouch);
    *pfCanManipulateElementsWithBringIntoViewport = static_cast<bool>(canManipulateElementsWithBringIntoViewport);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::get_CanManipulateElements(
    _Out_ BOOLEAN* pCanManipulateElementsByTouch,
    _Out_ BOOLEAN* pCanManipulateElementsNonTouch,
    _Out_ BOOLEAN* pCanManipulateElementsWithBringIntoViewport)
{
    HRESULT hr = S_OK;

    IFCPTR(pCanManipulateElementsByTouch);
    *pCanManipulateElementsByTouch = FALSE;
    IFCPTR(pCanManipulateElementsNonTouch);
    *pCanManipulateElementsNonTouch = FALSE;
    IFCPTR(pCanManipulateElementsWithBringIntoViewport);
    *pCanManipulateElementsWithBringIntoViewport = FALSE;

Cleanup:
    RRETURN(hr);
}

// Used to provide a manipulation handler for the control-to-InputManager notifications
_Check_return_ HRESULT UIElement::SetManipulationHandler(
    _In_ CUIElement* nativeTarget,
    _In_opt_ void* nativeManipulationHandler)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    IFC(spTarget.Cast<UIElement>()->put_ManipulationHandler(static_cast<HANDLE>(nativeManipulationHandler)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::put_ManipulationHandler(
    _In_opt_ HANDLE hManipulationHandler)
{
    RRETURN(E_NOTIMPL);
}

// Used to tell the container if the manipulation handler wants to be
// aware of manipulation characteristic changes even though no manipulation
// is in progress.
_Check_return_ HRESULT UIElement::SetManipulationHandlerWantsNotifications(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _In_ bool fWantsNotifications)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));

    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->SetManipulationHandlerWantsNotifications(
        spManipulatedElement.Cast<UIElement>(),
        static_cast<BOOLEAN>(fWantsNotifications)));

    return S_OK;
}

_Check_return_ HRESULT UIElement::SetManipulationHandlerWantsNotifications(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN wantsNotifications)
{
    RRETURN(E_NOTIMPL);
}

// Used to set the pointed dependency object representing the touched element
// triggering a touch-based manipulation.
_Check_return_ HRESULT UIElement::SetPointedElement(
    _In_ CUIElement* nativeTarget,
    _In_ CDependencyObject* nativePointedElement)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spPointedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativePointedElement);

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(pCore->GetPeer(nativePointedElement, &spPointedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->SetPointedElement(
        spPointedElement.Get()));

    return S_OK;
}

_Check_return_ HRESULT UIElement::SetPointedElement(
    _In_ DependencyObject* pPointedElement)
{
    RRETURN(E_NOTIMPL);
}

// Used to access a manipulated element.
_Check_return_ HRESULT UIElement::GetManipulatedElement(
    _In_ CUIElement* nativeTarget,
    _In_opt_ CDependencyObject* nativePointedElement,
    _In_opt_ CUIElement* nativeChildElement,
    _Out_ CUIElement** nativeManipulatedElement)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spPointedElement;
    ctl::ComPtr<DependencyObject> spChildElement;

    ctl::ComPtr<UIElement> spManipulatedUIElement;

    IFCPTR_RETURN(nativePointedElement);
    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    *nativeManipulatedElement = nullptr;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));

    IFC_RETURN(pCore->GetPeer(nativePointedElement, &spPointedElement));

    // TODO: This is dead code, and needs to be updated. See https://task.ms/win8/993954
    if (nativeChildElement)
    {
        IFC_RETURN(pCore->GetPeer(nativeChildElement, &spChildElement));
    }

    IFC_RETURN(spTarget.Cast<UIElement>()->GetManipulatedElement(
        spPointedElement.Get(),
        spChildElement.Cast<UIElement>(),
        &spManipulatedUIElement));

    *nativeManipulatedElement = static_cast<CUIElement*>(spManipulatedUIElement.Detach()->GetHandle());

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetManipulatedElement(
    _In_ DependencyObject* pPointedElement,
    _In_opt_ UIElement* pChildElement,
    _Out_ UIElement** ppManipulatedElement)
{
    RRETURN(E_NOTIMPL);
}

// Used to access information about a manipulation viewport.
_Check_return_ HRESULT UIElement::GetManipulationViewport(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ CMILMatrix* pInputTransform,
    _Out_opt_ XUINT32* pTouchConfiguration,
    _Out_opt_ XUINT32* pNonTouchConfiguration,
    _Out_opt_ XUINT32* pBringIntoViewportConfiguration,
    _Out_opt_ XUINT32* pHorizontalOverpanMode,
    _Out_opt_ XUINT32* pVerticalOverpanMode,
    _Out_opt_ XUINT8* pcConfigurations,
    _Outptr_result_buffer_maybenull_(*pcConfigurations) XUINT32** ppConfigurations,
    _Out_opt_ XUINT32* pChainedMotionTypes)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    if (pBounds)
    {
        pBounds->X = pBounds->Y = pBounds->Width = pBounds->Height = 0.0f;
    }
    if (pTouchConfiguration)
    {
        *pTouchConfiguration = 0;
    }
    if (pNonTouchConfiguration)
    {
        *pNonTouchConfiguration = 0;
    }
    if (pBringIntoViewportConfiguration)
    {
        *pBringIntoViewportConfiguration = 0;
    }
    if (pcConfigurations)
    {
        *pcConfigurations = 0;
    }
    if (ppConfigurations)
    {
        *ppConfigurations = nullptr;
    }
    if (pChainedMotionTypes)
    {
        *pChainedMotionTypes = 0;
    }

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->GetManipulationViewport(
        spManipulatedElement.Cast<UIElement>(),
        pBounds,
        pInputTransform,
        reinterpret_cast<DMConfigurations*>(pTouchConfiguration),
        reinterpret_cast<DMConfigurations*>(pNonTouchConfiguration),
        reinterpret_cast<DMConfigurations*>(pBringIntoViewportConfiguration),
        reinterpret_cast<DMOverpanMode*>(pHorizontalOverpanMode),
        reinterpret_cast<DMOverpanMode*>(pVerticalOverpanMode),
        pcConfigurations,
        reinterpret_cast<DMConfigurations**>(ppConfigurations),
        reinterpret_cast<DMMotionTypes*>(pChainedMotionTypes)));

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetManipulationViewport(
    _In_ UIElement* pManipulatedElement,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ CMILMatrix* pInputTransform,
    _Out_opt_ DMConfigurations* pTouchConfiguration,
    _Out_opt_ DMConfigurations* pNonTouchConfiguration,
    _Out_opt_ DMConfigurations* pBringIntoViewportConfiguration,
    _Out_opt_ DMOverpanMode* pHorizontalOverpanMode,
    _Out_opt_ DMOverpanMode* pVerticalOverpanMode,
    _Out_opt_ UINT8* pcConfigurations,
    _Outptr_result_buffer_maybenull_(*pcConfigurations) DMConfigurations** ppConfigurations,
    _Out_opt_ DMMotionTypes* pChainedMotionTypes)
{
    RRETURN(E_NOTIMPL);
}

// Used to access information about a manipulation primary content.
_Check_return_ HRESULT UIElement::GetManipulationPrimaryContent(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _Out_opt_ XSIZEF* pOffsets,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ XUINT32* pHorizontalAligment,
    _Out_opt_ XUINT32* pVerticalAligment,
    _Out_opt_ XFLOAT* pMinZoomFactor,
    _Out_opt_ XFLOAT* pMaxZoomFactor,
    _Out_opt_ bool* pfIsHorizontalStretchAlignmentTreatedAsNear,
    _Out_opt_ bool* pfIsVerticalStretchAlignmentTreatedAsNear,
    _Out_opt_ bool* pfIsLayoutRefreshed)
{
    BOOLEAN isHorizontalStretchAlignmentTreatedAsNear = FALSE;
    BOOLEAN isVerticalStretchAlignmentTreatedAsNear = FALSE;
    BOOLEAN isLayoutRefreshed = FALSE;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    if (pOffsets)
    {
        pOffsets->width = pOffsets->height = 0.0f;
    }
    if (pBounds)
    {
        pBounds->X = pBounds->Y = pBounds->Width = pBounds->Height = 0.0f;
    }
    if (pHorizontalAligment)
    {
        *pHorizontalAligment = 0;
    }
    if (pVerticalAligment)
    {
        *pVerticalAligment = 0;
    }
    if (pMinZoomFactor)
    {
        *pMinZoomFactor = 0;
    }
    if (pMaxZoomFactor)
    {
        *pMaxZoomFactor = 0;
    }
    if (pfIsHorizontalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pfIsHorizontalStretchAlignmentTreatedAsNear = FALSE;
    }
    if (pfIsVerticalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pfIsVerticalStretchAlignmentTreatedAsNear = FALSE;
    }
    if (pfIsLayoutRefreshed)
    {
        *pfIsLayoutRefreshed = FALSE;
    }

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->GetManipulationPrimaryContent(
        spManipulatedElement.Cast<UIElement>(),
        pOffsets,
        pBounds,
        reinterpret_cast<DMAlignment*>(pHorizontalAligment),
        reinterpret_cast<DMAlignment*>(pVerticalAligment),
        pMinZoomFactor,
        pMaxZoomFactor,
        pfIsHorizontalStretchAlignmentTreatedAsNear ? &isHorizontalStretchAlignmentTreatedAsNear : nullptr,
        pfIsVerticalStretchAlignmentTreatedAsNear ? &isVerticalStretchAlignmentTreatedAsNear : nullptr,
        pfIsLayoutRefreshed ? &isLayoutRefreshed : nullptr));

    if (pfIsHorizontalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pfIsHorizontalStretchAlignmentTreatedAsNear = isHorizontalStretchAlignmentTreatedAsNear;
    }

    if (pfIsVerticalStretchAlignmentTreatedAsNear != nullptr)
    {
        *pfIsVerticalStretchAlignmentTreatedAsNear = isVerticalStretchAlignmentTreatedAsNear;
    }

    if (pfIsLayoutRefreshed)
    {
        *pfIsLayoutRefreshed = static_cast<bool>(isLayoutRefreshed);
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetManipulationPrimaryContent(
    _In_ UIElement* pManipulatedElement,
    _Out_opt_ XSIZEF* pOffsets,
    _Out_opt_ XRECTF* pBounds,
    _Out_opt_ DMAlignment* pHorizontalAligment,
    _Out_opt_ DMAlignment* pVerticalAligment,
    _Out_opt_ FLOAT* pMinZoomFactor,
    _Out_opt_ FLOAT* pMaxZoomFactor,
    _Out_opt_ BOOLEAN* pIsHorizontalStretchAlignmentTreatedAsNear,
    _Out_opt_ BOOLEAN* pIsVerticalStretchAlignmentTreatedAsNear,
    _Out_opt_ BOOLEAN* pIsLayoutRefreshed)
{
    RRETURN(E_NOTIMPL);
}

// Used to access information about a manipulation secondary content.
_Check_return_ HRESULT UIElement::GetManipulationSecondaryContent(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeContentElement,
    _Out_ XSIZEF* pOffsets)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spContentElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeContentElement);
    IFCPTR_RETURN(pOffsets);

    pOffsets->width = pOffsets->height = 0.0f;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(pCore->GetPeer(nativeContentElement, &spContentElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->GetManipulationSecondaryContent(
        spContentElement.Cast<UIElement>(),
        pOffsets));

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetManipulationSecondaryContent(
    _In_ UIElement* pContentElement,
    _Out_ XSIZEF* pOffsets)
{
    RRETURN(E_NOTIMPL);
}

// Used to access information about a manipulation primary content transform.
_Check_return_ HRESULT UIElement::GetManipulationPrimaryContentTransform(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _In_ bool fInManipulation,
    _In_ bool fForInitialTransformationAdjustment,
    _In_ bool fForMargins,
    _Out_opt_ XFLOAT* pTranslationX,
    _Out_opt_ XFLOAT* pTranslationY,
    _Out_opt_ XFLOAT* pZoomFactor)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    if (pTranslationX)
    {
        *pTranslationX = 0;
    }
    if (pTranslationY)
    {
        *pTranslationY = 0;
    }
    if (pZoomFactor)
    {
        *pZoomFactor = 0;
    }

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));

    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->GetManipulationPrimaryContentTransform(
        spManipulatedElement.Cast<UIElement>(),
        static_cast<BOOLEAN>(fInManipulation),
        static_cast<BOOLEAN>(fForInitialTransformationAdjustment),
        static_cast<BOOLEAN>(fForMargins),
        pTranslationX,
        pTranslationY,
        pZoomFactor));

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetManipulationPrimaryContentTransform(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN inManipulation,
    _In_ BOOLEAN forInitialTransformationAdjustment,
    _In_ BOOLEAN forMargins,
    _Out_opt_ FLOAT* pTranslationX,
    _Out_opt_ FLOAT* pTranslationY,
    _Out_opt_ FLOAT* pZoomFactor)
{
    RRETURN(E_NOTIMPL);
}

// Used to access information about a secondary content's transform.
_Check_return_ HRESULT UIElement::GetManipulationSecondaryContentTransform(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeContentElement,
    _Out_ XFLOAT* pTranslationX,
    _Out_ XFLOAT* pTranslationY,
    _Out_ XFLOAT* pZoomFactor)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spContentElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeContentElement);
    IFCPTR_RETURN(pTranslationX);
    *pTranslationX = 0.0f;
    IFCPTR_RETURN(pTranslationY);
    *pTranslationY = 0.0f;
    IFCPTR_RETURN(pZoomFactor);
    *pZoomFactor = 1.0f;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(pCore->GetPeer(nativeContentElement, &spContentElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->GetManipulationSecondaryContentTransform(
        spContentElement.Cast<UIElement>(),
        pTranslationX,
        pTranslationY,
        pZoomFactor));

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetManipulationSecondaryContentTransform(
    _In_ UIElement* pContentElement,
    _Out_ FLOAT* pTranslationX,
    _Out_ FLOAT* pTranslationY,
    _Out_ FLOAT* pZoomFactor)
{
    RRETURN(E_NOTIMPL);
}

// Used to access information about a manipulation primary content.
_Check_return_ HRESULT UIElement::GetManipulationSnapPoints(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _In_ XUINT32 motionType,
    _Out_ bool* pfAreSnapPointsOptional,
    _Out_ bool* pfAreSnapPointsSingle,
    _Out_ bool* pfAreSnapPointsRegular,
    _Out_ XFLOAT* pRegularOffset,
    _Out_ XFLOAT* pRegularInterval,
    _Out_ XUINT32* pcIrregularSnapPoints,
    _Outptr_result_buffer_(*pcIrregularSnapPoints) XFLOAT** ppIrregularSnapPoints,
    _Out_ XUINT32* pSnapCoordinate)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;
    BOOLEAN bAreSnapPointsOptional = FALSE;
    BOOLEAN bAreSnapPointsSingle = FALSE;
    BOOLEAN bAreSnapPointsRegular = FALSE;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    IFCPTR_RETURN(pfAreSnapPointsOptional);
    IFCPTR_RETURN(pfAreSnapPointsSingle);
    IFCPTR_RETURN(pfAreSnapPointsRegular);
    IFCPTR_RETURN(pRegularOffset);
    IFCPTR_RETURN(pRegularInterval);
    IFCPTR_RETURN(pcIrregularSnapPoints);
    IFCPTR_RETURN(ppIrregularSnapPoints);
    IFCPTR_RETURN(pSnapCoordinate);

    *pfAreSnapPointsOptional = FALSE;
    *pfAreSnapPointsSingle = FALSE;
    *pfAreSnapPointsRegular = FALSE;
    *pRegularOffset = 0.0;
    *pRegularInterval = 0.0;
    *pcIrregularSnapPoints = 0;
    *ppIrregularSnapPoints = nullptr;
    *pSnapCoordinate = (motionType == DMMotionTypeZoom) ? DMSnapCoordinateOrigin : DMSnapCoordinateBoundary;

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->GetManipulationSnapPoints(
        spManipulatedElement.Cast<UIElement>(),
        static_cast<DMMotionTypes>(motionType),
        &bAreSnapPointsOptional,
        &bAreSnapPointsSingle,
        &bAreSnapPointsRegular,
        pRegularOffset,
        pRegularInterval,
        pcIrregularSnapPoints,
        ppIrregularSnapPoints,
        reinterpret_cast<DMSnapCoordinate*>(pSnapCoordinate)));

    *pfAreSnapPointsOptional = bAreSnapPointsOptional;
    *pfAreSnapPointsSingle = bAreSnapPointsSingle;
    *pfAreSnapPointsRegular = bAreSnapPointsRegular;

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetManipulationSnapPoints(
    _In_ UIElement* pManipulatedElement,        // Manipulated element.
    _In_ DMMotionTypes motionType,
    _Out_ BOOLEAN* pAreSnapPointsOptional,
    _Out_ BOOLEAN* pAreSnapPointsSingle,
    _Out_ BOOLEAN* pAreSnapPointsRegular,
    _Out_ FLOAT* pRegularOffset,
    _Out_ FLOAT* pRegularInterval,
    _Out_ UINT32* pcIrregularSnapPoints,
    _Outptr_result_buffer_(*pcIrregularSnapPoints) FLOAT** ppIrregularSnapPoints,
    _Out_ DMSnapCoordinate* pSnapCoordinate)
{
    RRETURN(E_NOTIMPL);
}

// Used when the container needs to reevaluate the value returned by
// GetCanManipulateElements because a characteristic affecting the
// manipulability of its children has changed.
_Check_return_ HRESULT UIElement::NotifyManipulatabilityAffectingPropertyChanged(
    _In_ CUIElement* nativeTarget,
    _In_ bool fIsInLiveTree)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    IFC(spTarget.Cast<UIElement>()->NotifyManipulatabilityAffectingPropertyChanged(static_cast<BOOLEAN>(fIsInLiveTree)));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::NotifyManipulatabilityAffectingPropertyChanged(
    _In_ BOOLEAN isInLiveTree)
{
    RRETURN(S_OK);
}

// Used when the container needs to reevaluate the alignment of the
// provided manipulated element because an alignemnt characteristic has changed.
_Check_return_ HRESULT UIElement::NotifyContentAlignmentAffectingPropertyChanged(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _In_ bool fIsForHorizontalAlignment,
    _In_ bool fIsForStretchAlignment,
    _In_ bool fIsStretchAlignmentTreatedAsNear)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));
    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->NotifyContentAlignmentAffectingPropertyChanged(
        spManipulatedElement.Cast<UIElement>(),
        static_cast<BOOLEAN>(fIsForHorizontalAlignment),
        static_cast<BOOLEAN>(fIsForStretchAlignment),
        static_cast<BOOLEAN>(fIsStretchAlignmentTreatedAsNear)));

    return S_OK;
}

_Check_return_ HRESULT UIElement::NotifyContentAlignmentAffectingPropertyChanged(
    _In_ UIElement* pManipulatedElement,
    _In_ BOOLEAN isForHorizontalAlignment,
    _In_ BOOLEAN isForStretchAlignment,
    _In_ BOOLEAN isStretchAlignmentTreatedAsNear)
{
    RRETURN(S_OK);
}

// Used to notify an IDirectManipulationContainer implementation of a manipulation's progress
_Check_return_ HRESULT UIElement::NotifyManipulationProgress(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _In_ XUINT32 state,
    _In_ XFLOAT xCumulativeTranslation,
    _In_ XFLOAT yCumulativeTranslation,
    _In_ XFLOAT zCumulativeFactor,
    _In_ XFLOAT xInertiaEndTranslation,
    _In_ XFLOAT yInertiaEndTranslation,
    _In_ XFLOAT zInertiaEndFactor,
    _In_ XFLOAT xCenter,
    _In_ XFLOAT yCenter,
    _In_ bool fIsInertiaEndTransformValid,
    _In_ bool fIsInertial,
    _In_ bool fIsTouchConfigurationActivated,
    _In_ bool fIsBringIntoViewportConfigurationActivated)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));

    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->NotifyManipulationProgress(
        spManipulatedElement.Cast<UIElement>(),
        static_cast<DMManipulationState>(state),
        static_cast<FLOAT>(xCumulativeTranslation),
        static_cast<FLOAT>(yCumulativeTranslation),
        static_cast<FLOAT>(zCumulativeFactor),
        static_cast<FLOAT>(xInertiaEndTranslation),
        static_cast<FLOAT>(yInertiaEndTranslation),
        static_cast<FLOAT>(zInertiaEndFactor),
        static_cast<FLOAT>(xCenter),
        static_cast<FLOAT>(yCenter),
        static_cast<BOOLEAN>(fIsInertiaEndTransformValid),
        static_cast<BOOLEAN>(fIsInertial),
        static_cast<BOOLEAN>(fIsTouchConfigurationActivated),
        static_cast<BOOLEAN>(fIsBringIntoViewportConfigurationActivated)));

    return S_OK;
}

_Check_return_ HRESULT UIElement::NotifyManipulationProgress(
    _In_ UIElement* pManipulatedElement,
    _In_ DMManipulationState state,
    _In_ FLOAT xCumulativeTranslation,
    _In_ FLOAT yCumulativeTranslation,
    _In_ FLOAT zCumulativeFactor,
    _In_ FLOAT xInertiaEndTranslation,
    _In_ FLOAT yInertiaEndTranslation,
    _In_ FLOAT zInertiaEndFactor,
    _In_ FLOAT xCenter,
    _In_ FLOAT yCenter,
    _In_ BOOLEAN isInertiaEndTransformValid,
    _In_ BOOLEAN isInertial,
    _In_ BOOLEAN isTouchConfigurationActivated,
    _In_ BOOLEAN isBringIntoViewportConfigurationActivated)
{
    RRETURN(S_OK);
}

// Called to raise the DirectManipulationStarted/Completed events
_Check_return_ HRESULT UIElement::NotifyManipulationStateChanged(
    _In_ CUIElement* nativeTarget,
    _In_ XUINT32 state)
{
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR_RETURN(nativeTarget);

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));

    IFC_RETURN(spTarget.Cast<UIElement>()->NotifyManipulationStateChanged(
        static_cast<DMManipulationState>(state)));

    return S_OK;
}

_Check_return_ HRESULT UIElement::NotifyManipulationStateChanged(
    _In_ DMManipulationState state)
{
    return S_OK;
}

// Used to determine whether transform data is stale and a BringIntoViewportNeeded
// is pending.
_Check_return_ HRESULT UIElement::IsBringIntoViewportNeeded(
    _In_ CUIElement* nativeTarget,
    _Out_ bool * bringIntoViewportNeeded)
{
    *bringIntoViewportNeeded = false;

    // Currently only ScrollViewers support this function
    if (nativeTarget->OfTypeByIndex<KnownTypeIndex::ScrollViewer>())
    {
        ctl::ComPtr<DependencyObject> spTarget;

        DXamlCore* pCore = DXamlCore::GetCurrent();
        IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));

        IFC_RETURN(spTarget.Cast<ScrollViewer>()->IsBringIntoViewportNeeded(bringIntoViewportNeeded));
    }
    return S_OK;
}

// Used to notify an IDirectManipulationContainer implementation that it needs to
// call IDirectManipulationContainerHandler::BringIntoViewport either to synchronize
// the DManip primary content transform with XAML when transformIsValid==False,
// or to jump to the provided transform when transformIsValid==True.
_Check_return_ HRESULT UIElement::NotifyBringIntoViewportNeeded(
    _In_ CUIElement* nativeTarget,
    _In_ CUIElement* nativeManipulatedElement,
    _In_ XFLOAT translationX,
    _In_ XFLOAT translationY,
    _In_ XFLOAT zoomFactor,
    _In_ bool fTransformIsValid,
    _In_ bool fTransformIsInertiaEnd)
{
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<DependencyObject> spManipulatedElement;

    IFCPTR_RETURN(nativeTarget);
    IFCPTR_RETURN(nativeManipulatedElement);

    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(nativeTarget, &spTarget));

    IFC_RETURN(pCore->GetPeer(nativeManipulatedElement, &spManipulatedElement));

    IFC_RETURN(spTarget.Cast<UIElement>()->NotifyBringIntoViewportNeeded(
        spManipulatedElement.Cast<UIElement>(),
        static_cast<FLOAT>(translationX),
        static_cast<FLOAT>(translationY),
        static_cast<FLOAT>(zoomFactor),
        static_cast<BOOLEAN>(fTransformIsValid),
        static_cast<BOOLEAN>(fTransformIsInertiaEnd)));

    return S_OK;
}

_Check_return_ HRESULT UIElement::NotifyBringIntoViewportNeeded(
    _In_ UIElement* pManipulatedElement,
    _In_ FLOAT translationX,
    _In_ FLOAT translationY,
    _In_ FLOAT zoomFactor,
    _In_ BOOLEAN transformIsValid,
    _In_ BOOLEAN transformIsInertiaEnd)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT UIElement::GetChildrenCount(
    _Out_ INT* pnCount)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VisualTreeHelper> spVisualTree;
    INT childCount = 0;

    IFCPTR(pnCount);
    IFC(ctl::make<VisualTreeHelper>(&spVisualTree));
    IFC(spVisualTree->GetChildrenCount(this, &childCount));
    *pnCount = childCount;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::GetChild(
    _In_ INT nChildIndex,
    _Outptr_ xaml::IDependencyObject** ppDO)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VisualTreeHelper> spVisualTree;
    ctl::ComPtr<IDependencyObject> spChildAsDO;

    IFCPTR(ppDO);
    IFC(ctl::make<VisualTreeHelper>(&spVisualTree));
    IFC(spVisualTree->GetChild(this, nChildIndex, &spChildAsDO));
    IFC(spChildAsDO.CopyTo(ppDO));

Cleanup:
    RRETURN(hr);
}

// Called when snap points were updated.
_Check_return_ HRESULT UIElement::NotifySnapPointsChanged(
    _In_ CUIElement* nativeTarget,
    _In_ bool fHorizontalSnapPoints)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<xaml_controls::IStackPanel> spStackPanel;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    spStackPanel = spTarget.AsOrNull<xaml_controls::IStackPanel>();
    if (spStackPanel)
    {
        IFC(spStackPanel.Cast<StackPanel>()->NotifySnapPointsChanged(static_cast<BOOLEAN>(fHorizontalSnapPoints)));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::GetDManipElementAndProperty(
    _In_ CUIElement* nativeTarget,
    _In_ KnownPropertyIndex targetProperty,
    _Outptr_ CDependencyObject** ppDManipElement,
    _Out_ XUINT32 *pDManipProperty)
{
    HRESULT hr = S_OK;
    CDependencyObject *pDManipElement = NULL;
    XUINT32 dManipProperty = static_cast<XUINT32>(XcpDMPropertyNone);
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR(nativeTarget);
    IFCPTR(ppDManipElement);
    IFCPTR(pDManipProperty);

    *ppDManipElement = NULL;
    *pDManipProperty = static_cast<XUINT32>(XcpDMPropertyNone);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    IFC(spTarget.Cast<UIElement>()->GetDManipElementAndProperty(
        targetProperty,
        &pDManipElement,
        &dManipProperty));

    *ppDManipElement = pDManipElement;
    *pDManipProperty = dManipProperty;

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::GetDManipElementAndProperty(
    _In_ KnownPropertyIndex targetProperty,
    _Outptr_ CDependencyObject** ppDManipElement,
    _Out_ XUINT32 *pDManipProperty)
{
    *ppDManipElement = NULL;
    *pDManipProperty = static_cast<XUINT32>(XcpDMPropertyNone);

    RRETURN(S_OK); // RRETURN_REMOVAL
}

_Check_return_ HRESULT UIElement::GetDManipElement(
    _In_ CUIElement* nativeTarget,
    _Outptr_ CDependencyObject** ppDManipElement)
{
    HRESULT hr = S_OK;
    CDependencyObject *pDManipElement = NULL;
    ctl::ComPtr<DependencyObject> spTarget;

    *ppDManipElement = NULL;

    IFCPTR(nativeTarget);
    IFCPTR(ppDManipElement);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    IFC(spTarget.Cast<UIElement>()->GetDManipElement(
        &pDManipElement));

    *ppDManipElement = pDManipElement;

Cleanup:
    RRETURN(hr);
}

// Elements deriving from DependencyObject that are DManip aware
// will override this to allow core to know what element is its
// element manipulated by DManip.
_Check_return_ HRESULT UIElement::GetDManipElement(
    _Outptr_ CDependencyObject** ppDManipElement)
{
    *ppDManipElement = NULL;
    RRETURN(S_OK);
}


bool UIElement::ShouldPlayImplicitShowHideAnimation(_In_ CUIElement* nativeTarget)
{
    ctl::ComPtr<DependencyObject> target;

    IFCFAILFAST(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &target));
    return target.Cast<UIElement>()->ShouldPlayImplicitShowHideAnimation();
}

// Helper function used by implicit animations to detect special conditions under which animations should trigger
bool UIElement::ShouldPlayImplicitShowHideAnimation()
{
    bool result = false;
    ThemeTransitionContext context = ThemeTransitionContext::None;
    BOOLEAN didCheck = FALSE;

    // The policy being applied here is tactical (as of RS2) as at this time we do not have enough infrastructure
    // in place to trigger contextual implicit show/hide animations for individual item adds/removes.
    // For now the policy is to detect if the ItemsSource is doing its initial load, or if the ItemsSource has
    // changed.  If either of these are detected, we play the animation.
    IFCFAILFAST(DirectUI::GetTransitionContext(this, &didCheck, &context));

    if (didCheck)
    {
        switch (context)
        {
        case ThemeTransitionContext::Entrance:
        case ThemeTransitionContext::ContentTransition:
            result = true;
            break;
        default:
            break;
        }
    }

    return result;
}

// called for creating automation peer on the target element
_Check_return_ HRESULT UIElement::OnCreateAutomationPeer(
    _In_ CDependencyObject* nativeTarget,
    _Out_ CAutomationPeer** returnAP)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;

    IFCPTR(nativeTarget);
    IFCPTR(returnAP);

    *returnAP = NULL;

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.Cast<UIElement>()->GetOrCreateAutomationPeer(&spAP));
    if (hr == S_OK)
    {
        *returnAP = static_cast<CAutomationPeer*>(spAP.Cast<AutomationPeer>()->GetHandle());
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElement::GetOrCreateAutomationPeer(xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    xaml::Visibility value = xaml::Visibility_Visible;
    BOOLEAN bIsPopupOpen = TRUE;

    IFCPTR(ppAutomationPeer);

    if (ctl::is<xaml_primitives::IPopup>(this))
    {
        IFC((static_cast<Popup*>(this))->get_IsOpen(&bIsPopupOpen));
    }
    IFC(this->get_Visibility(&value));

    // this condition checks that if Control is visible and if it's popup then it must be open
    if (value != xaml::Visibility_Collapsed && bIsPopupOpen)
    {
        if (!m_tpAP)
        {
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAP;
            if (FAILED(UIElementGenerated::OnCreateAutomationPeerProtected(&spAP)))
            {
                RRETURN(E_FAIL);
            }
            else if (!spAP)
            {
                RRETURN(S_FALSE);
            }

            // This FX peer gains state when the AutomationPeer is stored in m_tpAP, so mark as
            // having state. Otherwise, a stateless FX peer will be released, which will
            // release the automation peer.
            IFC(MarkHasState());

            SetPtrValue(m_tpAP, spAP.Get());
        }
    }
    else
    {
        if (m_tpAP)
        {
            m_tpAP.Clear();
        }
        RRETURN(S_FALSE);
    }

    IFC(m_tpAP.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

/* static */
_Check_return_ HRESULT UIElement::RaiseKeyboardAcceleratorInvokedStatic(
    _In_ CDependencyObject* pElement,
    _In_ KeyboardAcceleratorInvokedEventArgs *pKAIEventArgs,
    _Out_ BOOLEAN *pIsHandled)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pElement, &peer));
    if (!peer)
    {
        return S_OK;
    }

    ctl::ComPtr<UIElement> element;
    IFC_RETURN(peer.As(&element));
    IFCPTR_RETURN(element);

    IFC_RETURN(element->OnKeyboardAcceleratorInvokedProtected(pKAIEventArgs));
    IFCFAILFAST(pKAIEventArgs->get_Handled(pIsHandled));

    return S_OK;
}

/* static */
_Check_return_ HRESULT UIElement::RaiseProcessKeyboardAcceleratorsStatic(
    _In_ CUIElement* pUIElement,
    _In_ wsy::VirtualKey key,
    _In_ wsy::VirtualKeyModifiers keyModifiers,
    _Out_ BOOLEAN *pHandled,
    _Out_ BOOLEAN *pHandledShouldNotImpedeTextInput)
{
    ctl::ComPtr<DependencyObject> peer;
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pUIElement, &peer));
    if (!peer)
    {
        return S_OK;
    }
    ctl::ComPtr<UIElement> element;
    IFC_RETURN(peer.As(&element));
    IFCPTR_RETURN(element);

    ctl::ComPtr<ProcessKeyboardAcceleratorEventArgs> spProcessKeyboardAcceleratorEventArgs;
    IFC_RETURN(ctl::make(&spProcessKeyboardAcceleratorEventArgs));
    IFCFAILFAST(spProcessKeyboardAcceleratorEventArgs->put_Key(key));
    IFCFAILFAST(spProcessKeyboardAcceleratorEventArgs->put_Modifiers(keyModifiers));

    IFC_RETURN(element->OnProcessKeyboardAcceleratorsProtected(spProcessKeyboardAcceleratorEventArgs.Get()));

    IFCFAILFAST(spProcessKeyboardAcceleratorEventArgs->get_Handled(pHandled));
    IFCFAILFAST(spProcessKeyboardAcceleratorEventArgs->get_HandledShouldNotImpedeTextInput(pHandledShouldNotImpedeTextInput));

    if (*pHandled == FALSE)
    {
        ProcessKeyboardAcceleratorsEventSourceType* spkaEventSource = nullptr;
        IFC_RETURN(element->GetProcessKeyboardAcceleratorsEventSourceNoRef(&spkaEventSource));
        IFC_RETURN(spkaEventSource->Raise(element.Get(), spProcessKeyboardAcceleratorEventArgs.Get()));

        IFCFAILFAST(spProcessKeyboardAcceleratorEventArgs->get_Handled(pHandled));
        IFCFAILFAST(spProcessKeyboardAcceleratorEventArgs->get_HandledShouldNotImpedeTextInput(pHandledShouldNotImpedeTextInput));
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    ctl::ComPtr<IFlyoutBase> spFlyout;
    IFC_RETURN(get_ContextFlyout(spFlyout.ReleaseAndGetAddressOf()));
    if (spFlyout)
    {
        IFC_RETURN(spFlyout->TryInvokeKeyboardAccelerator(pArgs));

        BOOLEAN bHandled = FALSE;
        IFCFAILFAST(pArgs->get_Handled(&bHandled));
        if (bHandled)
        {
            return S_OK;
        }
    }

    // If event is not yet handled and current element is Button then TryInvoke on Flyout if it exists.
    if (GetHandle()->OfTypeByIndex<KnownTypeIndex::Button>())
    {
        static_cast<Button*>(this)->OnProcessKeyboardAcceleratorsImplLocal(pArgs);
    }
    return S_OK;
}

// Search for an accelerator that can be invoked
_Check_return_ HRESULT UIElement::TryInvokeKeyboardAcceleratorImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs)
{
    BOOLEAN handled = FALSE;
    BOOLEAN handledShouldNotImpedeTextInput = FALSE;
    wsy::VirtualKey key;
    wsy::VirtualKeyModifiers keyModifiers;

    IFCFAILFAST(pArgs->get_Key(&key));
    IFCFAILFAST(pArgs->get_Modifiers(&keyModifiers));
    if (KeyboardAcceleratorUtility::IsKeyValidForAccelerators(key, KeyboardAcceleratorUtility::MapVirtualKeyModifiersToIntegersModifiers(keyModifiers)))
    {
        // Get the focused element
        const CFocusManager* const focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
        const CDependencyObject* const pFocusedElement = focusManager? focusManager->GetFocusedElementNoRef(): nullptr;

        IFC_RETURN(CUIElement::TryInvokeKeyboardAccelerator(pFocusedElement, GetHandle(), key, keyModifiers, handled, handledShouldNotImpedeTextInput));
        IFC_RETURN(pArgs->put_Handled(handled));
        IFCFAILFAST(static_cast<ProcessKeyboardAcceleratorEventArgs*>(pArgs)->put_HandledShouldNotImpedeTextInput(handledShouldNotImpedeTextInput));
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::OnCreateAutomationPeerImpl(xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    RRETURN(S_FALSE);
}

/* static */
_Check_return_ HRESULT UIElement::OnBringIntoViewRequestedFromCore(_In_ CUIElement* pUIElement, _In_ CRoutedEventArgs* args)
{
    ctl::ComPtr<DependencyObject> peer;
    ctl::ComPtr<IInspectable> argsPeer;

    IFC_RETURN(args->CreateFrameworkPeer(&argsPeer));
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeer(pUIElement, &peer));

    if (peer)
    {
        ctl::ComPtr<UIElement> element;
        IFC_RETURN(peer.As(&element));
        IFCPTR_RETURN(element);

        ctl::ComPtr<xaml::IBringIntoViewRequestedEventArgs> bivrArgs;
        IFC_RETURN(argsPeer.As(&bivrArgs));
        IFCPTR_RETURN(bivrArgs);

        IFC_RETURN(element->OnBringIntoViewRequestedProtected(bivrArgs.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::OnBringIntoViewRequestedImpl(_In_ xaml::IBringIntoViewRequestedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);
    return S_OK;
}

_Check_return_ HRESULT UIElement::StartBringIntoViewImpl()
{
    wf::Rect rect = {};
    wf::Size size = {};

    IFC_RETURN(get_RenderSize(&size));

    rect.X = 0;
    rect.Y = 0;
    rect.Width = size.Width;
    rect.Height = size.Height;

    BringIntoView(rect, FALSE /*forceIntoView*/, TRUE /*useAnimation*/, FALSE /*skipDuringManipulation*/);

    return S_OK;
}

_Check_return_ HRESULT UIElement::StartBringIntoViewWithOptionsImpl(_In_ xaml::IBringIntoViewOptions* options)
{
    BOOLEAN animDesired = TRUE;
    double horizontalAlignmentRatio = DoubleUtil::NaN;
    double verticalAlignmentRatio = DoubleUtil::NaN;
    ctl::ComPtr<wf::IReference<wf::Rect>> targetRect;
    wf::Rect rect = {};
    DOUBLE offsetX = 0.0;
    DOUBLE offsetY = 0.0;

    IFCPTR_RETURN(options);

    IFC_RETURN(options->get_TargetRect(&targetRect));
    if (targetRect.Get() != nullptr)
    {
        IFC_RETURN(targetRect->get_Value(&rect));
    }
    else
    {
        wf::Size size = {};

        IFC_RETURN(get_RenderSize(&size));

        rect.X = 0;
        rect.Y = 0;
        rect.Width = size.Width;
        rect.Height = size.Height;
    }

    IFC_RETURN(options->get_AnimationDesired(&animDesired));

    IFC_RETURN(options->get_HorizontalAlignmentRatio(&horizontalAlignmentRatio));
    ASSERT(DoubleUtil::IsNaN(horizontalAlignmentRatio) || (horizontalAlignmentRatio >= 0.0 && horizontalAlignmentRatio <= 1.0));

    IFC_RETURN(options->get_VerticalAlignmentRatio(&verticalAlignmentRatio));
    ASSERT(DoubleUtil::IsNaN(verticalAlignmentRatio) || (verticalAlignmentRatio >= 0.0 && verticalAlignmentRatio <= 1.0));

    IFC_RETURN(options->get_HorizontalOffset(&offsetX));
    IFC_RETURN(options->get_VerticalOffset(&offsetY));

    BringIntoView(
        rect,
        FALSE /*forceIntoView*/,
        animDesired /*useAnimation*/,
        FALSE /*skipDuringManipulation*/,
        horizontalAlignmentRatio,
        verticalAlignmentRatio,
        offsetX,
        offsetY);

    return S_OK;
}

_Check_return_ HRESULT UIElement::FindSubElementsForTouchTargetingImpl(
    _In_ wf::Point point,
    _In_ wf::Rect boundingRect,
    _Outptr_ wfc::IIterable<wfc::IIterable<wf::Point>*>** pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    // No special touch targeting behavior by default.
    *pReturnValue = NULL;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::GetChildrenInTabFocusOrderImpl(
    _Outptr_ wfc::IIterable<xaml::DependencyObject*>** returnValue)
{
    *returnValue = nullptr;

    auto children = FocusProperties::GetFocusChildren<CDOCollection>(DependencyObject::GetHandle());
    if (children && !children->IsLeaving() && children->GetCount() > 0)
    {
        IFC_RETURN(FocusProperties::DefaultFocusChildrenIterable::CreateInstance(this, returnValue));
    }
    return S_OK;
}

// Cancels all ongoing DManip-based manipulations involving this element and its ancestors.
_Check_return_ HRESULT UIElement::CancelDirectManipulationsImpl(
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    bool fHandled = false;

    IFCPTR(pReturnValue);
    *pReturnValue = FALSE;

    CInputServices* inputServices = GetHandle()->GetContext()->GetInputServices();
    IFC(inputServices->CancelDirectManipulations(static_cast<CUIElement*>(GetHandle()), &fHandled));

    *pReturnValue = static_cast<BOOLEAN>(fHandled);

Cleanup:
    RRETURN(hr);
}

// Called when InputPane is showing or hiding.
_Check_return_ HRESULT UIElement::NotifyInputPaneStateChange(
    _In_ CUIElement* nativeTarget,
    _In_ InputPaneState inputPaneState,
    _In_ XRECTF inputPaneBounds)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));
    IFC(spTarget.Cast<RootScrollViewer>()->NotifyInputPaneStateChange(inputPaneState, inputPaneBounds));

Cleanup:
    RRETURN(hr);
}

// Called when InputPane is ready to transit.
_Check_return_ HRESULT UIElement::ApplyInputPaneTransition(
    _In_ CUIElement* nativeTarget,
    _In_ bool fEnableThemeTransition)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;

    IFCPTR(nativeTarget);

    IFC(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &spTarget));

    IFC(spTarget.Cast<RootScrollViewer>()->ApplyInputPaneTransition(!!fEnableThemeTransition));

Cleanup:
    RRETURN(hr);
}

/*static*/
_Check_return_ HRESULT UIElement::ApplyElevationEffectProxy(
    _In_ CUIElement* target,
    unsigned int depth)
{
    HRESULT hr = S_OK;
    auto ConvertToUIElementRuntime = [&hr](CUIElement* pNative, _Outptr_ xaml::IUIElement** ppResult) -> HRESULT
    {
        ctl::ComPtr<DependencyObject> spTarget;
        ctl::ComPtr<IUIElement> spRuntimeTarget;
        IFCPTR(pNative);
        IFC(DXamlCore::GetCurrent()->GetPeer(pNative, &spTarget));
        spRuntimeTarget = spTarget.AsOrNull<xaml::IUIElement>();
        IFCPTR(spRuntimeTarget);
        IFC(spRuntimeTarget.CopyTo(ppResult));

    Cleanup:
        RRETURN(hr);
    };

    ctl::ComPtr<xaml::IUIElement> spRuntimeTarget;
    IFC(ConvertToUIElementRuntime(target, &spRuntimeTarget));
    IFC(ApplyElevationEffect(spRuntimeTarget.Get(), depth));

Cleanup:
    RRETURN(hr);
}

// Returns the horizontal and vertical ratios between the ScrollViewer effective viewport
// and its actual size. That viewport is potentially reduced by the presence of headers.
_Check_return_ HRESULT UIElement::GetScrollContentPresenterViewportRatios(
    _In_ CUIElement* nativeTarget,
    _In_ CDependencyObject* nativeChild,
    _Out_ XSIZEF* ratios)
{
    ASSERT(nativeTarget);
    ASSERT(nativeChild);
    ASSERT(ratios);

    *ratios = { 1.0f, 1.0f };

    ctl::ComPtr<DependencyObject> target;
    ctl::ComPtr<DependencyObject> child;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &target));
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeChild, &child));

    const auto scp = target.AsOrNull<xaml_controls::IScrollContentPresenter>();

    if (scp)
    {
        ctl::ComPtr<IInspectable> scrollOwner;
        IFC_RETURN(scp->get_ScrollOwner(&scrollOwner));

        if (const auto scrollViewer = scrollOwner.AsOrNull<xaml_controls::IScrollViewer>())
        {
            IFC_RETURN(scrollViewer.Cast<ScrollViewer>()->GetViewportRatios(child.Cast<DependencyObject>(), ratios));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::IsScrollViewerContentScrollable(
    _In_ CUIElement* nativeTarget,
    _Out_ bool* isContentHorizontallyScrollable,
    _Out_ bool* isContentVerticallyScrollable)
{
    ASSERT(nativeTarget);

    *isContentHorizontallyScrollable = false;
    *isContentVerticallyScrollable = false;

    ctl::ComPtr<DependencyObject> target;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(nativeTarget, &target));

    const auto scp = target.AsOrNull<xaml_controls::IScrollContentPresenter>();

    if (scp)
    {
        ctl::ComPtr<IInspectable> scrollOwner;
        IFC_RETURN(scp->get_ScrollOwner(&scrollOwner));

        if (const auto scrollViewer = scrollOwner.AsOrNull<xaml_controls::IScrollViewer>())
        {
            IFC_RETURN(scrollViewer.Cast<ScrollViewer>()->IsContentScrollable(
                true, /* ignoreScrollMode */
                false, /* ignoreScrollBarVisibility */
                isContentHorizontallyScrollable,
                isContentVerticallyScrollable));
        }
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  DisconnectChildrenRecursive
//
//  This is main implementation of VisualTreeHelper::DisconnectChildrenRecursive.  This
//  calls the OnDisconnectVisualChildren virtual on itself, calls Disconnect on each of its
//  children, and then clears its visual children collection.
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT UIElement::DisconnectChildrenRecursive()
{
    HRESULT hr = S_OK;
    INT childrenCount = 0;
    INT childIndex = 0;

    // Allow subclass customization
    IFC(OnDisconnectVisualChildrenProtected());

    // Walk the visual children and call Disconnect
    IFC(VisualTreeHelper::GetChildrenCountStatic(this, &childrenCount));
    while (childIndex < childrenCount)
    {
        ctl::ComPtr<IDependencyObject> spChild;

        IFC(VisualTreeHelper::GetChildStatic(this, childIndex, &spChild));
        IFC(spChild.Cast<UIElement>()->DisconnectChildrenRecursive());
        childIndex++;
    }

    // Release the visual children
    IFC(CoreImports::ClearUIElementChildren(static_cast<CUIElement*>(GetHandle())));

    // Allow subclass prolog customization (this is internal only, not a public API)
    IFC(OnDisconnectVisualChildrenProtected());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::AddHandlerImpl(
    _In_ xaml::IRoutedEvent* pRoutedEvent,
    _In_ IInspectable* pEventHandler,
    _In_ BOOLEAN handledEventsToo)
{
    KnownEventIndex routedEventId = static_cast<RoutedEvent*>(pRoutedEvent)->GetEventId();
    IFC_RETURN(UIElement::EventAddHandlerByIndex(routedEventId, pEventHandler, handledEventsToo));

    return S_OK;
}

_Check_return_ HRESULT UIElement::RemoveHandlerImpl(
    _In_ xaml::IRoutedEvent* pRoutedEvent,
    _In_ IInspectable* pEventHandler)
{
    KnownEventIndex routedEventId = static_cast<RoutedEvent*>(pRoutedEvent)->GetEventId();
    IFC_RETURN(UIElement::EventRemoveHandlerByIndex(routedEventId, pEventHandler));

    return S_OK;
}

/*
CErrorsChangedEventSource* UIElement::GetErrorsChangedEventSourceNoRef()
{
    CErrorsChangedEventSource* eventSourceNoRef = nullptr;
    IFCFAILFAST(GetEventSourceNoRefWithArgumentValidation(KnownEventIndex::InputValidation_Error, reinterpret_cast<IUntypedEventSource**>(&eventSourceNoRef)));

    if (eventSourceNoRef == nullptr)
    {
        IFCFAILFAST(ctl::ComObject<CErrorsChangedEventSource>::CreateInstance(&eventSourceNoRef, TRUE));
        const bool useEventManager = true;
        eventSourceNoRef->Initialize(KnownEventIndex::InputValidation_Error, this, useEventManager);
        IFCFAILFAST(StoreEventSource(KnownEventIndex::InputValidation_Error, eventSourceNoRef));
        ctl::iunknown_cast(eventSourceNoRef)->Release();
    }

    return eventSourceNoRef;
}
*/

// Check whether this UIElement was generated by one of our internal GetContainerForItemOverride implementations.
BOOLEAN
UIElement::GetIsGeneratedContainer()
{
    return m_isGeneratedContainer;
}

// Set a flag showing whether this UIElement was generated by one of our internal GetContainerForItemOverride implementations.
void
UIElement::SetIsGeneratedContainer(bool value)
{
    VERIFYHR(MarkHasState());
    m_isGeneratedContainer = value;
}

_Check_return_ HRESULT
UIElementFactory::get_KeyDownEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetKeyDownEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PreviewKeyDownEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = nullptr;

    IFC_RETURN(StaticStore::GetPreviewKeyDownEvent(ppValue));
    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::get_KeyUpEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = nullptr;

    IFC(StaticStore::GetKeyUpEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PreviewKeyUpEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = nullptr;
    IFC_RETURN(StaticStore::GetPreviewKeyUpEvent(ppValue));
    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::get_CharacterReceivedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = nullptr;
    IFC_RETURN(StaticStore::GetCharacterReceivedEvent(ppValue));
    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::get_GettingFocusEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = NULL;

    IFC_RETURN(StaticStore::GetGettingFocusEvent(ppValue));

    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::get_LosingFocusEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = NULL;

    IFC_RETURN(StaticStore::GetLosingFocusEvent(ppValue));

    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::get_NoFocusCandidateFoundEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = NULL;

    IFC_RETURN(StaticStore::GetNoFocusCandidateFoundEvent(ppValue));

    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::get_ContextRequestedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = NULL;

    IFC_RETURN(StaticStore::GetContextRequestedEvent(ppValue));

    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::InternalGetIsEnabledImpl(
    _In_ xaml::IUIElement* pElement,
    _Out_ BOOLEAN* pResult)
{
    UIElement* pUIElement = static_cast<UIElement*>(pElement);

    CUIElement* pCUIElement = static_cast<CUIElement*>(pUIElement->GetHandle());
    *pResult = static_cast<BOOLEAN>(pCUIElement->IsEnabled());

    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::InternalPutIsEnabledImpl(
    _In_ xaml::IUIElement* pElement,
    _In_ BOOLEAN value)
{
    UIElement* pUIElement = static_cast<UIElement*>(pElement);

    CUIElement* pCUIElement = static_cast<CUIElement*>(pUIElement->GetHandle());
    IFC_RETURN(pCUIElement->CoerceIsEnabled(value, TRUE/*bCoerceChildren*/));

    return S_OK;
}


_Check_return_ HRESULT
UIElementFactory::get_PointerEnteredEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerEnteredEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PointerPressedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerPressedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PointerMovedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerMovedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PointerReleasedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerReleasedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PointerExitedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerExitedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PointerCaptureLostEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerCaptureLostEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PointerCanceledEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerCanceledEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_PointerWheelChangedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetPointerWheelChangedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_TappedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetTappedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_DoubleTappedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetDoubleTappedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_HoldingEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetHoldingEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_RightTappedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetRightTappedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_RightTappedUnhandledEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetRightTappedUnhandledEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_ManipulationStartingEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetManipulationStartingEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_ManipulationInertiaStartingEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetManipulationInertiaStartingEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_ManipulationStartedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetManipulationStartedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_ManipulationDeltaEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetManipulationDeltaEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_ManipulationCompletedEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetManipulationCompletedEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_DragEnterEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetDragEnterEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_DragLeaveEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetDragLeaveEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_DragOverEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetDragOverEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_DropEventImpl(
    _Outptr_ xaml::IRoutedEvent** ppValue)
{
    HRESULT hr = S_OK;

    IFCPTR(ppValue);
    *ppValue = NULL;

    IFC(StaticStore::GetDropEvent(ppValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElementFactory::get_BringIntoViewRequestedEventImpl(
    _Outptr_ xaml::IRoutedEvent** value)
{
    IFCPTR_RETURN(value);
    *value = NULL;

    IFC_RETURN(StaticStore::GetBringIntoViewRequestedEvent(value));

    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::RegisterAsScrollPortImpl(_In_ xaml::IUIElement* element)
{
    static_cast<CUIElement*>(static_cast<UIElement*>(element)->GetHandle())->RegisterAsScroller();
    return S_OK;
}

_Check_return_ HRESULT
UIElementFactory::TryStartDirectManipulationImpl(
    _In_ xaml_input::IPointer* pValue,
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    bool handled = false;

    *pReturnValue = FALSE;

    Pointer* pPointer = static_cast<Pointer*>(pValue);

    CInputServices* inputServices = DXamlCore::GetCurrent()->GetHandle()->GetInputServices();
    IFC(inputServices->TryStartDirectManipulation(static_cast<CPointer*>(pPointer->GetHandle()), &handled));

    *pReturnValue = static_cast<BOOLEAN>(handled);

Cleanup:
    return hr;
}

// Allocates the virtualization information
_Check_return_ HRESULT UIElement::InitVirtualizationInformation()
{
    HRESULT hr = S_OK;

    if (!m_pVirtualizationInformation)
    {
        m_pVirtualizationInformation.reset(new VirtualizationInformation(this));

        IFC(MarkHasState());
    }

Cleanup:
    RRETURN(hr);
}

// Returns virtualization information, or NULL if it hasn't been created.
_Check_return_ UIElement::VirtualizationInformation* UIElement::GetVirtualizationInformation()
{
    // Don't need to assert here or anything. It's possible that an app has manually added
    // UIElements to a panel's children. If so, we should be able to safely ignore them
    // without blowing up on an ASSERT.

    // If a caller truly expects to see valid VirtualizationInformation here, they should
    // be the ones calling ASSERT at the call site.

    return m_pVirtualizationInformation.get();
}

UIElement::VirtualizationInformation::VirtualizationInformation(_In_ xaml::IUIElement* pOwner)
    : m_pOwner(static_cast<UIElement*>(pOwner))
{
}

UIElement::VirtualizationInformation::~VirtualizationInformation()
{

}

void UIElement::VirtualizationInformation::SetBounds(_In_ const wf::Rect& bounds)
{
    m_bounds = bounds;
}

_Check_return_ HRESULT UIElement::VirtualizationInformation::SetItem(_In_opt_ IInspectable* pDataItem)
{
    HRESULT hr = S_OK;
    bool areEqual = false;

    IFC(ctl::are_equal(ctl::as_iinspectable(m_pOwner), pDataItem, &areEqual));
    if (areEqual)
    {
        m_isItemLinkedAsOwnContainer = true;
        m_tpDataItem.Clear();
    }
    else
    {
        m_isItemLinkedAsOwnContainer = false;
        m_pOwner->SetPtrValue(m_tpDataItem, pDataItem);
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::VirtualizationInformation::GetBuildTreeArgs(_Out_ ctl::ComPtr<xaml_controls::IContainerContentChangingEventArgs>* pspArgs)
{
    HRESULT hr = S_OK;


    if (!m_tpBuildTreeArgs)
    {
        ctl::ComPtr<ContainerContentChangingEventArgs> spArgs;
        IFC(ctl::make(&spArgs));
        m_pOwner->SetPtrValue(m_tpBuildTreeArgs, spArgs.Get());
        spArgs->RegisterBackpointerToContainer(m_pOwner);

        *pspArgs = std::move(spArgs);
    }
    else
    {
        *pspArgs = m_tpBuildTreeArgs.Get();
    }


Cleanup:
    RRETURN(hr);
}

ctl::ComPtr<IInspectable> UIElement::VirtualizationInformation::GetItem() const
{
    if (m_isItemLinkedAsOwnContainer)
    {
        return ctl::ComPtr<IInspectable>(ctl::as_iinspectable(m_pOwner));
    }
    else
    {
        return ctl::ComPtr<IInspectable>(m_tpDataItem.Get());
    }
}

ctl::ComPtr<xaml::IDataTemplate> UIElement::VirtualizationInformation::GetSelectedTemplate() const
{
    return ctl::ComPtr<xaml::IDataTemplate>(m_tpSelectedTemplate.Get());
}

void UIElement::VirtualizationInformation::SetSelectedTemplate(_In_ xaml::IDataTemplate* const pDataTemplate)
{
    m_pOwner->SetPtrValue(m_tpSelectedTemplate, pDataTemplate);
}

// Called when FocusManager process TabStop to interact with the focused control.
_Check_return_ HRESULT
UIElement::ProcessTabStop(
    _In_ CContentRoot* contentRoot,
    _In_opt_ CDependencyObject* pFocusedElement,
    _In_opt_ CDependencyObject* pCandidateTabStopElement,
    const bool isBackward,
    const bool didCycleFocusAtRootVisualScope,
    _Outptr_ CDependencyObject** ppNewTabStop,
    _Out_ bool* pIsTabStopOverridden)
{
    HRESULT hr = S_OK;
    BOOLEAN isTabStopOverridden = FALSE;
    BOOLEAN isCandidateTabStopOverridden = FALSE;
    ctl::ComPtr<DependencyObject> spFocusedTarget;
    ctl::ComPtr<DependencyObject> spCandidateTarget;
    ctl::ComPtr<IDependencyObject> spFocusedTargetParent;
    ctl::ComPtr<IDependencyObject> spCandidateTargetParent;
    ctl::ComPtr<IUIElement> spFocusedTargetAsUIE;
    ctl::ComPtr<IUIElement> spCandidateTargetAsUIE;
    ctl::ComPtr<IUIElement> spNewCandidateTargetAsUIE;
    ctl::ComPtr<DependencyObject> spNewTabStop;
    ctl::ComPtr<DependencyObject> spNewCandidateTabStop;
    ctl::ComPtr<IApplicationBarService> applicationBarService;

    IFCPTR(ppNewTabStop);
    IFCPTR(pIsTabStopOverridden);

    *ppNewTabStop = NULL;
    *pIsTabStopOverridden = FALSE;

    if (pFocusedElement)
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(pFocusedElement, &spFocusedTarget));
        spFocusedTargetAsUIE = spFocusedTarget.AsOrNull<IUIElement>();
        // Get the parent if it is not a UIElement. (E.g. Hyperlink)
        if (!spFocusedTargetAsUIE.Get())
        {
            IFC(VisualTreeHelper::GetParentStatic(spFocusedTarget.Get(), &spFocusedTargetParent));
            spFocusedTargetAsUIE = spFocusedTargetParent.AsOrNull<IUIElement>();
        }
    }

    if (pCandidateTabStopElement)
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(pCandidateTabStopElement, &spCandidateTarget));
        spCandidateTargetAsUIE = spCandidateTarget.AsOrNull<IUIElement>();
        // Get the parent if it is not a UIElement. (E.g. Hyperlink)
        if (!spCandidateTargetAsUIE.Get())
        {
            IFC(VisualTreeHelper::GetParentStatic(spCandidateTarget.Get(), &spCandidateTargetParent));
            spCandidateTargetAsUIE = spCandidateTargetParent.AsOrNull<IUIElement>();
        }
    }

    ASSERT(!spNewTabStop.Get());

    if (spFocusedTargetAsUIE)
    {
        IFC(spFocusedTargetAsUIE.Cast<UIElement>()->ProcessTabStopInternal(spCandidateTarget.Get(), isBackward, didCycleFocusAtRootVisualScope, &spNewTabStop, &isTabStopOverridden));
    }

    if (!isTabStopOverridden && spCandidateTargetAsUIE)
    {
        ASSERT(!spNewTabStop.Get());
        IFC(spCandidateTargetAsUIE.Cast<UIElement>()->ProcessCandidateTabStopInternal(spFocusedTarget.Get(), nullptr, isBackward, &spNewTabStop, &isTabStopOverridden));
    }
    else if (isTabStopOverridden && spNewTabStop.Get())
    {
        spNewCandidateTargetAsUIE = spNewTabStop.AsOrNull<IUIElement>();
        if (spNewCandidateTargetAsUIE)
        {
            IFC(spNewCandidateTargetAsUIE.Cast<UIElement>()->ProcessCandidateTabStopInternal(spFocusedTarget.Get(), spNewTabStop.Get(), isBackward, &spNewCandidateTabStop, &isCandidateTabStopOverridden));
        }
    }

    if (!isTabStopOverridden && !isCandidateTabStopOverridden && contentRoot)
    {
        ctl::ComPtr<xaml::IXamlRoot> xamlRoot;
        ctl::ComPtr<IInspectable> xamlRootInspectable{contentRoot->GetOrCreateXamlRootNoRef()};
        IFC(xamlRootInspectable.As(&xamlRoot));
        if (xamlRoot)
        {
            IFC(xamlRoot.Cast<XamlRoot>()->TryGetApplicationBarService(applicationBarService));
        }
        
        if (applicationBarService)
        {
            IFC(applicationBarService->ProcessTabStopOverride(spFocusedTarget.Get(), spCandidateTarget.Get(), !!isBackward, &spNewTabStop, &isTabStopOverridden));

            if (isTabStopOverridden && spNewTabStop.Get())
            {
                spNewCandidateTargetAsUIE = spNewTabStop.AsOrNull<IUIElement>();
                if (spNewCandidateTargetAsUIE)
                {
                    IFC(spNewCandidateTargetAsUIE.Cast<UIElement>()->ProcessCandidateTabStopInternal(spFocusedTarget.Get(), spNewTabStop.Get(), !!isBackward, &spNewCandidateTabStop, &isCandidateTabStopOverridden));
                }
            }
        }
    }

    if (isCandidateTabStopOverridden)
    {
        if (spNewCandidateTabStop)
        {
            *ppNewTabStop = spNewCandidateTabStop->GetHandleAddRef();
        }
        *pIsTabStopOverridden = TRUE;
    }
    else if (isTabStopOverridden)
    {
        if (spNewTabStop)
        {
            *ppNewTabStop = spNewTabStop->GetHandleAddRef();
        }
        *pIsTabStopOverridden = TRUE;
    }


Cleanup:
    RRETURN(hr);
}

// Called when ProcessTabStopInternal  interact with the tab stop element.
_Check_return_ HRESULT UIElement::ProcessTabStopInternal(
    _In_opt_ DependencyObject* pCandidateTabStop,
    const bool isBackward,
    const bool didCycleFocusAtRootVisualScope,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pCandidateTabStopOverridden)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spCurrent = this;

    IFCPTR(ppNewTabStop);
    IFCPTR(pCandidateTabStopOverridden);

    *ppNewTabStop = nullptr;
    *pCandidateTabStopOverridden = FALSE;

    while (spCurrent && !(*pCandidateTabStopOverridden))
    {
        ctl::ComPtr<IDependencyObject> spParent;

        IFC(spCurrent.Cast<UIElement>()->ProcessTabStopOverride(
            this,
            pCandidateTabStop,
            isBackward,
            didCycleFocusAtRootVisualScope,
            ppNewTabStop,
            pCandidateTabStopOverridden));

        IFC(VisualTreeHelper::GetParentStatic(spCurrent.Cast<UIElement>(), &spParent));
        spCurrent = spParent.AsOrNull<IUIElement>();
    }

Cleanup:
    RRETURN(hr);
}

// Called when ProcessCandidateTabStop  interact with the candidate tab stop element.
_Check_return_ HRESULT UIElement::ProcessCandidateTabStopInternal(
    _In_opt_ DependencyObject* pCurrentTabStop,
    _In_opt_ DependencyObject* pOverriddenCandidateTabStop,
    _In_ BOOLEAN isBackward,
    _Outptr_ DependencyObject** ppNewTabStop,
    _Out_ BOOLEAN* pCandidateTabStopOverridden)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spCurrent = this;

    IFCPTR(ppNewTabStop);
    IFCPTR(pCandidateTabStopOverridden);

    *ppNewTabStop = nullptr;
    *pCandidateTabStopOverridden = FALSE;

    while (spCurrent && !(*pCandidateTabStopOverridden))
    {
        ctl::ComPtr<IDependencyObject> spParent;

        IFC(spCurrent.Cast<UIElement>()->ProcessCandidateTabStopOverride(
            pCurrentTabStop,
            this,
            pOverriddenCandidateTabStop,
            isBackward,
            ppNewTabStop,
            pCandidateTabStopOverridden));

        IFC(VisualTreeHelper::GetParentStatic(spCurrent.Cast<UIElement>(), &spParent));
        spCurrent = spParent.AsOrNull<IUIElement>();
    }

Cleanup:
    RRETURN(hr);
}

// Called when FocusManager get the next TabStop to interact with the focused control.
_Check_return_ HRESULT
UIElement::GetNextTabStop(
    _In_ CDependencyObject* pFocusedElement,
    _Outptr_ CDependencyObject** ppNextTabStop)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IUIElement> spTargetAsUIE;
    ctl::ComPtr<DependencyObject> spNextTabStop;

    IFCPTR(pFocusedElement);
    IFCPTR(ppNextTabStop);

    *ppNextTabStop = NULL;

    IFC(DXamlCore::GetCurrent()->GetPeer(pFocusedElement, &spTarget));
    spTargetAsUIE = spTarget.AsOrNull<IUIElement>();
    if (spTargetAsUIE)
    {
        IFC(spTargetAsUIE.Cast<UIElement>()->GetNextTabStopOverride(&spNextTabStop));
        if (spNextTabStop)
        {
            *ppNextTabStop = spNextTabStop->GetHandleAddRef();
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when FocusManager get the previous TabStop to interact with the focused control.
_Check_return_ HRESULT
UIElement::GetPreviousTabStop(
    _In_ CDependencyObject* pFocusedElement,
    _Outptr_ CDependencyObject** ppPreviousTabStop)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IUIElement> spTargetAsUIE;
    ctl::ComPtr<DependencyObject> spPreviousTabStop;

    IFCPTR(pFocusedElement);
    IFCPTR(ppPreviousTabStop);

    *ppPreviousTabStop = NULL;

    IFC(DXamlCore::GetCurrent()->GetPeer(pFocusedElement, &spTarget));
    spTargetAsUIE = spTarget.AsOrNull<IUIElement>();
    if (spTargetAsUIE)
    {
        IFC(spTargetAsUIE.Cast<UIElement>()->GetPreviousTabStopOverride(&spPreviousTabStop));
        if (spPreviousTabStop)
        {
            *ppPreviousTabStop = spPreviousTabStop->GetHandleAddRef();
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT UIElement::FocusImpl(
    _In_ xaml::FocusState value,
    _Out_ BOOLEAN* returnValue)
{
    // Use InputActivationBehavior::RequestActivation to match legacy default.
    IFC_RETURN(FocusWithDirection(value, DirectUI::FocusNavigationDirection::None, InputActivationBehavior::RequestActivation, returnValue));
    return S_OK;
}

_Check_return_ HRESULT UIElement::FocusNoActivateImpl(
    _In_ xaml::FocusState value,
    _Out_ BOOLEAN* returnValue)
{
    // Use InputActivationBehavior::RequestActivation to match legacy default.
    IFC_RETURN(FocusWithDirection(value, DirectUI::FocusNavigationDirection::None, InputActivationBehavior::NoActivate, returnValue));
    return S_OK;
}



_Check_return_ HRESULT UIElement::FocusWithDirection(
    _In_ xaml::FocusState value,
    _In_ DirectUI::FocusNavigationDirection focusNavigationDirection,
    InputActivationBehavior inputActivationBehavior,
    _Out_ BOOLEAN* returnValue)
{
    // Throw if customer tries to call Focus(FocusState.Unfocused).
    if (xaml::FocusState_Unfocused == value)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    FocusState valueNative = static_cast<FocusState>(value);

    IFC_RETURN(CheckThread());

    CUIElement* coreUIE = GetHandle();
    if (!coreUIE)
    {
        // Focus may be called on a disconnected element (when the framework
        // peer has been disassociated from its core peer).  If the core peer
        // has already been disassociated, return 'unfocusable'.
        *returnValue = FALSE;

        return S_OK;
    }

    bool animateIfBringIntoView = m_animateIfBringIntoView;
    m_animateIfBringIntoView = false;

    bool focusUpdated = false;
    IFC_RETURN(coreUIE->Focus(valueNative,
        animateIfBringIntoView,
        &focusUpdated,
        focusNavigationDirection,
        inputActivationBehavior));

    *returnValue = focusUpdated;

    return S_OK;
}

// Called when FocusManager is looking for the first focusable element from the specified search scope.
_Check_return_ HRESULT
UIElement::GetFirstFocusableElement(
    _In_ CDependencyObject* pSearchStart,
    _Outptr_ CDependencyObject** ppFirstFocusable)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IUIElement> spTargetAsUIE;
    ctl::ComPtr<DependencyObject> spFirstFocusable;

    IFCPTR(pSearchStart);
    IFCPTR(ppFirstFocusable);

    *ppFirstFocusable = NULL;

    IFC(DXamlCore::GetCurrent()->GetPeer(pSearchStart, &spTarget));
    spTargetAsUIE = spTarget.AsOrNull<IUIElement>();
    if (spTargetAsUIE)
    {
        IFC(spTargetAsUIE.Cast<UIElement>()->GetFirstFocusableElementOverride(&spFirstFocusable));
        if (spFirstFocusable)
        {
            *ppFirstFocusable = spFirstFocusable->GetHandleAddRef();
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when FocusManager is looking for the last focusable element from the specified search scope.
_Check_return_ HRESULT
UIElement::GetLastFocusableElement(
    _In_ CDependencyObject* pSearchStart,
    _Outptr_ CDependencyObject** ppLastFocusable)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IUIElement> spTargetAsUIE;
    ctl::ComPtr<DependencyObject> spLastFocusable;

    IFCPTR(pSearchStart);
    IFCPTR(ppLastFocusable);

    *ppLastFocusable = NULL;

    IFC(DXamlCore::GetCurrent()->GetPeer(pSearchStart, &spTarget));
    spTargetAsUIE = spTarget.AsOrNull<IUIElement>();
    if (spTargetAsUIE)
    {
        IFC(spTargetAsUIE.Cast<UIElement>()->GetLastFocusableElementOverride(&spLastFocusable));
        if (spLastFocusable)
        {
            *ppLastFocusable = spLastFocusable->GetHandleAddRef();
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
UIElement::IsDraggableOrPannable(
    _In_ CUIElement* pElement,
    _Out_ bool* isDraggableOrPannable)
{
    IFCEXPECT_RETURN(pElement);

    *isDraggableOrPannable = false;
    auto current = static_cast<CDependencyObject*>(pElement);

    while (current != nullptr)
    {
        ctl::ComPtr<DependencyObject> peer;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(current, &peer));
        ASSERT(ctl::is<IUIElement>(peer));

        if (peer.Cast<UIElement>()->IsDraggableOrPannableImpl())
        {
            *isDraggableOrPannable = true;
            break;
        }

        current = current->GetParent();
    }

    return S_OK;
}

// Used to set flag for skipping rendering of recycled containers for chrome.
void UIElement::VirtualizationInformation::SetIsRealized(bool isRealized)
{
    CDependencyObject* pOwnerHandle = m_pOwner->GetHandle();

    m_isRealized = isRealized;

    if (pOwnerHandle)
    {
        CUIElement* pOwnerAsCUI = do_pointer_cast<CUIElement>(pOwnerHandle);

        if (pOwnerAsCUI)
        {
            CListViewBaseItemChrome* pChrome = do_pointer_cast<CListViewBaseItemChrome>(pOwnerAsCUI->GetFirstChildNoAddRef());

            if (pChrome)
            {
                pChrome->SetShouldRenderChrome(isRealized);
            }
        }
    }
}

// Returns element's HandOff visual, which is a WinRT Visual
_Check_return_ HRESULT
UIElement::GetElementVisual(
    _Outptr_ WUComp::IVisual** ppResult)
{
    ctl::ComPtr<IUnknown> spHandOffVisual;

    IFC_RETURN(static_cast<CUIElement*>(GetHandle())->GetHandOffVisual(&spHandOffVisual));

    IFC_RETURN(spHandOffVisual.Get()->QueryInterface(IID_PPV_ARGS(ppResult)));

    return S_OK;
}

HRESULT UIElement::get_AccessKeyScopeOwnerImpl(_Outptr_result_maybenull_ xaml::IDependencyObject** ppValue)
{
    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::UIElement_AccessKeyScopeOwner, ppValue));
    return S_OK;
}

HRESULT UIElement::put_AccessKeyScopeOwnerImpl(_In_opt_ xaml::IDependencyObject* pValue)
{
    BOOLEAN isAKO = TRUE;

    ctl::ComPtr<IDependencyObject> ownerAsDO(pValue);

    // pValue == nullptr is valid input.  It means to set the scope owner as the root scope.
    if (pValue && !AccessKeys::IsValidAKOwnerType(ownerAsDO.Cast<DependencyObject>()->GetHandle()))
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ACCESSKEYS_ACCESSKEYOWNER_CDO));
    }

    ctl::ComPtr<IUIElement> owner = ownerAsDO.AsOrNull<IUIElement>();

    if (owner)
    {
        IFC_RETURN(owner.Cast<UIElement>()->get_IsAccessKeyScope(&isAKO));
    }

    if (isAKO)
    {
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_AccessKeyScopeOwner, pValue));
    }
    else
    {
        IFC_RETURN(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_ACCESSKEYS_ACCESSKEYOWNER_ISSCOPE_FALSE));
    }

    return S_OK;
}

_Check_return_ HRESULT UIElement::GetUIElementFocusCandidate(
    _In_ IInspectable* pCandidate,
    _Out_  ctl::ComPtr<xaml::IUIElement>* pUIElementCandidate)
{
    ctl::ComPtr<IInspectable> spCandidate(pCandidate);
    ctl::ComPtr<IUIElement> spCandidateAsUI;

    ASSERT(pCandidate);

    spCandidateAsUI = spCandidate.AsOrNull<UIElement>();

    while (spCandidateAsUI == nullptr)
    {
        // If the candidate is not a UIElement (in the case of HyperLink) make its
        // host the candidate. i.e walk up until we can get an element to focus.
        ctl::ComPtr<IDependencyObject> spParent;
        ctl::ComPtr<DependencyObject> spCandidateAsDO;
        IFC_RETURN(spCandidate.As(&spCandidateAsDO));

        IFC_RETURN(VisualTreeHelper::GetParentStatic(spCandidateAsDO.Get(), &spParent));
        spCandidateAsUI = spParent.AsOrNull<UIElement>();
        spCandidate = spParent;
    }

    *pUIElementCandidate = std::move(spCandidateAsUI);
    ASSERT(pUIElementCandidate);

    return S_OK;
}

_Check_return_ HRESULT UIElement::TransformToVisualImpl(
    _In_ xaml::IUIElement* pVisual,
    _Outptr_ xaml_media::IGeneralTransform** ppReturnValue)
{
    CUIElement* pVisualCore = static_cast<CUIElement*>(pVisual ? static_cast<DirectUI::UIElement*>(pVisual)->GetHandle() : nullptr);

    xref_ptr<CGeneralTransform> transform;
    IFC_RETURN(static_cast<CUIElement*>(GetHandle())->TransformToVisual(pVisualCore, &transform));
    IFC_RETURN(CValueBoxer::ConvertToFramework(transform.get(), ppReturnValue, /* fReleaseCoreValue */ FALSE));

    return S_OK;
}

_Check_return_ HRESULT UIElement::get_LightsImpl(
    _Outptr_result_maybenull_ wfc::IVector<xaml_media::XamlLight*>** ppValue)
{
    IFCFAILFAST(GetValueByKnownIndex(KnownPropertyIndex::UIElement_Lights, ppValue));

    if (*ppValue == nullptr)
    {
        ctl::ComPtr<XamlLightCollection> xamlLightCollection;
        IFCFAILFAST(ctl::make<XamlLightCollection>(&xamlLightCollection));

        IFCFAILFAST(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Lights, xamlLightCollection.Get()));

        *ppValue = xamlLightCollection.Detach();
    }

    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::add_PreviewKeyDown(_In_ xaml_input::IKeyEventHandler* pValue, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(UIElementGenerated::add_PreviewKeyDown(pValue, pToken));
    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::add_PreviewKeyUp(_In_ xaml_input::IKeyEventHandler* pValue, _Out_ EventRegistrationToken* pToken)
{
    IFC_RETURN(UIElementGenerated::add_PreviewKeyUp(pValue, pToken));
    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::remove_PreviewKeyDown(_In_ EventRegistrationToken token)
{
    IFC_RETURN(UIElementGenerated::remove_PreviewKeyDown(token));
    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::remove_PreviewKeyUp(_In_ EventRegistrationToken token)
{
    IFC_RETURN(UIElementGenerated::remove_PreviewKeyUp(token));
    return S_OK;
}

_Check_return_ HRESULT UIElement::get_XamlRootImpl(_Outptr_result_maybenull_ xaml::IXamlRoot** ppValue)
{
    ctl::ComPtr<xaml::IXamlRoot> xamlRoot = XamlRoot::GetForElementStatic(this);
    *ppValue = xamlRoot.Detach();
    return S_OK;
}

_Check_return_ HRESULT UIElement::put_XamlRootImpl(_In_opt_ xaml::IXamlRoot* pValue)
{
    auto xamlRoot = XamlRoot::GetForElementStatic(this).Get();
    if( pValue == xamlRoot )
    {
        return S_OK;
    }

    if( xamlRoot != nullptr )
    {
        IFC_RETURN(DirectUI::ErrorHelper::OriginateErrorUsingResourceID(E_UNEXPECTED, ERROR_CANNOT_SET_XAMLROOT_WHEN_NOT_NULL));
    }

    IFC_RETURN(XamlRoot::SetForElementStatic(this, pValue));
    return S_OK;
}


#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::add_Shown(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    if (uie->HasImplicitShowAnimation() || uie->HasImplicitHideAnimation())
    {
        IFC_RETURN(uie->SetAndOriginateError(E_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SHOWN_HIDDEN_MIXED_WITH_ECP));
    }

    IFC_RETURN(UIElementGenerated::add_Shown(pValue, pToken));

    uie->ShownHiddenHandlerAdded();

    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::remove_Shown(_In_ EventRegistrationToken token)
{
    IFC_RETURN(UIElementGenerated::remove_Shown(token));

    if (!HasShownHiddenHandlers())
    {
        CUIElement* uie = static_cast<CUIElement*>(GetHandle());
        uie->AllShownHiddenHandlersRemoved();
    }

    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::add_Hidden(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken)
{
    CUIElement* uie = static_cast<CUIElement*>(GetHandle());
    if (uie->HasImplicitShowAnimation() || uie->HasImplicitHideAnimation())
    {
        IFC_RETURN(uie->SetAndOriginateError(E_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SHOWN_HIDDEN_MIXED_WITH_ECP));
    }

    IFC_RETURN(UIElementGenerated::add_Hidden(pValue, pToken));

    uie->ShownHiddenHandlerAdded();

    return S_OK;
}

_Check_return_ HRESULT STDMETHODCALLTYPE UIElement::remove_Hidden(_In_ EventRegistrationToken token)
{
    IFC_RETURN(UIElementGenerated::remove_Hidden(token));

    if (!HasShownHiddenHandlers())
    {
        CUIElement* uie = static_cast<CUIElement*>(GetHandle());
        uie->AllShownHiddenHandlersRemoved();
    }

    return S_OK;
}
#endif

bool UIElement::HasShownHiddenHandlers()
{
#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)
    ShownEventSourceType* shownEventSource = nullptr;
    IFCFAILFAST(GetShownEventSourceNoRef(&shownEventSource));
    if (shownEventSource->HasHandlers())
    {
        return true;
    }

    HiddenEventSourceType* hiddenEventSource = nullptr;
    IFCFAILFAST(GetHiddenEventSourceNoRef(&hiddenEventSource));
    return hiddenEventSource->HasHandlers();
#else
    return false;
#endif
}

bool UIElement::HasHiddenHandlers()
{
#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)
    HiddenEventSourceType* hiddenEventSource = nullptr;
    IFCFAILFAST(GetHiddenEventSourceNoRef(&hiddenEventSource));
    return hiddenEventSource->HasHandlers();
#else
        return false;
#endif
}

_Check_return_ HRESULT UIElement::get_InteractionsImpl(_Out_ wfc::IVector<xaml::InteractionBase*>** interactions)
{
#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    if (!m_interactions)
    {
        ctl::ComPtr<InteractionCollection> interactionCollection;
        IFC_RETURN(ctl::make(this, &interactionCollection));
        IFC_RETURN(interactionCollection.As(&m_interactions));
    }

    IFC_RETURN(m_interactions.CopyTo(interactions));
    return S_OK;
#else
    return E_NOTIMPL;
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
}
