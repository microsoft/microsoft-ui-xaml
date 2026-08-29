// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IdleSynchronizer.h"
#include "TestPoolFilter.h"
#include <vector>
#include <windows.ui.viewmanagement.h>
#include <microsoft.ui.input.h>

interface IXamlTestHooks;
interface IXamlErrorTestHooks;

namespace Private { namespace Infrastructure {

    class WindowHelper : public Microsoft::WRL::RuntimeClass<test_infra::IWindowHelper>
    {
        InspectableClass(RuntimeClass_Private_Infrastructure_WindowHelper, TrustLevel::BaseTrust);
        std::vector<TestPoolFilter*> testPoolFilters;
    public:
        WindowHelper(DWORD uiThreadId, wrl::ComPtr<test_infra::Hosting::IWin32Host> win32Host, test_infra::ITestServicesStatics* testServices);
        ~WindowHelper();

        IFACEMETHOD(RuntimeClassInitialize)();

        // IWindowHelper
        IFACEMETHOD(SetupSimulatedAppPage)(xaml_controls::IPage **ppPage) override;
        IFACEMETHOD(WaitForIdle)() override;
        IFACEMETHOD(WaitForIdleWithBuildTreeOption)(BOOLEAN waitForBuildTreeWork) override;
        IFACEMETHOD(WaitForTreeReset)() override;
        IFACEMETHOD(ResetWindowContentAndWaitForIdle)() override;
        IFACEMETHOD(GetCurrentWindowScale)(float* scale) override;
        IFACEMETHOD(ResetWindowContentAndScaleWaitForIdle)(float scale) override;
        IFACEMETHOD(WaitForImplicitShowHideComplete)() override;
        IFACEMETHOD(SynchronouslyTickUIThread)(unsigned int ticks) override;
        IFACEMETHOD(SetPostTickCallback)(_In_opt_ test_infra::IPostTickCallback* callback) override;
        IFACEMETHOD(RestoreForegroundWindow)() override;
        IFACEMETHOD(VerifyTestCleanup)() override;
        IFACEMETHOD(put_WindowContent)(xaml::IUIElement* pElement) override;
        IFACEMETHOD(get_WindowContent)(xaml::IUIElement** ppElement) override;
        IFACEMETHOD(get_WindowBounds)(wf::Rect* bounds) override;
        IFACEMETHOD(get_VisibleBounds)(wf::Rect* bounds) override;
        IFACEMETHOD(get_MonitorBounds)(wf::Rect* bounds) override;
        IFACEMETHOD(ShowWindow)() override;
        IFACEMETHOD(HideWindow)() override;
        IFACEMETHOD(MoveWindow)(int x, int y, int width, int height) override;
        IFACEMETHOD(InjectMockDComp)() override;
        IFACEMETHOD(DetachMockDComp)() override;
        IFACEMETHOD(StopMockDCompDetours)() override;
        IFACEMETHOD(get_MockDCompDevice)(
            _Outptr_result_maybenull_ mdc::IMockDCompDevice **ppMockDCompDevice  // null when no mock device exists
            ) override;
        IFACEMETHOD(SimulateDeviceLost)() override;
        IFACEMETHOD(ResetDeviceAndVisuals)() override;
        IFACEMETHOD(ResetDeviceOnly)() override;
        IFACEMETHOD(ConvertToPhysicalPixels)(wf::Point pointIn, wf::Point* pointOut) override;
        IFACEMETHOD(ConvertToPhysicalDisplayLocation)(wf::Point pointIn, wf::Point* pointOut) override;
        IFACEMETHOD(get_IsInputPaneOpen)(BOOLEAN* pIsInputPaneOpen) override;
        IFACEMETHOD(get_IsForegroundWindow)(BOOLEAN* pIsForegroundWindow) override;
        IFACEMETHOD(get_IsFocusedWindow)(BOOLEAN* pIsFocusedWindow) override;

        IFACEMETHOD(SetWindowSizeOverride)(wf::Size size) override;
        IFACEMETHOD(SetWindowSizeOverrideWithLayoutBounds)(wf::Size size, wf::Rect layoutBounds) override;

        IFACEMETHOD(SetWindowSizeOverrideWithScale)(wf::Size size, float zoomScaleOverride) override;
        IFACEMETHOD(SetWindowSizeOverrideWithWindowScale)(wf::Size size, float zoomScaleOverride) override;

        IFACEMETHOD(PrepareForPopupMenuWait)() override;
        IFACEMETHOD(WaitForPopupMenuCommandInvoked)(_In_ UINT32 timeoutMilliseconds, _Out_ BOOLEAN* pSuccess) override;
        IFACEMETHOD(Popup_SetWindowed)(_In_ xaml_primitives::IPopup* pPopup) override;
        IFACEMETHOD(FlyoutBase_GetWindow)(_In_ xaml_primitives::IFlyoutBase* pFlyoutBase, _Out_ UINT64* hwnd);
        IFACEMETHOD(Popup_GetWindow)(_In_ xaml_primitives::IPopup* pPopup, _Out_ UINT64* hwnd);
        IFACEMETHOD(ToolTip_GetWindow)(_In_ xaml_controls::IToolTip* pToolTip, _Out_ UINT64* hwnd);
        IFACEMETHOD(SetDesktopWindowSize)(unsigned int width, unsigned int height) override;
        IFACEMETHOD(MoveDesktopWindow)(unsigned int x, unsigned int y) override;
        IFACEMETHOD(MaximizeDesktopWindow)() override;
        IFACEMETHOD(IsDesktopWindowMaximized)(_Out_ BOOL* isMaximized) override;

        IFACEMETHOD(FrameworkElement_HasPeer)(
            _In_ xaml::IFrameworkElement* pFrameworkElement,
            _In_ HSTRING name,
            _Out_ BOOLEAN *pHasPeer) override;

        IFACEMETHOD(RequestReplayPreviousPointerUpdate_TempTestHook)() override;

        IFACEMETHOD(SimulateSuspendToPauseAnimations)() override;
        IFACEMETHOD(SimulateResumeToResumeAnimations)() override;
        IFACEMETHOD(SetIsSuspended)(BOOLEAN isSuspended) override;
        IFACEMETHOD(SetIsRenderEnabled)(BOOLEAN isRenderEnabled) override;
        IFACEMETHOD(SetTimeManagerClockOverrideConstant)(double newTime) override;
        IFACEMETHOD(FireDCompAnimationCompleted)(_In_ xaml_animation::IStoryboard* storyboard) override;
        IFACEMETHOD(CleanUpAfterTest)() override;
        IFACEMETHOD(ForceDisconnectRootOnSuspend)(_In_ BOOLEAN forceDisconnectRootOnSuspend) override;
        IFACEMETHOD(TriggerSuspend)(_In_ BOOLEAN isTriggeredByResourceTimer, _In_ BOOLEAN allowOfferResources) override;
        IFACEMETHOD(TriggerResume)() override;
        IFACEMETHOD(TriggerLowMemory)() override;

        IFACEMETHOD(TestGetGlobalBoundsForUIElement)(_In_ xaml::IUIElement* element, _In_ BOOLEAN ignoreClipping, _Out_ wf::Rect* bounds) override;
        IFACEMETHOD(GetAvailableMonitorBounds)(
            _In_ xaml::IUIElement* element,
            _In_ wf::Point targetPointClientLogical,
            _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
            _Out_ wf::Point* screenOffset,
            _Out_ wf::Point* targetPointScreenPhysical,
            _Out_ wf::Rect* inputPaneOccludeRectScreenLogical);

        IFACEMETHOD(SetLastInputMethod)(_In_ test_infra::LastInputDeviceType lastInputType, _In_ xaml::IXamlRoot* xamlRoot) override;
        IFACEMETHOD(GetLastInputMethod)(_In_ xaml::IXamlRoot* xamlRoot, _Out_ test_infra::LastInputDeviceType* lastDeviceType) override;

        IFACEMETHOD(SetLastLayoutExceptionElement)(_In_ xaml::IUIElement* element) override;

        IFACEMETHOD(ResetVisualTree)() override;

        IFACEMETHOD(ShutdownXaml)() override;
        IFACEMETHOD(EnsureSatelliteDLLCustomDPCleanup)() override;
        IFACEMETHOD(InitializeXaml)() override;
        IFACEMETHOD(InitializeXamlWithCustomMetadata)(_In_ xaml_markup::IXamlMetadataProvider* customProvider, _In_ test_infra::ICustomMetadataRegistrar* registrar) override;
        IFACEMETHOD(InitializeXamlWithProvider)(_In_ xaml_markup::IXamlMetadataProvider* customProvider) override;

        IFACEMETHOD(InjectWindowMessage)(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _In_ xaml::IXamlRoot* xamlRoot) override;

        // PointerDownThemeAnimation uses the location of the pointer to tilt the target element. These override provide a
        // consistent value for PointerDownThemeAnimation to use without having to inject input.
        IFACEMETHOD(SetPrimaryPointerLastPositionOverride)(_In_ wf::Point value, _In_ xaml::IXamlRoot* xamlRoot);
        IFACEMETHOD(ClearPrimaryPointerLastPositionOverride)(_In_ xaml::IXamlRoot* xamlRoot);

        IFACEMETHOD(SetVisibleBounds)(_In_ wf::Rect visibleBounds) override;

        IFACEMETHOD(IsWindowActivated)(_In_ xaml::IXamlRoot* xamlRoot, _Out_ BOOLEAN* active) override;

        IFACEMETHOD(SetForceIsFullScreen)(BOOLEAN forceIsFullScreen) override;

        IFACEMETHOD(CancelAllConnectedAnimationsAndResetDefaults)() override;

        IFACEMETHOD(SendWindowMessage)(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _Out_opt_ UINT* returnValue) override;

        IFACEMETHOD(SetIsHolographic)(BOOLEAN isHolographic) override;

        IFACEMETHOD(CreateNewView)(_In_ test_infra::IViewCreatedCallback* callback, _COM_Outptr_ test_infra::ISecondaryView** secondaryView) override;
        IFACEMETHOD(BringSecondaryViewToFront)(_In_ test_infra::ISecondaryView* view) override;
        IFACEMETHOD(BringMainViewToFront)() override;
        IFACEMETHOD(get_CurrentDispatcher)(_COM_Outptr_ msy::IDispatcherQueue** dispatcherQueue) override;
        IFACEMETHOD(SimulateInputPaneOccludedRect)(_In_ xaml::IXamlRoot* xamlRoot, wf::Rect occludedRect);

        IFACEMETHOD(GetLightsTargetingElement)(xaml::IUIElement* target, wfc::IVector<xaml_media::XamlLight*>* lights);
        IFACEMETHOD(GetElementsTargetedByLight)(xaml_media::IXamlLight* light, wfc::IVector<xaml::UIElement*>* targets);
        IFACEMETHOD(GetCountOfVisualsTargeted)(_In_ xaml_media::IXamlLight* light, _In_ xaml::IUIElement* element, _Out_ int* count);

        IFACEMETHOD(GetRealCompositionSurface)(_In_ xaml_media::ILoadedImageSurface *loadedImageSurface, _Outptr_ WUComp::ICompositionSurface **realCompositionSurface);

        IFACEMETHOD(SetHdrOutputOverride)(BOOLEAN forceHdr) override;

        IFACEMETHOD(GetWantsRenderingEvent)(_Out_ BOOLEAN* wantsRenderingEvent) override;

        IFACEMETHOD(GetWantsCompositionTargetRenderedEvent)(_Out_ BOOLEAN* wantsCompositionTargetRenderedEvent) override;

        IFACEMETHOD(SetThreadingAssertOverride)(BOOLEAN overrideThreadingAssert);

        IFACEMETHOD(AddTestLTE)(
            xaml::IUIElement* lteTarget,
            xaml::IUIElement* lteParent,
            test_infra::LTEParentMode lteParentMode,
            BOOLEAN isAbsolutelyPositioned,
            xaml::IUIElement** lte);
        IFACEMETHOD(RemoveTestLTE)(xaml::IUIElement* lte);
        IFACEMETHOD(ClearTestLTEs)();

        IFACEMETHOD(SetPlayingSoundNodeCallback)(_In_opt_ test_infra::IPlayingSoundNodeCallback* callback) override;

        IFACEMETHOD(IsTrackingEffectiveVisibility)(xaml::IUIElement* element, _Out_ BOOLEAN* isTracking) override;
        IFACEMETHOD(IsKeepingVisible)(xaml::IUIElement* element, _Out_ BOOLEAN* isKeepingVisible) override;
        IFACEMETHOD(RequestKeepAlive)(xaml::IUIElement* element) override;
        IFACEMETHOD(ReleaseKeepAlive)(xaml::IUIElement* element) override;

        IFACEMETHOD(TestGetActualToolTip)(
            _In_ xaml::IUIElement* element,
            _Outptr_ xaml_controls::IToolTip** ppValue) override;

        IFACEMETHOD(GetAnimatedTranslation)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* translation) override;
        IFACEMETHOD(GetAnimatedRotation)(_In_ xaml::IUIElement* element, _Out_ float* rotation) override;
        IFACEMETHOD(GetAnimatedScale)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* scale) override;
        IFACEMETHOD(GetAnimatedTransformMatrix)(_In_ xaml::IUIElement* element, _Out_ wfn::Matrix4x4* transformMatrix) override;
        IFACEMETHOD(GetAnimatedRotationAxis)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* rotationAxis) override;
        IFACEMETHOD(GetAnimatedCenterPoint)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* centerPoint) override;
        IFACEMETHOD(WaitForAnimatedFacadePropertyChanges)(int count) override;

        IFACEMETHOD(IsStoryboardActive)(_In_ xaml_animation::IStoryboard* board, _Out_ BOOLEAN* isActive) override;

        IFACEMETHOD(GetElementInputWindow)(_In_ xaml::IUIElement* element, _Out_ UINT64* inputHwnd) override;

        IFACEMETHOD(SetSuspendOffThreadDecoding)(_In_ BOOLEAN isOffThreadDecodingSuspended) override;

        IFACEMETHOD(SetSuspendSurfaceUpdates)(_In_ BOOLEAN isSuspended) override;

        IFACEMETHOD(ThrottleImageTaskDispatcher)(BOOLEAN enableThrottling, unsigned int numberOfTasksAllowedToDispatch) override;
        IFACEMETHOD(RequestExecuteImageTaskDispatcher)() override;

        IFACEMETHOD(GetUIAWindow)(_Outptr_ IInspectable** uiaWindow_IRawElementProviderFragment) override;

        IFACEMETHOD(ApplyElevationEffect)(_In_ xaml::IUIElement* element, UINT depth = 0) override;
        IFACEMETHOD(SetCaretBrowsingModeGlobal)(BOOLEAN caretBrowsingModeEnable, BOOLEAN caretBrowsingDialogNotPopAgain) override;

        IFACEMETHOD(SetGCCollectCallback)(_In_ test_infra::IGCCollectCallback* callback);

        IFACEMETHOD(GetUIAWindowHandle)(_Out_ UINT64* uiWindowHandle) override;

        IFACEMETHOD(ForceShadowsPolicy)(_In_ BOOLEAN forceShadowsOn) override;
        IFACEMETHOD(ClearShadowPolicyOverrides)() override;

        IFACEMETHOD(SetXamlVisibilityOverride)(_In_ BOOLEAN isVisible) override;

        IFACEMETHOD(SetBrushForXamlRoot)(
            _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
            _In_ IInspectable* xamlRoot,
            _In_ IInspectable* compBrush) override;

        IFACEMETHOD(GetBrushForXamlRoot)(
            _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
            _In_ IInspectable* xamlRoot,
            _Outptr_ IInspectable** compBrush) override;

        IFACEMETHOD(GetElementsRenderedCount)(_Out_ int* elementsRendered) override;

        IFACEMETHOD(PauseNewDispatchForTest)() override;
        IFACEMETHOD(ResumeNewDispatchForTest)() override;

        IFACEMETHOD(GetImageSourceMaxSize)(
            _In_ xaml_media::IImageSource* imageSource,
            _Out_ wf::Size* maxSize) override;

        IFACEMETHOD(GetInputPaneForMainView)(
            _Outptr_ wuv::IInputPane** inputPane) override;

        IFACEMETHOD(GetXamlRoots)(
            _Outptr_ wfc::IVectorView<xaml::XamlRoot*>** xamlRoots
            ) override;

        IFACEMETHOD(GetWindows)(
            _Outptr_ wfc::IVectorView<xaml::Window*>** windows
            ) override;

        IFACEMETHOD(DetachMemoryManagerEvents)() override;

        IFACEMETHOD(GetElementRenderedVisuals)(_In_ xaml::IUIElement* element, wfc::IVector<IInspectable*>* visuals);

        IFACEMETHOD(GetErrorHandlingTestHooks)(_Outptr_opt_ test_infra::IXamlErrorTestHooks** errorTestHooks) override;

        IFACEMETHOD(GCCollect());

        HRESULT OnSimulatedDeviceLost();
        wrl::ComPtr<mdc::IMockDCompDevice> TryGetMockDCompDevice();

        HRESULT WaitForIdle(bool waitForBuildTree);
        HRESULT ResetThemingToDefaultState();
        static void SetWindowContentStatic(xaml::IUIElement* pElement, wrl::ComPtr<test_infra::Hosting::IWin32Host> win32Host);
        static void GetWindowContentStatic(xaml::IUIElement** ppElement, wrl::ComPtr<test_infra::Hosting::IWin32Host> win32Host);
        static void GetOpenPopupsStatic(wfc::IVectorView<xaml_controls::Primitives::Popup*>** ppPopups);
        static void GetOpenPopupsInXamlRootStatic(
            _In_ xaml::IXamlRoot* xamlRoot,
            wfc::IVectorView<xaml_controls::Primitives::Popup*>** ppPopups);

        static void GetAllRootVisualsNoRef(_In_ wfc::IVectorView<IInspectable*>** visualsNoRef);

        // Returns the ratio between physical and logical pixels.
        static double GetPhysicalPixelsPerLogicalPixelStatic();

        // The intent of this function is to convert from logical XAML units to
        // physical pixels.
        static wf::Point ConvertToPhysicalPixelsStatic(wf::Point point);

        // This is like ConvertToPhysicalPixelsStatic except it accounts for
        // phone orientation. When the phone is rotated, XAML applies a DComp rotate
        // transform to apply the device orientation because the system doesn't do that for us.
        // Indeed, as far as the OS and input injector are concerned, the native top left location
        // hasn't changed, so we need to return locations relative to the native top left location.
        static wf::Point ConvertToPhysicalDisplayLocationStatic(wf::Point point, bool validatePhysicalLocationIsInsideCurrentWindow = true);

        static void GetWindowBoundsStatic(wf::Rect* bounds);
        static void GetVisibleBoundsStatic(wf::Rect* bounds, xaml::IUIElement* pElement);
        static void ShowWindowStatic();
        static void HideWindowStatic();
        static void MoveWindowStatic(int x, int y, int width, int height);

        static bool IsForegroundWindowStatic();
        static bool IsFocusedWindowStatic();
        static void IsInputPaneOpenStatic(_Out_ BOOLEAN* pIsInputPaneOpen);
        static void TryInputPaneHideStatic();

        static bool IsLeakDetectionEnabled();

        static wrl::ComPtr<IXamlTestHooks> GetTestHooks();
        static wrl::ComPtr<xaml::IWindow> GetXamlWindow();

        // IWindowHelper - Allocation statistics
        IFACEMETHOD(GetAllocationCount)(_Out_ UINT64* count) override;
        IFACEMETHOD(GetAllocationSize)(_Out_ UINT64* size) override;
        IFACEMETHOD(GetDeallocationCount)(_Out_ UINT64* count) override;
        static wrl::ComPtr<xaml::IWindowPrivate> GetXamlWindowPrivate();

        // Returns the handle of the current CoreWindow.
        static HWND GetCurrentWindowHandle();

        void OverrideMetadataProvider(xaml_markup::IXamlMetadataProvider * provider);

        static wrl::ComPtr<msy::IDispatcherQueue> GetDispatcherForView(const wrl::ComPtr<wac::ICoreApplicationView>& view);
        static wrl::ComPtr<msy::IDispatcherQueue> GetDispatcherForMainView();

    private:
        void InitializeXamlCore(_In_ xaml_markup::IXamlMetadataProvider* customProvider);

        static HRESULT OnAppSuspended();

        // Wrapper function for calling the post-tick callback
        void PostTickCallbackWrapper();

        // Wrapper function for calling the playing-sound-node callback
        void PlayingSoundNodeCallbackWrapper(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume);

        void SwitchToView(const wrl::ComPtr<wac::ICoreApplicationView>& view);
        static int GetApplicationViewIdForWindow(const wrl::ComPtr<wuc::ICoreWindow>& window);

        bool IsKeyLockedOrDown (
            _In_ ixp::IInputKeyboardSourceStatics* const keyboardStatics,
            _In_ const wsy::VirtualKey key
        ) const;

        bool IsKeyDown(
            _In_ ixp::IInputKeyboardSourceStatics* const keyboardStatics,
            _In_ const wsy::VirtualKey key
        ) const;

        void ClearKeyState();

        void ResetDeviceAndVisualsHelper();
        void TickUIThreadAfterDeviceLostIfNeeded();

        HRESULT SetHostSizeOverride(
            const wf::Size& size,
            const wf::Rect& layoutBounds,
            float zoomScaleOverride,
            bool scaleWindowSizeByScaleFactor);

        wrl::ComPtr<xaml::IApplication> m_spApp;
        EventRegistrationToken m_suspendedToken = {};

        // We store a pointer to the current core dispatcher
        // when this class is initialized so we can use
        // it from non-UIThreads without having to schedule work
        // on the UI thread to retrieve it.
        IdleSynchronizer m_idleSynchronizer;
        static bool s_foregroundWindowCraterArmed;
        static bool s_isShutdownEnabled;

        bool m_ensureSatelliteDLLCustomDPCleanup = false;

        // Delegate function the test can set to call it back after every UI thread tick
        wrl::ComPtr<test_infra::IPostTickCallback> m_spPostTickCallback;

        // Delegate function for detecting a sound node play
        wrl::ComPtr<test_infra::IPlayingSoundNodeCallback> m_spPlayingSoundNodeCallback;

        // Custom metadata registrar. We store it so when we shutdown we can tell the user
        // to unregister their dependency properties
        wrl::ComPtr<wf::IClosable> m_spClosableMetadataRegistrar;

        wrl::ComPtr<test_infra::Hosting::IWin32Host> m_win32Host;
        wrl::ComPtr<test_infra::IGCCollectCallback> m_gccollectCallback;
        test_infra::ITestServicesStatics* m_pTestServices;
    };

} }
