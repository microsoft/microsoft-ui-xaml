// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      UIElement is a base class for most of the objects that have visual
//      appearance and can process basic input.

#pragma once

#include <EventCallbacks.h>
#include <DirectManipulationTypes.h>
#include <DoubleUtil.h>
#include <FloatUtil.h>
#include "comTemplateLibrary.h"
#include "DependencyObject.h"
#include "UIElement.g.h"
#include "StickyHeaderWrapper.h"
#include <microsoft.ui.composition.private.h>

// TODO: Give codegen the ability to map non-codegen types to filenames
//       so it can add the proper #include directives.
//       Until then, we'll need to pull in core headers in some of our framework partial class headers
#include <Pointer.h>

class CRoutedEventArgs;

namespace DirectUI
{
    class DragVisual;
    class DragStartingEventSourceType;

    typedef CRoutedEventSource<
        wf::ITypedEventHandler<xaml::UIElement*, xaml_controls::InputValidationErrorEventArgs*>,
        xaml::IUIElement,
        xaml_controls::IInputValidationErrorEventArgs> CErrorsChangedEventSource;

    // UIElement is a base class for most of the objects that have visual
    // appearance and can process basic input.
    PARTIAL_CLASS(UIElement)
    {
        friend class VirtualizationInformation;
        friend class AutomaticDragHelper;

        public:
            // Initializes a new instance of the UIElement class.
            UIElement();

            // Destroys an instance of the UIElement class.
            ~UIElement() override;

            CUIElement* GetHandle() const;

            IFACEMETHOD(get_RenderTransform)(
                _Outptr_ xaml_media::ITransform** pValue) override;

            _Check_return_ HRESULT get_TranslationImpl(_Out_ wfn::Vector3* translation);

            _Check_return_ HRESULT put_TranslationImpl(const wfn::Vector3& translation);

            _Check_return_ HRESULT get_RotationImpl(_Out_ FLOAT* rotation);

            _Check_return_ HRESULT put_RotationImpl(FLOAT rotation);

            _Check_return_ HRESULT get_ScaleImpl(_Out_ wfn::Vector3* scale);

            _Check_return_ HRESULT put_ScaleImpl(const wfn::Vector3& scale);

            _Check_return_ HRESULT get_TransformMatrixImpl(_Out_ wfn::Matrix4x4* transformMatrix);

            _Check_return_ HRESULT put_TransformMatrixImpl(const wfn::Matrix4x4& transformMatrix);

            _Check_return_ HRESULT get_CenterPointImpl(_Out_ wfn::Vector3* centerPoint);

            _Check_return_ HRESULT put_CenterPointImpl(const wfn::Vector3& centerPoint);

            _Check_return_ HRESULT get_RotationAxisImpl(_Out_ wfn::Vector3* rotationAxis);

            _Check_return_ HRESULT put_RotationAxisImpl(const wfn::Vector3& rotationAxis);

            // Facades_TODO:  Remove these after codegen changes for Simple properties is complete
            _Check_return_ HRESULT get_AnimatedTranslationImpl(_Out_ wfn::Vector3* translation) { return E_NOTIMPL;}

            _Check_return_ HRESULT put_AnimatedTranslationImpl(const wfn::Vector3& translation) { return E_NOTIMPL;}

            _Check_return_ HRESULT get_AnimatedRotationImpl(_Out_ FLOAT* rotation) { return E_NOTIMPL;}

            _Check_return_ HRESULT put_AnimatedRotationImpl(FLOAT rotation) { return E_NOTIMPL;}

            _Check_return_ HRESULT get_AnimatedScaleImpl(_Out_ wfn::Vector3* scale) { return E_NOTIMPL;}

            _Check_return_ HRESULT put_AnimatedScaleImpl(const wfn::Vector3& scale) { return E_NOTIMPL;}

            _Check_return_ HRESULT get_AnimatedTransformMatrixImpl(_Out_ wfn::Matrix4x4* transformMatrix) { return E_NOTIMPL;}

            _Check_return_ HRESULT put_AnimatedTransformMatrixImpl(const wfn::Matrix4x4& transformMatrix) { return E_NOTIMPL;}

            _Check_return_ HRESULT get_AnimatedCenterPointImpl(_Out_ wfn::Vector3* centerPoint) { return E_NOTIMPL;}

            _Check_return_ HRESULT put_AnimatedCenterPointImpl(const wfn::Vector3& centerPoint) { return E_NOTIMPL;}

            _Check_return_ HRESULT get_AnimatedRotationAxisImpl(_Out_ wfn::Vector3* rotationAxis) { return E_NOTIMPL;}

            _Check_return_ HRESULT put_AnimatedRotationAxisImpl(const wfn::Vector3& rotationAxis) { return E_NOTIMPL;}

            _Check_return_ HRESULT get_KeepAliveCountImpl(_Out_ INT* value) { return E_NOTIMPL; }

            _Check_return_ HRESULT put_KeepAliveCountImpl(const INT& value) { return E_NOTIMPL; }
            // end Facades_TODO

            _Check_return_ HRESULT get_RasterizationScaleImpl(_Out_ DOUBLE* rasterizationScale);

            _Check_return_ HRESULT put_RasterizationScaleImpl(DOUBLE rasterizationScale);

            _Check_return_ HRESULT get_ActualOffsetImpl(_Out_ wfn::Vector3* layoutOffset);
            _Check_return_ HRESULT get_ActualSizeImpl(_Out_ wfn::Vector2* layoutSize);

            _Check_return_ HRESULT StartAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation);
            _Check_return_ HRESULT StopAnimationImpl(_In_ WUComp::ICompositionAnimationBase* animation);

            _Check_return_ HRESULT STDMETHODCALLTYPE PopulatePropertyInfo(
                _In_ HSTRING propertyName,
                _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
                ) override;

            _Check_return_ HRESULT PopulatePropertyInfoOverrideImpl(
                _In_ HSTRING propertyName,
                _In_ WUComp::IAnimationPropertyInfo* animationPropertyInfo
                );

            _Check_return_ HRESULT STDMETHODCALLTYPE GetVisualInternal(_Outptr_ WUComp::IVisual** visual) override;

            _Check_return_ HRESULT get_InteractionsImpl(_Out_ wfc::IVector<xaml::InteractionBase*>** interactions);

            // Gets the size that this UIElement computed during the measure
            // pass of the layout process.
            _Check_return_ HRESULT get_DesiredSizeImpl(
                _Out_ wf::Size* pValue);

            // Updates the DesiredSize of a UIElement. Typically, objects that
            // implement custom layout for their layout children call this
            // method from their own MeasureOverride implementations to form a
            // recursive layout update.
            _Check_return_ HRESULT MeasureImpl(
                _In_ wf::Size availableSize);

            _Check_return_ HRESULT InvalidateArrangeImpl();
            _Check_return_ HRESULT InvalidateMeasureImpl();

            // Returns True when this element is about to be re-arranged.
            bool IsArrangeDirty();

            // Positions child objects and determines a size for a UIElement.
            // Parent objects that implement custom layout for their child
            // elements should call this method from their layout override
            // implementations to form a recursive layout update.
            _Check_return_ HRESULT ArrangeImpl(
                _In_ wf::Rect finalRect);

            _Check_return_ HRESULT CapturePointerImpl(
                _In_ xaml_input::IPointer* value, _Out_ BOOLEAN* returnValue);

            _Check_return_ HRESULT get_XamlRootImpl(_Outptr_result_maybenull_ xaml::IXamlRoot** ppValue);
            virtual _Check_return_ HRESULT put_XamlRootImpl(_In_opt_ xaml::IXamlRoot* pValue);

            // Attempts to bring the given rectangle of this element into view
            // by originating a RequestBringIntoView event.
            void BringIntoView(
                wf::Rect bounds,
                BOOLEAN forceIntoView,
                BOOLEAN useAnimation,
                BOOLEAN skipDuringManipulation,
                double horizontalAlignmentRatio = DoubleUtil::NaN,
                double verticalAlignmentRatio = DoubleUtil::NaN,
                double offsetX = 0.0,
                double offsetY = 0.0);

            // Called to determine if this element has manipulatable elements
            virtual _Check_return_ HRESULT get_CanManipulateElements(
                _Out_ BOOLEAN* pCanManipulateElementsByTouch,
                _Out_ BOOLEAN* pCanManipulateElementsNonTouch,
                _Out_ BOOLEAN* pCanManipulateElementsWithBringIntoViewport);

            // Called to set a manipulation handler used in control-to-InputManager notifications
            virtual _Check_return_ HRESULT put_ManipulationHandler(
                _In_opt_ HANDLE hManipulationHandler);

            // Used to tell the container if the manipulation handler wants to be
            // aware of manipulation characteristic changes even though no manipulation
            // is in progress.
            virtual _Check_return_ HRESULT SetManipulationHandlerWantsNotifications(
                _In_ UIElement* pManipulatedElement,
                _In_ BOOLEAN wantsNotifications);

            // Caches the dependency object that is touched during the initiation of a touch-based manipulation.
            virtual _Check_return_ HRESULT SetPointedElement(
                _In_ DependencyObject* pPointedElement);

            // Returns the manipulated element for a given pointed element
            virtual _Check_return_ HRESULT GetManipulatedElement(
                _In_ DependencyObject* pPointedElement,     // Element that was touched by the user.
                _In_opt_ UIElement* pChildElement,          // Direct child on the parent chain between the pointed element and this element.
                _Out_ UIElement** ppManipulatedElement);    // Placeholder for the returned manipulated element.

            // Returns information about a manipulated element's viewport
            virtual _Check_return_ HRESULT GetManipulationViewport(
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
                _Out_opt_ DMMotionTypes* pChainedMotionTypes);

            // Returns information about a manipulated element's primary content
            virtual _Check_return_ HRESULT GetManipulationPrimaryContent(
                _In_ UIElement* pManipulatedElement,
                _Out_opt_ XSIZEF* pOffsets,
                _Out_opt_ XRECTF* pBounds,
                _Out_opt_ DMAlignment* pHorizontalAligment,
                _Out_opt_ DMAlignment* pVerticalAligment,
                _Out_opt_ FLOAT* pMinZoomFactor,
                _Out_opt_ FLOAT* pMaxZoomFactor,
                _Out_opt_ BOOLEAN* pIsHorizontalStretchAlignmentTreatedAsNear,
                _Out_opt_ BOOLEAN* pIsVerticalStretchAlignmentTreatedAsNear,
                _Out_opt_ BOOLEAN* pIsLayoutRefreshed);

            // Used to access information about a manipulation secondary content.
            virtual _Check_return_ HRESULT GetManipulationSecondaryContent(
                _In_ UIElement* pContentElement,
                _Out_ XSIZEF* pOffsets);

            // Returns information about a manipulated element's primary content transform
            virtual _Check_return_ HRESULT GetManipulationPrimaryContentTransform(
                _In_ UIElement* pManipulatedElement,
                _In_ BOOLEAN inManipulation,
                _In_ BOOLEAN forInitialTransformationAdjustment,
                _In_ BOOLEAN forMargins,
                _Out_opt_ FLOAT* pTranslationX,
                _Out_opt_ FLOAT* pTranslationY,
                _Out_opt_ FLOAT* pZoomFactor);

            // Returns information about a secondary content's transform
            virtual _Check_return_ HRESULT GetManipulationSecondaryContentTransform(
                _In_ UIElement* pContentElement,
                _Out_ FLOAT* pTranslationX,
                _Out_ FLOAT* pTranslationY,
                _Out_ FLOAT* pZoomFactor);

            // Returns the snap points for the provided manipulated element and motion type.
            virtual _Check_return_ HRESULT GetManipulationSnapPoints(
                _In_ UIElement* pManipulatedElement,   // Manipulated element for which the snap points are requested
                _In_ DMMotionTypes motionType,         // Motion type for which the snap points are requested
                _Out_ BOOLEAN* pAreSnapPointsOptional, // Set to True when returned snap points are optional
                _Out_ BOOLEAN* pAreSnapPointsSingle,   // Set to True when returned snap points are single (i.e. breaking inertia)
                _Out_ BOOLEAN* pAreSnapPointsRegular,  // Set to True when returned snap points are equidistant
                _Out_ FLOAT* pRegularOffset,           // Offset of regular snap points
                _Out_ FLOAT* pRegularInterval,         // Interval of regular snap points
                _Out_ UINT32* pcIrregularSnapPoints,   // Number of irregular snap points
                _Outptr_result_buffer_(*pcIrregularSnapPoints) FLOAT** ppIrregularSnapPoints,   // Array of irregular snap points
                _Out_ DMSnapCoordinate* pSnapCoordinate); // Coordinate system used for snap points

            virtual _Check_return_ HRESULT NotifyManipulatabilityAffectingPropertyChanged(
                _In_ BOOLEAN isInLiveTree);

            virtual _Check_return_ HRESULT NotifyContentAlignmentAffectingPropertyChanged(
                _In_ UIElement* pManipulatedElement,   // Manipulated element for which an alignment characteristic has changed
                _In_ BOOLEAN isForHorizontalAlignment,
                _In_ BOOLEAN isForStretchAlignment,
                _In_ BOOLEAN isStretchAlignmentTreatedAsNear);

            virtual _Check_return_ HRESULT NotifyManipulationProgress(
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
                _In_ BOOLEAN isBringIntoViewportConfigurationActivated);

            virtual _Check_return_ HRESULT NotifyManipulationStateChanged(
                _In_ DMManipulationState state);

            virtual _Check_return_ HRESULT NotifyBringIntoViewportNeeded(
                _In_ UIElement* pManipulatedElement,
                _In_ FLOAT translationX,
                _In_ FLOAT translationY,
                _In_ FLOAT zoomFactor,
                _In_ BOOLEAN transformIsValid,
                _In_ BOOLEAN transformIsInertiaEnd);

            virtual _Check_return_ HRESULT GetDManipElementAndProperty(
                _In_ KnownPropertyIndex targetProperty,
                _Outptr_ CDependencyObject** ppDManipElement,
                _Out_ XUINT32 *pDManipProperty);

            virtual _Check_return_ HRESULT GetDManipElement(
                _Outptr_ CDependencyObject** ppDManipElement);

            bool ShouldPlayImplicitShowHideAnimation();

            virtual _Check_return_ HRESULT GetChildrenCount(
                _Out_ INT* pnCount);

            virtual _Check_return_ HRESULT GetChild(
                _In_ INT nChildIndex,
                _Outptr_ xaml::IDependencyObject** ppDO);

            // Override this method and return TRUE in order to navigate among automation children in reverse order.
            virtual BOOLEAN AreAutomationPeerChildrenReversed()
            {
                return FALSE;
            }

            virtual bool IsDraggableOrPannableImpl()
            {
                BOOLEAN canDrag = FALSE;
                VERIFYHR(get_CanDrag(&canDrag));

                return !!canDrag;
            }

            static _Check_return_ HRESULT RaiseKeyboardAcceleratorInvokedStatic(
                _In_ CDependencyObject* pElement,
                _In_ KeyboardAcceleratorInvokedEventArgs *pKAIEventArgs,
                _Out_ BOOLEAN *pIsHandled);

            static _Check_return_ HRESULT RaiseProcessKeyboardAcceleratorsStatic(
                _In_ CUIElement* pUIElement,
                _In_ wsy::VirtualKey key,
                _In_ wsy::VirtualKeyModifiers keyModifiers,
                _Out_ BOOLEAN *pHandled,
                _Out_ BOOLEAN *pHandledShouldNotImpedeTextInput);

            _Check_return_ HRESULT OnKeyboardAcceleratorInvokedImpl(_In_ xaml_input::IKeyboardAcceleratorInvokedEventArgs* pArgs)
            {
                return S_OK;
            }

            _Check_return_ HRESULT OnProcessKeyboardAcceleratorsImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs);
            _Check_return_ HRESULT TryInvokeKeyboardAcceleratorImpl(_In_ xaml_input::IProcessKeyboardAcceleratorEventArgs* pArgs);

            _Check_return_ HRESULT OnCreateAutomationPeerImpl(_Outptr_ xaml_automation_peers::IAutomationPeer** returnValue);

            static _Check_return_ HRESULT OnBringIntoViewRequestedFromCore(_In_ CUIElement* pUIElement, _In_ CRoutedEventArgs* args);

            _Check_return_ HRESULT OnBringIntoViewRequestedImpl(_In_ xaml::IBringIntoViewRequestedEventArgs* pArgs);

            _Check_return_ HRESULT FindSubElementsForTouchTargetingImpl(
                _In_ wf::Point point,
                _In_ wf::Rect boundingRect,
                _Outptr_ wfc::IIterable<wfc::IIterable<wf::Point>*>** pReturnValue);

            _Check_return_ HRESULT GetChildrenInTabFocusOrderImpl(
                _Outptr_ wfc::IIterable<xaml::DependencyObject*>** returnValue);

            // Cancels all ongoing DManip-based manipulations involving this element and its ancestors.
            _Check_return_ HRESULT CancelDirectManipulationsImpl(
                _Out_ BOOLEAN* pReturnValue);

            //TODO: this doesn't need to be virtual on the base or an override here, requires a bit of codegen work to fix...
            _Check_return_ HRESULT GetDragStartingEventSourceNoRef(_Outptr_ DragStartingEventSourceType** ppEventSource) override;
            //CErrorsChangedEventSource* GetErrorsChangedEventSourceNoRef();

            _Check_return_ HRESULT StartDragAsyncImpl(
                _In_ ixp::IPointerPoint* pPointerPoint,
                _Outptr_ wf::IAsyncOperation<wadt::DataPackageOperation>** ppReturnValue);

            _Check_return_ HRESULT StartBringIntoViewImpl();
            _Check_return_ HRESULT StartBringIntoViewWithOptionsImpl(_In_ xaml::IBringIntoViewOptions* options);

            // perf: this really should be an attached DP on ItemContainerGenerator.
            // however, we call these in very tight loops and it made sense to pay for perf with a bit
            // We use this field to tell an ItemsControl to setup a transition or not. Needed to
            // know whether a container is _really_ new or just changed locations in the case
            // of virtualization.
            bool GetIsLocationValid() { return m_IsLocationValid; }
            void SetIsLocationValid(bool value)
            {
                VERIFYHR(MarkHasState());
                m_IsLocationValid = value;
            }

            _Check_return_ HRESULT AddHandlerImpl(
                _In_ xaml::IRoutedEvent* routedEvent,
                _In_ IInspectable* pEventHandler,
                _In_ BOOLEAN handledEventsToo);

            _Check_return_ HRESULT RemoveHandlerImpl(
                _In_ xaml::IRoutedEvent* routedEvent,
                _In_ IInspectable* pEventHandler);

#pragma warning( suppress : 6101 ) // Out params don't get set, but this copy of the function should never be called.
            // Tab navigation virtual override methods to interact with Focus Manager.
            virtual _Check_return_ HRESULT ProcessTabStopOverride(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_opt_ DependencyObject* pCandidateTabStopElement,
                const bool isBackward,
                const bool didCycleFocusAtRootVisualScope,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsTabStopOverridden)
            {
                RRETURN(S_OK);
            }

#pragma warning( suppress : 6101 ) // Out params don't get set, but this copy of the function should never be called.
            virtual _Check_return_ HRESULT ProcessCandidateTabStopOverride(
                _In_opt_ DependencyObject* pFocusedElement,
                _In_ DependencyObject* pCandidateTabStopElement,
                _In_opt_ DependencyObject* pOverriddenCandidateTabStopElement,
                const bool isBackward,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsCandidateTabStopOverridden)
            {
                RRETURN(S_OK);
            }

#pragma warning( suppress : 6101 ) // Out params don't get set, but this copy of the function should never be called.
            virtual _Check_return_ HRESULT GetNextTabStopOverride(
                _Outptr_ DependencyObject** ppNextTabStop)
            {
                RRETURN(S_OK);
            }

#pragma warning( suppress : 6101 ) // Out params don't get set, but this copy of the function should never be called.
            virtual _Check_return_ HRESULT GetPreviousTabStopOverride(
                _Outptr_ DependencyObject** ppPreviousTabStop)
            {
                RRETURN(S_OK);
            }

            virtual _Check_return_ HRESULT FocusImpl(
                _In_ xaml::FocusState value,
                _Out_ BOOLEAN* returnValue);

            _Check_return_ HRESULT FocusNoActivateImpl(
                _In_ xaml::FocusState value,
                _Out_ BOOLEAN* returnValue);

            _Check_return_ HRESULT FocusWithDirection(
                _In_ xaml::FocusState value,
                _In_ DirectUI::FocusNavigationDirection focusNavigationDirection,
                InputActivationBehavior inputActivationBehavior,
                _Out_ BOOLEAN* returnValue);

            void SetAnimateIfBringIntoView()
            {
                ASSERT(!m_animateIfBringIntoView);
                m_animateIfBringIntoView = TRUE;
            }

#pragma warning( suppress : 6101 ) // Out params don't get set, but this copy of the function should never be called.
            virtual _Check_return_ HRESULT GetFirstFocusableElementOverride(
                _Outptr_ DependencyObject** ppFirstFocusable)
            {
                RRETURN(S_OK);
            }

#pragma warning( suppress : 6101 ) // Out params don't get set, but this copy of the function should never be called.
            virtual _Check_return_ HRESULT GetLastFocusableElementOverride(
                _Outptr_ DependencyObject** ppLastFocusable)
            {
                RRETURN(S_OK);
            }

            virtual _Check_return_ HRESULT OnTouchDragStarted(
                _In_ ixp::IPointerPoint* pointerPoint,
                _In_ xaml_input::IPointer* pointer);

        public:

            _Check_return_ HRESULT DisconnectChildrenRecursive();

            // Check whether this UIElement was generated by one of our internal GetContainerForItemOverride implementations.
            BOOLEAN GetIsGeneratedContainer();

            // Set a flag showing whether this UIElement was generated by one of our internal GetContainerForItemOverride implementations.
            void SetIsGeneratedContainer(bool value);

            void ResetAutomationPeer();

            // This is fired on the drag source UIElement indicating drop occurred
            // accepted by the drop target.
            _Check_return_ HRESULT OnDropCompleted(
                _In_ wadt::DataPackageOperation dropStatus);

            _Check_return_ HRESULT OnDisconnectVisualChildrenImpl() { RRETURN(S_OK); }

            _Check_return_ HRESULT GetElementVisual(_Outptr_ WUComp::IVisual** ppResult);

            _Check_return_ HRESULT TransformToVisualImpl(
                _In_ xaml::IUIElement* pVisual,
                _Outptr_ xaml_media::IGeneralTransform** ppReturnValue);

        protected:
            _Check_return_ HRESULT ConfigureAutomaticDragHelper(_In_ bool startDetectingDrag);

            // Anchor point has to take into account RTL and scale factor as Core wants physical pixels
            void GetAnchorPoint(_In_ wf::Point pointerLocation, _Out_ wf::Point* anchorPoint);

        public:
            // Core callbacks
            static _Check_return_ HRESULT GetCanManipulateElements(
                _In_ CUIElement* nativeTarget,
                _Out_ bool* pfCanManipulateElementsByTouch,
                _Out_ bool* pfCanManipulateElementsNonTouch,
                _Out_ bool* pfCanManipulateElementsWithBringIntoViewport);

            static _Check_return_ HRESULT SetManipulationHandler(
                _In_ CUIElement* nativeTarget,
                _In_opt_ void* nativeManipulationHandler);

            static _Check_return_ HRESULT SetManipulationHandlerWantsNotifications(
                _In_ CUIElement* nativeTarget,
                _In_ CUIElement* nativeManipulatedElement,
                _In_ bool fWantsNotifications);

            static _Check_return_ HRESULT SetPointedElement(
                _In_ CUIElement* nativeTarget,
                _In_ CDependencyObject* nativePointedElement);

            static _Check_return_ HRESULT GetManipulatedElement(
                _In_ CUIElement* nativeTarget,
                _In_opt_ CDependencyObject* nativePointedElement,
                _In_opt_ CUIElement* nativeChildElement,
                _Out_ CUIElement** nativeManipulatedElement);

            static _Check_return_ HRESULT GetManipulationViewport(
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
                _Out_opt_ XUINT32* pChainedMotionTypes);

            static _Check_return_ HRESULT GetManipulationPrimaryContent(
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
                _Out_opt_ bool* pfIsLayoutRefreshed);

            static _Check_return_ HRESULT GetManipulationSecondaryContent(
                _In_ CUIElement* nativeTarget,
                _In_ CUIElement* nativeContentElement,
                _Out_ XSIZEF* pOffsets);

            static _Check_return_ HRESULT GetManipulationPrimaryContentTransform(
                _In_ CUIElement* nativeTarget,
                _In_ CUIElement* nativeManipulatedElement,
                _In_ bool fInManipulation,
                _In_ bool fForInitialTransformationAdjustment,
                _In_ bool fForMargins,
                _Out_opt_ XFLOAT* pTranslationX,
                _Out_opt_ XFLOAT* pTranslationY,
                _Out_opt_ XFLOAT* pZoomFactor);

            static _Check_return_ HRESULT GetManipulationSecondaryContentTransform(
                _In_ CUIElement* nativeTarget,
                _In_ CUIElement* nativeContentElement,
                _Out_ XFLOAT* pTranslationX,
                _Out_ XFLOAT* pTranslationY,
                _Out_ XFLOAT* pZoomFactor);

            static _Check_return_ HRESULT GetManipulationSnapPoints(
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
                _Out_ XUINT32* pSnapCoordinate);

            static _Check_return_ HRESULT NotifyManipulatabilityAffectingPropertyChanged(
                _In_ CUIElement* nativeTarget,
                _In_ bool fIsInLiveTree);

            static _Check_return_ HRESULT NotifyContentAlignmentAffectingPropertyChanged(
                _In_ CUIElement* nativeTarget,
                _In_ CUIElement* pManipulatedElement,
                _In_ bool fIsForHorizontalAlignment,
                _In_ bool fIsForStretchAlignment,
                _In_ bool fIsStretchAlignmentTreatedAsNear);

            static _Check_return_ HRESULT NotifyManipulationProgress(
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
                _In_ bool fIsBringIntoViewportConfigurationActivated);

            static _Check_return_ HRESULT NotifyManipulationStateChanged(
                _In_ CUIElement* nativeTarget,
                _In_ XUINT32 state);

            static _Check_return_ HRESULT IsBringIntoViewportNeeded(
                _In_ CUIElement* nativeTarget,
                _Out_ bool * bringIntoViewportNeeded);

            static _Check_return_ HRESULT NotifyBringIntoViewportNeeded(
                _In_ CUIElement* nativeTarget,
                _In_ CUIElement* nativeManipulatedElement,
                _In_ XFLOAT translationX,
                _In_ XFLOAT translationY,
                _In_ XFLOAT zoomFactor,
                _In_ bool fTransformIsValid,
                _In_ bool fTransformIsInertiaEnd);

            static _Check_return_ HRESULT NotifySnapPointsChanged(
                _In_ CUIElement* nativeTarget,
                _In_ bool fHorizontalSnapPoints);

            static _Check_return_ HRESULT NotifyCanDragChanged(
                _In_ CUIElement* nativeTarget,
                _In_ bool fCanDrag);

            static _Check_return_ HRESULT OnDirectManipulationDraggingStarted(
                _In_ CUIElement* nativeTarget);

            static _Check_return_ HRESULT GetDManipElementAndProperty(
                _In_ CUIElement* nativeTarget,
                _In_ KnownPropertyIndex targetProperty,
                _Outptr_ CDependencyObject** ppDManipElement,
                _Out_ XUINT32 *pDManipProperty);

            static _Check_return_ HRESULT GetDManipElement(
                _In_ CUIElement* nativeTarget,
                _Outptr_ CDependencyObject** ppDManipElement);

            static bool ShouldPlayImplicitShowHideAnimation(_In_ CUIElement* nativeTarget);

            // UIA callback
            static _Check_return_ HRESULT OnCreateAutomationPeer(
                _In_ CDependencyObject* nativeTarget,
                _Out_ CAutomationPeer** returnAP);

            _Check_return_ HRESULT GetOrCreateAutomationPeer(xaml_automation_peers::IAutomationPeer** ppAutomationPeer);

            // InputPane callback
            static _Check_return_ HRESULT NotifyInputPaneStateChange(
                _In_ CUIElement* nativeTarget,
                _In_ InputPaneState inputPaneState,
                _In_ XRECTF inputPaneBounds);

            static _Check_return_ HRESULT ApplyInputPaneTransition(
                _In_ CUIElement* nativeTarget,
                _In_ bool fEnableThemeTransition);

            static _Check_return_ HRESULT ApplyElevationEffectProxy(
               _In_ CUIElement* target,
               unsigned int depth);

            static _Check_return_ HRESULT GetScrollContentPresenterViewportRatios(
                _In_ CUIElement* nativeTarget,
                _In_ CDependencyObject* nativeChild,
                _Out_ XSIZEF* ratios);

            static _Check_return_ HRESULT IsScrollViewerContentScrollable(
                _In_ CUIElement* nativeTarget,
                _Out_ bool* isContentHorizontallyScrollable,
                _Out_ bool* isContentVerticallyScrollable);

            // FocusManager callbacks that will be called from DxamlCore.cpp
            static _Check_return_ HRESULT ProcessTabStop(
                _In_ CContentRoot* contentRoot,
                _In_opt_ CDependencyObject* pFocusedElement,
                _In_opt_ CDependencyObject* pCandidateTabStopElement,
                const bool isBackward,
                const bool didCycleFocusAtRootVisualScope,
                _Outptr_ CDependencyObject** ppNewTabStop,
                _Out_ bool* pIsTabStopOverridden);

            static _Check_return_ HRESULT GetNextTabStop(
                _In_ CDependencyObject* pFocusedElement,
                _Outptr_ CDependencyObject** ppNextTabStop);

            static _Check_return_ HRESULT GetPreviousTabStop(
                _In_ CDependencyObject* pFocusedElement,
                _Outptr_ CDependencyObject** ppPreviousTabStop);

            static _Check_return_ HRESULT GetFirstFocusableElement(
                _In_ CDependencyObject* pSearchStart,
                _Outptr_ CDependencyObject** ppFirstFocusable);

            static _Check_return_ HRESULT GetLastFocusableElement(
                _In_ CDependencyObject* pSearchStart,
                _Outptr_ CDependencyObject** ppLastFocusable);

            static _Check_return_ HRESULT IsDraggableOrPannable(
                _In_ CUIElement* pElement,
                _Out_ bool* isDraggableOrPannable);

        private:
            _Check_return_ HRESULT ProcessTabStopInternal(
                _In_opt_ DependencyObject* pCandidateTabStop,
                const bool isBackward,
                const bool didCycleFocusAtRootVisualScope,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsTabStopOverridden);

            _Check_return_ HRESULT ProcessCandidateTabStopInternal(
                _In_opt_ DependencyObject* pCurrentTabStop,
                _In_opt_ DependencyObject* pOverriddenCandidateTabStop,
                _In_ BOOLEAN isBackward,
                _Outptr_ DependencyObject** ppNewTabStop,
                _Out_ BOOLEAN* pIsCandidateTabStopOverridden);

            _Check_return_ HRESULT OnPointerPressed(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnPointerMoved(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnPointerReleased(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnPointerCaptureLost(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            _Check_return_ HRESULT OnHolding(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IHoldingRoutedEventArgs* pArgs);

        protected:
            // Called by a UIElement to signal that it implements IDirectManipulationContainer,
            // or that it got deleted before a manipulation handler was attached.
            _Check_return_ HRESULT put_IsDirectManipulationContainer(
                _In_ BOOLEAN isDirectManipulationContainer);

            // Used to register or unregister this element as a dummy DM container.
            _Check_return_ HRESULT put_IsDirectManipulationCrossSlideContainer(
                _In_ BOOLEAN isDirectManipulationCrossSlideContainer);

            // Used to declare that the cross-slide viewport for this element can be discarded.
            _Check_return_ HRESULT DirectManipulationCrossSlideContainerCompleted();

            // Protected virtual method that is called after CapturePointer() succeeds.  Override to perform custom handling.
            virtual _Check_return_ HRESULT OnPointerCaptured() { RRETURN(S_OK); }

            virtual bool ShouldAutomaticDragHelperHandleInputEvents() { return true; }

        private:
            TrackerPtr<xaml_automation_peers::IAutomationPeer> m_tpAP;

            bool m_IsLocationValid;
            bool m_isGeneratedContainer;
            // Set to True when the imminent Focus(FocusState) call needs to use an animation if bringing the focused
            // element into view.
            bool m_animateIfBringIntoView;

        public:
            HRESULT get_AccessKeyScopeOwnerImpl(_Outptr_result_maybenull_ xaml::IDependencyObject** ppValue);
            HRESULT put_AccessKeyScopeOwnerImpl(_In_opt_ xaml::IDependencyObject* pValue);

            HRESULT get_LightsImpl(_Outptr_result_maybenull_ wfc::IVector<xaml_media::XamlLight*>** ppValue);

            // Get the UIElement focus candidate given an IInspectable
            // If this inspectable is not a UIElement, we will walk up the tree
            // to get the host UIElement. For example, the candidate could be a hyperlink.
            static _Check_return_ HRESULT GetUIElementFocusCandidate(
                _In_ IInspectable* pCandidate,
                _Out_  ctl::ComPtr<xaml::IUIElement>* pUIElementCandidate);

            _Check_return_ HRESULT STDMETHODCALLTYPE add_PreviewKeyDown(_In_ xaml_input::IKeyEventHandler* pValue, _Out_ EventRegistrationToken* pToken) final;
            _Check_return_ HRESULT STDMETHODCALLTYPE remove_PreviewKeyDown(_In_ EventRegistrationToken token) final;

            _Check_return_ HRESULT STDMETHODCALLTYPE add_PreviewKeyUp(_In_ xaml_input::IKeyEventHandler* pValue, _Out_ EventRegistrationToken* pToken) final;
            _Check_return_ HRESULT STDMETHODCALLTYPE remove_PreviewKeyUp(_In_ EventRegistrationToken token) final;

#if WI_IS_FEATURE_PRESENT(Feature_XamlMotionSystemHoldbacks)
            _Check_return_ HRESULT STDMETHODCALLTYPE add_Shown(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) final;
            _Check_return_ HRESULT STDMETHODCALLTYPE remove_Shown(_In_ EventRegistrationToken token) final;

            _Check_return_ HRESULT STDMETHODCALLTYPE add_Hidden(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* pToken) final;
            _Check_return_ HRESULT STDMETHODCALLTYPE remove_Hidden(_In_ EventRegistrationToken token) final;
#endif

            bool HasShownHiddenHandlers();
            bool HasHiddenHandlers();

        public:
            // nested class that holds virtualization information. We do not create this class indiscriminately, but only
            // for containers that happen to be virtualizing.
            class VirtualizationInformation
            {

            public:
                VirtualizationInformation(_In_ xaml::IUIElement* pOwner);
                ~VirtualizationInformation();

                const wf::Rect& GetBounds() const { return m_bounds; }
                void SetBounds(_In_ const wf::Rect& bounds);

                ctl::ComPtr<IInspectable> GetItem() const;
                _Check_return_ HRESULT SetItem(_In_opt_ IInspectable* pDataitem);

                void SetIsRealized(bool isRealized);
                bool GetIsRealized() const { return m_isRealized; }

                void SetIsGenerated(bool isGenerated) { m_isGeneratedContainer = isGenerated; }
                bool GetIsGenerated() const { return m_isGeneratedContainer; }

                void SetIsHeader(bool isHeader) { m_isHeader = isHeader; }
                bool GetIsHeader() const { return m_isHeader; }

                void SetWasRejectedAsAContainerByApp(_In_ bool wasRejected) { m_wasRejectedAsAContainerByApp = wasRejected; }
                bool GetWasRejectedAsAContainerByApp() const { return m_wasRejectedAsAContainerByApp; }

                _Check_return_ HRESULT GetBuildTreeArgs(_Out_ ctl::ComPtr<xaml_controls::IContainerContentChangingEventArgs>* pspArgs);

                // the size the strategy will measure this container with
                void SetMeasureSize(_In_ const wf::Size& measureSize) { m_measureSize = measureSize; }
                const wf::Size& GetMeasureSize() const { return m_measureSize; }

                ctl::ComPtr<xaml::IDataTemplate> GetSelectedTemplate() const;
                void SetSelectedTemplate(_In_ xaml::IDataTemplate* const pDataTemplate);

                bool GetIsSticky() const { return m_stickyHeaderWrapper != nullptr; }
                std::shared_ptr<StickyHeaderWrapper> m_stickyHeaderWrapper{};

                // Container preparation should only be skippable if we've already prepared the container;
                // otherwise, we'll be using a container that has never been prepared even once, which is bad.
                void SetWantsToSkipContainerPreparation(_In_ bool skipContainerPreparation) { m_wantsToSkipContainerPreparation = skipContainerPreparation; }
                bool GetWantsToSkipContainerPreparation() const { return m_wantsToSkipContainerPreparation; }
                bool GetMaySkipPreparation() const { return m_wantsToSkipContainerPreparation && m_isPrepared; }

                void SetIsPrepared(_In_ bool isPrepared) { m_isPrepared = isPrepared; }
                bool GetIsPrepared() const { return m_isPrepared; }

                void SetIsContainerFromTemplateRoot(bool isFromTemplateRoot) { m_isContainerFromTemplateRoot = isFromTemplateRoot; }
                bool GetIsContainerFromTemplateRoot() const { return m_isContainerFromTemplateRoot; }

            private:
                UIElement* m_pOwner;
                wf::Rect m_bounds{};
                TrackerPtr<IInspectable> m_tpDataItem;
                TrackerPtr<xaml::IDataTemplate> m_tpSelectedTemplate;
                TrackerPtr<xaml_controls::IContainerContentChangingEventArgs> m_tpBuildTreeArgs;
                wf::Size m_measureSize{};
                bool m_isItemLinkedAsOwnContainer{ false };
                bool m_isRealized{ false };
                bool m_isGeneratedContainer{ false };
                bool m_isHeader{ false };
                bool m_wantsToSkipContainerPreparation{ false };
                bool m_isPrepared{ false };
                bool m_wasRejectedAsAContainerByApp{ false };
                bool m_isContainerFromTemplateRoot{ false };
            };

        public:
            // Allocates the virtualization information
            _Check_return_ HRESULT InitVirtualizationInformation();

            // Returns virtualization information, or NULL if it hasn't been created.
            _Check_return_ VirtualizationInformation* GetVirtualizationInformation();

        private:
            std::unique_ptr<VirtualizationInformation> m_pVirtualizationInformation;

#if WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
            ctl::ComPtr<wfc::IVector<xaml::InteractionBase*>> m_interactions;
#endif // WI_IS_FEATURE_PRESENT(Feature_Xaml2018)
    };
}
