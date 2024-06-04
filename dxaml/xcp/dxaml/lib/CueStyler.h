// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Helper class for closed caption that creates the TextBlock
//      and styles it as needed.

#pragma once

#include "Image.g.h"
#include <fwd/windows.media.h>
#include <Windows.Media.Closedcaptioning.h>
#include <microsoft.ui.text.h>

namespace DirectUI
{
    class TextBlock;
    class StackPanel;
    class Border;
    class Grid;
    class Canvas;

    class CCueStyler
    {
    public:
        CCueStyler();

        _Check_return_ HRESULT Initialize();

        void CleanupDeviceRelatedResources(const bool cleanupDComp);

        _Check_return_ HRESULT SetTextBlock(_In_ wmc::ITimedTextLine* pLine,
                                            _In_ wmc::ITimedTextStyle* pStyle,
                                            _In_ TextBlock* pTextBlock,
                                            _In_ wmc::ITimedTextRegion* pRegion,
                                            _In_ Grid* pCueElement,
                                            _In_ bool isOutline) noexcept;

        _Check_return_ HRESULT SetImage(_In_ ctl::ComPtr<Image>& spImage, _In_ const ctl::ComPtr<wmc::IImageCue>& spImageCue);

        _Check_return_ HRESULT SetRegionConfiguration(_In_ Border* pRegion,
                                                      _In_opt_ wmc::ITimedTextRegion* pRegionStyle,
                                                      _In_ double parentWidth,
                                                      _In_ double parentHeight,
                                                      _In_ double xOffset,
                                                      _In_ double yOffset) noexcept;

        _Check_return_ HRESULT SetImageRegionConfiguration(_In_ Border* const pRegion,
                                                            _In_ const ctl::ComPtr<wmc::IImageCue>& spCue,
                                                            _In_ double parentWidth,
                                                            _In_ double parentHeight,
                                                            _In_ double xOffset,
                                                            _In_ double yOffset);

        _Check_return_ HRESULT CreateOutline(_In_ wmc::ITimedTextLine* pLine,
                                             _In_ wmc::ITimedTextStyle* pStyle,
                                             _In_ wmc::ITimedTextRegion* pRegion,
                                             _In_ Grid* pCueElement);

        _Check_return_ HRESULT SetTextBlockEffect(
            _In_ TextBlock* pTextBlock,
            _In_ Canvas* underTextBlock,
            _In_ Grid* textContainer,
            _In_ Grid* topWrapper);

        void SetNaturalVideoSize(_In_ double height, _In_ double width);
        void SetFontSizeRatio(_In_ double videoActualWidth);

        static wrl_wrappers::HStringReference ConvertUserFontStyle(_In_ wm::ClosedCaptioning::ClosedCaptionStyle fontStyle);
        static double ConvertUserFontSizeScale(_In_ wm::ClosedCaptioning::ClosedCaptionSize fontStyle);
        static wrl_wrappers::HStringReference ConvertUserFontStyle(_In_ LPCWSTR fontStyle);
        static bool IsW3CFontStyle(_In_ LPWSTR fontStyle);
        static wu::Color ConvertUserColor(_In_ wm::ClosedCaptioning::ClosedCaptionColor color,
                                                   _In_ wm::ClosedCaptioning::ClosedCaptionOpacity opacity);

    private:
        _Check_return_ HRESULT CreateOutlineHelper(_In_ wmc::ITimedTextLine* pLine,
                                                   _In_ wmc::ITimedTextStyle* pStyle,
                                                   _In_ wmc::ITimedTextRegion* pRegion,
                                                   _In_ Grid* pCueElement,
                                                   _In_ double x,
                                                   _In_ double y);

        _Check_return_ HRESULT CalculateFontSize(_In_ wmc::ITimedTextStyle* pStyle, _Out_ double* actualSize);

        _Check_return_ HRESULT CreateRun(_In_ wmc::ITimedTextLine* pLine,
                                         _In_ wmc::ITimedTextStyle* pStyle,
                                         _In_ int start,
                                         _In_ int length,
                                         _In_ bool isOutline,
                                         _Outptr_ xaml_docs::IRun** ppRun) noexcept;

        _Check_return_ HRESULT CreateVisualAndAdd(
            _In_ WUComp::ICompositionBrush* brush,
            const wfn::Vector2 textSize,
            wfn::Vector3 offset,
            _In_ WUComp::IVisualCollection* children);

        wrl::ComPtr<WUComp::ICompositionBrush> CreateMaskBrush(_In_ WUComp::ICompositionBrush* surfaceBrush, _In_ WUComp::ICompositionBrush* colorBrush);

        _Check_return_ HRESULT ApplyDropShadowStyle(_In_ TextBlock* pTextBlock, wm::ClosedCaptioning::ClosedCaptionSize userFontSize);
        _Check_return_ HRESULT ApplyRaisedStyle(_In_ WUComp::ICompositionBrush* alphaMask, wfn::Vector2 textSize, _In_ WUComp::IVisualCollection* children, wm::ClosedCaptioning::ClosedCaptionSize userFontSize);
        _Check_return_ HRESULT ApplyDepressedStyle(_In_ WUComp::ICompositionBrush* alphaMask, wfn::Vector2 textSize, _In_ WUComp::IVisualCollection* children, wm::ClosedCaptioning::ClosedCaptionSize userFontSize);
        _Check_return_ HRESULT ApplyUniformStyle(_In_ WUComp::ICompositionBrush* alphaMask, wfn::Vector2 textSize, _In_ WUComp::IVisualCollection* children, wm::ClosedCaptioning::ClosedCaptionSize userFontSize);

        double m_videoHeight;
        double m_videoWidth;
        double m_fontScaleFactor;
        double m_xOffset;
        double m_yOffset;
        ctl::ComPtr<wm::ClosedCaptioning::IClosedCaptionPropertiesStatics> m_pSettingsStatics;
        ctl::ComPtr<xaml_media::IFontFamilyFactory> m_spFontFamilyFactory;
        ctl::ComPtr<mut::IFontWeightsStatics> m_spFontWeightsStatics;

        // Used for text effects
        wrl::ComPtr<WUComp::ICompositor> m_compositor;
        wrl::ComPtr<WUComp::ICompositor2> m_compositor2;
        wrl::ComPtr<WUComp::ICompositionBrush> m_blackBrush;
        wrl::ComPtr<WUComp::ICompositionBrush> m_blackBrush20;
        wrl::ComPtr<WUComp::ICompositionBrush> m_whiteBrush60;
        wrl::ComPtr<WUComp::IDropShadow> m_dropShadow;
        wrl::ComPtr<WUComp::ICompositionShadow> m_compositionShadow;
    };
}
