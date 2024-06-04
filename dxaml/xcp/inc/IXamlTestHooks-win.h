// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Microsoft.UI.Xaml.controls.controls.h>
#include <Microsoft.UI.Xaml.private.h>
#include "IXamlTestHooks-log.h"
#include <microsoft.ui.input.experimental.h>
#include <NamespaceAliases.h>

struct IDCompositionDesktopDevicePartner;
struct IDWriteFontCollection;
interface IXamlErrorTestHooks;

typedef HRESULT (*PFNPROPERTYVALUETOSTRING)(_In_ IInspectable* pValue, _Out_ HSTRING* phstrValue);

namespace RuntimeFeatureBehavior
{
    enum class RuntimeEnabledFeature;
}

namespace ABI { namespace Microsoft { namespace UI { namespace Xaml {
    struct IPopup;
}}}}

using namespace ErrorHandling;

enum XamlInternalCommand : int
{
    Popup_SetWindowed,
    WebView_SetInUserAction,
    WebView_SimulateUpdateEngagementState,
    FrameworkElement_HasPeer,
    FlyoutBase_GetWindow,
    Popup_GetWindow,
    ToolTip_GetWindow
};

struct JupiterGripperPoint
{
    FLOAT X;
    FLOAT Y;
};

struct JupiterGripperMetrics
{
    JupiterGripperPoint CenterLocalCoordinate;
};

struct JupiterGripperData
{
    JupiterGripperMetrics Start;
    JupiterGripperMetrics End;
};

// Private interface for unit test framework
// References macros defined in <combaseapi.h>, which conflicts with
// code in the core, particularly around GetClassName.
DECLARE_INTERFACE_IID_(IXamlTestHooks, IXamlLoggerTestHooks, "43d4bcbd-4f02-4651-9ecc-dcfec9f786a7")
{
    IFACEMETHOD(SimulateDeviceLost)() = 0;
    IFACEMETHOD(SimulateDeviceLostOnOffThreadImageUpload)() = 0;
    IFACEMETHOD(SimulateSwallowedDeviceLostOnStartup)() = 0;
    IFACEMETHOD(SimulateDeviceLostOnMetadataParse)() = 0;
    IFACEMETHOD(SimulateDeviceLostOnCreatingSvgDecoder)() = 0;
    IFACEMETHOD(ResetDeviceAndVisuals)() = 0;
    IFACEMETHOD(ResetDeviceOnly)() = 0;
    IFACEMETHOD(ResetDeviceAndVisualsAndDManip)() = 0;
    IFACEMETHOD_(void, SetDCompDeviceLeakDetectionEnabled)(bool enableLeakDetection) = 0;

    IFACEMETHOD_(bool, IsDragDropInProgress)() = 0;

    IFACEMETHOD_(void, GetDCompDevice)(
        _Outptr_ IDCompositionDesktopDevicePartner **ppDCompDevice
        ) const = 0;

    IFACEMETHOD(MarkDeviceInstanceLost)() const = 0;
    IFACEMETHOD(GetD3D11GraphicsDeviceAddress)(_Out_ INT64* ppCD3D11Device) const = 0;

    IFACEMETHOD(SetWindowSizeOverride)(
        _In_ const wf::Size& size,
        _In_ const wf::Rect& layoutBounds,
        _In_ float zoomScaleOverride,
        _In_ bool scaleWindowSizeByScaleFactor,
        _In_opt_ xaml::IWindow* iwindow = nullptr
        ) = 0;

    // Reports the zoom scale that Xaml applies. Note that this is not the same as the system scale. For Xaml island scenarios,
    // the island could be placed in a window with a DPI awareness context of Unaware/System/PerMonitor/UnawareGdiScaled. Under
    // those contexts, the island will not change its scale in response to the system scale changing. For normal CoreWindow
    // scenarios Xaml will respond to system scale changes.
    IFACEMETHOD_(float, GetZoomScale)() const = 0;

    IFACEMETHOD_(void, OverrideTrimImageResourceDelay)(_In_ bool enabled) = 0;

    IFACEMETHOD_(void, CleanupReleaseQueue)() = 0;

    IFACEMETHOD(GetFinalReleaseQueue)(_Outptr_ wfc::IVectorView<IInspectable*>** queue) = 0;

    IFACEMETHOD_(bool, IsRuntimeEnabledFeatureEnabled)(_In_ RuntimeFeatureBehavior::RuntimeEnabledFeature feature) = 0;
    IFACEMETHOD_(void, SetRuntimeEnabledFeatureOverride)(_In_ RuntimeFeatureBehavior::RuntimeEnabledFeature feature, _In_ bool enabled, _Out_opt_ bool* wasPreviouslyEnabled) = 0;
    IFACEMETHOD_(void, ClearAllRuntimeFeatureOverrides)() = 0;

    // Causes Jupiter to emit its internal private heap handle using an ETW event.
    // This heap handle will be read by PerfAnalyzer and used to compute Jupiter's
    // private heap usage.
    IFACEMETHOD_(void, EmitHeapHandleExportEtwEvent)() = 0;

    IFACEMETHOD_(void, SetPostTickCallback)(_In_opt_ std::function<void()> callback) = 0;

    IFACEMETHOD(GetDependencyObjectPropertyValues)(
        _In_ xaml::IDependencyObject* pObject,
        _In_ bool excludeDefaultPropertyValues,
        _In_ PFNPROPERTYVALUETOSTRING pfnPropertyValueToString,
        _Out_ std::shared_ptr<std::map<std::wstring, std::wstring>>& spPropertyValuesMap
        );

    IFACEMETHOD(SetStoryboardStartedCallback)(_In_ HRESULT(*pCallback)(xaml_animation::IStoryboard*, xaml::IUIElement*)) = 0;
    IFACEMETHOD(InvokeInternalCommand)(
        _In_ xaml::IDependencyObject* pObject,
        _In_ XamlInternalCommand command,
        _In_opt_ std::vector<wf::IPropertyValue*>* pArgs,
        _Out_opt_ wf::IPropertyValue **ppReturnValue) = 0;

    IFACEMETHOD(SetApplicationRequestedTheme)(xaml::ApplicationTheme theme) = 0;
    IFACEMETHOD(UnsetApplicationRequestedTheme)() = 0;

    IFACEMETHOD(OverrideSystemTheme)(xaml::ApplicationTheme theme) = 0;
    IFACEMETHOD(OverrideHighContrast)(std::shared_ptr<std::list<std::pair<int, unsigned int>>> sysColorPalette) = 0;
    IFACEMETHOD(OverrideAccentColor)(unsigned int accentColor) = 0;
    IFACEMETHOD(RemoveThemingOverrides)() = 0;

    // Clear the system font collection cached by core text services so that it retrieves the recent one after fonts have changed.
    IFACEMETHOD(SetSystemFontCollectionOverride)(_In_ IDWriteFontCollection* pFontCollection) = 0;
    IFACEMETHOD(ShouldUseTypographicFontModel)(_Out_ bool* useDWriteTypographicModel) = 0;

    IFACEMETHOD(GetGripperData)(_In_ IInspectable* textControl, _Inout_ JupiterGripperData* returnValue);

    IFACEMETHOD(ResetMetadata)();

    IFACEMETHOD(CreateLoopingSelector)(_Outptr_ IInspectable** ppLoopingSelector) = 0;

    IFACEMETHOD(InjectBackButtonPress)(_Out_ BOOLEAN* pHandled) = 0;

    IFACEMETHOD(IsWindowActivated)(_In_ xaml::IXamlRoot* xamlRoot, _Out_ BOOLEAN* active) = 0;

    IFACEMETHOD_(void, ResetAtlasSizeHint)() = 0;

    IFACEMETHOD_(void, ShrinkApplicationViewVisibleBounds)(_In_ bool enabled, _In_opt_ xaml::IWindow* iwindow = nullptr) = 0;

    IFACEMETHOD_(void, RequestReplayPreviousPointerUpdate_TempTestHook)() = 0;

    IFACEMETHOD_(void, SimulateSuspendToPauseAnimations)() = 0;
    IFACEMETHOD_(void, SimulateResumeToResumeAnimations)() = 0;
    IFACEMETHOD_(void, SetIsSuspended)(bool isSuspended) = 0;
    IFACEMETHOD_(void, SetIsRenderEnabled)(bool value) = 0;
    IFACEMETHOD_(void, SetTimeManagerClockOverrideConstant)(double newTime) = 0;
    IFACEMETHOD_(void, FireDCompAnimationCompleted)(_In_ xaml_animation::IStoryboard* storyboard) = 0;
    IFACEMETHOD_(void, CleanUpAfterTest)() = 0;
    IFACEMETHOD_(void, ForceDisconnectRootOnSuspend)(bool forceDisconnectRootOnSuspend) = 0;
    IFACEMETHOD_(void, TriggerSuspend)(bool isTriggeredByResourceTimer, bool allowOfferResources) = 0;
    IFACEMETHOD_(void, TriggerResume)() = 0;
    IFACEMETHOD_(void, TriggerLowMemory)() = 0;

    IFACEMETHOD_(void, SetPrimaryPointerLastPositionOverride)(_In_ wf::Point value, _In_ xaml::IXamlRoot* xamlRoot) = 0;
    IFACEMETHOD_(void, ClearPrimaryPointerLastPositionOverride)(_In_ xaml::IXamlRoot* xamlRoot) = 0;

    IFACEMETHOD(TestGetGlobalBoundsForUIElement)(_In_ xaml::IUIElement* element, _In_ BOOLEAN ignoreClipping, _Out_ wf::Rect* bounds) = 0;

    IFACEMETHOD(GetVisibleContentBounds)(_In_ xaml::IUIElement* element, _Out_ wf::Rect* pValue) = 0;

    IFACEMETHOD(CalculateAvailableMonitorRect)(
        _In_ xaml::IUIElement* element,
        _In_ wf::Point targetPointClientLogical,
        _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
        _Out_ wf::Point* screenOffset,
        _Out_ wf::Point* targetPointScreenPhysical,
        _Out_ wf::Rect* inputPaneOccludeRectScreenLogical) = 0;

    IFACEMETHOD_(void, SimulateThemeChanged)() = 0;

    IFACEMETHOD_(void, SetLastInputMethod)(_In_ xaml_input::LastInputDeviceType lastInputType, _In_ xaml::IXamlRoot* xamlRoot) = 0;
    IFACEMETHOD_(xaml_input::LastInputDeviceType, GetLastInputMethod)(_In_ xaml::IXamlRoot* xamlRoot) = 0;

    IFACEMETHOD(ClearDefaultLanguageString)() = 0;

    IFACEMETHOD(WaitForCommitCompletion)() = 0;

    IFACEMETHOD_(void, ResetVisualTree)() = 0;

    IFACEMETHOD(IsWindowFocused)(_Out_ BOOLEAN* isFocused, _In_ xaml::IXamlRoot* xamlRoot) = 0;

    IFACEMETHOD(ShutdownXaml)() = 0;

    IFACEMETHOD(EnsureSatelliteDLLCustomDPCleanup)() = 0;

    IFACEMETHOD(InitializeXaml)() = 0;

    IFACEMETHOD_(BOOLEAN, InjectWindowMessage)(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _In_ xaml::IXamlRoot* xamlRoot) = 0;

    IFACEMETHOD(UpdateFontScale)(_In_ float scale);

    IFACEMETHOD_(void, SetVisibleBounds)(_In_ const wf::Rect& visibleBounds, _In_opt_ xaml::IWindow* iwindow = nullptr) = 0;

    IFACEMETHOD_(void, SetForceIsFullScreen)(_In_ bool forceIsFullScreen) = 0;

    IFACEMETHOD(CancelAllConnectedAnimationsAndResetDefaults)() = 0;

    IFACEMETHOD(GetPopupOverlayElement)(_In_ xaml_primitives::IPopup* popup, _Outptr_ xaml::IFrameworkElement** overlayElement) = 0;

    IFACEMETHOD(EnableKeyboardInputEvent)() = 0;
    IFACEMETHOD_(void, DisableKeyboardInputEvent)() = 0;
    IFACEMETHOD_(bool, CanFireKeyboardInputEvent)() = 0;

    IFACEMETHOD(EnablePointerInputEvent)() = 0;
    IFACEMETHOD_(void, DisablePointerInputEvent)() = 0;
    IFACEMETHOD_(bool, CanFirePointerInputEvent)() = 0;

    IFACEMETHOD_(void, DeletePlatformFamilyCache)() = 0;

    IFACEMETHOD_(void, DeleteResourceDictionaryCaches)() = 0;

    IFACEMETHOD_(void, SetLastLayoutExceptionElement)(_In_ xaml::IUIElement* pElement) = 0;

    // Uses the logger callback mechanism in TAEF tests and will log errors for the detected leaks.
    // Will only show a summary if the amount of found leaked blocks is greater than
    // leakThresholdForSummary
    IFACEMETHOD_(void, PostTestCheckForLeaks)(_In_ unsigned int leakThresholdForSummary) = 0;

    IFACEMETHOD_(void, SetIsHolographic)(_In_ bool value) = 0;

    IFACEMETHOD(SimulateInputPaneOccludedRect)(xaml::IXamlRoot* xamlRoot, _In_ wf::Rect occludedRect) = 0;

    IFACEMETHOD_(void, SetMockUIAClientsListening)() = 0;
    IFACEMETHOD_(void, ClearMockUIAClientsListening)() = 0;

    IFACEMETHOD(GetLightsTargetingElement)(_In_ xaml::IUIElement* target, _In_ wfc::IVector<xaml_media::XamlLight*>* lights) = 0;
    IFACEMETHOD(GetElementsTargetedByLight)(_In_ xaml_media::IXamlLight* light, _In_ wfc::IVector<xaml::UIElement*>* targets) = 0;
    IFACEMETHOD(GetCountOfVisualsTargeted)(_In_ xaml_media::IXamlLight* light, _In_ xaml::IUIElement* element, _Out_ int* count) = 0;

    IFACEMETHOD(GetRealCompositionSurface)(_In_ xaml_media::ILoadedImageSurface *loadedImageSurface, _Outptr_ WUComp::ICompositionSurface **realCompositionSurface) = 0;

    IFACEMETHOD_(void, SetGenericXamlFilePathForMUX)(_In_ HSTRING filePath) = 0;

    IFACEMETHOD(SetHdrOutputOverride)(bool value) = 0;

    IFACEMETHOD_(void, GetWantsRenderingEvent(_Out_ BOOLEAN* wantsRenderingEvent)) = 0;

    IFACEMETHOD_(void, GetWantsCompositionTargetRenderedEvent(_Out_ BOOLEAN* wantsCompositionTargetRenderedEvent)) = 0;

    IFACEMETHOD_(void, SetThreadingAssertOverride)(_In_ bool value) = 0;

    IFACEMETHOD_(void, SetCanTickWithNoContent)(bool canTickWithNoContent) = 0;

    IFACEMETHOD(AddTestLTE)(
        _In_ xaml::IUIElement* lteTarget,
        _In_ xaml::IUIElement* lteParent,
        bool parentIsRootVisual,
        bool parentIsPopupRoot,
        bool isAbsolutelyPositioned,
        _Outptr_ xaml::IUIElement** lte) = 0;
    IFACEMETHOD(RemoveTestLTE)(_In_ xaml::IUIElement* lte) = 0;
    IFACEMETHOD(ClearTestLTEs)() = 0;
    IFACEMETHOD_(void, SetPlayingSoundNodeCallback)(_In_opt_ std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> callback) = 0;

    IFACEMETHOD_(BOOLEAN, IsTrackingEffectiveVisibility)(_In_ xaml::IUIElement* element) = 0;
    IFACEMETHOD_(BOOLEAN, IsKeepingVisible)(_In_ xaml::IUIElement* element) = 0;
    IFACEMETHOD(RequestKeepAlive)(_In_ xaml::IUIElement* element) = 0;
    IFACEMETHOD(ReleaseKeepAlive)(_In_ xaml::IUIElement* element) = 0;

    IFACEMETHOD(TestGetActualToolTip)(
        _In_ xaml::IUIElement* element,
        _Outptr_ xaml_controls::IToolTip** ppValue) = 0;
    IFACEMETHOD_(void, GetAnimatedTranslation)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* translation) = 0;
    IFACEMETHOD_(void, GetAnimatedRotation)(_In_ xaml::IUIElement* element, _Out_ float* rotation) = 0;
    IFACEMETHOD_(void, GetAnimatedScale)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* scale) = 0;
    IFACEMETHOD_(void, GetAnimatedTransformMatrix)(_In_ xaml::IUIElement* element, _Out_ wfn::Matrix4x4* transformMatrix) = 0;
    IFACEMETHOD_(void, GetAnimatedRotationAxis)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* rotationAxis) = 0;
    IFACEMETHOD_(void, GetAnimatedCenterPoint)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* centerPoint) = 0;
    IFACEMETHOD_(void, ScheduleWaitForAnimatedFacadePropertyChanges)(int count) = 0;

    IFACEMETHOD_(bool, IsStoryboardActive)(_In_ xaml_animation::IStoryboard* storyboard) = 0;

    IFACEMETHOD(GetElementInputWindow)(_In_ xaml::IUIElement* element, _Out_ HWND* inputHwnd) = 0;

    IFACEMETHOD_(void, SetSuspendOffThreadDecoding)(bool isOffThreadDecodingSuspended) = 0;

    IFACEMETHOD_(void, SetSuspendSurfaceUpdates)(bool isSuspended) = 0;

    IFACEMETHOD(GetUIAWindow)(
        _In_ xaml::IDependencyObject* pElement,
        _In_ HWND hwnd,
        _Outptr_ IInspectable** uiaWindowNoRef) = 0;

    IFACEMETHOD_(void, RestoreDefaultFlipViewScrollWheelDelay)() = 0;
    IFACEMETHOD_(void, SetFlipViewScrollWheelDelay)(int scrollWheelDelayMS) = 0;

    IFACEMETHOD(ApplyElevationEffect)(_In_ xaml::IUIElement* element, UINT depth = 0) = 0;

    IFACEMETHOD(SetApplicationLanguageOverride)(_In_ HSTRING languageName) = 0;
    IFACEMETHOD(ClearApplicationLanguageOverride)() = 0;

    IFACEMETHOD_(void, SetCaretBrowsingModeGlobal)(bool caretBrowsingModeEnable, bool caretBrowsingDialogNotPopAgain) = 0;

    IFACEMETHOD_(void, CloseAllPopupsForTreeReset)() = 0;

    IFACEMETHOD_(void, GetAllXamlRoots)(_Outptr_ wfc::IVectorView<xaml::XamlRoot*>** xamlRoots) = 0;

    IFACEMETHOD(ForceShadowsPolicy)(_In_ BOOLEAN forceShadowsOn) = 0;
    IFACEMETHOD(ClearShadowPolicyOverrides)() = 0;

    // This is a one-time override. To clear it, call it again with true
    IFACEMETHOD(SetXamlVisibilityOverride)(_In_ BOOLEAN isVisible) = 0;

    IFACEMETHOD(SetBrushForXamlRoot)(
        _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
        _In_ IInspectable* xamlRoot,
        _In_ IInspectable* compBrush) = 0;

    IFACEMETHOD(GetBrushForXamlRoot)(
        _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
        _In_ IInspectable* xamlRoot,
        _Outptr_ IInspectable** ppResult) = 0;

    IFACEMETHOD_(void, StopAllInteractions)() = 0;

    IFACEMETHOD(GetImageSourceMaxSize)(
        _In_ xaml_media::IImageSource* imageSource,
        _Out_ wf::Size* maxSize) = 0;

    // Returns a list of all root visuals (for the main tree and all islands). Used for MockDComp dumping.
    IFACEMETHOD_(void, GetAllRootVisualsNoRef)(_In_ wfc::IVectorView<IInspectable*>** visualsNoRef) = 0;

    IFACEMETHOD(DetachMemoryManagerEvents)() = 0;
    IFACEMETHOD_(void, SetTransparentBackground)(bool value) = 0;

    // Force DebugSettings to raise tracing events which normally only fire if a debugger is attached
    IFACEMETHOD_(void, SetForceDebugSettingsTracingEvents)(bool value) = 0;

    IFACEMETHOD(GetIslandAndBridge)(
        _In_ xaml::IFrameworkElement* element,
        bool followPopups,
        _Outptr_opt_ IInspectable** island,
        _Outptr_opt_ IInspectable** bridge) = 0;

    IFACEMETHOD_(void, ThrottleImageTaskDispatcher)(bool enableThrottling, unsigned int numberOfTasksAllowedToDispatch) = 0;
    IFACEMETHOD_(void, RequestExecuteImageTaskDispatcher)() = 0;

    IFACEMETHOD(GetElementRenderedVisuals)(_In_ xaml::IUIElement* element, _In_ wfc::IVector<IInspectable*>* visuals) = 0;

    IFACEMETHOD(GetErrorHandlingTestHooks)(_Outptr_opt_ IXamlErrorTestHooks** errorTestHooks) = 0;

    IFACEMETHOD(GetAllContentIslands)(_In_ xaml_hosting::IDesktopWindowXamlSource* desktopWindowXamlSource, _In_ wfc::IVector<IInspectable*>* contentIslands) = 0;

    IFACEMETHOD(GetElementsRenderedCount)(_Out_ int* elementsRendered) = 0;
};
