// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimedTextSource.h"
#include "MediaPlayerElement.g.h"
#include "MediaTransportControls.g.h"
#include "NamespaceAliases.h"
#include <Windows.Media.Playback.h>
#include <FrameworkUdk/MediaPlayerExtension.h>
#include <FrameworkUdk/TimedMetadataTrackExtension.h>
#include "Callback.h"
#include <WRLHelper.h>

using namespace DirectUI;

CTimedTextSource::CTimedTextSource() :
    m_cueRenderer(this)
{
    m_trackAddedEventToken.value = 0;
    m_mediaSourceChangedToken.value = 0;
    m_mediaCurrentItemChangedToken.value = 0;
    m_presentationModeChangedToken.value = 0;
}

CTimedTextSource::~CTimedTextSource()
{
    VERIFYHR(RemoveAllTrackCallbacks());
    VERIFYHR(RemoveMediaPlayerEventRegistrations());
    VERIFYHR(RemoveCuePresentationModeChangedCallback());
}

void CTimedTextSource::CleanupDeviceRelatedResources(const bool cleanupDComp)
{
    m_cueRenderer.CleanupDeviceRelatedResources(cleanupDComp);
}

_Check_return_ HRESULT
CTimedTextSource::OnTimedMetadataTracksChanged(_In_ wfc::IVectorChangedEventArgs* pArgs)
{
    IFCPTR_RETURN(pArgs);

    wfc::CollectionChange action;
    IFC_RETURN(pArgs->get_CollectionChange(&action));

    UINT index = 0;
    IFC_RETURN(pArgs->get_Index(&index));

    ctl::ComPtr<DependencyObject> spOwner;
    IFC_RETURN(m_wrOwner.As(&spOwner));
    if (spOwner.Get() && m_spCurrentItem.Get())
    {
        switch (action)
        {
            case wfc::CollectionChange_ItemInserted:
            {
                ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
                ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;

                IFC_RETURN(m_spCurrentItem->get_TimedMetadataTracks(&spTextTracks));
                IFC_RETURN(spTextTracks->GetAt(index, &spTrack));
                AddCueCallbacks(spTrack.Get(), index);
            }
            break;

            case wfc::CollectionChange_ItemRemoved:
            {
                ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
                ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;
                std::shared_ptr<CTrackTokens> deadItem;
                IFC_RETURN(m_spCurrentItem->get_TimedMetadataTracks(&spTextTracks));
                IFC_RETURN(spTextTracks->GetAt(index, &spTrack));

                for (auto& item : m_tokensList)
                {
                    if (item->Track == index)
                    {
                        if (item->EnteredToken.value != 0)
                        {
                            IFC_RETURN(TimedMetadataTrackExtension_RemoveCueEntered(spTrack.Get(), item->EnteredToken));
                            item->EnteredToken.value = 0;
                        }

                        if (item->ExitedToken.value != 0)
                        {
                            IFC_RETURN(spTrack->remove_CueExited(item->ExitedToken));
                            item->ExitedToken.value = 0;
                        }

                        item->spTrack.Reset();

                        deadItem = item;
                        break;
                    }
                }

                m_tokensList.remove(deadItem);
            }
            break;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::AddTrackCallback()
{
    ctl::WeakRefPtr wrThis;
    // We need to use a weak reference to avoid a circular reference.
    // Which will lead to CTimedTextSource leaking
    IFC_RETURN(ctl::ComPtr<IInspectable>(this).AsWeak(&wrThis));
    auto spTrackAddedHandler = Callback<wf::ITypedEventHandler<wmp::MediaPlaybackItem*, wfc::IVectorChangedEventArgs*>>(
        [wrThis, this](wmp::IMediaPlaybackItem* pItem, wfc::IVectorChangedEventArgs* pArgs) mutable -> HRESULT
    {
        auto spInspectable = wrThis.AsOrNull<IInspectable>();
        if (spInspectable.Get())
        {
            ctl::ComPtr<DependencyObject> spOwner;
            ctl::ComPtr<CTimedTextSource> spThis(this);
            IFC_RETURN(spThis->m_wrOwner.As(&spOwner));
            if (spOwner.Get())
            {
                ctl::ComPtr<IDispatcher> spDispatcher;
                if(SUCCEEDED(spOwner->GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                {
                    IFC_RETURN(spDispatcher->RunAsync(MakeCallback(
                        spThis, &CTimedTextSource::OnTimedMetadataTracksChanged, ctl::ComPtr<wfc::IVectorChangedEventArgs>(pArgs))));
                }
            }
        }
        return S_OK;
    });

    if (m_spCurrentItem)
    {
        IFC_RETURN(m_spCurrentItem->add_TimedMetadataTracksChanged(spTrackAddedHandler.Get(), &m_trackAddedEventToken));
    }

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::Initialize(_In_opt_ xaml::IDependencyObject* pOwner, _In_ xaml::IUIElement* pLayoutRoot)
{
    if (pOwner)
    {
        IFC_RETURN(ctl::AsWeak(pOwner, &m_wrOwner));
    }

    IFC_RETURN(m_cueRenderer.Initialize(pLayoutRoot));

    // NOTE: cues need to be added on the UI thread, this caches a DispatcherQueue
    // to be used when cue are received
    ctl::ComPtr<msy::IDispatcherQueueStatics> spDispatcherQueueStatics;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
        spDispatcherQueueStatics.ReleaseAndGetAddressOf()));

    // We won't always have a DispatcherQueue for the thread. For example, users of
    // XAMLPresenter that don't explicitly initialize a DispatcherQueue won't have one.
    // Therefore all uses of m_spDispatcherQueue must check for null, and in those scenarios
    // CC, Transport Controls, and Cue Text will not work.

    IFC_RETURN(spDispatcherQueueStatics->GetForCurrentThread(&m_spDispatcherQueue));

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::SetMediaPlayer(_In_opt_ wmp::IMediaPlayer* pMediaPlayer)
{
    IFC_RETURN(RemoveMediaPlayerEventRegistrations());

    m_spMediaPlaybackList.Reset();
    if (m_spCurrentItem)
    {
        IFC_RETURN(RemoveCuePresentationModeChangedCallback());
        m_spCurrentItem.Reset();
    }
    m_spMediaPlayer = pMediaPlayer;

    IFC_RETURN(Reset());

    IFC_RETURN(AddMediaPlayerEventRegistrations());

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::UpdateCurrentItem()
{
    IFC_RETURN(RemoveCuePresentationModeChangedCallback());

    auto spOwner = m_wrOwner.AsOrNull<DependencyObject>();
    if (m_spMediaPlayer.Get())
    {
        ctl::ComPtr<wmp::IMediaPlaybackItem> spPlaybackItem;
        if (SUCCEEDED(MediaPlayerExtension_GetCurrentMediaPlaybackItem(m_spMediaPlayer.Get(), &spPlaybackItem)) && spPlaybackItem.Get())
        {
            m_spCurrentItem = spPlaybackItem;
        }
    }

    if (m_spCurrentItem.Get())
    {
        ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
        unsigned int tracksCount = 0;
        IFC_RETURN(m_spCurrentItem->get_TimedMetadataTracks(&spTextTracks));
        IFC_RETURN(spTextTracks->get_Size(&tracksCount));
        for (unsigned int i = 0; i < tracksCount; i++)
        {
            ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;
            IFC_RETURN(spTextTracks->GetAt(i, &spTrack));
            IFC_RETURN(AddCueCallbacks(spTrack.Get(), i));
        }

        IFC_RETURN(AddTrackCallback());

        IFC_RETURN(AddCuePresentationModeChangedCallback());
    }

#if DBG
    if (m_spCurrentItem) // we should always register presentationModeChanged callback on the newly acquired playback item
    {
        ASSERT(m_presentationModeChangedToken.value != 0);
    }
#endif
    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::AddToFullWindowMediaRoot()
{
    return m_cueRenderer.AddToFullWindowMediaRoot();
}

_Check_return_ HRESULT
CTimedTextSource::RemoveFromFullWindowMediaRoot()
{
    return m_cueRenderer.RemoveFromFullWindowMediaRoot();
}

_Check_return_ HRESULT
CTimedTextSource::Create(
    _In_ xaml::IDependencyObject* pOwner,
    _In_ xaml::IUIElement* pLayoutRoot,
    _Outptr_ CTimedTextSource** ppTimedTextSource)
{
    ctl::ComPtr<CTimedTextSource> spTimedTextSource;

    IFCPTR_RETURN(ppTimedTextSource);
    *ppTimedTextSource = nullptr;

    IFC_RETURN(ctl::make<CTimedTextSource>(&spTimedTextSource));
    IFC_RETURN(spTimedTextSource->Initialize(pOwner, pLayoutRoot));

    *ppTimedTextSource = spTimedTextSource.Detach();

    return S_OK;
}


// Reset is called when a new playbackitem has been set (this is when a play list is used) or
// when a media opened event has occurred
_Check_return_ HRESULT
CTimedTextSource::Reset()
{
    // remove any existing callbacks for cues and tracks
    IFC_RETURN(RemoveAllTrackCallbacks());

    IFC_RETURN(UpdateCurrentItem());

    if (m_spCurrentItem.Get())
    {
        if (m_spDispatcherQueue)
        {
            boolean enqueued;
            // If timed text is not visible and nobody is listening for events then
            // cues will never be shown, no need to process them

            // Keep a strong ref on 'this' to prevent its destruction before the callback executes.
            ctl::ComPtr<CTimedTextSource> spThis(this);
            IFC_RETURN(m_spDispatcherQueue->TryEnqueue(
                WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([spThis]() mutable -> HRESULT
            {
                return spThis->m_cueRenderer.Reset();
            }).Get(), &enqueued));
            IFCEXPECT_RETURN(enqueued);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::OnCueEntered(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmc::IMediaCueEventArgs* pArgs)
{
    if (IsVisibleTrack(pTrack, m_spCurrentItem.Get()))
    {
        ctl::ComPtr<wmc::IMediaCue> spCue;
        IFC_RETURN(pArgs->get_Cue(&spCue));

        if (IsSupportedCue(spCue))
        {
            IFC_RETURN(m_cueRenderer.AddCue(spCue));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::OnCueExited(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmc::IMediaCueEventArgs* pArgs)
{
    if (IsVisibleTrack(pTrack, m_spCurrentItem.Get()))
    {
        ctl::ComPtr<wmc::IMediaCue> spCue;
        IFC_RETURN(pArgs->get_Cue(&spCue));

        if (IsSupportedCue(spCue))
        {
            IFC_RETURN(m_cueRenderer.RemoveCue(spCue));
        }
    }
    return S_OK;
}

// Support text cue and image cue with alpha mode premultiplied
bool CTimedTextSource::IsSupportedCue(_In_ const ctl::ComPtr<wmc::IMediaCue>& spCue) const
{
    ctl::ComPtr<wmc::ITimedTextCue> spTTCue;
    ctl::ComPtr<wmc::IImageCue> spImageCue;

    if (SUCCEEDED(spCue.As(&spTTCue)))
    {
        return true;
    }

    ctl::ComPtr<wgri::ISoftwareBitmap> spBitmap;
    if (SUCCEEDED(spCue.As(&spImageCue)) && SUCCEEDED(spImageCue->get_SoftwareBitmap(&spBitmap)))
    {
        wgri::BitmapAlphaMode alphaMode = wgri::BitmapAlphaMode_Ignore;
        if (SUCCEEDED(spBitmap->get_BitmapAlphaMode(&alphaMode)))
        {
            if (alphaMode == wgri::BitmapAlphaMode_Premultiplied)
                return true;
        }
    }

    return false;
}

_Check_return_ HRESULT
CTimedTextSource::AddCueCallbacks(wmc::ITimedMetadataTrack * pTrack, _In_ unsigned int trackIndex)
{
    ctl::WeakRefPtr wrThis;
    // We need to use a weak reference to avoid a circular reference.
    // Which will lead to CTimedTextSource leaking
    IFC_RETURN(ctl::ComPtr<IInspectable>(this).AsWeak(&wrThis));

    auto currentToken = std::make_shared<CTrackTokens>();
    currentToken->Track = trackIndex;

    auto spCueEnteredHandler = Callback<wf::ITypedEventHandler<wmc::TimedMetadataTrack*, wmc::MediaCueEventArgs*>>([wrThis, this](wmc::ITimedMetadataTrack* pTrack, wmc::IMediaCueEventArgs* pArgs) mutable -> HRESULT
    {
        auto spInspectable = wrThis.AsOrNull<IInspectable>();
        if (spInspectable.Get())
        {
            ctl::ComPtr<DependencyObject> spOwner;
            ctl::ComPtr<CTimedTextSource> spThis(this);
            IFC_RETURN(spThis->m_wrOwner.As(&spOwner));
            if (spOwner.Get())
            {
                ctl::ComPtr<IDispatcher> spDispatcher;
                if(SUCCEEDED(spOwner->GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                {
                    IFC_RETURN(spDispatcher->RunAsync(MakeCallback(
                        spThis, &CTimedTextSource::OnCueEntered, ctl::ComPtr<wmc::ITimedMetadataTrack>(pTrack), ctl::ComPtr<wmc::IMediaCueEventArgs>(pArgs))));
                }
            }
        }
        return S_OK;
    });

    auto spCueExitedHandler = Callback<wf::ITypedEventHandler<wmc::TimedMetadataTrack*, wmc::MediaCueEventArgs*>>([wrThis, this](wmc::ITimedMetadataTrack* pTrack, wmc::IMediaCueEventArgs* pArgs) mutable -> HRESULT
    {
        auto spInspectable = wrThis.AsOrNull<IInspectable>();
        if (spInspectable.Get())
        {
            ctl::ComPtr<DependencyObject> spOwner;
            ctl::ComPtr<CTimedTextSource> spThis(this);
            IFC_RETURN(spThis->m_wrOwner.As(&spOwner));
            if (spOwner.Get())
            {
                ctl::ComPtr<IDispatcher> spDispatcher;
                if(SUCCEEDED(spOwner->GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                {
                    IFC_RETURN(spDispatcher->RunAsync(MakeCallback(
                        spThis, &CTimedTextSource::OnCueExited, ctl::ComPtr<wmc::ITimedMetadataTrack>(pTrack), ctl::ComPtr<wmc::IMediaCueEventArgs>(pArgs))));
                }
            }
        }
        return S_OK;
    });

    // Register for the CueEntered extension event 
    // (will be called after the public CueEntered handlers to give app a chance to modify the cue styling prior to rendering)
    IFC_RETURN(TimedMetadataTrackExtension_AddCueEntered(pTrack, spCueEnteredHandler.Get(), &(currentToken->EnteredToken)));

    IFC_RETURN(pTrack->add_CueExited(spCueExitedHandler.Get(), &(currentToken->ExitedToken)));
    currentToken->spTrack = pTrack;

    m_tokensList.push_back(currentToken);

    return S_OK;
}


// Remove the cue events handlers
_Check_return_ HRESULT
CTimedTextSource::RemoveAllTrackCallbacks()
{
    if (m_spCurrentItem.Get())
    {
        if (m_trackAddedEventToken.value != 0)
        {
            IFC_RETURN(m_spCurrentItem->remove_TimedMetadataTracksChanged(m_trackAddedEventToken));
            m_trackAddedEventToken.value = 0;
        }
    }

    for (auto& item : m_tokensList)
    {
        if (item->spTrack.Get())
        {
            if (item->EnteredToken.value != 0)
            {
                IFC_RETURN(TimedMetadataTrackExtension_RemoveCueEntered(item->spTrack.Get(), item->EnteredToken));
                item->EnteredToken.value = 0;
            }
            if (item->ExitedToken.value != 0)
            {
                IFC_RETURN(item->spTrack->remove_CueExited(item->ExitedToken));
                item->ExitedToken.value = 0;
            }

            item->spTrack.Reset();
        }
    }

    m_tokensList.clear();
    return S_OK;
}

// Helper to determine if this is something I should render (i.e. subtitle or caption)
bool
CTimedTextSource::IsSupportedTrack(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmp::IMediaPlaybackItem* pCurrentItem)
{
    HRESULT hr = S_OK;

    bool isSupportedTrack = false;

    if (pTrack && pCurrentItem)
    {
        wmc::TimedMetadataKind trackKind = {};
        IFC(pTrack->get_TimedMetadataKind(&trackKind));
        isSupportedTrack = (trackKind == wmc::TimedMetadataKind_Subtitle || trackKind == wmc::TimedMetadataKind_Caption || trackKind == wmc::TimedMetadataKind_ImageSubtitle );
    }

Cleanup:
    return isSupportedTrack;
}

// Helper function to determine is a given track is currently visible and
// if it is something I should render (i.e. subtitle or caption)
bool
CTimedTextSource::IsVisibleTrack(_In_ wmc::ITimedMetadataTrack* pTrack, _In_ wmp::IMediaPlaybackItem* pCurrentItem)
{
    HRESULT hr = S_OK;

    bool isVisibleTrack = false;

    if (pTrack && pCurrentItem)
    {
        // Determine if this is a text track
        if (IsSupportedTrack(pTrack, pCurrentItem))
        {
            ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
            BOOLEAN wasFound = FALSE;
            unsigned int itemIndex = 0;

            // Determine if this track is visible
            IFC(pCurrentItem->get_TimedMetadataTracks(&spTextTracks));
            IFC(spTextTracks->IndexOf(pTrack, &itemIndex, &wasFound));

            if (wasFound)
            {
                ctl::ComPtr<wmp::IMediaPlaybackTimedMetadataTrackList> spTrackList;
                wmp::TimedMetadataTrackPresentationMode presentationMode;

                IFC(spTextTracks.As(&spTrackList));
                IFC(spTrackList->GetPresentationMode(itemIndex, &presentationMode));

                isVisibleTrack = (presentationMode == wmp::TimedMetadataTrackPresentationMode_PlatformPresented);
            }
        }
    }

Cleanup:
    return isVisibleTrack;
}

// When the MTC is shown we need to shift the CC text up above the MTC
// this is given the height of the MTC when shown and 0 when hidden
IFACEMETHODIMP
CTimedTextSource::SetMTCOffset(_In_ double offset)
{
    IFC_RETURN(m_cueRenderer.SetMTCOffset(offset));

    return S_OK;
}

// Clear all the timed text cues currently on the screen
void CTimedTextSource::ClearCues()
{
    m_cueRenderer.ClearCues();
}

// Clear all the timed text cues currently on the screen, then
// go back and add the active cues from the visible text tracks.
// This is called when the size of the video changes and requires
// all the text sizes and layout to be recalculated based
// on the new video size
IFACEMETHODIMP
CTimedTextSource::ResetActiveCues()
{
    HRESULT hr = S_OK;

    if (m_spCurrentItem)
    {
        ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
        unsigned int trackCount = {};

        ClearCues();
        m_cueRenderer.ResetParentSize();

        if (m_spDispatcherQueue)
        {

            IFC(m_spCurrentItem->get_TimedMetadataTracks(&spTextTracks));
            IFC(spTextTracks->get_Size(&trackCount));

            for (unsigned int i = 0; i < trackCount; i++)
            {
                ctl::ComPtr<wmc::ITimedMetadataTrack> spTrack;
                IFC(spTextTracks->GetAt(i, &spTrack));

                if (CTimedTextSource::IsSupportedTrack(spTrack.Get(), m_spCurrentItem.Get()))
                {
                    wmp::TimedMetadataTrackPresentationMode presentationMode;
                    ctl::ComPtr<wmp::IMediaPlaybackTimedMetadataTrackList> spTrackList;

                    IFC(spTextTracks.As(&spTrackList));
                    IFC(spTrackList->GetPresentationMode(i, &presentationMode));
                    if (presentationMode == wmp::TimedMetadataTrackPresentationMode_PlatformPresented)
                    {
                        ctl::ComPtr<wfc::IVectorView<wmc::IMediaCue*>> spActiveCues;
                        ctl::ComPtr<DependencyObject> spOwner;
                        unsigned int activeCueCount = {};

                        IFC(spTrack->get_ActiveCues(&spActiveCues));
                        IFC(spActiveCues->get_Size(&activeCueCount));

                        // For each active cue re-add the cue on the UI thread
                        // spOwner is to ensure the object stays alive for the
                        // cross thread call
                        IFC(m_wrOwner.As(&spOwner));
                        if (spOwner.Get())
                        {
                            for (unsigned int j = 0; j < activeCueCount; j++)
                            {
                                ctl::ComPtr<wmc::IMediaCue> spActiveMediaCue;
                                ctl::ComPtr<wmc::ITimedTextCue> spActiveTextCue;
                                ctl::ComPtr<wmc::IImageCue> spActiveImageCue;
                                boolean enqueued;

                                IFC(spActiveCues->GetAt(j, &spActiveMediaCue));
                                if (SUCCEEDED(spActiveMediaCue.As(&spActiveTextCue)) || SUCCEEDED(spActiveMediaCue.As(&spActiveImageCue)))
                                {
                                    // Keep a strong ref on 'this' to prevent its destruction before the callback executes.
                                    ctl::ComPtr<CTimedTextSource> spThis(this);
                                    IFC(m_spDispatcherQueue->TryEnqueue(
                                        WRLHelper::MakeAgileCallback<msy::IDispatcherQueueHandler>([spThis, spOwner, spActiveMediaCue]() mutable -> HRESULT
                                    {
                                        return spThis->m_cueRenderer.AddCue(spActiveMediaCue);
                                    }).Get(), &enqueued));
                                    IFCEXPECT(enqueued);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
CTimedTextSource::GetNaturalVideoSize(_Out_ INT32* pHeight, _Out_ INT32* pWidth)
{
    *pHeight = 0;
    *pWidth = 0;

    auto spOwner = m_wrOwner.AsOrNull<DependencyObject>();
    if (m_spMediaPlayer.Get())
    {
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
        ctl::ComPtr<wmp::IMediaPlaybackSession> spPlaybackSession;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer3));
        IFC_RETURN(spMediaPlayer3->get_PlaybackSession(&spPlaybackSession));

        UINT32 value = 0;
        if (FAILED(spPlaybackSession->get_NaturalVideoHeight(&value)))
        {
            *pHeight = 0;
        }
        else
        {
            *pHeight = static_cast<INT32>(value);
        }

        value = 0;
        if (FAILED(spPlaybackSession->get_NaturalVideoWidth(&value)))
        {
            *pWidth = 0;
        }
        else
        {
            *pWidth = static_cast<INT32>(value);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::get_IsFullWindow(_Out_ BOOLEAN* isFullWindow)
{
    *isFullWindow = FALSE;
    // For MediaPlayerElement, the entire layout grid is moved. So we dont need anything special here
    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::get_Stretch(_Out_ xaml_media::Stretch* stretch)
{
    auto spOwner = m_wrOwner.AsOrNull<DependencyObject>();
    if (spOwner.Get())
    {
        if (spOwner->GetTypeIndex() == KnownTypeIndex::MediaPlayerElement)
        {
            ctl::ComPtr<MediaPlayerElement> spOwnerMPE;
            IFC_RETURN(spOwner.As(&spOwnerMPE));
            IFC_RETURN(spOwnerMPE->get_Stretch(stretch));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::AddMediaPlayerEventRegistrations()
{
    if (m_spMediaPlayer.Get())
    {
        ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer3));
        ctl::WeakRefPtr wrThis;
        // We need to use a weak reference to avoid a circular reference.
        // Which will lead to CTimedTextSource leaking
        IFC_RETURN(ctl::ComPtr<IInspectable>(this).AsWeak(&wrThis));
        auto callback = wrl::Callback<
            Microsoft::WRL::Implements<
                Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                wf::ITypedEventHandler<wmp::MediaPlayer*, IInspectable*>,
                Microsoft::WRL::FtmBase>>([wrThis, this](wmp::IMediaPlayer*, IInspectable*) mutable -> HRESULT
                {
                    auto spInspectable = wrThis.AsOrNull<IInspectable>();
                    if (spInspectable.Get())
                    {
                        ctl::ComPtr<DependencyObject> spOwner;
                        ctl::ComPtr<CTimedTextSource> spThis(this);
                        IFC_RETURN(spThis->m_wrOwner.As(&spOwner));
                        if (spOwner.Get())
                        {
                            ctl::ComPtr<IDispatcher> spDispatcher;
                            if(SUCCEEDED(spOwner->GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                            {
                                IFC_RETURN(spDispatcher->RunAsync(MakeCallback(
                                    spThis, &CTimedTextSource::OnMediaPlayerSourceChanged)));
                            }
                        }
                    }
                    return S_OK;
                });
        IFC_RETURN(spMediaPlayer3->add_SourceChanged(callback.Get(), &m_mediaSourceChangedToken));

        IFC_RETURN(AddTrackEventListeners());
    }

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::AddTrackEventListeners()
{
    IFC_RETURN(RemoveTrackEventListeners());

    if (m_spMediaPlayer.Get())
    {
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

                if (m_spMediaPlaybackList.Get())
                {
                    ctl::WeakRefPtr wrThis;
                    // We need to use a weak reference to avoid a circular reference.
                    // Which will lead to CTimedTextSource leaking
                    IFC_RETURN(ctl::ComPtr<IInspectable>(this).AsWeak(&wrThis));
                    auto callback = wrl::Callback<
                        Microsoft::WRL::Implements<
                            Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                            wf::ITypedEventHandler<wmp::MediaPlaybackList*, wmp::CurrentMediaPlaybackItemChangedEventArgs*>,
                            Microsoft::WRL::FtmBase>>([wrThis, this](wmp::IMediaPlaybackList*, wmp::ICurrentMediaPlaybackItemChangedEventArgs*) mutable -> HRESULT
                            {
                                auto spInspectable = wrThis.AsOrNull<IInspectable>();
                                if (spInspectable.Get())
                                {
                                    ctl::ComPtr<DependencyObject> spOwner;
                                    ctl::ComPtr<CTimedTextSource> spThis(this);
                                    IFC_RETURN(spThis->m_wrOwner.As(&spOwner));
                                    if (spOwner.Get())
                                    {
                                        ctl::ComPtr<IDispatcher> spDispatcher;
                                        if(SUCCEEDED(spOwner->GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                                        {
                                            IFC_RETURN(spDispatcher->RunAsync(MakeCallback(
                                                spThis, &CTimedTextSource::OnMediaCurrentItemChanged)));
                                        }
                                    }
                                }
                                return S_OK;
                            });
                    IFC_RETURN(m_spMediaPlaybackList->add_CurrentItemChanged(callback.Get(), &m_mediaCurrentItemChangedToken));
                }
            }
        }
    }

    return S_OK;
}


_Check_return_ HRESULT
CTimedTextSource::RemoveMediaPlayerEventRegistrations()
{
    if (m_spMediaPlayer.Get())
    {
        if (m_mediaSourceChangedToken.value != 0)
        {
            ctl::ComPtr<wmp::IMediaPlayer3> spMediaPlayer3;
            IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayer3));
            IFC_RETURN(spMediaPlayer3->remove_SourceChanged(m_mediaSourceChangedToken));
            m_mediaSourceChangedToken.value = 0;
        }
    }

    IFC_RETURN(RemoveTrackEventListeners());

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::RemoveTrackEventListeners()
{
    if (m_spMediaPlaybackList.Get())
    {
        if (m_mediaCurrentItemChangedToken.value != 0)
        {
            IFC_RETURN(m_spMediaPlaybackList->remove_CurrentItemChanged(m_mediaCurrentItemChangedToken));
            m_mediaCurrentItemChangedToken.value = 0;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::AddCuePresentationModeChangedCallback()
{
    IFC_RETURN(RemoveCuePresentationModeChangedCallback());

    ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
    IFC_RETURN(m_spCurrentItem->get_TimedMetadataTracks(&spTextTracks));

    ctl::ComPtr<wmp::IMediaPlaybackTimedMetadataTrackList> spTrackList;
    IFC_RETURN(spTextTracks.As(&spTrackList));

    ctl::WeakRefPtr wrThis;
    // We need to use a weak reference to avoid a circular reference.
    // Which will lead to CTimedTextSource leaking
    IFC_RETURN(ctl::ComPtr<IInspectable>(this).AsWeak(&wrThis));
    auto callback = wrl::Callback<
        Microsoft::WRL::Implements<
        Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
        wf::ITypedEventHandler<wmp::MediaPlaybackTimedMetadataTrackList*, wmp::TimedMetadataPresentationModeChangedEventArgs*>,
        Microsoft::WRL::FtmBase>>([wrThis, this](
            wfc::IVectorView<wmc::TimedMetadataTrack*>*,
            wmp::ITimedMetadataPresentationModeChangedEventArgs* pArgs) mutable -> HRESULT
    {
        auto spInspectable = wrThis.AsOrNull<IInspectable>();
        if (spInspectable.Get())
        {
            ctl::ComPtr<CTimedTextSource> spThis(this);
            ctl::ComPtr<DependencyObject> spOwner;
            IFC_RETURN(spThis->m_wrOwner.As(&spOwner));
            if (spOwner.Get())
            {
                ctl::ComPtr<wmp::ITimedMetadataPresentationModeChangedEventArgs> spArgs(pArgs);
                ctl::ComPtr<IDispatcher> spDispatcher;
                if(SUCCEEDED(spOwner->GetXamlDispatcher(&spDispatcher)) && spDispatcher)
                {
                    IFC_RETURN(spDispatcher->RunAsync(MakeCallback(
                        spThis, &CTimedTextSource::OnCuePresentationModeChanged, spArgs)));
                }
            }
        }
        return S_OK;
    });

    IFC_RETURN(spTrackList->add_PresentationModeChanged(callback.Get(), &m_presentationModeChangedToken));

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::OnCuePresentationModeChanged(_In_ wmp::ITimedMetadataPresentationModeChangedEventArgs* pArgs)
{
    IFC_RETURN(m_cueRenderer.OnCuePresentationModeChangedCallback(pArgs));
    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::RemoveCuePresentationModeChangedCallback()
{
    if (m_presentationModeChangedToken.value != 0)
    {
        ctl::ComPtr<wfc::IVectorView<wmc::TimedMetadataTrack*>> spTextTracks;
        IFC_RETURN(m_spCurrentItem->get_TimedMetadataTracks(&spTextTracks));

        ctl::ComPtr<wmp::IMediaPlaybackTimedMetadataTrackList> spTrackList;
        IFC_RETURN(spTextTracks.As(&spTrackList));

        IFC_RETURN(spTrackList->remove_PresentationModeChanged(m_presentationModeChangedToken));
        m_presentationModeChangedToken.value = 0;
    }

    return S_OK;
}


_Check_return_ HRESULT
CTimedTextSource::OnMediaPlayerSourceChanged()
{
    IFC_RETURN(AddTrackEventListeners());

    IFC_RETURN(Reset());

    return S_OK;
}

_Check_return_ HRESULT
CTimedTextSource::OnMediaCurrentItemChanged()
{
    IFC_RETURN(Reset());

    return S_OK;
}

