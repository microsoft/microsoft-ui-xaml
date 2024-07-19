// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class AgCoreCallbacks;

#include "DiagnosticsInteropModel.h"
#include "AccessKeyEvents.h"
#include "PLMIntegration.h"
#include "Dispatcher.h"
#include "UIAffinityReleaseQueue.h"
#include "InitializationType.h"
#include <fwd/windows.graphics.h>
// wil::unique_registry_watcher_nothrow
#include <wil\registry.h>
#include <fwd/microsoft.ui.xaml.media.h>
#include <microsoft.UI.Dispatching.h>

struct IDCompositionDesktopDevicePartner;

class CJupiterControl;
class CDependencyObject;
class CContentRoot;
class CoreWindowRootScale;
class CUIAWrapper;
struct IPALEvent;
class WindowsPresentTarget;

namespace Parser
{
    struct XamlBuffer;
}

namespace DebugTool
{
    struct IInternalDebugInterop;
}

namespace DirectUI
{
    class UIAffinityReleaseQueue;
    class Application;
    class DependencyObject;
    class DefaultStyles;
    class Window;
    class DragDrop;
    class ApplicationBarService;
    class BuildTreeService;
    class BudgetManager;
    class ToolTipServiceMetadata;
    class FlyoutMetadata;
    class ThemeResourceExpression;
    interface IUntypedEventSource;
    class AutoReentrantReferenceLock;
    class VectorChangedEventArgs;
    class ApplicationBarServiceGenerated;
    class UIElement;
    class AutomaticDragHelper;
    class ElementSoundPlayerService;
    class TouchHitTestingHandler;
    class DxamlCoreTestHooks;
    class NullKeyedResource;
    interface IApplicationBarService;
    class StaticStore;
    class FlyoutBase;
    class TextControlFlyout;
    class XamlRoot;

    class XamlDirect;

    class DXamlCore final : public IDXamlCore, public XAML::PLM::IPLMHandlerCallbacks
    {
        friend class ::AgCoreCallbacks;
        friend class FrameworkElement;
        friend class ctl::WeakReferenceSource;
        friend class DependencyObject;

        friend _Check_return_ HRESULT DirectUI::RegisterUntypedEventSourceInCore(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager);
        friend _Check_return_ HRESULT DirectUI::UnregisterUntypedEventSourceInCore(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager);

        template <class TSOURCE, class THANDLER, class TARGS> friend class CEventSource;
        template <class TSOURCE, class THANDLER, class TARGS> friend class CRoutedEventSource;

    public:
        ~DXamlCore();

        // For some private features such as TileRenderingService on phone, we need to be
        // able to run XAML Background Task without a Windows.UI.Core.CoreApplication object
        static _Check_return_ HRESULT Initialize(_In_ InitializationType initializationType);
        static _Check_return_ HRESULT Deinitialize(_In_ DeinitializationType deinitializeType = DeinitializationType::Default);

        // Should we just make these a IsInState(DXamlCore::State)?
        static bool IsInitializingStatic();
        static bool IsIdleStatic();
        static bool IsInitializedStatic();
        static bool IsShutdownStatic();
        static bool IsShuttingDownStatic();

        // Note: DXamlServices::GetDXamlCore should be preferred to the functions below. This new
        // function returns an IDXamlCore* instead of a DXamlCore*, so any new required functionality
        // should be added to the IDXamlCore interface or one of the interfaces it derives from.
        static DXamlCore* GetCurrent();
        static DXamlCore* GetCurrentNoCreate();

        static DXamlCore* GetFromDependencyObject( DependencyObject *pDO );

        _Check_return_ HRESULT GetDataContextChangedEventArgsFromPool(_In_ IInspectable* pNewDataContext, _Outptr_ xaml::IDataContextChangedEventArgs** ppValue);
        _Check_return_ HRESULT ReleaseDataContextChangedEventArgsToPool(_In_ xaml::IDataContextChangedEventArgs* pValue);

        _Check_return_ HRESULT GetVectorChangedEventArgsFromPool(_Outptr_ VectorChangedEventArgs** ppValue);
        _Check_return_ HRESULT ReleaseVectorChangedEventArgsToPool(_In_ VectorChangedEventArgs* pValue);

        _Check_return_ HRESULT EnsureCoreApplicationInitialized();
        bool ShouldTryLoadAppXaml(const xstring_ptr& appXamlLocation) const;
        _Check_return_ HRESULT ClearCaches();

        _Check_return_ HRESULT NotifyImmersiveColorsChanged();
        _Check_return_ HRESULT OnThemeChanged();
        void OnApplicationFocusVisualKindChanged();
        _Check_return_ HRESULT OnApplicationHighContrastAdjustmentChanged();

        _Check_return_ HRESULT UpdateFontScale(_In_ XFLOAT newFontScale);

        _Check_return_ HRESULT GetLocalizedResourceString(_In_ XUINT32 resourceStringID, _Out_ HSTRING* resourceString);
        _Check_return_ HRESULT GetNonLocalizedResourceString(_In_ XUINT32 resourceStringID, _Out_ HSTRING* resourceString);
        _Check_return_ HRESULT GetNonLocalizedErrorString(_In_ XUINT32 resourceStringID, _Out_ HSTRING* errorString);

        _Check_return_ HRESULT GetResourceBytes(_In_z_ const WCHAR* pszResourceName, _Out_ Parser::XamlBuffer* pBuffer);

        _Check_return_ HRESULT GetDebugInterop(_Outptr_result_maybenull_ DebugTool::IInternalDebugInterop** ppDebugInterop);
        _Check_return_ HRESULT SetDebugInterop(_In_ DebugTool::IInternalDebugInterop* pDebugInterop);

        void SignalWindowMutation(VisualMutationType type);
        CCoreServices* GetHandle() const
        {
            return m_hCore;
        }

        _Check_return_ HRESULT ActivatePeer(_In_ KnownTypeIndex nTypeIndex, _COM_Outptr_ DependencyObject** ppObject);

        _Check_return_ HRESULT GetPeer(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject);

        template <typename PeerType>
        _Check_return_ HRESULT GetPeer(_In_ CDependencyObject* pDO, _COM_Outptr_ PeerType** ppObject)
        {
            ctl::ComPtr<DependencyObject> peer;
            IFC_RETURN(GetPeer(pDO, peer.ReleaseAndGetAddressOf()));
            return peer.CopyTo(ppObject);
        }

        _Check_return_ HRESULT GetPeerWithInternalRef(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT GetPeer(_In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT GetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT GetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT GetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _In_ bool fCreatePegged, _COM_Outptr_ DependencyObject** ppObject);

        template <typename PeerType>
        _Check_return_ HRESULT TryGetPeer(_In_ CDependencyObject* pDO, _COM_Outptr_ PeerType** ppObject)
        {
            *ppObject = nullptr;
            ctl::ComPtr<DependencyObject> peer;
            IFC_RETURN(TryGetPeer(pDO, peer.ReleaseAndGetAddressOf()));
            if (peer)
            {
                IFC_RETURN(peer.CopyTo(ppObject));
            }
            return S_OK;
        }

        _Check_return_ HRESULT TryGetPeer(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT TryGetPeerWithInternalRef(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT TryGetPeer(_In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT TryGetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT TryGetPeer(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ KnownTypeIndex hClass, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT TryGetPeer(_In_ CDependencyObject* pDO, _Out_ bool *pIsPendingDelete, _COM_Outptr_ DependencyObject** ppObject);
        _Check_return_ HRESULT TryGetOrCreatePeer(_In_ CDependencyObject* pDO, _COM_Outptr_ DependencyObject** ppObject);

        void RemovePeer(_In_ DependencyObject* pDO);

        // Begin IDXamlCore

        bool IsInitializing() const override { return m_state == Initializing; }
        bool IsInitialized() override { return m_state == Initialized; }
        bool IsShutdown() override { return m_state == Deinitializing || m_state == Deinitialized; }

        bool IsShuttingDown() { return m_state == Deinitializing; }
        bool IsIdle() { return m_state == Idle; }

        PeerTable& GetPeers() override { return m_Peers; }
        ReferenceTrackerTable& GetReferenceTrackers() override { return m_ReferenceTrackers; }
        void AddToReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem) override;
        void RemoveFromReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem) override;
        _Check_return_ HRESULT ShutdownAllPeers() override;

        // These two methods handle objects that have been Released on another thread, but
        // need to be cleaned up on this Core's UI thread
        void QueueObjectForUnreachableCleanup(_In_ ctl::WeakReferenceSourceNoThreadId *pItem) override;
        void QueueObjectForFinalRelease(_In_ ctl::WeakReferenceSourceNoThreadId *pItem) override;

        BOOLEAN IsFinalReleaseQueueEmpty() override;
        HRESULT GetFinalReleaseQueue(_Outptr_ wfc::IVectorView<IInspectable*>** queue);

        void ReferenceTrackerWalkOnCoreGCRoots(_In_ EReferenceTrackerWalkType walkType) override;

        UINT32 GetThreadId() override
        {
            return m_threadId;
        }

#if DBG
        void SetTrackerPtrsNeedValidation(BOOLEAN value) override { m_fTrackerPtrsNeedValidation = value; }
        BOOLEAN GetTrackerPtrsNeedValidation() override { return m_fTrackerPtrsNeedValidation; }

        bool IsInReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* pItem) override
        {
            if (m_state == Deinitialized)
            {
                return false;
            }
            return m_ReferenceTrackers.find(pItem) != m_ReferenceTrackers.end();
        }
#endif

        // Make the lock available to the ReferenceTrackerManager
        SRWLOCK& GetReferenceSrwLock() override { return m_peerReferenceLock; }


        LONG IncrementReferenceLockEnterCount() override { return ++m_cReferenceLockEnters; }
        LONG DecrementReferenceLockEnterCount() override { return --m_cReferenceLockEnters; }
        LONG GetReferenceLockEnterCount() override { return m_cReferenceLockEnters; }

        // Generates runtimeIds uniquely across the board for UIA.
        UINT32 GenerateRawElementProviderRuntimeId() override;

        // Methods for accessing memory diagnostic information that is
        // determined via ReferenceTrackerWalk
        std::uint64_t GetRTWElementCount() const override { return m_rtwElementCount; }
        void SetRTWElementCount(std::uint64_t count) override { m_rtwElementCount = count; }
        std::uint64_t GetRTWTotalCompressedImageSize() const override { return m_rtwCompressedImageSize; }
        void SetRTWTotalCompressedImageSize(std::uint64_t imageSize) override { m_rtwCompressedImageSize = imageSize; }

        // End IDXamlCore

        _Check_return_ HRESULT RunCoreWindowMessageLoop();

        _Check_return_ HRESULT ConfigureJupiterWindow(_In_opt_ wuc::ICoreWindow* pCoreWindow);

        wgrd::IDisplayInformation* GetDisplayInformationNoRef();

        enum State
        {
            Initializing,
            InitializationFailed,
            Initialized,
            Deinitializing,
            Deinitialized,
            Idle,
        };

        State GetState()
        {
            return m_state;
        }

        // Note: the returned Window is not addref'd, and may be NULL if GetWindow()
        // is called during core shutdown. Callers should check for null (unless the call
        // is guaranteed to never happen during a shutdown scenario) and handle it.
        //
        // This function is deprecated and actively being removed.
        Window* GetDummyWindowNoRef()
        {
            return m_uwpWindowNoRef;
        }

        // GetUwpWindowNoRef and GetDummyWindowNoRef exist as a way to isolate behaviors in the framework
        // which use GetWindow(). Those in the context of UWP to mean "get me my actual window" vs those which
        // (outside of UWP) use it as a work-around meaning "get me the dummy window so I can fake a behavior".
        // When UWP support is removed from the codebase, any logic referencing this function
        // should be removed. This function will fail when called outside of UWP context.
        Window* GetUwpWindowNoRef();

        // Attempts to retrieve the Window instance which the given UIElement belongs to.
        // This function will only work after all UIElements are loaded into a VisualTree.
        // Do not attempt to call this function during initialization or state preparation
        // for an instance of UIElement. To attach to Window events as early as possible,
        // listen for IFrameworkElement::Loaded and invoke this function in the event handler.
        //
        // NOTE: this function signature is written with MultiWindow support for both
        // desktop apps and UWP apps in mind. To that end, the 1st parameter is required
        // even though it _technically_ isn't needed for the existing UWP functionality.
        _Check_return_ HRESULT GetAssociatedWindowNoRef(
            _In_ UIElement* element,
            _Outptr_result_maybenull_  Window** windowNoRef);

        // Returns a VectorView of all Window instances known to DXamlCore
        // TODO: This function cannot fail. Function signature should be changed to
        // const std::vector<Window*>& DXamlCore::GetAllWindows and maintain this vector internally.
        // this should be a somewhat simple change technically speaking, but will complete that as part of the next iteration
        // as it has some design implications and should solicit feedback seperately.
        _Check_return_ HRESULT GetAllWindows(
            _Out_ std::vector<Window*>&  windowsNoRef);

        bool HasDesktopWindow() const;

        //
        // IMPORTANT: Don't use CoreDispatcher in XAML framework code.
        //
        // CoreDispatcher is exposed through our API for apps to use,
        // but we need to avoid taking any internal dependencies on it,
        // because it's not available in all environments we support.
        //
        // Instead, use DirectUI::IDispatcher. This is available through:
        //     DXamlCore::GetXamlDispatcher()
        //     DependencyObject::GetXamlDispatcher()
        //
        wuc::ICoreDispatcher* GetCoreDispatcherNoRef()
        {
            return m_pDispatcher;
        }

        //
        // IMPORTANT: Don't use CoreDispatcher in XAML framework code.
        //
        // CoreDispatcher is exposed through our API for apps to use,
        // but we need to avoid taking any internal dependencies on it,
        // because it's not available in all environments we support.
        //
        // Instead, use DirectUI::IDispatcher. This is available through:
        //     DXamlCore::GetXamlDispatcher()
        //     DependencyObject::GetXamlDispatcher()
        //
        _Check_return_ HRESULT GetCoreDispatcher(_Outptr_result_maybenull_ wuc::ICoreDispatcher** ppDispatcher)
        {
            *ppDispatcher = m_pDispatcher;
            AddRefInterface(m_pDispatcher);
            RRETURN(S_OK);
        }

        msy::IDispatcherQueue* GetDispatcherQueueNoRef();

        _Check_return_ HRESULT GetDispatcherQueue(_Outptr_ msy::IDispatcherQueue** ppDispatcher);

        _Check_return_ HRESULT GetFocusManagerGotFocusEventSource(_Outptr_
            CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>** ppEventSource);
        _Check_return_ HRESULT GetFocusManagerLostFocusEventSource(_Outptr_
            CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>** ppEventSource);
        _Check_return_ HRESULT GetFocusManagerGettingFocusEventSource(_Outptr_
            CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>** ppEventSource);
        _Check_return_ HRESULT GetFocusManagerLosingFocusEventSource(_Outptr_
            CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>** ppEventSource);
        _Check_return_ HRESULT GetRenderingEventSource(_Outptr_
            CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>** ppEventSource);
        _Check_return_ HRESULT GetRenderedEventSource(_Outptr_
            CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>** ppEventSource);
        _Check_return_ HRESULT GetSurfaceContentsLostEventSource(_Outptr_
            CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>** ppEventSource);

        _Check_return_ HRESULT GetFocusedElementRemovedEventSource(_Outptr_
            CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>** eventSource);

        CDependencyObject* GetCoreAppHandle()
        {
            return m_pDOCoreApp;
        }

        void ReleaseQueuedObjects( BOOLEAN bSync = TRUE );

        ElementSoundPlayerService* TryGetElementSoundPlayerServiceNoRef() const;
        ElementSoundPlayerService* GetElementSoundPlayerServiceNoRef();

        _Check_return_ HRESULT GetBuildTreeService(_Out_ ctl::ComPtr<BuildTreeService>& spService);

        _Check_return_ HRESULT GetUISettings(_Out_ ctl::ComPtr<wuv::IUISettings>& spUISettings);

        _Check_return_ HRESULT GetBudgetManager(_Out_ ctl::ComPtr<BudgetManager>& spManager);

        // Can return a null pToolTipServiceMetadata if and only if createIfNeeded is false
        _Check_return_ HRESULT GetToolTipServiceMetadata(
            _Out_ ToolTipServiceMetadata*& pToolTipServiceMetadata,
            bool createIfNeeded = true);

        _Check_return_ HRESULT GetFlyoutMetadata(_Out_ FlyoutMetadata** ppFlyoutMetadata);

        _Check_return_ HRESULT GetRadioButtonGroupsByName(
            _In_ bool ensure,
            _Out_ xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*>*& pRadioButtonGroupsByName);

        // Obtains the instance of DragDrop specific to this DXamlCore.
        // DragDrop is responsible for initiating and managing drag and drop operations.
        DragDrop* GetDragDrop()
        {
            EnsureDragDrop();
            return m_pDragDrop;
        }

        // no-op if helper is already in the map
        void SetAutomaticDragHelper(_In_ UIElement* uielement, _In_ AutomaticDragHelper* helper);
        AutomaticDragHelper* GetAutomaticDragHelper(_In_ UIElement* uielement);

        // no-op if helper is already in the map
        void SetTextControlFlyout(_In_ FlyoutBase* flyout, _In_ TextControlFlyout* helper);
        TextControlFlyout* GetTextControlFlyout(_In_opt_ CFlyoutBase* flyout) const;

        void SetIsWinRTDndOperationInProgress(bool inProgress);
        static bool IsWinRTDndOperationInProgress();

        DefaultStyles* GetDefaultStyles()
        {
            return m_pDefaultStyles;
        }

        bool IsInBackgroundTask() const;

        xstring_ptr GetGenericXamlFilePathFromMUX()
        {
            if (m_genericXamlFilePathFromMUX.IsNull() && m_shouldCheckGenericXamlFilePathFromMUX)
            {
                TrySetGenericXamlFilePathFromMUX(XSTRING_PTR_EPHEMERAL(L"ms-appx:///Microsoft.UI.Xaml.Controls/Themes/generic.xaml"));
            }

            return m_genericXamlFilePathFromMUX;
        }

        void ClearGenericXamlFilePathFromMUX()
        {
            m_genericXamlFilePathFromMUX.Reset();
        }

        const WCHAR* GetGenericXamlFilePath()
        {
            if (!m_genericXamlFilePathFromMUX.IsNull())
            {
                return m_genericXamlFilePathFromMUX.GetBuffer();
            }

            return nullptr;
        }

        void TrySetGenericXamlFilePathFromMUX(const xstring_ptr_view& strUri);

        _Check_return_ HRESULT ActivateWindow();

        _Check_return_ HRESULT NotifyFirstFramePending();

        _Check_return_ HRESULT CreateProviderForAP(_In_ CAutomationPeer* pAP, _Outptr_result_maybenull_ CUIAWrapper** ppRet);

        void PhysicalScreenToClient(_Inout_ POINT* physicalPoint);

        void ScreenToClient(_Inout_ wf::Point* pDipPoint);
        void ClientToScreen(_Inout_ wf::Point* pDipPoint);

        void ScreenToClient(_Inout_ wf::Rect* pDipRect);
        void ClientToScreen(_Inout_ wf::Rect* pDipRect);

        void ScreenToClient(_Inout_ RECT* pPhysicalPixelsRect);
        void ClientToScreen(_Inout_ RECT* pPhysicalPixelsRect);

        // returns the PLM handler for this core, or NULL if there is no PLM handler
        // if this isn't a packaged process, there won't be a PLM handler
        XAML::PLM::PLMHandler* GetPLMHandler()
        {
            return m_pPLMHandler;
        }

        // IPLMHandlerCallbacks
        _Check_return_ HRESULT OnBeforeAppSuspend() override;
        _Check_return_ HRESULT OnAfterAppSuspend() override;
        _Check_return_ HRESULT OnBeforeAppResume() override;

        void PhysicalPixelsToDips(_In_ float scale, _In_ SIZE* pSize, _Out_ wf::Rect* pRect);
        void PhysicalPixelsToDips(_In_ float scale, _In_ RECT* pRectIn, _Out_ wf::Rect* pRectOut);
        void PhysicalPixelsToDips(_In_ float scale, _In_ POINT* pPhysicalPoint, _Out_ wf::Point* pDipPoint);
        void PhysicalPixelsToDips(_In_ float scale, _In_ wf::Point* pPhysicalPoint, _Out_ wf::Point* pDipPoint);
        void PhysicalPixelsToDips(_In_ float scale, _In_ wf::Rect* pPhysicalRect, _Out_ wf::Rect* pDipRect);

        void DipsToPhysicalPixels(_In_ float scale, _In_ wf::Point* pDipPoint, _Out_ POINT* pPhysicalPoint);
        void DipsToPhysicalPixels(_In_ float scale, _In_ wf::Point* pDipPoint, _Out_ wf::Point* pPhysicalPoint);
        void DipsToPhysicalPixels(_In_ float scale, _In_ wf::Rect* pDipRect, _Out_ wf::Rect* pPhysicalRect);

        // Gets the XAML dispatcher associated with this core - callable from any thread
        IDispatcher* GetXamlDispatcherNoRef();
        _Check_return_ HRESULT GetXamlDispatcher(_Out_ ctl::ComPtr<IDispatcher>* pspDispatcher) override;

        void RegisterInputPaneHandler(_In_ wuc::ICoreWindow * coreWindow);

        void OnWindowDestroyed(_In_ HWND hwndDestroyed);

        _Check_return_ HRESULT XamlPalSetCoreWindow();

        // Page waiter event Apis
        _Check_return_ HRESULT RetrievePageNavigationCompleteEvent();
        bool HasPageNavigationCompleteEvent();
        _Check_return_ HRESULT SetPageNavigationCompleteEvent();

        void SetAtlasSizeHint(XUINT32 width, XUINT32 height);
        void ResetAtlasSizeHint();
        void DisableAtlas();

        WindowsPresentTarget* GetCurrentPresentTargetNoRef();

        void GetTransparentBackground(_Out_ bool* pIsTransparent);
        void SetTransparentBackground(bool isTransparent);

        static _Check_return_ HRESULT ForwardWindowedPopupMessageToJupiterWindow(
                _In_ HWND window,
                _In_ UINT message,
                _In_ WPARAM wParam,
                _In_ LPARAM lParam,
                _In_opt_ CContentRoot* contentRoot,
                _Out_ LRESULT *pResult);

        // Calcuates the available monitor bounds that Flyout or ToolTip can be shown with
        // the windowed Popup.
        _Check_return_ HRESULT CalculateAvailableMonitorRect(
            _In_ UIElement* pTargetElement,
            _In_ wf::Point targetPointClientDips,
            _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
            _Out_opt_ wf::Point* screenOffset = nullptr,
            _Out_opt_ wf::Point* targetPointScreenPhysical = nullptr,
            _Out_opt_ wf::Rect* inputPaneOccludeRectScreenLogical = nullptr);
        _Check_return_ HRESULT CalculateAvailableMonitorRect(
            _In_ CUIElement* pTargetElement,
            _In_ wf::Point targetPointClientDips,
            _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
            _Out_opt_ wf::Point* screenOffset = nullptr,
            _Out_opt_ wf::Point* targetPointScreenPhysical = nullptr,
            _Out_opt_ wf::Rect* inputPaneOccludeRectScreenLogical = nullptr);

        _Check_return_ HRESULT GetInputPaneOccludeRect(
            _In_ UIElement* element,
            _Out_ wf::Rect* inputPaneOccludeRectInDips);

        _Check_return_ HRESULT GetInputPaneOccludeRect(
            _In_ CUIElement* element,
            _Out_ wf::Rect* inputPaneOccludeRectInDips);

        _Check_return_ HRESULT GetInputPaneOccludeRect(
            _In_ CUIElement* element,
            _In_ XamlRoot* xamlRoot,
            _Out_ wf::Rect* inputPaneOccludeRectInDips);

        AccessKeys::AKEvents& AccessKeyEvents() { return m_akEvents; }

        CJupiterControl* GetControl() const { return m_pControl; }

        static bool CompositionTarget_HasHandlers();

        _Check_return_ HRESULT GetContentBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue);
        _Check_return_ HRESULT GetContentLayoutBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue);
        _Check_return_ HRESULT GetVisibleContentBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue);
        _Check_return_ HRESULT GetVisibleContentBoundsForElement(_In_ bool ignoreIHM, _In_ bool inDesktopCoordinates, _In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue);
        bool TryGetXamlIslandBoundsForElement(_In_opt_ CDependencyObject* dependencyObject, _Out_ wf::Rect* pValue);

        bool GetIsKeyboardPresent();

        ctl::ComPtr<DirectUI::NullKeyedResource> GetCachedNullKeyedResource();

        xaml::DispatcherShutdownMode GetDispatcherShutdownMode() const { return m_dispatcherShutdownMode; }
        void SetDispatcherShutdownMode(xaml::DispatcherShutdownMode value) { m_dispatcherShutdownMode = value; }

#pragma region IXamlTestHooks

        _Check_return_ HRESULT SimulateDeviceLost(bool resetVisuals, bool resetDManip);

        void GetDCompDevice(
            _Outptr_ IDCompositionDesktopDevicePartner **ppDCompDevice
            ) const;

        _Check_return_ HRESULT SetWindowSizeOverride(
            _In_ const XSIZE *pWindowSize,
            XFLOAT testOverrideScale
            );

        void SetHdrOutputOverride(bool isHdrOutputOverride);
        BOOLEAN GetWantsRenderingEvent();
        bool GetWantsCompositionTargetRenderedEvent();

        void SetPostTickCallback(_In_opt_ std::function<void()> callback);

        void OverrideTrimImageResourceDelay(bool enabled);

        // Set custom font collection in core text services for testing.
        _Check_return_ HRESULT SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection);

        void RequestReplayPreviousPointerUpdate_TempTestHook();

        void SimulateSuspendToPauseAnimations();
        void SimulateResumeToResumeAnimations();
        bool IsRenderingFrames();
        void SetIsSuspended(bool isSuspended);
        void SetIsRenderEnabled(bool value);
        void SetTimeManagerClockOverrideConstant(double newTime);
        HRESULT CleanUpAfterTest();

        void ForceDisconnectRootOnSuspend(bool forceDisconnectRootOnSuspend);
        void TriggerSuspend(bool isTriggeredByResourceTimer, bool allowOfferResources);
        void TriggerResume();
        void TriggerLowMemory();

        void SimulateThemeChanged();

        BOOLEAN InjectWindowMessage(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _In_ xaml::IXamlRoot* xamlRoot);

        HRESULT WaitForCommitCompletion();

        void SetForceIsFullScreen(_In_ bool forceIsFullScreen) { m_forceIsFullScreen = forceIsFullScreen; }

        // This method is meant to be run after tests and we've called DeinitializeInstanceToIdle. The intention is that
        // there are certain things that we need to mark with the LeakIgnoringAllocator (mainly, collections). We want to
        // do this is a little as possible, but if we do, we use this method to make sure those objects (collections) are empty.
        void CheckForLeaks();

        void SetIsHolographic(bool value);

        void SetMockUIAClientsListening(bool isEnabledMockUIAClientsListening);

        void SetGenericXamlFilePathForMUX(const xstring_ptr_view& filePath);
        void SetThreadingAssertOverride(bool enabled);

        _Check_return_ HRESULT OnCompositionContentStateChangedForUWP();
        _Check_return_ HRESULT OnUWPWindowSizeChanged();

        static _Check_return_ HRESULT SetBinding(
            _In_ IInspectable* source,
            _In_ HSTRING pathString,
            _In_ xaml::IDependencyObject* target,
            KnownPropertyIndex targetPropertyIndex,
            _In_opt_ xaml_data::IValueConverter* converter = nullptr);

        static _Check_return_ HRESULT SetBindingCore(
            _In_ CDependencyObject* source,
            _In_ HSTRING path,
            _In_ CDependencyObject* target,
            KnownPropertyIndex targetPropertyIndex);

        void RegisterForChangeVisualStateOnDynamicScrollbarsSettingChanged(_In_ Control* control);
        void UnregisterFromDynamicScrollbarsSettingChanged(_In_ Control* element);
        _Check_return_ HRESULT OnAutoHideScrollbarsChanged();
        void RemoveAutoHideScrollBarsChangedHandler();
        _Check_return_ HRESULT AddAnimationsEnabledChangedHandler();
        void RemoveAnimationsEnabledChangedHandler();
        _Check_return_ HRESULT UpdateAnimationsEnabled();

        static bool ShouldUseDynamicScrollbars();
        static bool ShouldUseDynamicScrollbars_CheckRegistryKey();

        ctl::ComPtr<DxamlCoreTestHooks> GetTestHooks();
#pragma endregion

        XamlDirect* GetXamlDirectDefaultInstanceNoRef();

        static void DisableNotifyEndOfReferenceTrackingOnThread()
        {
            s_enableNotifyEndOfReferenceTrackingOnThread = false;
        }

        static bool NotifyEndOfReferenceTrackingOnThread()
        {
            return s_enableNotifyEndOfReferenceTrackingOnThread;
        }

        containers::vector_map<HWND, DirectUI::Window*> m_handleToDesktopWindowMap;

    private:
        _Check_return_ HRESULT InitializeInstance(_In_ InitializationType initializationType);
        _Check_return_ HRESULT InitializeInstanceForXbf();
        _Check_return_ HRESULT InitializeInstanceFromIdle();
        _Check_return_ HRESULT DeinitializeInstance();
        _Check_return_ HRESULT DeinitializeInstanceToIdle();

        void RegisterForChangeVisualStateOnDynamicScrollbarsRegistryKeySettingChanged();
        _Check_return_ HRESULT RegistryUpdatedCallback();
        _Check_return_ HRESULT UpdateVisualStateForConsciousScrollbar();

        void ReleaseWindow();
        _Check_return_ HRESULT CommonShutdown();

        _Check_return_ HRESULT EnsureEventArgs();

        _Check_return_ HRESULT RegisterVisualDiagnosticsPort();
        void UnregisterVisualDiagnosticsPort();
        static _Check_return_ HRESULT CALLBACK OnVisualDiagInit(_In_ void * userContext, _In_ const void * pPayload, _In_ int size);

        typedef wf::IEventHandler<wa::SuspendingEventArgs*> SuspendingEventHandler;
        _Check_return_ HRESULT add_Suspending(_In_ SuspendingEventHandler* pValue, _Out_ EventRegistrationToken* ptToken);
        _Check_return_ HRESULT add_Resuming(_In_ wf::IEventHandler<IInspectable*>* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT add_LeavingBackground(_In_ wf::IEventHandler<wa::LeavingBackgroundEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken);
        _Check_return_ HRESULT add_EnteredBackground(_In_ wf::IEventHandler<wa::EnteredBackgroundEventArgs*>* pHandler, _Out_ EventRegistrationToken* pToken);

        DXamlCore();

        void EnsureDragDrop();
        void DeleteDragDrop();

        // This is used as a parameter to GetPeerPrivate
        enum class GetPeerPrivateCreateMode
        {
            // Return the peer only if it already exists
            GetOnly,

            // Return an existing peer, or create a new one
            CreateIfNecessary,

            // Return an existing peer, or if possible create a new one
            TryCreateIfNecessary
        };

        _Check_return_ HRESULT GetPeerPrivate(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ GetPeerPrivateCreateMode bCreateIfNecessary, _In_ bool bInternalRef, _Out_opt_ bool *pIsPendingDelete, _Out_ DependencyObject** ppObject);
        _Check_return_ HRESULT GetPeerPrivate(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ GetPeerPrivateCreateMode bCreateIfNecessary, _In_ KnownTypeIndex hClass, _In_ bool bInternalRef, _Out_opt_ bool *pIsPendingDelete, _Out_ DependencyObject** ppObject);
        _Check_return_ HRESULT GetPeerPrivate(_In_opt_ IInspectable* pOuter, _In_ CDependencyObject* pDO, _In_ GetPeerPrivateCreateMode bCreateIfNecessary, _In_ KnownTypeIndex hClass, _In_ bool bInternalRef, _In_ bool bCreatePegged, _Out_opt_ bool *pIsPendingDelete, _Out_ DependencyObject** ppObject);

        _Check_return_ HRESULT RegisterEventSource(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager);
        _Check_return_ HRESULT UnregisterEventSource(_In_ IUntypedEventSource* pEventSource, _In_ bool bUseEventManager);
        _Check_return_ HRESULT RegisterLayoutUpdatedEventSource(_In_ FrameworkElement *pFrameworkElement);
        void UnregisterLayoutUpdatedEventSource(_In_ FrameworkElement *pFrameworkElement);
        _Check_return_ HRESULT FireEvent(_In_ CDependencyObject* pCoreListener, _In_ KnownEventIndex eventId, _In_ CDependencyObject* pCoreSender, _In_opt_ CEventArgs* pCoreArgs, _In_ XUINT32 flags);
        _Check_return_ HRESULT RaiseEvent(_In_ CDependencyObject* target, _In_opt_ CEventArgs* pCoreArgs, _In_ ManagedEvent eventId);
        _Check_return_ HRESULT OnLayoutUpdated(_In_ IInspectable* pArgs);
        _Check_return_ HRESULT OnFocusManagerGotFocusEvent(_In_ xaml_input::IFocusManagerGotFocusEventArgs* pArgs);
        _Check_return_ HRESULT OnFocusManagerLostFocusEvent(_In_ xaml_input::IFocusManagerLostFocusEventArgs* pArgs);
        _Check_return_ HRESULT OnFocusManagerGettingFocusEvent(_In_ xaml_input::IGettingFocusEventArgs* pArgs);
        _Check_return_ HRESULT OnFocusManagerLosingFocusEvent(_In_ xaml_input::ILosingFocusEventArgs* pArgs);
        _Check_return_ HRESULT OnRenderingEvent(_In_ IInspectable* pArgs);
        _Check_return_ HRESULT OnRenderedEvent(_In_ xaml_media::IRenderedEventArgs* pArgs);
        _Check_return_ HRESULT OnSurfaceContentsLostEvent(_In_ IInspectable* pArgs);
        _Check_return_ HRESULT OnFocusedElementRemovedEvent(_In_ xaml_input::IFocusedElementRemovedEventArgs* pArgs);

        _Check_return_ HRESULT SetCoreWindow(_In_ wuc::ICoreWindow* pCoreWindow);

        LRESULT ForwardWindowedPopupMessageToJupiterWindow(
            _In_ HWND window,
            _In_ UINT message,
            _In_ WPARAM wParam,
            _In_ LPARAM lParam,
            _In_opt_ CContentRoot* contentRoot);

        static _Check_return_ HRESULT SetCustomProperty(
            _In_ CDependencyObject* pNativeTarget,
            _In_ XUINT32 customPropertyID,
            _In_ CValue* pPropertyValue,
            _In_ XUINT32 typeIndex,
            _In_ bool fAnimatedValue);

        static _Check_return_ HRESULT InitializeImpl(_In_ InitializationType initializationType);

        State m_state {State::Deinitialized};

        // This lock is used to coordinate between application threads and reference-tracking (during GC).
        // It is acquired for shared access by application threads, shared so that it can be re-entered by the thread like a critical section.
        // It is acquired for exclusive access during reference tracking, so that references are not modified during tracking.
        SRWLOCK m_peerReferenceLock;
        LONG m_cReferenceLockEnters {0};

        CCoreServices* m_hCore {nullptr};
        CJupiterControl* m_pControl {nullptr};
        DefaultStyles* m_pDefaultStyles {nullptr};    // Cached default control styles
        ctl::ComPtr<xaml::IDataContextChangedEventArgs> m_spDataContextChangedEventArgs;
        ctl::ComPtr<wfc::IVectorChangedEventArgs> m_spVectorChangedEventArgs;

        PeerTable m_Peers;
        ReferenceTrackerTable m_ReferenceTrackers;

        // map of IWeakReference of IFrameworkElements. IFrameworkElement key is used as handle and should never be deref'ed.
        containers::vector_map<HANDLE, ctl::WeakRefPtr> m_LayoutUpdatedEventSources;
        ctl::ComPtr<ApplicationBarServiceGenerated> m_spApplicationBarService;
        ctl::ComPtr<ElementSoundPlayerService> m_elementSoundPlayerService;
        ctl::ComPtr<BuildTreeService> m_spBuildTreeService;
        ctl::ComPtr<BudgetManager> m_spBudgetManager;
        ctl::ComPtr<wuv::IUISettings> m_spUISettings;
        ToolTipServiceMetadata* m_pToolTipServiceMetadata {nullptr};
        ctl::ComPtr<FlyoutMetadata> m_spFlyoutMetadata;
        Window* m_uwpWindowNoRef {nullptr};
        ctl::ComPtr<wgrd::IDisplayInformation> m_displayInformation;
        bool m_isDisplayInformationInitialized {false};
        wuc::ICoreDispatcher* m_pDispatcher {nullptr};
        CDependencyObject* m_pDOCoreApp {nullptr};
        DragDrop* m_pDragDrop {nullptr};
        std::map<UIElement*, std::unique_ptr<AutomaticDragHelper>> m_autoDragHelperMap;
        containers::vector_map<FlyoutBase*, std::unique_ptr<TextControlFlyout>> m_textControlFlyoutHelperMap;
        CEventSource<wf::IEventHandler<xaml_input::FocusManagerGotFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerGotFocusEventArgs>* m_pFocusManagerGotFocusEvent {nullptr};
        CEventSource<wf::IEventHandler<xaml_input::FocusManagerLostFocusEventArgs*>, IInspectable, xaml_input::IFocusManagerLostFocusEventArgs>* m_pFocusManagerLostFocusEvent {nullptr};
        CEventSource<wf::IEventHandler<xaml_input::GettingFocusEventArgs*>, IInspectable, xaml_input::IGettingFocusEventArgs>* m_pFocusManagerGettingFocusEvent {nullptr};
        CEventSource<wf::IEventHandler<xaml_input::LosingFocusEventArgs*>, IInspectable, xaml_input::ILosingFocusEventArgs>* m_pFocusManagerLosingFocusEvent {nullptr};
        CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* m_pCompositionTargetRenderingEvent {nullptr};
        CEventSource<wf::IEventHandler<xaml_media::RenderedEventArgs*>, IInspectable, xaml_media::IRenderedEventArgs>* m_pCompositionTargetRenderedEvent {nullptr};
        CEventSource<wf::IEventHandler<IInspectable*>, IInspectable, IInspectable>* m_pCompositionTargetSurfaceContentsLostEvent {nullptr};
        CEventSource<xaml_input::IFocusedElementRemovedEventHandler, IInspectable, xaml_input::IFocusedElementRemovedEventArgs>* m_focusedElementRemovedEvent {nullptr};

        ctl::ComPtr<DirectUI::NullKeyedResource> m_spCachedNullKeyedResource;

        wrl::WeakRef m_wpDebugInterop;

        UINT32 m_threadId = ::GetCurrentThreadId();

        // Core-wide mapping of namespace names to NamespaceInfos used by RadioButton and
        // the FocusedIndices in those namespaces.
        xchainedmap<xstring_ptr, std::list<ctl::WeakRefPtr>*>* m_pRadioButtonGroupsByName {nullptr};

        #if DBG
        private:
        BOOLEAN m_fTrackerPtrsNeedValidation {};
        #endif

        ctl::ComPtr<UIAffinityReleaseQueue> m_spReleaseQueue;

        EventRegistrationToken m_compositionIslandTransformChangedToken = {};
        XAML::PLM::PLMHandler* m_pPLMHandler {nullptr};

        TouchHitTestingHandler* m_pTouchHitTestingHandler {nullptr};

        ctl::ComPtr<DispatcherImpl> m_spDispatcherImpl;

        AccessKeys::AKEvents m_akEvents;

        xstring_ptr m_genericXamlFilePathFromMUX;
        bool m_shouldCheckGenericXamlFilePathFromMUX {true};

        IPALEvent *m_pPageNavigationCompleteEvent {nullptr};

        // Visual Diagnostics
        WCHAR                           m_pszVisualDiagEndpointName[35] = {0};
        static std::atomic<DWORD>       s_instanceCount;

        // Data for xaml test hooks
        bool m_forceIsFullScreen {false};

        // Cached pointer to keyboard capabilities object.
        ctl::ComPtr<wdei::IKeyboardCapabilities> m_keyboardCapabilities;

        ixp::IContentIsland* m_coreWindowContentNoRef {nullptr};
        std::uint64_t m_rtwElementCount {0};
        std::uint64_t m_rtwCompressedImageSize {0};
        ctl::ComPtr<DxamlCoreTestHooks> m_spTestHooks;

        std::vector<Control*> m_registeredControlsForSettingsChanged;
        EventRegistrationToken m_autoHideScrollbarsChangedToken = {};
        EventRegistrationToken m_animationsEnabledChangedToken = {};

        wil::unique_registry_watcher_nothrow m_regKeyWatcher;

        xaml::DispatcherShutdownMode m_dispatcherShutdownMode {xaml::DispatcherShutdownMode_OnExplicitShutdown};

        static bool s_dynamicScrollbars;
        static bool s_dynamicScrollbarsDirty;

    public:
        _Check_return_ HRESULT IsAnimationEnabled(_Out_ bool* result);

    private:
        ctl::ComPtr<DirectUI::XamlDirect> m_spDefaultXamlDirectInstance;
        xref_ptr<StaticStore> m_staticStoreGuard;
        bool m_isAnimationEnabled = false;
        static bool s_enableNotifyEndOfReferenceTrackingOnThread;
        static HANDLE s_diagnosticToolingEvent;
        wrl::ComPtr<msy::IDispatcherQueueController> m_dispatcherQueueController;
    };
}
