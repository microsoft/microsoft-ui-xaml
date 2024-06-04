// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ConnectedAnimation.h"
#include "CDependencyObject.h"

class CEasingFunctionBase;

class CConnectedAnimationService : public CDependencyObject
{
public:
    CConnectedAnimationService(_In_ CCoreServices *pCore);

    KnownTypeIndex GetTypeIndex() const override;
    bool ControlsManagedPeerLifetime() override
    {
        // Don't let GC happen with strengthen the reference on the managed peer while
        // a native object is holding this object.
        return true;
    }

    _Check_return_ bool HasAnimations() const {return m_animations.size() > 0;}
    _Check_return_ HRESULT CreateAnimation(const xstring_ptr_view& key, _Outptr_ CConnectedAnimation ** animation);
    _Check_return_ HRESULT CreateCoordinatedAnimation(_In_ CConnectedAnimation * baseAnimation, _Outptr_ CConnectedAnimation ** animation);
    _Check_return_ HRESULT GetAnimation(const xstring_ptr_view& key, _Outptr_opt_ CConnectedAnimation ** animation);

    const std::vector<xref_ptr<CUIElement>>& GetUnloadingElements();
    _Check_return_ HRESULT OnUnloadingElement(_In_ CUIElement* unloadingElement, _Out_ bool* shouldRetain);
    _Check_return_ HRESULT PreRenderWalk();
    _Check_return_ HRESULT PreCommit();
    _Check_return_ HRESULT PostCommit();
    _Check_return_ HRESULT CancelAllAnimationsAndResetDefaults();

    const wf::TimeSpan GetAnimationDuration();

    wf::TimeSpan GetDefaultDuration(){return m_defaultDuration;}
    void SetDefaultDuration(_In_ wf::TimeSpan duration){m_defaultDuration = duration;}

    _Check_return_ HRESULT GetDefaultEasingFunction(_Outptr_result_maybenull_ WUComp::ICompositionEasingFunction** ppValue);
    _Check_return_ HRESULT SetDefaultEasingFunction(_In_opt_ WUComp::ICompositionEasingFunction* pValue);

    _Check_return_ HRESULT TimeoutAnimations();
    void DelayStartOfAnimations() { m_delayStartOfAnimations = true; }
    bool AnimationStartsAreDelayed() { return m_delayStartOfAnimations; }

private:
    _Check_return_ HRESULT CleanupRetainedElements();

    std::vector<xref_ptr<CConnectedAnimation>> m_animations;
    std::vector<xref_ptr<CUIElement>> m_retainedElements;
    Microsoft::WRL::ComPtr<WUComp::ICompositionEasingFunction> m_defaultEasingFunction;
    wf::TimeSpan m_defaultDuration;
    bool m_waitForCommitCompletion = false;

    // allows us to delay the actual start of animations a tick if something is changing.
    bool m_delayStartOfAnimations = false;


};
