// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DxamlCoreTestHooks_Partial.h"
#include "Window.g.h"
#include "Popup.g.h"
#include "FlyoutBase.g.h"
#include "ToolTip.g.h"
#include "ToolTipService.g.h"
#include "Storyboard.g.h"
#include <RuntimeEnabledFeatures.h>
#include <ErrorHandlerSettings.h>
#include <DependencyLocator.h>
#include <FrameworkTheming.h>
#include <SystemThemingInterop.h>
#include <JupiterTextHelper.h>
#include <XcpAllocation.h>
#include <Storyboard.h>
#include "DefaultStyles.h"
#include <FocusMgr.h>
#include <DragDropInternal.h>
#include "InternalDebugInteropModel.h"
#include "theming\inc\Theme.h"
#include <collectionbase.h>
#include <UIElement_Partial.h>
#include <DCompTreeHost.h>
#include <XamlLight.h>
#include <XamlLight.g.h>
#include <UIElement.g.h>
#include <FlipView.g.h>
#include <DesktopUtility.h>
#include <DesignMode.h>
#include "VisualTreeHelper.h"
#include <ContentDialog_Partial.h>
#include <RootScrollViewer_Partial.h>
#include "JupiterControl.h"
#include "AsyncImageDecoder.h"
#include <ElevationHelper.h>
#include "CaretBrowsingGlobal.h"
#include <ContentRoot.h>
#include "ContentRootCoordinator.h"
#include "XamlRoot.g.h"
#include "FocusObserver.h"
#include "XamlCompositionBrushBase.g.h"
#include "RootScale.h"
#include <PhoneImports.h>
#include <FrameworkUdk/BackButtonIntegration.h>
#include <ImageSource.g.h>
#include <ImageSource.h>
#include "HWTextureMgr.h"
#include <Hyperlink.h>
#include "ListViewBaseItemChrome.h"
#include "AsyncCopyToSurfaceTask.h"
#include "EncodedImageData.h"
#include "SvgImageDecoder.h"
#include "XboxUtility.h"
#include "ElementSoundPlayerService_Partial.h"
#include "ImageTaskDispatcher.h"
#include "d3D11SharedDeviceGuard.h"
#include "D3D11Device.h"
#include "xvector.h"
#include "PCRenderDataList.h"
#include "IXamlTestHooks-errors.h"
#include "DesktopWindowXamlSource_Partial.h"
#include "XamlIslandRoot_Partial.h"
#include "HWWalk.h"
#include "LoadLibraryAbs.h"

#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data

static DependencyLocator::Dependency<RuntimeFeatureBehavior::RuntimeEnabledFeatureDetector> s_runtimeEnabledFeatureDetector;
static DependencyLocator::Dependency<ErrorHandling::ErrorHandlingSettings> s_errorHandlingSettings;

IFACEMETHODIMP_(void) EmitHeapHandleExportEtwEvent();

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
DxamlCoreTestHooks::QueryInterfaceImpl(
    _In_ REFIID riid,
    _Outptr_ void** ppvObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(IXamlTestHooks)))
    {
        *ppvObject = static_cast<IXamlTestHooks*>(this);
    }
    else
    {
        return DirectUI::DxamlCoreTestHooksGenerated::QueryInterfaceImpl(riid, ppvObject);
    }

    AddRefOuter();
    return S_OK;
}

_Check_return_ HRESULT
DxamlCoreTestHooksFactory::GetForCurrentThreadImpl(_Outptr_ xaml::IDxamlCoreTestHooks** ppResult)
{
    *ppResult = nullptr;

    ctl::ComPtr<xaml::IDxamlCoreTestHooks> spRetVal;
    DXamlCore* core = DXamlCore::GetCurrent();

    if (core)
    {
        ctl::ComPtr<DxamlCoreTestHooks> spTestHooks = core->GetTestHooks();
        IFC_RETURN(spTestHooks.As(&spRetVal));
    }
    else
    {
        // We return NULL when the calling thread doesn't have a DXamlCore instance.
        // This can happen if called on a non-UI thread, or if it's called on a UI
        // thread before we've initialized DXamlCore or after we've deinitialized it.
    }

    return spRetVal.CopyTo(ppResult);
}

void DeinitializeDll();

_Check_return_ HRESULT
DxamlCoreTestHooksFactory::PerformProcessWideLeakDetectionImpl(int threshold)
{
    // Simulate DLL detach.
    ::DeinitializeDll();
#if XCP_MONITOR
    ::XcpCheckLeaks(static_cast<unsigned int>(threshold));
#endif
    return S_OK;
}

_Check_return_ HRESULT DxamlCoreTestHooks::GetVisibleContentBounds(_In_ xaml::IUIElement* element, _Out_ wf::Rect* pValue)
{
    CDependencyObject* dependencyObject = nullptr;
    if (element)
    {
        dependencyObject = static_cast<DirectUI::UIElement*>(element)->GetHandle();
    }
    return DXamlCore::GetCurrent()->GetVisibleContentBoundsForElement(dependencyObject, pValue);
}

IFACEMETHODIMP DxamlCoreTestHooks::CalculateAvailableMonitorRect(
    _In_ xaml::IUIElement* element,
    _In_ wf::Point targetPointClientLogical,
    _Out_ wf::Rect* availableMonitorRectClientLogicalResult,
    _Out_ wf::Point* screenOffset,
    _Out_ wf::Point* targetPointScreenPhysical,
    _Out_ wf::Rect* inputPaneOccludeRectScreenLogical)
{
    return DXamlCore::GetCurrent()->CalculateAvailableMonitorRect(
        static_cast<DirectUI::UIElement*>(element) /*pTargetElement*/,
        targetPointClientLogical,
        availableMonitorRectClientLogicalResult,
        screenOffset,
        targetPointScreenPhysical,
        inputPaneOccludeRectScreenLogical);
}

// Simulate a device-lost error to trigger re-creation of DComp device(s).
// Called by the test framework.
IFACEMETHODIMP
    DxamlCoreTestHooks::SimulateDeviceLost()
{
    return DXamlCore::GetCurrent()->SimulateDeviceLost(false /* resetVisuals */, false /* resetDManip */);
}

// Reset the device and DComp visuals.
IFACEMETHODIMP DxamlCoreTestHooks::ResetDeviceAndVisuals()
{
    return DXamlCore::GetCurrent()->SimulateDeviceLost(true /* resetVisuals */, false /* resetDManip */);
}

// Reset the device but not the DComp visuals.
IFACEMETHODIMP DxamlCoreTestHooks::ResetDeviceOnly()
{
    return DXamlCore::GetCurrent()->SimulateDeviceLost(false /* resetVisuals */, false /* resetDManip */);
}

// Reset the device, DComp visuals, and compositors inside of DManip
IFACEMETHODIMP
    DxamlCoreTestHooks::ResetDeviceAndVisualsAndDManip()
{
    return DXamlCore::GetCurrent()->SimulateDeviceLost(true /* resetVisuals */, true /* resetDManip */);
}

// Simulate a device lost error on the next off-thread image decode.
IFACEMETHODIMP DxamlCoreTestHooks::SimulateDeviceLostOnOffThreadImageUpload()
{
    AsyncCopyToSurfaceTask::s_testHook_ForceDeviceLostOnNextUpload = true;
    return S_OK;
}

// Simulate a device lost during startup swallowed by a call to CCoreServices::GetMaxTextureSize. This puts DCompTreeHost
// in a partially initialized state. We see this in the wild when an app loads a LoadedImageSurface during startup (e.g.
// via the AcrylicBrush in Microsoft_UI_Xaml_Controls), and the device is already lost. The LoadedImageSurface::InitFromMemory
// call will start decoding, which checks the max texture size first. This is actually the first call that initializes the
// DCompTreeHost. Normally we initialize through CCoreServices::BuildDeviceRelatedResources when we render a frame, but this
// is before the first frame is rendered.
IFACEMETHODIMP DxamlCoreTestHooks::SimulateSwallowedDeviceLostOnStartup()
{
    CCoreServices* core = DXamlCore::GetCurrent()->GetHandle();
    // Note: Don't call ReleaseDeviceResources. That marks the CCoreServices::m_deviceLost state as well. In the state that
    // we want to simulate, the device is not considered lost. We hit a device host HR during initialization, then swallowed
    // that failure without marking anything.
    core->CleanupDeviceRelatedResources(true /* releaseDCompDevice */, true /* isDeviceLost */);
    core->GetMaxTextureSize();
    return S_OK;
}

void DxamlCoreTestHooks::SetDCompDeviceLeakDetectionEnabled(bool enableLeakDetection)
{
    CCoreServices* core = DXamlCore::GetCurrent()->GetHandle();
    core->SetDCompDeviceLeakDetectionEnabled(enableLeakDetection);
}

// Determine whether there is a drag/drop in progress
IFACEMETHODIMP_(bool) DxamlCoreTestHooks::IsDragDropInProgress()
{
    return !!DXamlCore::GetCurrent()->GetDragDrop()->GetIsDragDropInProgress();
}

// Returns the DComp device for the current thread. If there are multiple windows
// this will return the device associated with the calling thread.
IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetDCompDevice(_Outptr_ IDCompositionDesktopDevice **ppDCompDevice) const
{
    DXamlCore::GetCurrent()->GetDCompDevice(ppDCompDevice);
}

IFACEMETHODIMP DxamlCoreTestHooks::GetD3D11GraphicsDeviceAddress(_Out_ INT64* ppCD3D11Device) const
{
    CD3D11Device *d3d11Device = DXamlCore::GetCurrent()->GetHandle()->GetGraphicsDevice();
    *ppCD3D11Device = reinterpret_cast<INT64>(d3d11Device->TestHook_GetDevice());
    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::MarkDeviceInstanceLost() const
{
    CD3D11Device *d3d11Device = DXamlCore::GetCurrent()->GetHandle()->GetGraphicsDevice();
    d3d11Device->TestHook_LoseDevice();
    return S_OK;
}

// Forces the window to a certain size. Called by the test framework.
// The main idea behind this function is that many tests which check the results of layout or rendering
// require a particular size (height and/or width) to be used to produce the right results.
// This used to be most important when the VM's people created didn't always have the minimum
// display width expected by some tests. Overriding the width made the tests reliable regardless
// of the display resolution of the VM/machine. This worked really well in a single-window UWP
// world, but is more problematic with MultiWindow support. The first issue being that we are
// not guaranteed to have a Window instance with which to override when this is first called during
// test module setup. Some tests call also call this explicitly after rendering some content, which
// should be fine. The second issue is that we will never have a Window instance when running in WPF,
// which tests have historically relied on the dummy UWP window to fake this.
IFACEMETHODIMP
    DxamlCoreTestHooks::SetWindowSizeOverride(
        _In_ const wf::Size& size,
        _In_ const wf::Rect& layoutBounds,
        _In_ float zoomScaleOverride,
        _In_ bool scaleWindowSizeByScaleFactor,
        _In_opt_ xaml::IWindow* iwindow
    )
{
    HRESULT hr = S_OK;

    // NOTE: pre 1.1 legacy assumption here was that UWP window would always be available.
    // with MultiWindow support being added in late 2021, this no longer works when called
    // before a window is actually available. This behavior is still the same with the UWP Window
    // since that window is created early (in FrameworkView::Initialize), but is now no-op in desktop
    // which previously had no effect anyway since it would -still- invoke overrides on the UWP Window.
    auto window = GetTargetWindow(iwindow);

    XCP_FAULT_ON_FAILURE(size.Width >= 0 && size.Height >= 0);
    XSIZE sizeOverride{static_cast<int>(size.Width), static_cast<int>(size.Height)};

    if (window)
    {
        window->SetWidthOverride(static_cast<UINT32>(XcpCeiling(size.Width)));
        window->SetHeightOverride(static_cast<UINT32>(XcpCeiling(size.Height)));
        window->SetHasSizeOverrides((window->GetWidthOverride() > 0) && (window->GetHeightOverride() > 0));

        window->SetLayoutBoundsOverrides(layoutBounds);

        // Reset the visible bounds overrides whenever the window size is overridden.
        window->SetVisibleBoundsOverrides(wf::Rect());

        sizeOverride.Width = static_cast<int>(window->GetWidthOverride());
        sizeOverride.Height = static_cast<int>(window->GetHeightOverride());
    }

    if (scaleWindowSizeByScaleFactor && zoomScaleOverride > 0)
    {
        sizeOverride.Width = static_cast<int>(sizeOverride.Width * zoomScaleOverride);
        sizeOverride.Height = static_cast<int>(sizeOverride.Height  * zoomScaleOverride);
    }
    IFC(DXamlCore::GetCurrent()->SetWindowSizeOverride(&sizeOverride, zoomScaleOverride));

    //
    // Note: Don't fire OnWindowSizeChanged. This is used only for test hooks, and
    // tests that use it won't care. Don't do anything with the underlying ICoreWindow
    // either. Its bounds cannot be set. We already stored the override size to be
    // reported back in get_Bounds.
    //

Cleanup:
    RRETURN(hr);
}

float DxamlCoreTestHooks::GetZoomScale() const
{
    const auto coreServices = DXamlCore::GetCurrent()->GetHandle();
    const auto contentRootCoordinator = coreServices->GetContentRootCoordinator();

    // We're being called from a test, so check for the common cases.
    const auto& roots = contentRootCoordinator->GetContentRoots();

    // If there's a single ContentRoot, then go with that.
    if (roots.size() == 1)
    {
        const auto root = roots[0];
        const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);
        return scale;
    }
    // If there are two ContentRoots, then we might be in a "-hostingMode wpf" test with a Xaml island.
    // In this case, the first ContentRoot is the CoreWindow and the second ContentRoot is the island.
    // Use the island.
    else if (roots.size() == 2
        && roots[0]->GetType() == CContentRoot::Type::CoreWindow
        && roots[1]->GetType() == CContentRoot::Type::XamlIslandRoot
        // XamlIslandTests does not use DesktopWindowXamlSource and does not use an AppWindow. It does its
        // own thing and expects inject input to not be scaled at all. Those tests don't have a ContentIsland.
        && roots[1]->GetXamlIslandRootNoRef()->GetContentIsland())
    {
        const auto root = roots[1];
        const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);
        return scale;
    }
    // Otherwise we're in some unrecognized pattern. Use the default CoreWindow ContentRoot.
    else
    {
        const auto root = contentRootCoordinator->Unsafe_IslandsIncompatible_CoreWindowContentRoot();
        const auto scale = RootScale::GetRasterizationScaleForContentRoot(root);
        return scale;
    }
}

IFACEMETHODIMP_(void)
    DxamlCoreTestHooks::OverrideTrimImageResourceDelay(_In_ bool enabled)
{
    DXamlCore::GetCurrent()->OverrideTrimImageResourceDelay(enabled);
}

IFACEMETHODIMP_(void)
    DxamlCoreTestHooks::CleanupReleaseQueue()
{
    if (!DXamlCore::GetCurrent()->IsFinalReleaseQueueEmpty())
    {
        DXamlCore::GetCurrent()->ReleaseQueuedObjects();
    }
}

IFACEMETHODIMP DxamlCoreTestHooks::GetFinalReleaseQueue(_Outptr_ wfc::IVectorView<IInspectable*>** queue)
{
    return DXamlCore::GetCurrent()->GetFinalReleaseQueue(queue);
}

IFACEMETHODIMP_(bool) DxamlCoreTestHooks::IsRuntimeEnabledFeatureEnabled(_In_ RuntimeFeatureBehavior::RuntimeEnabledFeature feature)
{
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    return runtimeEnabledFeatureDetector->IsFeatureEnabled(feature);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetRuntimeEnabledFeatureOverride(_In_ RuntimeFeatureBehavior::RuntimeEnabledFeature feature, _In_ bool enabled, _Out_opt_ bool* wasPreviouslyEnabled)
{
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    runtimeEnabledFeatureDetector->SetFeatureOverride(feature, enabled, wasPreviouslyEnabled);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetErrorHandlerCallback(_In_ std::shared_ptr<std::function<void(const ErrorHandling::XamlFailureInfo&)>> callback)
{
    const auto errorHandlingSettings = ErrorHandling::GetErrorHandlingSettings();
    errorHandlingSettings->SetErrorHandlingCallback(*callback);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetLoggerCallback(_In_ std::shared_ptr<LoggerCallback> callback)
{
    const auto errorHandlingSettings = ErrorHandling::GetErrorHandlingSettings();
    errorHandlingSettings->SetLoggerCallback(*callback);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetPostTickCallback(_In_ std::function<void()> callback)
{
    DXamlCore::GetCurrent()->SetPostTickCallback(callback);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetPlayingSoundNodeCallback(_In_ std::function<void(xaml::ElementSoundKind, BOOLEAN isSpatialAudio, float x, float y, float z, double volume)> callback)
{
    ElementSoundPlayerService* service = DXamlCore::GetCurrent()->GetElementSoundPlayerServiceNoRef();
    service->SetPlayingSoundNodeCallback(callback);
}

IFACEMETHODIMP
    DxamlCoreTestHooks::GetDependencyObjectPropertyValues(
        _In_ xaml::IDependencyObject* pObject,
        _In_ bool excludeDefaultPropertyValues,
        _In_ PFNPROPERTYVALUETOSTRING pfnPropertyValueToString,
        _Out_ std::shared_ptr<std::map<std::wstring, std::wstring>>& spPropertyValuesMap
    )
{
    std::map<std::wstring, std::wstring> localPropertyValuesMap;

    ctl::ComPtr<xaml::IDependencyObject> spObject(pObject);

    std::vector<DebugTool::DebugPropertyInfo> propInfoList;
    IFC_RETURN(DebugTool::GetPropertiesForDO(
        pObject,
        true,
        pfnPropertyValueToString,
        propInfoList
    ));

    for (auto& iter : propInfoList)
    {
        wrl_wrappers::HStringReference propertyName(iter.GetName());

        if (excludeDefaultPropertyValues &&
            iter.GetSource() == BaseValueSourceDefault &&
            !(iter.IsNonDefaultValue() &&
                (propertyName == wrl_wrappers::HStringReference(L"ActualHeight") ||
                propertyName == wrl_wrappers::HStringReference(L"ActualWidth") ||
                propertyName == wrl_wrappers::HStringReference(L"RenderSize"))))
        {
            continue;
        }

        // RuntimeId can change between runs, so exclude it from our list otherwise
        // it will destabilize our stress runs.
        if (propertyName == wrl_wrappers::HStringReference(L"RuntimeId"))
        {
            continue;
        }

        localPropertyValuesMap.insert(std::pair<std::wstring, std::wstring>(
            std::wstring(iter.GetName()),
            std::wstring(iter.GetValue())));
    }

    spPropertyValuesMap = std::make_shared<std::map<std::wstring, std::wstring>>(std::move(localPropertyValuesMap));

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ClearAllRuntimeFeatureOverrides()
{
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    runtimeEnabledFeatureDetector->ClearAllFeatureOverrides();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::EmitHeapHandleExportEtwEvent()
{
    ::EmitHeapHandleExportEtwEvent();
}

IFACEMETHODIMP
    DxamlCoreTestHooks::SetStoryboardStartedCallback(
        _In_ HRESULT(*pCallback)(xaml_animation::IStoryboard*, xaml::IUIElement*)) /*override*/
{
    if (pCallback)
    {
        IFC_RETURN(CoreImports::CStoryboard_SetStoryboardStartedCallback([pCallback](CDependencyObject* pStoryboardAsDO, CDependencyObject* pTargetAsDO)
        {
            ctl::ComPtr<DependencyObject> spStoryboardAsDO;
            ctl::ComPtr<xaml_animation::IStoryboard> spStoryboard;
            IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pStoryboardAsDO, &spStoryboardAsDO));
            IFC_RETURN(spStoryboardAsDO.As(&spStoryboard));

            ctl::ComPtr<DependencyObject> spTargetAsDO;
            ctl::ComPtr<xaml::IUIElement> spTarget;
            // Many top-level storyboards don't have a target element themselves, instead
            // serving as a container for children Storyboards that atually target elements.
            if (pTargetAsDO)
            {
                IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pTargetAsDO, &spTargetAsDO));
                IFC_RETURN(spTargetAsDO.As(&spTarget));
            }

            return pCallback(spStoryboard.Get(), spTarget.Get());
        }));
    }
    else
    {
        IFC_RETURN(CoreImports::CStoryboard_SetStoryboardStartedCallback(nullptr));
    }
    return S_OK;
}

IFACEMETHODIMP
    DxamlCoreTestHooks::InvokeInternalCommand(
        _In_ xaml::IDependencyObject* pObject,
        _In_ XamlInternalCommand command,
        _In_opt_ std::vector<wf::IPropertyValue*>* pArgs,
        _Out_opt_ wf::IPropertyValue **ppReturnValue)
{
    ctl::ComPtr<xaml::IDependencyObject> spObject(pObject);

    switch (command)
    {
        case Popup_SetWindowed:
        {
            CPopup *pPopup = static_cast<CPopup*>(spObject.Cast<Popup>()->GetHandle());
            pPopup->SetIsWindowed();
            break;
        }

        case FrameworkElement_HasPeer:
        {
            ctl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
            ctl::ComPtr<wf::IPropertyValue> spBooleanValue;
            FrameworkElement *pFrameworkElement = spObject.Cast<FrameworkElement>();
            wrl_wrappers::HString name;

            wf::IPropertyValue *pArg = (*pArgs)[0];
            IFC_RETURN(pArg->GetString(name.ReleaseAndGetAddressOf()));

            xref_ptr<CDependencyObject> spCoreDO =
                DXamlCore::GetCurrent()->GetHandle()->TryGetElementByName(
                    XSTRING_PTR_EPHEMERAL_FROM_HSTRING(name.Get()), pFrameworkElement->GetHandle());
            IFCPTR_RETURN(spCoreDO);

            IFC_RETURN(ctl::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
                &spPropertyValueFactory));
            IFC_RETURN(spPropertyValueFactory->CreateBoolean(!!spCoreDO->HasManagedPeer(), &spBooleanValue));
            *ppReturnValue = spBooleanValue.Detach();

            break;
        }

        case Popup_GetWindow:
        {
            // Looking at the usage in the tests, these are attempting to retrieve the window rect.
            // We should return the positioning HWND and not the input HWND.
            // If any tests need the input HWND, we will need a new separate API to retrieve it.
            CPopup *pPopup = static_cast<CPopup*>(spObject.Cast<DependencyObject>()->GetHandle());
            HWND hwnd = pPopup->GetPopupPositioningWindow();

            wrl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
            wrl::ComPtr<wf::IPropertyValue> spHandleValue;
            wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
                &spPropertyValueFactory);
            spPropertyValueFactory->CreateUInt64(reinterpret_cast<uintptr_t>(hwnd), &spHandleValue);
            spHandleValue.CopyTo(ppReturnValue);
            break;
        }

        case FlyoutBase_GetWindow:
        {
            // Looking at the usage in the tests, these are attempting to retrieve the window rect.
            // We should return the positioning HWND and not the input HWND.
            // If any tests need the input HWND, we will need a new separate API to retrieve it.
            CPopup *pPopup = static_cast<CPopup*>(spObject.Cast<FlyoutBase>()->m_tpPopup.Cast<Popup>()->GetHandle());
            HWND hwnd = pPopup->GetPopupPositioningWindow();

            wrl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
            wrl::ComPtr<wf::IPropertyValue> spHandleValue;
            wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
                &spPropertyValueFactory);
            spPropertyValueFactory->CreateUInt64(reinterpret_cast<uintptr_t>(hwnd), &spHandleValue);
            spHandleValue.CopyTo(ppReturnValue);
            break;
        }

        case ToolTip_GetWindow:
        {
            // Looking at the usage in the tests, these are attempting to retrieve the window rect.
            // We should return the positioning HWND and not the input HWND.
            // If any tests need the input HWND, we will need a new separate API to retrieve it.
            CPopup *pPopup = static_cast<CPopup*>((spObject.Cast<ToolTip>()->m_wrPopup).AsOrNull<Popup>()->GetHandle());
            HWND hwnd = pPopup->GetPopupPositioningWindow();

            wrl::ComPtr<wf::IPropertyValueStatics> spPropertyValueFactory;
            wrl::ComPtr<wf::IPropertyValue> spHandleValue;
            wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
                &spPropertyValueFactory);
            spPropertyValueFactory->CreateUInt64(reinterpret_cast<uintptr_t>(hwnd), &spHandleValue);
            spHandleValue.CopyTo(ppReturnValue);
            break;
        }

        default:
            return E_NOTIMPL;
    }

    return S_OK;
}

IFACEMETHODIMP
DxamlCoreTestHooks::SetApplicationRequestedTheme(xaml::ApplicationTheme theme)
{
    auto dp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Application_RequestedTheme);

    CValue value;
    value.Set(static_cast<DirectUI::ApplicationTheme>(theme), KnownTypeIndex::ApplicationTheme);

    auto coreApp = DXamlCore::GetCurrent()->GetCoreAppHandle();
    IFC_RETURN(coreApp->SetValue(dp, value));

    return S_OK;
}

IFACEMETHODIMP
DxamlCoreTestHooks::UnsetApplicationRequestedTheme()
{
    auto pCore = DXamlCore::GetCurrent()->GetHandle();
    pCore->GetFrameworkTheming()->UnsetRequestedTheme();
    return S_OK;
}

IFACEMETHODIMP
DxamlCoreTestHooks::OverrideSystemTheme(xaml::ApplicationTheme theme)
{
    auto pCore = DXamlCore::GetCurrent()->GetHandle();

    SystemThemingInterop::OverrideSystemTheme(theme == xaml::ApplicationTheme_Light ? Theming::Theme::Light : Theming::Theme::Dark);

    // Trigger a theme change to pick up the new accent color.
    IFC_RETURN(pCore->GetFrameworkTheming()->OnThemeChanged(true));

    return S_OK;
}

IFACEMETHODIMP
DxamlCoreTestHooks::OverrideHighContrast(std::shared_ptr<std::list<std::pair<int, unsigned int>>> sysColorPalette)
{
    auto pCore = DXamlCore::GetCurrent()->GetHandle();

    SystemThemingInterop::OverrideHighContrast(sysColorPalette);

    // Trigger a theme change to pick up the high-contrast setting change.
    // Force an update only if the specified color palette is not null to make
    // sure we update brush resources.  If it's null, then we're turning HC off
    // and that will automatically cause a theme walk.
    IFC_RETURN(pCore->GetFrameworkTheming()->OnThemeChanged(sysColorPalette != nullptr));

    return S_OK;
}

IFACEMETHODIMP
DxamlCoreTestHooks::OverrideAccentColor(unsigned int accentColor)
{
    auto pCore = DXamlCore::GetCurrent()->GetHandle();

    SystemThemingInterop::OverrideAccentColor(accentColor);

    // Trigger a theme change to pick up the new accent color.
    IFC_RETURN(pCore->GetFrameworkTheming()->OnThemeChanged(true));

    return S_OK;
}

IFACEMETHODIMP
DxamlCoreTestHooks::RemoveThemingOverrides()
{
    auto pCore = DXamlCore::GetCurrent()->GetHandle();

    SystemThemingInterop::RemoveThemingOverrides();

    // Trigger a theme change to pick up the new accent color.
    IFC_RETURN(pCore->GetFrameworkTheming()->OnThemeChanged());

    return S_OK;
}

IFACEMETHODIMP
DxamlCoreTestHooks::SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection)
{
    return DXamlCore::GetCurrent()->SetSystemFontCollectionOverride(pFontCollection);
}

IFACEMETHODIMP
DxamlCoreTestHooks::ShouldUseTypographicFontModel(_Out_ bool* useDWriteTypographicModel)
{
    return DXamlCore::GetCurrent()->ShouldUseTypographicFontModel(useDWriteTypographicModel);
}

IFACEMETHODIMP DxamlCoreTestHooks::GetGripperData(_In_ IInspectable* textControl, _Inout_ JupiterGripperData* returnValue)
{
    ctl::ComPtr<DependencyObject> textControlAsDO;

    IFC_RETURN(ctl::do_query_interface(textControlAsDO, textControl));
    IFC_RETURN(JupiterTextHelper::GetGripperData(textControlAsDO->GetHandle(), returnValue));

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::ResetMetadata()
{
    DXamlCore::GetCurrent()->GetDefaultStyles()->GetStyleCache()->Clear();
    MetadataAPI::Reset();
    IFC_RETURN(DXamlCore::GetCurrent()->GetHandle()->RefreshXamlSchemaContext());

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::ClearDefaultLanguageString()
{
    IFC_RETURN(DXamlCore::GetCurrent()->GetHandle()->ClearDefaultLanguageString());
    return S_OK;
}


IFACEMETHODIMP DxamlCoreTestHooks::CreateLoopingSelector(_Outptr_ IInspectable** ppLoopingSelector)
{
    auto xamlControlsTestHookCreateLoopingSelectorPtr = reinterpret_cast<decltype(&XamlControlsTestHookCreateLoopingSelector)>(::GetProcAddress(GetPhoneModule(), "XamlControlsTestHookCreateLoopingSelector"));
    return xamlControlsTestHookCreateLoopingSelectorPtr(ppLoopingSelector);
}

IFACEMETHODIMP DxamlCoreTestHooks::InjectBackButtonPress(_Out_ BOOLEAN* pHandled)
{
    BOOL handled = FALSE;
    IFC_RETURN(BackButtonIntegration_InjectBackButtonPress(&handled));
    *pHandled = (handled) ? TRUE : FALSE;
    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::IsWindowActivated(_In_ xaml::IXamlRoot* xamlRoot, _Out_ BOOLEAN* activated)
{
    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    *activated = spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef()->GetFocusManagerNoRef()->GetFocusObserverNoRef()->IsActivated();
    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ResetAtlasSizeHint()
{
    m_pDXamlCoreNoRef->ResetAtlasSizeHint();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ShrinkApplicationViewVisibleBounds(_In_ bool enabled, _In_opt_ xaml::IWindow* iwindow)
{
    auto window = GetTargetWindow(iwindow);
    if (!window)
    {
        return;
    }

    window->SetShrinkApplicationViewVisibleBounds(enabled);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::RequestReplayPreviousPointerUpdate_TempTestHook()
{
    m_pDXamlCoreNoRef->RequestReplayPreviousPointerUpdate_TempTestHook();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ResetVisualTree()
{
    m_pDXamlCoreNoRef->GetHandle()->ResetCoreWindowVisualTree();
}

static HRESULT EnsureClassInitialized(
    const xstring_ptr& typeName)
{
    const CClassInfo* type = nullptr;

    IFC_RETURN(MetadataAPI::GetClassInfoByFullName(
        typeName,
        &type));

    IFC_RETURN(const_cast<CClassInfo*>(type)->RunClassConstructorIfNecessary());

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::EnsureSatelliteDLLCustomDPCleanup()
{
    // Microsoft.UI.Xaml.controls.dll
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_ColorPicker, L"Microsoft.UI.Xaml.Controls.ColorPicker");
    IFC_RETURN(EnsureClassInitialized(c_ColorPicker));

    // Microsoft.UI.Xaml.phone.dll
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_Pivot, L"Microsoft.UI.Xaml.Controls.Pivot");
    IFC_RETURN(EnsureClassInitialized(c_Pivot));

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::ShutdownXaml()
{
    IFC_RETURN(DXamlCore::Deinitialize(DeinitializationType::ToIdle));

    ctl::__module.ResetFactories();

    MetadataAPI::Destroy();

    DesignerInterop::SetAllowDesignModeMock(false);

    AsyncCopyToSurfaceTask::s_testHook_ForceDeviceLostOnNextUpload = false;
    EncodedImageData::s_testHook_ForceDeviceLostOnMetadataParse = false;
    SvgImageDecoder::s_testHook_ForceDeviceLostOnCreatingSvgDecoder = false;
    return S_OK;
}

IFACEMETHODIMP_(BOOLEAN) DxamlCoreTestHooks::InjectWindowMessage(_In_ UINT msg, _In_ UINT wParam, _In_ UINT lParam, _In_ xaml::IXamlRoot* xamlRoot)
{
    return DXamlCore::GetCurrent()->InjectWindowMessage(msg, wParam, lParam, xamlRoot);
}

IFACEMETHODIMP DxamlCoreTestHooks::UpdateFontScale(_In_ float scale)
{
    return DXamlCore::GetCurrent()->UpdateFontScale(scale);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SimulateSuspendToPauseAnimations()
{
    m_pDXamlCoreNoRef->SimulateSuspendToPauseAnimations();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetTimeManagerClockOverrideConstant(double newTime)
{
    m_pDXamlCoreNoRef->SetTimeManagerClockOverrideConstant(newTime);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::CleanUpAfterTest()
{
    m_pDXamlCoreNoRef->CleanUpAfterTest();
}

IFACEMETHODIMP DxamlCoreTestHooks::InitializeXaml()
{
#if XCP_MONITOR
    ::XcpIgnoreAllOutstandingAllocations();
#endif
    DesignerInterop::SetAllowDesignModeMock(true);

    return DXamlCore::Initialize(InitializationType::FromIdle);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::PostTestCheckForLeaks(_In_ unsigned int leakThresholdForSummary)
{
    // Freeing memory allocated for ErrorContext extra info to avoid leak reports because ErrorContextThreadDeinit()
    // has not been called yet for the thread that allocated the extra info strings.
    ResetErrorContextsExtraInfo();

#if XCP_MONITOR
    ::XcpCheckLeaks(leakThresholdForSummary);
#endif

    DXamlCore::GetCurrent()->CheckForLeaks();
}

IFACEMETHODIMP DxamlCoreTestHooks::SimulateInputPaneOccludedRect(_In_ xaml::IXamlRoot* xamlRoot, _In_ wf::Rect occludedRect)
{
    ctl::ComPtr<DirectUI::XamlRoot> xamlRootImpl;
    IFC_RETURN(ctl::do_query_interface(xamlRootImpl, xamlRoot));

    ASSERT(xamlRootImpl);
    xamlRootImpl->SetSimulatedInputPaneOccludedRect(occludedRect);
    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::FireDCompAnimationCompleted(_In_ xaml_animation::IStoryboard* storyboard)
{
    auto pSB = static_cast<DirectUI::Storyboard*>(storyboard)->GetHandle();
    static_cast<CStoryboard*>(pSB)->FireDCompAnimationCompleted();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SimulateResumeToResumeAnimations()
{
    m_pDXamlCoreNoRef->SimulateResumeToResumeAnimations();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetIsSuspended(bool isSuspended)
{
    m_pDXamlCoreNoRef->SetIsSuspended(isSuspended);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetIsRenderEnabled(bool value)
{
    m_pDXamlCoreNoRef->SetIsRenderEnabled(value);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetIsHolographic(bool value)
{
    m_pDXamlCoreNoRef->SetIsHolographic(value);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ForceDisconnectRootOnSuspend(bool forceDisconnectRootOnSuspend)
{
    m_pDXamlCoreNoRef->ForceDisconnectRootOnSuspend(forceDisconnectRootOnSuspend);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::TriggerSuspend(bool isTriggeredByResourceTimer, bool allowOfferResources)
{
    m_pDXamlCoreNoRef->TriggerSuspend(isTriggeredByResourceTimer, allowOfferResources);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::TriggerResume()
{
    m_pDXamlCoreNoRef->TriggerResume();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::TriggerLowMemory()
{
    m_pDXamlCoreNoRef->TriggerLowMemory();
}

IFACEMETHODIMP DxamlCoreTestHooks::TestGetGlobalBoundsForUIElement(_In_ xaml::IUIElement* element, _In_ BOOLEAN ignoreClipping, _Out_ wf::Rect* bounds)
{
    XRECTF_RB pBounds = { };
    auto pElement = static_cast<DirectUI::UIElement*>(element)->GetHandle();

    IFC_RETURN(pElement->GetGlobalBounds(&pBounds, ignoreClipping));

    // NOTE: This conversion from LTRB to XYWH loses accuracy when infinite bounds are involved.
    // It is up to the caller to determine what to do in the case of infinite bounds; the bounds
    // returned in the inf case will include either a W or H of +/-inf, which is an adequate check.
    bounds->X = pBounds.left;
    bounds->Y = pBounds.top;
    bounds->Width = pBounds.right - pBounds.left;
    bounds->Height = pBounds.bottom - pBounds.top;

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SimulateThemeChanged()
{
    m_pDXamlCoreNoRef->SimulateThemeChanged();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetLastInputMethod(_In_ xaml_input::LastInputDeviceType lastInputType, _In_ xaml::IXamlRoot* xamlRoot)
{
    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef()->GetInputManager().SetLastInputDeviceType(static_cast<DirectUI::InputDeviceType>(lastInputType));
}

IFACEMETHODIMP_(xaml_input::LastInputDeviceType) DxamlCoreTestHooks::GetLastInputMethod(_In_ xaml::IXamlRoot* xamlRoot)
{
    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    CContentRoot* contentRoot = spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef();
    return static_cast<xaml_input::LastInputDeviceType>(contentRoot->GetInputManager().GetLastInputDeviceType());
}

HRESULT DxamlCoreTestHooks::WaitForCommitCompletion()
{
    return m_pDXamlCoreNoRef->WaitForCommitCompletion();
}

IFACEMETHODIMP DxamlCoreTestHooks::IsWindowFocused(_Out_ BOOLEAN* isFocused, _In_ xaml::IXamlRoot* xamlRoot)
{
    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    *isFocused = spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef()->GetFocusManagerNoRef()->IsPluginFocused() == TRUE;

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetPrimaryPointerLastPositionOverride(_In_ wf::Point value, _In_ xaml::IXamlRoot* xamlRoot)
{
    XPOINTF point = {value.X, value.Y};

    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef()->GetInputManager().SetPrimaryPointerLastPositionOverride(point);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ClearPrimaryPointerLastPositionOverride(_In_ xaml::IXamlRoot* xamlRoot)
{
    ctl::ComPtr<XamlRoot> spXamlRoot;
    IFCFAILFAST(xamlRoot->QueryInterface(IID_PPV_ARGS(&spXamlRoot)));

    spXamlRoot->GetVisualTreeNoRef()->GetContentRootNoRef()->GetInputManager().ClearPrimaryPointerLastPositionOverride();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetVisibleBounds(_In_ const wf::Rect& visibleBounds, _In_opt_ xaml::IWindow* iwindow)
{
    auto window = GetTargetWindow(iwindow);
    if (!window)
    {
        return;
    }

    window->SetVisibleBoundsOverrides(visibleBounds);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetForceIsFullScreen(_In_ bool forceIsFullScreen)
{
    m_pDXamlCoreNoRef->SetForceIsFullScreen(forceIsFullScreen);
}

IFACEMETHODIMP DxamlCoreTestHooks::CancelAllConnectedAnimationsAndResetDefaults()
{
    return DXamlCore::GetCurrent()->GetHandle()->CancelAllConnectedAnimationsAndResetDefaults();
}

IFACEMETHODIMP
DxamlCoreTestHooks::GetPopupOverlayElement(
    _In_ xaml_primitives::IPopup* popup,
    _Outptr_ xaml::IFrameworkElement** overlayElement
    )
{
    return static_cast<Popup*>(popup)->get_OverlayElement(overlayElement);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::DeletePlatformFamilyCache()
{
    XboxUtility::DeleteIsOnXboxCache();
    DesktopUtility::DeleteIsOnDesktopCache();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::DeleteResourceDictionaryCaches()
{
    CCalendarViewBaseItemChrome::ClearIsRoundedCalendarViewBaseItemChromeEnabledCache();
    CHyperlink::ClearUnderlineVisibleResourceDirectiveCache();
    CListViewBaseItemChrome::ClearIsRoundedListViewBaseItemChromeEnabledCache();
}

IFACEMETHODIMP DxamlCoreTestHooks::EnableKeyboardInputEvent()
{
    CCoreServices* coreServices = m_pDXamlCoreNoRef->GetHandle();
    if (coreServices != nullptr)
    {
        IFC_RETURN(coreServices->EnableKeyboardInputEvent());
    }

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::DisableKeyboardInputEvent()
{
    CCoreServices* coreServices = m_pDXamlCoreNoRef->GetHandle();
    if (coreServices != nullptr)
    {
        coreServices->DisableKeyboardInputEvent();
    }
}

IFACEMETHODIMP_(bool) DxamlCoreTestHooks::CanFireKeyboardInputEvent()
{
    CCoreServices* coreServices = m_pDXamlCoreNoRef->GetHandle();
    if (coreServices != nullptr)
    {
        return coreServices->CanFireKeyboardInputEvent();
    }

    return false;
}

IFACEMETHODIMP DxamlCoreTestHooks::EnablePointerInputEvent()
{
    CCoreServices* coreServices = m_pDXamlCoreNoRef->GetHandle();
    if (coreServices != nullptr)
    {
        IFC_RETURN(coreServices->EnablePointerInputEvent());
    }

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::DisablePointerInputEvent()
{
    CCoreServices* coreServices = m_pDXamlCoreNoRef->GetHandle();
    if (coreServices != nullptr)
    {
        coreServices->DisablePointerInputEvent();
    }
}

IFACEMETHODIMP_(bool) DxamlCoreTestHooks::CanFirePointerInputEvent()
{
    CCoreServices* coreServices = m_pDXamlCoreNoRef->GetHandle();
    if (coreServices != nullptr)
    {
        return coreServices->CanFirePointerInputEvent();
    }

    return false;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetLastLayoutExceptionElement(_In_ xaml::IUIElement* element)
{
    ctl::ComPtr<DependencyObject> elementAsDO;
    CUIElement* uie = nullptr;
    if (element != nullptr)
    {
        IFCFAILFAST(ctl::do_query_interface(elementAsDO, element));
        uie = static_cast<CUIElement*>(elementAsDO->GetHandle());
    }
    DXamlCore::GetCurrent()->GetHandle()->SetLastLayoutExceptionElement(uie);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetMockUIAClientsListening()
{
    DXamlCore::GetCurrent()->SetMockUIAClientsListening(true);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ClearMockUIAClientsListening()
{
    DXamlCore::GetCurrent()->SetMockUIAClientsListening(true);
}

IFACEMETHODIMP DxamlCoreTestHooks::GetLightsTargetingElement(
    _In_ xaml::IUIElement* target,
    _In_ wfc::IVector<xaml_media::XamlLight*>* lights)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    DCompTreeHost* dcompTreeHost = coreServices->GetDCompTreeHost();

    CUIElement* targetElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(target)->GetHandle());

    const auto& map = dcompTreeHost->GetXamlLightTargetMap().m_map;
    const auto& pair = map.find(targetElement);
    if (pair != map.end())
    {
        for (const auto& light : pair->second)
        {
            DirectUI::XamlLight* dxamlLight = static_cast<DirectUI::XamlLight*>(light->GetDXamlPeer());
            lights->Append(static_cast<xaml_media::IXamlLight*>(dxamlLight));
        }
    }

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::GetElementsTargetedByLight(
    _In_ xaml_media::IXamlLight* light,
    _In_ wfc::IVector<xaml::UIElement*>* targets)
{
    CXamlLight* xamlLight = static_cast<CXamlLight*>(static_cast<DirectUI::XamlLight*>(light)->GetHandle());

    for (const auto& elementPair : xamlLight->m_targetedVisuals)
    {
        DirectUI::UIElement* target = static_cast<DirectUI::UIElement*>(elementPair.first->GetDXamlPeer());
        targets->Append(static_cast<xaml::IUIElement*>(target));
    }

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::GetCountOfVisualsTargeted(
    _In_ xaml_media::IXamlLight* light,
    _In_ xaml::IUIElement* element,
    _Out_ int* count)
{
    CXamlLight* xamlLight = static_cast<CXamlLight*>(static_cast<DirectUI::XamlLight*>(light)->GetHandle());
    *count = -1;

    for (const auto& elementPair : xamlLight->m_targetedVisuals)
    {
        DirectUI::UIElement* target = static_cast<DirectUI::UIElement*>(elementPair.first->GetDXamlPeer());
        if (target == element)
        {
            *count = elementPair.second.size();
            break;
        }
    }

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::GetRealCompositionSurface(
    _In_ xaml_media::ILoadedImageSurface *loadedImageSurface,
    _Outptr_ WUComp::ICompositionSurface **realCompositionSurface)
{
    wrl::ComPtr<WUComp::ICompositionSurfaceFacade> partner;
    IFC_RETURN(loadedImageSurface->QueryInterface(IID_PPV_ARGS(&partner)));

    IFC_RETURN(partner->GetRealSurface(realCompositionSurface));

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetGenericXamlFilePathForMUX(_In_ HSTRING filePath)
{
    DXamlCore::GetCurrent()->SetGenericXamlFilePathForMUX(XSTRING_PTR_EPHEMERAL_FROM_HSTRING(filePath));
}

IFACEMETHODIMP DxamlCoreTestHooks::SetHdrOutputOverride(bool value)
{
    m_pDXamlCoreNoRef->SetHdrOutputOverride(value);
    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetWantsRenderingEvent(_Out_ BOOLEAN* wantsRenderingEvent)
{
    *wantsRenderingEvent = m_pDXamlCoreNoRef->GetWantsRenderingEvent();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetWantsCompositionTargetRenderedEvent(_Out_ BOOLEAN* wantsCompositionTargetRenderedEvent)
{
    *wantsCompositionTargetRenderedEvent = m_pDXamlCoreNoRef->GetWantsCompositionTargetRenderedEvent();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetThreadingAssertOverride(_In_ bool value)
{
    DXamlCore::GetCurrent()->SetThreadingAssertOverride(value);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetCanTickWithNoContent(bool canTickWithNoContent)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());

    coreServices->SetCanTickWithNoContent(canTickWithNoContent);
}

IFACEMETHODIMP DxamlCoreTestHooks::AddTestLTE(
    _In_ xaml::IUIElement* lteTarget,
    _In_ xaml::IUIElement* lteParent,
    bool parentIsRootVisual,
    bool parentIsPopupRoot,
    bool isAbsolutelyPositioned,
    _Outptr_ xaml::IUIElement** lte)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());

    CUIElement* lteTargetCUIE = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(lteTarget)->GetHandle());
    CUIElement* lteParentCUIE = nullptr;
    if (lteParent != nullptr)
    {
        lteParentCUIE = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(lteParent)->GetHandle());
    }

    xref_ptr<CLayoutTransitionElement> coreLTE = coreServices->AddTestLTE(lteTargetCUIE, lteParentCUIE, parentIsRootVisual, parentIsPopupRoot, isAbsolutelyPositioned);
    ctl::ComPtr<DependencyObject> ltePeer;
    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(coreLTE, &ltePeer));
    *lte = static_cast<DirectUI::UIElement*>(ltePeer.Detach());

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::RemoveTestLTE(_In_ xaml::IUIElement* lte)
{
    CUIElement* coreLTE = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(lte)->GetHandle());

    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    coreServices->RemoveTestLTE(coreLTE);
    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::ClearTestLTEs()
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    coreServices->ClearTestLTEs();
    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::TestGetActualToolTip(
    _In_ xaml::IUIElement* element,
    _Outptr_ xaml_controls::IToolTip** ppValue)
{
    ctl::ComPtr<xaml::IDependencyObject> elementAsDO;
    IFC_RETURN(ctl::do_query_interface(elementAsDO, element));
    ToolTipService::GetActualToolTipObjectStatic(elementAsDO.Get(), ppValue);
    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAnimatedTranslation(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* translation)
{
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    *translation = SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTranslation>::Get(coreElement);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAnimatedRotation(_In_ xaml::IUIElement* element, _Out_ float* rotation)
{
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    *rotation = static_cast<float>(SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotation>::Get(coreElement));
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAnimatedScale(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* scale)
{
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    *scale = SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedScale>::Get(coreElement);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAnimatedTransformMatrix(_In_ xaml::IUIElement* element, _Out_ wfn::Matrix4x4* transformMatrix)
{
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    *transformMatrix = SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedTransformMatrix>::Get(coreElement);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAnimatedRotationAxis(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* rotationAxis)
{
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    *rotationAxis = SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedRotationAxis>::Get(coreElement);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAnimatedCenterPoint(_In_ xaml::IUIElement* element, _Out_ wfn::Vector3* centerPoint)
{
    CUIElement* coreElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    *centerPoint = SimpleProperty::Property::id<KnownPropertyIndex::UIElement_AnimatedCenterPoint>::Get(coreElement);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ScheduleWaitForAnimatedFacadePropertyChanges(int count)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    coreServices->ScheduleWaitForAnimatedFacadePropertyChanges(count);
}

IFACEMETHODIMP_(BOOLEAN) DxamlCoreTestHooks::IsTrackingEffectiveVisibility(_In_ xaml::IUIElement* element)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    DCompTreeHost* dcompTreeHost = coreServices->GetDCompTreeHost();

    CUIElement* uiElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());

    return dcompTreeHost->IsTrackingEffectiveVisibility(uiElement);
}

IFACEMETHODIMP_(BOOLEAN) DxamlCoreTestHooks::IsKeepingVisible(_In_ xaml::IUIElement* element)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    DCompTreeHost* dcompTreeHost = coreServices->GetDCompTreeHost();

    CUIElement* uiElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());

    return dcompTreeHost->IsKeepVisible(uiElement);
}

IFACEMETHODIMP DxamlCoreTestHooks::RequestKeepAlive(_In_ xaml::IUIElement* element)
{
    CUIElement* uiElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    uiElement->RequestKeepAlive();
    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::ReleaseKeepAlive(_In_ xaml::IUIElement* element)
{
    CUIElement* uiElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    uiElement->ReleaseKeepAlive();
    return S_OK;
}

IFACEMETHODIMP_(bool) DxamlCoreTestHooks::IsStoryboardActive(_In_ xaml_animation::IStoryboard* storyboard)
{
    ctl::ComPtr<DirectUI::DependencyObject> storyboardDO;
    IFCFAILFAST(storyboard->QueryInterface(IID_PPV_ARGS(&storyboardDO)));

    auto coreSB = checked_cast<CStoryboard>(storyboardDO->GetHandle());
    return coreSB->IsInActiveState();
}

IFACEMETHODIMP DxamlCoreTestHooks::GetElementInputWindow(_In_ xaml::IUIElement* element, _Out_ HWND* inputHwnd)
{
    CUIElement* uiElement = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(element)->GetHandle());
    *inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(uiElement->GetElementIslandInputSite().Get());

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetSuspendOffThreadDecoding(bool isOffThreadDecodingSuspended)
{
    AsyncImageDecoder::SetSuspendOffThreadDecoding(isOffThreadDecodingSuspended);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetSuspendSurfaceUpdates(bool isSuspended)
{
    CCoreServices* core = DXamlCore::GetCurrent()->GetHandle();

    HWTextureManager* hwTextureManager = core->GetHWTextureManagerNoRef();
    if (hwTextureManager)
    {
        hwTextureManager->SetSuspendSurfaceUpdates(isSuspended);
    }
}

IFACEMETHODIMP DxamlCoreTestHooks::GetUIAWindow(
    _In_ xaml::IDependencyObject* pElement,
    _In_ HWND hwnd,
    _Outptr_ IInspectable** uiaWindowNoRef)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CJupiterControl* control = dxamlCore->GetControl();
    auto cDo = static_cast<DirectUI::DependencyObject*>(pElement)->GetHandle();

    // GetUIAWindow doesn't actually add a ref to the out void** param
    return control->GetUIAWindow(cDo, hwnd, false /* onlyGet */, reinterpret_cast<CUIAWindow**>(uiaWindowNoRef));
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::RestoreDefaultFlipViewScrollWheelDelay()
{
    DirectUI::FlipView::RestoreDefaultScrollWheelDelay();
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetFlipViewScrollWheelDelay(int scrollWheelDelayMS)
{
    DirectUI::FlipView::SetScrollWheelDelay(scrollWheelDelayMS);
}

IFACEMETHODIMP DxamlCoreTestHooks::ApplyElevationEffect(
    _In_ xaml::IUIElement* element,
    UINT depth)
{
    return DirectUI::ApplyElevationEffect(element, depth);
}

IFACEMETHODIMP DxamlCoreTestHooks::SetApplicationLanguageOverride(_In_ HSTRING languageName)
{
    Microsoft::WRL::ComPtr<wg::IApplicationLanguagesStatics> applicationLanguagesStatics;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(),
        &applicationLanguagesStatics));

    IFC_RETURN(applicationLanguagesStatics->put_PrimaryLanguageOverride(languageName));

    FreeResourceLibraries();
    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::ClearApplicationLanguageOverride()
{
    Microsoft::WRL::ComPtr<wg::IApplicationLanguagesStatics> applicationLanguagesStatics;

    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(),
        &applicationLanguagesStatics));

    IFC_RETURN(applicationLanguagesStatics->put_PrimaryLanguageOverride(nullptr));

    FreeResourceLibraries();
    return S_OK;
}

void DxamlCoreTestHooks::FreeResourceLibraries()
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    auto browserHost = coreServices->GetBrowserHost();

    if (browserHost != nullptr)
    {
        browserHost->FreeResourceLibraries();
    }

    auto freeResourceLibrary = [](const WCHAR* moduleFileName, const char* freeResourceLibraryProcName)
    {
        wil::unique_hmodule resourceLibraryModule(LoadLibraryExWAbs(moduleFileName, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH));

        if (resourceLibraryModule)
        {
            typedef void (*FreeResourceLibraryFuncPtr)();
            if (auto pfnFreeResourceLibrary = reinterpret_cast<FreeResourceLibraryFuncPtr>(GetProcAddress(resourceLibraryModule.get(), freeResourceLibraryProcName)))
            {
                pfnFreeResourceLibrary();
            }
        }
    };

    freeResourceLibrary(L"Microsoft.UI.Xaml.Phone.dll", "XamlTestHookFreePhoneResourceLibrary");
    freeResourceLibrary(L"Microsoft.UI.Xaml.Controls.dll", "XamlTestHookFreeControlsResourceLibrary");
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetCaretBrowsingModeGlobal(bool caretBrowsingModeEnable, bool caretBrowsingDialogNotPopAgain)
{
    SetCaretBrowsingModeEnable(caretBrowsingModeEnable);
    SetCaretBrowsingDialogNotPopAgain(caretBrowsingDialogNotPopAgain);
    SetCaretBrowsingF7Disabled(false); // for test purposes, we can just always reset this to false (to reenable F7)
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::CloseAllPopupsForTreeReset()
{
    CCoreServices* coreServices = m_pDXamlCoreNoRef->GetHandle();
    ContentRootCoordinator* contentRootCoordinator = coreServices->GetContentRootCoordinator();

    for (const xref_ptr<CContentRoot>& contentRoot: contentRootCoordinator->GetContentRoots())
    {
        // Closing a popup can cause a reentrancy problem with UIAutomation peers. Closing a popup will remove
        // it from the popup root, which can unpeg and release its managed peer, which can release a tree of
        // DXaml objects, which calls CAutomationPeer::Deinit, which calls back into GetFocusedAutomationPeer
        // to return the focused popup's DXaml peer. But the popup just released its DXaml peer as part of Close,
        // so we hit an assert in CDependencyObject::OnManagedPeerCreated that the popup shouldn't have ever had
        // a peer.
        //
        // This all takes place when the popup releases its DXaml peer in CPopup::Close, before it updates the
        // focused element. Since we're resetting the tree anyway, just clear focus before closing popups and
        // avoid this reentrancy.
        contentRoot->GetFocusManagerNoRef()->ClearFocus();
        contentRoot->GetVisualTreeNoRef()->CloseAllPopupsForTreeReset();

        // Also close all popups that are open in islands.
        if (coreServices->HasXamlIslandRoots())
        {
            if (CXamlIslandRootCollection* xamlIslandRootCollection = contentRoot->GetVisualTreeNoRef()->GetXamlIslandRootCollection())
            {
                if (CDOCollection* collection = xamlIslandRootCollection->GetChildren())
                {
                    for (CDependencyObject* child : *collection)
                    {
                        CPopupRoot* islandPopupRoot = do_pointer_cast<CXamlIslandRoot>(child)->GetPopupRootNoRef();
                        if (islandPopupRoot != nullptr)
                        {
                            islandPopupRoot->CloseAllPopupsForTreeReset();
                        }
                    }
                }
            }
        }
    }
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAllXamlRoots(_Outptr_ wfc::IVectorView<xaml::XamlRoot*>** xamlRoots)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();

    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());

    ContentRootCoordinator* contentRootCoordinator = coreServices->GetContentRootCoordinator();
    const std::vector<xref_ptr<CContentRoot>>& contentRootVector = contentRootCoordinator->GetContentRoots();

    // For tests running in WPF hosting mode, there will be a single XamlRoot that corresponds to the dummy content of
    // the window, outside the island. We don't want to return this XamlRoot, because it will pollute the VisualTreeDumper
    // output. Check for this XamlRoot and ignore it.
    VisualTree* dummyVisualTree = nullptr;

    int dummyRootVisualCount = 0;
    int xamlIslandRootCount = 0;

    for (const auto& coreContentRoot : contentRootVector)
    {
        VisualTree* visualTree = coreContentRoot->GetVisualTreeNoRef();
        CUIElement* rootElement = visualTree->GetRootElementNoRef();

        if (rootElement->OfTypeByIndex<KnownTypeIndex::RootVisual>())
        {
            // The dummy XamlRoot has this visual tree:
            //
            //  CRootVisual
            //    [island roots, popup root, etc]
            //    CRootScrollViewer
            //      CScrollContentPresenter
            //        CBorder
            //          CGrid
            //
            // Specifically, the VisualTree has a RootScrollViewer, and the public root is a Grid with no children.
            CUIElement* publicRoot = visualTree->GetPublicRootVisual();
            if (visualTree->GetRootScrollViewer() != nullptr
                && publicRoot != nullptr
                && publicRoot->OfTypeByIndex<KnownTypeIndex::Grid>()
                && publicRoot->GetFirstChildNoAddRef() == nullptr)
            {
                dummyRootVisualCount++;
                dummyVisualTree = visualTree;
            }
        }
        else if (rootElement->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>())
        {
            xamlIslandRootCount++;
        }
    }

    const bool skipDummyVisualTree = (dummyRootVisualCount == 1 && xamlIslandRootCount > 0);

    ctl::ComPtr<TrackerCollection<xaml::XamlRoot*>> xamlRootCollection;
    IFCFAILFAST(ctl::make(&xamlRootCollection));

    for (const auto& coreContentRoot : contentRootVector)
    {
        VisualTree* visualTree = coreContentRoot->GetVisualTreeNoRef();

        if (!skipDummyVisualTree || visualTree != dummyVisualTree)
        {
            IInspectable* xamlRootII = visualTree->GetOrCreateXamlRootNoRef();

            ctl::ComPtr<xaml::IXamlRoot> xamlRoot;
            IFCFAILFAST(xamlRootII->QueryInterface(IID_PPV_ARGS(xamlRoot.ReleaseAndGetAddressOf())));
            xamlRootCollection->Append(xamlRoot.Get());
        }
    }

    IFCFAILFAST(xamlRootCollection->GetView(xamlRoots));
}

IFACEMETHODIMP DxamlCoreTestHooks::ForceShadowsPolicy(_In_ BOOLEAN forceShadowsOn)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    DCompTreeHost* dcompTreeHost = coreServices->GetDCompTreeHost();
    ProjectedShadowManager* projectedShadowManager = dcompTreeHost->GetProjectedShadowManager();
    projectedShadowManager->ForceShadowsPolicy(coreServices, forceShadowsOn);

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::ClearShadowPolicyOverrides()
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    DCompTreeHost* dcompTreeHost = coreServices->GetDCompTreeHost();
    ProjectedShadowManager* projectedShadowManager = dcompTreeHost->GetProjectedShadowManager();
    projectedShadowManager->ClearShadowPolicyOverrides();

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::SetXamlVisibilityOverride(_In_ BOOLEAN isVisible)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    coreServices->ForceXamlInvisible_TestHook(isVisible);

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::SetBrushForXamlRoot(
    _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
    _In_ IInspectable* xamlRoot,
    _In_ IInspectable* cbrush)
{
    ctl::ComPtr<xaml_media::IXamlCompositionBrushBase> xcbbPtr;
    IFC_RETURN(ctl::do_query_interface(xcbbPtr, xcbb));
    ctl::ComPtr<DirectUI::XamlCompositionBrushBase> brushBase = xcbbPtr.Cast<DirectUI::XamlCompositionBrushBase>();

    ctl::ComPtr<WUComp::ICompositionBrush> compBrush;
    IFC_RETURN(ctl::do_query_interface(compBrush, cbrush));
    brushBase->SetBrushForXamlRootImpl(xamlRoot, compBrush.Detach());

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::GetBrushForXamlRoot(
    _In_ xaml_media::IXamlCompositionBrushBase* xcbb,
    _In_ IInspectable* xamlRoot,
    _Outptr_ IInspectable** cbrush)
{
    ctl::ComPtr<xaml_media::IXamlCompositionBrushBase> xcbbPtr;
    IFC_RETURN(ctl::do_query_interface(xcbbPtr, xcbb));
    ctl::ComPtr<DirectUI::XamlCompositionBrushBase> brushBase = xcbbPtr.Cast<DirectUI::XamlCompositionBrushBase>();

    ctl::ComPtr<WUComp::ICompositionBrush> compBrush;
    IFC_RETURN(ctl::do_query_interface(compBrush, *cbrush));

    brushBase->GetBrushForXamlRootImpl(xamlRoot, &compBrush);
    *cbrush = compBrush.Detach();

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::StopAllInteractions()
{
    auto dxamlCore = DXamlCore::GetCurrent();
    auto coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    coreServices->GetInputServices()->GetInteractionManager().StopAllInteractions();
}

IFACEMETHODIMP DxamlCoreTestHooks::GetImageSourceMaxSize(
    _In_ xaml_media::IImageSource* imageSource,
    _Out_ wf::Size* maxSize)
{
    CImageSource* imageSourceCoreNoRef = static_cast<CImageSource*>(static_cast<DirectUI::ImageSource*>(imageSource)->GetHandle());
    *maxSize = imageSourceCoreNoRef->GetMaxRootSize();

    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::GetAllRootVisualsNoRef(_In_ wfc::IVectorView<IInspectable*>** visualsNoRef)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    ContentRootCoordinator* contentRootCoordinator = coreServices->GetContentRootCoordinator();
    const std::vector<xref_ptr<CContentRoot>>& contentRoots = contentRootCoordinator->GetContentRoots();

    ctl::ComPtr<TrackerCollection<IInspectable*>> rootVisualsNoRef;
    IFCFAILFAST(ctl::make(&rootVisualsNoRef));

    for (const auto& contentRoot : contentRoots)
    {
        wrl::ComPtr<ixp::IVisual> visual;

        if (contentRoot->GetType() == CContentRoot::Type::CoreWindow)
        {
            // The bridge for the CoreWindow lives in the DCompTreeHost.
            DCompTreeHost* dcompTreeHost = coreServices->GetDCompTreeHost();
            visual = dcompTreeHost->GetInprocIslandRoot();
        }
        else if (contentRoot->GetType() == CContentRoot::Type::XamlIslandRoot)
        {
            CXamlIslandRoot* island = contentRoot->GetXamlIslandRootNoRef();
            visual = island->GetRootVisual();
        }
        else
        {
            // There are no other types of content root
            ASSERT(false);
        }

        // In island scenarios, we'll have a dummy CoreWindow ContentRoot. That ContentRoot will not have a root,
        // which works out because there's nothing under that ContentRoot to dump anyway.
        if (visual != nullptr)
        {
            rootVisualsNoRef->Append(visual.Get());
        }

        // If there are any popups then add them as well.
        VisualTree* visualTree = contentRoot->GetVisualTreeNoRef();
        if (visualTree != nullptr)
        {
            CPopupRoot* pPopupRoot = visualTree->GetPopupRoot();
            if (pPopupRoot)
            {
                CPopup** ppOpenedPopups = nullptr;
                XINT32 countOpenedPopups = 0;

                IFCFAILFAST(pPopupRoot->GetOpenPopups(&countOpenedPopups, &ppOpenedPopups));
                if (countOpenedPopups > 0)
                {
                    for (XINT32 i = 0; i < countOpenedPopups; i++)
                    {
                        CPopup* pPopup = ppOpenedPopups[i];
                        if (pPopup)
                        {
                            auto desktopPopupSiteBridge = pPopup->GetDesktopPopupSiteBridgeNoRef();
                            auto popupWindowSiteBridge = pPopup->GetPopupWindowBridgeNoRef();
                            if (desktopPopupSiteBridge != nullptr || popupWindowSiteBridge != nullptr)
                            {
                                // For popup root visuals, we also want to capture the offset of the
                                // top level HWND hosting the popup to its owner HWND. The
                                // _XAML_DEBUG_TAG_HwndLeft and _XAML_DEBUG_TAG_HwndTop scalar
                                // properties on the root visual are written in
                                // DxamlCoreTestHooks::GetAllRootVisualsNoRef and read below so that
                                // we write it to the output XML.


                                // Get the root visual of the popup.
                                ctl::ComPtr<WUComp::IVisual> popupRootVisual = pPopup->GetWindowRootVisual_TestHook();

                                // Store the MoveAndResize rect on the IVisual property bag.
                                ctl::ComPtr<WUComp::ICompositionObject> compositionObject;
                                IFCFAILFAST(popupRootVisual.As(&compositionObject));
                                ctl::ComPtr<WUComp::ICompositionPropertySet> compositionPropertySet;
                                IFCFAILFAST(compositionObject->get_Properties(&compositionPropertySet));

                                wgr::RectInt32 moveAndResizeRect = pPopup->GetWindowedPopupMoveAndResizeRect();
                                IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(L"_XAML_DEBUG_TAG_HwndLeft").Get(), static_cast<float>(moveAndResizeRect.X)));
                                IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(L"_XAML_DEBUG_TAG_HwndTop").Get(), static_cast<float>(moveAndResizeRect.Y)));
                                IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(L"_XAML_DEBUG_TAG_HwndWidth").Get(), static_cast<float>(moveAndResizeRect.Width)));
                                IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(L"_XAML_DEBUG_TAG_HwndHeight").Get(), static_cast<float>(moveAndResizeRect.Height)));

                                // Add the popup root visual to the returned collection.
                                rootVisualsNoRef->Append(popupRootVisual.Get());
                            }

                            ReleaseInterface(pPopup);
                        }
                    }
                }

                if (ppOpenedPopups != nullptr)
                {
                    delete[] ppOpenedPopups;
                }
            }
        }
    }

    IFCFAILFAST(rootVisualsNoRef->GetView(visualsNoRef));
}

IFACEMETHODIMP DxamlCoreTestHooks::GetIslandAndBridge(
    _In_ xaml::IFrameworkElement* element,
    bool followPopups,
    _Outptr_opt_ IInspectable** island,
    _Outptr_opt_ IInspectable** bridge)
{
    CDependencyObject* elementAsDO = static_cast<CDependencyObject*>(static_cast<FrameworkElement*>(element)->GetHandle());

    if (!island && !bridge)
    {
        IFC_RETURN(E_INVALIDARG);   // The caller must ask for something.
    }

    if (island)
    {
        *island = nullptr;
    }

    if (bridge)
    {
        *bridge = nullptr;
    }

    while (elementAsDO)
    {
        if (elementAsDO->OfTypeByIndex<KnownTypeIndex::XamlIslandRoot>())
        {
            CXamlIslandRoot* xamlIslandRoot = static_cast<CXamlIslandRoot*>(elementAsDO);

            if (island)
            {
                const auto& contentIsland = xamlIslandRoot->GetContentRootNoRef()->GetCompositionContentNoRef();
                if (contentIsland)
                {
                    IFCFAILFAST(contentIsland->QueryInterface(IID_PPV_ARGS(island)));
                }
            }

            if (bridge)
            {
                IFC_RETURN(xamlIslandRoot->GetDesktopContentBridgeNoRef()->QueryInterface(IID_PPV_ARGS(bridge)));
            }

            return S_OK;
        }
        else if (followPopups
            && elementAsDO->OfTypeByIndex<KnownTypeIndex::Popup>()
            && static_cast<CPopup*>(elementAsDO)->IsWindowed())
        {
            CPopup* popup = static_cast<CPopup*>(elementAsDO);

            if (island)
            {
                const auto& contentIsland = popup->GetContentIslandNoRef();
                if (contentIsland)
                {
                    IFCFAILFAST(contentIsland->QueryInterface(IID_PPV_ARGS(island)));
                }
            }

            if (bridge)
            {
                if (popup->GetDesktopPopupSiteBridgeNoRef() != nullptr)
                {
                    IFC_RETURN(popup->GetDesktopPopupSiteBridgeNoRef()->QueryInterface(IID_PPV_ARGS(bridge)));
                }
                else
                {
                    IFC_RETURN(popup->GetPopupWindowBridgeNoRef()->QueryInterface(IID_PPV_ARGS(bridge)));
                }
            }

            return S_OK;
        }

        if (followPopups)
        {
            elementAsDO = elementAsDO->GetParentFollowPopups();
        }
        else
        {
            // There will be cases where we explicitly don't want to go up to the Popup if we started in the Popup's
            // subtree. This is the case when computing screen coordinates. We'll call TransformToVisual(nullptr) on the
            // element, which transforms it up to the CXamlIslandRoot. We're missing the part of the transform from the
            // CXamlIslandRoot up to the screen. We always want to walk up to the CXamlIslandRoot here, rather than
            // follow the link to the windowed popup, otherwise we'll double-count the transform between the windowed
            // popup and the CXamlIslandRoot.
            elementAsDO = elementAsDO->GetParentInternal();
        }
    }

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::DetachMemoryManagerEvents()
{
    CCoreServices* core = DXamlCore::GetCurrent()->GetHandle();
    core->DetachMemoryManagerEvents();
    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetTransparentBackground(bool makeTransparent)
{
    auto dxamlCore = DXamlCore::GetCurrent();
    dxamlCore->SetTransparentBackground(makeTransparent);
}

ctl::ComPtr<Window> DxamlCoreTestHooks::GetTargetWindow(_In_opt_ xaml::IWindow* windowParam)
{
    ctl::ComPtr<xaml::IWindow> ixamlWindow{windowParam};
    ctl::ComPtr<DirectUI::Window> window;
    if (ixamlWindow)
    {
        // cannot cast directly from xaml::IWindow to DirectUI::Window, so this extra step to DependencyObject is necessary
        ctl::ComPtr<DirectUI::DependencyObject> dependencyObject;
        IFCFAILFAST(ixamlWindow.As(&dependencyObject));
        IFCFAILFAST(dependencyObject.As(&window));
    }
    else
    {
        // assume the first window from the list of windows
        // TODO: remove double-vector creation- see note in DXamlCore::GetAllWindows as well as task below
        // https://microsoft.visualstudio.com/OS/_workitems/edit/37010942
        std::vector<DirectUI::Window*> windowVector;
        IFCFAILFAST(DXamlCore::GetCurrent()->GetAllWindows(windowVector));

        if (!windowVector.empty())
        {
            window = windowVector[0];
        }
    }

    return window;
}

// Simulate a device lost error on the next off-thread image decode.
IFACEMETHODIMP DxamlCoreTestHooks::SimulateDeviceLostOnMetadataParse()
{
    EncodedImageData::s_testHook_ForceDeviceLostOnMetadataParse = true;
    return S_OK;
}

// Simulate a device lost error on the next off-thread image decode.
IFACEMETHODIMP DxamlCoreTestHooks::SimulateDeviceLostOnCreatingSvgDecoder()
{
    SvgImageDecoder::s_testHook_ForceDeviceLostOnCreatingSvgDecoder = true;
    return S_OK;
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::SetForceDebugSettingsTracingEvents(bool value)
{
    DebugOutput::SetForceDebugSettingsTracingEvents(value);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::ThrottleImageTaskDispatcher(bool enableThrottling, unsigned int numberOfTasksAllowedToDispatch)
{
    CCoreServices* core = DXamlCore::GetCurrent()->GetHandle();

    // Side effect - this ensures the imaging task dispatcher
    core->GetImageFactory();

    core->m_imageTaskDispatcher->ThrottleExecution_TestHook(enableThrottling, numberOfTasksAllowedToDispatch);
}

IFACEMETHODIMP_(void) DxamlCoreTestHooks::RequestExecuteImageTaskDispatcher()
{
    CCoreServices* core = DXamlCore::GetCurrent()->GetHandle();

    // Side effect - this ensures the imaging task dispatcher
    core->GetImageFactory();

    core->m_imageTaskDispatcher->RequestExecution_TestHook();
}

IFACEMETHODIMP DxamlCoreTestHooks::GetElementRenderedVisuals(
    _In_ xaml::IUIElement* iUIElement,
    _In_ wfc::IVector<IInspectable*>* visuals)
{
    CUIElement* element = static_cast<CUIElement*>(static_cast<DirectUI::UIElement*>(iUIElement)->GetHandle());

    // Note: Platform::Collections::Vector::Append automatically does an AddRef on the object passed in.

    if (element->OfTypeByIndex(KnownTypeIndex::Popup) &&
        static_cast<CPopup*>(element)->IsWindowed() &&
        static_cast<CPopup*>(element)->IsOpen())
    {
        visuals->Append(static_cast<CPopup*>(element)->GetWindowRootVisual_TestHook());
    }
    else if (element->GetCompositionPeer())
    {
        visuals->Append(static_cast<HWCompTreeNodeWinRT*>(element->GetCompositionPeer())->GetWUCVisual().get());
    }
    else if (element->IsInPCScene())
    {
        PCRenderDataList *pPreChildrenRenderData = nullptr;
        IFC_RETURN(element->GetPCPreChildrenRenderDataNoRef(&pPreChildrenRenderData));
        for (auto it = pPreChildrenRenderData->begin(); it != pPreChildrenRenderData->end(); ++it)
        {
            visuals->Append(*it);
        }

        PCRenderDataList *pPostChildrenRenderData = nullptr;
        IFC_RETURN(element->GetPCPostChildrenRenderDataNoRef(&pPostChildrenRenderData));
        for (auto it = pPostChildrenRenderData->begin(); it != pPostChildrenRenderData->end(); ++it)
        {
            visuals->Append(*it);
        }
    }

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::GetErrorHandlingTestHooks(_Outptr_opt_ IXamlErrorTestHooks** errorTestHooks)
{
    return CreateErrorHandlingTestHooks(errorTestHooks);
}

IFACEMETHODIMP DxamlCoreTestHooks::GetAllContentIslands(_In_ xaml_hosting::IDesktopWindowXamlSource* desktopWindowXamlSource, _In_ wfc::IVector<IInspectable*>* contentIslands)
{
    ctl::ComPtr<DirectUI::DesktopWindowXamlSource> dwxs;
    IFCFAILFAST(desktopWindowXamlSource->QueryInterface(IID_PPV_ARGS(&dwxs)));
    ctl::ComPtr<IXamlIslandRoot> xamlIslandRoot = dwxs->GetXamlIslandRootNoRef();
    CXamlIslandRoot* xamlIslandRootNoRef = static_cast<CXamlIslandRoot*>(xamlIslandRoot.Cast<XamlIslandRoot>()->GetHandle());

    ixp::IContentIsland* islandNoRef = xamlIslandRootNoRef->GetContentIsland();
    wrl::ComPtr<IInspectable> islandInspectable;
    IFCFAILFAST(islandNoRef->QueryInterface(IID_PPV_ARGS(&islandInspectable)));
    // Note: Platform::Collections::Vector::Append automatically does an AddRef on the object passed in.
    contentIslands->Append(islandInspectable.Get());

    CContentRoot* islandContentRootNoRef = xamlIslandRootNoRef->GetContentRootNoRef();
    VisualTree* visualTreeNoRef = islandContentRootNoRef->GetVisualTreeNoRef();
    CPopupRoot* popupRoot = visualTreeNoRef->GetPopupRoot();
    if (popupRoot)
    {
        // Note: CPopupRoot::m_pOpenPopups is a CXcpList, and new entries get added at the head, so this is in reverse
        // opened order - the most recently opened popup is first.
        std::vector<CPopup*> openPopups = popupRoot->GetOpenPopupList(true /* includeUnloadingPopups */);

        for (CPopup* popup : openPopups)
        {
            ixp::IContentIsland* contentIslandNoRef = popup->GetContentIslandNoRef();
            if (contentIslandNoRef)
            {
                wrl::ComPtr<IInspectable> popupContentIslandInspectable;
                IFCFAILFAST(contentIslandNoRef->QueryInterface(IID_PPV_ARGS(&popupContentIslandInspectable)));
                // Note: Platform::Collections::Vector::Append automatically does an AddRef on the object passed in.
                contentIslands->Append(popupContentIslandInspectable.Get());
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP DxamlCoreTestHooks::GetElementsRenderedCount(_Out_ int* elementsRendered)
{
    DXamlCore* dxamlCore = DXamlCore::GetCurrent();
    CCoreServices* coreServices = static_cast<CCoreServices*>(dxamlCore->GetHandle());
    HWWalk* hwWalk = coreServices->GetHWWalk();
    *elementsRendered = hwWalk->GetElementsRenderedCount();
    return S_OK;
}
