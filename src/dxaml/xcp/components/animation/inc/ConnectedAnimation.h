// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "CDependencyObject.h"
#include "Request.h"
#include "ConnectedAnimationCoreConfiguration.h"
#include <fwd/windows.ui.composition.h>
#include <fwd/Microsoft.UI.Xaml.media.animation.h>

class CUIElement;
class CConnectedAnimationService;
class CConnectedAnimationRoot;
class CXamlIslandRoot;
interface IUnknown;
class CEasingFunctionBase;

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

    _Check_return_ HRESULT OnUnloadingElement(_In_ CUIElement* unloadingElement, _Out_ bool* shouldRetain);
    _Check_return_ HRESULT UpdatePreRenderWalk();
    _Check_return_ HRESULT UpdatePreCommit(_Out_ bool * remove, _Out_ bool * waitForCommitCompletion);

    bool ShouldRaiseEvent(_In_ EventHandle hEvent, _In_ bool fInputEvent = false, _In_opt_ CEventArgs *pArgs = NULL) final
    {
        UNREFERENCED_PARAMETER(hEvent);
        UNREFERENCED_PARAMETER(fInputEvent);
        UNREFERENCED_PARAMETER(pArgs);
        return (m_pEventList != nullptr);
    }

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult,
        _In_ bool fHandledEventsToo = false) override;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

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

    CXcpList<REQUEST> *m_pEventList = nullptr;

    // We need to keep a reference to any property sets we use because DComp only keeps
    // a weak ref when they are used.
    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> m_effectPropertySet;
    Microsoft::WRL::ComPtr<WUComp::ICompositionPropertySet> m_primaryPropertySet;


};
