// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Object responsible for managing DComp resources and changes to DComp tree or visuals.
//      It maintains a map of DComp visuals and their owners. The map is always accessed on
//      render thread, except the device loss case, where we tear down the DComp
//      resouces on UI thread.
//
//      In addition, UI and render thread submit DComp commands to this object. It maintains
//      3 lists (UI thread, waiting for compositor and ready for compositor) for DComp commands.
//      It takes into account expected frame latency before commiting DComp commands, thus
//      attempting to minimize synchronization problems b/w our compositor presented swap chain scene
//      and DComp visual tree changes.

#pragma once

#include <windows.h>
#include <windows.ui.composition.interop.h>
#include <DependencyObjectDCompRegistry.h>
#include <EnumDefs.h>
#include <EnumDefs.g.h>
#include <NamespaceAliases.h>
#include <vector_map.h>
#include <DCompPropertyChangedListener.h>
#include <HandOffVisualData.h>
#include <WUCBrushManager.h>
#include "SharedTransitionAnimations.h"
#include <ImplicitAnimations.h>
#include <XamlLightTargetIdMap.h>
#include <XamlLightTargetMap.h>
#include <windowspresenttarget.h>
#include <ProjectedShadowManager.h>
#include <fwd/windows.applicationmodel.core.h>
#include <fwd/windows.ui.composition.h>
#include <fwd/windows.foundation.h>
#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>
#include <microsoft.ui.composition.interop.h>
#include "VisualDebugTags.h"
#include <dcompinternal.h>
#include <dcompprivate.h>
#include <microsoft.ui.input.experimental.h>
#include <Microsoft.UI.Content.h>

enum XcpGradientWrapMode : uint8_t;

struct AnimationTrackingScenarioInfo;
struct IDXGIDevice;
struct IUnknown;
struct IDWriteTextFormat;
struct ID2D1SolidColorBrush;
struct ID3D11Device;

class OfferTracker;
class CWindowRenderTarget;
class WindowsGraphicsDeviceManager;
class DCompInteropCompositorPartnerCallback;
class DCompSurfaceFactory;
class DCompSurfaceFactoryManager;
class DCompSurface;
class CD3D11Device;
class HWCompNode;
class HWCompTreeNode;
class HWCompTreeNodeWinRT;
class CUIElement;
class CXamlIslandRoot;
class CoreWindowIslandAdapter;
class ProjectedShadowManager;

const XFLOAT c_FrameRateWhiteSpaceWidth = 20.0f;
const XFLOAT c_FrameRateHeight = 24.0f;
const XFLOAT c_FrameRateFrameCountWidth = 40.0f;
const XFLOAT c_FrameRateCpuTimeWidth = 40.0f;
const XFLOAT c_FrameRateTotalWidth =
      c_FrameRateWhiteSpaceWidth * 2.0f
    + c_FrameRateFrameCountWidth
    + c_FrameRateCpuTimeWidth;

typedef HRESULT(WINAPI *DCOMPOSITIONCREATETARGETFORHANDLEFUNC)(
    _In_ HANDLE handle,
    _Outptr_ void **sharedTarget
    );

// flags below control whether or not every UIElement is backed by a CompNode
static bool s_isFullCompNodeTree = false;
static bool s_isFullCompNodeTreeInitialized = false;

// flags below control whether or not extra debugging info is added to the WinRT visuals we create
static bool s_visualDebugTagsEnabled = false;
static bool s_visualDebugTagsEnabledInitialized = false;

// flags below control whether or not WUC APIs are used to create shapes
static bool s_WUCShapesEnabled = false;
static bool s_WUCShapesEnabledInitialized = false;

class DCompTreeHost : public CXcpObjectBase<IObject>
{
public:
    static bool IsFullCompNodeTree();
    static bool VisualDebugTagsEnabled();
    static bool WUCShapesEnabled();

public:
    static _Check_return_ HRESULT Create(
            _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
            _Outptr_ DCompTreeHost **ppDCompTreeHost
        );

    _Check_return_ HRESULT EnsureDCompDevice() noexcept;
    _Check_return_ HRESULT EnsureResources() noexcept;

    // The following methods are safe to call before EnsureResources.
    _Check_return_ HRESULT ReleaseResources(bool shouldDeferClosingInteropCompostior);

    void CloseAndReleaseInteropCompositor();

    void ReleaseGraphicsResources();

    bool CheckMainDeviceState();

    void RegisterDCompAnimationCompletedCallbackThread();

    _Check_return_ HRESULT PreCommitMainDevice();

    _Check_return_ HRESULT CommitMainDevice();

    uint32_t GetMaxTextureSize() const;

    _Check_return_ HRESULT FreezeDWMSnapshot();
    _Check_return_ HRESULT UnfreezeDWMSnapshotIfFrozen();

    _Check_return_ HRESULT OfferResources();
    _Check_return_ HRESULT ReclaimResources(_Out_ bool *pDiscarded);
    OfferTracker* GetOfferTrackerNoRef() const { return m_offerTracker; }
    void GetSurfaceFactoriesForCurrentThread(_Out_ std::vector<IDCompositionSurfaceFactoryPartner3*>* surfaceFactoryVector);

    _Check_return_ HRESULT NotifyUWPWindowLayoutComplete(CWindowRenderTarget &renderTarget);

    // The following methods are NOT SAFE to call before EnsureResources.
    // The caller must be certain that a call to WindowsGraphicsDeviceManager::WaitForD3DDependentResourceCreation preceded a call here.
    _Check_return_ HRESULT SetTargetWindowUWP(HWND targetHwnd);

    void EnsureRootConnected();
    _Check_return_ HRESULT DisconnectRoot(UINT32 backgroundColor, _In_ const XRECTF &backgroundRect);

    _Check_return_ HRESULT CreateContainerVisual(_Outptr_ WUComp::IContainerVisual **ppContainerVisual);

    _Check_return_ HRESULT CreateSurface(
        XUINT32 width,
        XUINT32 height,
        bool isOpaque,
        bool isAlphaMask,
        bool isVirtual,
        bool isHDR,
        bool requestAtlas,
        _Outptr_ DCompSurface **ppSurface
        );

    xref_ptr<DCompSurface> CreateSurfaceWithNoHardware(bool isVirtual);

    _Check_return_ HRESULT EnsureLegacyDeviceSurface(_In_ DCompSurface* dcompSurface, unsigned int width, unsigned int height);
    _Check_return_ HRESULT CreateCompositionSurfaceForHandle(_In_ HANDLE swapChainHandle, _Outptr_ WUComp::ICompositionSurface** compositionSurface);

    _Check_return_ HRESULT ObtainSurfaceFactory(
        _In_ IUnknown *pIUnk,
        _Outptr_ DCompSurfaceFactory **ppSurfaceFactory
        );

    _Check_return_ HRESULT OnSurfaceFactoryDestroyed(
        _In_ DCompSurfaceFactory* pSurfaceFactory
        );

    _Check_return_ HRESULT SetRoot(_In_ HWCompTreeNodeWinRT *pRoot);

    CD3D11Device * GetGraphicsDevice() const;

    ixp::ICompositionEasingFunctionStatics* GetEasingFunctionStatics() { return m_easingFunctionStatics.Get(); }

    WUComp::ICompositor* GetCompositor() const
    {
        return m_spCompositor.Get();
    }

    ixp::ICompositor2* GetCompositor2() const
    {
        return m_spCompositor2.Get();
    }

    WUComp::ICompositor5* GetCompositor5() const
    {
        return m_spCompositor5.Get();
    }

    WUComp::ICompositor6* GetCompositor6() const
    {
        return m_spCompositor6.Get();
    }

    WUComp::ICompositorPrivate* GetCompositorPrivate() const
    {
        return m_spCompositorPrivate.Get();
    }

    WUComp::ICompositionGraphicsDevice* GetCompositionGraphicsDevice() const
    {
        return m_compositionGraphicsDevice.Get();
    }

    bool HasInteropCompositorPartner() const;

    IDCompositionDesktopDevicePartner3 *GetMainDevice() const
    {
        ASSERT(!m_isInitialized || m_spMainDevice != NULL);
        return m_spMainDevice.Get();
    }

    bool HasSurfaceFactory() const
    {
        return m_spMainSurfaceFactoryPartner != nullptr;
    }

    IDCompositionSurfaceFactoryPartner *GetMainSurfaceFactory() const
    {
        // The SurfaceFactory may be NULL even after initialization, in a device lost situation.
        // Users of the SurfaceFactory must take care not to rely on this, particularly in the OnResume handler.
        FAIL_FAST_ASSERT(!m_isInitialized || m_spMainSurfaceFactoryPartner != NULL);
        return m_spMainSurfaceFactoryPartner.Get();
    }

    IDCompositionSurfaceFactoryPartner2 *GetMainSurfaceFactory2() const
    {
        // The SurfaceFactory may be NULL even after initialization, in a device lost situation.
        // Users of the SurfaceFactory must take care not to rely on this, particularly in the OnResume handler.
        FAIL_FAST_ASSERT(!m_isInitialized || m_spMainSurfaceFactoryPartner2 != nullptr);
        return m_spMainSurfaceFactoryPartner2.Get();
    }

    IDCompositionSurfaceFactoryPartner3 *GetMainSurfaceFactory3() const
    {
        // The SurfaceFactory may be NULL even after initialization, in a device lost situation.
        // Users of the SurfaceFactory must take care not to rely on this, particularly in the OnResume handler.
        FAIL_FAST_ASSERT(!m_isInitialized || m_spMainSurfaceFactoryPartner3 != nullptr);
        return m_spMainSurfaceFactoryPartner3.Get();
    }

    ixp::IContentIsland* GetCoreWindowContentIsland() const
    {
        return m_coreWindowContentIsland.Get();
    }

    bool HasNative8BitSurfaceSupport() const { return m_hasNative8BitSurfaceSupport; }

    void SetAtlasSizeHint(XUINT32 width, XUINT32 height);
    void ResetAtlasSizeHint();
    void DisableAtlas();
    bool AtlasDisabled() { return m_disableAtlas; }

    _Check_return_ HRESULT UpdateAtlasHintForRequest(XUINT32 requestWidth, XUINT32 requestHeight);
    _Check_return_ HRESULT UpdateExplicitAtlasHint();

    _Check_return_ HRESULT UpdateDebugSettings(
        bool isFrameRateCounterEnabled
    );

    void UpdateUIThreadCounters(
        XUINT32 uiThreadFrameRate,
        XFLOAT uiThreadCPUTime);

    _Check_return_ HRESULT AnimationTrackingScenarioBegin(_In_ const AnimationTrackingScenarioInfo* pScenarioInfo);

    _Check_return_ HRESULT AnimationTrackingScenarioReference(
        _In_opt_ const GUID* pScenarioGuid,
        XUINT64 uniqueKey);

    _Check_return_ HRESULT AnimationTrackingScenarioUnreference(
        _In_opt_ const GUID* pScenarioGuid,
        XUINT64 uniqueKey);

    bool IsAnimationTrackingEnabled();

    _Check_return_ HRESULT RequestMainDCompDeviceCommit();

    DependencyObjectDCompRegistry* GetDCompObjectRegistry();

    WUCBrushManager* GetWUCBrushManager();

    _Check_return_ HRESULT WaitForCommitCompletion();

    void AbandonDCompObjectRegistry();

    HandOffVisualDataMap& GetHandOffVisualDataMap()
    {
        return m_handOffVisualDataMap;
    }

    containers::vector_map<CUIElement*, Microsoft::WRL::ComPtr<WUComp::IVisual>>& GetHandInVisualsMap()
    {
        return m_handInVisuals;
    }

    typedef containers::vector_map<CUIElement*, ImplicitAnimationInfo> ImplicitAnimationsMap;
    ImplicitAnimationsMap& GetImplicitAnimationsMap(ImplicitAnimationType iaType)
    {
        return iaType == ImplicitAnimationType::Show ? m_implicitShowAnimationsMap : m_implicitHideAnimationsMap;
    }

    void AddElementWithKeepAliveCount(_In_ CUIElement* element);
    void RemoveElementWithKeepAliveCount(_In_ CUIElement* element);
    bool HasElementsWithKeepAliveCount() const;
    bool HasDescendantWithKeepAliveCount(_In_ CUIElement* ancestor) const;

    // Helper method used when removing an element from the tree. Find all elements in the subtree with Hidden event handlers,
    // and call RequestKeepAlive on them if they're not being kept alive already.
    void RequestKeepAliveForHiddenEventHandlersInSubtree(_In_ CUIElement* ancestor);

    _Check_return_ HRESULT GetPointerSourceForElement(_In_ CUIElement *element, _Out_ std::shared_ptr<void> *pointerSourceWrapper);
    void StorePointerSourceForElement(_In_ CUIElement *element, _In_ std::shared_ptr<void> pointerSourceWrapper);
    void ReleasePointerSourceForElement(_In_ CUIElement *element);

    struct XamlIslandRenderData
    {
        xref_ptr<WindowsPresentTarget> windowsPresentTarget;
        bool contentConnected = false;
        // For testing - normally we can get the visual out of the Composition island, but for tests this returns a
        // real visual when we want the mock. So track the root visual separately.
        wrl::ComPtr<ixp::IVisual> m_islandRootVisual;
    };

    using XamlIslandRenderDataMap = containers::vector_map<CXamlIslandRoot*, XamlIslandRenderData>;

    bool HasXamlIslandData() const;

    // Used for ElementCompositionPreview's implicit show/hide animations as well as UIElement.Shown/Hidden events
    void SetTrackEffectiveVisibility(_In_ CUIElement* uiElement, bool track);
    bool IsTrackingEffectiveVisibility(_In_ CUIElement* uiElement) const;

    void UpdateImplicitShowHideAnimations();
    void SetTrackKeepVisible(_In_ CUIElement* uiElement, bool track);
    bool IsKeepVisible(_In_ CUIElement* uiElement);
    void CleanupEffectiveVisibilityTracker(_In_ CUIElement* uiElement);
    bool ShouldKeepVisible(_In_ CUIElement* uiElement);
    bool ShouldKeepVisible(_In_ CUIElement* uiElement, _In_ CUIElement* uiElementWithKeepAlive) const;

    XamlLightTargetIdMap& GetXamlLightTargetIdMap();
    XamlLightTargetMap& GetXamlLightTargetMap();

    void AddXamlIslandTarget(_In_ CXamlIslandRoot* xamlIslandRoot);
    void UpdateXamlIslandTargetSize(_In_ CXamlIslandRoot* xamlIslandRoot);
    void RemoveXamlIslandTarget(_In_ CXamlIslandRoot* xamlIslandRoot);
    _Check_return_ HRESULT ConnectXamlIslandTargetRoots();
    _Check_return_ HRESULT EnsureXamlIslandTargetRoots();

    XamlIslandRenderDataMap& GetXamlIslandRenderData() { return m_islandRenderData; };
    void SetCoreWindow(_In_ wuc::ICoreWindow* const coreWindow) { m_coreWindowNoRef = coreWindow; }

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    UINT64 GetCompositionIslandId();
#endif

    static void SetTag(_In_ WUComp::IVisual* visual, _In_ const wchar_t* debugTag, float value);

    static void SetTagIfEnabled(_In_ WUComp::IVisual* visual, VisualDebugTags tag);

    WUComp::IContainerVisual* GetHwndRootVisual() { return m_hwndVisual.Get(); }
    ProjectedShadowManager* GetProjectedShadowManager() { return m_projectedShadowManager.get(); }
    std::weak_ptr<ProjectedShadowManager> GetProjectedShadowManagerWeakPtr();

    SharedTransitionAnimations* GetSharedTransitionAnimationsNoRef() { return &m_sharedTransitionAnimations; }

    _Check_return_ HRESULT OnSurfaceFactoryCreated(_In_ DCompSurfaceFactory* newSurfaceFactory);

    _Check_return_ HRESULT GetSystemBackdropBrush(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush);
    _Check_return_ HRESULT SetSystemBackdropBrush(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush);

    wrl::ComPtr<ixp::IVisual> GetInprocIslandRoot() { return m_inprocIslandRootVisual; }

    ixp::ICoreWindowSiteBridge* GetCoreWindowBridgeNoRef() { return m_contentBridgeCW.Get(); }

private:
    XamlIslandRenderDataMap m_islandRenderData;

    DCompTreeHost(_In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager);
    ~DCompTreeHost() override;

    CCoreServices* GetCoreServicesNoRef() const;

    void ComputeAndCachePrimaryMonitorSize();
    void CleanupSurfaceFactoryMap();

    _Check_return_ HRESULT EnsureTextFormat();
    void ShowUIThreadCounters();
    void HideUIThreadCounters();
    _Check_return_ HRESULT CreateFrameRateSurface(
        XUINT32 uiThreadFrameRate,
        XFLOAT uiThreadCPUTime,
        _Outptr_ DCompSurface** ppFrameRateSurface);
    _Check_return_ HRESULT EnsureAnimationTrackingAppId();

    _Check_return_ HRESULT UpdateAtlasHint();

    _Check_return_ HRESULT CreateWinRTInteropCompositionDevice(
        _In_ REFIID iid,
        _Out_ void **ppDevice);

    _Check_return_ HRESULT SetRootForCorrectContext(_In_ WUComp::IVisual *visual);
    bool HasDCompTarget();
    _Check_return_ HRESULT SetTargetHelper();

    void UpdateEffectiveVisibilities();
    void UpdateEffectiveVisibilityTracker(_In_ CUIElement* element);
    EffectiveVisibilityTracker GetEffectiveVisibilityTracker(_In_ CUIElement* element) const;
    EffectiveVisibilityInfo ComputeEffectiveVisibility(_In_ CUIElement* uiElement);

    void CreateVisualTreeCompositionIslandAdapter();
    void ReleaseVisualTreeCompositionIslandAdapter();

    bool IsElementInKeepAliveVector(_In_ CUIElement* element);

private:
    // Storage for references to handin and handoff visuals that, once created, persist throughout UIElement's lifetime.
    // When app calls ElementCompositionPreview::GetElementVisual / GetElementChildVisual / SetElementChildVisual API
    // or the internal IXamlDCompInteropPrivate interface is used, these get added to the appropriate map below,
    // keyed on the specified UIElement. They are transferred (AddRef'ed) to associated compnodes as those are created and destroyed.
    // Finally, when that UIE is destroyed, the entry here is removed. At that time, we also call Close (handin) or RealClose
    // (handoff) API to make sure the visual is disconnected and not holding graphics resources or other visuals, even if app still has a reference.
    HandOffVisualDataMap m_handOffVisualDataMap;
    containers::vector_map<CUIElement*, Microsoft::WRL::ComPtr<WUComp::IVisual>> m_handInVisuals;

    std::unordered_map<CUIElement*, std::shared_ptr<void>> m_hoverPointerSourceMap;
    ImplicitAnimationsMap m_implicitHideAnimationsMap;
    ImplicitAnimationsMap m_implicitShowAnimationsMap;

    // We track the effective visibility of an element if it has ECP show/hide animations or if it has UIE.Shown/Hidden
    // event handlers.
    containers::vector_map<CUIElement*, EffectiveVisibilityInfo> m_effectiveVisibilityMap;
    containers::vector_map<CUIElement*, EffectiveVisibilityTracker> m_effectiveVisibilityTrackerMap;

    // Elements being kept visible. These could be running ElementCompositionPreview's hide animations, or they could be
    // on the ancestor chain of elements with RequestKeepAlive called on them.
    std::vector<CUIElement*> m_keepVisible;

    // Elements with a KeepAlive count. These are the elements that actually had RequestKeepAlive() called on them.
    // m_keepVisible are the elements that are kept a live, possibly as a result of an element in this list. For example,
    // an ancestor Grid could be in m_keepVisible because one of its descendant Borders has a KeepAlive count.
    //
    // This list is an optimization. When a UIElement is removed from the tree, we need to see whether it has any children
    // that has a KeepAlive count. So either we walk down the entire subtree, or we walk up from every known element with
    // a KeepAlive count. We can also skip unloading storage work if we know there's nothing in the tree with a KeepAlive
    // count (as well as knowing there's nothing using any of the other features require unloading storage).
    std::vector<CUIElement*> m_elementsWithKeepAliveCount;

    XamlLightTargetIdMap m_xamlLightTargetIdMap;
    XamlLightTargetMap m_xamlLightTargetMap;

    std::shared_ptr<ProjectedShadowManager> m_projectedShadowManager;

    WindowsGraphicsDeviceManager    *m_pGraphicsDeviceManagerNoRef;

    // DComp resources
    _Maybenull_ Microsoft::WRL::ComPtr<IDCompositionDesktopDevicePartner3> m_spMainDevice;
    _Maybenull_ Microsoft::WRL::ComPtr<IDCompositionDeviceInternal> m_spMainDeviceInternal;
    _Maybenull_ Microsoft::WRL::ComPtr<IDCompositionSurfaceFactoryPartner> m_spMainSurfaceFactoryPartner;
    _Maybenull_ Microsoft::WRL::ComPtr<IDCompositionSurfaceFactoryPartner2> m_spMainSurfaceFactoryPartner2;
    _Maybenull_ Microsoft::WRL::ComPtr<IDCompositionSurfaceFactoryPartner3> m_spMainSurfaceFactoryPartner3;

#pragma region ::Windows::UI::Composition

    // WinRT composition objects
    _Maybenull_ Microsoft::WRL::ComPtr<WUComp::ICompositor> m_spCompositor;
    _Maybenull_ Microsoft::WRL::ComPtr<ixp::ICompositor2> m_spCompositor2;
    _Maybenull_ Microsoft::WRL::ComPtr<WUComp::ICompositor5> m_spCompositor5;
    _Maybenull_ Microsoft::WRL::ComPtr<WUComp::ICompositor6> m_spCompositor6;
    _Maybenull_ Microsoft::WRL::ComPtr<WUComp::ICompositorPrivate> m_spCompositorPrivate;
    _Maybenull_ Microsoft::WRL::ComPtr<WUComp::ICompositorInterop> m_spCompositorInterop;
    _Maybenull_ Microsoft::WRL::ComPtr<WUComp::IInteropCompositorPartner> m_spInteropCompositorPartner;
    _Maybenull_ Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionBrush> m_systemBackdropBrush; // Note: This is a system compositor brush!
    wrl::ComPtr<WUComp::ICompositionGraphicsDevice> m_compositionGraphicsDevice;
    wrl::ComPtr<ixp::ICompositionEasingFunctionStatics> m_easingFunctionStatics;

#pragma endregion

    // Application ID saved for animation/touch tracking.
    xstring_ptr m_strAnimationTrackingAppId;
    bool m_animationTrackingAppIdSetOnDevice;
    bool m_hasNative8BitSurfaceSupport;

    wrl::ComPtr<WUComp::IContainerVisual> m_hwndVisual;
    wrl::ComPtr<WUComp::IVisual> m_disconnectedHWndVisual;
    wrl::ComPtr<WUComp::IVisual> m_compNodeRootVisual;
    wrl::ComPtr<WUComp::IVisual> m_frameRateVisual;

    // DCompInteropCompositorPartnerCallback instance, a WinRT interop compositor callback, which implements WUComp::IInteropCompositorPartnerCallback
    xref_ptr<DCompInteropCompositorPartnerCallback> m_spInteropCompositorPartnerCallback;

    // Dummy visuals for MockDComp output.
    // Xaml's visual tree used to contain extra visuals at the root. They've been removed, but we haven't updated our
    // MockDComp test masters to account for the difference. When all changes to the root of the tree are done, we can
    // remove these dummy visuals and update MockDComp masters together in an isolated change. In the meantime we'll
    // add dummy visuals to the tree to keep MockDComp comparisons passing and not have to churn many hundreds of test
    // masters.
    wrl::ComPtr<WUComp::IVisual> m_mockDCompDummyVisual1;
    wrl::ComPtr<WUComp::IVisual> m_mockDCompDummyVisual2;

    IDWriteTextFormat *m_pFrameRateTextFormat;
    ID2D1SolidColorBrush *m_pFrameRateScratchBrush;

    HWND m_targetHwnd;
    Microsoft::WRL::ComPtr<wac::ICoreApplicationView> m_View;

    enum DwmThumbnailState
    {
        Unfrozen,
        Frozen
    };

    DwmThumbnailState m_thumbnailState;
    xref_ptr<OfferTracker> m_offerTracker;

    XUINT32 m_primaryMonitorWidth;
    XUINT32 m_primaryMonitorHeight;
    XUINT32 m_atlasSizeHintWidth;
    XUINT32 m_atlasSizeHintHeight;
    bool m_disableAtlas;
    bool m_useExplicitAtlasHint;
    bool m_isInitialized;
    bool m_needsFrameRateVisual = false;

    XRECTF m_backgroundRect;

    DependencyObjectDCompRegistry m_dcompObjectRegistry;
    WUCBrushManager m_wucBrushManager;

    // We want callbacks from DComp on the UI thread when animations complete. By default callbacks are delivered
    // to the thread that created the DComp device, which in Xaml's case is a worker thread. So we need to call the
    // RegisterCallbackThread method on the UI thread, which we'll do when we commit the device. This flag marks
    // whether the call has been made.
    bool m_isCallbackThreadRegistered;

    wuc::ICoreWindow* m_coreWindowNoRef = nullptr;

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    std::unique_ptr<CoreWindowIslandAdapter> m_onecoreIslandAdapter;
#endif

    SharedTransitionAnimations m_sharedTransitionAnimations;

    // For testing - normally we can get the visual out of m_inprocIsland, but for tests this returns a real visual when
    // we want the mock. So track the root visual separately.
    _Maybenull_ wrl::ComPtr<ixp::IVisual> m_inprocIslandRootVisual;
    _Maybenull_ wrl::ComPtr<ixp::IContentSiteBridge> m_contentBridge;
    _Maybenull_ wrl::ComPtr<ixp::ICoreWindowSiteBridge> m_contentBridgeCW;
    _Maybenull_ wrl::ComPtr<ixp::IContentIsland> m_coreWindowContentIsland;
};

class DCompSurfaceFactory : public CXcpObjectBase<IObject>
{
private:
    DCompSurfaceFactory(
        _In_ DCompTreeHost *pDCompTreeHost,
        _In_ IDCompositionSurfaceFactory *pSurfaceFactory
        );
    ~DCompSurfaceFactory() override;

public:
    static _Check_return_ HRESULT Create(
        _In_ DCompTreeHost *pDCompTreeHost,
        _In_ IDCompositionDesktopDevicePartner *pMainDevice,
        _In_ IUnknown *pIUnk,
        _Outptr_ DCompSurfaceFactory **ppSurfaceFactory
        );

    _Check_return_ HRESULT CreateSurface(
        XUINT32 width,
        XUINT32 height,
        bool isOpaque,
        bool isVirtual,
        bool isHDR,
        bool requestAtlas,
        _Outptr_ DCompSurface **ppSurface
        );

    _Check_return_ HRESULT Flush();

    IDCompositionSurfaceFactory* GetRealSurfaceFactory() { return m_SurfaceFactory; }

    DCompTreeHost* GetDCompTreeHost() { return m_DCompTreeHostNoRef; }

    IDCompositionSurfaceFactoryPartner3* GetSurfaceFactoryPartner() { return m_SurfaceFactoryPartner.Get();  }

private:
    DCompTreeHost *m_DCompTreeHostNoRef;
    IDCompositionSurfaceFactory *m_SurfaceFactory;
    Microsoft::WRL::ComPtr<IDCompositionSurfaceFactoryPartner3> m_SurfaceFactoryPartner;
};

namespace DCompHelpers
{
    _Check_return_ HRESULT D3D11DeviceFromUnknownDevice(
        _In_ IUnknown *pUnknownDevice,
        _Outptr_ ID3D11Device **ppD3D11Device
        );
}
