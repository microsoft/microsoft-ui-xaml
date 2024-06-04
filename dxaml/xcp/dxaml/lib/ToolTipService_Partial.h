// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//     Service class that provides the system implementation for
//     displaying ToolTips.

#pragma once

#include "WeakReferenceSource.h"
#include "DXamlTypes.h"

// Timer constants for showing/hiding ToolTips.
// The default values for mouse delay and show duration are only used if the calls to fetch those SystemParameters fail.
//  NOTE: GetTickCount() actually measures time in millseconds, but wf::TimeSpan expects ticks.
//  NOTE: 1 Tick == 100 ns == 0.1 us == 0.0001 ms
#define BETWEEN_SHOW_DELAY_MS                   200         // 0.2 seconds, in milliseconds
#define DEFAULT_SPI_GETMOUSEHOVERTIME           400         // 0.4 seconds, in milliseconds
#define DEFAULT_SHOW_DURATION_SECONDS           5           // 5 seconds
#define TICKS_PER_MILLISECOND                   10000       // Number of ticks in a millisecond

constexpr INT64 s_safeZoneCheckTimerDuration = 10000000L;   // 1s

namespace DirectUI
{
    class ToolTip;

    class ToolTipServiceMetadata:
        public ctl::WeakReferenceSource
    {
        public:
            ToolTipServiceMetadata();
            ~ToolTipServiceMetadata() override;

            // Display event is hooked up to avoid SafeZoneCheckTimer introduced performance issue when display is off
            bool m_displayOn { true };

            TrackerPtr<xaml::IDependencyObject> m_tpOwner;
            TrackerPtr<xaml::IFrameworkElement> m_tpContainer;
            TrackerPtr<xaml_controls::IToolTip> m_tpCurrentToolTip;
            TrackerPtr<msy::IDispatcherQueueTimer> m_tpSafeZoneCheckTimer;

            // ToolTipService puts size change and mouse move handlers on the roots elements of each XamlRoot
            // (i.e. RootVisual & Xaml islands). This collection tracks whether a root has handlers on it already.
            std::vector<xaml::IUIElement*> m_rootElementsWithHandlersNoRef;

            // We keep a strong ref to the Popup of the current ToolTip.  Silverlight's ToolTip has its own
            // Popup member, but this creates a circular dependency in C++ when releasing ToolTip, which we can
            // evade by tracking the Popup in ToolTipServiceMetadata instead.
            TrackerPtr<xaml_primitives::IPopup> m_tpCurrentPopup;

            TrackerPtr<IInspectable> m_tpLastEnterSource;
            TrackerPtr<xaml::IDispatcherTimer> m_tpOpenTimer;
            TrackerPtr<xaml::IDispatcherTimer> m_tpCloseTimer;

            // List of nested owners. Owner is added to the list when entered by the pointer, and
            // removed when left by the pointer. This list supports the pointer coming back to an
            // ancestor after it enters and leaves a nested descendant element, without ever
            // leaving the ancestor.
            std::list<ctl::WeakRefPtr>* m_nestedOwners;

            // Because of Safe Zone, ToolTip is kept even if pointer is out side of the control.
            // We need to remove it from m_nestedOwners when toolTip is closed
            ctl::WeakRefPtr m_lastToolTipOwnerInSafeZone;

            // crashes is found in RemoveFromNestedOwners/PurgeInvalidNestedOwners
            // and m_nestedOwners is HEAP CORRUPTION or element in it is empty.
            // One possible reason is nested erase. When we erase one element, it trig another erase
            // and finally make the external iterator invalid.
            // m_isErasingNestedOwners is used to help detect the nested erase.
            bool m_isErasingNestedOwners{ false };

            // Boolean variables used to catch circumstances where we shouldn't
            // perform any list operations immediately - we'll cache any requests
            // to do so in the vectors below to be done once the current operation completes.
            bool m_isAddingToNestedOwners{ false };
            bool m_isRemovingFromNestedOwners{ false };
            bool m_isPurgingInvalidNestedOwners{ false };

            std::vector<ctl::WeakRefPtr> m_objectsToAdd;
            std::vector<ctl::WeakRefPtr> m_objectsToRemove;

            _Check_return_ HRESULT  EnsureNestedOwnersInstance();
            // after delete, it automatically moved to next element.
            _Check_return_ HRESULT  DeleteElementFromNestedOwners(_Inout_ std::list<ctl::WeakRefPtr>::iterator &it);

            // Accessors for the tracker pointers
            void SetCurrentToolTip(_In_ xaml_controls::IToolTip* const pValue)
            {
                SetPtrValue(m_tpCurrentToolTip, pValue);
            }
            void SetCurrentPopup(_In_ xaml_primitives::IPopup* const pValue)
            {
                SetPtrValue(m_tpCurrentPopup, pValue);
            }
            void SetCloseTimer(_In_ xaml::IDispatcherTimer* const pValue)
            {
                SetPtrValue(m_tpCloseTimer, pValue);
            }
            void SetSafeZoneTimer(_In_ msy::IDispatcherQueueTimer* const pValue)
            {
                SetPtrValue(m_tpSafeZoneCheckTimer, pValue);
            }
            void SetOpenTimer(_In_ xaml::IDispatcherTimer* const pValue)
            {
                SetPtrValue(m_tpOpenTimer, pValue);
            }
            void SetOwner(_In_ xaml::IDependencyObject* const pValue)
            {
                SetPtrValue(m_tpOwner, pValue);
            }
            void SetContainer(_In_ xaml::IFrameworkElement* const pValue)
            {
                SetPtrValue(m_tpContainer, pValue);
            }
            void SetLastEnterSource(_In_ IInspectable* const pValue)
            {
                SetPtrValue(m_tpLastEnterSource, pValue);
            }

        private:
            HPOWERNOTIFY m_displayStateHandle{ nullptr };

            static ULONG CALLBACK DisplayStateNotification(_In_ PVOID pvContext, _In_ ULONG, _In_ PVOID pvSetting);
    };


    // Service class that provides the system implementation for displaying ToolTips.
    class ToolTipService
    {
        public:
            static BOOLEAN s_bOpeningAutomaticToolTip;
            static AutomaticToolTipInputMode s_lastEnterInputMode;
            static wf::Point s_lastPointerEnteredPoint;

            // Keyboard opens tooltip too, and most of time, pointer is out of safe zone.
            // we should not start the safe zone check until Pointer is moved
            static POINT s_pointerPointWhenSafeZoneTimerStart;

            static _Check_return_ HRESULT RegisterToolTip(
                _In_ xaml::IDependencyObject* pOwner,
                _In_ xaml::IFrameworkElement* pContainer,
                _In_ IInspectable* pToolTipAsIInspectable,
                _In_ const bool isKeyboardAcceleratorToolTip);

            static _Check_return_ HRESULT RegisterToolTipFromCore(
                _In_ CDependencyObject* owner,
                _In_ CFrameworkElement* container);

            static _Check_return_ HRESULT UnregisterToolTip(
                _In_ xaml::IDependencyObject* pOwner,
                _In_ xaml::IFrameworkElement* pContainer,
                _In_ const bool isKeyboardAcceleratorToolTip);

            static _Check_return_ HRESULT UnregisterToolTipFromCore(
                _In_ CDependencyObject* owner,
                _In_ CFrameworkElement* container);

            static _Check_return_ HRESULT GetKeyboardAcceleratorToolTipStatic(
                _In_ xaml::IDependencyObject* pElement,
                _Outptr_result_maybenull_ IInspectable** ppValue);

            static _Check_return_ HRESULT EnsureHandlersAttachedToRootElement(_In_ VisualTree* visualTree);

            static void OnPublicRootRemoved(_In_ CUIElement* publicRoot);

            static _Check_return_ HRESULT GetToolTipOwnersBoundary(
                _In_ ctl::ComPtr<xaml::IDependencyObject> const& ownerDO,
                _Out_ XRECTF_RB *ownerBounds);

            static _Check_return_ HRESULT HandleToolTipSafeZone(
                _In_ wf::Point point,
                _In_ ctl::ComPtr<xaml::IUIElement> const& toolTip,
                _In_ ctl::ComPtr<xaml::IDependencyObject> const& ownerDO);

            static _Check_return_ HRESULT CloseToolTipInternal(
                _In_opt_ xaml_input::IKeyRoutedEventArgs* pArgs);

            static _Check_return_ HRESULT GetOwner(
                _Outptr_ xaml::IDependencyObject** ppOwner);

            // Add current owner to list of nested owners, sorted by
            // ancestry. The highest ancestor is at the end of the list.
            static _Check_return_ HRESULT AddToNestedOwners(
                _In_ xaml::IDependencyObject* pOwner);

            // If a nested owner has been been removed from the visual tree
            // or made invisible, remove it from the list, because it can
            // no longer display tooltips.
            static _Check_return_ HRESULT PurgeInvalidNestedOwners();

            static _Check_return_ HRESULT GetFirstNestedOwner(
                _Outptr_ xaml::IDependencyObject** pOwner);

            static _Check_return_ HRESULT OnToolTipChanged(
                _In_ xaml::IDependencyObject* pSender,
                _In_ const PropertyChangedParams& args);

            // If there is an automatic ToolTip in the process of opening, stop it from opening.
            // If one is already open, close it.
            // Clear any state associated with the current automatic ToolTip and its owner.
            static _Check_return_ HRESULT CancelAutomaticToolTip();

            static _Check_return_ HRESULT OnOwnerPointerEnteredFromCore(
                _In_ CDependencyObject* sender,
                _In_ CPointerEventArgs* args);

            static _Check_return_ HRESULT OnOwnerPointerExitedOrLostOrCanceledFromCore(
                _In_ CDependencyObject* sender,
                _In_ CPointerEventArgs* args);

            static _Check_return_ HRESULT OnOwnerGotFocusFromCore(
                _In_ CDependencyObject* sender,
                _In_ CRoutedEventArgs* args);

            static _Check_return_ HRESULT OnOwnerLostFocusFromCore(
                _In_ CDependencyObject* sender,
                _In_ CRoutedEventArgs* args);

            static void GetActualToolTipObjectStatic(
                    _In_ xaml::IDependencyObject* pElement,
                    _Outptr_result_maybenull_ xaml_controls::IToolTip** ppValue);

        private:
            static INT64 s_lastToolTipOpenedTime;    // Ticks since last open.

            static _Check_return_ HRESULT OnSafeZoneCheck();

            static _Check_return_ HRESULT StartSafeZoneCheckTimer(
                _In_ ToolTipServiceMetadata* pToolTipServiceMetadataNoRef);

            static bool IsToolTipInSafeZone(
                _In_ wf::Point const& point,
                _In_ XRECTF_RB const& ownerBounds,
                _In_ XRECTF_RB const& toolTipBounds);

            static bool IsPointInRect(
                _In_ wf::Point const& point,
                _In_ XRECTF_RB const& rect);

            static _Check_return_ HRESULT GetGlobalBoundsLogical(
                _In_ ctl::ComPtr<xaml::IUIElement> const& element,
                _Out_ XRECTF_RB* bounds);

            static _Check_return_ HRESULT RemoveFromNestedOwners(
                _In_ xaml::IDependencyObject* pOwner);

            static _Check_return_ HRESULT RunPendingOwnerListOperations(
                _In_ ToolTipServiceMetadata* pToolTipServiceMetadataNoRef);

            static _Check_return_ HRESULT ConvertToToolTip(
                _In_ IInspectable* pIInspectable,
                _Outptr_ xaml_controls::IToolTip** ppReturnValue);

            static BOOLEAN IsSpecialKey(
                _In_ wsy::VirtualKey key);

            // We need to pass args (IInspectable*, IInspectable*) to match IEventHandler.Invoke()'s signature.
            static _Check_return_ HRESULT OpenAutomaticToolTip(
                _In_opt_ IInspectable* pUnused1,
                _In_opt_ IInspectable* pUnused2);

            static _Check_return_ HRESULT CloseAutomaticToolTip(
                _In_opt_ IInspectable* pUnused1,
                _In_opt_ IInspectable* pUnused2);

            static _Check_return_ HRESULT OnOwnerEnterInternal(
                _In_ IInspectable* pSender,
                _In_ IInspectable* pOriginalSource,
                _In_ AutomaticToolTipInputMode mode);

            static _Check_return_ HRESULT OnOwnerLeaveInternal(
                _In_ IInspectable* pSender);

            // Used to handle PointerEntered on a ToolTip's owner FrameworkElement.
            static _Check_return_ HRESULT OnOwnerPointerEntered(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            // Used to handle PointerExited, PointerCaptureLost, and PointerCanceled on a ToolTip's owner FrameworkElement.
            static _Check_return_ HRESULT OnOwnerPointerExitedOrLostOrCanceled(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            // Used to handle GotFocus on a ToolTip's owner FrameworkElement.
            static _Check_return_ HRESULT OnOwnerGotFocus(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Used to handle LostFocus on a ToolTip's owner FrameworkElement.
            static _Check_return_ HRESULT OnOwnerLostFocus(
                _In_ IInspectable* pSender,
                _In_ xaml::IRoutedEventArgs* pArgs);

            // Used to handle PointerMoved on the application root visual FrameworkElement.
            static _Check_return_ HRESULT OnRootVisualPointerMoved(
                _In_ IInspectable* pSender,
                _In_ xaml_input::IPointerRoutedEventArgs* pArgs);

            // Used to handle SizeChanged on the application root visual FrameworkElement.
            static _Check_return_ HRESULT OnRootVisualSizeChanged(
                _In_ IInspectable* pSender,
                _In_ xaml::ISizeChangedEventArgs* pArgs);

            // For a given input mode, returns the initial delay before the ToolTip shows according to spec.
            //
            // There are normal and reshow timers.  The normal timer is used when first opening a ToolTip.
            // The reshow timer is used when a previous ToolTip has been shown within BETWEEN_SHOW_DELAY_MS
            // of invoking this one.
            //
            //          Touch   Mouse   Keyboard
            //  --------------------------------
            //  Normal     1x      2x         2x
            //  Reshow      0    1.5x         2x
            //
            //  where x = SPI_GETMOUSEHOVERTIME (400 ms by default)
            static _Check_return_ HRESULT GetInitialShowDelay(
                _In_ AutomaticToolTipInputMode mode,
                _In_ BOOLEAN isReshow,
                _Out_ wf::TimeSpan* pDelay);

            static _Check_return_ HRESULT GetContainerFromOwner(
                _In_ xaml::IDependencyObject *pOwner,
                _Outptr_ xaml::IFrameworkElement** pContainer);

            static _Check_return_ HRESULT GetDispatcherQueueForCurrentThread(
                _Outptr_ msy::IDispatcherQueue** value);
    };

    // Note that the code assumes full-screen mode and does not work correctly for non-full-screen scenarios.
    namespace ToolTipPositioning
    {
        #define RECTWIDTH(x) ((x).right - (x).left)
        #define RECTHEIGHT(x) ((x).bottom - (x).top)

        const int TYPOGRAPHIC_LARGE = 20;
        const int TYPOGRAPHIC_SMALL = 4;

        // Enables an option to not snap to the typographic grid since ToolTips do not want this behavior.
        // Note: We don't need PS_TYPOGRAPHIC_GRID positioning but I'm keeping it in this code since it's
        // a direct port and we want to modify as little as possible.
        enum POPUP_SNAPPING
        {
            PS_NONE = 0,
            PS_TYPOGRAPHIC_GRID,
        };

        // This class exists to make is difficult to mix containers
        // and rects that need to constrain.
        class CConstraint: public RECT
        {
        public:
            CConstraint() { left = right = top = bottom = 0; }

            CConstraint(
                _In_ INT iLeft,
                _In_ INT iTop,
                _In_ INT iRight,
                _In_ INT iBottom);

            void SetRect(
                _In_ const RECT &rc);

            void SetRect(
                _In_ INT iLeft,
                _In_ INT iTop,
                _In_ INT iRight,
                _In_ INT iBottom);
        };

        static BOOL OffsetRect(
            _Inout_ PRECT prc,
            _In_ const POINT &rc)
        {
            return OffsetRect(prc, rc.x, rc.y);
        }

        SIZE MakeMultipleOfUnit(
            _In_ const CConstraint &constraint,
            _In_ SIZE size,
            _In_range_(>, 0) INT unit);

        BOOL IsLefthandedUser();

        SIZE ConstrainSize(
            _In_ const SIZE &size,
            _In_ const INT xMax,
            _In_ const INT yMax);

        RECT VerticallyCenterRect(
            _In_ const CConstraint &container,
            _In_ const RECT &rcToCenter);

        RECT HorizontallyCenterRect(
            _In_ const CConstraint &container,
            _In_ const RECT &rcToCenter);

        RECT MoveNearRect(
            _In_ const RECT &rcWindow,
            _In_ const RECT &rcWindowToTract,
            _In_ const xaml_primitives::PlacementMode nSide);

        BOOL IsContainedInRect(
            _In_ const CConstraint &container,
            _In_ const RECT &rc);

        RECT MoveRectToPoint(
            _In_ const RECT &rc,
            _In_ INT x,
            _In_ INT y);

        RECT ShiftRectIntoContainer(
            _In_ const CConstraint &container,
            _In_ const RECT &rcToShift);

        BOOL CanPositionRelativeOnSide(
            _In_ const CConstraint &windowToTrack,
            _In_ const RECT &rcWindow, xaml_primitives::PlacementMode nSide,
            _In_ const CConstraint &constrain);

        RECT PositionRelativeOnSide(
            _In_ const CConstraint &windowToTrack,
            _In_ const RECT &rcWindow,
            _In_ xaml_primitives::PlacementMode nSide,
            _In_ const CConstraint &constrain);

        RECT QueryRelativePosition(
            _In_ const CConstraint &constraint,
            _In_ SIZE sizeFlyout,
            _In_ RECT rcDockTo,
            _In_ xaml_primitives::PlacementMode nSidePreferred,
            _In_ POPUP_SNAPPING popupSnapping,
            _Out_ xaml_primitives::PlacementMode* pSideChosen);
    }
}
