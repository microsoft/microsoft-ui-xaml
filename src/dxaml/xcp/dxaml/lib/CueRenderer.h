// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CueStyler.h>
#include <ITimedTextSourcePresenter.h>

namespace DirectUI
{
    class Grid;
    class Border;
    class UIElement;
    class DispatcherTimer;
    class Storyboard;
    class DoubleAnimation;
    class CTimedTextSource;
    class LayoutBoundsChangedHelper;

    //  Responsible for interacting with the visual tree,
    //  gets cues from the timed text source and uses the styler
    //  to create the visual elements to be inserted into the tree.
    class CCueRenderer
    {
    public:
        CCueRenderer(_In_ ITimedTextSourcePresenter* pTimedTextSource);
        ~CCueRenderer();

        _Check_return_ HRESULT Initialize(_In_ xaml::IUIElement* pOwner);

        void CleanupDeviceRelatedResources(const bool cleanupDComp);

        _Check_return_ HRESULT AddCue(_In_ const ctl::ComPtr<wmc::IMediaCue>& spCue) noexcept;
        _Check_return_ HRESULT RemoveCue(_In_ const ctl::ComPtr<wmc::IMediaCue>& spCue);
        _Check_return_ HRESULT ClearCues();
        _Check_return_ HRESULT Reset();

        _Check_return_ HRESULT AddToOwner();
        _Check_return_ HRESULT RemoveFromOwner();
        _Check_return_ HRESULT AddToFullWindowMediaRoot();
        _Check_return_ HRESULT RemoveFromFullWindowMediaRoot();

        _Check_return_ HRESULT SetMTCOffset(_In_ double offset);
        void ResetParentSize() { m_needsParentSize = true; }

        _Check_return_ HRESULT OnCuePresentationModeChangedCallback(_In_ wmp::ITimedMetadataPresentationModeChangedEventArgs* pArgs);

    private:
        struct  CCueItem
        {
            INT64 Id;
            ctl::ComPtr<UIElement> CueElement;
            std::wstring Region;
            int AnimationStep;
            double AnimationDistance;
            bool UseAnimation;
        };

        _Check_return_ HRESULT AddCueContainerToVisualTree();
        _Check_return_ HRESULT SetNaturalVideoSize();

        _Check_return_ HRESULT UpdateBounds();

        _Check_return_ HRESULT OnSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs);
        _Check_return_ HRESULT OnOrientationChanged(_In_ wgrd::IDisplayInformation* pSender, _In_ IInspectable* pArgs);

        _Check_return_ HRESULT UpdateClipping();
        _Check_return_ HRESULT ResetCues();

        INT64 GetCueId(_In_ const wmc::IMediaCue* const pCue) const;

        _Check_return_ HRESULT CalculateActualVideoSize();

        _Check_return_ HRESULT GetNaturalVideoSize(_Out_ INT32* pHeight, _Out_ INT32* pWidth);

        _Check_return_ HRESULT AddCueItem(
            _In_ const ctl::ComPtr<Grid>& spCueElement,
            _In_ const ctl::ComPtr<StackPanel>& spRegion,
            _In_ const ctl::ComPtr<UIElement>& spChild,
            _In_ const std::wstring& regionName,
            _In_ const INT64 cueId,
            _In_ double lineHeight);

        _Check_return_ HRESULT UpdateVideoSize();

        _Check_return_ HRESULT CreateCueRegion(
            _In_ const ctl::ComPtr<wmc::IMediaCue>& spCue,
            _Out_ ctl::ComPtr<Border>& spRegionBorder,
            _Inout_ std::wstring& regionName);

        _Check_return_ HRESULT CreateCueItem(
            _In_ const ctl::ComPtr<wmc::IMediaCue>& spCue,
            _In_ ctl::ComPtr<Border>& spRegionBorder,
            _In_ std::wstring& regionName);

        bool m_isInVisualTree;
        bool m_isFullWindow;
        bool m_needsParentSize;
        double m_parentWidth;
        double m_parentHeight;
        double m_videoWidth;
        double m_videoHeight;
        double m_mtcVideoOverlapAmount;
        double m_mtcHeight;
        double m_videoXOffset;
        double m_videoYOffset;
        wgrd::DisplayOrientations m_orientation;

        ITimedTextSourcePresenter* m_pTimedTextSourceNoRef;
        ctl::ComPtr<Grid> m_cueContainer;
        ctl::WeakRefPtr m_wrOwner;
        CCueStyler m_cueStyler;

        std::list<std::shared_ptr<CCueItem>> m_cueList;
        std::map<std::wstring, ctl::ComPtr<Border>> m_regionList;
        ctl::ComPtr<Storyboard> m_spStoryboard;
        EventRegistrationToken m_tokLayoutBoundsChanged{0};
        EventRegistrationToken m_tokSizeChanged;
        EventRegistrationToken m_orientationChangedToken;
    };
}
