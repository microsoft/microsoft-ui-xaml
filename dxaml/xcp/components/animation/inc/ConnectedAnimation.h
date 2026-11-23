// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "CDependencyObject.h"
#include "Request.h"
#include "ConnectedAnimationCoreConfiguration.h"
#include <fwd/windows.ui.composition.h>
#include <fwd/Microsoft.UI.Xaml.media.animation.h>
#include <ImmediateAnimations.h> // ---- transitions
#include <vector>

class CUIElement;
class CConnectedAnimationService;
class CConnectedAnimationRoot;
class CXamlIslandRoot;
interface IUnknown;
class CEasingFunctionBase;
class CMemorySurface;

enum class ConnectedAnimationState
{
    Idle,           // Created but nothing has been done with it
    Initialized,    // Prepare Source has been requested
    Unloaded,       // Initialized and unloaded before Prepared
    Prepared,       // Source is prepared (popped out of the scene)
    Started,        // Animation has been requested to start
    Running,        // Animation has been started
    Canceled,       // Animation has been canceled
    Complete,       // Animation has been completed
    ReadyToComplete  // Ready to complete when parent transition completes.
};

struct ConnectedAnimationElementInfo
{
    bool refreshSnapshot = true;
    xref_ptr<CUIElement> element;
    Microsoft::WRL::ComPtr<WUComp::IVisual> visual;
    Microsoft::WRL::ComPtr<WUComp::ICompositionBrush> surfaceBrush;
    XRECTF_WH clippedRect{};
    XRECTF_WH unclippedRect{};
    CMILMatrix transform;
 };

// ---- transitions ----
// Immediate rendering (F10): when the source's window renders with an immediate backend, CConnectedAnimation does not
// build Composition snapshots or a host visual. It captures the source once (premultiplied BGRA at the rasterization
// scale, capped), evaluates the host visual's offset/scale/clip/cross-fade itself and the walk paints the snapshot and
// the live destination on top of the window (core\hw\ImmediateRenderer.Connected.inl). Rects are island DIPs; "host"
// rects are in the host visual's local (pre-scale) space.
struct ConnectedAnimationImmediateState
{
    bool active = false;            // emulated by the immediate walk
    bool running = false;           // host animates (Running/ReadyToComplete)
    bool shadow = false;            // configuration casts a shadow
    bool approximated = false;      // easing could not be read back (default curve used)
    bool coordinated = false;       // no source snapshot: the host fades in
    bool customComponents = false;  // app component animations (not evaluated: the default motion is used)
    bool hasDestination = false;    // destination drawn (Comp: a destination snapshot brush exists)
    int64_t startUs = 0;
    int64_t durationUs = 0;
    ImmediateAnimations::Curve curve;
    ImmediateAnimations::ConnectedParams params;
    float hostWidth = 0, hostHeight = 0;
    // Source snapshot (null when none) drawn into sourceContent, showing sourceCrop (snapshot pixels).
    xref_ptr<CMemorySurface> snapshot;
    std::vector<uint8_t> pixels;
    XRECTF_WH sourceContent{};
    XRECTF_WH sourceCrop{};
    // Destination is walked live: destRect maps onto destContent.
    XRECTF_WH destRect{};
    XRECTF_WH destContent{};
};
class CConnectedAnimation : public CDependencyObject
{
public:
    CConnectedAnimation(_In_ xstring_ptr& key, _In_ CCoreServices *pCore, _In_ CConnectedAnimationService* service);
    CConnectedAnimation(_In_ CCoreServices *pCore, _In_ CConnectedAnimation * baseAnimation, _In_ CConnectedAnimationService* service);
    ~CConnectedAnimation() override;

    KnownTypeIndex GetTypeIndex() const override;

    _Check_return_ HRESULT Prepare(_In_ CUIElement* sourceElement);
    _Check_return_ HRESULT TryStart(_In_ CUIElement* destinationElement, _Out_ BOOLEAN* result);
    _Check_return_ HRESULT AnimateCoordinatedEntrance(_In_ CUIElement* pElement);
    _Check_return_ HRESULT Cancel();
    _Check_return_ HRESULT SetAnimationComponent(_In_ xaml_animation::ConnectedAnimationComponent component, _In_opt_ WUComp::ICompositionAnimationBase* animation);
    void GetScaleAnimationEnabled (_Out_ BOOLEAN * enabled) const;
    void PutScaleAnimationEnabled(_In_ BOOLEAN enabled);
    void GetConfiguration(_Out_ IConnectedAnimationCoreConfiguration** configuration);
    void PutConfiguration(_In_ IConnectedAnimationCoreConfiguration* configuration);

    ConnectedAnimationState GetState() { return m_state; }
    xstring_ptr GetKey() const { return m_key; }
    CUIElement* GetSourceElementNoRef() { return m_source.element.get(); }

    // ---- transitions ----
    const ConnectedAnimationImmediateState& GetImmediateState() const { return m_immediate; }
    CUIElement* GetDestinationElementNoRef() { return m_destination.element.get(); }
    CConnectedAnimationRoot* GetRootNoRef() { return GetConnectedAnimationRootNoRef(); }
    // Whether the immediate walk hides lement (an emulated animation's source or destination).
    bool IsHiddenByImmediateEmulation(_In_ const CUIElement* element) const;
    // Whether the element is the source or destination of an animation emulated by the immediate walk.
    bool IsEmulatedImmediately(_In_ const CUIElement* element) const
    {
        return m_immediate.active && element != nullptr && (element == m_source.element.get() || element == m_destination.element.get());
    }
    bool IsPreparedState() const { return m_state == ConnectedAnimationState::Prepared || m_state == ConnectedAnimationState::Started; }

    _Check_return_ HRESULT OnUnloadingElement(_In_ CUIElement* unloadingElement, _Out_ bool* shouldRetain);
    _Check_return_ HRESULT UpdatePreRenderWalk();
    _Check_return_ HRESULT UpdatePreCommit(_Out_ bool * remove, _Out_ bool * waitForCommitCompletion);

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL) final
    {
        UNREFERENCED_PARAMETER(hEvent);
        UNREFERENCED_PARAMETER(fInputEvent);
        UNREFERENCED_PARAMETER(pArgs);
        return (m_eventList != nullptr);
    }

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _In_ bool fHandledEventsToo = false) override;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    gsl::span<REQUEST> GetEventHandlers() override
    {
        return (m_eventList) ? gsl::span<REQUEST>(*m_eventList) : gsl::span<REQUEST>();
    }

    // COD overrides, this is required for event manager to add request for this animation, see CEventManager::AddEventListener.
    bool IsActive() const override { return m_state != ConnectedAnimationState::Idle; }

    // WUC::CompositionScopedBatch::Completed handler.
    _Check_return_ HRESULT OnAnimationCompleted(
        _In_ IInspectable* sender,
        _In_ WUComp::ICompositionBatchCompletedEventArgs* args);

    bool ControlsManagedPeerLifetime() override
    {
        // Don't let GC happen with strengthen the reference on the managed peer while
        // a native object is holding this object.
        return true;
    }
    _Check_return_ HRESULT TimeoutIfNecessary(_Out_ UINT64* requestTickInverval);

private:
    _Check_return_ HRESULT Reset();
    _Check_return_ HRESULT ClearElementInfo(_In_ ConnectedAnimationElementInfo & info);
    _Check_return_ HRESULT CreateSnapshotBrush(_In_ ConnectedAnimationElementInfo & info, bool createClippedBrush, bool freezeBrush) noexcept;
    _Check_return_ HRESULT StartSpriteAnimations();
    _Check_return_ HRESULT GetSnapshotTransformInfo(_In_ ConnectedAnimationElementInfo & info);
    void GetBaseAnimationSourceInfo();
    _Check_return_ HRESULT TryToCompleteAnimation();
    _Check_return_ HRESULT ConvertSourceBrushToClipped();
    _Check_return_ HRESULT UpdateVisualRectForClippedBrushes();
    void ComputeAnimationOffset(_In_ ConnectedAnimationElementInfo& info, _In_ ConnectedAnimationElementInfo& info2, wfn::Vector3& offset);

    bool IsUnloadingElementAncestor(_In_ CUIElement* unloadingElement, _In_ ConnectedAnimationElementInfo& animationElementInfo);

    void FireCompletedEvent();

    _Check_return_ HRESULT StartCustomAnimation(
        _In_ WUComp::ICompositionObject * compositionObject,
        _In_ LPCWSTR propertyName,
        _In_ xaml_animation::ConnectedAnimationComponent animationComponent,
        _In_ float from,
        _In_ float to,
        _In_ wf::TimeSpan ts,
        _In_ WUComp::ICompositionEasingFunction * easingFunction,
        _In_ WUComp::ICompositor * compositor);

    _Check_return_ HRESULT StartScalarAnimation(
        _In_ WUComp::ICompositionObject * compositionObject,
        _In_ LPCWSTR propertyName,
        _In_ float from,
        _In_ float to,
        _In_ wf::TimeSpan ts,
        _In_ WUComp::ICompositionEasingFunction * easingFunction,
        _In_ WUComp::ICompositor * compositor);

    _Check_return_ HRESULT StartVector3Animation(
        _In_ WUComp::ICompositionObject * compositionObject,
        _In_ LPCWSTR propertyName,
        _In_ wfn::Vector3 from,
        _In_ wfn::Vector3 to,
        _In_ wf::TimeSpan ts,
        _In_ WUComp::ICompositionEasingFunction * easingFunction,
        _In_ WUComp::ICompositor * compositor);

    CConnectedAnimationRoot* GetConnectedAnimationRootNoRef();
    CXamlIslandRoot* GetXamlIslandRootNoRef();

    ConnectedAnimationState m_state;
    ConnectedAnimationElementInfo m_source;
    ConnectedAnimationElementInfo m_destination;

    UINT64 m_preparedTimestamp;
    CConnectedAnimationService* m_connectedAnimationServiceNoRef;
    Microsoft::WRL::ComPtr<WUComp::IVisual> m_hostVisual;
    Microsoft::WRL::ComPtr<WUComp::IInsetClip> m_insetClip;
    wrl::ComPtr<WUComp::ICompositionScopedBatch> m_scopedAnimationBatch;
    EventRegistrationToken m_scopedAnimationBatchCompletedToken{};
    xref_ptr<CConnectedAnimation> m_baseAnimation;
    xstring_ptr m_key;

    bool m_isSourceBrushClipped = false;
    bool m_isScaleAnimationEnabled = true;

    wrl::ComPtr<WUComp::ICompositionAnimation> m_componentAnimation[4];
    wrl::ComPtr<IConnectedAnimationCoreConfiguration> m_configuration;

    std::unique_ptr<std::vector<REQUEST>> m_eventList;

    // We need to keep a reference to any property sets we use because DComp only keeps
    // a weak ref when they are used.
    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> m_effectPropertySet;
    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> m_primaryPropertySet;

    // ---- transitions ----
    _Check_return_ HRESULT CaptureSourceImmediate();
    _Check_return_ HRESULT UpdatePreCommitImmediate(_Out_ bool* remove);
    _Check_return_ HRESULT StartImmediate();
    ConnectedAnimationImmediateState m_immediate;


};
