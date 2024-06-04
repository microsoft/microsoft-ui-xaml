// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Handle.h"
#include "PowerModeRequestor.h"

class HWWalk;
class CMediaPlayerElement;

class CMediaPlayerPresenter final : public CFrameworkElement, public IPLMListener
{
    friend class HWWalk;
public:
    DECLARE_CREATE(CMediaPlayerPresenter);

    CMediaPlayerPresenter(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore),
        m_isFullWindow(false),
        m_Stretch(DirectUI::Stretch::Uniform),
        m_nNaturalVideoHeight(0),
        m_nNaturalVideoWidth(0),
        m_fRegisteredForUpdate(false),
        m_suspended(false),
        m_hasFirstFrameBasedOnState(false),
        m_previousDestSize({}),
        m_previousNormalizedRect({})
    {
        m_requiresThreadSafeAddRefRelease = true;

        m_mediaOpenedToken.value = 0;
        m_mediaCurrentStateChangedToken.value = 0;
        m_mediaCastingRenderLocationChangedToken.value = 0;
        m_naturalVideoSizeChangedToken.value = 0;
        m_mediaCurrentItemChangedToken.value = 0;
        m_mediaSourceChangedToken.value = 0;
        m_playbackStateChangedToken.value = 0;
        m_bufferingEndedToken.value = 0;

        m_stretchTransform.SetToIdentity();
        EmptyRect(&m_destinationRect);
        SetInfiniteClip(&m_stretchClip);
    }

    ~CMediaPlayerPresenter() override
    {
        VERIFYHR(RemoveEventRegistrations());
        // Sanity check to make sure we restore power saving when ME is destroyed.
        // (This should normally be handled by transition to Closed state).
        VERIFYHR(UpdatePowerSettings());
    }

    bool m_isFullWindow;
    DirectUI::Stretch m_Stretch;

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CMediaPlayerPresenter>::Index;
    }

    HANDLE GetSwapChainHandle() const
    {
        return (HANDLE)m_mediaPlayerSwapChain.get();
    }

    const XRECT* GetDestinationRect() const
    {
        return &m_destinationRect;
    }

    const CMILMatrix* GetStretchTransform() const
    {
        return &m_stretchTransform;
    }

    const XRECTF* GetStretchClip() const
    {
        return &m_stretchClip;
    }

    _Check_return_ HRESULT GetNaturalBounds(_Inout_ XRECT& pNaturalBounds);

    _Check_return_ HRESULT UpdateState() override;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    _Check_return_ bool GetIsLayoutElement() const override
    {
        return true;
    }

    // IPLMListener
    _Check_return_ HRESULT OnSuspend(_In_ bool isTriggeredByResourceTimer) override;
    _Check_return_ HRESULT OnResume() override;
    void OnLowMemory() override {}

    void PreMeasure(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize);

    void SetOwner(_In_ CMediaPlayerElement* pOwner);

protected:

    _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize) override;

private:

    CFrameworkElement* GetLayoutOwnerNoRef();

    _Check_return_ HRESULT UpdateInternalSize(
        _Out_ float &rActualWidth,
        _Out_ float &rActualHeight,
        _Out_ DirectUI::AlignmentX &alignmentX,
        _Out_ DirectUI::AlignmentY &alignmentY);

    _Check_return_ HRESULT UpdateMediaPlayer();

    _Check_return_ HRESULT SetMediaPlayerSwapChain();
    _Check_return_ HRESULT SetMediaPlayerSwapChain(_In_ HANDLE newHandle);

    _Check_return_ HRESULT CalculateDCompParameters();

    void ResetNaturalVideoSize()
    {
        m_nNaturalVideoHeight = 0;
        m_nNaturalVideoWidth = 0;
    }

    _Check_return_ HRESULT UpdateNaturalBounds();

    _Check_return_ HRESULT UpdateVideoStream(_In_ const XRECTF_RB &normalizedSourceRect);

    bool HasFirstFrameBasedOnState();

    _Check_return_ HRESULT AddEventRegistrations();
    _Check_return_ HRESULT RemoveEventRegistrations();

    _Check_return_ HRESULT OnMediaPlayerCastingLocationChanged(_In_ wmp::IMediaPlayer*, _In_ IInspectable*);
    _Check_return_ HRESULT OnMediaPlayerSourceChanged(_In_ wmp::IMediaPlayer*, _In_ IInspectable*);
    _Check_return_ HRESULT OnMediaOpened(wmp::IMediaPlayer* pSender, IInspectable* pArgs);

    _Check_return_ HRESULT OnNaturalVideoSizeChanged(_In_ wmp::IMediaPlaybackSession*, _In_ IInspectable*);
    _Check_return_ HRESULT OnMediaPlaybackSessionBufferingEnded(_In_ wmp::IMediaPlaybackSession*, _In_ IInspectable*);
    _Check_return_ HRESULT OnMediaPlaybackSessionPlaybackStateChanged(_In_ wmp::IMediaPlaybackSession*, _In_ IInspectable*);

    _Check_return_ HRESULT OnMediaCurrentItemChanged(_In_ wmp::IMediaPlaybackList*, _In_ wmp::ICurrentMediaPlaybackItemChangedEventArgs*);

    _Check_return_ HRESULT UpdatePowerSettings();

    _Check_return_ HRESULT UpdateRenderingAndPowerSettings();

    _Check_return_ bool HasVideo() const
    {
        return (m_nNaturalVideoHeight > 0 && m_nNaturalVideoWidth > 0);
    }

    _Check_return_ bool HasRenderSize()
    {
        return (
            !GetIsMeasureDirty() &&
            !GetIsArrangeDirty() &&
            HasLayoutStorage() &&
            RenderSize.width > 0.0 && RenderSize.height > 0.0);
    }

    _Check_return_ bool CanPresent()
    {
        return HasRenderSize() && HasVideo() && HasFirstFrameBasedOnState();
    }

    _Check_return_ HRESULT HandleError(_In_ HRESULT hr);

    _Check_return_ HRESULT RegisterForUpdate();
    _Check_return_ HRESULT UnregisterForUpdate();

    Handle m_mediaPlayerSwapChain;
    ctl::ComPtr<wmp::IMediaPlayer> m_spMediaPlayer;
    ctl::ComPtr<wmp::IMediaPlaybackSession> m_spPlaybackSession;
    ctl::ComPtr<wmp::IMediaPlaybackList> m_spMediaPlaybackList;

    XRECT m_destinationRect;
    CMILMatrix m_stretchTransform;
    XRECTF m_stretchClip;

    XUINT32 m_nNaturalVideoHeight;
    XUINT32 m_nNaturalVideoWidth;
    wf::Size m_previousDestSize;
    wf::Rect m_previousNormalizedRect;

    //Events
    EventRegistrationToken m_mediaOpenedToken;
    EventRegistrationToken m_mediaCurrentStateChangedToken;
    EventRegistrationToken m_mediaCastingRenderLocationChangedToken;
    EventRegistrationToken m_naturalVideoSizeChangedToken;
    EventRegistrationToken m_mediaCurrentItemChangedToken;
    EventRegistrationToken m_mediaSourceChangedToken;
    EventRegistrationToken m_playbackStateChangedToken;
    EventRegistrationToken m_bufferingEndedToken;

    bool m_fRegisteredForUpdate;
    bool m_suspended;
    bool m_hasFirstFrameBasedOnState;
    CPowerModeRequestor m_powerModeRequestor;
    xref::weakref_ptr<CMediaPlayerElement> m_wpOwner;
};
