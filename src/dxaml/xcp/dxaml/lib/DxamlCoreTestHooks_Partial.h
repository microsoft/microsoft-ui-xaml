// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DxamlCoreTestHooks.g.h"
#include "IXamlTestHooks-win.h"

namespace DirectUI
{
    PARTIAL_CLASS(DxamlCoreTestHooks)
        , public IXamlTestHooks
    {
    public:
        void SetDXamlCore(_In_opt_ DXamlCore* pDXamlCore) { m_pDXamlCoreNoRef = pDXamlCore; }

        IFACEMETHOD(SimulateDeviceLost)() override;
        IFACEMETHOD(ResetDeviceAndVisuals)() override;
        IFACEMETHOD(ResetDeviceOnly)() override;
        IFACEMETHOD(ResetDeviceAndVisualsAndDManip)() override;
        IFACEMETHOD(SimulateDeviceLostOnOffThreadImageUpload)() override;
        IFACEMETHOD(SimulateSwallowedDeviceLostOnStartup)() override;
        IFACEMETHOD_(void, SetDCompDeviceLeakDetectionEnabled)(bool enableLeakDetection) override;

        IFACEMETHOD_(bool, IsDragDropInProgress)() override;

        IFACEMETHOD_(void, GetDCompDevice)(_Outptr_ IDCompositionDesktopDevicePartner **ppDCompDevice) const override;

        IFACEMETHOD(MarkDeviceInstanceLost)() const override;
        IFACEMETHOD(GetD3D11GraphicsDeviceAddress)(_Out_ INT64* ppCD3D11Device) const override;

        IFACEMETHOD(SetWindowSizeOverride)(
            _In_ const wf::Size& size,
            _In_ const wf::Rect& layoutBounds,
            _In_ float zoomScaleOverride,
            _In_ bool scaleWindowSizeByScaleFactor,
            _In_opt_ xaml::IWindow* iwindow = nullptr
            ) override;

        IFACEMETHOD_(float, GetZoomScale)() const override;

        IFACEMETHOD_(void, OverrideTrimImageResourceDelay)(_In_ bool enabled) override;

        IFACEMETHOD_(void, CleanupReleaseQueue)() override;

        IFACEMETHOD(GetFinalReleaseQueue)(_Outptr_ wfc::IVectorView<IInspectable*>** queue) override;

        IFACEMETHOD_(bool, IsRuntimeEnabledFeatureEnabled)(_In_ RuntimeFeatureBehavior::RuntimeEnabledFeature feature) override;
        IFACEMETHOD_(void, SetRuntimeEnabledFeatureOverride)(_In_ RuntimeFeatureBehavior::RuntimeEnabledFeature feature, _In_ bool enabled, _Out_opt_ bool* wasPreviouslyEnabled) override;
        IFACEMETHOD_(void, ClearAllRuntimeFeatureOverrides)() override;

        IFACEMETHOD_(void, EmitHeapHandleExportEtwEvent)() override;

        IFACEMETHOD_(void, SetErrorHandlerCallback)(_In_ std::shared_ptr<std::function<void(const ErrorHandling::XamlFailureInfo&)>> callback) override;
        IFACEMETHOD_(void, SetLoggerCallback)(_In_ std::shared_ptr<LoggerCallback> callback) override;
        IFACEMETHOD_(void, SetPostTickCallback)(_In_opt_ std::function<void()> callback) override;

        IFACEMETHOD(GetDependencyObjectPropertyValues)(
            _In_ xaml::IDependencyObject* pObject,
            _In_ bool excludeDefaultPropertyValues,
            _In_ PFNPROPERTYVALUETOSTRING pfnPropertyValueToString,
            _Out_ std::shared_ptr<std::map<std::wstring, std::wstring>>& spPropertyValuesMap
            ) override;

        IFACEMETHOD(SetStoryboardStartedCallback)(_In_ HRESULT(*pCallback)(xaml_animation::IStoryboard*,  xaml::IUIElement*)) override;
        IFACEMETHOD(InvokeInternalCommand)(
            _In_ xaml::IDependencyObject* pObject,
            _In_ XamlInternalCommand command,
            _In_opt_ std::vector<wf::IPropertyValue*>* pArgs,
            _Out_opt_ wf::IPropertyValue **ppReturnValue) override;

        IFACEMETHOD(SetApplicationRequestedTheme)(xaml::ApplicationTheme theme) override;
        IFACEMETHOD(UnsetApplicationRequestedTheme)() override;

        IFACEMETHOD(OverrideSystemTheme)(xaml::ApplicationTheme theme) override;
        IFACEMETHOD(OverrideHighContrast)(std::shared_ptr<std::list<std::pair<int, unsigned int>>> sysColorPalette) override;
        IFACEMETHOD(OverrideAccentColor)(unsigned int accentCOlor) override;
        IFACEMETHOD(RemoveThemingOverrides)() override;

        IFACEMETHOD(SetSystemFontCollectionOverride)(_In_opt_ IDWriteFontCollection* pFontCollection) override;
        IFACEMETHOD(ShouldUseTypographicFontModel)(_Out_ bool* useDWriteTypographicModel) override;

        IFACEMETHOD(GetGripperData)(_In_ IInspectable* textControl, _Inout_ JupiterGripperData* returnValue) override;

        IFACEMETHOD(ResetMetadata)() override;
        IFACEMETHOD(ClearDefaultLanguageString)() override;

        IFACEMETHOD(CreateLoopingSelector)(_Outptr_ IInspectable** ppLoopingSelector) override;

        IFACEMETHOD(InjectBackButtonPress)(_Out_ BOOLEAN* pHandled) override;

        IFACEMETHOD(IsWindowActivated)(_In_ xaml::IXamlRoot* xamlRoot, _Out_ BOOLEAN* windowIsActivated) override;

        IFACEMETHOD_(void, ResetAtlasSizeHint)() override;

        IFACEMETHOD_(void, ShrinkApplicationViewVisibleBounds)(_In_ bool enabled, _In_opt_ xaml::IWindow* iwindow = nullptr) override;

        IFACEMETHOD_(void, RequestReplayPreviousPointerUpdate_TempTestHook)() override;

        IFACEMETHOD_(void, SimulateSuspendToPauseAnimations)() override;
        IFACEMETHOD_(void, SimulateResumeToResumeAnimations)() override;
        IFACEMETHOD_(void, SetIsSuspended)(bool isSuspended) override;
        IFACEMETHOD_(void, SetIsRenderEnabled)(bool value) override;
        IFACEMETHOD_(void, SetTimeManagerClockOverrideConstant)(double newTime) override;
        IFACEMETHOD_(void, FireDCompAnimationCompleted)(_In_ xaml_animation::IStoryboard* storyboard) override;
        IFACEMETHOD_(void, CleanUpAfterTest)() override;
        IFACEMETHOD_(void, ForceDisconnectRootOnSuspend)(bool forceDisconnectRootOnSuspend) override;
        IFACEMETHOD_(void, TriggerSuspend)(bool isTriggeredByResourceTimer, bool allowOfferResources) override;
        IFACEMETHOD_(void, TriggerResume)() override;
        IFACEMETHOD_(void, TriggerLowMemory)() override;

        IFACEMETHOD(TestGetGlobalBoundsForUIElement)(_In_ xaml::IUIElement* element, _In_ BOOLEAN ignoreClipping, _Out_ wf::Rect* bounds) override;

        IFACEMETHOD_(void, SimulateThemeChanged)() override;

        IFACEMETHOD_(void, SetLastInputMethod)(_In_ xaml_input::LastInputDeviceType lastInputType, _In_ xaml::IXamlRoot* xamlRoot) override;
        IFACEMETHOD_(xaml_input::LastInputDeviceType, GetLastInputMethod)(_In_ xaml::IXamlRoot* xamlRoot) override;

        IFACEMETHOD_(void, ResetVisualTree)() override;
        IFACEMETHOD(ShutdownXaml)() override;
        IFACEMETHOD(EnsureSatelliteDLLCustomDPCleanup)() override;
        IFACEMETHOD(InitializeXaml)() final;

        IFACEMETHOD_(BOOLEAN, InjectWindowMessage)(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _In_ xaml::IXamlRoot* xamlRoot) override;

        IFACEMETHOD(UpdateFontScale)(_In_ float scale) override;

        IFACEMETHOD_(void, SetPrimaryPointerLastPositionOverride)(_In_ wf::Point value, _In_ xaml::IXamlRoot* xamlRoot) override;
        IFACEMETHOD_(void, ClearPrimaryPointerLastPositionOverride)(_In_ xaml::IXamlRoot* xamlRoot) override;

        IFACEMETHOD(WaitForCommitCompletion)() override;

        IFACEMETHOD(IsWindowFocused)(_Out_ BOOLEAN* isFocused, _In_ xaml::IXamlRoot* xamlRoot) override;

        IFACEMETHOD_(void, SetVisibleBounds)(_In_ const wf::Rect& visibleBounds, _In_opt_ xaml::IWindow* iwindow = nullptr) override;

        IFACEMETHOD_(void, SetForceIsFullScreen)(_In_ bool forceIsFullScreen) override;

        IFACEMETHOD(CancelAllConnectedAnimationsAndResetDefaults)() override;

        IFACEMETHOD(GetPopupOverlayElement)(
            _In_ xaml_primitives::IPopup* popup,
            _Outptr_ xaml::IFrameworkElement** overlayElement
            ) override;

        IFACEMETHOD(EnableKeyboardInputEvent)() final;
        IFACEMETHOD_(void, DisableKeyboardInputEvent)() final;
        IFACEMETHOD_(bool, CanFireKeyboardInputEvent)() final;

        IFACEMETHOD(EnablePointerInputEvent)() final;
        IFACEMETHOD_(void, DisablePointerInputEvent)() final;
        IFACEMETHOD_(bool, CanFirePointerInputEvent)() final;

        IFACEMETHOD_(void, DeletePlatformFamilyCache)() final;

        IFACEMETHOD_(void, DeleteResourceDictionaryCaches)() override final;

        IFACEMETHOD_(void, SetLastLayoutExceptionElement)(_In_ xaml::IUIElement* element) final;

        IFACEMETHOD_(void, PostTestCheckForLeaks)(_In_ unsigned int leakThreshold) final;

        IFACEMETHOD_(void, SetIsHolographic)(bool value) override;

        IFACEMETHOD(SimulateInputPaneOccludedRect)(xaml::IXamlRoot* xamlRoot, _In_ wf::Rect occludedRect) override;

        IFACEMETHOD_(void, SetMockUIAClientsListening)() override;
        IFACEMETHOD_(void, ClearMockUIAClientsListening)() override;

        IFACEMETHOD(GetLightsTargetingElement)(_In_ xaml::IUIElement* target, _In_ wfc::IVector<xaml_media::XamlLight*>* lights) override;
        IFACEMETHOD(GetElementsTargetedByLight)(_In_ xaml_media::IXamlLight* light, _In_ wfc::IVector<xaml::UIElement*>* targets) override;
        IFACEMETHOD(GetCountOfVisualsTargeted)(_In_ xaml_media::IXamlLight* light, _In_ xaml::IUIElement* element, _Out_ int* count) override;

        IFACEMETHOD(GetRealCompositionSurface)(_In_ xaml_media::ILoadedImageSurface *loadedImageSurface, _Outptr_ WUComp::ICompositionSurface **realCompositionSurface) override;

        IFACEMETHOD_(void, SetGenericXamlFilePathForMUX)(_In_ HSTRING filePath) override;

        IFACEMETHOD(SetHdrOutputOverride)(bool value) override;

        IFACEMETHOD_(void, GetWantsRenderingEvent)(_Out_ BOOLEAN* wantsRenderingEvent) override;

        IFACEMETHOD_(void, GetWantsCompositionTargetRenderedEvent)(_Out_ BOOLEAN* wantsCompositionTargetRenderedEvent) override;

        IFACEMETHOD_(void, SetThreadingAssertOverride)(_In_ bool value) override;

        IFACEMETHOD_(void, SetCanTickWithNoContent)(bool canTickWithNoContent) override;

        IFACEMETHOD(AddTestLTE)(
            _In_ xaml::IUIElement* lteTarget,
            _In_ xaml::IUIElement* lteParent,
            bool parentIsRootVisual,
            bool parentIsPopupRoot,
            bool isAbsolutelyPositioned,
            _Outptr_ xaml::IUIElement** lte) override;
        IFACEMETHOD(RemoveTestLTE)(_In_ xaml::IUIElement* lte) override;
        IFACEMETHOD(ClearTestLTEs)() override;
        IFACEMETHOD_(void, SetPlayingSoundNodeCallback)(_In_opt_ std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> callback) override;

        IFACEMETHOD(TestGetActualToolTip)(
            _In_ xaml::IUIElement* element,
            _Outptr_ xaml_controls::IToolTip** ppValue) override;

        IFACEMETHOD_(void, GetAnimatedTranslation)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* translation) override;
        IFACEMETHOD_(void, GetAnimatedRotation)(_In_ xaml::IUIElement* element, _Out_ float* rotation) override;
        IFACEMETHOD_(void, GetAnimatedScale)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* scale) override;
        IFACEMETHOD_(void, GetAnimatedTransformMatrix)(_In_ xaml::IUIElement* element, _Out_ wfn::Matrix4x4* transformMatrix) override;
        IFACEMETHOD_(void, GetAnimatedRotationAxis)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* rotationAxis) override;
        IFACEMETHOD_(void, GetAnimatedCenterPoint)(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* centerPoint) override;
        IFACEMETHOD_(void, ScheduleWaitForAnimatedFacadePropertyChanges)(int count) override;

        IFACEMETHOD_(BOOLEAN, IsTrackingEffectiveVisibility)(_In_ xaml::IUIElement* element) override;
        IFACEMETHOD_(BOOLEAN, IsKeepingVisible)(_In_ xaml::IUIElement* element) override;
        IFACEMETHOD(RequestKeepAlive)(_In_ xaml::IUIElement* element) override;
        IFACEMETHOD(ReleaseKeepAlive)(_In_ xaml::IUIElement* element) override;

        _Check_return_ IFACEMETHOD(GetVisibleContentBounds)(_In_ xaml::IUIElement* element, _Out_ wf::Rect* pValue) override;

        IFACEMETHOD(CalculateAvailableMonitorRect)(
            _In_ xaml::IUIElement* element,
            _In_ wf::Point targetPointClientLogical,
            _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
            _Out_ wf::Point* screenOffset,
            _Out_ wf::Point* targetPointScreenPhysical,
            _Out_ wf::Rect* inputPaneOccludeRectScreenLogical) override;

        IFACEMETHOD_(bool, IsStoryboardActive)(_In_ xaml_animation::IStoryboard* storyboard) override;

        IFACEMETHOD(GetElementInputWindow)(_In_ xaml::IUIElement* element, _Out_ HWND* inputHwnd) override;

        IFACEMETHOD_(void, SetSuspendOffThreadDecoding)(bool isOffThreadDecodingSuspended) override;

        IFACEMETHOD_(void, SetSuspendSurfaceUpdates)(bool isSuspended) override;

        IFACEMETHOD(GetUIAWindow)(
            _In_ xaml::IDependencyObject* pElement,
            _In_ HWND hwnd,
            _Outptr_ IInspectable** uiaWindowNoRef) override;

        IFACEMETHOD_(void, RestoreDefaultFlipViewScrollWheelDelay)() override;
        IFACEMETHOD_(void, SetFlipViewScrollWheelDelay)(int scrollWheelDelayMS) override;
        IFACEMETHOD_(void, SetCaretBrowsingModeGlobal)(bool caretBrowsingModeEnable, bool caretBrowsingDialogNotPopAgain) override;

        IFACEMETHOD(ApplyElevationEffect)(_In_ xaml::IUIElement* element, UINT depth = 0) override;

        IFACEMETHOD(SetApplicationLanguageOverride)(_In_ HSTRING languageName) override;
        IFACEMETHOD(ClearApplicationLanguageOverride)() override;

        IFACEMETHOD_(void, CloseAllPopupsForTreeReset)() override;

        IFACEMETHOD_(void, GetAllXamlRoots)(_Outptr_ wfc::IVectorView<xaml::XamlRoot*>** xamlRoots) override;

        IFACEMETHOD(ForceShadowsPolicy)(_In_ BOOLEAN forceShadowsOn) override;
        IFACEMETHOD(ClearShadowPolicyOverrides)() override;

        IFACEMETHOD(SetXamlVisibilityOverride)(_In_ BOOLEAN isVisible) override;

        IFACEMETHOD(SetBrushForXamlRoot)(
            _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
            _In_ IInspectable* xamlRoot,
            _In_ IInspectable* compBrush) override;

        IFACEMETHOD(GetBrushForXamlRoot)(
            _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
            _In_ IInspectable* contentRoot,
            _Outptr_ IInspectable** compBrush) override;

        IFACEMETHOD_(void, StopAllInteractions)() override;

        IFACEMETHOD(GetImageSourceMaxSize)(
            _In_ xaml_media::IImageSource* imageSource,
            _Out_ wf::Size* maxSize) override;

        IFACEMETHOD_(void, GetAllRootVisualsNoRef)(_In_ wfc::IVectorView<IInspectable*>** visualsNoRef) override;

        IFACEMETHOD(DetachMemoryManagerEvents)() override;
        IFACEMETHOD_(void, SetTransparentBackground)(bool value) override;

        IFACEMETHOD(SimulateDeviceLostOnMetadataParse)() override;
        IFACEMETHOD(SimulateDeviceLostOnCreatingSvgDecoder)() override;

        IFACEMETHOD_(void, SetForceDebugSettingsTracingEvents)(bool value) override;

        IFACEMETHOD(GetIslandAndBridge)(
            _In_ xaml::IFrameworkElement* element,
            bool followPopups,
            _Outptr_opt_ IInspectable** island,
            _Outptr_opt_ IInspectable** bridge) override;

        IFACEMETHOD_(void, ThrottleImageTaskDispatcher)(bool enableThrottling, unsigned int numberOfTasksAllowedToDispatch) override;
        IFACEMETHOD_(void, RequestExecuteImageTaskDispatcher)() override;

        IFACEMETHOD(GetElementRenderedVisuals)(_In_ xaml::IUIElement* element, _In_ wfc::IVector<IInspectable*>* visuals) override;

        IFACEMETHOD(GetErrorHandlingTestHooks)(_Outptr_opt_ IXamlErrorTestHooks** errorTestHooks) override;

        IFACEMETHOD(GetAllContentIslands)(_In_ xaml_hosting::IDesktopWindowXamlSource* desktopWindowXamlSource, _In_ wfc::IVector<IInspectable*>* contentIslands) override;

        IFACEMETHOD(GetElementsRenderedCount)(_Out_ int* elementsRendered) override;

    protected:
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void **ppvObject) override;

    private:
        void FreeResourceLibraries();

        ctl::ComPtr<Window> GetTargetWindow(_In_opt_ xaml::IWindow* windowParam);

        DXamlCore* m_pDXamlCoreNoRef = nullptr;
    };
}
