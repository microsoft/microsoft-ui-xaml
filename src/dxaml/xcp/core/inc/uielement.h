// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "Indexes.g.h"
#include "PCRenderDataList.h"
#include "pal.h"
#include "EnumDefs.g.h"
#include "RenderParams.h"
#include "BoundsWalkHitResult.h"
#include "rendertypes.h"
#include "HitTestPolygon.h"
#include "InputActivationBehavior.h"
#include "LayoutStorage.h"
#include "GeneralTransform.h"
#include "RenderWalkType.h"
#include "RenderData.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include <DoubleUtil.h>
#include <SimpleProperties.h>
#include <FeatureFlags.h>
#include "FacadeStorage.h"
#include "FacadeReferenceWrapper.h"
#include "AutomationEventsHelper.h"
#include <fwd/windows.foundation.h>
#include "xcpmath.h"
#include <Microsoft.UI.Input.Partner.h>

#undef GetFirstChild

struct REQUEST;
class CRectangleGeometry;
class CProjection;
class CTransform3D;
class CGeometry;
struct ITransformer;
class CPerspectiveTransformer;
class CAggregateTransformer;
class HitTestPolygon;
class LayoutTransitionStorage;
class CTransitionCollection;
class CTransitionTarget;

class HWRealization;
class CTransformToRoot;
class HWCompNode;
class HWCompRenderDataNode;
class CLayoutTransitionElement;
class CLayoutManager;

class CWindowRenderTarget;
class CImageBrush;
class CPopup;
class CLayoutTransitionElement;
class CPointer;
class CPointerCollection;
class HWShapeRealization;
class CEventArgs;
class CBoundedHitTestVisitor;

class CHitTestResults;

class CTransform;
class CUIElementCollection;
class CUIElementCollectionWrapper;
class CStyle;
class CTransitionRoot;
class CUIDMContainer;

struct IDirectManipulationContainerHandler;

class CInputServices;
class CSolidColorBrush;
class ImageSurfaceWrapper;
class LocalTransformBuilder;

class HitTestParams;
class WinRTExpressionConversionContext;
struct RedirectionTransformInfo;
struct ImplicitAnimationInfo;
enum class ImplicitAnimationType;
struct EffectiveVisibilityInfo;
struct FacadeTransformInfo;
struct FacadeMatcherEntry;

namespace Microsoft {
    namespace WRL   {
        namespace Wrappers  {
            class HString;
        }
    }
}

namespace DirectUI
{
    class ScalarTransition;
    class Vector3Transition;
    class BrushTransition;
}

struct D2D_MATRIX_4X4_F;
struct D2D_MATRIX_3X2_F;

enum class TransformRetrievalOptions : unsigned char
{
    None = 0x0,
    IncludePropertiesSetInComposition = 0x1,
    UseManipulationTargets = 0x2                // When getting a transform that is in the process of being manipulated return the target offsets instead of the current values
};
DEFINE_ENUM_FLAG_OPERATORS(TransformRetrievalOptions);

// MAX Z-Index value
#define XCP_MAX_ZINDEX_VALUE 1000000

FLOAT ClampOpacity(_In_ FLOAT opacity);

#define TAB_NAVIGATION_NOTSET   (static_cast<DirectUI::KeyboardNavigationMode>(3))

//------------------------------------------------------------------------
//
//  Class:  CUIElement
//
//  Synopsis:
//      Base UI element object.
//
//------------------------------------------------------------------------
class CUIElement : public CDependencyObject
{
    friend class HWWalk;
    friend class CPopup;
    friend class CPopupRoot;

public:
#if defined(__XAML_UNITTESTS__)
    CUIElement()  // !!! FOR UNIT TESTING ONLY !!!
        : CUIElement(nullptr)
    {}
#endif

    ~CUIElement() override;

public:
    DECLARE_CREATE(CUIElement);

    // CDependencyObject overrides
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    // Scoping
protected:
    CUIElement(_In_ CCoreServices *pCore);

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;
    _Check_return_ HRESULT MarkInheritedPropertyDirty(
        _In_ const CDependencyProperty* pdp,
        _In_ const CValue* pValue) override;

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

    virtual _Check_return_ HRESULT NotifyApplicationHighContrastAdjustmentChanged();
public:

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL) override;

    void PropagateLayoutDirty(bool affectsParentMeasure, bool affectsParentArrange) final;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CUIElement>::Index;
    }

    _Check_return_ HRESULT GetValue(_In_ const CDependencyProperty* dp, _Out_ CValue* value) override;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // We just declared an override for SetValue, which hides all SetValue base methods.
    // We want to unhide SetValue(const CDependencyProperty*, const CValue&), so subclasses
    // can continue to easily call that overload.
    using CDependencyObject::SetValue;

    _Check_return_ HRESULT PullInheritedTextFormatting() override;

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false) override;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    // Typically a UIElement removes all its event listers from the event manager when it leaves the tree. One exception
    // is popups, which can still render even if they're not in the live tree (via parentless popups). The other exceptions
    // are the UIElement.Shown and UIE.Hidden events, which should fire for an element that isn't live if it just entered
    // or left the tree. The param lets us leave Shown and Hidden event handlers on the element. These will be removed
    // later when the UIElement is deleted. If this element enters the tree again, these events won't be added back, because
    // they'll be guarded by the REQUEST::m_bAdded flag.
    _Check_return_ HRESULT RemoveAllEventListeners(bool leaveUIEShownHiddenEventListenersAttached);

    bool IsLoadedEventPending();

    CTransitionRoot* GetLocalTransitionRoot(bool ensureTransitionRoot);

    virtual _Check_return_ HRESULT AddChild(_In_ CUIElement *pChild);
    _Check_return_ HRESULT RemoveChild(_In_ CUIElement *pChild);

    _Check_return_ HRESULT InsertChild(_In_ XUINT32 nIndex, _In_ CUIElement *pChild);

    bool CanModifyChildrenCollection() const;

    _Check_return_ HRESULT NotifyApplicationHighContrastAdjustmentChangedCore();

    void SetLayoutClipDirty();

    DirectUI::ElementCompositeMode GetCompositeMode() const;

    DirectUI::ManipulationModes GetManipulationMode() const;
    _Check_return_ HRESULT static ManipulationMode(
        _In_ CDependencyObject* obj,
        _In_ UINT32 numberOfArgs,
        _Inout_updates_(numberOfArgs) CValue* args,
        _In_opt_ IInspectable* valueOuter,
        _Out_ CValue* result);

    FLOAT GetScaleFactorForLayoutRounding();
    _Check_return_ HRESULT ResetGlobalScaleFactor();
    _Check_return_ HRESULT static GlobalScaleFactor(
        _In_ CDependencyObject* obj,
        _In_ UINT32 numberOfArgs,
        _Inout_updates_(numberOfArgs) CValue* args,
        _In_opt_ IInspectable* valueOuter,
        _Out_ CValue* result);

    xref_ptr<CPointerCollection> GetPointerCaptures() const;
    xref_ptr<CProjection> GetProjection() const;
    xref_ptr<CTransform> GetRenderTransform() const;
    CTransform* GetRenderTransformLocal() const;
    XPOINTF GetRenderTransformOrigin() const;
    xref_ptr<CTransform3D> GetTransform3D() const;
    xref_ptr<CTransitionCollection> GetTransitions() const;
    xref_ptr<WUComp::IExpressionAnimation> GetWUCCanvasOffsetExpression() const;
    xref_ptr<WUComp::IExpressionAnimation> GetWUCOpacityExpression() const;
    xref_ptr<CTransitionTarget> GetTransitionTarget() const;

    // Retrieves a 4x4 matrix used for hit testing. This matrix might come from the hand off visual, rather than a Xaml Transform3D object.
    CMILMatrix4x4 GetHitTestingTransform3DMatrix() const;
    CMILMatrix4x4 GetHitTestingTransform3DMatrix(bool includePropertySetInComposition) const;

    xref_ptr<CGeometry> GetClip() const;
    _Check_return_ HRESULT SetClip(
        _In_ xref_ptr<CGeometry> value
        );

    xref_ptr<CTransform> GetHandOffVisualTransform() const;

    virtual xref_ptr<CFlyoutBase> GetContextFlyout() const;
    _Check_return_ HRESULT SetContextFlyout(
        _In_ xref_ptr<CFlyoutBase> value
        );

    xref_ptr<CXamlLightCollection> GetXamlLightCollection() const;
    bool HasXamlLights() const;

    bool StoreLayoutCycleWarningContexts();

    void StoreLayoutCycleWarningContext(size_t framesToSkip = DEFAULT_WARNING_FRAMES_TO_SKIP + 2);
    void StoreLayoutCycleWarningContext(_In_opt_ CLayoutManager* layoutManager, size_t framesToSkip = DEFAULT_WARNING_FRAMES_TO_SKIP + 1);
    void StoreLayoutCycleWarningContext(_In_ std::vector<std::wstring>& warningInfo, _In_opt_ CLayoutManager* layoutManager = nullptr, size_t framesToSkip = DEFAULT_WARNING_FRAMES_TO_SKIP);

public:
    bool IsInUIElementsAdjustedVisualSubTree(_In_ const CUIElement *pPotentialParentOrSelf);

    // Entry points for all hit testing, internal (eg mouse move event hit testing)
    // and external (VisualTreeHelpers.FindElementsInHostCoordinates) APIs.
    _Check_return_ HRESULT HitTestEntry(
        const HitTestParams& hitTestParams,
        _In_ XPOINTF hitPoint,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_ CUIElement **ppHitElement
        );

    _Check_return_ HRESULT HitTestEntry(
        const HitTestParams& hitTestParams,
        _In_ XPOINTF hitPoint,
        _In_ bool fHitMultiple,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Inout_ CHitTestResults *pHitElements
        );

    _Check_return_ HRESULT HitTestEntry(
        const HitTestParams& hitTestParams,
        const HitTestPolygon& hitPolygon,
        _In_ bool canHitMultipleElements,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Inout_ CHitTestResults *pHitElements
        );

    bool IsEnabledAndVisibleForHitTest(bool canHitDisabledElements = false, bool canHitInvisibleElements = false) const;

    bool IsEnabledAndVisibleForDCompHitTest() const;

    virtual bool IsMaskDirty(
        _In_ HWShapeRealization *pHwShapeRealization,
        const bool renderCollapsedMask,
        bool isFillBrushAnimated,
        bool isStrokeBrushAnimated,
        _Inout_ bool* pIsFillForHitTestOnly,
        _Inout_ bool* pIsStrokeForHitTestOnly
        )
    {
        return false;
    }

    _Check_return_ HRESULT IsOccluded(_In_ CUIElement *pChildElement, _In_ const XRECTF_RB& elementBounds, _Out_ bool* isOccluded);

private:
    _Check_return_ HRESULT NWGetTransformerHelper(
        _Outptr_result_maybenull_ CPerspectiveTransformer **ppTransformer3D
        );

    // function to return the Transfomer from the current UI element to the element provided
    // isReverse determines whether the provided element is a child to this element
    _Check_return_ HRESULT TransformToVisualHelperGetTransformer(
        _In_ CUIElement* pVisual,
        _Out_ bool* isReverse,
        _Outptr_ ITransformer** returnValue);

public:
    // GetClickablePoint for AutomationPeer
    _Check_return_ HRESULT GetClickablePointRasterizedClient(_Out_ XPOINTF *pPoint);

    // Adjust factor to account for intermediates
    _Check_return_ HRESULT AdjustBoundingRectToRoot( _Inout_ XRECTF *pRect);

    // Gets the first element in the ancestor chain that renders to an intermediate
    _Ret_maybenull_ CUIElement* GetFirstAncestorWithIntermediate();

    // Gets the parent of element and adjust it to popup if parent is PopupRoot
    _Ret_maybenull_ CUIElement* GetUIElementAdjustedParentInternal(
        bool fPublicParentsOnly = true,
        bool useRealParentForClosedParentedPopups = false
        );


    bool IsVisible() const;
    bool IsCollapsed() const;
    DirectUI::Visibility GetVisibility() const;
    void SetVisibility(DirectUI::Visibility value);
    bool AreAllAncestorsVisible();
    EffectiveVisibilityInfo ComputeEffectiveVisibility();

    //
    // "Keep visible" mechanism
    //
    // Several features need a UIElement to be "kept visible" when it shouldn't be. These include ElementCompositionPreview's
    // implicit hide animations and the UIElement.Hidden event, where an app can choose to start animations on the element
    // which would not otherwise be visible.
    //
    // The "keep visible" mechanism has several layers, because there are several independent reasons why an element could be
    // made invisible. Each layer has its own entrypoint to the KV mechanism, and they are all called at the time when an element
    // would be made invisible. Timing matters here, because once an element is made invisible, KV cannot retroactively make it
    // visible. That would require things like adding the element back into the tree, which has lots of side effects.
    //
    // At the beginning of each frame, we walk all elements currently in KV to check whether they still belong in KV. Any elements
    // that should no longer be in KV are unmarked at this point. We do things this way because there are several things that can
    // make en element invisible. So in general an element enters KV when it hits any one of these conditions, and it exits KV
    // at the beginning of the frame after all conditions have been removed.
    //
    // These are the ways that an element can enter the KV mechanism:
    //
    //   1. An element could be made invisible through Visibility="Collapsed".
    //
    //      CUIElement::SetVisibility can compute whether an element should be kept visible (by checking for features needing it
    //      in the subtree) and add it to the KV list directly. After that, the element gets collapsed normally. When rendering,
    //      we allow Visiblity="Collapsed" elements to be rendered if they're in the KV list.
    //
    //      Setting an element back to visible does not cancel the existing KV. We wait until the beginning of the next frame, when
    //      we can calculate whether the element should still have KV.
    //
    //   2. An element could be taken out of the UIElement tree.
    //
    //      This is a more complex case. Typically there are operations that need to be done when an element is taken out, such as
    //      removing the element from a child collection and clearing its parent pointer, that cannot be done if the element is to
    //      continue rendering. With KV, we defer these operations by using unloading storage. The element is moved from the public
    //      child collection into the unloading storage of that collection, and it continues to render.
    //
    //      Putting the element back in the tree will flush these pending operations first before proceeding with inserting the
    //      element, but it will not cancel the existing KV. We do that at the beginning of the next frame.
    //
    //   3. If the element is a popup, it could get closed. Popups have many additional operations when closing that need to be
    //      deferred, including removing the popup from the popup root's list of open popups, tearing down rendering primitives for
    //      the popup, and hiding the window for windowed popups. These aren't things supported by unloading storage, so we defer
    //      them manually with a flag in CPopup.
    //
    //      Popups also internally calls close a lot in scenarios like being removed from the tree or changing a child element. We
    //      will defer during these implicit close calls as well.
    //
    //      Opening a closed popup will flush these pending operations first before proceeding with opening the popup, but it will
    //      not cancel the existing KV. We do that at the beginning of the next frame.
    //
    //   4. An element could be made invisible if it's the Child of an open popup and the Child is changed. This is different from
    //      case 2 because Popup.Child is not part of the official child collection, so it doesn't have an unloading storage to back
    //      it up. Here, we'll roll our own unloading storage with extremely limited features to handle just this case. The operations
    //      that need to be deferred are severing the connection between the popup and the previous child, and disassociating the
    //      previous child with the popup.
    //
    //      Putting the element back in the tree will flush these pending operations first before proceeding with inserting the
    //      element, but it will not cancel the existing KV. We do that at the beginning of the next frame. Note that the element
    //      could be set as the Child of a different popup, or it could enter the Children collection of some other element.
    //
    // There are two ways for an element to exit the KV mechanism:
    //
    //   1. An element no longer requests KV. This happens if its subtree no longer uses any of the features associated with KV.
    //      In this case, we'll detect that it no longer requests KV at the beginning of a frame, and unmark it. Any pending operations
    //      associated with KV like unloading storage are flushed at this point.
    //
    //   2. An element becomes visible. This happens if the conditions that made the element invisible (e.g. Visibility="Collapsed",
    //      being taken out of the tree) are removed (e.g. visibility gets switched back to Visible, element added back to the tree).
    //      In this case, we will immediately flush any pending operations as the condition is removed, but we will not unmark the
    //      element until the beginning of the next frame. We do things this way because an element could have multiple things that
    //      put it in KV (e.g. both collapsed and out of the tree), and restoring only one of these (e.g. putting it back in the tree)
    //      means we should finish pending work (e.g. take it out of unloading storage), but we should not unmark it for KV.
    //

    //
    //  TODO_KV - what if a collapsed element is taken out? It should still be KV. We shouldn't cancel.
    //  TODO_KV - what if a collapsed element is added back? It should still be KV. We shouldn't cancel.
    //  TODO_KV - what if a ECP hide child is removed from an unattached subtree? It should not be KV.
    //  TODO_KV - unload a Popup.Child, then unload a second one before the first is done. The second should replace the first.
    //  TODO_KV - remove a popup.child, then unparent the popup, then reparent the popup and open it.
    //

    void SetImplicitShowHideAnimation(ImplicitAnimationType iaType, _In_opt_ WUComp::ICompositionAnimationBase* animation);
    static CompositionRequirement ImplicitAnimationTypeToCompositionRequirement(ImplicitAnimationType iaType);
    bool ComputeKeepVisible();
    void SetKeepVisible(bool keepVisible);
    bool IsKeepVisible();

    // Used by derived types to clean up pending operations when leaving the KeepVisible mechanism. Popup uses this to close
    // windowed popups, for example.
    virtual void FlushPendingKeepVisibleOperations();

    bool HasImplicitAnimation(ImplicitAnimationType iaType) const;
    bool HasImplicitShowAnimation() const { return m_hasImplicitShowAnimation == TRUE; }
    bool HasImplicitHideAnimation() const  { return m_hasImplicitHideAnimation == TRUE; }
    ImplicitAnimationInfo GetImplicitAnimationInfo(ImplicitAnimationType iaType);
    bool IsImplicitAnimationPlaying(ImplicitAnimationType iaType);
    void SetImplicitAnimationRequested(ImplicitAnimationType iaType, bool requested);
    bool IsImplicitAnimationRequested(ImplicitAnimationType iaType);
    void TriggerImplicitShowHideAnimations();
    void PlayImplicitAnimation(ImplicitAnimationInfo& info, ImplicitAnimationType iaType);
    void CancelImplicitAnimation(ImplicitAnimationType iaType);
    void SetDirtyToRoot();

    // Bookkeeping methods. A UIElement cares about trackign its effective visibility if anybody has a Shown/Hidden handler attached.
    // TODO: Does this work if you attach a Hidden handler while it's in the tree? We'll correctly detect that it's visible to begin with, right?
    void ShownHiddenHandlerAdded();
    void AllShownHiddenHandlersRemoved();
    bool AllowsHandlerWhenNotLive(XINT32 iListenerType, KnownEventIndex eventIndex) const override;
    void FireShownHiddenEvent(KnownEventIndex eventIndex);
    bool HasShownHiddenHandlers() const;
    bool HasHiddenHandlers() const;

    bool HasKeepAliveCount() const;
    int GetKeepAliveCount() const;
    void RequestKeepAlive();
    void ReleaseKeepAlive();
    bool IsKeepingAliveUntilHiddenEventIsRaised() const;
    void SetIsKeepingAliveUntilHiddenEventIsRaised(bool value);

    XCP_FORCEINLINE XFLOAT GetOpacityLocal();
    XCP_FORCEINLINE void SetOpacityLocal(XFLOAT value);
    XFLOAT GetOpacityCombined();
    FLOAT GetEffectiveOpacity();
    XCP_FORCEINLINE void HideElementForSuspendRendering(bool hidden);

    // CUIElement methods

    // Implements a depth-first search of the element's sub-tree,
    // looking for an accelerator that can be invoked
    static _Check_return_ HRESULT TryInvokeKeyboardAccelerator(
        _In_ const CDependencyObject* const pFocusedElement,
        _In_ CUIElement* const pElement,
        _In_ const wsy::VirtualKey key,
        _In_ const wsy::VirtualKeyModifiers keyModifiers,
        _Inout_ BOOLEAN& handled,
        _Inout_ BOOLEAN& handledShouldNotImpedeTextInput
    );

    bool NeedsIntermediateRendering() const;

    _Check_return_ HRESULT static ZIndex(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    // Pointer CapturePointer/ReleasePointerCapture/ReleasePointerCaptures
    _Check_return_ HRESULT CapturePointer(_In_ CPointer* pPointer, _Out_ bool *pbCapture);
    _Check_return_ HRESULT ReleasePointerCapture(_In_ CPointer* pPointer);
    _Check_return_ HRESULT ReleasePointerCaptures();

    _Check_return_ HRESULT TransformToVisual(
        _In_ CUIElement* pVisual,
        _Out_ xref_ptr<CGeneralTransform>* transform
        );

    _Check_return_ HRESULT TransformToVisual(
        _In_ CUIElement* pVisual,
        _In_ const bool ignore3D,
        _Out_ xref_ptr<CGeneralTransform>* transform
        );

    _Check_return_ HRESULT TransformToAncestor(
        _In_ const CUIElement *pAncestor,
        _Outptr_ ITransformer** ppTransformer
        );

    // See: /design-notes/OneCoreTransforms.md for more information about this function's coordinate space
    _Check_return_ HRESULT TransformToRoot(
        _Outptr_ ITransformer** ppTransformer
        );

    _Check_return_ HRESULT TransformToParentWithIntermediate(
        _Outptr_ ITransformer** ppTransformer
        );

    _Check_return_ HRESULT GetTransformer(
        _Outptr_result_maybenull_ ITransformer** ppTransformer );

    virtual void GetShouldFlipRTL(
        _Out_ bool *pShouldFlipRTL,
        _Out_ bool *pShouldFlipRTLInPlace
        );

    _Check_return_ bool GetLocalTransform(
        TransformRetrievalOptions transformRetrievalOptions,
        _Out_ CMILMatrix *pLocalTransformNoRef);

    static void GetLocalTransformHelper(
        _Inout_ LocalTransformBuilder* pBuilder,
        XFLOAT offsetX,
        XFLOAT offsetY,
        XFLOAT dmOffsetX,
        XFLOAT dmOffsetY,
        XFLOAT dmZoomFactorX,
        XFLOAT dmZoomFactorY,
        bool flipRTL,
        bool flipRTLInPlace,
        XFLOAT elementWidth,
        XFLOAT elementHeight,
        _In_opt_ CTransform *pRenderTransform,
        XPOINTF renderTransformOrigin,
        _In_opt_ CTransitionTarget* pTransitionTarget,
        bool applyDMZoomToOffset,
        _In_opt_ IUnknown* pDManipSharedTransform,
        _In_opt_ RedirectionTransformInfo* redirInfo,
        _In_opt_ FacadeTransformInfo* facadeInfo
        );

    CMILMatrix4x4 GetLocalTransform4x4();

    _Check_return_ HRESULT PushTransform( _Out_ XINT32 *pbPushed );

    template <typename NWHitType>
    _Check_return_ HRESULT NWPushTransformToRootTransform(
        _Inout_ NWHitType &hitData,
        _Inout_ CMILMatrix *pTransform
        );

    FLOAT GetOffsetX() const;
    FLOAT GetOffsetY() const;
    INT32 GetZIndex() const;

    _Check_return_ HRESULT SetZIndex(_In_ INT32 value);
    bool IsDrawOrderSmallerThan(_In_ const CUIElement *pOther) const;
    _Check_return_ HRESULT OnZOrderChanged();

    CUIElementCollectionWrapper GetUnsortedChildren();

    CAutomationPeer* OnCreateAutomationPeer() final;
    CAutomationPeer* GetAutomationPeer() final;

    _Check_return_ HRESULT SetAutomationPeer(_In_ CAutomationPeer* pAP) final;

    CAutomationPeer* OnCreateAutomationPeerImpl() override;

    _Check_return_ XUINT32 GetAPChildrenCount();
    virtual _Check_return_ XUINT32 GetLinkAPChildrenCount()
    {
        return 0;
    }
    _Check_return_ XUINT32 GetAPChildren(_Outptr_result_buffer_(return) CAutomationPeer ***pppReturnAP) final;
    bool ShouldSkipForAPChildren(_In_ CUIElement *pChild);
    bool ShouldSkipForAPPopupChildren(_In_ CUIElement *pChild, _In_opt_ const CPopupRoot* pPopupRoot) const;
    _Check_return_ CPopupRoot* GetPopupRoot();

    CAutomationPeer* GetPopupAssociatedAutomationPeer() final;

    static _Check_return_ HRESULT HitTestVisible(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static _Check_return_ HRESULT AllowDrop(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static _Check_return_ HRESULT IsTapEnabled(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static _Check_return_ HRESULT IsDoubleTapEnabled(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static _Check_return_ HRESULT IsRightTapEnabled(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static _Check_return_ HRESULT IsHoldEnabled(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static _Check_return_ HRESULT CanDrag(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    static  _Check_return_ HRESULT UseLayoutRounding(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    virtual _Check_return_ HRESULT CoerceIsEnabled(_In_ bool bIsEnabled, _In_ bool bCoerceChildren);

    virtual _Check_return_ HRESULT RaiseIsEnabledChangedEvent(_In_ CValue *pValue)
    {
        (pValue); // Ignore the parameter.
        return S_OK;
    }

    static _Check_return_ HRESULT VisibilityState(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    virtual _Check_return_ XFLOAT GetActualWidth();
    virtual _Check_return_ XFLOAT GetActualHeight();

    //
    // UIElement methods for layout transitions
    //
    _Check_return_ HRESULT AddLayoutTransitionRenderer(
        _In_ CLayoutTransitionElement *pLayoutTransitionElement
        );

    _Check_return_ HRESULT RemoveLayoutTransitionRenderer(
        _In_ CLayoutTransitionElement *pLayoutTransitionElement
        );

    bool IsHiddenForLayoutTransition() const
    {
        return m_pLayoutTransitionRenderers != nullptr && !m_pLayoutTransitionRenderers->empty();
    }

    const xvector<CLayoutTransitionElement*>* GetLayoutTransitionElements() const { return m_pLayoutTransitionRenderers; }

    // Returns TRUE if this element has any absolutely positioned layout transition
    // renderers.
    bool HasAbsolutelyPositionedLayoutTransitionRenderers() const ;

    // Removes any absolutely positioned layout transition renderers.
    _Check_return_ HRESULT RemoveAbsolutelyPositionedLayoutTransitionRenderers();

    //
    // Other UIElement methods
    //
    bool IsHitTestVisible(bool canHitInvisibleElements = false) const
    {
        return (m_fHitTestVisible || canHitInvisibleElements) && IsVisible() && !m_isHitTestingSuppressed;
    }

    bool AllowsDrop() const
    {
        return m_fAllowDrop;
    }

    bool IsTapEnabled() const
    {
        return m_bTapEnabled;
    }

    bool IsDoubleTapEnabled() const
    {
        return m_bDoubleTapEnabled;
    }

    bool IsRightTapEnabled() const
    {
        return m_bRightTapEnabled;
    }

    bool IsHoldEnabled() const
    {
        return m_bHoldEnabled;
    }

    _Check_return_ HRESULT CanDrag(_Out_ bool *canDrag)
    {
        *canDrag = !!m_bCanDrag;

        if (!(*canDrag) && OfTypeByIndex<KnownTypeIndex::ListViewBaseItem>())
        {
            CValue value;
            IFC_RETURN(GetValueByIndex(KnownPropertyIndex::ListViewBaseItem_IsDraggable, &value));
            *canDrag = value.AsBool();
        }
        return S_OK;
    }

    bool IsEnabled() const
    {
        return m_fCoercedIsEnabled;
    }

    bool GetIsEnabled() const
    {
        return m_fIsEnabled;
    }

    void SetIsEnabled(bool fIsEnabled);

    bool GetCoercedIsEnabled() final
    {
        return m_fCoercedIsEnabled;
    }

    void SetCoercedIsEnabled(bool fIsEnabled)
    {
        m_fCoercedIsEnabled = fIsEnabled;
    }

    void SetUseLayoutRounding(bool fUseLayoutRounding)
    {
        m_fUseLayoutRounding = fUseLayoutRounding;
    }

    bool GetUseLayoutRounding() const override
    {
        return !!m_fUseLayoutRounding;
    }

    void SetSkipFocusSubtree(bool skipFocusSubtree, bool forOffScreenPosition = false)
    {
        if (forOffScreenPosition)
        {
            m_skipFocusSubtree_OffScreenPosition = skipFocusSubtree;
        }
        else
        {
            m_skipFocusSubtree_Other = skipFocusSubtree;
        }
    }

    bool SkipFocusSubtree(bool ignoreOffScreenPosition = false) const
    {
        return (!ignoreOffScreenPosition && m_skipFocusSubtree_OffScreenPosition) || m_skipFocusSubtree_Other;
    }

    // Inherited text property support
    TextFormatting **GetTextFormattingMember() final {return &m_pTextFormatting;}
    bool             HasInheritedProperties() final  {return true;}

    virtual void EvaluateIsRightToLeft();

    bool IsRightToLeft() override;
  
    wil::details::lambda_call<std::function<void()>> LockParent();
  
    _Check_return_ HRESULT Focus(_In_ DirectUI::FocusState focusState,
        _In_ bool animateIfBringIntoView,
        _Out_ bool* focusChanged,
        _In_ DirectUI::FocusNavigationDirection focusNavigationDirection = DirectUI::FocusNavigationDirection::None,
        InputActivationBehavior inputActivationBehavior = InputActivationBehavior::RequestActivation); // default to request activation to match legacy behavior

    bool IsFocused() const
    {
        return GetFocusState() != DirectUI::FocusState::Unfocused;
    }
    bool IsKeyboardFocused() const
    {
        return GetFocusState() == DirectUI::FocusState::Keyboard;
    }

    DirectUI::FocusState GetFocusState() const
    {
        // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
        if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_FocusState))
        {
            CValue result;
            VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_FocusState, &result));
            return static_cast<DirectUI::FocusState>(result.AsEnum());
        }
        return DirectUI::FocusState::Unfocused;
    }

    virtual HRESULT UpdateFocusState(_In_ DirectUI::FocusState focusState);

    virtual bool IsFocusable();

    virtual bool IsFocusableForFocusEngagement()
    {
        return false;
    }

    virtual XINT32 TabIndex() const
    {
        // Perf:  Avoid overhead of GetValueByIndex if the effective value is not actually set
        if (IsEffectiveValueInSparseStorage(KnownPropertyIndex::UIElement_TabIndex))
        {
            CValue result;
            VERIFYHR(GetValueByIndex(KnownPropertyIndex::UIElement_TabIndex, &result));
            return (result.As<valueSigned>());
        }
        return XINT32_MAX;
    }

    bool IsTabStop() const
    {
        return m_isTabStop;
    }

    void SetIsTabStop(bool isTabStop)
    {
        m_isTabStop = isTabStop;
    }

    bool IsProtectedCursorSet() const
    {
        return m_isProtectedCursorSet;
    }

    void SetIsProtectedCursorSet(bool isProtectedCursorSet)
    {
        m_isProtectedCursorSet = isProtectedCursorSet;
    }

    static _Check_return_ HRESULT IsTabStopPropertyGetterSetter(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    // Variables used for UIAutomation
    _Check_return_ HRESULT CheckAutomaticAutomationChanges();

    void RegisterForStructureChangedEvent(AutomationEventsHelper::StructureChangedType type);


    // implicit style tree walker
    virtual _Check_return_ HRESULT UpdateImplicitStyle(
        _In_opt_ CStyle *pOldStyle,
        _In_opt_ CStyle *pNewStyle,
        bool bForceUpdate,
        bool bUpdateChildren = true,
        bool isLeavingParentStyle = false
        );

    bool GetAllowDrop() const
    {
        return m_fAllowDrop;
    }

    void SetAllowDrop(bool fAllowDrop)
    {
        m_fAllowDrop = fAllowDrop;
    }

    bool ReferenceTrackerWalkCore(
        _In_ DirectUI::EReferenceTrackerWalkType walkType,
        _In_ bool isRoot,
        _In_ bool shouldWalkPeer) override;

    _Check_return_ HRESULT EnsureChildrenCollection();

    _Check_return_ XUINT32 GetAPPopupChildrenCount();
    _Check_return_ XUINT32 AppendAPPopupChildren(_In_ XUINT32 nCount, _Inout_updates_(nCount) CAutomationPeer ** ppChildrenAP);
    _Check_return_ XUINT32 GetAPPopupChildren(_In_ XUINT32 nCount, _Inout_updates_opt_(nCount) CAutomationPeer ** ppChildrenAP);

    static CBrushTransition* GetBrushTransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex);
    static CVector3Transition* GetVector3TransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex);
    static CScalarTransition* GetScalarTransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex);

    wrl::ComPtr<mui::IInputCursor> GetProtectedCursor();

private:
    template <typename InterfaceType, typename DXamlType, typename CoreType> static CoreType* GetTransitionNoRef(_In_ CDependencyObject& object, KnownPropertyIndex propertyIndex);

    typedef bool (CUIElement::*BitfieldGetFn)() const;
    typedef void (CUIElement::*BitfieldSetFn)(bool newValue);

    static _Check_return_ HRESULT GetOrSetBitfield(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_ BitfieldGetFn pGetFn,
        _In_ BitfieldSetFn pSetFn,
        _Out_ CValue *pResult
        );

    // The global scale factor is retrieved in CUIElement::LayoutRound only for elements that are manipulatable by DManip.
    bool UseGlobalScaleFactorForLayoutRounding() const
    {
        return IsManipulatable();
    }

    bool GetIsHitTestVisible() const
    {
        return m_fHitTestVisible;
    }

    void SetIsHitTestVisible(bool fIsHitTestVisible);

    bool GetIsTapEnabled() const
    {
        return m_bTapEnabled;
    }

    void SetIsTapEnabled(bool isTapEnabled)
    {
        m_bTapEnabled = isTapEnabled;
    }

    bool GetIsDoubleTapEnabled() const
    {
        return m_bDoubleTapEnabled;
    }

    void SetIsDoubleTapEnabled(bool isDoubleTapEnabled)
    {
        m_bDoubleTapEnabled = isDoubleTapEnabled;
    }

    bool GetIsRightTapEnabled() const
    {
        return m_bRightTapEnabled;
    }

    void SetIsRightTapEnabled(bool isRightTapEnabled)
    {
        m_bRightTapEnabled = isRightTapEnabled;
    }

    bool GetIsHoldEnabled() const
    {
        return m_bHoldEnabled;
    }

    void SetIsHoldEnabled(bool isHoldEnabled)
    {
        m_bHoldEnabled = isHoldEnabled;
    }

    bool GetCanDrag() const
    {
        return m_bCanDrag;
    }

    void SetCanDrag(bool canDrag)
    {
        m_bCanDrag = canDrag;
    }

    static bool GetIsManipulationModeInvalid(_In_ DirectUI::ManipulationModes manipulationMode);

    _Check_return_ HRESULT GetTransformToAncestorCommon(
        _In_opt_ const CUIElement *pAncestor,
        bool fPublicParentsOnly,
        bool fStopAtParentWithIntermediate,
        _Outptr_ ITransformer** ppTransformer
        );

    _Check_return_ HRESULT PushUseLayoutRounding(bool fUseLayoutRounding);

    _Check_return_ HRESULT CoerceIsEnabledOnVisualChildren(bool fIsEnabled);

    _Check_return_ HRESULT CreateChildrenCollection(
        _Outptr_ CUIElementCollection** ppChildrenCollection
        );

    // Discards the potential rejection viewports associated with this element or its subtree.
    _Check_return_ HRESULT DiscardRejectionViewportsInSubTree();

    // Process PointerExited event if the pointer entered element change the state by leaving tree,
    // disabled or collapsed.
    _Check_return_ HRESULT ProcessPointerExitedEventByPointerEnteredElementStateChange();

    // Note: We listen for property changes (for transforms & clips) in the hand off visual and put them in a private stash,
    // then when it comes time to do hit testing or bounds calculations we prefer the private stash over the Xaml properties.
    // This lets us stay updated with values that the app has set directly on the hand off visual, but the property notification
    // is not synchronous. As a result, if the app updates a property in DComp, then immediately does programmatic hit testing
    // or asks for the bounds, they'll get a stale value. If the app updates a Xaml property on an element with a hand off visual,
    // then does programmatic hit testing or accesses the bounds, they'll also get a stale value, because Xaml has to render with
    // the updated values first, then DComp has to send the notifications, and finally the private stash will be updated with
    // fresh values.  The one exception for XAML property changes being stale is the layout offset (ActualOffsetX/Y), these two
    // properties are forcefully updated in the private stash just after Arrange in order to help address this limitation.
    _Check_return_ HRESULT EnsureHandOffVisualTransformGroupAndCollection();
    _Check_return_ HRESULT EnsureHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType);
    void ClearHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType);
    _Check_return_ HRESULT CreateAndInsertHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType);
    xref_ptr<CRectangleGeometry> EnsureHandOffVisualClip();

    unsigned int GetHandOffVisualSubTransformOrder(_In_ KnownTypeIndex subTransformType);
    void SetHandOffVisualTransformDirty();

    xref_ptr<CTransformGroup> GetHandOffVisualTransformGroup() const;
    xref_ptr<CTransformCollection> GetHandOffVisualTransformCollection() const;
    xref_ptr<CTransform> GetHandOffVisualSubTransform(_In_ KnownTypeIndex subTransformType) const;

    xref_ptr<CRectangleGeometry> GetHandOffVisualClip() const;

    // Returns whether we have a transform matrix reported by WUC. If one exists, then we'll always prefer it over the Xaml
    // properties in container visuals mode. It's possible that the Xaml properties contain a Transform3D, but the app has
    // overwritten the WUC visual's TransformMatrix with a 2D matrix, in which case we'll use the 2D matrix in the private stash
    // and ignore the Transform3D.
    // In legacy DComp visuals mode, the hand off transform stacks with the Xaml property, since they're set on different visuals.
    bool HasHandOffVisualTransform() const;

    bool HasHandOffVisualTransformMatrix3D() const;
    CMILMatrix4x4 GetHandOffVisualTransformMatrix3D() const;
    void EnsureHandOffVisualTransformMatrix3D(_In_ const CMILMatrix4x4& matrix);
    void ClearHandOffVisualTransformMatrix3D();

    // Used mainly for hit testing. Retrieves the WUC clip on the WUC visual, if the clip exists. Otherwise falls back to the Xaml
    // clip. This only deals with UIElement.Clip, and not the layout clip. The layout clip is never set on the hand off (priamry)
    // visual, it'll be set on the prepend or the transition clip visual instead.
    // There are other places that use the element clip, like printing, and there we're intentionally ignoring the hand off visual
    // clip. The software rasterizer code path is aware of the WUC tranform, so we'll make it aware of the WUC clip as well.
    xref_ptr<CGeometry> GetHandOffVisualClipOrElementClip() const;
    _Check_return_ HRESULT OnImplicitHideAnimationCompleted(
        _In_ IInspectable* sender,
        _In_ WUComp::ICompositionBatchCompletedEventArgs* args);

    _Check_return_ HRESULT OnImplicitShowAnimationCompleted(
        _In_ IInspectable* sender,
        _In_ WUComp::ICompositionBatchCompletedEventArgs* args);

    void UpdateImplicitShowHideAnimationInfo(ImplicitAnimationType iaType, _In_opt_ WUComp::ICompositionAnimationBase* animation);
    void CleanupImplicitAnimationOnCompletion(ImplicitAnimationType iaType);
    void CleanupImplicitAnimationInfo(ImplicitAnimationType iaType);

protected:
    virtual bool IgnoresInheritedClips() { return false; }

    static _Check_return_ HRESULT AggregateElementTransform(_In_ CUIElement* pElement, _In_ CAggregateTransformer* pAggregate);


public:
    // Default to FALSE and expose as needed.  Elements that don't support having children will never
    // allocate children collections.  Elements that do support children may do so as an implementation
    // detail (e.g. selection grippers for TextBlock), or to support public API exposure (e.g. Panel.Children).
    virtual bool CanHaveChildren() const
    {
        return false;
    }

    _Check_return_ HRESULT static GetChildren(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    _Check_return_ HRESULT static GetChildrenInternal(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
        );

    CUIElement* GetFirstChildNoAddRef();
    virtual CUIElement* GetFirstChild();

    virtual bool AreChildrenInLogicalTree() { return false; }

    _Check_return_ HRESULT InvalidateSubTree( _In_ XINT32 nFlags);

    _Check_return_ HRESULT HasCycle(_Out_ bool *pfHasCycle);

    virtual void Shutdown();

    CUIElementCollection *GetChildren();

    virtual void GetChildrenInRenderOrder(
        _Outptr_result_buffer_maybenull_(*puiChildCount) CUIElement ***pppUIElements,
        _Out_ XUINT32 *puiChildCount
        );

    bool HasProjection() const;
    bool HasActiveProjection() const;
    bool HasTransitionTarget() const;

    _Check_return_ HRESULT GetForegroundBrush(
        _Out_ bool *pHasForegroundProperty,
        _Outptr_result_maybenull_ CBrush **ppForegroundBrush
        );

    // Returns the DComp handoff visual for this CUIElement - it is being requested via the
    // IXamlDCompInteropPrivate implementation of UIElement. The value returned is an IUnknown
    // which is in fact a WUComp::IVisual.
    _Check_return_ HRESULT GetHandOffVisual(
        _Outptr_result_nullonfailure_ IUnknown** ppUnkHandOffVisual
        );

    _Check_return_ HRESULT GetHandOffLayerVisual(_Outptr_result_nullonfailure_ WUComp::ILayerVisual** layerVisual);

    // Discards the DComp handoff visual which was retrieved earlier with the GetHandOffVisual method.
    // Does nothing if GetHandOffVisual was not called.
    void DiscardHandOffVisual();

    // Invoked by ElementCompositionPreviewFactory::GetElementChildVisual to retrieve the previously set child WinRT Visual for this UIElement.
    // Returns nullptr if none was set.
    _Check_return_ HRESULT GetHandInVisual(
        _Outptr_result_maybenull_ WUComp::IVisual** ppChildVisual
        );

    // Invoked by ElementCompositionPreviewFactory::SetElementChildVisual to hook up a WinRT Visual as a child of this UIElement.
    _Check_return_ HRESULT SetHandInVisual(
        _In_opt_ WUComp::IVisual* pChildVisual
        );

    // Returns the previously created WUComp::IVisual instance for this CUIElement, or NULL if none was created.
    // If previously created, the WUComp::IVisual is stored in sparse storage.
    void GetStoredHandOffVisual(_Outptr_result_maybenull_ WUComp::IVisual** ppHandOffVisual);

    // Returns the WUComp::IVisual hand-in visual for this CUIElement.
    void GetStoredHandInVisual(_Outptr_result_maybenull_ WUComp::IVisual** ppChildVisual);

    void SetIsTranslationEnabled(bool enabled);
    bool GetIsTranslationEnabled() const { return m_isTranslationEnabled == TRUE; }
    void AttachListenerToPrependVisual(_In_ WUComp::IVisual* prependVisual);
    void DetachListenerFromPrependVisual();

    wfn::Vector3 GetTranslation(bool preferAnimatingValue = false) const;

    void SetTranslation(const wfn::Vector3& translation);

    void SetTranslationImpl(const wfn::Vector3& oldTranslation);

    FLOAT GetRotation(bool preferAnimatingValue = false) const;

    void SetRotation(FLOAT rotation);

    void SetRotationImpl(DOUBLE oldRotation);

    wfn::Vector3 GetScale(bool preferAnimatingValue = false) const;

    void SetScale(const wfn::Vector3& scale);

    void SetScaleImpl(const wfn::Vector3& oldScale);

    wfn::Matrix4x4 GetTransformMatrix(bool preferAnimatingValue = false) const;

    void SetTransformMatrix(const wfn::Matrix4x4& transformMatrix);

    void SetTransformMatrixImpl(const wfn::Matrix4x4& oldTransformMatrix);

    void OnTransformMatrixForHitTestingChanged(const wfn::Matrix4x4& transformMatrix);

    wfn::Vector3 GetCenterPoint(bool preferAnimatingValue = false) const;

    void SetCenterPoint(const wfn::Vector3& centerPoint);

    void SetCenterPointImpl(const wfn::Vector3& oldCenterPoint);

    wfn::Vector3 GetRotationAxis(bool preferAnimatingValue = false) const;

    void SetRotationAxis(const wfn::Vector3& rotationAxis);

    void SetRotationAxisImpl(const wfn::Vector3& oldRotationAxis);

    double GetRasterizationScale() const;
    void SetRasterizationScale(double rasterizationScale);
    void SetRasterizationScaleImpl(double oldRaserizationScale);
    double GetRasterizationScaleIncludingAncestors();

    // Returns whether an opacity ScalarTransition was started.
    bool BeforeSetLocalOpacity(float newOpacity);
    void OnSetLocalOpacity(bool wasImplicitAnimationStarted);

    void OnActualOffsetChanged();
    wfn::Vector3 GetActualOffset();

    void OnActualSizeChanged();
    wfn::Vector2 GetActualSize();

    bool IsStrictOnly() const;
    const WCHAR* FindFirstUIElementNonStrictOnlyPropertyInUse() const;
    const WCHAR* FindFirstUIElementStrictOnlyPropertyInUse() const;

    // Public Facade Methods
    _Check_return_ HRESULT StartAnimation(_In_ WUComp::ICompositionAnimationBase* animation);
    _Check_return_ HRESULT StopAnimation(_In_ WUComp::ICompositionAnimationBase* animation);
    _Check_return_ HRESULT PopulatePropertyInfo(
        _In_ HSTRING propertyName,
        _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
        );

    // Facade Animation Helper Callback methods.
    void GetFacadeEntries(_Out_ const FacadeMatcherEntry** entries, _Out_ size_t * count);
    void PopulateBackingCompositionObjectWithFacade(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeID);
    _Check_return_ HRESULT PullFacadePropertyValueFromCompositionObject(_In_ WUComp::ICompositionObject* backingCO, KnownPropertyIndex facadeId);
    void FacadeAnimationComplete(KnownPropertyIndex animatedProperty);
    void AllFacadeAnimationsComplete();
    void CreateBackingCompositionObjectForFacade(_In_ WUComp::ICompositor* compositor, _Out_ WUComp::ICompositionObject** backingCO, _Outptr_result_maybenull_ IFacadePropertyListener** listener);

    void AddLightTargetId(_In_ const xstring_ptr& lightId);
    void RemoveLightTargetId(_In_ const xstring_ptr& lightId);

    bool IsLightTarget();
    bool IsTargetedByLight(_In_ CXamlLight* light);
    void UntargetByLight(_In_ CXamlLight* light);

    void SetIsListeningForHandOffVisualPropertyChanges(bool value)
    {
        m_listeningForHandOffVisualPropertyChanges = value;
    }

    bool IsListeningForHandOffVisualPropertyChanges() const
    {
        return m_listeningForHandOffVisualPropertyChanges;
    }

    DCompTreeHost* GetDCompTreeHost() const;

    static void EnsureTranslationInitialized(_In_ IUnknown* primaryVisual);

private:
    _Check_return_ HRESULT StartAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation, bool isImplicitAnimation);
    void StopAllFacadeAnimations();

    _Check_return_ HRESULT EnsureDCompDevice();

    // Start the process of caching the given HandOff visual and other supporting data structures
    void StartHandOffVisualCaching(_In_ WUComp::IVisual* pHandOffVisual);

    // Stop the process of caching this CUIElement's HandOff visual and other supporting data structures
    void StopHandOffVisualCaching();

    // Stores the provided WUComp::IVisual instance in this CUIElement's sparse storage.
    void SetStoredHandInVisual(
        _In_opt_ WUComp::IVisual* pChildVisual
        );

    // Returns the existing DComp handoff visual for this CUIElement, or creates a new one.
    _Check_return_ HRESULT EnsureHandOffVisual(
        _Outptr_result_nullonfailure_ WUComp::IVisual** ppHandOffVisual,
        bool createLayerVisual);

    // Discards the hand-in visual which was set earlier with the SetHandInVisual method.
    // Does nothing if SetHandInVisual was not called.
    void DiscardHandInVisual(_In_ bool isHandInVisualReplaced = false);

    void EnsureTranslationPropertyInitialized();

protected:
    XCP_FORCEINLINE
    bool SoftwareAppliedOpacity();

    XCP_FORCEINLINE _Check_return_
    HRESULT CalculateTransformsForIntermediate(
        _Inout_ CMILMatrix *pmatRTITransform,
        _Inout_ CMILMatrix *pmatRTITransformInverse,
        _Inout_ CMILMatrix *pmatWorldTransformWithoutRTI,
        _Inout_ bool bRenderAtScaleOnly
        );

    XCP_FORCEINLINE _Check_return_
    HRESULT CalculateRTITransform(
        _Inout_ CMILMatrix *pmatRTITransform,
        _Inout_ bool bRenderAtScaleOnly
        );

    void EnsureRootCanvasCompNode();

    //-----------------------------------------------------------------------------
    // LAYOUT Flags and Storage
    //-----------------------------------------------------------------------------

private:
#ifndef LAYOUT_FLAGS
#define LAYOUT_FLAGS

    enum LayoutFlags
    {
        LF_MEASURE_DIRTY          = 0x00000001, // Element requires measure.
        LF_MEASURE_DIRTY_PENDING  = 0x00000002, // Element will require layout when parented to a layout element.
        LF_ON_MEASURE_DIRTY_PATH  = 0x00000004, // A descendant element is dirty.
        LF_ON_MEASURE_STACK       = 0x00000008, // This element is on the path to the element currently being measured.

        LF_ARRANGE_DIRTY          = 0x00000010, // Analogous to measure flags above.
        LF_ARRANGE_DIRTY_PENDING  = 0x00000020,
        LF_ON_ARRANGE_DIRTY_PATH  = 0x00000040,
        LF_ON_ARRANGE_STACK       = 0x00000080,

        LF_HAS_BEEN_MEASURED      = 0x00000100, // This element has been measured.
        LF_ANCESTOR_DIRTY         = 0x00000200, // An ancestor has been made dirty. Used only during a given measure or arrange pass.
        LF_MEASURE_DURING_ARRANGE = 0x00000400, // Element is being measured during an arrange call. Controls invalidation of parent.
        LF_REQUIRES_CLIP          = 0x00000800, // Element is too big for its layout slot and the container wants to clip it.
        LF_SIZE_CHANGED           = 0x00001000, // Element's RenderSize has changed, and it is queued for SizeChanged event.
        LF_WANTS_SIZE_CHANGED     = 0x00002000, // Element is listening for the size changed event
        LF_WANTS_LAYOUT_UPDATED   = 0x00004000, // Element is listening for the layout updated event
        LF_IS_MEASURING_SELF      = 0x00008000, // Element is currently measuring itself in MeasureInternal.

        LF_LAYOUTTRANSITION_DIRTY          = 0x00010000, // Analogous to measure flags above.
        LF_LAYOUTTRANSITION_DIRTY_PENDING  = 0x00020000,
        LF_ON_LAYOUTTRANSITION_DIRTY_PATH  = 0x00040000,
        LF_ON_LAYOUTTRANSITION_STACK       = 0x00080000,

        LF_VIEWPORT_DIRTY             = 0x00100000, // Element is requesting the effective viewports of its descendants to be recalculated.
        LF_ON_VIEWPORT_DIRTY_PATH     = 0x00200000, // A descendant element is dirty.
        LF_CONTRIBUTES_TO_VIEWPORT    = 0x00400000, // A descendant element is listening to changes to its effective viewport.
        LF_WANTS_VIEWPORT             = 0x00800000, // Element is listening to changes to its effective viewport.

        // Automation Peer dirty flags
        LF_AUTOMATION_PEER_DIRTY           = 0x10000000, // Analogous to measure flags above.
        LF_ON_AUTOMATION_PEER_DIRTY_PATH   = 0x40000000,
    };

    static const XUINT32 LF_EXPECTED_EVENTS = LF_WANTS_SIZE_CHANGED |
                                              LF_WANTS_LAYOUT_UPDATED |
                                              LF_WANTS_VIEWPORT;

#endif

    _Check_return_ XCP_FORCEINLINE XUINT32 GetLayoutFlagsOr(XUINT32 flags) const { return m_layoutFlags & flags ? TRUE : FALSE; }
    _Check_return_ XCP_FORCEINLINE XUINT32 GetLayoutFlagsAnd(XUINT32 flags) const { return (m_layoutFlags & flags) == flags ? TRUE : FALSE; }
    XCP_FORCEINLINE void SetLayoutFlags(XUINT32 flags, XUINT32 value)
    {
        if (value) m_layoutFlags |= flags;
        else m_layoutFlags &= ~flags;
    }

public:
    _Check_return_ XCP_FORCEINLINE XUINT32 GetLayoutFlagsRaw() const { return m_layoutFlags; }

    // IsLayoutElement
    // Determines whether or not an element requires layout. Note that all elements will support
    // layout, but only some require it. In general, all 1.0 elements are not layout elements.
    virtual bool GetIsLayoutElement() const { return false; }

    _Check_return_ XUINT32 GetIsParentLayoutElement() const { const CUIElement* pParent = GetUIElementParentInternal(); return pParent && pParent->GetIsLayoutElement(); }

    // IsMeasureDirty
    // This element requires Measure to be called on it. All nodes between it and the root
    // will have IsOnMeasureDirtyPath set.
    bool GetIsMeasureDirty() const { return !!GetLayoutFlagsAnd(LF_MEASURE_DIRTY); }
    void SetIsMeasureDirty(XUINT32 value) { SetLayoutFlags(LF_MEASURE_DIRTY, value); }

    // IsMeasureDirtyPending
    // A layout-affecting property of this element has been changed, but right now, its parent is not
    // a layout element, so we don't want to mark it dirty. This bit will be converted to IsMeasureDirty
    // when this element is parented to a layout element. Until then, we don't care.
    bool GetIsMeasureDirtyPending() const { return !!GetLayoutFlagsAnd(LF_MEASURE_DIRTY_PENDING); }
    void SetIsMeasureDirtyPending(XUINT32 value) { SetLayoutFlags(LF_MEASURE_DIRTY_PENDING, value); }

    // IsOnMeasureDirtyPath
    // A descendant element has IsMeasureDirty = true.
    bool GetIsOnMeasureDirtyPath() const { return !!GetLayoutFlagsAnd(LF_ON_MEASURE_DIRTY_PATH); }
    void SetIsOnMeasureDirtyPath(XUINT32 value) { SetLayoutFlags(LF_ON_MEASURE_DIRTY_PATH, value); }

    // IsOnMeasureStack
    // A descendant element is IsMeasuring = true.
    bool GetIsOnMeasureStack() const { return !!GetLayoutFlagsAnd(LF_ON_MEASURE_STACK); }
    void SetIsOnMeasureStack(XUINT32 value) { SetLayoutFlags(LF_ON_MEASURE_STACK, value); }

    // HasBeenMeasured
    // This element has been measured.
    bool GetHasBeenMeasured() const { return !!GetLayoutFlagsAnd(LF_HAS_BEEN_MEASURED); }
    void SetHasBeenMeasured(XUINT32 value) { SetLayoutFlags(LF_HAS_BEEN_MEASURED, value); }

    // IsMeasuringSelf
    // Element is currently measuring itself in MeasureInternal.
    bool GetIsMeasuringSelf() const { return !!GetLayoutFlagsAnd(LF_IS_MEASURING_SELF); }
    void SetIsMeasuringSelf(XUINT32 value) { SetLayoutFlags(LF_IS_MEASURING_SELF, value); }

    // For Arrange method descriptions, see Measure methods.
    bool GetIsArrangeDirty() const { return !!GetLayoutFlagsAnd(LF_ARRANGE_DIRTY); }
    void SetIsArrangeDirty(XUINT32 value) { SetLayoutFlags(LF_ARRANGE_DIRTY, value); }
    bool GetIsArrangeDirtyPending() const { return !!GetLayoutFlagsAnd(LF_ARRANGE_DIRTY_PENDING); }
    void SetIsArrangeDirtyPending(XUINT32 value) { SetLayoutFlags(LF_ARRANGE_DIRTY_PENDING, value); }
    bool GetIsOnArrangeDirtyPath() const { return !!GetLayoutFlagsAnd(LF_ON_ARRANGE_DIRTY_PATH); }
    void SetIsOnArrangeDirtyPath(XUINT32 value) { SetLayoutFlags(LF_ON_ARRANGE_DIRTY_PATH, value); }
    bool GetIsOnArrangeStack() const { return !!GetLayoutFlagsAnd(LF_ON_ARRANGE_STACK); }
    void SetIsOnArrangeStack(XUINT32 value) { SetLayoutFlags(LF_ON_ARRANGE_STACK, value); }

    // An ancestor has been made measure- or arrange-dirty (depending if we are measuring or arranging)
    bool GetIsAncestorDirty() const { return !!GetLayoutFlagsAnd(LF_ANCESTOR_DIRTY); }
    void SetIsAncestorDirty(XUINT32 value) { SetLayoutFlags(LF_ANCESTOR_DIRTY, value); }

    // A Measure call has occurred while arranging
    bool GetIsMeasureDuringArrange() const { return !!GetLayoutFlagsAnd(LF_MEASURE_DURING_ARRANGE); }
    void SetIsMeasureDuringArrange(XUINT32 value) { SetLayoutFlags(LF_MEASURE_DURING_ARRANGE, value); }

    // The element requires clipping to bounds
    bool GetRequiresClip() const { return !!GetLayoutFlagsAnd(LF_REQUIRES_CLIP); }
    void SetRequiresClip(XUINT32 value) { SetLayoutFlags(LF_REQUIRES_CLIP, value); }

    // The element's size changed and it is queued to receive SizeChanged events.
    bool GetSizeChanged() const { return !!GetLayoutFlagsAnd(LF_SIZE_CHANGED); }
    void SetSizeChanged(XUINT32 value) { SetLayoutFlags(LF_SIZE_CHANGED, value); }

    // For Viewport method descriptions, see Measure methods.
    bool GetIsViewportDirty() const { return !!GetLayoutFlagsAnd(LF_VIEWPORT_DIRTY); }
    void SetIsViewportDirty(XUINT32 value) { SetLayoutFlags(LF_VIEWPORT_DIRTY, value); }
    bool GetIsOnViewportDirtyPath() const { return !!GetLayoutFlagsAnd(LF_ON_VIEWPORT_DIRTY_PATH); }
    void SetIsOnViewportDirtyPath(XUINT32 value) { SetLayoutFlags(LF_ON_VIEWPORT_DIRTY_PATH, value); }

    // The element requires the SizeChanged event to be raised.
    bool GetWantsSizeChanged() const { return !!GetLayoutFlagsAnd(LF_WANTS_SIZE_CHANGED); }
    void SetWantsSizeChanged(bool value) { SetLayoutFlags(LF_WANTS_SIZE_CHANGED, value); }

    // The element requires the LayoutUpdated event to be raised.
    bool GetWantsLayoutUpdated() const { return !!GetLayoutFlagsAnd(LF_WANTS_LAYOUT_UPDATED); }
    void SetWantsLayoutUpdated(XUINT32 value) { SetLayoutFlags(LF_WANTS_LAYOUT_UPDATED, value); }

    // The element requires the EffectiveViewportChanged event to be raised.
    bool GetWantsViewport() const { return !!GetLayoutFlagsAnd(LF_WANTS_VIEWPORT); }
    void SetWantsViewport(XUINT32 value) { SetLayoutFlags(LF_WANTS_VIEWPORT, value); }

    // A descendant requires the EffectiveViewportChanged event to be raised.
    bool GetContributesToViewport() const { return !!GetLayoutFlagsAnd(LF_CONTRIBUTES_TO_VIEWPORT); }
    void SetContributesToViewport(XUINT32 value) { SetLayoutFlags(LF_CONTRIBUTES_TO_VIEWPORT, value); }

    // Automation Peer Dirty Flags
    bool GetIsAutomationPeerDirty() const { return !!GetLayoutFlagsAnd(LF_AUTOMATION_PEER_DIRTY); }
    void SetIsAutomationPeerDirty(XUINT32 value) { SetLayoutFlags(LF_AUTOMATION_PEER_DIRTY, value); }
    bool GetIsOnAutomationPeerDirtyPath() const { return !!GetLayoutFlagsAnd(LF_ON_AUTOMATION_PEER_DIRTY_PATH); }
    void SetIsOnAutomationPeerDirtyPath(XUINT32 value) { SetLayoutFlags(LF_ON_AUTOMATION_PEER_DIRTY_PATH, value); }

    bool HasLayoutStorage() const { return m_hasLayoutStorage; }
    bool HasLayoutClip() { return (m_hasLayoutStorage && LayoutClipGeometry); }
    bool ShouldApplyLayoutClipAsAncestorClip() const;

    void SetIsScrollViewerHeader(bool value) { m_isScrollViewerHeader = value; }
    bool GetIsScrollViewerHeader() const { return m_isScrollViewerHeader; }

    void SetIsItemContainer(bool value) { m_isItemContainer = value; }
    bool GetIsItemContainer() const { return m_isItemContainer; }

    bool GetRequiresMeasure() const;
    bool GetRequiresArrange() const;
    bool GetRequiresLayout() const;
    bool GetIsViewportDirtyOrOnViewportDirtyPath() const;
    bool GetWantsViewportOrContributesToViewport() const;
    bool GetIsLayoutSuspended() const { return IsCollapsed(); }

    void RegisterAsScroller() { m_isScroller = true; }
    bool IsScroller() const { return m_isScroller; }

    bool IsScrollAnchorProvider(_In_ CUIElement* element);

    _Check_return_ HRESULT InitializeAnchorProviderParent();
    _Check_return_ HRESULT UpdateAnchorCandidateOnParentScrollProvider(bool add);

//-----------------------------------------------------------------------------
// LAYOUT Storage
//-----------------------------------------------------------------------------
public:
    CLayoutStorage* GetLayoutStorage()
    {
        return m_hasLayoutStorage? &m_layoutStorage : nullptr;
    }

    HRESULT EnsureLayoutStorage()
    {
        m_hasLayoutStorage = true;
        RRETURN(S_OK);//RRETURN_REMOVAL
    }

    _Check_return_ HRESULT static GetRenderSize(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

public:
    _Check_return_ virtual XFLOAT GetActualOffsetX()
    {
        return HasLayoutStorage() ? m_layoutStorage.m_offset.x : GetOffsetX();
    }

    _Check_return_ virtual XFLOAT GetActualOffsetY()
    {
        return HasLayoutStorage() ? m_layoutStorage.m_offset.y : GetOffsetY();
    }


    //-----------------------------------------------------------------------------
    // LAYOUT Methods
    //-----------------------------------------------------------------------------
public:

    // Holds information about a given viewport in a specific dimension
    // (i.e. horizontal or vertical).
    class UnidimensionalViewportInformation
    {
    public:
        UnidimensionalViewportInformation(float globalOffset, float length)
            : m_globalOffset(globalOffset)
            , m_length(length)
        {}

        float GetGlobalOffset() const { return m_globalOffset; }
        float GetLength() const { return m_length; }

    private:
        float m_globalOffset;
        float m_length;
    };

    class TransformToPreviousViewport
    {
    public:
        TransformToPreviousViewport(_In_ CUIElement* element, _In_ xref_ptr<CGeneralTransform> transform)
            : m_elementNoRef(element)
            , m_transform(transform)
        {}

        CUIElement* GetElement() const { return m_elementNoRef; }
        xref_ptr<CGeneralTransform> GetTransform() const { return m_transform; }

    private:
        CUIElement* m_elementNoRef;
        xref_ptr<CGeneralTransform> m_transform;
    };


public:
    _Check_return_ HRESULT Measure(XSIZEF availableSize);
    _Check_return_ HRESULT Arrange(XRECTF finalRect);

    _Check_return_ HRESULT EffectiveViewportWalk(
        const bool dirtyFound,
        _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports);

    virtual _Check_return_ HRESULT RecursiveInvalidateFontSize();
    virtual _Check_return_ HRESULT InvalidateFontSize();

    void InvalidateMeasure();
    virtual void RecursiveInvalidateMeasure();
    void InvalidateArrange();
    void InvalidateViewport();
    _Check_return_ HRESULT UpdateLayout();

    virtual void OnChildDesiredSizeChanged(_In_ CUIElement* pElement);
    XRECTF GetProperArrangeRect(XRECTF parentFinalRect);

    CDependencyObject* GetRootOfPopupSubTree();

    void BringIntoView();

    void BringIntoView(
        XRECTF rectangle,
        bool forceIntoView,
        bool useAnimation,
        bool skipDuringManipulation,
        double horizontalAlignmentRatio = DirectUI::DoubleUtil::NaN,
        double verticalAlignmentRatio = DirectUI::DoubleUtil::NaN,
        double horizontalOffset = 0.0,
        double verticalOffset = 0.0);

    virtual _Check_return_ CDependencyObject* GetLogicalParentNoRef() { return nullptr; }

    virtual _Check_return_ HRESULT UpdateLayoutClip(bool forceClipToRenderSize);

    // indicates whether the subtree should be taking its available size as a restriction to clip or not
    bool GetIsNonClippingSubtree() const
    {
        return m_isNonClippingSubtree;
    }

    void SetIsNonClippingSubtree(bool isNonClippingSubtree)
    {
        m_isNonClippingSubtree = isNonClippingSubtree;
    }

    // Plateau-aware layout rounding utilty.
    XFLOAT LayoutRound(_In_ XFLOAT value);
    _Check_return_ HRESULT LayoutRound(
        _In_ XFLOAT value,
        _Out_ XFLOAT* returnValue)
    {
        HRESULT hr = S_OK;
        *returnValue = LayoutRound(value);
        return hr;
    }

    float LayoutRoundCeiling(const float value);
    float LayoutRoundFloor(const float value);

protected:
    virtual _Check_return_ HRESULT MeasureCore(XSIZEF availableSize, _Out_ XSIZEF& desiredSize);
    virtual _Check_return_ HRESULT ArrangeCore(XRECTF finalRect);

    static XUINT32 IsSameSize(_In_ const XSIZEF& size1, _In_ const XSIZEF& size2);

    static XUINT32 XCP_FORCEINLINE IsSameRect(_In_ const XRECTF& r1, _In_ const XRECTF& r2)
    {
        return r1.X == r2.X && r1.Y == r2.Y && r1.Width == r2.Width && r1.Height == r2.Height;
    }

    virtual DirectUI::ManipulationModes GetManipulationModeCore() const;
    virtual _Check_return_ HRESULT SetManipulationModeCore(_In_ DirectUI::ManipulationModes value);

    FLOAT GetGlobalScaleFactorCore() const;
    _Check_return_ HRESULT SetGlobalScaleFactorCore(_In_ FLOAT value);

    virtual _Check_return_ HRESULT EffectiveViewportWalkCore(
        _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports,
        _Out_ bool& addedViewports);

    virtual _Check_return_ HRESULT EffectiveViewportWalkToChild(
        _In_ CUIElement* child,
        const bool dirtyFound,
        _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& horizontalViewports,
        _In_ std::vector<UnidimensionalViewportInformation>& verticalViewports);

private:
    typedef XINT32 (&RoundCeilOrFloorFn)(_In_ XDOUBLE x);
    float LayoutRoundHelper(const float value, _In_ RoundCeilOrFloorFn operationFn);

    bool RestoreDirtyPathFlagIfNeeded(const bool wasOnDirtyPath, const bool isMeasureNotArrange);

    _Check_return_ HRESULT MeasureInternal(XSIZEF availableSize);
    _Check_return_ HRESULT ArrangeInternal(XRECTF finalRect);

    _Check_return_ HRESULT TransformToGlobalCoordinateSpaceThroughViewports(
        _In_ const bool treatAsViewport,
        _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
        _Out_ XRECTF& rect);
    _Check_return_ HRESULT TransformToElementCoordinateSpaceThroughViewports(
        _In_ const std::vector<TransformToPreviousViewport>& transformsToViewports,
        _Inout_ XRECTF& rect);
    _Check_return_ HRESULT ComputeEffectiveViewportChangedEventArgsAndNotifyLayoutManager(
        _In_ std::vector<TransformToPreviousViewport>& transformsToViewports,
        _In_ const std::vector<UnidimensionalViewportInformation>& horizontalViewports,
        _In_ const std::vector<UnidimensionalViewportInformation>& verticalViewports);

    void InvalidateMeasureInternal(_In_opt_ CLayoutManager* layoutManager);
    void InvalidateArrangeInternal(_In_opt_ CLayoutManager* layoutManager);
    void InvalidateViewportInternal();
    void InvalidateAutomationPeerDataInternal();

    void PropagateOnMeasureDirtyPath();
    void PropagateOnArrangeDirtyPath(bool stopAtLayoutSuspended = false);
    void PropagateOnAutomationPeerDirtyPath();
    void PropagateOnViewportDirtyPath();
    void PropagateOnContributesToViewport();
    void PropagateOnPathInternal(XUINT32 pathFlag, XUINT32 dirtyFlag = 0, XUINT32 pendingFlag = 0, bool stopAtLayoutSuspended = false);

    void ResetLayoutInformation()
    {
        m_layoutStorage.ResetLayoutInformation();
    }

    void RaiseRequestBringIntoViewEvent(
        XRECTF rectangle,
        bool forceIntoView,
        bool useAnimation,
        bool skipDuringManipulation,
        double horizontalAlignmentRatio,
        double verticalAlignmentRatio,
        double horizontalOffset,
        double verticalOffset);

    void SetChildRenderOrderDirty();

public:
    static void NWSetVisibilityDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetTransformDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetProjectionDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetTransform3DDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetClipDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetOpacityDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetCompositeModeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetContentDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetContentAndBoundsDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetSubgraphDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetTransitionTargetDirty(_In_ CDependencyObject* pTarget, DirtyFlags flags);

    static void NWSetHasTransformAffectingAnimationStoppedDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetHasOpacityAnimationDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetHasCompositionNodeDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetRedirectionDataDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetHandOffVisualTransformDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags);

    static void NWSetLightTargetDirty(_In_ CDependencyObject* target, bool isLightTarget);
    static void NWSetLightCollectionDirty(_In_ CDependencyObject* target, DirtyFlags flags);

    void SetEntireSubtreeDirty();
    bool IsEntireSubtreeDirty() const { return !!m_isEntireSubtreeDirty; }

    void SetAndPropagateForceNoCulling(bool forceNoCulling);
    bool IsForceNoCulling() const;

    XSIZEF NWComputeElementSize();

    _Check_return_ HRESULT GetElementSizeForProjection(_Out_ XSIZEF *pElementSize);

    void NotifyParentChange(
        _In_ CDependencyObject *pNewParent,
        _In_ RENDERCHANGEDPFN pfnNewParentRenderChangedHandler
        ) final;

    static void NWComputeProjectionQuadPadding(
        XINT32 quadOffsetX,
        XINT32 quadOffsetY,
        XINT32 quadWidth,
        XINT32 quadHeight,
        _In_ const XSIZEF *pElementSize,
        _Out_ XRECTF_RB *pQuadPadding
        );

    bool NWNeedsRendering();

    //
    // Provided for CPopups which don't render with the rest of the tree during
    // the render walk
    //
    virtual bool NWSkipRendering()
    {
        return false;
    }

    float GetOpacityToRoot();

    _Check_return_ HRESULT GetRedirectionTransformsAndParentCompNode(
        _In_ CWindowRenderTarget *pRenderTarget,
        _Inout_ CMILMatrix *pTransformsToParentCompNode,
        bool allowClosedPopup,
        _Out_ CTransformToRoot *pTransformToRoot,
        _Out_ bool *pIsTransformToRootAnimating,
        _Out_ bool *pAreAllAncestorsVisible,
        _Outptr_result_maybenull_ HWCompTreeNode **ppNearestCompNode
        );

protected:
    _Check_return_ HRESULT NWGetProjectionTransformer(
        _Outptr_result_maybenull_ CPerspectiveTransformer **ppTransformer
        );

    // Virtual that allows for some 'leaf' elements to apply opacity directly to their edges instead of requiring
    // a rasterizer layer.  An element can only do this if it is drawing non-overlapping shapes, and has no children
    // that may overlap, because any overlap will blend incorrectly without a layer.  Use layers by default.
    virtual _Check_return_ bool NWCanApplyOpacityWithoutLayer() { return false; }

    void NWPropagateDirtyFlag(
        DirtyFlags flags
        ) override;

    void NWPropagateDirtyFlagForLayoutTransitions(DirtyFlags flags);

#if DBG
    void DbgCheckElementDirtyFlags(DirtyFlags flags);
#endif

    void NWSetDirtyFlagsAndPropagate(
        DirtyFlags flags,
        bool renderConditionFlag
        );

    void NWSetDirtyFlagsFromChildAndPropagate(
        DirtyFlags flags,
        bool renderConditionFlag
        );

    // Called whenever NWSetContentDirty is called on the element. Override for things like discarding alpha masks.
    virtual void OnContentDirty();

public:
    // If a RENDERCHANGEDHANDLER is added for a derived class, it should override
    // NWIsContentDirty, so that the flag the handler sets contributes to the UIE being dirty,
    // and NWCleanDirtyFlags, so that the UIE will clean the flag when rendering is complete.
    // Overrides of these two methods must always call into the base implementation.
    virtual bool NWIsContentDirty()
    {
        return m_fNWContentDirty;
    }

    void PropagateNWClean();

protected:
    virtual void EnsureContentRenderDataVirtual(
        RenderWalkType oldType,
        RenderWalkType newType
        );

    virtual void NWCleanDirtyFlags()
    {
        NWClean();
    }

    void EnsurePropertyRenderData(RenderWalkType walkType);
    _Check_return_ HRESULT EnsureContentRenderData(RenderWalkType walkType);

private:
    bool NWShouldAddEdges(_In_ const NWRenderParams &nwRP);

    void NWClean();

    // Called when the hand off or hand in visual changes.
    void OnHandOffOrHandInVisualDirty();

public:
    bool NWNeedsElementRendering();
    bool NWNeedsElementRenderingInnerProjection();
    bool NWNeedsSubgraphRendering() const;

private:
    bool PCArePropertiesDirty() const
    {
        return m_fNWTransformDirty
            || m_fNWProjectionDirty
            || m_isTransform3DDirty
            || m_fNWClipDirty
            || m_fNWLayoutClipDirty
            || m_fNWOpacityDirty;
    }

    bool PCHasOpacityAnimationDirty() const
    {
        return m_fNWHasOpacityAnimationDirty;
    }

    bool PCHasCompositionNodeDirty() const
    {
        return m_fNWHasCompNodeDirty;
    }

    bool PCIsCompositeModeDirty() const
    {
        return m_fNWCompositeModeDirty;
    }

    bool PCIsRedirectionDataDirty() const
    {
        return m_fNWRedirectionDataDirty;
    }

    bool IsTransformDirty() const
    {
        return !!m_fNWTransformDirty;
    }

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT Print(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams
        );

    virtual _Check_return_ HRESULT PreChildrenPrintVirtual(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams
        );

protected:
    virtual _Check_return_ HRESULT PrintChildren(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams
        );

private:
    _Check_return_ HRESULT PrintTransformed(
        _In_ const SharedRenderParams& sharedPrintParams,
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams
        );

    _Check_return_ HRESULT PrintPushUIEClip(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        _Out_ bool* pushedUIEClip
        );

    _Check_return_ HRESULT PrintPushLayoutClip(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams& printParams,
        _Out_ bool* pushedLayoutClip
        );

    static _Check_return_ HRESULT PrintPopClip(
        _In_ const D2DRenderParams &printParams
        );

    _Check_return_ HRESULT PrintPushOpacity(
        _In_ const D2DRenderParams &printParams
        );

    _Check_return_ HRESULT PrintPopOpacity(
        _In_ const D2DRenderParams &printParams
        );

//-----------------------------------------------------------------------------
// DirectManipulation Methods
//-----------------------------------------------------------------------------
protected:
    // CreateDirectManipulationContainerHandler is virtual so that Core controls for instance can instantiate a IDirectManipulationContainerHandler
    // implementation that does not do p-invokes.
    _Check_return_ HRESULT CreateDirectManipulationContainerHandler(
        _Outptr_ IDirectManipulationContainerHandler** ppDirectManipulationContainerHandler);

    // Called for framework elements, when their horizontal or vertical alignment has changed.
    _Check_return_ HRESULT OnAlignmentChanged(
        _In_ bool fIsForHorizontalAlignment,
        _In_ bool fIsForStretchAlignment,
        _In_ bool fIsStretchAlignmentTreatedAsNear);

public:
    // CreateDirectManipulationContainer is virtual so that Core controls for instance can instantiate a CUIDMContainer.
    // implementation that does not do reverse-p-invokes.
    _Check_return_ HRESULT CreateDirectManipulationContainer();

    // Returns a previously created CUIDMContainer implementation for this CUIElement, or NULL if none was created.
    _Check_return_ HRESULT GetDirectManipulationContainer(_Outptr_ CUIDMContainer** directManipulationContainer);

    // Retrieve the DManip transform directly from DManip Compositor
    _Check_return_ HRESULT GetDirectManipulationCompositorTransform(
        _In_ TransformRetrievalOptions transformRetrievalOptions,
        _Out_ BOOL& fTransformSet,
        _Out_ FLOAT& translationX,
        _Out_ FLOAT& translationY,
        _Out_ FLOAT& uncompressedZoomFactor,
        _Out_ FLOAT& zoomFactorX,
        _Out_ FLOAT& zoomFactorY);

    // Retrieve the content clip's DManip transform
    _Check_return_ HRESULT GetDirectManipulationClipContentTransform(
        _In_ IPALDirectManipulationService* pDirectManipulationService,
        _In_ IObject* pCompositorClipContent,
        _Out_ FLOAT& translationX,
        _Out_ FLOAT& translationY,
        _Out_ FLOAT& zoomFactorX,
        _Out_ FLOAT& zoomFactorY);

    // Returns True when this element is:
    // - declared manipulatable for DirectManipulation
    // - is the primary content of a viewport
    _Check_return_ HRESULT IsManipulatablePrimaryContent(
        _Out_ bool* pIsManipulatablePrimaryContent);

    // Returns True if this element is manipulatable by DirectManipulation and is a primary content.
    // It is then aligned by DirectManipulation as opposed to our layout engine.
    _Check_return_ HRESULT IsAlignedByDirectManipulation(
        _Out_ bool* pIsAlignedByDirectManipulation);

    // If this CUIElement is manipulatable, returns the DManip service and content for this element, otherwise nullptrs.
    _Check_return_ HRESULT GetDirectManipulationServiceAndContent(
        _Outptr_result_maybenull_ IPALDirectManipulationService** ppDMService,
        _Outptr_result_maybenull_ IObject** ppCompositorContent,
        _Outptr_result_maybenull_ IObject** ppCompositorClipContent,
        _Out_ XDMContentType* pDMContentType,
        _Out_ float* pContentOffsetX,
        _Out_ float* pContentOffsetY);

    void ResetCompositionNodeManipulationData();
    bool HasSharedManipulationTransform(bool targetsClip);
    void PrepareForSecondaryCurveUpdate(bool targetsClip);
    IUnknown* GetSharedPrimaryContentTransform() const;
    float GetDirectManipulationContentOffsetX() const;
    float GetDirectManipulationContentOffsetY() const;

    void SetIsDirectManipulationCrossSlideContainer(bool isDMCrossSlideContainer)
    {
        m_fIsDirectManipulationCrossSlideContainer = isDMCrossSlideContainer;
    }

    bool GetIsDirectManipulationCrossSlideContainer() const
    {
        return m_fIsDirectManipulationCrossSlideContainer;
    }

    void SetIsDirectManipulationContainer(bool isDMContainer)
    {
        m_fIsDirectManipulationContainer = isDMContainer;
    }

    bool GetIsDirectManipulationContainer() const
    {
        return m_fIsDirectManipulationContainer;
    }

    // The DirectManipulationManager for a DMContainer can only occur after GetElementIslandInputSite returns a valid
    // IslandInputSite since it is being used for DManip's initialization.
    bool CanDMContainerInitialize()
    {
        wrl::ComPtr<ixp::IIslandInputSitePartner> islandInputSite = GetElementIslandInputSite();
        return (nullptr != islandInputSite);
    }

public:
    void GetCumulativeTransform(
        _Inout_ CMILMatrix *pLocalTransform,
        _Inout_ const CMILMatrix **ppCumulativeTransform
        );

public:
    static _Check_return_ HRESULT D2DSetUpClipHelper(
        _In_ IPALAcceleratedRenderTarget *pD2DRenderTarget,
        bool pushAxisAlignedClip,
        _In_opt_ IPALAcceleratedGeometry *pAcceleratedClipGeometry,
        _In_opt_ const XRECTF_RB *pContentBounds,
        _Inout_ bool *pPushedClipLayer,
        _Inout_ bool *pPushedAxisAlignedClip,
        _Out_ bool *pIsClipEmpty
        );

    static _Check_return_ HRESULT D2DPopClipHelper(
        _In_ IPALAcceleratedRenderTarget *pD2DRenderTarget,
        bool pushedShapeClipLayer,
        bool pushedAxisAlignedShapeClip
        );

//-----------------------------------------------------------------------------
// LayoutTransition  methods
//-----------------------------------------------------------------------------
public:
    CREATE_GROUP_FN static EnsureLayoutTransitionStorage;
    XCP_FORCEINLINE bool HasLayoutTransitionStorage() const { return m_pLayoutTransitionStorage != nullptr; }
    LayoutTransitionStorage* GetLayoutTransitionStorage() { return m_pLayoutTransitionStorage; }
    _Check_return_ HRESULT DeleteLayoutTransitionStorage();
    virtual CTransitionCollection* GetTransitionsForChildElementNoAddRef(_In_ CUIElement* pChild) { return NULL; }

    _Check_return_ XCP_FORCEINLINE XUINT32 GetIsLayoutTransitionDirty() const { return GetLayoutFlagsAnd(LF_LAYOUTTRANSITION_DIRTY); }
    XCP_FORCEINLINE void SetIsLayoutTransitionDirty(_In_ XUINT32 value) { SetLayoutFlags(LF_LAYOUTTRANSITION_DIRTY, value); }
    _Check_return_ XCP_FORCEINLINE XUINT32 GetIsLayoutTransitionDirtyPending() const { return GetLayoutFlagsAnd(LF_LAYOUTTRANSITION_DIRTY_PENDING); }
    XCP_FORCEINLINE void SetIsLayoutTransitionDirtyPending(_In_ XUINT32 value) { SetLayoutFlags(LF_LAYOUTTRANSITION_DIRTY_PENDING, value); }
    _Check_return_ XCP_FORCEINLINE XUINT32 GetIsOnLayoutTransitionDirtyPath() const { return GetLayoutFlagsAnd(LF_ON_LAYOUTTRANSITION_DIRTY_PATH); }
    XCP_FORCEINLINE void SetIsOnLayoutTransitionDirtyPath(_In_ XUINT32 value) { SetLayoutFlags(LF_ON_LAYOUTTRANSITION_DIRTY_PATH, value); }
    _Check_return_ XCP_FORCEINLINE XUINT32 GetIsOnLayoutTransitionStack() const { return GetLayoutFlagsAnd(LF_ON_LAYOUTTRANSITION_STACK); }
    XCP_FORCEINLINE void SetIsOnLayoutTransitionStack(_In_ XUINT32 value) { SetLayoutFlags(LF_ON_LAYOUTTRANSITION_STACK, value); }
    _Check_return_ XCP_FORCEINLINE XUINT32 GetRequiresLayoutTransition() const { return GetLayoutFlagsOr(LF_LAYOUTTRANSITION_DIRTY | LF_ON_LAYOUTTRANSITION_DIRTY_PATH); }

    _Check_return_ HRESULT TransitionLayout(_In_ CUIElement *pRoot, _In_ XRECTF rootRect);

    _Check_return_ HRESULT OnAssociationFailure();

    // Whether this element should skip rendering when it is the target of a layout transition.
    virtual bool SkipRenderForLayoutTransition() { return false; }

//-----------------------------------------------------------------------------
// TouchInteraction Methods
//-----------------------------------------------------------------------------
public:
    bool IsInteractionEngineRequired(_In_ bool ignoreActiveState) const;

//-----------------------------------------------------------------------------
// PC walk methods/data
//-----------------------------------------------------------------------------

    bool HasIndependentAnimationsOrManipulations() const;

    bool RequiresCompositionNode() const;

    bool IsTransformOrOffsetAffectingPropertyIndependentlyAnimating() const;

    bool IsOffsetIndependentlyAnimating() const;

    // Normally this is the same as IsTransformOrOffsetAffectingPropertyIndependentlyAnimating, except WUC splits
    // offset animations apart from RenderTransform animations. In WUC mode, having only an offset animation means
    // we don't need a render transform expression.
    bool IsTransformAffectingPropertyIndependentlyAnimating() const;

    bool IsTransformIndependentlyAnimating() const;

    bool IsProjectionIndependentlyAnimating() const;

    bool IsTransform3DIndependentlyAnimating() const;

    bool IsOpacityIndependentlyAnimating() const;

    bool IsLocalOpacityIndependentlyAnimating() const;

    bool IsTransitionOpacityIndependentlyAnimating() const;

    bool IsClipIndependentlyAnimating() const;

    bool IsLocalClipIndependentlyAnimating() const;

    bool IsTransitionClipIndependentlyAnimating() const;

    bool IsBrushIndependentlyAnimating() const;

    // Returns whether there are brush transitions currently playing. This is checked alongside
    // IsBrushIndependentlyAnimating for rendering optimizations. Animating transparent elements must still render a
    // mask, for example.
    bool HasActiveBrushTransitions() const;

    bool IsManipulatable() const;

    bool IsManipulatedIndependently() const;

    bool IsClipManipulatedIndependently() const;

    bool IsRenderWalkRoot() const;

    bool IsRedirectionElement() const;

    bool HasComplexTransformation() const;

    bool HasAxisUnalignedTransform() const;

    bool HasAxisUnalignedLocalClip() const;

    bool HasSwapChainContent() const;

    bool IsUsingHandOffVisual() const;

    bool IsUsingHandInVisual() const;

    bool IsUsingCompositeMode() const;

    bool RequiresComposition() const;

    bool HasActiveConnectedAnimation() const;

    bool IsRenderTargetSource() const;

    bool RequiresCompNodeForRoundedCorners() const
    {
        return m_requiresCompNodeForRoundedCorners;
    }

    bool RequiresViewportInteraction();

    bool HasFacadeAnimation() const
    {
        return m_hasFacadeAnimation;
    }

    void UpdateHasTranslateZ(const wfn::Vector3& translation);

    bool HasTranslateZ() const
    {
        return m_hasTranslateZ;
    }

    void UpdateHasNonZeroRotation(float rotation);

    bool HasNonZeroRotation() const
    {
        return m_hasNonZeroRotation;
    }

    void UpdateHasScaleZ(const wfn::Vector3& scale);

    bool HasScaleZ() const
    {
        return m_hasScaleZ;
    }

    void UpdateHasNonIdentityTransformMatrix(const wfn::Matrix4x4& transformMatrix);

    bool HasNonIdentityTransformMatrix() const
    {
        return m_hasNonIdentityTransformMatrix;
    }

    void UpdateHasNonZeroCenterPoint(const wfn::Vector3& centerPoint);

    bool HasNonZeroCenterPoint() const
    {
        return m_hasNonZeroCenterPoint;
    }

    void UpdateHasNonDefaultRotationAxis(const wfn::Vector3& rotationAxis);

    bool HasNonDefaultRotationAxis() const
    {
        return m_hasNonDefaultRotationAxis;
    }

    bool IsShadowCaster() const
    {
        return m_isShadowCaster;
    }

    bool IsProjectedShadowDefaultReceiver() const
    {
        return m_isProjectedShadowDefaultReceiver;
    }

    bool IsProjectedShadowCustomReceiver() const
    {
        return m_isProjectedShadowCustomReceiver;
    }

    void SetShouldFadeInDropShadow(bool shouldFadeInDropShadow)
    {
        m_shouldFadeInDropShadow = shouldFadeInDropShadow;
    }

    bool ShouldFadeInDropShadow() const
    {
        return m_shouldFadeInDropShadow;
    }

    bool IsTransitionRootWithChildren() const
    {
        return m_isTransitionRootWithChildren;
    }

    bool AreFacadesInUse() const
    {
        // TODO - replace with strict mode check when it becomes available
        return m_areFacadesInUse;
    }

    FacadeTransformInfo GetFacadeTransformInfo(bool preferAnimatingValue) const;

    // Returns whether the Transform3D property has a non-null value. Note that this doesn't actually relate to whether the
    // UIElement has a 3D depth. The Transform3D could be set to a CompositeTransform3D with no Z components, in which case
    // the element had depth even though it has a Transform3D set. Or the UIElement could have a matrix set in WUC via a
    // hand off visual or a facade property, in which case the UIElement has depth even though it has no Transform3D set.
    bool HasTransform3DCompositionRequirement() const { return m_hasTransform3D; };

    bool RequiresHitTestInvisibleCompNode() const;

    void SetRequiresHitTestInvisibleCompNode(bool value);

    void UpdateHitTestVisibilityForComposition();

    bool IsRedirectedChildOfSwapChainPanel() const;

    void SetIsRedirectedChildOfSwapChainPanel(bool value);

    bool HasAttachedInteractions() const
    {
        return m_hasAttachedInteractions;
    }

    void SetHasAttachedInteractions(bool hasAttachedInteractions)
    {
        m_hasAttachedInteractions = hasAttachedInteractions;
    }

    // A UIElement can be a receiver for multiple ThemeShadows, and it needs a comp node as long as there is at least one
    // ThemeShadow that's using it in the Receivers collection. Keep a count in sparse storage.
    void AddedAsShadowReceiver();
    void RemovedAsShadowReceiver();

    bool HasWUCOpacityAnimation() const { return m_hasWUCOpacityAnimation; }
    void SetHasWUCOpacityAnimation(bool value) { m_hasWUCOpacityAnimation = value; }

    _Check_return_ HRESULT SetRequiresComposition(
        CompositionRequirement compositionReason,
        IndependentAnimationType detailedAnimationReason
        ) noexcept final;

    void UnsetRequiresComposition(
        CompositionRequirement compositionReason,
        IndependentAnimationType detailedAnimationReason
        ) noexcept final;

    _Check_return_ HRESULT EnsureCompositionPeer(
        _In_ HWCompTreeNode *pCompositionPeer,
        _In_opt_ HWCompTreeNode *pParentNode,
        _In_ HWCompNode *pPreviousSibling,
        _In_opt_ WUComp::IVisual* previousSiblingVisual
        );

    static _Check_return_ HRESULT EnsureCompositionPeer(
        _In_ DCompTreeHost* dcompTreeHost,
        _In_ HWCompTreeNode *pCompositionPeer,
        _In_opt_ HWCompTreeNode *pParentNode,
        _In_ HWCompNode *pPreviousSibling,
        // When we're doing synchronous comp tree updates, there are no render data comp nodes, and sprite visuals
        // are added directly to the comp node children. That means the reference WUC visual might be a sprite visual
        // from a UIElement, or it might be an interop visual from a comp node. It's specified here, and it's used
        // instead of pPreviousSibling if it's provided.
        _In_opt_ WUComp::IVisual* previousSiblingVisual,
        _In_opt_ CUIElement *pElement,
        _Inout_ HWCompTreeNode **ppTargetCompositionPeer
        );

    _Check_return_ HRESULT CreateTemporaryCompositionPeer(
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ HWCompTreeNode *pParentNode,
        _In_ const CMILMatrix *pPrependMatrix,
        _Outptr_ HWCompTreeNode **ppCompNode
        );

    void RemoveCompositionPeer();

    HWCompTreeNode* GetCompositionPeer() const { return m_pCompositionPeer; }

    _Ret_maybenull_ HWRealization *GetHWRealizationCache();

    virtual void SetHWRealizationCache(_In_opt_ HWRealization *pNewRenderingCache);

    virtual void ClearPCRenderData();
    _Check_return_ HRESULT GetPCPreChildrenRenderDataNoRef(_Outptr_ PCRenderDataList **ppRenderData);
    _Check_return_ HRESULT GetPCPostChildrenRenderDataNoRef(_Outptr_ PCRenderDataList **ppRenderData);

    virtual void GetIndependentlyAnimatedBrushes(
        _Outptr_result_maybenull_ CSolidColorBrush **ppFillBrush,
        _Outptr_result_maybenull_ CSolidColorBrush **ppStrokeBrush
        );

    void EnterPCScene();
    void ReEnterPCScene();
    bool IsInPCScene() const;
    bool IsInPCScene_IncludingDeviceLost() const;
    void LeavePCSceneRecursive();
    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;
    void ReleaseDCompResources() final;

    void StoreLastSpriteVisual(_In_opt_ WUComp::IVisual* spriteVisual);
    WUComp::IVisual* GetLastSpriteVisual();

    void StoreLastCompNode(_In_opt_ HWCompNode *pCompNode);
    HWCompNode* GetLastCompNode();

    void SetWasRedirectedTransformAnimating(bool wasRedirectedTransformAnimating);
    bool GetWasRedirectedTransformAnimating();

    bool UpdateCompositionPeerTransformToRoot(_In_ const CTransformToRoot *pTransformToRoot);

    bool ShouldDisablePixelSnapping();

    bool HasHitTestVisibleVisuals() const;

private:
    virtual void LeavePCSceneSubgraph();

    static bool RenderDataListHasHitTestVisibleVisuals(const PCRenderDataList* renderDataList);

    bool OnSetShadowValue(_In_ const SetValueParams& args);


    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT GetInnerBounds(
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT GetInnerBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT GetContentInnerBounds(
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ HRESULT GetOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        );

    // See: /design-notes/OneCoreTransforms.md for more information about this function's coordinate space
    _Check_return_ HRESULT GetGlobalBounds(_Out_ XRECTF_RB* pBounds);

    _Check_return_ HRESULT GetGlobalBounds(_Out_ XRECTF_RB* pBounds, const bool ignoreClipping);

    _Check_return_ HRESULT GetGlobalBoundsWithOptions(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping,
        // The ScrollContentPresenter in the ScrollViewers has a viewport-sized clip on it. This flag ignores that clip
        // and returns non-zero bounds for elements that are scrolled off screen. This is used for GetGlobalBounds for UIA.
        const bool ignoreClippingOnScrollContentPresenters,
        // Use target values for animations (e.g. manipulations/LTEs) instead of current values
        _In_ bool useTargetInformation
        );

    _Check_return_ HRESULT GetGlobalBoundsLogical(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping = false,
        _In_ bool useTargetInformation = false
        );

    virtual _Check_return_ HRESULT GetTightGlobalBounds(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping = false,
        _In_ bool useTargetInformation = false
        );

    virtual _Check_return_ HRESULT GetTightInnerBounds(
        _Out_ XRECTF_RB* pBounds
        );

    bool AreBoundsDirty();

    template <typename HitType>
    _Check_return_ HRESULT BoundsTestEntry(
        const HitTestParams& hitTestParams,
        _In_ const HitType& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements
        );

    template <typename HitType>
    _Check_return_ HRESULT HitTestLocal(
        _In_ const HitType& target,
        _Out_ bool* pHit
        );

    template <typename HitType>
    _Check_return_ HRESULT HitTestLocalPostChildren(
        _In_ const HitType& target,
        _Out_ bool* pHit
        );

    _Check_return_ HRESULT ComputeVisibleBoundsOfRect(
        _In_ const XRECTF_RB* pRect,
        _In_ const XRECTF_RB* pWindowBounds,
        _Out_writes_all_(numPoints) XPOINTF* pPoints,
        UINT numPoints
        );

    _Check_return_ HRESULT CacheVisibleBounds(
        _In_ const XRECTF_RB *pWindowBounds
        );

    _Check_return_ HRESULT GetVisibleImageBrushBounds(
         _In_ CImageBrush *pImageBrush,
         _In_ const XRECTF_RB *pWindowBounds,
         _Out_ XRECTF *pBounds
         );

    static _Check_return_ HRESULT TransformOuterToInnerChain(
        _In_ CUIElement* pDescendantElement,
        _In_opt_ const CUIElement* pAncestorElement,
        _In_ const XPOINTF* pOuterPoints,
        _Out_ XPOINTF* pInnerPoints,
        XUINT32 numPoints,
        _Out_ bool* pTransformedPoints);

    static _Check_return_ HRESULT TransformInnerToOuterChain(
        _In_ CUIElement* pDescendantElement,
        _In_opt_ const CUIElement* pAncestorElement,
        _In_ const XRECTF_RB* pInnerRect,
        _Out_ XRECTF_RB* pOuterRect
        );

    // See: /design-notes/OneCoreTransforms.md for more information about this function's coordinate space
    _Check_return_ HRESULT TransformToWorldSpace(
        _In_ const XRECTF_RB* localSpaceRect,
        _Out_ XRECTF_RB* worldSpaceRect,
        bool ignoreClipping,
        // The ScrollContentPresenter in the ScrollViewers has a viewport-sized clip on it. This flag ignores that clip
        // and returns non-zero bounds for elements that are scrolled off screen. This is used for GetGlobalBounds for UIA.
        const bool ignoreClippingOnScrollContentPresenters,
        bool useTargetInformation);
    XRECTF_RB TransformToWorldSpace(_In_ const XRECTF_RB* localSpaceRect);

    // Exposed because Popup doesn't get its bounds invalidated correctly.
    void InvalidateChildBounds();

    virtual _Check_return_ HRESULT BoundsTestInternal(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    virtual _Check_return_ HRESULT BoundsTestInternal(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    template <typename HitType>
    _Check_return_ HRESULT BoundsTestContentAndChildren(
        _In_ const HitType& testTarget,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    _Check_return_ HRESULT IsReadyForRenderTargetElementRender(
        _Out_ BOOLEAN * pIsReadyForRenderTargetElementRender);

    bool IsCanvasLeftAnimationDirty() const { return m_isCanvasLeftAnimationDirty; }
    bool IsCanvasTopAnimationDirty() const { return m_isCanvasTopAnimationDirty; }

    void SetIsCanvasLeftAnimationDirty() { m_isCanvasLeftAnimationDirty = true; }
    void SetIsCanvasTopAnimationDirty() { m_isCanvasTopAnimationDirty = true; }

    bool IsOpacityAnimationDirty() const { return m_isOpacityAnimationDirty; }
    void SetIsOpacityAnimationDirty(bool value) { m_isOpacityAnimationDirty = value; }

    void GetViewportInteraction(
        _In_ DCompTreeHost * dcompTreeHost,
        _Out_ IInspectable** interaction);
    static bool IsInTransform3DSubtree(
        _In_ CUIElement* pDescendantElement,
        _In_opt_ const CUIElement* pAncestorElement
        );

    _Check_return_ HRESULT SetHandOffVisualOffset(float x, float y);

    _Check_return_ HRESULT SetHandOffVisualScale(float scaleX, float scaleY);

    // Note: This assumes that the rotation axis is the Z axis. Xaml doesn't handle the case of arbitrary rotations.
    _Check_return_ HRESULT SetHandOffVisualRotationAngle(float angle);

    _Check_return_ HRESULT SetHandOffVisualCenterPoint(float centerX, float centerY);

    void SetHandOffVisualTransformMatrix(_In_ const wfn::Matrix4x4& value);

    void SetHandOffVisualClip(_In_ const XRECTF& clip);
    void ClearHandOffVisualClip();
    void SetHandOffVisualClipTransform(_In_ const wfn::Matrix3x2& transform);

    void DetachWUCPropertyListenerFromWUCClip();

    bool CanUseHandOffVisualTransformGroupForLocalTransform() const;

    bool Has3DDepth() const { return m_has3DDepth; }
    void SetHas3DDepth(bool value);
    void UpdateHas3DDepth();

    bool Has3DDepthInSubtree() const { return m_has3DDepthInSubtree; }
    void UpdateHas3DDepthInSubtree();
    bool ComputeDepthInSubtree();   // Iterates over the children to see if m_has3DDepthInSubtree should be marked

    bool Has3DDepthOnSelfOrSubtree() const { return m_has3DDepth || m_has3DDepthInSubtree; }
    bool Has3DDepthOnSelfOrSubtreeOrLTETargetingSelf() const;

    void PropagateDepthInSubtree();

    // In addition to the Transform3D property, we also have to consider 3D transforms set directly on the hand off visual.
    // This method is optimized with a cached flag because it's frequently called during hit testing.
    // DEAD_CODE_REMOVAL - will need to convert some places to new hit test code path first
    bool HasTransform3DForHitTestingLegacy() const;
    // DEAD_CODE_REMOVAL - will need to convert some places to new hit test code path first
    bool HasDepthLegacy() const;    // Returns true iff this element itself has 3D depth. This method computes rather than looks up a flag.

    bool NeedsWUCOffsetExpression();
    void EnsureWUCOffsetExpression(_Inout_ WinRTExpressionConversionContext* context);
    void ClearWUCOffsetExpression();

    // The opacity expression exists only because we need a place to attach WUC opacity animations on collapsed elements. These
    // elements won't have WUC visuals to attach the animations to, so we create an expression and attach to that. The alternative
    // is to create a WUC visual for the collapsed element and attach the animation, but then we'll have to manage the lifetime
    // of that visual, including hooking it up with the UIE's comp node after the comp node is created and deleting it after
    // the opacity animation ends. Hand off visuals also pre-create WUC visuals, so we have to make sure that opacity and
    // hand off play well with each other. Right now it's simpler to add an expression.
    //
    // TODO_WinRT: For RS2 we can pre-create WUC visuals (and comp nodes) whenever a comp node will be needed, which means handoff
    // and opacity will no longer be special cases.
    bool NeedsWUCOpacityExpression();
    void EnsureWUCOpacityExpression(_Inout_ WinRTExpressionConversionContext* context);
    void ClearWUCOpacityExpression();

    bool NeedsWUCTransitionOpacityExpression();

    void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context) override;

    //
    // There are 4 combinations:
    //
    // Not marked with IA, no WUC animation
    //    - This is the common case for something that isn't animated.
    //    - In this case we don't need to put a WUC expression in the tree. We can release an expression if there was one.
    //
    // Marked with IA, has a WUC animation
    //    - This is the common case for something that is animated.
    //    - In this case we need to put a WUC expression in the tree.
    //
    // Not marked with IA, but has a WUC animation
    //    - This case can come up when an independent animation is paused. Xaml unmarks the target for an indepedent animation,
    //      but we have to keep the WUC animation around. Releasing the WUC animation would destroy its current state, which is
    //      needed when the animation is resumed.
    //    - In this case we don't need to put a WUC expression in the tree, but we still have to hold on to it.
    //
    // Marked with IA, but no WUC animation
    //    - This case can come up because of a long existing bug - see MSFT: 8667408 <Xaml objects can be marked as having an
    //      independent animation when they don't anymore>
    //    - In this case we don't need to put a WUC expression in the tree. We can release an expression if there was one.
    //
    bool MustAttachWUCExpression(const KnownPropertyIndex propertyIndex) const;
    bool CanReleaseWUCExpression(const KnownPropertyIndex propertyIndex) const;

    bool GetAllowsDragAndDropPassThrough() const { return m_allowsDragAndDropPassThrough; }
    void SetAllowsDragAndDropPassThrough(bool allowsDragAndDropPassThrough) { m_allowsDragAndDropPassThrough = allowsDragAndDropPassThrough; }

    bool GetIsHitTestingSuppressed() const { return m_isHitTestingSuppressed; }
    void SetIsHitTestingSuppressed(bool isHitTestingSuppressed) { m_isHitTestingSuppressed = isHitTestingSuppressed; }

protected:
    _Check_return_ HRESULT EnsureInnerBounds(_In_opt_ HitTestParams *hitTestParams);

    _Check_return_ virtual HRESULT GenerateContentBounds(
        _Out_ XRECTF_RB* pBounds
        );

    _Check_return_ virtual HRESULT GenerateChildOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        );

    virtual _Check_return_ HRESULT BoundsTestChildren(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    virtual _Check_return_ HRESULT BoundsTestChildren(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    bool TransformOuterToInner(
        _In_reads_(numPoints) const XPOINTF* pOuterPoints,
        _Out_writes_(numPoints) XPOINTF* pInnerPoints,
        XUINT32 numPoints);

    _Check_return_ HRESULT TransformInnerToOuter(
        _In_ const XRECTF_RB* pInnerBounds,
        _Out_ XRECTF_RB* pOuterBounds,
        _In_ bool ignoreClipping);

    _Check_return_ virtual HRESULT HitTestLocalInternal(
        _In_ const XPOINTF& target,
        _Out_ bool* pHit
        );

    _Check_return_ virtual HRESULT HitTestLocalInternal(
        _In_ const HitTestPolygon& target,
        _Out_ bool* pHit
        );

public:
    _Check_return_ virtual HRESULT GetBoundsForImageBrushVirtual(
        _In_ const CImageBrush *pImageBrush,
        _Out_ XRECTF *pBounds
        );

    _Check_return_ HRESULT ComputeRealizationTransform(_Out_ CTransformToRoot *pRealizationTransform);

    bool IsPointerPositionOver(_In_ XPOINTF pointPosition);

    // Returns whether another element is an ancestor of this one. This differs from CDependencyObject::IsAncestorOf
    // because this knows how to walk from the contents of a popup back to the popup and continue through the popup's
    // ancestor chain. This also checks for the unloading child of a popup.
    bool HasAncestorIncludingPopups(_In_ const CUIElement* const other);

    template <typename HitType> bool PrepareHitTestParamsStartingHere(HitTestParams& hitTestParams, HitType& transformedTarget);

    // Used for walks up the tree. If there's an LTE targeting this element, then return the first LTE. Otherwise, return the
    // parent element.
    // Note: does not correctly handle the case of having multiple LTEs. In that case we should walk up each LTE. Instead we'll
    // only walk up the first.
    CUIElement* GetParentOrLTEForWalkUp();
    CUIElement* GetFirstLTETargetingThis() const;

    void SetShadowVisual(_In_opt_ WUComp::IVisual* shadowVisual);
    WUComp::IVisual* GetShadowVisualNoRef();

    // Register and unregister change handlers for simple properties like Rotation, Opacity, Translation, etc.
    // These ensure the UI element visually updates when simple properties are set, and are called on DXamlCore init/deinit
    static void RegisterSimplePropertyCallbacks();
    static void UnregisterSimplePropertyCallbacks();

private:
    void FillAncestorChainForTransformToRoot(Jupiter::stack_vector<CUIElement*, 32>& ancestorChain, bool includeThisElement, bool useTargetInformation);

    _Check_return_ HRESULT GetGlobalBoundsOnSelf(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping,
        // The ScrollContentPresenter in the ScrollViewers has a viewport-sized clip on it. This flag ignores that clip
        // and returns non-zero bounds for elements that are scrolled off screen. This is used for GetGlobalBounds for UIA.
        const bool ignoreClippingOnScrollContentPresenters,
        _In_ bool useTargetInformation);

    // Under the new hit testing code path, elements with 3D depth don't contribute to the outer bounds of their parent
    // elements, and won't be automatically included when getting global bounds. Explicitly walk down the 3D branches
    // and get their global bounds. Does not consider this element's own bounds.
    _Check_return_ HRESULT GetGlobalBoundsIn3DDescendants(
        _Out_ XRECTF_RB* pBounds,
        _In_ bool ignoreClipping,
        // The ScrollContentPresenter in the ScrollViewers has a viewport-sized clip on it. This flag ignores that clip
        // and returns non-zero bounds for elements that are scrolled off screen. This is used for GetGlobalBounds for UIA.
        const bool ignoreClippingOnScrollContentPresenters,
        _In_ bool useTargetInformation);

    static _Check_return_ HRESULT TransformInnerToOuterChain(
        _In_ CUIElement* pDescendantElement,
        _In_opt_ const CUIElement* pAncestorElement,
        _In_reads_(numPoints) const XPOINTF *pOuterPoints,
        _Out_writes_(numPoints) XPOINTF* pInnerPoints,
        XUINT32 numPoints
        );

    static CUIElement* TransformInnerToOuterChainThrough3DSubtree(
        _Inout_ CUIElement* pDescendantElement,
        _In_opt_ const CUIElement* pAncestorElement,
        _In_ const XRECTF_RB* pInnerRect,
        _Out_ XRECTF_RB* pOuterRect
        );

    static CUIElement* GatherTransformsUpTreeAndMoveToPerspectiveAncestor(
        _In_ CUIElement* pDescendantElement,
        _In_opt_ const CUIElement* pAncestorElement,
        _Inout_ HitTestParams* hitTestParams
        );

    template <typename HitType>
    _Check_return_ HRESULT BoundsTestInternalImpl(
        _In_ const HitType& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    template <typename HitType>
    _Check_return_ HRESULT BoundsTestChildrenImpl(
        _In_ const HitType& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );

    _Check_return_ HRESULT ApplyUIEClipToBounds(
        _Inout_ XRECTF_RB *bounds
        );

    _Check_return_ HRESULT ApplyLayoutClipToBounds(
        _Inout_ XRECTF_RB *bounds
        );

    bool AreInnerBoundsDirty();

    bool AreOuterBoundsDirty();

    bool AreContentInnerBoundsDirty();

    bool AreChildBoundsDirty();

    void CleanOuterBounds();

    void CleanContentInnerBounds();

    void CleanChildBounds();

    void CleanInnerBounds();

    void InvalidateElementBounds();

    _Check_return_ HRESULT EnsureOuterBounds(_In_opt_ HitTestParams *hitTestParams);

    _Check_return_ HRESULT EnsureContentInnerBounds(_In_opt_ HitTestParams *hitTestParams);

    _Check_return_ HRESULT EnsureChildBounds(_In_opt_ HitTestParams *hitTestParams);

    bool HasTransform3DInSubtree(_In_opt_ const HitTestParams *hitTestParams) const;

    void ForceCloseHandInAndHandOffVisuals();

    template <typename HitType> bool CollectTransformsAndTransformToInner(HitTestParams& hitTestParams, HitType& transformedTarget);
    bool TransformToOuter2D(const HitTestParams* hitTestParams, XRECTF_RB& bounds);
    void TransformToOuter(HitTestPolygon& polygon, TransformRetrievalOptions transformRetrievalOptions, bool ignoreClipping);

    bool TransformWorldToLocal3D(const HitTestParams& hitTestParams, XPOINTF& localSpacePoint) const;
    bool TransformWorldToLocal3D(const HitTestParams& hitTestParams, HitTestPolygon& localSpacePolygon) const;

    void TransformTargetThroughMatrix(const CMILMatrix& matrix, XPOINTF& point) const;
    void TransformTargetThroughMatrix(const CMILMatrix& matrix, HitTestPolygon& polygon) const;
    bool TransformTargetThroughMatrix4x4(const CMILMatrix4x4& matrix, XPOINTF& point) const;
    bool TransformTargetThroughMatrix4x4(const CMILMatrix4x4& matrix, HitTestPolygon& polygon) const;

    CMILMatrix4x4 GetProjectionMatrix4x4() const;

public:
    // ------------------------------------------------------------------------
    // CUIElement Static Event Handlers
    // ------------------------------------------------------------------------
    static _Check_return_ HRESULT OnKeyUp(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnKeyDown(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnPreviewKeyDown(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnPreviewKeyUp(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnProcessKeyboardAccelerators(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnKeyboardAcceleratorInvoked(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnGotFocus(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnLostFocus(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnGettingFocus(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnLosingFocus(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnNoFocusCandidateFound(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnIsEnabledChanged(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnInheritedPropertyChanged(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnCharacterReceived(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnDragEnter(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnDragLeave(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnDragOver(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnDrop(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnBringIntoViewRequested(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnPointerEntered(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnPointerPressed(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnPointerMoved(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnPointerReleased(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnPointerExited(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnPointerCaptureLost(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnPointerCanceled(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnPointerWheelChanged(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnTapped(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnDoubleTapped(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnHolding(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnContextRequested(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnContextRequestedCore(_In_ CDependencyObject *sender, _In_ CDependencyObject *contextFlyoutObject, _In_ CEventArgs* eventArgs);
    static _Check_return_ HRESULT OnContextCanceled(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnRightTapped(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnRightTappedUnhandled(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    static _Check_return_ HRESULT OnManipulationStarting(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnManipulationInertiaStarting(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnManipulationStarted(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnManipulationDelta(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);
    static _Check_return_ HRESULT OnManipulationCompleted(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs);

    // ------------------------------------------------------------------------
    // CUIElement Static Event Handlers array (array of the delegates)
    // ------------------------------------------------------------------------
    static const INTERNAL_EVENT_HANDLER Delegates[static_cast<XUINT32>(LastControlEvent)];

    // ------------------------------------------------------------------------
    // CUIElement Static helper to check if nDelegate
    // is appropriate delegate number or not
    // ------------------------------------------------------------------------
    __inline static bool IsValidDelegate(_In_ KnownEventIndex nDelegate)
    {
        return nDelegate > KnownEventIndex::UnknownType_UnknownEvent && nDelegate <= LastControlEvent;
    }

protected:
    // ------------------------------------------------------------------------
    // CUIElement Default Event Handlers
    // ------------------------------------------------------------------------
    virtual _Check_return_ HRESULT OnKeyUp(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnKeyDown(_In_ CEventArgs* pEventArgs);

    virtual _Check_return_ HRESULT OnGotFocus(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnLostFocus(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnIsEnabledChanged(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnInheritedPropertyChanged(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnCharacterReceived(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnPointerEntered(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnPointerPressed(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnPointerMoved(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnPointerReleased(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnPointerExited(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnPointerCaptureLost(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnTapped(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnDoubleTapped(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnHolding(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

    virtual _Check_return_ HRESULT OnRightTapped(_In_ CEventArgs* pEventArgs)
    {
        return S_OK;
    }

// CUIElement fields

public:
    CXcpList<REQUEST> *m_pEventList;

// Other public storage
    TextFormatting *m_pTextFormatting;


    LayoutTransitionStorage* m_pLayoutTransitionStorage;
    XUINT16 m_enteredTreeCounter    : 16;
    XUINT16 m_leftTreeCounter       : 16;

private:
    // Placing this private member here because it ensures better data packing on x64 by packing with the two 16-bit fields above it.
    XUINT32 m_layoutFlags;

protected:
    CAutomationPeer *m_pAP;

    // TODO - this entire thing can be deleted. It's used to clear out software resources, and we can make those calls explicitly.
    ContentRenderData m_contentRenderData;
        // Render data is walk-specific. Content render data is storage for data used to represent the UIE content in a specific
        // walk, e.g. edges for software rendering.  Protected so derived classes can read the type.


    // ********************************************************************
    // Bitfield group 1 (32 bits)
    // ********************************************************************
    unsigned int m_isRightToLeft                 : 1; // Coerced value of FrameworkElement.FlowDirection
    unsigned int m_fVisibility                   : 1; // UIElement.Visibility
    unsigned int m_fHitTestVisible               : 1; // UIElement.IsHitTestVisible
    unsigned int m_fAllowDrop                    : 1; // UIElement.AllowDrop
    unsigned int m_bTapEnabled                   : 1; // UIElement.IsTapEnabled, use to set the gesture Tap configuration
    unsigned int m_bDoubleTapEnabled             : 1; // UIElement.IsDoubleTapEnabled, use to set the gesture DoubleTap configuration
    unsigned int m_bRightTapEnabled              : 1; // UIElement.IsRightTapEnabled, use to set the gesture RightTap configuration
    unsigned int m_bHoldEnabled                  : 1; // UIElement.IsHoldEnabled, use to set the gesture Hold configuration
    unsigned int m_fIsEnabled                    : 1;
    unsigned int m_fCoercedIsEnabled             : 1;
    unsigned int m_fUseLayoutRounding            : 1;

    // Rendering dirty flags
    unsigned int m_fNWVisibilityDirty            : 1; // The element's Visibility has changed.
    unsigned int m_fNWTransformDirty             : 1; // The element's Transform, offset, or z-index has changed.
    unsigned int m_isTransform3DDirty            : 1; // The element's Transform3D has changed.
    unsigned int m_fNWProjectionDirty            : 1; // The element's Projection has changed.
    unsigned int m_fNWClipDirty                  : 1; // The element's Clip has changed.
    unsigned int m_fNWOpacityDirty               : 1; // The element's Opacity has changed.
    unsigned int m_fNWCompositeModeDirty         : 1; // The element's CompositeMode has changed

    // True if there's an active ::Windows::UI::Composition animation on the element's Opacity. We normally cull elements with
    // Opacity=0 from the render walk, but we have to keep rendering them if there's the element is animating. We have flags
    // for Storyboard animations (m_hasLocalOpacityIA and m_hasTransitionOpacityIA). This flag tracks active OpacityTransition
    // animations and animations started with UIElement.StartAnimation.
    unsigned int m_hasWUCOpacityAnimation        : 1;

    unsigned int m_fNWContentDirty               : 1; // One of this element's other render-affecting properties has changed.
    unsigned int m_fNWSubgraphDirty              : 1; // One of the element's children has changed and requires rendering.

    // Other rendering flags
    unsigned int m_fNWHadUserClip                : 1; // True iff the UIElement had a Clip set on the previous frame
    unsigned int m_fNWHadLayoutClip              : 1; // True iff the UIElement had a layout clip set on the previous frame
    unsigned int m_fNWLayoutClipDirty            : 1;

    // Indicates that the layout clip has been changed and is now in need of a new
    // path realization. We need dirty information for the clip geometry because
    // it doesn't maintain any itself.

    unsigned int m_fNWHasOpacityAnimationDirty   : 1; // True iff the UIElement has transitioned from having an opacity animation to not, or vice versa
    unsigned int m_fNWHasCompNodeDirty           : 1; // True iff the UIElement has transitioned from having a comp node to not, or vice versa
    unsigned int m_fNWRedirectionDataDirty       : 1; // True iff the UIElement's redirection transform may have changed

    // TODO: DCompAnim: Once we switch over to DComp animations, we may no longer need this kind of granularity
    // with independent animation tracking, in which case we can reclaim some flags.
    unsigned int m_hasTransformIA                : 1;
    unsigned int m_hasProjectionIA               : 1;
    unsigned int m_hasTransform3DIA              : 1;
    unsigned int m_hasLocalOpacityIA             : 1;
    unsigned int m_hasTransitionOpacityIA        : 1;

    // ********************************************************************
    // Bitfield group 2 (32 bits)
    // ********************************************************************
    unsigned int m_hasLocalClipIA                : 1;
    unsigned int m_hasTransitionClipIA           : 1;
    unsigned int m_hasBrushColorIA               : 1;
    unsigned int m_isManipulatable               : 1;
    unsigned int m_hasManipulation               : 1;
    unsigned int m_hasClipManipulation           : 1;
    unsigned int m_isRootElement                 : 1; // True iff this element is the root of a render walk.
    unsigned int m_isRedirectionElement          : 1; // True iff this element renders with redirection
    unsigned int m_hasNonIdentityProjection      : 1;
    unsigned int m_hasAxisUnalignedTransform     : 1;
    unsigned int m_hasAxisUnalignedLocalClip     : 1;
    unsigned int m_hasSwapChainContent           : 1; // True iff this element has media content to compose
    unsigned int m_isUsingHandOffVisual          : 1; // True to force this element to use a composition node because a DComp visual needs to be handed off externally via IXamlDCompInteropPrivate or ElementCompositionPreviewFactory::GetElementVisual
    unsigned int m_isUsingHandInVisual           : 1; // True to force this element to use a composition node because a DComp visual is required to parent the WinRT Visual provided by ElementCompositionPreviewFactory::SetElementChildVisual
    unsigned int m_usesCompositeMode             : 1; // True iff this element is using composition due to a composite mode other than Inherit

    // UIElement requires a DM cross-slide viewport to get a chance to recognize gestures
    unsigned int m_fIsDirectManipulationCrossSlideContainer : 1;

    // UIElement supports CUIDMContainer when set to True
    unsigned int m_fIsDirectManipulationContainer      : 1;

    // Bounds caching
    unsigned int m_contentInnerboundsDirty       : 1;
    unsigned int m_childBoundsDirty              : 1;
    unsigned int m_combinedInnerBoundsDirty      : 1;
    unsigned int m_outerBoundsDirty              : 1;

    // If any of these flags is True, CFocusManager does not set the focus on children or the element itself. Notice that these flags only
    // regulate the focus manager tab behavior.
    unsigned int m_skipFocusSubtree_OffScreenPosition : 1; // Set to True when the element is placed off screen at position ModernCollectionBasePanel::GetOffScreenPosition.
    unsigned int m_skipFocusSubtree_Other             : 1; // Set to True to skip focus for other reasons (like the element is in a recycle pool or hidden SemanticZoom view).

    // True iff CheckAutomaticChanges have been called for this object at least once.
    // It is to make sure not to call costly UIARaisePropertyChangedEvent during the
    // first load if any UIAClient listening.
    unsigned int m_fireUIAPropertyChanges        : 1;

    // DComp animation dirty flags
    unsigned int m_isCanvasLeftAnimationDirty : 1;
    unsigned int m_isCanvasTopAnimationDirty : 1;
    unsigned int m_isOpacityAnimationDirty : 1;

    unsigned int m_bCanDrag                  : 1; // UIElement.CanDrag

    unsigned int m_bBindingHidden : 1;

    unsigned int m_requiresHitTestInvisibleCompNode : 1;            // This node requires a CompNode for DComp hit-test-invisibility purposes

    unsigned int m_isRedirectedChildOfSwapChainPanel : 1;      // This node is the child of a SwapChainPanel and requires redirection walk

    // Indicates this element has non-zero CornerRadius for any of the 4 corners, this requires a CompNode for rounded corner clipping
    unsigned int m_requiresCompNodeForRoundedCorners : 1;

    // ********************************************************************
    // Bitfield group 3 (32 bits)
    // ********************************************************************
    unsigned int m_hasLayoutStorage : 1;

    unsigned int m_hasTransform3D : 1;                              // Keeps track of if this element has a Transform3D (regardless of depth), so we can properly maintain a comp node.
    unsigned int m_isNonClippingSubtree : 1;

    // 3D hit testing optimization flag - Tracks whether this element has 3D depth.
    //
    // Note that this is entirely different from whether there's a Transform3D property set. A UIElement could have a
    // Transform3D set to a CompositeTransform3D with no Z components, in which case it has no depth even though it
    // has a Transform3D. Or it can have Z components set via a hand off visual or a facade property, but have no
    // Transform3D, in which case it has depth even though it has no Transform3D. The Transform3D value can also get
    // overwritten by values from the hand off visual or facade properties.
    //
    // This flag goes hand-in-hand with the 3D matrix we use for hit testing. This flag is true iff that 4x4 matrix
    // has depth. This flag is an optimization that's calculated ahead of time and avoids 2 sparse storage lookups (one
    // to check for existence, one to get the matrix).
    unsigned int m_has3DDepth : 1;

    // 3D hit testing flag - Tracks whether an element in this element's subtree has 3D depth. Does not include this
    // element itself.
    //
    // Like m_has3DDepth, this has nothing to do with whether the Transform3D property is set, because not all
    // Transform3D objects have a Z component set, and because there are other ways to set 3D depth, like the hand
    // off visual and facade properties.
    unsigned int m_has3DDepthInSubtree : 1;

    unsigned int m_fIsAutomationPeerFactorySet : 1;
    unsigned int m_hasOffsetIA : 1;
    unsigned int m_hasActiveConnectedAnimation : 1; // This flag is set when this element is a source or target of an active connected(continuity) animation.
    unsigned int m_listeningForHandOffVisualPropertyChanges : 1;
    unsigned int m_hasEverStoredHandoffOrHandinVisual : 1;
    unsigned int m_isScrollViewerHeader : 1;
    unsigned int m_isItemContainer : 1;     // True if this is a modern panel item container (typically a ListViewItem/GridViewItem)

    // Used to track last time the RTL property was coerced.
    unsigned int m_isRightToLeftGeneration : 8;

    // Indicates that this UI element has hit-testing temporarily suppressed - for example, to support input pass-through
    // for light-dismiss layers.
    unsigned int m_isHitTestingSuppressed : 1;

    unsigned int m_hasImplicitShowAnimation: 1;     // An implicit Show animation is defined (but not necessarily requested, or playing)
    unsigned int m_hasImplicitHideAnimation: 1;     // An implicit Hide animation is defined (but not necessarily requested, or playing)
    unsigned int m_requestedPlayImplicitShow : 1;   // An implicit Show animation is requested to be triggered on the following CompNode tree walk
    unsigned int m_requestedPlayImplicitHide : 1;   // An implicit Hide animation is requested to be triggered on the following CompNode tree walk

    // Lighting flags
    // This element's light targets changed. We need a walk down to this element to retarget it.
    unsigned int m_isLightTargetDirty : 1;
    // Something in this element's XamlLights collection changed. We need to walk down to this element and check the lights in it. If there are any
    // lights that have just entered the tree, then we need to rewalk the entire subtree so the new light can pick up targets. If there aren't any
    // new lights, then we don't need to do anything. Light itself has no Xaml properties that need the render walk to update, and any XamlLights
    // that have left the tree would have taken care of untargeting the CompositionLight when they left the tree.
    unsigned int m_isLightCollectionDirty : 1;
    // Either this element requested to be lit by setting light target IDs, or it has lights in its light collection. Either way, this element needs
    // a comp node.
    unsigned int m_isLightTargetOrHasLight : 1;

    // Indicates that the element is being used as the source for a RenderTargetBitmap during the rendering process.  This may be removed
    // after rendering is completed.  With RS2 and beyond, UIElement render target's need to generate their own composition visual so
    // that they can be captured using composition API's.
    unsigned int m_isRenderTargetSource : 1;

    // This flag is checked by the HWCompNode to determine if an expression animation needs to be set up  (or destroyed) to allow the developer to use a Translation
    // property in the hand-off visual's property set.
    unsigned int m_isTranslationEnabled : 1;

    // This element and its entire subtree must be re-rendered.  This is different than m_fNWSubgraphDirty, which does not cause a walk of the entire subtree.
    unsigned int m_isEntireSubtreeDirty : 1;

    // When true, the RenderWalk avoids culling this element.  Used for forcefully walking to hidden elements.  Has performance implications, use with care!
    unsigned int m_forceNoCulling : 1;

    // ********************************************************************
    // Bitfield group 4 (32 bits)
    // ********************************************************************
    // Indicates that this UI element allows drag-and-drop events to pass through it for the purposes of hit-testing.
    unsigned int m_allowsDragAndDropPassThrough : 1;

    // Used to store the KeyboardNavigationMode enum set through TabFocusNavigation/TabNavigation.
    unsigned int m_eKeyboardNavigationMode : 2;

    unsigned int m_hasFacadeAnimation : 1;

    // True when UIElement.Translation.Z is non-zero
    unsigned int m_hasTranslateZ : 1;

    // True when UIElement.Rotation is non-zero
    unsigned int m_hasNonZeroRotation : 1;

    // True when UIElement.Scale.Z is something other than 1
    unsigned int m_hasScaleZ : 1;

    // True when UIElement.TransformMatrix is something other than the identity matrix
    unsigned int m_hasNonIdentityTransformMatrix : 1;

    // True when UIElement.CenterPoint is something other than (0,0,0)
    unsigned int m_hasNonZeroCenterPoint : 1;

    // True when UIElement.RotationAxis is something other than (0,0,1)
    unsigned int m_hasNonDefaultRotationAxis : 1;

    // True if any facade API has ever been called on this element
    unsigned int m_areFacadesInUse : 1;

    unsigned int m_isScroller : 1;

    // True if there are any attached interations for this element.
    unsigned int m_hasAttachedInteractions : 1;

    unsigned int m_isShadowCaster : 1;

    unsigned int m_isProjectedShadowDefaultReceiver : 1;

    unsigned int m_isProjectedShadowCustomReceiver : 1;

    // Request/ReleaseKeepAlive are APIs used to keep rendering elements that have been removed from the tree, meant to
    // be used with UIElement.Hidden to keep animated elements visible. But we still need to keep an element visible between
    // the time it's removed from the tree (with a Hidden event handler attached) and the time we raise the Hidden event.
    // We automatically call RequestKeepAlive during that time and mark this flag. After we raise Hidden, we call ReleaseKeepAlive
    // and clear this flag again.
    unsigned int m_isKeepingAliveUntilHiddenEventIsRaised : 1;

    // True if this element is a TransitionRoot and also has children
    unsigned int m_isTransitionRootWithChildren : 1;

    unsigned int m_isTabStop : 1;

    unsigned int m_isProtectedCursorSet : 1; // True if UIElement::ProtectedCursor property is set to non-null

    // True if this UIElement should play an opacity animation to fade in its drop shadow
    unsigned int m_shouldFadeInDropShadow : 1;

    /* Remove one of these each time you use a new bit so we can keep track of when we'll pop to a new word of size, which we really should avoid

    unsigned int m_unused22 : 1;
    unsigned int m_unused23 : 1;
    unsigned int m_unused24 : 1;
    unsigned int m_unused25 : 1;
    unsigned int m_unused26 : 1;
    unsigned int m_unused27 : 1;
    unsigned int m_unused28 : 1;
    unsigned int m_unused29 : 1;
    unsigned int m_unused30 : 1;
    unsigned int m_unused31 : 1;
    unsigned int m_unused32 : 1;
*/

public:
    // This field is moved here awkwardly in between other private fields because it improves padding and alignment on x64 to save size in CUIElement.
    // use accessor methods GetOpacityLocal and GetOpacityCombined instead of using this field.
    XFLOAT m_eOpacityPrivate;               // UIElement.Opacity.

    //-----------------------------------------------------------------------------
    // AccessKey Events
    //-----------------------------------------------------------------------------
    bool RaiseAccessKeyInvoked();
    void RaiseAccessKeyShown(_In_z_ const wchar_t* strPressedKeys);
    void RaiseAccessKeyHidden();

    static _Check_return_ HRESULT TabFocusNavigation(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
    );

    DirectUI::KeyboardNavigationMode TabNavigation() const
    {
        if (static_cast<DirectUI::KeyboardNavigationMode>(m_eKeyboardNavigationMode) == TAB_NAVIGATION_NOTSET)
        {
            // If this was never set, we want to use the default value of local.
            return DirectUI::KeyboardNavigationMode::Local;
        }

        return static_cast<DirectUI::KeyboardNavigationMode>(m_eKeyboardNavigationMode);
    }

    void SetTabNavigation(_In_ DirectUI::KeyboardNavigationMode keyboardNavigationMode)
    {
        int mode = static_cast<int>(keyboardNavigationMode);

        ASSERT(mode < 4);
        m_eKeyboardNavigationMode = mode & 0x3; // This field is only 2 bits.
    }

    bool IsTabNavigationSet() const
    {
        return static_cast<DirectUI::KeyboardNavigationMode>(m_eKeyboardNavigationMode) != TAB_NAVIGATION_NOTSET;
    }

private:
    XRECTF_RB m_contentInnerBounds;  // The bounds of this element's content only (e.g. the background of a Panel). Relative to this element.
    XRECTF_RB m_childBounds;         // The unioned outer bounds of all of this element's children. Relative to this element.
    XRECTF_RB m_combinedInnerBounds; // The union of m_contentInnerBounds and m_childBounds. Relative to this element.
    XRECTF_RB m_outerBounds;         // 2D: m_combinedInnerBounds transformed through this element's properties (the same as its parent's inner space)
                                     // 3D: Null bounds; see EnsureOuterBounds.  Hit testing walks down to all 3D elements and transforms the
                                     // world-space rect down to the element before proceeding with 2D hit testing.
    // TODO: HWPC: Are all these separate bounds caches really necessary?

private:
    PropertyRenderData m_propertyRenderData;
        // Render data is walk-specific. Property render data is storage for all UIE properties (Transform through CacheMode).

    HWCompTreeNode *m_pCompositionPeer;

    CLayoutStorage m_layoutStorage;

    CUIElementCollection *m_pChildren;

    xvector<CLayoutTransitionElement*> *m_pLayoutTransitionRenderers;

    static const wchar_t* const s_translationString;
};

void ComputeUnidimensionalEffectiveViewport(
    _In_ const std::vector<CUIElement::UnidimensionalViewportInformation>& viewports,
    _Out_ float& visibleOffset,
    _Out_ float& visibleLength);

void ComputeUnidimensionalMaxViewport(
    _In_ const std::vector<CUIElement::UnidimensionalViewportInformation>& viewports,
    _Out_ float& offset,
    _Out_ float& length);

void ComputeUnidimensionalBringIntoViewDistance(
    _In_ const float elementOffset,
    _In_ const float elementLength,
    _In_ const std::vector<CUIElement::UnidimensionalViewportInformation>& viewports,
    _Out_ float& distance);
