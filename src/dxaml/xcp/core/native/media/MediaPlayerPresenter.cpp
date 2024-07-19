// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MediaPlayerPresenter.h"
#include "MediaPlayerElement.h"
#include <FrameworkUdk/MediaPlayerExtension.h>
#include <Windows.Media.Playback.h>
#include "CValueBoxer.h"
#include "HWWalk.h"
#include "MetadataAPI.h"
#include "AgileCallback.h"
#include "MediaPlayerExtensions.h"

#pragma warning(disable:4996) // use of apis marked as [[deprecated("PrivateAPI")]]

_Check_return_ HRESULT CMediaPlayerPresenter::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    if ( args.m_pDP->GetIndex() == KnownPropertyIndex::MediaPlayerPresenter_MediaPlayer )
    {
        IFC_RETURN(UpdateMediaPlayer());
    }
    else if ( args.m_pDP->GetIndex() == KnownPropertyIndex::MediaPlayerPresenter_IsFullWindow )
    {
        // Need to update power requests after entering Full Window mode
        IFC_RETURN(UpdatePowerSettings());
    }
    else
    {
        return CFrameworkElement::OnPropertyChanged(args);
    }

    return S_OK;
}

_Check_return_ HRESULT
CMediaPlayerPresenter::UpdateMediaPlayer()
{
    IFC_RETURN(RemoveEventRegistrations());

    m_hasFirstFrameBasedOnState = false;
    m_spPlaybackSession.Reset();
    m_spMediaPlaybackList.Reset();
    m_previousNormalizedRect = {};
    m_previousDestSize = {};
    ResetNaturalVideoSize();

    auto pDP = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::MediaPlayerPresenter_MediaPlayer);
    CValue value;
    IFC_RETURN(GetValue(pDP, &value));
    ctl::ComPtr<IInspectable> spValue;
    IFC_RETURN(DirectUI::CValueBoxer::UnboxObjectValue(&value, pDP->GetPropertyType(), &spValue));
    IFC_RETURN(spValue.As(&m_spMediaPlayer));

    if (m_spMediaPlayer.Get())
    {
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer3));
        IFC_RETURN(spMediaPlayer3->get_PlaybackSession(&m_spPlaybackSession));

        ctl::ComPtr<wmp::IMediaPlayerSource2> spMediaPlayerSource;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerSource));
        if (spMediaPlayerSource.Get())
        {
            ctl::ComPtr<wmp::IMediaPlaybackSource> spMediaPlaybackSource;
            IFC_RETURN(spMediaPlayerSource->get_Source(&spMediaPlaybackSource));
            if (spMediaPlaybackSource.Get())
            {
                ctl::ComPtr<wmp::IMediaPlaybackList> spMediaPlaybackList;
                if SUCCEEDED(spMediaPlaybackSource.As(&spMediaPlaybackList))
                {
                    m_spMediaPlaybackList = spMediaPlaybackList;
                }
            }
        }
    }

    IFC_RETURN(AddEventRegistrations());

    IFC_RETURN(UpdateNaturalBounds());

    IFC_RETURN(SetMediaPlayerSwapChain());

    IFC_RETURN(UpdatePowerSettings());

    return S_OK;
}

_Check_return_ HRESULT
CMediaPlayerPresenter::SetMediaPlayerSwapChain(_In_ HANDLE newHandle)
{
    HANDLE oldHandle = GetSwapChainHandle();

    m_mediaPlayerSwapChain.reset(newHandle);

    if (oldHandle == nullptr && m_mediaPlayerSwapChain.get() != nullptr)
    {
        IFC_RETURN(SetRequiresComposition(
            CompositionRequirement::SwapChainContent,
            IndependentAnimationType::None));
    }
    else if (oldHandle != nullptr && m_mediaPlayerSwapChain.get() == nullptr)
    {
        UnsetRequiresComposition(
            CompositionRequirement::SwapChainContent,
            IndependentAnimationType::None);
    }

    return S_OK;
}

_Check_return_ HRESULT
CMediaPlayerPresenter::SetMediaPlayerSwapChain()
{
    if (m_spMediaPlayer.Get() && m_spPlaybackSession.Get() && CanPresent())
    {
        HANDLE swapChainHandle = nullptr;

        IFC_RETURN(CalculateDCompParameters());
        IFC_RETURN(MediaPlayerExtension_GetVideoSwapchainHandle(m_spMediaPlayer.Get(), &swapChainHandle));
        IFC_RETURN(SetMediaPlayerSwapChain(swapChainHandle));
    }
    else
    {
        IFC_RETURN(SetMediaPlayerSwapChain(nullptr));
    }

    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::CalculateDCompParameters()
{
    if (CanPresent())
    {
        XRECTF_RB normalizedSourceRect = { };
        CWindowRenderTarget *pRenderTargetNoRef = GetContext()->NWGetWindowRenderTarget();
        XSIZE windowSize = {
            pRenderTargetNoRef->GetWidth(),
            pRenderTargetNoRef->GetHeight() };

        IFC_RETURN(HWWalk::CalculateMediaEngineAndDCompParameters(
            this,
            m_isFullWindow,
            &windowSize,
            &normalizedSourceRect,
            &m_destinationRect,
            &m_stretchTransform,
            &m_stretchClip));

        IFC_RETURN(UpdateVideoStream(normalizedSourceRect));
    }

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::UpdateVideoStream(_In_ const XRECTF_RB &normalizedSourceRect)
{
    // CalculateMediaEngineAndDCompParameters checked whether a video was available and returns an empty destination rect
    // if there is no video.
    if (m_destinationRect.Width > 0 && m_destinationRect.Height > 0)
    {
        //
        // Call UpdateVideoStream immediately to avoid producing frames of the wrong size. The swap chain
        // will be hooked up to the DComp tree later during the render walk, using the transform and clip calculated.
        //
        if (m_spMediaPlayer.Get())
        {
            bool hasSourceRect =
                normalizedSourceRect.left >= 0.0f
                || normalizedSourceRect.top >= 0.0f
                || normalizedSourceRect.right <= 1.0f
                || normalizedSourceRect.bottom <= 1.0f;

            if (hasSourceRect)
            {
                wf::Rect sourceRect = {
                    std::max(normalizedSourceRect.left, 0.0f),
                    std::max(normalizedSourceRect.top, 0.0f),
                    std::min(normalizedSourceRect.right - normalizedSourceRect.left, 1.0f),
                    std::min(normalizedSourceRect.bottom - normalizedSourceRect.top, 1.0f),
                };

                if (sourceRect.X != m_previousNormalizedRect.X ||
                    sourceRect.Y != m_previousNormalizedRect.Y ||
                    sourceRect.Width != m_previousNormalizedRect.Width ||
                    sourceRect.Height != m_previousNormalizedRect.Height)
                {
                    ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
                    IFC_RETURN(MediaPlayer_GetCurrentPlaybackSession(m_spMediaPlayer.Get(), &spPlaybackSession));

                    if (spPlaybackSession.Get())
                    {
                        IFC_RETURN(spPlaybackSession->put_NormalizedSourceRect(sourceRect));
                        m_previousNormalizedRect = sourceRect;
                    }
                }
            }
            if (m_destinationRect.Width != m_previousDestSize.Width || m_destinationRect.Height != m_previousDestSize.Height)
            {
                wf::Size destSize = { static_cast<float>(m_destinationRect.Width), static_cast<float>(m_destinationRect.Height) };

                ctl::ComPtr<wmp::IMediaPlayer4> spMediaPlayer4;
                IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer4));

                IFC_RETURN(spMediaPlayer4->SetSurfaceSize(destSize));

                CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
                m_previousDestSize = destSize;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CMediaPlayerPresenter::GetNaturalBounds(_Inout_ XRECT& pNaturalBounds)
{
    if(!HasVideo())
    {
        IFC_NOTRACE_RETURN(E_UNEXPECTED);
    }

    pNaturalBounds = {};
    pNaturalBounds.Height = static_cast<XINT32>(m_nNaturalVideoHeight);
    pNaturalBounds.Width = static_cast<XINT32>(m_nNaturalVideoWidth);

    return S_OK;
}

_Check_return_ HRESULT
CMediaPlayerPresenter::UpdateNaturalBounds()
{
    if (m_spPlaybackSession.Get())
    {
        if (FAILED(m_spPlaybackSession->get_NaturalVideoHeight(&m_nNaturalVideoHeight)))
        {
            m_nNaturalVideoHeight = 0;
        }
        if (FAILED(m_spPlaybackSession->get_NaturalVideoWidth(&m_nNaturalVideoWidth)))
        {
            m_nNaturalVideoWidth = 0;
        }
    }
    else
    {
        ResetNaturalVideoSize();
    }

    if (!CanPresent())
    {
        IFC_RETURN(SetMediaPlayerSwapChain(nullptr));
    }

    CFrameworkElement* pOwner = GetLayoutOwnerNoRef();
    pOwner->InvalidateArrange();
    pOwner->InvalidateMeasure();

    IFC_RETURN(UpdatePowerSettings());

    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::UpdateInternalSize(
        _Out_ float &rActualWidth,
        _Out_ float &rActualHeight,
        _Out_ DirectUI::AlignmentX &alignmentX,
        _Out_ DirectUI::AlignmentY &alignmentY)
{
    XFLOAT rImageWidth = 0.0f;
    XFLOAT rImageHeight = 0.0f;

    // If GetNaturalBounds returns an error, we'll use the zero-size
    // rectangle as bounds.
    XRECT naturalBounds = { 0 };
    if (SUCCEEDED(GetNaturalBounds(naturalBounds)))
    {
        rImageWidth = static_cast<XFLOAT>(naturalBounds.Width);
        rImageHeight = static_cast<XFLOAT>(naturalBounds.Height);
    }

    // Compute Actual Width & Height
    if (HasLayoutStorage())
    {
        // If Layout is present, use rendered size.
        rActualWidth = RenderSize.width;
        rActualHeight = RenderSize.height;
    }
    else
    {
        bool needsWidth = false;
        bool needsHeight = false;

        // Is Height or Width the default?
        if (IsDefaultWidth())
        {
            needsWidth = TRUE;
        }
        if (IsDefaultHeight())
        {
            needsHeight = TRUE;
        }

        // If Layout is not present, use given size if available.
        // If only height or width are given, compute the other using
        // aspect ratio. If given size is not available, use natural
        // size.

        rActualWidth = m_eWidth;
        rActualHeight = m_eHeight;

        if (needsWidth || needsHeight)
        {
            if (needsWidth ^ needsHeight)
            {
                // Only one dimension is specified

                //Compute natural aspect ratio
                XFLOAT rAspectRatio = 1.0f;
                if (rImageHeight > 0.0f)
                {
                    rAspectRatio = rImageWidth / rImageHeight;
                }

                // Compute other dimension using aspect ratio
                if (needsWidth)
                {
                    if (m_Stretch == DirectUI::Stretch::None)
                    {
                        rActualWidth = rImageWidth;
                    }
                    else
                    {
                        rActualWidth = m_eHeight * rAspectRatio;
                    }
                }
                else
                {
                    if (m_Stretch == DirectUI::Stretch::None)
                    {
                        rActualHeight = rImageHeight;
                    }
                    else
                    {
                        rActualHeight = m_eWidth / rAspectRatio;
                    }
                }
            }
            else
            {
                // Neither dimension is specified. Use natural size.
                rActualWidth = rImageWidth;
                rActualHeight = rImageHeight;
            }
        }

        // Clamp to max/min values
        rActualWidth =  MAX(MIN(rActualWidth, m_pLayoutProperties->m_eMaxWidth), m_pLayoutProperties->m_eMinWidth);
        rActualHeight = MAX(MIN(rActualHeight, m_pLayoutProperties->m_eMaxHeight), m_pLayoutProperties->m_eMinHeight);
    }

    // There is an odd behavior with Stretch=NONE. If the element is smaller than
    // the image natural dimensions, then use Top/Left alignment.  If it is larger
    // use Center/Center.
    alignmentX = (m_Stretch == DirectUI::Stretch::None && rActualWidth < rImageWidth)
        ? DirectUI::AlignmentX::Left : DirectUI::AlignmentX::Center;

    alignmentY = (m_Stretch == DirectUI::Stretch::None && rActualHeight < rImageHeight)
        ? DirectUI::AlignmentY::Top : DirectUI::AlignmentY::Center;

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::AddEventRegistrations()
{
    auto core = GetContext();

    if (m_spMediaPlayer.Get())
    {
        auto mediaOpenedCallback = DispatcherCallback<
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            CMediaPlayerPresenter>(this, &CMediaPlayerPresenter::OnMediaOpened);
        IFC_RETURN(m_spMediaPlayer->add_MediaOpened(mediaOpenedCallback.Get(), &m_mediaOpenedToken));

        auto castingLocationChangedCallback = DispatcherCallback<
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            CMediaPlayerPresenter>(this, &CMediaPlayerPresenter::OnMediaPlayerCastingLocationChanged);
        IFC_RETURN(MediaPlayerExtension_AddCastingRenderLocationChanged(m_spMediaPlayer.Get(), castingLocationChangedCallback.Get(), &m_mediaCastingRenderLocationChangedToken));

        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer3));
        auto sourceChangedCallback = DispatcherCallback<
            wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
            CMediaPlayerPresenter>(this, &CMediaPlayerPresenter::OnMediaPlayerSourceChanged);
        IFC_RETURN(spMediaPlayer3->add_SourceChanged(sourceChangedCallback.Get(), &m_mediaSourceChangedToken));
    }

    if (m_spMediaPlaybackList.Get())
    {
        auto callback = DispatcherCallback<
            wf::ITypedEventHandler<wmp::MediaPlaybackList*, wmp::CurrentMediaPlaybackItemChangedEventArgs*>,
            CMediaPlayerPresenter>(this, &CMediaPlayerPresenter::OnMediaCurrentItemChanged);
        IFC_RETURN(m_spMediaPlaybackList->add_CurrentItemChanged(callback.Get(), &m_mediaCurrentItemChangedToken));
    }

    if (m_spPlaybackSession.Get())
    {
        auto naturalVideoSizeCallback = DispatcherCallback<
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            CMediaPlayerPresenter>(this, &CMediaPlayerPresenter::OnNaturalVideoSizeChanged);
        IFC_RETURN(m_spPlaybackSession->add_NaturalVideoSizeChanged(naturalVideoSizeCallback.Get(), &m_naturalVideoSizeChangedToken));

        auto playbackStateChangedCallback = DispatcherCallback<
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            CMediaPlayerPresenter>(this, &CMediaPlayerPresenter::OnMediaPlaybackSessionPlaybackStateChanged);
        IFC_RETURN(m_spPlaybackSession->add_PlaybackStateChanged(playbackStateChangedCallback.Get(), &m_playbackStateChangedToken));

        auto bufferingEndedCallback = DispatcherCallback<
            wf::ITypedEventHandler<wmp::MediaPlaybackSession*, IInspectable*>,
            CMediaPlayerPresenter>(this, &CMediaPlayerPresenter::OnMediaPlaybackSessionBufferingEnded);
        IFC_RETURN(m_spPlaybackSession->add_BufferingEnded(bufferingEndedCallback.Get(), &m_bufferingEndedToken));
    }

    IFC_RETURN(core->RegisterPLMListener(this));

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::RemoveEventRegistrations()
{
    if (m_spMediaPlayer.Get())
    {
        if (m_mediaOpenedToken.value != 0)
        {
            IFC_RETURN(m_spMediaPlayer->remove_MediaOpened(m_mediaOpenedToken));
            m_mediaOpenedToken.value = 0;
        }
        if (m_mediaCastingRenderLocationChangedToken.value != 0)
        {
            IFC_RETURN(MediaPlayerExtension_RemoveCastingRenderLocationChanged(m_spMediaPlayer.Get(), m_mediaCastingRenderLocationChangedToken));
            m_mediaCastingRenderLocationChangedToken.value = 0;
        }
        if (m_mediaSourceChangedToken.value != 0)
        {
            ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
            IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer3));
            IFC_RETURN(spMediaPlayer3->remove_SourceChanged(m_mediaSourceChangedToken));
            m_mediaSourceChangedToken.value = 0;
        }
    }

    if (m_spMediaPlaybackList.Get())
    {
        if (m_mediaCurrentItemChangedToken.value != 0)
        {
            IFC_RETURN(m_spMediaPlaybackList->remove_CurrentItemChanged(m_mediaCurrentItemChangedToken));
            m_mediaCurrentItemChangedToken.value = 0;
        }
    }

    if(m_spPlaybackSession.Get())
    {
        if (m_naturalVideoSizeChangedToken.value != 0)
        {
            IFC_RETURN(m_spPlaybackSession->remove_NaturalVideoSizeChanged(m_naturalVideoSizeChangedToken));
            m_naturalVideoSizeChangedToken.value = 0;
        }

        if (m_playbackStateChangedToken.value != 0)
        {
            IFC_RETURN(m_spPlaybackSession->remove_PlaybackStateChanged(m_playbackStateChangedToken));
            m_playbackStateChangedToken.value = 0;
        }

        if (m_bufferingEndedToken.value != 0)
        {
            IFC_RETURN(m_spPlaybackSession->remove_BufferingEnded(m_bufferingEndedToken));
            m_bufferingEndedToken.value = 0;
        }
    }
    IFC_RETURN(GetContext()->UnregisterPLMListener(this));

    return S_OK;
}

__declspec(noinline) _Check_return_ HRESULT CMediaPlayerPresenter::OnMediaOpened(wmp::IMediaPlayer*, IInspectable*)
{
    IFC_RETURN(UpdateNaturalBounds());
    IFC_RETURN(UpdateRenderingAndPowerSettings());
    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::OnNaturalVideoSizeChanged(_In_ wmp::IMediaPlaybackSession*, _In_ IInspectable*)
{
    IFC_RETURN(UpdateNaturalBounds());
    IFC_RETURN(SetMediaPlayerSwapChain());

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::OnMediaCurrentItemChanged(_In_ wmp::IMediaPlaybackList*, _In_ wmp::ICurrentMediaPlaybackItemChangedEventArgs*)
{
    IFC_RETURN(UpdateNaturalBounds());

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::OnMediaPlayerSourceChanged(_In_ wmp::IMediaPlayer*, _In_ IInspectable*)
{
    m_hasFirstFrameBasedOnState = false;
    IFC_RETURN(UpdateMediaPlayer());

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params)
{
    IFC_RETURN(CFrameworkElement::EnterImpl(pNamescopeOwner, params));
    if(params.fIsLive)
    {
        IFC_RETURN(RegisterForUpdate());
    }

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CFrameworkElement::LeaveImpl(pNamescopeOwner, params));

    // Only stop requesting update callbacks when truly leaving the tree. In the case
    // that this element becomes part of a popup it will get a non-live leave of the
    // tree and then a non-live enter.
    if(params.fIsLive)
    {
        IFC_RETURN(UnregisterForUpdate());
    }

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::RegisterForUpdate()
{
    //register the media element for update so it can update its dirty state before rendering
    if(!m_fRegisteredForUpdate)
    {
        IFC_RETURN(GetContext()->AddChildForUpdate(this));
        m_fRegisteredForUpdate = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::UnregisterForUpdate()
{
    //register the media element for update so it can update its dirty state before rendering
    if(m_fRegisteredForUpdate)
    {
        IFC_RETURN(GetContext()->RemoveChildForUpdate(this));
        m_fRegisteredForUpdate = false;
    }

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::UpdateState()
{
    HRESULT hr = S_OK;

    if (GetSwapChainHandle() == nullptr)
    {
        IFC(SetMediaPlayerSwapChain());
    }
    else
    {
        IFC(CalculateDCompParameters());
    }

Cleanup:
    return HandleError(hr);
}

_Check_return_ HRESULT CMediaPlayerPresenter::HandleError(HRESULT hr)
{
    if (FAILED(hr))
    {
        if (HasVideo())
        {
            ResetNaturalVideoSize();
        }

        IFC_RETURN(SetMediaPlayerSwapChain(nullptr));

        return S_OK;
    }

    return hr;
}

bool CMediaPlayerPresenter::HasFirstFrameBasedOnState()
{
    // Evaluate heuristic for first-frame-ready by examining PlaybackState. 
    // Do not re-evaluate once readiness is confirmed to avoid blinking poster image in later stage of playback
    if (!m_hasFirstFrameBasedOnState && m_spMediaPlayer.Get() && m_spPlaybackSession.Get())
    {
        // We assume first frame is ready if PlaybackState is Playing or Paused. 
        // Note that initial buffering (immediately following Opening) does not imply first frame readiness
        //  public enum MediaPlaybackState
        //  {
        //     None = 0,
        //     Opening = 1,
        //     Buffering = 2,
        //     Playing = 3,
        //     Paused = 4,
        //  }
        wmp::MediaPlaybackState playbackState{ wmp::MediaPlaybackState::MediaPlaybackState_None };
        VERIFYHR(m_spPlaybackSession->get_PlaybackState(&playbackState));

        m_hasFirstFrameBasedOnState = 
            (playbackState == wmp::MediaPlaybackState::MediaPlaybackState_Playing ||
             playbackState == wmp::MediaPlaybackState::MediaPlaybackState_Paused);
    }

    return m_hasFirstFrameBasedOnState;
}

__declspec(noinline) _Check_return_ HRESULT CMediaPlayerPresenter::OnMediaPlaybackSessionBufferingEnded(_In_ wmp::IMediaPlaybackSession*, _In_ IInspectable*)
{
    IFC_RETURN(UpdateRenderingAndPowerSettings());
    return S_OK;
}

__declspec(noinline) _Check_return_ HRESULT CMediaPlayerPresenter::OnMediaPlaybackSessionPlaybackStateChanged(_In_ wmp::IMediaPlaybackSession*, _In_ IInspectable*)
{
    IFC_RETURN(UpdateRenderingAndPowerSettings());
    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::OnMediaPlayerCastingLocationChanged(_In_ wmp::IMediaPlayer*, _In_ IInspectable*)
{
    IFC_RETURN(UpdatePowerSettings());
    return S_OK;
}


_Check_return_ HRESULT CMediaPlayerPresenter::UpdateRenderingAndPowerSettings()
{
    IFC_RETURN(SetMediaPlayerSwapChain());
    IFC_RETURN(UpdatePowerSettings());

    return S_OK;
}

HRESULT CMediaPlayerPresenter::UpdatePowerSettings()
{
    bool mediaPlaying = false;
    MediaPlayerExtension_MediaPlayerCastingRenderLocation castingRenderLocation = MediaPlayerExtension_MediaPlayerCastingRenderLocation::None;

    if (m_spMediaPlayer.Get())
    {
        wmp::MediaPlayerState mediaState = {};
        IFC_RETURN(m_spMediaPlayer->get_CurrentState(&mediaState));
        mediaPlaying =
            (mediaState == wmp::MediaPlayerState_Playing ||
             mediaState == wmp::MediaPlayerState_Buffering);
        if (mediaPlaying)
        {
            IFC_RETURN(MediaPlayerExtension_GetMediaPlayerCastingRenderLocation(m_spMediaPlayer.Get(), &castingRenderLocation));
        }
    }

    // disable power saving when playing full window video on local
    const bool shouldDisablePowerSaving =
        m_spMediaPlayer.Get() != nullptr &&
        !m_suspended &&
        m_isFullWindow &&
        mediaPlaying &&     // This includes Buffering state, assuming it's a short-lived precursor to Playing... is this true in general?
        (castingRenderLocation == MediaPlayerExtension_MediaPlayerCastingRenderLocation::None ||
         castingRenderLocation == MediaPlayerExtension_MediaPlayerCastingRenderLocation::RenderLocal) &&
        HasVideo();

    // Disable/Restore will no-op if power saving state is already correct
    if (shouldDisablePowerSaving)
    {
        m_powerModeRequestor.DisablePowerSaving();
    }
    else
    {
        m_powerModeRequestor.RestorePowerSaving();
    }

    return S_OK;
}


// IPLMListener
_Check_return_ HRESULT CMediaPlayerPresenter::OnSuspend(_In_ bool isTriggeredByResourceTimer)
{
    m_suspended = true;
    VERIFYHR(UpdatePowerSettings());

    return S_OK;
}

_Check_return_ HRESULT CMediaPlayerPresenter::OnResume()
{
    m_suspended = false;
    VERIFYHR(UpdatePowerSettings());
    return S_OK;
}

void CMediaPlayerPresenter::SetOwner(_In_ CMediaPlayerElement* pOwner)
{
    m_wpOwner = xref::weakref_ptr<CMediaPlayerElement>(pOwner);
}

float GetScaledOtherDimension(
    _In_ const float scaledOneDimension,
    _In_ const XUINT32 naturalOneDimension,
    _In_ const XUINT32 naturalOtherDimension)
{
    //
    // naturalOneDimension is mapped to scaledOneDimension. Map naturalOtherDimension to scaledOtherDimension using the
    // same scale factor.
    //
    // scaledOther / naturalOther = scaledOne / naturalOne
    //                scaledOther = naturalOther * scaledOne / naturalOne
    //

    if (naturalOneDimension == 0)
    {
        return 0.0f;
    }
    else
    {
        return static_cast<float>(naturalOtherDimension) * scaledOneDimension / static_cast<float>(naturalOneDimension);
    }
}

////////
//
// The Presenter can be used in 3 ways, each affect how layout is performed
//
//  1. Using the Presenter directly.
//      In this case the MPP behaves as any other element with size.
//      Its MEdiaPlayerElement owner is null.
//       _________________
//      | MPP             |
//      |                 |
//      |                 |     <---- The layout owner is the MPP
//      |                 |
//      |                 |
//      |_________________|
//
//  2. Using the Presenter owner by a MediaPlayerElement in NON full window mode
//      In this case the MPP is owned by the MediaPlayerElement
//      But the size of the MPE is subject to the size of the MPP if it has video
//       _________________
//      | MPE             |
//      |  ______________ |
//      | |              ||     <---- The layout owner is the MPE
//      | |     MPP      ||
//      | |______________||
//      |_________________|
//
//  3. Using the Presenter owner by a MediaPlayerElement in full window mode
//      In this case the MPP is reparented to the FullScreenMediaRoot
//       _____________________
//      | FullScreenMediaRoot |
//      |  _______________    |
//      | |               |   |     <---- The layout owner is the MPP
//      | |     MPP       |   |
//      | |_______________|   |
//      |_____________________|
//
//
//////
CFrameworkElement* CMediaPlayerPresenter::GetLayoutOwnerNoRef()
{
    CMediaPlayerElement* pOwner = m_wpOwner.lock();
    if(pOwner && !m_isFullWindow)
    {
        return pOwner;
    }
    else
    {
        return this;
    }
}

_Check_return_ HRESULT CMediaPlayerPresenter::MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize)
{
    PreMeasure(availableSize, desiredSize);

    return S_OK;
}

void CMediaPlayerPresenter::PreMeasure(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize)
{
    CFrameworkElement* pLayoutOwner = GetLayoutOwnerNoRef();
    const float explicitWidth = pLayoutOwner->IsDefaultWidth() ? -1.0f : pLayoutOwner->GetSpecifiedWidth();
    const float explicitHeight = pLayoutOwner->IsDefaultHeight() ? -1.0f : pLayoutOwner->GetSpecifiedHeight();

    if (explicitWidth >= 0 && explicitHeight >= 0)
    {
        desiredSize.width = explicitWidth;
        desiredSize.height = explicitHeight;
    }
    else
    {
        //
        // When determining the layout size:
        //
        //   1. If the explicit size is provided, then use that.
        //
        //   2. If the explicit size is only provided in one dimension, then use the explicit size in that dimension
        //      and infer the other from the natural size of the media.
        //
        //      Note that if the media is not ready yet, then we use 0x0.
        //
        //   3. If neither dimension is provided and the stretch is None, then use the natural size of the media.
        //
        //   4. If neither dimension is provided, the stretch isn't None, and the available size is finite, then use
        //      the available size.
        //
        //   5. If neither dimension is provided, the stretch isn't None, and the available size is infinite in one
        //      dimension, then fill the available area in one dimension and infer the other from the natural size
        //      of the media.
        //
        //   6. If neither dimension is provided, the stretch isn't None, and the available size is infinite in both
        //      dimensions, use the natural size of the media.
        //

        if (explicitWidth >= 0.0f)
        {
            desiredSize.width = explicitWidth;
            desiredSize.height = GetScaledOtherDimension(explicitWidth, m_nNaturalVideoWidth, m_nNaturalVideoHeight);
        }
        else if (explicitHeight >= 0.0f)
        {
            desiredSize.width = GetScaledOtherDimension(explicitHeight, m_nNaturalVideoHeight, m_nNaturalVideoWidth);
            desiredSize.height = explicitHeight;
        }
        else if (m_Stretch == DirectUI::Stretch::None)
        {
            desiredSize.width = static_cast<float>(m_nNaturalVideoWidth);
            desiredSize.height = static_cast<float>(m_nNaturalVideoHeight);
        }
        else
        {
            const bool isFiniteWidth = !IsInfiniteF(availableSize.width);
            const bool isFiniteHeight = !IsInfiniteF(availableSize.height);

            if (isFiniteWidth && isFiniteHeight)
            {
                desiredSize = availableSize;
            }
            else if (isFiniteWidth)
            {
                desiredSize.width = availableSize.width;
                desiredSize.height = GetScaledOtherDimension(availableSize.width, m_nNaturalVideoWidth, m_nNaturalVideoHeight);
            }
            else if (isFiniteHeight)
            {
                desiredSize.width = GetScaledOtherDimension(availableSize.height, m_nNaturalVideoHeight, m_nNaturalVideoWidth);
                desiredSize.height = availableSize.height;
            }
            else
            {
                desiredSize.width = static_cast<float>(m_nNaturalVideoWidth);
                desiredSize.height = static_cast<float>(m_nNaturalVideoHeight);
            }
        }
    }
}
