// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MediaPlayerElement_partial.h"
#include "MediaPlayerPresenter_partial.h"
#include "MediaPlayerElement.h"
#include "MediaPlayerPresenter.h"
#include "MediaTransportControls_partial.h"
#include "Panel_partial.h"
#include "ContentPresenter_partial.h"
#include "Window.g.h"
#include "FocusMgr.h"
#include "VisualTreeHelper.h"
#include "Callback.h"
#include "theming\inc\Theme.h"
#include "XamlRoot.g.h"
#include <wininet.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

MediaPlayerElement::MediaPlayerElement()
    :m_bInit(false),
    m_bOwnsMediaPlayer(false)
{
}

MediaPlayerElement::~MediaPlayerElement()
{
    ctl::ComPtr<wmp::IMediaPlayer> spOldMediaPlayer(m_spMediaPlayer);
    m_spMediaPlayer.Reset();

    VERIFYHR(UpdateTimedTextSource()); // We need to clear the events in timed text source
    VERIFYHR(CloseMediaPlayer(spOldMediaPlayer.Get()));
}

_Check_return_ HRESULT MediaPlayerElement::get_AutoPlayImpl(_Out_ BOOLEAN* pValue)
{
    if (m_spMediaPlayer.Get() == nullptr)
    {
        IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_AutoPlay, pValue));
    }
    else
    {
        IFC_RETURN(m_spMediaPlayer->get_AutoPlay(pValue));
    }
    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::put_AutoPlayImpl(_In_ BOOLEAN value)
{
    return (SetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_AutoPlay, value));
}

_Check_return_ HRESULT
MediaPlayerElement::get_SourceImpl(_Outptr_result_maybenull_ wmp::IMediaPlaybackSource** ppValue)
{
    if (m_spMediaPlayer.Get() == nullptr)
    {
        IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_Source, ppValue));
    }
    else
    {
        ctl::ComPtr<wmp::IMediaPlayerSource2> spMediaPlayerSource;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerSource));
        IFC_RETURN(spMediaPlayerSource->get_Source(ppValue));
    }

    return S_OK;
}

// MF does not support ms-appx URIs when running unpackaged. In this case, convert to file:/// and return in ppFileUri. 
_Check_return_ HRESULT
MediaPlayerElement::ResolveLocalSourceUri(_In_ wf::IUriRuntimeClass* pUri, _Outptr_result_maybenull_ wf::IUriRuntimeClass **ppFileUri)
{
    // If nullptr is returned at exit, no conversion was needed and pUri should be used directly.
    *ppFileUri = nullptr;
    
    const bool isProcessPackaged = gps->IsProcessPackaged();
    if (!isProcessPackaged)
    {
        wrl_wrappers::HString strSchemeName;
        INT nResult = 0;
        IFC_RETURN(pUri->get_SchemeName(strSchemeName.GetAddressOf()));

        // Support "ms-appx" scheme in unpackaged apps by converting to file , convert to file:/// 
        IFC_RETURN(WindowsCompareStringOrdinal(strSchemeName.Get(), wrl_wrappers::HStringReference(L"ms-appx").Get(), &nResult));
        
        // Convert unpackaged "ms-appx" URI's to file:/// before passing to MPE
        if (nResult == 0)
        {
            WCHAR applicationDirectory[MAX_PATH] = {0};
            WCHAR baseUri[INTERNET_MAX_URL_LENGTH + 1] = {0};
            DWORD baseUriLength = INTERNET_MAX_URL_LENGTH;
            DWORD result = 0;
            wrl_wrappers::HString strPath;
            wrl_wrappers::HString strRelativeUri;
            wrl_wrappers::HString strBaseUri;

            // Get Relative URI
            IFC_RETURN(pUri->get_Path(strPath.GetAddressOf()));
            // Remove initial '/' from Path so it is combined correctly with BaseURI in (CreateWithRelativeUri)
            IFC_RETURN(WindowsTrimStringStart(strPath.Get(), wrl_wrappers::HStringReference(L"/").Get(), strRelativeUri.GetAddressOf()));

            // Get Base URI (this follows the pattern in CommonResourceProvider::Create)
            result = ::GetModuleFileName(nullptr, applicationDirectory, MAX_PATH);
            if (result == 0)
            {
                IFC_RETURN(HRESULT_FROM_WIN32(GetLastError()));
            }

            IFC_RETURN(::UrlCreateFromPath(applicationDirectory, baseUri, &baseUriLength, NULL));
            IFC_RETURN(WindowsCreateString(baseUri, baseUriLength, strBaseUri.GetAddressOf()));            

            // Combine Base + Relative to get file:/// URI
            ctl::ComPtr<wf::IUriRuntimeClassFactory> spUriFactory;
            IFC_RETURN(ctl::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_Uri).Get(), &spUriFactory));
            IFC_RETURN(spUriFactory->CreateWithRelativeUri(strBaseUri.Get(), strRelativeUri.Get(), ppFileUri));
        }
    }
    
    return S_OK;
}


_Check_return_ HRESULT
MediaPlayerElement::put_SourceImpl(_In_opt_ wmp::IMediaPlaybackSource* pValue)
{
    return (SetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_Source, pValue));
}

_Check_return_ HRESULT
MediaPlayerElement::get_TransportControlsImpl(_Outptr_result_maybenull_ xaml_controls::IMediaTransportControls** ppMediaTransportControl)
{
    return (GetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_TransportControls, ppMediaTransportControl));
}

_Check_return_ HRESULT
MediaPlayerElement::put_TransportControlsImpl(_In_opt_ xaml_controls::IMediaTransportControls* pMediaTransportControl)
{
    ctl::ComPtr<IMediaTransportControls> spOldTransportControls;

    IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_TransportControls, &spOldTransportControls));
    // Before setting new TransportControls, Make sure disable the Old TransportControls so that this will remove from old MTC
    // from visual tree which results release of the old MTC object reference.
    if (spOldTransportControls && spOldTransportControls.Get() != pMediaTransportControl)
    {
        // if (spOldTransportControls.Cast<MediaTransportControls>()->GetIsEnabled() == TRUE)
        ctl::ComPtr<MediaTransportControls> spMTC = 
            static_cast<MediaTransportControls*>(ctl::impl_cast<MediaTransportControlsGenerated>(spOldTransportControls.Get()));

        if (spMTC->GetIsEnabled() == TRUE)
        {
            IFC_RETURN(spMTC->Disable());
        }
    }
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_TransportControls, pMediaTransportControl));

    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::SetMediaPlayerImpl(_In_ wmp::IMediaPlayer* pMediaPlayer)
{
    ctl::ComPtr<wmp::IMediaPlayer> spOldMediaPlayer(m_spMediaPlayer.Get());
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_MediaPlayer, pMediaPlayer));
    IFC_RETURN(CloseMediaPlayer(spOldMediaPlayer.Get()));
    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::PrepareState()
{
    ctl::ComPtr<MediaTransportControls> spMediaTransportControls;
    ctl::ComPtr<IMediaTransportControls> spIMTC;
    ctl::ComPtr<IMediaTransportControls> spOldMediaTransportControls;

    IFC_RETURN(MediaPlayerElementGenerated::PrepareState());

    IFC_RETURN(get_TransportControls(&spOldMediaTransportControls));
    // Check if already MediaTransports defined in XAML defintions.
    if (!spOldMediaTransportControls)
    {
        IFC_RETURN(MediaTransportControls::Create(&spMediaTransportControls));
        ASSERT(spMediaTransportControls.Get());
        IFC_RETURN(spMediaTransportControls.As(&spIMTC));
        IFC_RETURN(put_TransportControls(spIMTC.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::UpdateTransportControlsState()
{
    ctl::ComPtr<IMediaTransportControls> spIMTC;
    IFC_RETURN(get_TransportControls(&spIMTC));
    if (spIMTC.Get() != nullptr)
    {
        ctl::ComPtr<MediaTransportControls> spMTC = 
            static_cast<MediaTransportControls*>(ctl::impl_cast<MediaTransportControlsGenerated>(spIMTC.Get()));

        IFC_RETURN(spMTC->InitializeTransportControls(this));

        BOOLEAN bIsEnabled = spMTC->GetIsEnabled();
        BOOLEAN showTransportControls = FALSE;
        IFC_RETURN(get_AreTransportControlsEnabled(&showTransportControls));

        if (showTransportControls)
        {
            // If controls are already enabled, this is a no-op.
            if (!bIsEnabled)
            {
                IFC_RETURN(spMTC->Enable());
            }
            IFC_RETURN(spMTC->put_Visibility(xaml::Visibility_Visible));
        }
        else
        {
            if (bIsEnabled)
            {
                IFC_RETURN(spMTC->Disable());
            }
            IFC_RETURN(spMTC->put_Visibility(xaml::Visibility_Collapsed));
        }
    }
    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(MediaPlayerElementGenerated::OnPropertyChanged2(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::MediaPlayerElement_TransportControls:
        {
            IFC_RETURN(UpdateTransportControlsPresenter());
        }
        case KnownPropertyIndex::MediaPlayerElement_AreTransportControlsEnabled:
        {
            IFC_RETURN(UpdateTransportControlsState());

            ctl::ComPtr<IMediaTransportControls> spIMTC;
            IFC_RETURN(get_TransportControls(&spIMTC));

            if (spIMTC.Get())
            {
                ctl::ComPtr<MediaTransportControls> spMTC = 
                    static_cast<MediaTransportControls*>(ctl::impl_cast<MediaTransportControlsGenerated>(spIMTC.Get()));
                spMTC->OnOwnerMPEPropertyChanged(args.m_pDP->GetIndex());
            }
            break;
        }
#if false // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaPlayerElement_IsFullWindow:
        {
            IFC_RETURN(UpdateIsFullWindow());
            // Fall through to next case
        }
#endif // DISABLE_FULL_WINDOW
        case KnownPropertyIndex::MediaPlayerElement_Stretch:
        case KnownPropertyIndex::MediaPlayerElement_MediaPlayer:
        {
            if (args.m_pDP->GetIndex() == KnownPropertyIndex::MediaPlayerElement_MediaPlayer)
            {
                IFC_RETURN(get_MediaPlayer(&m_spMediaPlayer));
                IFC_RETURN(UpdateTransportControlsState());
                IFC_RETURN(UpdateAutoPlay());
                IFC_RETURN(UpdateSource());
                IFC_RETURN(UpdateTimedTextSource());
            }

            if (args.m_pDP->GetIndex() == KnownPropertyIndex::MediaPlayerElement_Stretch &&
                m_spTimedTextSource.Get()!=nullptr)
            {
                m_spTimedTextSource->ResetActiveCues();
            }

            ctl::ComPtr<IMediaTransportControls> spIMTC;
            IFC_RETURN(get_TransportControls(&spIMTC));
            if (spIMTC.Get())
            {
                ctl::ComPtr<MediaTransportControls> spMTC = 
                    static_cast<MediaTransportControls*>(ctl::impl_cast<MediaTransportControlsGenerated>(spIMTC.Get()));
                spMTC->OnOwnerMPEPropertyChanged(args.m_pDP->GetIndex());

            }

            break;
        }

        case KnownPropertyIndex::MediaPlayerElement_AutoPlay:
        {
            if (!m_bInit && m_spMediaPlayer.Get() == nullptr)
            {
                IFC_RETURN(CreateDefaultMediaPlayer());
            }
            else
            {
                IFC_RETURN(UpdateAutoPlay());
            }
            break;
        }

        case KnownPropertyIndex::MediaPlayerElement_Source:
        {
            if (!m_bInit && m_spMediaPlayer.Get() == nullptr)
            {
                IFC_RETURN(CreateDefaultMediaPlayer());
            }
            else
            {
                IFC_RETURN(UpdateSource());
            }
            break;
        }

        case KnownPropertyIndex::MediaPlayerElement_PosterSource:
        {
            UpdatePosterImageVisibility();
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::OnApplyTemplate()
{
    IFC_RETURN(MediaPlayerElementGenerated::OnApplyTemplate());

    CMediaPlayerElement* pElement = static_cast<CMediaPlayerElement*>(GetHandle());
    pElement->MarkTemplateApplied();
    if (m_tpMediaPlayerPresenterPart.Get())
    {
        ctl::ComPtr<MediaPlayerPresenter> spDxamlPresenter;
        IFC_RETURN(m_tpMediaPlayerPresenterPart.As(&spDxamlPresenter));
        CMediaPlayerPresenter* pPresenterNoRef = static_cast<CMediaPlayerPresenter*>(spDxamlPresenter.Get()->GetHandle());
        pElement->SetPresenter(pPresenterNoRef);
    }

    IFC_RETURN(UpdateTransportControlsPresenter());
    IFC_RETURN(UpdateTimedTextSource());
#if false // DISABLE_FULL_WINDOW
    IFC_RETURN(UpdateIsFullWindow());
#endif // DISABLE_FULL_WINDOW
    IFC_RETURN(UpdatePosterImageVisibility());

    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::UpdateTransportControlsPresenter()
{
    if (m_tpTransportControlsPresenterPart.Get())
    {
        ctl::ComPtr<IMediaTransportControls> pMTC;
        IFC_RETURN(get_TransportControls(&pMTC));
        IFC_RETURN(m_tpTransportControlsPresenterPart->put_Content(pMTC.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::UpdateIsFullWindow()
{
#if false // DISABLE_FULL_WINDOW
    BOOLEAN isFullWindow = FALSE;
    IFC_RETURN(get_IsFullWindow(&isFullWindow));
    if (isFullWindow)
    {
        return AddToFullWindowMediaRoot();
    }
    else
    {
        return RemoveFromFullWindowMediaRoot();
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

_Check_return_ HRESULT ContainsChild(
    _In_ wfc::IVector<xaml::UIElement*>* pChildren,
    _In_ xaml::IUIElement* pChild,
    _Out_ int& index)
{
    index = -1;
    unsigned int childrenSize = 0;
    IFC_RETURN(pChildren->get_Size(&childrenSize));

    for (unsigned int i = 0; i < childrenSize; i++)
    {
        ctl::ComPtr<xaml::IUIElement> spUIE;
        IFC_RETURN(pChildren->GetAt(i, &spUIE));
        if (spUIE.Get() == pChild)
        {
            index = static_cast<int>(i);
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::SetFocusAfterEnteringFullWindowMode()
{
#if false // DISABLE_FULL_WINDOW
    CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this->GetHandle());
    if (pFocusManager)
    {
        // We need to ensure to not leave the focus on an item behind the full window media root
        pFocusManager->ClearFocus();
        // if MTC is enable then focus set by MTC
        BOOLEAN showTransportControls = FALSE;
        IFC_RETURN(get_AreTransportControlsEnabled(&showTransportControls));
        if (showTransportControls)
        {
            ctl::ComPtr<IMediaTransportControls> spMediaTransportControls;
            IFC_RETURN(get_TransportControls(&spMediaTransportControls));
            if (spMediaTransportControls)
            {
                // Set the focus on the PlayPause button on MTC
                IFC_RETURN(spMediaTransportControls.Cast<MediaTransportControls>()->SetFocusAfterEnteringFullWindowMode());
            }
        }
        else
        {
            IFC_RETURN(pFocusManager->SetFocusOnNextFocusableElement(
                DirectUI::FocusState::Programmatic,
                true /*shouldFireFocusedRemoved*/));
        }
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::AddToFullWindowMediaRoot()
{
#if false // DISABLE_FULL_WINDOW
    if (m_tpLayoutRootPart.Get())
    {
        ctl::ComPtr<xaml_controls::IPanel> spFullWindowMediaRoot;
        IFC_RETURN(VisualTreeHelper::GetFullWindowMediaRootStatic(this, &spFullWindowMediaRoot));
        ASSERT(spFullWindowMediaRoot.Get());

        if (spFullWindowMediaRoot.Get())
        {
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

            IFC_RETURN(this->get_ChildrenInternal(&spChildren));

            int index = -1;
            IFC_RETURN(ContainsChild(spChildren.Get(), m_tpLayoutRootPart.Get(), index));
            if (index != -1)
            {
                auto pLayoutRoot = static_cast<UIElement*>(m_tpLayoutRootPart.Get());
                auto pCoreLayoutRoot = static_cast<CUIElement*>(pLayoutRoot->GetHandle());
                IFC_RETURN(spChildren->RemoveAt(index));

                IFC_RETURN(spFullWindowMediaRoot.Cast<Panel>()->get_ChildrenInternal(&spChildren));
                index = -1;
                IFC_RETURN(ContainsChild(spChildren.Get(), m_tpLayoutRootPart.Get(), index));
                if (index == -1)
                {
                    IFC_RETURN(spChildren->Append(m_tpLayoutRootPart.Get()));
                }

                // Forward theme value to FullWindowMediaRoot's RequestedTheme
                CValue themeValue;
                auto pPanel = spFullWindowMediaRoot.Cast<Panel>()->GetHandle();
                themeValue.Set(Theming::GetBaseValue(GetHandle()->GetTheme()), KnownTypeIndex::ElementTheme);
                IFC_RETURN(pPanel->SetValueByIndex(KnownPropertyIndex::FrameworkElement_RequestedTheme, themeValue));

                IFC_RETURN(spFullWindowMediaRoot.Cast<Panel>()->put_Visibility(xaml::Visibility_Visible));

                // TODO: reparenting should set dirtiness, but it does not, mark element dirty here directly
                CUIElement::NWSetContentDirty(
                    pCoreLayoutRoot,
                    DirtyFlags::Render | DirtyFlags::Bounds);

                if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
                {
                    auto layoutBoundsHelper = xamlRoot->GetLayoutBoundsHelperNoRef();

                    ctl::WeakRefPtr wrThis;
                    IFC_RETURN(ctl::AsWeak(this, &wrThis));

                    layoutBoundsHelper->AddLayoutBoundsChangedCallback(
                        [wrThis]() mutable
                        {
                            ctl::ComPtr<MediaPlayerElement> spThis;
                            IFC_RETURN(wrThis.As(&spThis));

                            if(spThis.Get())
                            {
                                BOOLEAN isFullWindow = FALSE;
                                IFC_RETURN(spThis->get_IsFullWindow(&isFullWindow));
                                if (isFullWindow && spThis->IsInLiveTree())
                                {
                                    // Update the media transport control's bounds with the available layout bounds.
                                    // showing or hiding the soft buttons.
                                    IFC_RETURN(spThis->UpdateMediaTransportBounds());
                                }
                            }
                            return S_OK;
                        }, &m_tokLayoutBoundsChanged);
                }

                // Update the media transport control's bounds with the current available layout bounds.
                IFC_RETURN(UpdateMediaTransportBounds());

                IFC_RETURN(GetXamlDispatcherNoRef()->RunAsync(
                    MakeCallback(ctl::ComPtr<MediaPlayerElement>(this), &MediaPlayerElement::SetFocusAfterEnteringFullWindowMode)));

                IFC_RETURN(GetXamlDispatcherNoRef()->RunAsync(
                    MakeCallback(
                        ctl::ComPtr<MediaPlayerElement>(this), &MediaPlayerElement::RedirectEvents)));
            }
        }
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::RemoveFromFullWindowMediaRoot()
{
#if false // DISABLE_FULL_WINDOW
    if (m_tpLayoutRootPart.Get())
    {
        ctl::ComPtr<xaml_controls::IPanel> spFullWindowMediaRoot;
        IFC_RETURN(VisualTreeHelper::GetFullWindowMediaRootStatic(this, &spFullWindowMediaRoot));
        ASSERT(spFullWindowMediaRoot.Get());

        if (spFullWindowMediaRoot.Get())
        {
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

            IFC_RETURN(spFullWindowMediaRoot.Cast<Panel>()->get_ChildrenInternal(&spChildren));

            int index = -1;
            IFC_RETURN(ContainsChild(spChildren.Get(), m_tpLayoutRootPart.Get(), index));
            if (index != -1)
            {
                auto pLayoutRoot = static_cast<UIElement*>(m_tpLayoutRootPart.Get());
                auto pCoreLayoutRoot = static_cast<CUIElement*>(pLayoutRoot->GetHandle());
                IFC_RETURN(spChildren->RemoveAt(index));

                IFC_RETURN(this->get_ChildrenInternal(&spChildren));
                index = -1;
                IFC_RETURN(ContainsChild(spChildren.Get(), m_tpLayoutRootPart.Get(), index));
                if (index == -1)
                {
                    IFC_RETURN(spChildren->Append(m_tpLayoutRootPart.Get()));
                }

                // TODO: reparenting should set dirtiness, but it does not, mark element dirty here directly
                CUIElement::NWSetContentDirty(
                    pCoreLayoutRoot,
                    DirtyFlags::Render | DirtyFlags::Bounds);

                IFC_RETURN(spFullWindowMediaRoot.Cast<Panel>()->put_Visibility(xaml::Visibility_Collapsed));

                // Remove the LayoutBoundsChanged callbacks
                if (m_tokLayoutBoundsChanged.value)
                {
                    if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(this))
                    {
                        xamlRoot->GetLayoutBoundsHelperNoRef()->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
                    }
                }

                IFC_RETURN(GetXamlDispatcherNoRef()->RunAsync(
                    MakeCallback(
                        ctl::ComPtr<MediaPlayerElement>(this), &MediaPlayerElement::RestoreEventsRedirection)));

                // Tell the MTC we're exiting full window mode
                BOOLEAN showTransportControls = FALSE;
                IFC_RETURN(get_AreTransportControlsEnabled(&showTransportControls));
                if (showTransportControls)
                {
                    ctl::ComPtr<IMediaTransportControls> spMediaTransportControls;
                    IFC_RETURN(get_TransportControls(&spMediaTransportControls));
                    if (spMediaTransportControls)
                    {
                        IFC_RETURN(spMediaTransportControls.Cast<MediaTransportControls>()->HandleExitFullWindowMode());
                    }
                }
            }
        }
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::UpdateMediaTransportBounds()
{
#if false // DISABLE_FULL_WINDOW
    auto* pFullWindowMediaRoot = GetHandle()->GetContext()->GetMainFullWindowMediaRoot();
    if (pFullWindowMediaRoot)
    {
        // Update the media transport control's bounds when the layout bounds is changed by
        // showing or hiding the soft buttons.
        // The SystemTray and AppBar will be suppressed on the full windowed media mode
        // so the layout bounds is the right bounds of the media transport control.
        pFullWindowMediaRoot->InvalidateArrange();
        pFullWindowMediaRoot->InvalidateMeasure();
    }
#endif // DISABLE_FULL_WINDOW
    return S_OK;
}

// ISupportInitialize
_Check_return_ HRESULT MediaPlayerElement::BeginInitImpl()
{
    m_bInit = true;
    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::EndInitImpl(_In_opt_ DirectUI::XamlServiceProviderContext*)
{
    ctl::ComPtr<wmp::IMediaPlayer> spMediaPlayer;

    m_bInit = false;
    IFC_RETURN(get_MediaPlayer(&spMediaPlayer));

    // Check if already have the MediaPlayer set, if not create one by default
    if ((spMediaPlayer.Get() == nullptr) &&
        (IsPropertySet(KnownPropertyIndex::MediaPlayerElement_Source) || IsPropertySet(KnownPropertyIndex::MediaPlayerElement_AutoPlay)))
    {
        IFC_RETURN(CreateDefaultMediaPlayer());
    }


    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::CreateDefaultMediaPlayer()
{
    ctl::ComPtr<wmp::IMediaPlayer> spMediaPlayer;

    IFC_RETURN(wf::ActivateInstance(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_Playback_MediaPlayer).Get(),
        spMediaPlayer.GetAddressOf()));

    IFC_RETURN(SetMediaPlayer(spMediaPlayer.Get()));

    m_bOwnsMediaPlayer = true;

    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::CloseMediaPlayer(_In_opt_ wmp::IMediaPlayer* pOldMediaPlayer)
{
    ctl::ComPtr<wmp::IMediaPlayer> spOldMediaPlayer(pOldMediaPlayer);
    if (m_bOwnsMediaPlayer && spOldMediaPlayer.Get())
    {
        ctl::ComPtr<wf::IClosable> spClosable;
        IFC_RETURN(spOldMediaPlayer.As(&spClosable));
        IFC_RETURN(spClosable->Close());
    }
    m_bOwnsMediaPlayer = false;
    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::UpdateSource()
{
    // One way binding of MediaPlayer object's Source property when its value has been set
    if (IsPropertySet(KnownPropertyIndex::MediaPlayerElement_Source) && m_spMediaPlayer.Get())
    {
        ctl::ComPtr<wmp::IMediaPlaybackSource> spMediaPlaybackSource;
        IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_Source, &spMediaPlaybackSource));
        ctl::ComPtr<wmp::IMediaPlayerSource2> spMediaPlayerSource;
        IFC_RETURN(m_spMediaPlayer.As(&spMediaPlayerSource));
        IFC_RETURN(spMediaPlayerSource->put_Source(spMediaPlaybackSource.Get()));
    }

    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::UpdateAutoPlay()
{
    // One way binding of MediaPlayer object's AutoPlay property when
    // - its value has been set
    // - MPE is currently live

    if (IsPropertySet(KnownPropertyIndex::MediaPlayerElement_AutoPlay) &&
        m_spMediaPlayer.Get() &&
        static_cast<CMediaPlayerElement*>(GetHandle())->IsActive())
    {
        BOOLEAN fAutoPlay = FALSE;
        IFC_RETURN(GetValueByKnownIndex(KnownPropertyIndex::MediaPlayerElement_AutoPlay, &fAutoPlay));
        IFC_RETURN(m_spMediaPlayer->put_AutoPlay(static_cast<BOOLEAN>(fAutoPlay)));
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::EnterImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding
)
{
    IFC_RETURN(__super::EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

    if (bLive)
    {
        IFC_RETURN(UpdateAutoPlay());

        IFC_RETURN(UpdateTimedTextSource());
    }

    return S_OK;
}

_Check_return_ HRESULT
MediaPlayerElement::LeaveImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
#if false // DISABLE_FULL_WINDOW
    IFC_RETURN(RemoveFromFullWindowMediaRoot());
#endif // DISABLE_FULL_WINDOW
    if (m_spTimedTextSource.Get())
    {
        IFC_RETURN(m_spTimedTextSource->SetMediaPlayer(nullptr));
    }

    IFC_RETURN(__super::LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));

    return S_OK;
}

bool MediaPlayerElement::IsPropertySet(KnownPropertyIndex propertyIndex)
{
    auto pDP = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyIndex);
    return (!static_cast<CMediaPlayerElement*>(GetHandle())->IsPropertyDefault(pDP));
}

_Check_return_ HRESULT MediaPlayerElement::RedirectEvents()
{
    auto pLayoutRoot = static_cast<UIElement*>(m_tpLayoutRootPart.Get());
    if (pLayoutRoot)
    {
        IFC_RETURN(MoveEventSources(pLayoutRoot));
    }
    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::RestoreEventsRedirection()
{
   auto pLayoutRoot = static_cast<UIElement*>(m_tpLayoutRootPart.Get());
   if (pLayoutRoot)
   {
       IFC_RETURN(RestoreEventSources(pLayoutRoot));
   }
   return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::UpdateTimedTextSource()
{
    if (m_spTimedTextSource.Get() == nullptr)
    {
        auto pLayoutRoot = static_cast<UIElement*>(m_tpTimedTextSourcePresenterPart.Get());
        if (pLayoutRoot)
        {
            IFC_RETURN(CTimedTextSource::Create(this, pLayoutRoot, &m_spTimedTextSource));
        }
    }

    if(m_spTimedTextSource.Get())
    {
        IFC_RETURN(m_spTimedTextSource->SetMediaPlayer(m_spMediaPlayer.Get()));
    }

    return S_OK;
}

void MediaPlayerElement::CleanupDeviceRelatedResources(const bool cleanupDComp)
{
    if (m_spTimedTextSource)
    {
        m_spTimedTextSource->CleanupDeviceRelatedResources(cleanupDComp);
    }
}

_Check_return_ HRESULT MediaPlayerElement::OnMTCVisible(_In_ double height)
{
    if (m_spTimedTextSource.Get())
    {
        IFC_RETURN(m_spTimedTextSource->SetMTCOffset(height));
    }

    return S_OK;
}

_Check_return_ HRESULT MediaPlayerElement::UpdatePosterImageVisibility()
{
    ctl::ComPtr<IImageSource> spImageSource;
    IFC_RETURN(get_PosterSource(&spImageSource));

    if (m_tpPosterImagePart.Get())
    {
        if (spImageSource)
        {
            m_tpPosterImagePart.AsOrNull<IUIElement>().Get()->put_Visibility(xaml::Visibility_Visible);
        }
        else
        {
            m_tpPosterImagePart.AsOrNull<IUIElement>().Get()->put_Visibility(xaml::Visibility_Collapsed);
        }
    }
    return S_OK;
}

#define EVENT_IMPL_ADD(EventName) \
    _Check_return_ HRESULT MediaPlayerElement::add_##EventName(_In_ EventName##EventSourceType::HandlerType* pValue, _Out_ EventRegistrationToken* ptToken) \
    {\
        ctl::ComPtr<IInspectable> spThis = ctl::ComPtr<MediaPlayerElement>(this).AsOrNull<xaml_controls::IMediaPlayerElement>();\
        ctl::ComPtr<EventName##EventSourceType::SenderType> spSender;\
        IFC_RETURN(spThis.As(&spSender));\
        ctl::ComPtr<CForwardEventSource<\
            EventName##EventSourceType::HandlerType,\
                EventName##EventSourceType::SenderType,\
                EventName##EventSourceType::ArgsType>> spHandler;\
        IFC_RETURN(ctl::make(pValue, spSender.Get(), &spHandler));\
        ctl::ComPtr<EventName##EventSourceType::HandlerType> spValue;\
        IFC_RETURN(spHandler.As(&spValue));\
        IFC_RETURN(__super::add_##EventName(spValue.Get(), ptToken));\
        BOOLEAN isFullWindow = FALSE;\
        IFC_RETURN(get_IsFullWindow(&isFullWindow));\
        if (isFullWindow) IFC_RETURN(RedirectEvents());\
        return S_OK;\
    }

#define EVENT_IMPL_DEL(EventName) \
    _Check_return_ HRESULT MediaPlayerElement::remove_##EventName(_In_ EventRegistrationToken tToken) \
    {\
        BOOLEAN isFullWindow = FALSE;\
        IFC_RETURN(get_IsFullWindow(&isFullWindow));\
        if (isFullWindow) IFC_RETURN(RestoreEventsRedirection()); \
        IFC_RETURN(__super::remove_##EventName(tToken)); \
        if (isFullWindow) IFC_RETURN(RedirectEvents()); \
        return S_OK; \
    }

#define EVENT_IMPL(EventName) \
    EVENT_IMPL_ADD(EventName) \
    EVENT_IMPL_DEL(EventName)

EVENT_IMPL(KeyUp)
EVENT_IMPL(KeyDown)
EVENT_IMPL(GotFocus)
EVENT_IMPL(LostFocus)
EVENT_IMPL(DragStarting)
EVENT_IMPL(DropCompleted)
EVENT_IMPL(DragEnter)
EVENT_IMPL(DragLeave)
EVENT_IMPL(DragOver)
EVENT_IMPL(Drop)
EVENT_IMPL(PointerPressed)
EVENT_IMPL(PointerMoved)
EVENT_IMPL(PointerReleased)
EVENT_IMPL(PointerEntered)
EVENT_IMPL(PointerExited)
EVENT_IMPL(PointerCaptureLost)
EVENT_IMPL(PointerCanceled)
EVENT_IMPL(PointerWheelChanged)
EVENT_IMPL(Tapped)
EVENT_IMPL(DoubleTapped)
EVENT_IMPL(Holding)
EVENT_IMPL(ContextRequested)
EVENT_IMPL(ContextCanceled)
EVENT_IMPL(RightTapped)
EVENT_IMPL(ManipulationStarting)
EVENT_IMPL(ManipulationInertiaStarting)
EVENT_IMPL(ManipulationStarted)
EVENT_IMPL(ManipulationDelta)
EVENT_IMPL(ManipulationCompleted)

