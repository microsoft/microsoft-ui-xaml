// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CueStyler.h"
#include "TextBlock.g.h"
#include "StackPanel.g.h"
#include "Border.g.h"
#include "SolidColorBrush.g.h"
#include "TranslateTransform.g.h"
#include "Grid.g.h"
#include "Canvas.g.h"
#include "ColorUtil.h"
#include "Windows.Media.h"
#include "Run.g.h"
#include "SoftwareBitmapSource.g.h"
#include <MFMediaEngine.h>
#include <Microsoft.UI.h>

using namespace DirectUI;
using namespace Microsoft::WRL;
using namespace xaml_media;
using namespace xaml_docs;

///
///                      <--------------------------------------
///       /                     | %DefaultFontPadding          |
///   |-----             <-----------------                    |
///   |                         |                              |
///   |-----        *           |                        %DefaultFontSize
///   |             |           |                              |
///   |-----        |    <-----------------                    |
///                 /           | %DefaultFontPadding          |
///             \--/     <--------------------------------------
const double DefaultFontPadding = 2.5;
const double DefaultFontSize = 5.0;
const auto DefaultLineStackingStrategy = xaml::LineStackingStrategy::LineStackingStrategy_MaxHeight;
const auto DefaultTextLineBounds = xaml::TextLineBounds::TextLineBounds_Full;

CCueStyler::CCueStyler() :
    m_videoHeight(0),
    m_videoWidth(0),
    m_fontScaleFactor(0.0),
    m_xOffset(0),
    m_yOffset(0)
{}

_Check_return_ HRESULT CCueStyler::Initialize()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IActivationFactory> spActivationFactory;

    hr = wf::GetActivationFactory(
                    wrl_wrappers::HStringReference(RuntimeClass_Windows_Media_ClosedCaptioning_ClosedCaptionProperties).Get(),
                    spActivationFactory.ReleaseAndGetAddressOf());
    if (REGDB_E_CLASSNOTREG == hr)
    {
        // Ignore error if Class is not available which means we are running on N-SKU
        return S_OK;
    }
    // All other failures are not accepted
    IFC(hr);

    IFC(spActivationFactory.As(&m_pSettingsStatics));

Cleanup:
    return hr;
}

 // Setup up the region (StackPanel is used to represent a region) properties
 // with the given region information and parent size
_Check_return_ HRESULT
CCueStyler::SetRegionConfiguration(
    _In_ Border* pRegion,
    _In_opt_ wmc::ITimedTextRegion* pRegionStyle,
    _In_ double parentWidth,
    _In_ double parentHeight,
    _In_ double xOffset,
    _In_ double yOffset) noexcept
{
    m_xOffset = xOffset;
    m_yOffset = yOffset;

    parentWidth = std::max<double>(0, parentWidth);
    parentHeight = std::max<double>(0, parentHeight);

    IFC_RETURN(pRegion->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Left));
    IFC_RETURN(pRegion->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Top));

    // Alignment
    wmc::TimedTextDisplayAlignment alignment = {};
    {
        ctl::ComPtr<xaml::IUIElement> spChild;
        ctl::ComPtr<StackPanel> spChildAsStackPanel;
        IFC_RETURN(pRegion->get_Child(&spChild));
        IFC_RETURN(spChild.As(&spChildAsStackPanel));

        if (pRegionStyle)
        {
            IFC_RETURN(pRegionStyle->get_DisplayAlignment(&alignment));
        }
        else
        {
            alignment = wmc::TimedTextDisplayAlignment::TimedTextDisplayAlignment_Before;
        }

        switch (alignment)
        {
        case wmc::TimedTextDisplayAlignment::TimedTextDisplayAlignment_Before:
            IFC_RETURN(spChildAsStackPanel->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Top));
            break;
        case wmc::TimedTextDisplayAlignment::TimedTextDisplayAlignment_Center:
            IFC_RETURN(spChildAsStackPanel->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Center));
            break;
        case wmc::TimedTextDisplayAlignment::TimedTextDisplayAlignment_After:
            IFC_RETURN(spChildAsStackPanel->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Bottom));
            break;
        }
    }

    //
    // The closed caption styles specify an explicit height and position for the closed caption area, but this causes
    // clipping when using large text sizes because the height isn't large enough. Instead, we sometimes choose to
    // ignore the explicit size so that the captions won't be clipped. We'll still align the text with the explicit
    // position in the style.
    //
    // Closed captions are placed inside a StackPanel, and that StackPanel is placed inside the pRegion Border. There
    // are two ways of sizing and positioning the pRegion border using the position/extent height specified in the
    // styles.
    //
    // For bottom-aligned captions:
    //      +-----------------------------------+----------+-------------
    //      |    ^                     ^        |          |     ^
    //      |    |                     |        |          |     |
    //      |    |                     |        |          |     |
    //      |    |                    top       |          |     |
    //      |    |                  margin      |          |    top
    //      |    |                     |        |          |  margin
    //      | parent                   |        |          |     +
    //      | height                   v        |          |  extent
    //      |    |    +----------+    ---       |          |  height
    //      |    |    |          |     ^        |          |     |
    //      |    |    |  closed  |  extent      |  closed  |     |
    //      |    |    | captions |  height      | captions |     |
    //      |    |    | layout 1 |     v        | layout 2 |     v
    //      |    |    +----------+    ---       +----------+    ---
    //      |    v               bottom margin              bottom margin
    //      +-------------------------------------------------------------
    //
    // For top-aligned captions:
    //      +------------------------------------------------------------
    //      |    ^                 top margin                top margin
    //      |    |    +----------+    ---       +----------+    ---
    //      |    |    |  closed  |     ^        |  closed  |     ^
    //      |    |    | captions |  extent      | captions |     |
    //      |    |    | layout 1 |  height      | layout 2 |     |
    //      |    |    |          |     v        |          |     |
    //      | parent  +----------+    ---       |          |  parent
    //      | height                            |          |  height
    //      |    |                              |          |     -
    //      |    |                              |          |    top
    //      |    |                              |          |  margin
    //      |    |                              |          |     |
    //      |    |                              |          |     |
    //      |    |                              |          |     |
    //      |    v                              |          |     v
    //      +-----------------------------------+----------+--------------
    //
    // If the closed captions are top-aligned or bottom-aligned, we use layout 2 above so that text with large font
    // sizes don't get clipped. Note that this is the size of the pRegion Border. The StackPanel containing the text is
    // then placed inside that Border with the proper vertical alignment.
    //
    // If the closed captions are center-aligned, we use layout 1. This is to reduce the number of behavior changes, as
    // we haven't seen center-aligned captions that have been clipped out in the wild. If we encounter them in the future
    // then we'll remove this restriction and show all captions with layout 2.
    //

    double paddingSize = 0.0;
    bool hasExtentHeight = false;
    double extentHeight = 0;
    // Extent
    {
        wmc::TimedTextSize extent = {};

        if (pRegionStyle)
        {
            IFC_RETURN(pRegionStyle->get_Extent(&extent));
        }
        else
        {
            extent.Width = 0;
            extent.Height = 0;
            extent.Unit = wmc::TimedTextUnit_Percentage;
        }

        if (extent.Width != 0 && extent.Height != 0)
        {
            double extentWidth = 0;
            hasExtentHeight = true;

            switch (extent.Unit)
            {
            case wmc::TimedTextUnit_Pixels:
                extentWidth = extent.Width;
                extentHeight = extent.Height;
                break;
            case wmc::TimedTextUnit_Percentage:
                extentWidth = parentWidth * (extent.Width / 100.0);
                extentHeight = parentHeight * (std::min(100.0, extent.Height + (DefaultFontPadding*2)) / 100.0);
                break;
            }

            pRegion->put_Width(extentWidth);
            pRegion->put_Height(extentHeight);
        }
        else
        {
            // No extent was specified, add a Padding of half one line of text.
            // One line of text is %5 of parentHeight
            paddingSize += parentHeight * (DefaultFontPadding / 100.0);
        }
    }

    // Position
    {
        xaml::Thickness thickness = { -paddingSize, -paddingSize, -paddingSize, -paddingSize };
        wmc::TimedTextPoint position = {};

        if (pRegionStyle)
        {
            IFC_RETURN(pRegionStyle->get_Position(&position));
        }
        else
        {
            position.X = 0;
            position.Y = 0;
            position.Unit = wmc::TimedTextUnit_Percentage;
        }

        switch (position.Unit)
        {
        case wmc::TimedTextUnit_Pixels:
            thickness.Top += position.Y;
            thickness.Left += position.X;
            break;
        case wmc::TimedTextUnit_Percentage:
            thickness.Top += parentHeight * (position.Y / 100);
            thickness.Left += parentWidth * (position.X / 100);
            break;
        }

        if (hasExtentHeight)
        {
            // Apply layout 2. See diagrams above.

            if (alignment == wmc::TimedTextDisplayAlignment_After)
            {
                pRegion->put_Height(extentHeight + thickness.Top);

                // Layout 2 puts the pRegion Border at the top of the screen, so reset the top margin. We want to keep the
                // space at the sides though, so just reset just the top margin and still apply the rest.
                thickness.Top = 0;
            }
            else if (alignment == wmc::TimedTextDisplayAlignment_Before)
            {
                // Note: This math looks different from the math for TimedTextDisplayAlignment_After, because we don't have bottom
                // margins available - only the top margin (which comes from the vertical position). So we can't do extentHeight +
                // thickness.Bottom. Instead, take the parentHeight and subtract out the top margin. Also take a max with the explicit
                // extentHeight to avoid the parentHeight being too small and the calculation producing a negative number.
                pRegion->put_Height(std::max<double>(extentHeight, parentHeight - thickness.Top));

                // Layout 2 puts the pRegion Border at the bottom of the screen, so reset the bottom margin. We want to keep the
                // space at the sides though, so just reset just the bottom margin and still apply the rest.
                thickness.Bottom = 0;
            }
            // Future note: If we wanted to use layout 2 for center-aligned captions as well, then reset both the top and bottom
            // thicknesses so we don't have any vertical margin for the pRegion Border, allowing it to span the full extent height.
            // Then center-align the spChildAsStackPanel StackPanel inside the pRegion Border.
        }
        IFC_RETURN(pRegion->put_Margin(thickness));
    }

    // Background
    {
        ctl::ComPtr<SolidColorBrush> spBackgroundBrush;
        wu::Color backgroundColor = {};
        wm::ClosedCaptioning::ClosedCaptionOpacity userOpacity = wm::ClosedCaptioning::ClosedCaptionOpacity_Default;
        wm::ClosedCaptioning::ClosedCaptionColor userColor = wm::ClosedCaptioning::ClosedCaptionColor_Default;

        if (m_pSettingsStatics)
        {
            // IClosedCaptionPropertiesStatics can return E_ACCESSDENIED if the registry key that backs the closed
            // caption settings doesn't have the correct permissions set. Tolerate those errors and just go with
            // default values instead.
            HRESULT settingsHR = S_OK;

            settingsHR = m_pSettingsStatics->get_RegionColor(&userColor);
            if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
            {
                IFC_RETURN(settingsHR);
            }
            settingsHR = m_pSettingsStatics->get_RegionOpacity(&userOpacity);
            if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
            {
                IFC_RETURN(settingsHR);
            }
        }

        IFC_RETURN(ctl::make(&spBackgroundBrush));
        if (userOpacity != wm::ClosedCaptioning::ClosedCaptionOpacity_Default || userColor != wm::ClosedCaptioning::ClosedCaptionColor_Default)
        {
            // If the user has a non-default setting, it has the highest priority
            backgroundColor = ConvertUserColor(userColor, userOpacity);
        }
        else
        {
            if (pRegionStyle)
            {
                IFC_RETURN(pRegionStyle->get_Background(&backgroundColor));
            }
            else
            {
                backgroundColor.R = 0;
                backgroundColor.G = 0;
                backgroundColor.B = 0;
                backgroundColor.A = 0;
            }
        }

        IFC_RETURN(spBackgroundBrush->put_Color(backgroundColor));
        IFC_RETURN(pRegion->put_Background(spBackgroundBrush.Get()));
    }

    // Padding
    {
        wmc::TimedTextPadding padding = {};
        xaml::Thickness xamlPadding = { paddingSize, paddingSize, paddingSize, paddingSize };

        if (pRegionStyle)
        {
            IFC_RETURN(pRegionStyle->get_Padding(&padding));
        }

        if (padding.Unit == wmc::TimedTextUnit_Percentage)
        {
            xamlPadding.Top +=  parentHeight * (padding.Start / 100);
            xamlPadding.Bottom += parentHeight * (padding.End / 100);
            xamlPadding.Left += parentWidth * (padding.Before / 100);
            xamlPadding.Right += parentWidth * (padding.After / 100);
        }
        else
        {
            xamlPadding.Top += padding.Start;
            xamlPadding.Bottom += padding.End;
            xamlPadding.Left += padding.Before;
            xamlPadding.Right += padding.After;
        }
        IFC_RETURN(pRegion->put_Padding(xamlPadding));
    }

    // ZIndex
    {
        int ZIndex = {};

        if (pRegionStyle)
        {
            IFC_RETURN(pRegionStyle->get_ZIndex(&ZIndex));
        }

        IFC_RETURN(static_cast<CUIElement*>(pRegion->GetHandle())->SetZIndex(ZIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT
CCueStyler::SetImageRegionConfiguration(_In_ Border* const pRegion,
    _In_ const ctl::ComPtr<wmc::IImageCue>& spImageCue,
    _In_ double parentWidth,
    _In_ double parentHeight,
    _In_ double xOffset,
    _In_ double yOffset)
{
    m_xOffset = xOffset;
    m_yOffset = yOffset;

    parentWidth = std::max<double>(0, parentWidth);
    parentHeight = std::max<double>(0, parentHeight);

    IFC_RETURN(pRegion->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Left));
    IFC_RETURN(pRegion->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Top));

    double paddingSize = 0.0;
    // Extent
    {
        wmc::TimedTextSize extent = {};

        IFC_RETURN(spImageCue->get_Extent(&extent));

        if (extent.Width != 0 && extent.Height != 0)
        {
            switch (extent.Unit)
            {
            case wmc::TimedTextUnit_Pixels:
                IFC_RETURN(pRegion->put_Width(extent.Width));
                IFC_RETURN(pRegion->put_Height(extent.Height));
                break;
            case wmc::TimedTextUnit_Percentage:
                const double regionWidth = parentWidth * (extent.Width / 100.0);
                IFC_RETURN(pRegion->put_Width(regionWidth));
                const double regionHeight = parentHeight * (std::min(100.0, extent.Height + (DefaultFontPadding * 2)) / 100.0);
                IFC_RETURN(pRegion->put_Height(regionHeight));
                break;
            }
        }
        else
        {
            // No extent was specified, add a Padding of half one line of text.
            // One line of text is %5 of parentHeight
            paddingSize += parentHeight * (DefaultFontPadding / 100.0);
        }
    }

    // Position
    {
        xaml::Thickness thickness = { -paddingSize, -paddingSize, -paddingSize, -paddingSize };
        wmc::TimedTextPoint position = {};

        IFC_RETURN(spImageCue->get_Position(&position));

        switch (position.Unit)
        {
        case wmc::TimedTextUnit_Pixels:
            thickness.Top += position.Y;
            thickness.Left += position.X;
            break;
        case wmc::TimedTextUnit_Percentage:
            thickness.Top += parentHeight * (position.Y / 100);
            thickness.Left += parentWidth * (position.X / 100);
            break;
        }

        IFC_RETURN(pRegion->put_Margin(thickness));
    }

    return S_OK;
}

_Check_return_ HRESULT CCueStyler::SetImage(_In_ ctl::ComPtr<Image>& spImage,  _In_ const ctl::ComPtr<wmc::IImageCue>& spImageCue)
{
    ctl::ComPtr<SoftwareBitmapSource> spBitmapSource;
    ctl::ComPtr<wgri::ISoftwareBitmap> spBitmap;
    ctl::ComPtr<wf::IAsyncAction> spAsyncActionNotUsed;

    IFC_RETURN(ctl::make(&spBitmapSource));
    IFC_RETURN(spImageCue->get_SoftwareBitmap(&spBitmap));
    IFC_RETURN(spBitmapSource->SetBitmapAsync(spBitmap.Get(), &spAsyncActionNotUsed));
    IFC_RETURN(spImage->put_Source(spBitmapSource.Get()));
    return S_OK;
}


// Populated the given Textblock with the given style and text
_Check_return_ HRESULT
CCueStyler::SetTextBlock(_In_ wmc::ITimedTextLine* pLine,
                         _In_ wmc::ITimedTextStyle* pStyle,
                         _In_ TextBlock* pTextBlock,
                         _In_ wmc::ITimedTextRegion* pRegion,
                         _In_ Grid* pCueElement,
                         _In_ bool isOutline) noexcept
{
    HRESULT hr = S_OK;
    ctl::ComPtr<wfc::IVector<wmc::TimedTextSubformat*>> spSubformats;
    unsigned int size = 0;

    wm::ClosedCaptioning::ClosedCaptionOpacity userBackgroundOpacity = wm::ClosedCaptioning::ClosedCaptionOpacity_Default;
    wm::ClosedCaptioning::ClosedCaptionColor userBackgroundColor = wm::ClosedCaptioning::ClosedCaptionColor_Default;
    if (m_pSettingsStatics)
    {
        // IClosedCaptionPropertiesStatics can return E_ACCESSDENIED if the registry key that backs the closed
        // caption settings doesn't have the correct permissions set. Tolerate those errors and just go with
        // default values instead.
        HRESULT settingsHR = S_OK;
        settingsHR = m_pSettingsStatics->get_BackgroundColor(&userBackgroundColor);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC(settingsHR);
        }
        settingsHR = m_pSettingsStatics->get_BackgroundOpacity(&userBackgroundOpacity);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC(settingsHR);
        }
    }
    IFC(pLine->get_Subformats(&spSubformats));
    IFC(spSubformats->get_Size(&size));

    IFC(pTextBlock->put_TextLineBounds(DefaultTextLineBounds));
    IFC(pTextBlock->put_LineStackingStrategy(DefaultLineStackingStrategy));

    if (size == 0)
    {
        ctl::ComPtr<wfc::IVector<xaml_docs::Inline*>> spInlines;
        ctl::ComPtr<IInline> spInline;
        ctl::ComPtr<IRun> spRun;

        IFC(CreateRun(pLine, pStyle, 0, -1, isOutline, &spRun));

        IFC(pTextBlock->get_Inlines(&spInlines));
        IFC(spRun.As<IInline>(&spInline));
        IFC(spInlines->Append(spInline.Get()));
    }
    else
    {
        ctl::ComPtr<IInline> spInline;
        ctl::ComPtr<IRun> spRun;
        ctl::ComPtr<wmc::ITimedTextStyle> spStyle;
        ctl::ComPtr<wmc::ITimedTextSubformat> spSubformat;
        ctl::ComPtr<wfc::IVector<xaml_docs::Inline*>> spInlines;
        int currentIndex = {};
        int textLength = {};
        wrl_wrappers::HString text;

        IFC(pTextBlock->get_Inlines(&spInlines));

        IFC(pLine->get_Text(text.ReleaseAndGetAddressOf()));
        textLength = text.Length();

        for (unsigned int i = 0; i < size; i++)
        {
            int start = 0;
            int length = 0;

            IFC(spSubformats->GetAt(i, &spSubformat));
            IFC(spSubformat->get_StartIndex(&start));
            IFC(spSubformat->get_Length(&length));
            IFC(spSubformat->get_SubformatStyle(&spStyle));

            if (start == currentIndex)
            {
                IFC(CreateRun(pLine, spStyle.Get(), start, length, isOutline, &spRun));
                currentIndex += length;

                IFC(spRun.As<IInline>(&spInline));
                IFC(spInlines->Append(spInline.Get()));
            }
            else
            {
                // if start != currentIndex then there is text between the styles (or at the beginning)
                // that uses the default style

                // Apply the in-between style
                int inBetweenLength = start - currentIndex;
                IFC(CreateRun(pLine, pStyle, currentIndex, inBetweenLength, isOutline, &spRun));
                currentIndex += inBetweenLength;
                IFC(spRun.As<IInline>(&spInline));
                IFC(spInlines->Append(spInline.Get()));

                // Apply the current substyle
                IFC(CreateRun(pLine, spStyle.Get(), start, length, isOutline, &spRun));
                currentIndex += length;
                IFC(spRun.As<IInline>(&spInline));
                IFC(spInlines->Append(spInline.Get()));
            }
        }

        // There are no more subformats, if there is still more text left, it gets the default style
        if (currentIndex < textLength )
        {
            int remainingText = textLength - currentIndex;
            IFC(CreateRun(pLine, spStyle.Get(), currentIndex, remainingText, isOutline, &spRun));
            IFC(spRun.As<IInline>(&spInline));
            IFC(spInlines->Append(spInline.Get()));
        }
    }

    // Set the background color for this line, outlines do not have their own background color
    if (!isOutline)
    {
        ctl::ComPtr<SolidColorBrush> spSolidColorBrush;
        wu::Color color = {};

        IFC(ctl::make(&spSolidColorBrush));
        if (userBackgroundOpacity != wm::ClosedCaptioning::ClosedCaptionOpacity_Default || userBackgroundColor != wm::ClosedCaptioning::ClosedCaptionColor_Default)
        {
            // If the user has a not default setting, it has the highest priority
            color = ConvertUserColor(userBackgroundColor, userBackgroundOpacity);
        }
        else
        {
            if (pStyle)
            {
                IFC(pStyle->get_Background(&color));
            }
            else
            {
                color.R = 0;
                color.G = 0;
                color.B = 0;
                color.A = 0;
            }
        }

        IFC(spSolidColorBrush->put_Color(color));
        IFC(pCueElement->put_Background(spSolidColorBrush.Get()));
    }

    // LineAlignment
    {
        wmc::TimedTextLineAlignment alignment = {};

        if (pStyle)
        {
            IFC(pStyle->get_LineAlignment(&alignment));
        }
        else
        {
            alignment = wmc::TimedTextLineAlignment_Start;
        }

        // XAML already treats left/right based on the flow direction, no need to convert the start/end
        switch (alignment)
        {
        case wmc::TimedTextLineAlignment_Start:
            IFC(pCueElement->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Left));
            break;
        case wmc::TimedTextLineAlignment_Center:
            IFC(pCueElement->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Center));
            break;
        case wmc::TimedTextLineAlignment_End:
            IFC(pCueElement->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Right));
            break;
        }
    }

    // Line wrapping
    {
        wmc::TimedTextWrapping wrapping = {};

        if (pRegion)
        {
            IFC(pRegion->get_TextWrapping(&wrapping));
        }
        else
        {
            wrapping = wmc::TimedTextWrapping_Wrap;
        }

        switch (wrapping)
        {
        case wmc::TimedTextWrapping_NoWrap:
            IFC(pTextBlock->put_TextWrapping(xaml::TextWrapping::TextWrapping_NoWrap));
            break;
        case wmc::TimedTextWrapping_Wrap:
            IFC(pTextBlock->put_TextWrapping(xaml::TextWrapping::TextWrapping_Wrap));
            break;
        }
    }

Cleanup:
    return hr;
}

//
// Applies closed caption edge effects to a TextBlock containing the closed caption text. Drop shadows can be applied
// for real by putting a CompositionDropShadow on the TextBlock via a LayerVisual. Raised, Depressed, and Uniform are
// emulated by creating copies of the TextBlock via TextBlock.AlphaMask, giving the copies a different color, and
// offsetting them to create the desired elevated/embossed/outline effect.
//
_Check_return_ HRESULT CCueStyler::SetTextBlockEffect(_In_ TextBlock* pTextBlock, _In_ Canvas* underTextBlock, _In_ Grid* textContainer, _In_ Grid* topWrapper)
{
    if (!m_compositor)
    {
        wrl::ComPtr<WUComp::ILayerVisual> handoffVisual;
        IFC_RETURN(static_cast<CUIElement*>(pTextBlock->GetHandle())->GetHandOffLayerVisual(handoffVisual.ReleaseAndGetAddressOf()));

        wrl::ComPtr<WUComp::ICompositionObject> co;
        IFC_RETURN(handoffVisual.As(&co));
        IFC_RETURN(co->get_Compositor(&m_compositor));
        IFCFAILFAST(m_compositor.As(&m_compositor2));
    }

    // The look of each edge effect varies based on what the closed caption text size is.
    wm::ClosedCaptioning::ClosedCaptionSize userFontSize = wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_Default;
    wm::ClosedCaptioning::ClosedCaptionEdgeEffect userEdgeEffect = wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_None;
    if (m_pSettingsStatics)
    {
        // IClosedCaptionPropertiesStatics can return E_ACCESSDENIED if the registry key that backs the closed
        // caption settings doesn't have the correct permissions set. Tolerate those errors and just go with
        // default values instead.
        HRESULT settingsHR = S_OK;
        settingsHR = m_pSettingsStatics->get_FontSize(&userFontSize);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC_RETURN(settingsHR);
        }
        settingsHR = m_pSettingsStatics->get_FontEffect(&userEdgeEffect);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC_RETURN(settingsHR);
        }
    }

    switch (userFontSize)
    {
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_Default:
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredPercent:
            IFC_RETURN(textContainer->put_Margin({ 6, 6, 6, 6 }));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_FiftyPercent:
            IFC_RETURN(textContainer->put_Margin({ 4, 4, 4, 4 }));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredFiftyPercent:
            IFC_RETURN(textContainer->put_Margin({ 8, 8, 8, 8 }));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_TwoHundredPercent:
            IFC_RETURN(textContainer->put_Margin({ 12, 12, 12, 12 }));
            break;
    }

    if (userEdgeEffect == wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_None)
    {
        // Nothing to do
    }
    else if (userEdgeEffect == wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_DropShadow)
    {
        IFC_RETURN(ApplyDropShadowStyle(pTextBlock, userFontSize));
    }
    else
    {
        // Create copies of the text via a TextBlock.AlphaMask. This lets us avoid all the layout and text rendering
        // costs for all the copies.
        wrl::ComPtr<WUComp::ICompositionBrush> surfaceBrush;
        IFC_RETURN(pTextBlock->GetAlphaMask(surfaceBrush.ReleaseAndGetAddressOf()));

        // Put the copies in the tree via a UIElement hand-in visual. We can't use the TextBlock as the UIElement
        // because hand-in visuals render on top of Xaml content and we want these copies to go underneath the Xaml
        // rendered text. So we ask the caller to supply a UIElement under the TextBlock visual (underTextBlock).
        // We're also allowed only one hand-in visual, so make it a ContainerVisual and put all the SpriteVisuals
        // inside it.
        wrl::ComPtr<WUComp::IContainerVisual> containerVisual;
        wrl::ComPtr<WUComp::IVisual> containerVisualAsVisual;
        IFC_RETURN(m_compositor->CreateContainerVisual(&containerVisual));
        IFCFAILFAST(containerVisual.As(&containerVisualAsVisual));
        IFC_RETURN(static_cast<CUIElement*>(underTextBlock->GetHandle())->SetHandInVisual(containerVisualAsVisual.Get()));

        wrl::ComPtr<WUComp::IVisualCollection> children;
        containerVisual->get_Children(&children);

        // A SpriteVisual still requires an explicit size, even when rendering with an alpha mask. Use the same size
        // as the closed caption TextBlock. That requires running layout first. Whenever the text changes, we'll call
        // back into this method to get the updated size of the TextBlock. The alternative is to create an expression
        // that binds the copy's size to the size of the TextBlock visual, but that's overkill considering the TextBlock
        // isn't animated.
        IFC_RETURN(pTextBlock->UpdateLayout());
        wfn::Vector2 textSize;
        IFC_RETURN(pTextBlock->get_ActualSize(&textSize));

        // Apply the edge effect here. We pass down the ContainerVisual that will contain all the text copies.
        switch (userEdgeEffect)
        {
            case wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_None:
            case wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_DropShadow:
                // Should already be handled above
                FAIL_FAST_ASSERT(false);
                break;

            case wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_Raised:
                IFC_RETURN(ApplyRaisedStyle(surfaceBrush.Get(), textSize, children.Get(), userFontSize));
                break;

            case wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_Depressed:
                IFC_RETURN(ApplyDepressedStyle(surfaceBrush.Get(), textSize, children.Get(), userFontSize));
                break;

            case wm::ClosedCaptioning::ClosedCaptionEdgeEffect::ClosedCaptionEdgeEffect_Uniform:
                IFC_RETURN(ApplyUniformStyle(surfaceBrush.Get(), textSize, children.Get(), userFontSize));
                break;
        }
    }

    // Create a CompositionVisualSurface (CVS) through which we will render caption text/edge effects with group opacity.
    // The resulting rendering redirection supports efficient group opacity and is used to to apply Foreground Opacity.
    // We capture a COMP "snapshot" of the text + edge effects subtree and render it on a handin visual above.
    // To avoid duplicated rendering (i.e TextBlock/Effect Visuals themselves and their CVS snapshot) we introduce three 
    // wrapper containers, which function as follows:
    //
    //
    //     CueElement [Grid]      <-- (Existing) Container for caption, draws the BG color + caption text, applies margins around text
    //          |
    //    TopWrapper [Grid]       <-- Placeholder for rendering CVS snapshot. Receives hand-in Visual filled with CVS brush.
    //          |                          NOTE: Hand-in must be above the 0x0 Clip (or it will itself be clipped) necessitating this wrapper.
    //          |
    //    MiddleWrapper [Grid]    <-- Placeholder for 0x0 Clip. The clip culls TextContainer from the tree (preventing duplicated rendering)
    //          |                          NOTE: Normally, a 0x0 Clip will cull the element's subtree from RenderWalk, preventing the CVS from capturing the text/effects.
    //          |                                Having a handoff is one condition that prevents the culling, so take a (dummy) handoff from MiddleWrapper.
    //          |      
    //    BottomWrapper [Grid]    <-- Placeholder for sourcing CVS Snapshot. Provides handoff Visual used as CVS.SourceVisual
    //          |                          NOTE: Captures layout (eg margins) of TextContainer inside CueElement, so that snapshot size matches TopWrapper 
    //          |                                and we don't need speical offsets/positioning of handin w/ CVS.
    //          |                          NOTE: CVS Handoff needs to be from a different element than the one that has the Clip set,
    //          |                                otherwise it will capture the zero-sized clip and render nothing (this necessitates introduction of 'MiddleWrapper').
    //          |                      
    //     TextContainer [Grid]   <-- (Existing) Container for caption TextBlock + Edge Effects visuals

    wrl::ComPtr<WUComp::ICompositionVisualSurface> captionCVS;
    wrl::ComPtr<WUComp::ICompositorWithVisualSurface> compositorVisualSurface;
    IFC_RETURN(m_compositor->QueryInterface(IID_PPV_ARGS(&compositorVisualSurface)));
    IFC_RETURN(compositorVisualSurface->CreateVisualSurface(&captionCVS));

    // Get wrapper containers so they can be configured for CVS rendering (at TopWrapper)
    ctl::ComPtr<xaml::IUIElement> spUIE;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    IFC_RETURN(topWrapper->get_Children(&spChildren));
    IFC_RETURN(spChildren->GetAt(0, &spUIE));
    ctl::ComPtr<Grid> middleWrapper = spUIE.Cast<Grid>();
    IFC_RETURN(middleWrapper->get_Children(&spChildren));
    IFC_RETURN(spChildren->GetAt(0, &spUIE));
    ctl::ComPtr<Grid> bottomWrapper = spUIE.Cast<Grid>();

    // Get TopWrapper's ActualSize to size CVS / Hosting Visual
    wfn::Vector2 topWrapperVisualSize;
    IFC_RETURN(topWrapper->get_ActualSize(&topWrapperVisualSize));
    captionCVS->put_SourceSize(topWrapperVisualSize);

    // Create SpriteVisual & fill it with CompositionSurfaceBrush obtained from CVS
    wrl::ComPtr<WUComp::ICompositionSurfaceBrush> captionSurfaceBrush;
    wrl::ComPtr<WUComp::ICompositionSurface> captionCompositionSurface;
    IFCFAILFAST(captionCVS.As(&captionCompositionSurface));
    IFCFAILFAST(m_compositor->CreateSurfaceBrushWithSurface(captionCompositionSurface.Get(), &captionSurfaceBrush));
    captionSurfaceBrush->put_Stretch(WUComp::CompositionStretch::CompositionStretch_Fill);
    wrl::ComPtr<WUComp::ICompositionBrush> captionCompositionBrush;
    IFCFAILFAST(captionSurfaceBrush.As(&captionCompositionBrush));
    wrl::ComPtr<WUComp::ISpriteVisual> renderCaptionVisual;
    wrl::ComPtr<WUComp::IVisual> renderCaptionVisualAsVisual;
    IFC_RETURN(m_compositor->CreateSpriteVisual(&renderCaptionVisual));
    IFCFAILFAST(renderCaptionVisual.As(&renderCaptionVisualAsVisual));
    IFC_RETURN(renderCaptionVisualAsVisual->put_Size(topWrapperVisualSize));
    IFC_RETURN(renderCaptionVisual->put_Brush(captionCompositionBrush.Get()));

    //
    // Configure wrappers to hook up CVS snapshot rendering at TopWrapper
    //

    // MiddleWrapper: Get dummy handoff to force Xaml rendering of subtree despite 0x0 Clip
    //                (Note that we don't actually need to hold a reference on the hand off visual.
    //                 Xaml will know that somebody requested a hand off visual and keep the visual around 
    //                 for as long as the element is alive; there's no API to "return" a hand off visual and tell Xaml.)
    wrl::ComPtr<WUComp::IVisual> middleWrapperHandoffVisual;
    IFC_RETURN(middleWrapper->GetElementVisual(&middleWrapperHandoffVisual));

    // BottomWrapper: Get handoff visual for snapshot source (CVS.SourceVisual)
    wrl::ComPtr<WUComp::IVisual> bottomWrapperHandoffVisual;
    IFC_RETURN(bottomWrapper->GetElementVisual(&bottomWrapperHandoffVisual));
    captionCVS->put_SourceVisual(bottomWrapperHandoffVisual.Get());

    // TopWrapper: Set handin to our new SpriteVisual filled with CVS brush
    IFC_RETURN(static_cast<CUIElement*>(topWrapper->GetHandle())->SetHandInVisual(renderCaptionVisualAsVisual.Get()));

    // Apply Foreground Opacity. CVS.SourceVisual is filled with a snapshot of BottomWrapper subtree (which includes 
    // TextContainer/TextBlock/Edge Effects), so setting the SourceVisual's Opacity achieves the desired effect.
    wm::ClosedCaptioning::ClosedCaptionOpacity userForegroundOpacity = wm::ClosedCaptioning::ClosedCaptionOpacity::ClosedCaptionOpacity_Default; 
    if (m_pSettingsStatics)
    { 
        // IClosedCaptionPropertiesStatics can return E_ACCESSDENIED if the registry key that backs the closed
        // caption settings doesn't have the correct permissions set. Tolerate those errors and just go with
        // default values instead.
        HRESULT settingsHR = S_OK;
        settingsHR = m_pSettingsStatics->get_FontOpacity(&userForegroundOpacity);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC_RETURN(settingsHR);
        }
    }    
    if (userForegroundOpacity != wm::ClosedCaptioning::ClosedCaptionOpacity_Default &&
        userForegroundOpacity != wm::ClosedCaptioning::ClosedCaptionOpacity_OneHundredPercent)
    {
        switch (userForegroundOpacity)
        {
        case wm::ClosedCaptioning::ClosedCaptionOpacity::ClosedCaptionOpacity_SeventyFivePercent:
            IFC_RETURN(renderCaptionVisualAsVisual->put_Opacity(0.75f));
            break;
        case wm::ClosedCaptioning::ClosedCaptionOpacity::ClosedCaptionOpacity_TwentyFivePercent:
            IFC_RETURN(renderCaptionVisualAsVisual->put_Opacity(0.25f));
            break;
        case wm::ClosedCaptioning::ClosedCaptionOpacity::ClosedCaptionOpacity_ZeroPercent:
            IFC_RETURN(renderCaptionVisualAsVisual->put_Opacity(0.0f));
            break;
        default:
            ASSERT(false); // unknown value received
            break;
        }
    }

    return S_OK;
}

wrl::ComPtr<WUComp::ICompositionBrush> CCueStyler::CreateMaskBrush(_In_ WUComp::ICompositionBrush* surfaceBrush, _In_ WUComp::ICompositionBrush* colorBrush)
{
    wrl::ComPtr<WUComp::ICompositionMaskBrush> maskBrush;
    IFCFAILFAST(m_compositor2->CreateMaskBrush(maskBrush.ReleaseAndGetAddressOf()));
    IFCFAILFAST(maskBrush->put_Mask(surfaceBrush));
    IFCFAILFAST(maskBrush->put_Source(colorBrush));

    wrl::ComPtr<WUComp::ICompositionBrush> brush;
    IFCFAILFAST(maskBrush.As(&brush));
    return brush;
}

_Check_return_ HRESULT CCueStyler::ApplyRaisedStyle(_In_ WUComp::ICompositionBrush* alphaMask, wfn::Vector2 textSize, _In_ WUComp::IVisualCollection* children, wm::ClosedCaptioning::ClosedCaptionSize userFontSize)
{
    if (!m_whiteBrush60)
    {
        wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
        IFC_RETURN(m_compositor->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
        IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(0x99ffffff)));
        IFCFAILFAST(colorBrush.As(&m_whiteBrush60));
    }

    if (!m_blackBrush)
    {
        wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
        IFC_RETURN(m_compositor->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
        IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(0xff000000)));
        IFCFAILFAST(colorBrush.As(&m_blackBrush));
    }

    wrl::ComPtr<WUComp::ICompositionBrush> lightBrush = CreateMaskBrush(alphaMask, m_whiteBrush60.Get());
    wrl::ComPtr<WUComp::ICompositionBrush> darkBrush = CreateMaskBrush(alphaMask, m_blackBrush.Get());

    switch (userFontSize)
    {
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_FiftyPercent:
            // text-shadow: -0.5px -0.5px 0 white 60%; 1px 1px 1 black
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { -0.5, -0.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1, 1, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_Default:
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredPercent:
            // text-shadow: -1px -1px 0 white 60%; 1px 1px 1 black
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { -1, -1, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1, 1, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredFiftyPercent:
            // text-shadow: -2px -2px 0 white 60%, 1px 1px 1 black, 2px 2px 2 black
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { -2, -2, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1, 1, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 2, 2, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_TwoHundredPercent:
            // text-shadow: -2px -2px 0 white 60%, 2px 2px 2 black, 4px 4px 4 black
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { -2, -2, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 2, 2, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 4, 4, 0 }, children));
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CCueStyler::ApplyDepressedStyle(_In_ WUComp::ICompositionBrush* alphaMask, wfn::Vector2 textSize, _In_ WUComp::IVisualCollection* children, wm::ClosedCaptioning::ClosedCaptionSize userFontSize)
{
    if (!m_blackBrush20)
    {
        wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
        IFC_RETURN(m_compositor->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
        IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(0x33000000)));
        IFCFAILFAST(colorBrush.As(&m_blackBrush20));
    }

    if (!m_blackBrush)
    {
        wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
        IFC_RETURN(m_compositor->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
        IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(0xff000000)));
        IFCFAILFAST(colorBrush.As(&m_blackBrush));
    }

    wrl::ComPtr<WUComp::ICompositionBrush> lightBrush = CreateMaskBrush(alphaMask, m_blackBrush20.Get());
    wrl::ComPtr<WUComp::ICompositionBrush> darkBrush = CreateMaskBrush(alphaMask, m_blackBrush.Get());

    switch (userFontSize)
    {
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_FiftyPercent:
            // text-shadow: -1px -1px 0 black, 1px 1px 0 black 20%
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1, -1, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { 1, 1, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_Default:
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredPercent:
            // text-shadow: -2px -2px 0 black, 2px 2px 0 black 20%
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -2, -2, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { 2, 2, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredFiftyPercent:
            // text-shadow: -2.5px -2.5px 0 black, 2.5px 2.5px 0 black 20%
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -2.5, -2.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { 2.5, 2.5, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_TwoHundredPercent:
            // text-shadow: -3px -3px 0 black, 3px 3px 0 black 20%
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -3, -3, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(lightBrush.Get(), textSize, { 3, 3, 0 }, children));
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CCueStyler::ApplyUniformStyle(_In_ WUComp::ICompositionBrush* alphaMask, wfn::Vector2 textSize, _In_ WUComp::IVisualCollection* children, wm::ClosedCaptioning::ClosedCaptionSize userFontSize)
{
    if (!m_blackBrush)
    {
        wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
        IFC_RETURN(m_compositor->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
        IFC_RETURN(colorBrush->put_Color(ColorUtils::GetWUColor(0xff000000)));
        IFCFAILFAST(colorBrush.As(&m_blackBrush));
    }

    wrl::ComPtr<WUComp::ICompositionBrush> darkBrush = CreateMaskBrush(alphaMask, m_blackBrush.Get());

    switch (userFontSize)
    {
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_FiftyPercent:
            // outline-width: 0.5px
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -0.5, -0.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 0.5, -0.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -0.5, 0.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 0.5, 0.5, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_Default:
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredPercent:
            // outline-width: 1px - fall through
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredFiftyPercent:
            // outline-width: 1px
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1, -1, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 0, -1, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1, -1, 0 }, children));

            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1, 0, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1, 0, 0 }, children));

            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1, 1, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 0, 1, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1, 1, 0 }, children));
            break;
        case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_TwoHundredPercent:
            // outline-width: 1.5px
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1.5, -1.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -0.5, -1.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 0.5, -1.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1.5, -1.5, 0 }, children));

            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1.5, -0.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1.5, -0.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1.5, 0.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1.5, 0.5, 0 }, children));

            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -1.5, 1.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { -0.5, 1.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 0.5, 1.5, 0 }, children));
            IFC_RETURN(CreateVisualAndAdd(darkBrush.Get(), textSize, { 1.5, 1.5, 0 }, children));
            break;
    }

    return S_OK;
}

_Check_return_ HRESULT CCueStyler::ApplyDropShadowStyle(_In_ TextBlock* pTextBlock, wm::ClosedCaptioning::ClosedCaptionSize userFontSize)
{
    if (!m_dropShadow)
    {
        wrl::ComPtr<WUComp::IDropShadow2> dropShadow2;
        IFC_RETURN(m_compositor2->CreateDropShadow(&m_dropShadow));
        IFC_RETURN(m_dropShadow->put_Color(ColorUtils::GetWUColor(0xff000000)));
        IFCFAILFAST(m_dropShadow.As(&dropShadow2));
        IFC_RETURN(dropShadow2->put_SourcePolicy(WUComp::CompositionDropShadowSourcePolicy_InheritFromVisualContent));
        IFCFAILFAST(m_dropShadow.As(&m_compositionShadow));
    }

    wrl::ComPtr<WUComp::ILayerVisual> handoffVisual;
    wrl::ComPtr<WUComp::ILayerVisual2> layerVisual2;
    IFC_RETURN(static_cast<CUIElement*>(pTextBlock->GetHandle())->GetHandOffLayerVisual(handoffVisual.ReleaseAndGetAddressOf()));
    IFCFAILFAST(handoffVisual.As(&layerVisual2));
    IFC_RETURN(layerVisual2->put_Shadow(m_compositionShadow.Get()));

    // Composition drop shadows have an offset and a radius, and can't be directly translated from
    // the css specs from design. These are the values that look correct according to the pictures.
    switch (userFontSize)
    {
    case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_FiftyPercent:
        // text-shadow: 0 2px 2 black 25%, 0 1px 1 black 50%
        IFC_RETURN(m_dropShadow->put_BlurRadius(5.0f));
        IFC_RETURN(m_dropShadow->put_Offset({ 0, 4, 0 }));
        break;
    case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_Default:
    case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredPercent:
        // text-shadow: 0 5px 3 black, 0 2px 2 black 50%
        IFC_RETURN(m_dropShadow->put_BlurRadius(6.0f));
        IFC_RETURN(m_dropShadow->put_Offset({ 0, 7, 0 }));
        break;
    case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_OneHundredFiftyPercent:
        // text-shadow: 0 5px 3 black, 0 4px 4 black 50%
        IFC_RETURN(m_dropShadow->put_BlurRadius(7.0f));
        IFC_RETURN(m_dropShadow->put_Offset({ 0, 8, 0 }));
        break;
    case wm::ClosedCaptioning::ClosedCaptionSize::ClosedCaptionSize_TwoHundredPercent:
        // text-shadow: 0 6px 6 black, 0 4px 4 black 50%
        IFC_RETURN(m_dropShadow->put_BlurRadius(11.0f));
        IFC_RETURN(m_dropShadow->put_Offset({ 0, 8, 0 }));
        break;
    }

    return S_OK;
}

void CCueStyler::CleanupDeviceRelatedResources(const bool cleanupDComp)
{
    if (cleanupDComp)
    {
        m_compositor.Reset();
        m_compositor2.Reset();
        m_blackBrush.Reset();
        m_blackBrush20.Reset();
        m_whiteBrush60.Reset();
        m_dropShadow.Reset();
        m_compositionShadow.Reset();
    }
}

_Check_return_ HRESULT CCueStyler::CreateVisualAndAdd(
    _In_ WUComp::ICompositionBrush* brush,
    const wfn::Vector2 textSize,
    wfn::Vector3 offset,
    _In_ WUComp::IVisualCollection* children)
{
    wrl::ComPtr<WUComp::ISpriteVisual> spriteVisual;
    IFC_RETURN(m_compositor->CreateSpriteVisual(&spriteVisual));
    IFC_RETURN(spriteVisual->put_Brush(brush));

    wrl::ComPtr<WUComp::IVisual> visual;
    IFCFAILFAST(spriteVisual.As(&visual));
    IFC_RETURN(visual->put_Size(textSize));
    IFC_RETURN(visual->put_Offset(offset));
    IFC_RETURN(children->InsertAtTop(visual.Get()));

    return S_OK;
}

_Check_return_ HRESULT
CCueStyler::CreateRun(_In_ wmc::ITimedTextLine* pLine,
                      _In_ wmc::ITimedTextStyle* pStyle,
                      _In_ int start,
                      _In_ int length,
                      _In_ bool isOutline,
                      _Outptr_ IRun** ppRun) noexcept
{
    ctl::ComPtr<Run> spRun;

    UNREFERENCED_PARAMETER(start);
    UNREFERENCED_PARAMETER(length);

    IFCPTR_RETURN(ppRun);
    *ppRun = nullptr;

    IFC_RETURN(ctl::make(&spRun));

    wm::ClosedCaptioning::ClosedCaptionOpacity userForegroundOpacity = wm::ClosedCaptioning::ClosedCaptionOpacity_Default;
    wm::ClosedCaptioning::ClosedCaptionColor userForegroundColor = wm::ClosedCaptioning::ClosedCaptionColor_Default;
    wm::ClosedCaptioning::ClosedCaptionStyle userFontStyle = wm::ClosedCaptioning::ClosedCaptionStyle_Default;
    if (m_pSettingsStatics)
    {
        // IClosedCaptionPropertiesStatics can return E_ACCESSDENIED if the registry key that backs the closed
        // caption settings doesn't have the correct permissions set. Tolerate those errors and just go with
        // default values instead.
        HRESULT settingsHR = S_OK;
        settingsHR = m_pSettingsStatics->get_FontColor(&userForegroundColor);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC_RETURN(settingsHR);
        }
        settingsHR = m_pSettingsStatics->get_FontOpacity(&userForegroundOpacity);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC_RETURN(settingsHR);
        }
        settingsHR = m_pSettingsStatics->get_FontStyle(&userFontStyle);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC_RETURN(settingsHR);
        }
    }

    // Text, if length is -1 then use the whole text string
    if (length == -1)
    {
        wrl_wrappers::HString text;

        IFC_RETURN(pLine->get_Text(text.ReleaseAndGetAddressOf()));
        IFC_RETURN(spRun->put_Text(text.Get()));
    }
    else
    {
        wrl_wrappers::HString text;
        wrl_wrappers::HString hSubText;

        IFC_RETURN(pLine->get_Text(text.ReleaseAndGetAddressOf()));
        IFC_RETURN(text.Substring(start, length, hSubText));
        IFC_RETURN(spRun->put_Text(hSubText.Get()));
    }

    // FontFamily
    {
        wrl_wrappers::HString fontFamilyName;
        ctl::ComPtr<xaml_media::IFontFamily> spFontFamily;

        if (pStyle)
        {
            IFC_RETURN(pStyle->get_FontFamily(fontFamilyName.ReleaseAndGetAddressOf()));
        }
        else
        {
            wrl_wrappers::HStringReference(L"default").CopyTo(fontFamilyName.ReleaseAndGetAddressOf());
        }

        // If the font is set to small caps (by user override or by file with no user override)
        // we need to set the small caps typography property as well.
        // Must read fontFamilyName here before it is converted to the real font
        if (userFontStyle == wm::ClosedCaptioning::ClosedCaptionStyle_SmallCapitals ||
            (userFontStyle == wm::ClosedCaptioning::ClosedCaptionStyle_Default && _wcsicmp(fontFamilyName.GetRawBuffer(nullptr), L"smallCaps") == 0))
        {
            CValue value;
            value.Set(DirectUI::FontCapitals::SmallCaps);
            IFC_RETURN(spRun->SetValueByKnownIndex(KnownPropertyIndex::Typography_Capitals, value));
        }

        if (userFontStyle == wm::ClosedCaptioning::ClosedCaptionStyle_Default)
        {
            fontFamilyName.Set(CCueStyler::ConvertUserFontStyle(fontFamilyName.GetRawBuffer(nullptr)).Get());
        }
        else
        {
            fontFamilyName.Set(CCueStyler::ConvertUserFontStyle(userFontStyle).Get());
        }

        // If the user has a non-default setting, it has the highest priority
        if (!m_spFontFamilyFactory)
        {
            IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_FontFamily).Get(), m_spFontFamilyFactory.ReleaseAndGetAddressOf()));
        }
        IFC_RETURN(m_spFontFamilyFactory->CreateInstanceWithName(fontFamilyName.Get(), nullptr, nullptr, &spFontFamily));

        IFC_RETURN(spRun->put_FontFamily(spFontFamily.Get()));
    }

    // FontSize
    {
        double actualSize = {};
        if (pStyle)
        {
            IFC_RETURN(CalculateFontSize(pStyle, &actualSize));
        }
        else
        {
            actualSize = (m_videoHeight / (100.0 / DefaultFontSize)) * m_fontScaleFactor; // default font size should be 5% of video height
        }
        IFC_RETURN(spRun->put_FontSize(actualSize));
    }

    // FontWeight
    {
        wmc::TimedTextWeight fontWeight = {};
        wut::FontWeight uiFontWeight = {};

        if (pStyle)
        {
            IFC_RETURN(pStyle->get_FontWeight(&fontWeight));
        }
        else
        {
            fontWeight = wmc::TimedTextWeight_Normal;
        }

        if (m_spFontWeightsStatics == nullptr)
        {
            IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Text_FontWeights).Get(), m_spFontWeightsStatics.ReleaseAndGetAddressOf()));
        }

        switch (fontWeight)
        {
        case wmc::TimedTextWeight_Normal:
            IFC_RETURN(m_spFontWeightsStatics->get_Normal(&uiFontWeight));
            break;
        case wmc::TimedTextWeight_Bold:
            IFC_RETURN(m_spFontWeightsStatics->get_Bold(&uiFontWeight));
            break;
        }

        IFC_RETURN(spRun->put_FontWeight(uiFontWeight));
    }

    // Colors
    {
        ctl::ComPtr<SolidColorBrush> spSolidColorBrush;
        wu::Color color = {};

        IFC_RETURN(ctl::make(&spSolidColorBrush));

        // Set foreground color, for outlines the color is always black, for foreground the default is white
        if (isOutline)
        {
            if (pStyle)
            {
                IFC_RETURN(pStyle->get_OutlineColor(&color));
            }
            else
            {
                color.R = 0;
                color.G = 0;
                color.B = 0;
                color.A = 255;
            }
            IFC_RETURN(spSolidColorBrush->put_Color(color));
        }
        else
        {
            if (userForegroundOpacity != wm::ClosedCaptioning::ClosedCaptionOpacity_Default || userForegroundColor != wm::ClosedCaptioning::ClosedCaptionColor_Default)
            {
                // If the user has a not default setting, it has the highest priority
                // Note: In case there are edge effects, we apply opacity as a layer opacity on a text container
                // element. Don't blend the opacity into the foreground brush here. We'll set the opacity later in
                // SetTextBlockEffect.
                wu::Color c = ConvertUserColor(userForegroundColor, wm::ClosedCaptioning::ClosedCaptionOpacity_Default);
                IFC_RETURN(spSolidColorBrush->put_Color(c));
            }
            else
            {
                if (pStyle)
                {
                    IFC_RETURN(pStyle->get_Foreground(&color));
                }
                else
                {
                    color.R = 255;
                    color.G = 255;
                    color.B = 255;
                    color.A = 255;
                }

                IFC_RETURN(spSolidColorBrush->put_Color(color));
            }
        }

        IFC_RETURN(spRun->put_Foreground(spSolidColorBrush.Get()));
    }

    // FlowDirection
    {
        wmc::TimedTextFlowDirection flow = {};

        if (pStyle)
        {
            IFC_RETURN(pStyle->get_FlowDirection(&flow));
        }
        else
        {
            flow = wmc::TimedTextFlowDirection_LeftToRight;
        }

        switch (flow)
        {
        case wmc::TimedTextFlowDirection_LeftToRight:
            IFC_RETURN(spRun->put_FlowDirection(xaml::FlowDirection::FlowDirection_LeftToRight));
            break;
        case wmc::TimedTextFlowDirection_RightToLeft:
            IFC_RETURN(spRun->put_FlowDirection(xaml::FlowDirection::FlowDirection_RightToLeft));
            break;
        }
    }

    if (pStyle)
    {
        ctl::ComPtr<wmc::ITimedTextStyle2> spStyle2;
        IFCFAILFAST(pStyle->QueryInterface(IID_PPV_ARGS(&spStyle2)));

        // FontStyle
        wmc::TimedTextFontStyle timedTextFontStyle;
        IFC_RETURN(spStyle2->get_FontStyle(&timedTextFontStyle));
        switch (timedTextFontStyle)
        {
        case wmc::TimedTextFontStyle_Oblique:
            IFC_RETURN(spRun->put_FontStyle(wut::FontStyle::FontStyle_Oblique));
            break;
        case wmc::TimedTextFontStyle_Italic:
            IFC_RETURN(spRun->put_FontStyle(wut::FontStyle::FontStyle_Italic));
            break;
        }

        // TextDecorations
        wut::TextDecorations textDecorations = wut::TextDecorations::TextDecorations_None;
        boolean fUnderLine;
        IFC_RETURN(spStyle2->get_IsUnderlineEnabled(&fUnderLine));
        if (fUnderLine)
        {
            textDecorations |= wut::TextDecorations::TextDecorations_Underline;
        }

        boolean fStrikethrough;
        IFC_RETURN(spStyle2->get_IsLineThroughEnabled(&fStrikethrough));
        if (fStrikethrough)
        {
            textDecorations |= wut::TextDecorations::TextDecorations_Strikethrough;
        }

        // ToDo: wut::TextDecorations::TextDecorations_Overline is missing
        // Bug 9855314
        IFC_RETURN(spRun->put_TextDecorations(textDecorations));
    }

    IFC_RETURN(spRun.CopyTo(ppRun));

    return S_OK;
}

// This mimics a text outline.
// TODO: This is not ideal but there is currently no native support
// for this. XAML really needs to add a border functionality around text to
// improve this performance
_Check_return_ HRESULT
CCueStyler::CreateOutline(_In_ wmc::ITimedTextLine* pLine,
                          _In_ wmc::ITimedTextStyle* pStyle,
                          _In_ wmc::ITimedTextRegion* pRegion,
                          _In_ Grid* pCueElement)
{
    HRESULT hr = S_OK;

    wmc::TimedTextDouble thickness = {};
    double fontSize = {};
    double offset = {};

    IFC(CalculateFontSize(pStyle, &fontSize));

    IFC(pStyle->get_OutlineThickness(&thickness));

    if (thickness.Unit == wmc::TimedTextUnit_Percentage)
    {
        offset = (thickness.Value / 100) * fontSize;
    }
    else
    {
        offset = thickness.Value;
    }

    // Upper left
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, -offset, -offset));

    // Upper center
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, 0, -offset));

    // Upper right
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, offset, -offset));

    // Left
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, -offset, 0));

    // Right
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, offset, 0));

    // Lower left
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, -offset, offset));

    // Lower center
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, 0, offset));

    // Lower right
    IFC(CreateOutlineHelper(pLine, pStyle, pRegion, pCueElement, offset, offset));

Cleanup:
    return hr;
}

// Helper that creates a textblock, styles it, positions it and adds it to the parent
// container
_Check_return_ HRESULT
CCueStyler::CreateOutlineHelper(_In_ wmc::ITimedTextLine* pLine,
                                _In_ wmc::ITimedTextStyle* pStyle,
                                _In_ wmc::ITimedTextRegion* pRegion,
                                _In_ Grid* pCueElement,
                                _In_ double x,
                                _In_ double y)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<TextBlock> spTextBlock;
    ctl::ComPtr<TranslateTransform> spTransform;
    ctl::ComPtr<xaml_media::ITranslateTransform> spITransform;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC(pCueElement->get_Children(&spChildren));

    IFC(ctl::make(&spTextBlock));
    IFC(SetTextBlock(pLine, pStyle, spTextBlock.Get(), pRegion, pCueElement, true));

    IFC(ctl::make(&spTransform));
    IFC(spTextBlock->put_RenderTransform(spTransform.Get()));
    spITransform = spTransform.AsOrNull<xaml_media::ITranslateTransform>();
    ASSERT(spITransform != nullptr);
    IFC(spITransform->put_X(x));
    IFC(spITransform->put_Y(y));
    IFC(spChildren->Append(spTextBlock.Get()));

Cleanup:
    return hr;
}

// Converts the caption color into a XAML usable color
wu::Color
CCueStyler::ConvertUserColor(_In_ wm::ClosedCaptioning::ClosedCaptionColor color, _In_ wm::ClosedCaptioning::ClosedCaptionOpacity opacity)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES

    wu::Color retColor = {255, 255, 255, 255}; // default to white

    ctl::ComPtr<ABI::Microsoft::UI::IColorsStatics> spColorsStatics;
    IFC(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Colors).Get(), spColorsStatics.ReleaseAndGetAddressOf()));

    switch (color)
    {
    case wm::ClosedCaptioning::ClosedCaptionColor_Default:
        // default is white, fall through
    case wm::ClosedCaptioning::ClosedCaptionColor_White:
        IFC(spColorsStatics->get_White(&retColor));
        break;
    case wm::ClosedCaptioning::ClosedCaptionColor_Black:
        IFC(spColorsStatics->get_Black(&retColor));
        break;
    case wm::ClosedCaptioning::ClosedCaptionColor_Red:
        IFC(spColorsStatics->get_Red(&retColor));
        break;
    case wm::ClosedCaptioning::ClosedCaptionColor_Green:
        IFC(spColorsStatics->get_Lime(&retColor)); // FCC requirement: map green to exactly 0x00FF00 which is Lime in WinRT
        break;
    case wm::ClosedCaptioning::ClosedCaptionColor_Blue:
        IFC(spColorsStatics->get_Blue(&retColor));
        break;
    case wm::ClosedCaptioning::ClosedCaptionColor_Yellow:
        IFC(spColorsStatics->get_Yellow(&retColor));
        break;
    case wm::ClosedCaptioning::ClosedCaptionColor_Magenta:
        IFC(spColorsStatics->get_Magenta(&retColor));
        break;
    case wm::ClosedCaptioning::ClosedCaptionColor_Cyan:
        IFC(spColorsStatics->get_Cyan(&retColor));
        break;
    default:
        ASSERT(false); // unknown value received
        break;
    }

    switch (opacity)
    {
    case wm::ClosedCaptioning::ClosedCaptionOpacity_Default:
        // default is 100%, fall through
    case wm::ClosedCaptioning::ClosedCaptionOpacity_OneHundredPercent:
        retColor.A = 0xFF; // 100% of 255
        break;
    case wm::ClosedCaptioning::ClosedCaptionOpacity_SeventyFivePercent:
        retColor.A = 0xC0; // 75% of 255
        break;
    case wm::ClosedCaptioning::ClosedCaptionOpacity_TwentyFivePercent:
        retColor.A = 0x40; // 25% of 255
        break;
    case wm::ClosedCaptioning::ClosedCaptionOpacity_ZeroPercent:
        retColor.A = 0x00; // 0% of 255
        break;
    default:
        ASSERT(false); // unknown value received
        break;
    }

Cleanup:
    // if any errors just return retColor as it was initialized
    return retColor;
}

// Helper to determine if the font string is a "font style"
// if not it will be treated as a real font family name
bool
CCueStyler::IsW3CFontStyle(LPWSTR fontStyle)
{
    if (fontStyle != nullptr)
    {
        if (_wcsicmp(fontStyle, L"default") == 0 ||
            _wcsicmp(fontStyle, L"monospaceSerif") == 0 ||
            _wcsicmp(fontStyle, L"proportionalSerif") == 0 ||
            _wcsicmp(fontStyle, L"monospaceSansSerif") == 0 ||
            _wcsicmp(fontStyle, L"proportionalSansSerif") == 0 ||
            _wcsicmp(fontStyle, L"casual") == 0 ||
            _wcsicmp(fontStyle, L"cursive") == 0 ||
            _wcsicmp(fontStyle, L"smallCaps") == 0)
        {
            return true;
        }
    }

    return false;
}

// Converts a user string into the captions enum type
// The font family is not required to be your typical "Segoe" or "Times New roman", it
// can be one of the below string which are then converted to be system specific
// http://www.w3.org/TR/ttml10-sdp-us/#Constrained_TTML_Feature_display_display_fontFamily_generic
//
wrl_wrappers::HStringReference
CCueStyler::ConvertUserFontStyle(_In_ LPCWSTR fontStyle)
{
    wm::ClosedCaptioning::ClosedCaptionStyle ccs = wm::ClosedCaptioning::ClosedCaptionStyle_Default;

    if (fontStyle != nullptr)
    {
        if (_wcsicmp(fontStyle, L"default") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_Default;
        }
        else if (_wcsicmp(fontStyle, L"monospaceSerif") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_MonospacedWithSerifs;
        }
        else if (_wcsicmp(fontStyle, L"proportionalSerif") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_ProportionalWithSerifs;
        }
        else if (_wcsicmp(fontStyle, L"monospaceSansSerif") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_MonospacedWithoutSerifs;
        }
        else if (_wcsicmp(fontStyle, L"proportionalSansSerif") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_ProportionalWithoutSerifs;
        }
        else if (_wcsicmp(fontStyle, L"casual") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_Casual;
        }
        else if (_wcsicmp(fontStyle, L"cursive") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_Cursive;
        }
        else if (_wcsicmp(fontStyle, L"smallCaps") == 0)
        {
            ccs = wm::ClosedCaptioning::ClosedCaptionStyle_SmallCapitals;
        }
        else
        {
            // If it is none of the above, we assume that the CC file
            // has set a specific font family to use and will use that
            // Since this function can be called alot we do not validate if
            // the font is actually installed, we let the font system handle
            // picking a default if it is not there.
            return wrl_wrappers::HStringReference(fontStyle);
        }
    }

    return ConvertUserFontStyle(ccs);
}

// Converts the caption style into a platform font
wrl_wrappers::HStringReference
CCueStyler::ConvertUserFontStyle(_In_ wm::ClosedCaptioning::ClosedCaptionStyle fontStyle)
{
    switch (fontStyle)
    {
    case wm::ClosedCaptioning::ClosedCaptionStyle_Default:
        return wrl_wrappers::HStringReference(L"Segoe UI");
        break;
    case wm::ClosedCaptioning::ClosedCaptionStyle_MonospacedWithSerifs:
        return wrl_wrappers::HStringReference(L"Courier New");
        break;
    case wm::ClosedCaptioning::ClosedCaptionStyle_ProportionalWithSerifs:
        return wrl_wrappers::HStringReference(L"Georgia");
        break;
    case wm::ClosedCaptioning::ClosedCaptionStyle_MonospacedWithoutSerifs:
        return wrl_wrappers::HStringReference(L"Lucida Console");
        break;
    case wm::ClosedCaptioning::ClosedCaptionStyle_ProportionalWithoutSerifs:
        return wrl_wrappers::HStringReference(L"Segoe UI");
        break;
    case wm::ClosedCaptioning::ClosedCaptionStyle_Casual:
        return wrl_wrappers::HStringReference(L"Comic Sans MS");
        break;
    case wm::ClosedCaptioning::ClosedCaptionStyle_Cursive:
        return wrl_wrappers::HStringReference(L"Ink Free");
        break;
    case wm::ClosedCaptioning::ClosedCaptionStyle_SmallCapitals:
        return wrl_wrappers::HStringReference(L"Segoe UI");
        break;
    }

    return wrl_wrappers::HStringReference(L"Segoe UI");
}


// Converts the caption size enum into a real value
double
CCueStyler::ConvertUserFontSizeScale(_In_ wm::ClosedCaptioning::ClosedCaptionSize fontSize)
{
    switch (fontSize)
    {
    case wm::ClosedCaptioning::ClosedCaptionSize_Default:
    case wm::ClosedCaptioning::ClosedCaptionSize_OneHundredPercent:
        return 1.0;
    case wm::ClosedCaptioning::ClosedCaptionSize_FiftyPercent:
        return 0.5;
    case wm::ClosedCaptioning::ClosedCaptionSize_OneHundredFiftyPercent:
        return 1.5;
    case wm::ClosedCaptioning::ClosedCaptionSize_TwoHundredPercent:
        return 2.0;
    }

    return 1.0;
}

// Used to store the parent videos natural size
void
CCueStyler::SetNaturalVideoSize(_In_ double height, _In_ double width)
{
    m_videoHeight = height;
    m_videoWidth = width;
}

// The font size must proportionally scale based on the video size
// this calculates what the scale will be, the scale is applied
// when the fonts size is set
void
CCueStyler::SetFontSizeRatio(_In_ double videoActualHeight)
{
    if (videoActualHeight <= 0 || m_videoWidth == 0 || m_videoHeight == 0)
    {
        m_fontScaleFactor = 1.0;
    }
    else
    {
        // Currently only the height is used to determine the scaling factor
        // of the font size
        m_fontScaleFactor = videoActualHeight / m_videoHeight;
    }
}

// Determine what the real font size is based on video scale and user preferences
_Check_return_ HRESULT
CCueStyler::CalculateFontSize(_In_ wmc::ITimedTextStyle* pStyle, _Out_ double* actualSize)
{
    HRESULT hr = S_OK;

    wmc::TimedTextDouble fontSize = {};
    wm::ClosedCaptioning::ClosedCaptionSize userFontSize = wm::ClosedCaptioning::ClosedCaptionSize_Default;

    IFCPTR(actualSize);

    if (m_pSettingsStatics)
    {
        // IClosedCaptionPropertiesStatics can return E_ACCESSDENIED if the registry key that backs the closed
        // caption settings doesn't have the correct permissions set. Tolerate those errors and just go with
        // default values instead.
        HRESULT settingsHR = S_OK;
        settingsHR = m_pSettingsStatics->get_FontSize(&userFontSize);
        if (FAILED(settingsHR) && settingsHR != E_ACCESSDENIED)
        {
            IFC(settingsHR);
        }
    }

    IFC(pStyle->get_FontSize(&fontSize));
    switch (fontSize.Unit)
    {
    case wmc::TimedTextUnit_Pixels:
        *actualSize = fontSize.Value;
        break;
    case wmc::TimedTextUnit_Percentage:
        // if percentage, font size is based on height, 100% is 5% of the videos height. W3C states both, but XAML
        // on supports setting a fonts height
        *actualSize = (m_videoHeight / (100.0 / DefaultFontSize)) * (fontSize.Value / 100);
        break;
    }

    if (userFontSize != wm::ClosedCaptioning::ClosedCaptionSize_Default)
    {
        // scale font based on their user preference
        *actualSize *= ConvertUserFontSizeScale(userFontSize);
    }

    // m_fontScaleFactor scale the font size based on the video scale factor
    *actualSize *= m_fontScaleFactor;

Cleanup:
    return hr;
}
