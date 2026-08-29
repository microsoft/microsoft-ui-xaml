// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CueRenderer.h"
#include "Grid.g.h"
#include "Canvas.g.h"
#include "Border.g.h"
#include "TextBlock.g.h"
#include "StackPanel.g.h"
#include "SolidColorBrush.g.h"
#include "DispatcherTimer.g.h"
#include "Window.g.h"
#include "DoubleAnimation.g.h"
#include "StoryBoard.g.h"
#include "TranslateTransform.g.h"
#include "RectangleGeometry.g.h"
#include "VisualTreeHelper.h"
#include "Image.g.h"
#include "XamlRoot.g.h"

using namespace DirectUI;
using namespace xaml_media;
using namespace Microsoft::WRL;

CCueRenderer::CCueRenderer(_In_ ITimedTextSourcePresenter* pTimedTextSource) :
    m_pTimedTextSourceNoRef(pTimedTextSource),
    m_isInVisualTree(false),
    m_isFullWindow(false),
    m_needsParentSize(true),
    m_parentWidth(0),
    m_parentHeight(0),
    m_videoWidth(0),
    m_videoHeight(0),
    m_videoXOffset(0),
    m_videoYOffset(0),
    m_orientation(wgrd::DisplayOrientations_None),
    m_mtcVideoOverlapAmount(0),
    m_mtcHeight(0),
    m_wrOwner(nullptr)
{
    m_tokSizeChanged.value = 0;
}

CCueRenderer::~CCueRenderer()
{
    m_cueList.clear();

    ctl::ComPtr<xaml::IFrameworkElement> spFrameworkElement;
    ctl::ComPtr<DependencyObject> ownerDo;

    VERIFYHR(m_wrOwner.As(&spFrameworkElement));
    if (spFrameworkElement.Get())
    {
        VERIFYHR(spFrameworkElement->remove_SizeChanged(m_tokSizeChanged));
    }

    if (m_orientationChangedToken.value != 0)
    {
        wrl::ComPtr<wgrd::IDisplayInformationStatics> spDisplayInformationStatics;
        wrl::ComPtr<wgrd::IDisplayInformation> spDisplayInformation;
        VERIFYHR(wf::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(), &spDisplayInformationStatics));
        if (spDisplayInformationStatics)
        {
            VERIFYHR(spDisplayInformationStatics->GetForCurrentView(&spDisplayInformation));
            if (spDisplayInformation)
            {
                VERIFYHR(spDisplayInformation->remove_OrientationChanged(m_orientationChangedToken));
            }
        }
    }

    // We can't use m_cueContainer to find xamlRoot anymore because at this point
    // we might not be in the live visual tree. We us the owner instead.
    VERIFYHR(m_wrOwner.As(&ownerDo));
    if (ownerDo.Get())
    {
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(ownerDo.Get()))
        {   
            auto layoutBoundsHelper = xamlRoot->GetLayoutBoundsHelperNoRef();
            layoutBoundsHelper->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
        }
    }
}

// Initialize only creates the Grid for the container, this is to prevent any playback glitches
// during the video, it is not added to the visual tree until the first cue needs to be drawn
_Check_return_ HRESULT
CCueRenderer::Initialize(_In_ xaml::IUIElement* pOwner)
{
    ctl::ComPtr<SolidColorBrush> spTransparentBrush;
    wu::Color transparentColor;
    wrl::ComPtr<wgrd::IDisplayInformationStatics> spDisplayInformationStatics;
    wrl::ComPtr<wgrd::IDisplayInformation> spDisplayInformation;
    ctl::ComPtr<FrameworkElement> spParent;

    IFC_RETURN(ctl::make<SolidColorBrush>(&spTransparentBrush));
    transparentColor.A = 0;
    transparentColor.B = 0;
    transparentColor.G = 0;
    transparentColor.R = 0;
    IFC_RETURN(spTransparentBrush->put_Color(transparentColor));

    IFC_RETURN(ctl::AsWeak(pOwner, &m_wrOwner));

    IFC_RETURN(ctl::make<Grid>(&m_cueContainer));
    IFC_RETURN(m_cueContainer->put_Background(spTransparentBrush.Get()));
    IFC_RETURN(m_cueContainer->put_IsHitTestVisible(false));

    IFC_RETURN(m_wrOwner.As(&spParent));
    if (spParent.Get())
    {
        IFC_RETURN(spParent->add_SizeChanged(
            Microsoft::WRL::Callback<xaml::ISizeChangedEventHandler>(
                this, &CCueRenderer::OnSizeChanged).Get(),
            &m_tokSizeChanged));
    }

    IFC_RETURN(ctl::make(&m_spStoryboard));

    IFC_RETURN(m_cueStyler.Initialize());

    IFC_RETURN(SetNaturalVideoSize());

    IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(
        RuntimeClass_Windows_Graphics_Display_DisplayInformation).Get(),
        &spDisplayInformationStatics
        ));

    if (spDisplayInformationStatics)
    {
        // TODO: WinUI Desktop support
#if false
        VERIFYHR(spDisplayInformationStatics->GetForCurrentView(&spDisplayInformation));
        if (spDisplayInformation)
        {
            IFC_RETURN(spDisplayInformation->add_OrientationChanged(
                Microsoft::WRL::Callback<wf::ITypedEventHandler<wgrd::DisplayInformation*, IInspectable*>>(
                    this, &CCueRenderer::OnOrientationChanged).Get(),
                &m_orientationChangedToken));
            IFC_RETURN(spDisplayInformation->get_CurrentOrientation(&m_orientation));
        }
#endif
    }

    return S_OK;
}

void CCueRenderer::CleanupDeviceRelatedResources(const bool cleanupDComp)
{
    m_cueStyler.CleanupDeviceRelatedResources(cleanupDComp);
}

//
//  +---------------------------------+
//  |            \|/ ____ \|/         |
//  |             @~/ ,. \~@          |
//  |            /_( \__/ )_\         |
//  |               \__U_/            |
//  |                                 |
//  |  +----------------+             |
//  |  |                |             |
//  |  | Saying caption |<------+     |
//  |  +----------------+       |     |
//  |       ^      ^            |     |
//  +-------|------|------------|-----+
//     ^    |      |            |
//     |    |      |            |
// [Grid]   |      |            |
//   +-[Border]    |            |             <- region border
//       +-[StackPanel]         |
//           +-[Grid](*)        |             <- CueElement
//               +-[Grid]       |             <- TextContainer
//                   +--[TextBlock](*)        <- Caption
//                   +--[Canvas]              <- UnderTextBlock
//
// "Caption" is the TextBlock that exists to show the closed caption.
//
// "UnderTextBlock" exists to implement text edge styles. We simulate raised/depressed/uniform edge styles by creating
// copies of the text with different colors and putting them underneath the text with an offset. We create copies of
// the text with GetAlphaMask and display the mask via composition SpriteVisuals. These SpriteVisuals are inserted
// back in the Xaml tree as hand-in visuals. The problem is that hand-in visuals display on top of Xaml content, and
// here we want the copies to display underneath the text, so we have to create a separate element to hold the copies.
// That's what "UnderTextBlock" is for.
//
// "TextContainer" is an element that holds the closed caption TextBlock plus the edge effect SpriteVisuals. The user
// can select partial opacity on the text color, and we want to apply the opacity evenly on both the text and any edge
// effects. Setting opacity separately on the TextBlock and the edge effect SpriteVisuals will cause them to blend with
// each other, and instead we want group opacity. So we put them in a TextContainer, ask for group opacity, and apply
// an opacity there.
//
// Note: There are two ways that Xaml supports text effects. One is the Raised/Depressed/Uniform/Drop Shadow setting
// selectable from the Captions page in the Settings app and exposed from IClosedCaptionPropertiesStatics, and the
// other is an OutlineThickness specified by the closed caption source itself. Both text effects can be applied at
// the same time (which permits two outlines - one from the Uniform text effect and the other built into the caption).
// The OutlineThickness effect was supported by creating duplicate TextBlock elements with an offset and placing them
// under the real caption TextBlock. This is similar to the SpriteVisual approach with the "UnderTextBlock" container.
// We chose to leave this implementation alone instead of converting it to SpriteVisuals to reduce code churn due to
// the lack of test videos. We can merge these two code paths in a future version. These duplicate TextBlocks are also
// added to the TextContainer element for the same reason - they should use group opacity to avoid blending with the
// other copies and with the caption text itself. In the future we can also combine CueElement and TextContainer
// together into a single element.
//

// Remove the given cue from the visual tree
_Check_return_ HRESULT
CCueRenderer::RemoveCue(_In_ const ctl::ComPtr<wmc::IMediaCue>& spCue)
{
    TraceCueRemoved();
    INT64 cueId = GetCueId(spCue.Get());
    std::list<std::shared_ptr<CCueItem>> deadItems;

    for (auto& item : m_cueList)
    {
        if (item->Id == cueId)
        {
            ASSERT(item->CueElement.Get());
            if (item->CueElement.Get())
            {
                deadItems.push_back(item);

                ctl::ComPtr<StackPanel> spRegion;
                ctl::ComPtr<Border> spRegionBorder;

                auto result = m_regionList.find(item->Region);
                if (result != m_regionList.end())
                {
                    spRegionBorder = result->second;

                    ctl::ComPtr<xaml::IUIElement> spElement;
                    spRegionBorder->get_Child(spElement.ReleaseAndGetAddressOf());
                    spElement.As(&spRegion);
                }

                if (spRegion.Get())
                {
                    UINT nIndex = 0;
                    BOOLEAN bFound = FALSE;

                    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

                    IFC_RETURN(spRegion->get_ChildrenInternal(&spChildren));
                    IFC_RETURN(spChildren->IndexOf(item->CueElement.Get(), &nIndex, &bFound));
                    if (bFound)
                    {
                        IFC_RETURN(spChildren->RemoveAt(nIndex));
                    }

                    // If the region does not have any children, then hide it
                    unsigned int childrenSize = 0;
                    IFC_RETURN(spChildren->get_Size(&childrenSize));
                    if (childrenSize == 0)
                    {
                        IFC_RETURN(spRegionBorder->put_Visibility(xaml::Visibility_Collapsed));
                    }
                }
            }
        }
    }

    for (auto& item : deadItems)
    {
      m_cueList.remove(item);
    }

    return S_OK;
}

// Get the the id of the cue, since I will get the same object
// for adding and removing, just use its address as its id
INT64
CCueRenderer::GetCueId(_In_ const wmc::IMediaCue* const pCue) const
{
    return reinterpret_cast<INT64>(pCue);
}

// This is called when the user is scrubbing and we want to remove all the cues
// from the screen, but not reset the collection
_Check_return_ HRESULT
CCueRenderer::ClearCues()
{
    m_cueList.clear();
    m_regionList.clear();

    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    IFC_RETURN(m_cueContainer->get_Children(&spChildren));
    IFC_RETURN(spChildren->Clear());

    return S_OK;
}

_Check_return_ HRESULT
CCueRenderer::AddCue(_In_ const ctl::ComPtr<wmc::IMediaCue>& spCue) noexcept
{
    TraceTimedTextCueBegin();

    ctl::ComPtr<Border> spRegionBorder;
    std::wstring regionName = {};
    const INT64 cueId = GetCueId(spCue.Get());

    auto scopeGuard = wil::scope_exit([&]
    {
        TraceTimedTextCueEnd();
    });

    // Only draw a cue once.
    for (const auto& item : m_cueList)
    {
        if (item->Id == cueId)
        {
            return S_OK;
        }
    }

    IFC_RETURN(UpdateVideoSize());
    //When transition between full media window with cc enabled, we start reporting incorrect values for width and height,
    //which interfere with our calculations. Therefore, if we hit this state, don't add the cue
    if ((_isnan(m_videoWidth) || m_videoWidth <= 0) || (_isnan(m_videoHeight) || m_videoHeight <= 0) || (_isnan(m_mtcVideoOverlapAmount) || m_mtcVideoOverlapAmount > m_videoHeight))
    {
        return S_OK;
    }

    IFC_RETURN(AddCueContainerToVisualTree());
    IFC_RETURN(CreateCueRegion(spCue, spRegionBorder, regionName));

    // This border may be a collapsed border recycled in RemoveCueInternal. Make it visible before adding text to it.
    // Text edge effects may require running layout on the TextBlock, which requires it to be parented to a visible
    // branch of the tree.
    IFC_RETURN(spRegionBorder->put_Visibility(xaml::Visibility_Visible));

    IFC_RETURN(CreateCueItem(spCue, spRegionBorder, regionName));

    return S_OK;
}

_Check_return_ HRESULT CCueRenderer::CreateCueRegion(
    _In_ const ctl::ComPtr<wmc::IMediaCue>& spCue,
    _Out_ ctl::ComPtr<Border>& spRegionBorder,
    _Inout_ std::wstring& regionName
)
{
    ctl::ComPtr<StackPanel> spRegion;
    ctl::ComPtr<wmc::ITimedTextCue> spTextCue;
    wrl_wrappers::HString strName = {};

    if (SUCCEEDED(spCue.As(&spTextCue)))
    {
        ctl::ComPtr<wmc::ITimedTextRegion> spTextRegion;
        ctl::ComPtr<wmc::ITimedTextStyle> spTextStyle;
        IFC_RETURN(spTextCue->get_CueRegion(spTextRegion.GetAddressOf()));
        IFC_RETURN(spTextCue->get_CueStyle(spTextStyle.GetAddressOf()));
        if (spTextRegion)
        {
            IFC_RETURN(spTextRegion->get_Name(strName.GetAddressOf()));
        }
        if (strName.IsEmpty())
        {
            wrl_wrappers::HStringReference(L"generated_default").CopyTo(strName.GetAddressOf());
        }
        m_cueStyler.SetFontSizeRatio(m_videoHeight - m_mtcVideoOverlapAmount);
    }
    else
    {
        wrl_wrappers::HStringReference(L"generated_image_default").CopyTo(strName.GetAddressOf());
    }

    regionName = std::wstring(strName.GetRawBuffer(nullptr));
    auto result = m_regionList.find(regionName);
    if (result != m_regionList.end())
    {
        spRegionBorder = result->second;

        ctl::ComPtr<xaml::IUIElement> spElement;
        spRegionBorder->get_Child(spElement.ReleaseAndGetAddressOf());
        spElement.As(&spRegion);
    }
    else
    {
        // New region used, create it
        IFC_RETURN(ctl::make<Border>(&spRegionBorder));
        IFC_RETURN(ctl::make<StackPanel>(&spRegion));

        // SP should always aligned to bottom of the regionBorder
        IFC_RETURN(spRegion->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Bottom));

        IFC_RETURN(spRegionBorder->put_Child(spRegion.Get()));

        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
        IFC_RETURN(m_cueContainer->get_Children(&spChildren));
        IFC_RETURN(spChildren->Append(spRegionBorder.Get()));

        m_regionList.emplace(regionName, spRegionBorder.Get());
    }

    return S_OK;
}

_Check_return_ HRESULT CCueRenderer::CreateCueItem(
    _In_ const ctl::ComPtr<wmc::IMediaCue>& spCue,
    _In_ ctl::ComPtr<Border>& spRegionBorder,
    _In_ std::wstring& regionName
)
{
    ctl::ComPtr<wmc::ITimedTextCue> spTextCue;
    ctl::ComPtr<wmc::IImageCue> spImageCue;
    ctl::ComPtr<xaml::IUIElement> spElement;
    ctl::ComPtr<StackPanel> spRegion;

    spRegionBorder->get_Child(spElement.ReleaseAndGetAddressOf());
    spElement.As(&spRegion);

    const INT64 cueId = GetCueId(spCue.Get());

    if (SUCCEEDED(spCue.As(&spImageCue)))
    {
        IFC_RETURN(m_cueStyler.SetImageRegionConfiguration(
            spRegionBorder.Get(),
            spImageCue,
            m_videoWidth,
            m_videoHeight - m_mtcVideoOverlapAmount,
            m_videoXOffset,
            m_videoYOffset));

        ctl::ComPtr<Grid> spCueElement;
        ctl::ComPtr<Image> spImage;

        IFC_RETURN(ctl::make(&spCueElement));
        IFC_RETURN(ctl::make(&spImage));
        IFC_RETURN(m_cueStyler.SetImage(spImage, spImageCue));
        IFC_RETURN(AddCueItem(spCueElement, spRegion, spImage, regionName, cueId, 0));
    }
    else
    {
        IFC_RETURN(spCue.As(&spTextCue));
        ctl::ComPtr<wmc::ITimedTextRegion> spTextRegion;
        IFC_RETURN(spTextCue->get_CueRegion(spTextRegion.GetAddressOf()));
        IFC_RETURN(m_cueStyler.SetRegionConfiguration(spRegionBorder.Get(), spTextRegion.Get(), m_videoWidth, m_videoHeight - m_mtcVideoOverlapAmount, m_videoXOffset, m_videoYOffset));

        ctl::ComPtr<wfc::IVector<wmc::TimedTextLine*>> spLines;
        IFC_RETURN(spTextCue->get_Lines(&spLines));
        unsigned int count = {};
        wmc::TimedTextDouble lineHeight = {};

        IFC_RETURN(spLines->get_Size(&count));
        if (spTextRegion)
        {
            IFC_RETURN(spTextRegion->get_LineHeight(&lineHeight));
        }

        for (unsigned int i = 0; i < count; i++)
        {
            ctl::ComPtr<TextBlock> spTextBlock;
            ctl::ComPtr<Grid> spCueElement;
            ctl::ComPtr<Grid> textContainer;
            ctl::ComPtr<Canvas> underTextBlock;
            ctl::ComPtr<wmc::ITimedTextLine> spLine;
            ctl::ComPtr<wmc::ITimedTextStyle> spTextStyle;
            wmc::TimedTextDouble thickness = {};

            IFC_RETURN(spTextCue->get_CueRegion(spTextRegion.ReleaseAndGetAddressOf()));
            IFC_RETURN(spTextCue->get_CueStyle(spTextStyle.GetAddressOf()));

            IFC_RETURN(ctl::make(&spCueElement));
            IFC_RETURN(ctl::make(&textContainer));
            IFC_RETURN(ctl::make(&underTextBlock));
            IFC_RETURN(ctl::make(&spTextBlock));
            IFC_RETURN(spLines->GetAt(i, &spLine));
            IFC_RETURN(m_cueStyler.SetTextBlock(spLine.Get(), spTextStyle.Get(), spTextBlock.Get(), spTextRegion.Get(), spCueElement.Get(), false));

            if (spTextStyle)
            {
                IFC_RETURN(spTextStyle->get_OutlineThickness(&thickness));
            }

            if (thickness.Value > 0)
            {
                IFC_RETURN(m_cueStyler.CreateOutline(spLine.Get(), spTextStyle.Get(), spTextRegion.Get(), textContainer.Get()));
            }

            // This is a panel that renders under the TextBlock in z-order. It's used to put alpha mask visuals for
            // text edge effects.
            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
            IFC_RETURN(textContainer->get_Children(&spChildren));
            IFC_RETURN(spChildren->Append(underTextBlock.Get()));
            IFC_RETURN(spChildren->Append(spTextBlock.Get()));

            IFC_RETURN(AddCueItem(spCueElement, spRegion, textContainer, regionName, cueId, lineHeight.Value));
    
            // AddCueItem has added the chain of wrappers for CVS-based rendering - get the top one and pass it to SetTextBlockEffect.
            ctl::ComPtr<xaml::IUIElement> spUIE;
            IFC_RETURN(spCueElement->get_Children(&spChildren));
            IFC_RETURN(spChildren->GetAt(0, &spUIE));
            ctl::ComPtr<Grid> spTopWrapper = spUIE.Cast<Grid>();

            IFC_RETURN(m_cueStyler.SetTextBlockEffect(spTextBlock.Get(), underTextBlock.Get(), textContainer.Get(), spTopWrapper.Get()));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CCueRenderer::UpdateVideoSize()
{
    if (m_needsParentSize)
    {
        ctl::ComPtr<FrameworkElement> spOwner;
        IFC_RETURN(m_wrOwner.As(&spOwner));
        if (spOwner.Get() && m_pTimedTextSourceNoRef)
        {
            BOOLEAN isFullWindow = FALSE;
            IFC_RETURN(m_pTimedTextSourceNoRef->get_IsFullWindow(&isFullWindow));
            m_isFullWindow = !!isFullWindow;

            if (m_isFullWindow)
            {
                wf::Rect layoutBounds = {};
                IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(spOwner->GetHandle(), &layoutBounds));
                m_parentWidth = layoutBounds.Width;
                m_parentHeight = layoutBounds.Height;
            }
            else
            {
                IFC_RETURN(spOwner->get_ActualWidth(&m_parentWidth));
                IFC_RETURN(spOwner->get_ActualHeight(&m_parentHeight));
            }

            IFC_RETURN(CalculateActualVideoSize());

            IFC_RETURN(UpdateClipping());

            IFC_RETURN(SetNaturalVideoSize());

            // There is a timing issue when the phone is rotated. Some times the m_parentWidth and m_parentHeight (above)
            // do not correspond to the current orientation, the sizes are updated after I get the orientation event.
            // This is a workaround to detect if the sizes don't match the orientation given.
            if (m_orientation == wgrd::DisplayOrientations_Landscape || m_orientation == wgrd::DisplayOrientations_LandscapeFlipped)
            {
                m_needsParentSize = m_parentWidth < m_parentHeight;
            }
            else if (m_orientation == wgrd::DisplayOrientations_Portrait || m_orientation == wgrd::DisplayOrientations_PortraitFlipped)
            {
                m_needsParentSize = m_parentWidth > m_parentHeight;
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CCueRenderer::AddCueItem(
    _In_ const ctl::ComPtr<Grid>& spCueElement,
    _In_ const ctl::ComPtr<StackPanel>& spRegion,
    _In_ const ctl::ComPtr<UIElement>& spChild,
    _In_ const std::wstring& regionName,
    _In_ const INT64 cueId,
    _In_ const double lineHeight)
{
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    IFC_RETURN(spCueElement->put_Visibility(xaml::Visibility_Visible));
    IFC_RETURN(spCueElement->get_Children(&spChildren));
    
    xaml::HorizontalAlignment ha = xaml::HorizontalAlignment_Center;
    xaml::VerticalAlignment va = xaml::VerticalAlignment_Center;
    IFC_RETURN(spCueElement->get_HorizontalAlignment(&ha));
    IFC_RETURN(spCueElement->get_VerticalAlignment(&va));

    // Create chain of 3 wrapper Grids to support CVS Rendering & insert between CueElement and TextContainer
    // Resulting subtree:
    //   .. -> CueElement (Grid) -> TopWrapper (Grid) -> MiddleWrapper (Grid) ->  BottomWrapper (Grid) -> TextContainer (Grid)} -> ..

    // Create TopWrapper, connect to CueElement
    ctl::ComPtr<Grid> topWrapper;
    IFC_RETURN(ctl::make(&topWrapper));
    IFC_RETURN(topWrapper->put_HorizontalAlignment(ha));
    IFC_RETURN(topWrapper->put_VerticalAlignment(va));
    IFC_RETURN(spChildren->Append(topWrapper.Get()));

    // Create MiddleWrapper, connect to TopWrapper
    ctl::ComPtr<Grid> middleWrapper;
    IFC_RETURN(ctl::make(&middleWrapper));
    IFC_RETURN(middleWrapper->put_HorizontalAlignment(ha));
    IFC_RETURN(middleWrapper->put_VerticalAlignment(va));
    IFC_RETURN(topWrapper->get_Children(&spChildren));
    IFC_RETURN(spChildren->Append(middleWrapper.Get()));

    // Create BottomWrapper, connect to MiddleWrapper
    ctl::ComPtr<Grid> bottomWrapper;
    IFC_RETURN(ctl::make(&bottomWrapper));
    IFC_RETURN(bottomWrapper->put_HorizontalAlignment(ha));
    IFC_RETURN(bottomWrapper->put_VerticalAlignment(va));
    IFC_RETURN(middleWrapper->get_Children(&spChildren));
    IFC_RETURN(spChildren->Append(bottomWrapper.Get()));

    // Connect TextContainer to BottomWrapper
    IFC_RETURN(bottomWrapper->get_Children(&spChildren));
    IFC_RETURN(spChildren->Append(spChild.Get()));

    // Apply 0x0 Clip on MiddleWrapper so that Text/Effects don't render twice (normal + via CVS).
    // In particular, this clip out the actual TextBlock + EdgeEffects rendering
    ctl::ComPtr<RectangleGeometry> spClippingGeometry;
    IFC_RETURN(ctl::make<RectangleGeometry>(&spClippingGeometry));
    wf::Rect clipRect = {};
    clipRect.X = clipRect.Y = clipRect.Width = clipRect.Height = 0;
    IFC_RETURN(spClippingGeometry->put_Rect(clipRect));
    IFC_RETURN(middleWrapper->put_Clip(spClippingGeometry.Get()));    

    // Add CueElement to live tree
    IFC_RETURN(spRegion->get_Children(&spChildren));
    IFC_RETURN(spChildren->Append(spCueElement.Get()));

    auto currentItem = std::make_shared<CCueItem>();
    currentItem->Region = regionName;
    currentItem->CueElement = spCueElement;
    currentItem->AnimationStep = -1;
    currentItem->AnimationDistance = lineHeight;
    currentItem->Id = cueId;
    m_cueList.push_back(currentItem);

    return S_OK;
}


// Resets the CueRender, happens when the cue list changes or video changes
_Check_return_ HRESULT
CCueRenderer::Reset()
{
    HRESULT hr = S_OK;

    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    IFC(m_cueContainer->get_Children(&spChildren));
    IFC(spChildren->Clear());

    IFC(SetNaturalVideoSize());

Cleanup:
    m_cueList.clear();
    m_regionList.clear();

    return hr;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Adds the cue container to visual tree
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CCueRenderer::AddCueContainerToVisualTree()
{
    if (!m_isInVisualTree)
    {
        ctl::ComPtr<DependencyObject> spOwner;
        IFC_RETURN(m_wrOwner.As(&spOwner));
        if (spOwner.Get() && m_pTimedTextSourceNoRef)
        {
            BOOLEAN isFullWindow = FALSE;
            IFC_RETURN(m_pTimedTextSourceNoRef->get_IsFullWindow(&isFullWindow));

            if (isFullWindow)
            {
                IFC_RETURN(AddToFullWindowMediaRoot());
            }
            else
            {
                IFC_RETURN(AddToOwner());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CCueRenderer::AddToOwner()
{
    ctl::ComPtr<UIElement> spOwner;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC_RETURN(m_wrOwner.As(&spOwner));
    if (spOwner.Get())
    {
        IFC_RETURN(spOwner->get_ChildrenInternal(&spChildren));

        IFC_RETURN(spChildren->Append(m_cueContainer.Get()));
    }

    m_isInVisualTree = true;
    IFC_RETURN(ResetCues());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to remove the cue container
//      This is used for non-full screen elements
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CCueRenderer::RemoveFromOwner()
{
    ctl::ComPtr<UIElement> spOwner;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;

    IFC_RETURN(m_wrOwner.As(&spOwner));
    if (spOwner.Get())
    {
        IFC_RETURN(spOwner->get_ChildrenInternal(&spChildren));

        unsigned int childrenSize = 0;
        IFC_RETURN(spChildren->get_Size(&childrenSize));
        for (unsigned int i = 0; i < childrenSize; i++)
        {
            ctl::ComPtr<xaml::IUIElement> spUIE;
            IFC_RETURN(spChildren->GetAt(i, &spUIE));
            if (spUIE.Get() == m_cueContainer.Cast<Grid>())
            {
                IFC_RETURN(spChildren->RemoveAt(i));
                break;
            }
        }
    }

    m_isInVisualTree = false;

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to parent cue container to associated MediaPlayerElement
//      This is used for full screen elements
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CCueRenderer::AddToFullWindowMediaRoot()
{
    ctl::ComPtr<xaml::IDependencyObject> spOwnerME;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml_controls::IPanel> spFullWindowMediaRoot;

    IFC_RETURN(m_wrOwner.As(&spOwnerME));
    if (spOwnerME.Get())
    {
        IFC_RETURN(VisualTreeHelper::GetFullWindowMediaRootStatic(spOwnerME.Get(), &spFullWindowMediaRoot));
        ASSERT(spFullWindowMediaRoot.Get());

        if (spFullWindowMediaRoot)
        {
            IFC_RETURN(spFullWindowMediaRoot.Cast<Panel>()->get_ChildrenInternal(&spChildren));
            IFC_RETURN(spChildren->Append(m_cueContainer.Get()));
        }

        // Add a callback for the parent bounds changed, this is only needed for full screen
        // This should not assume that MTC will take care of the bounds if it is there
        // CCueRenderer has a tight lifetime, it appears only as a private member variable
        // on TimedTextSource. This coupled with us removing the callback in CCueRenderer's
        // destructor means we can be confident that this lambda will not be call after the
        // CCueRenderer has been destructed.
        // We only attach the callback if the CueRenderer was actually placed in a live tree.
        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(m_cueContainer.Get()))
        {
            if (m_tokLayoutBoundsChanged.value == 0)
            {
                auto layoutBoundsHelper = xamlRoot->GetLayoutBoundsHelperNoRef(); 
                layoutBoundsHelper->AddLayoutBoundsChangedCallback(
                    [this]() mutable
                    {
                        ctl::ComPtr<UIElement> spOwner;
                        IFC_RETURN(m_wrOwner.As(&spOwner));
                        if (spOwner.Get())
                        {
                            if (m_isFullWindow && spOwner->IsInLiveTree())
                            {
                                // Update the CC container bounds with the available layout bounds.
                                IFC_RETURN(UpdateBounds());
                            }
                        }
                        return S_OK;
                    }, &m_tokLayoutBoundsChanged);
            }
        }
    }

    m_isInVisualTree = true;

    IFC_RETURN(UpdateBounds());

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to remove cue container
//      This is used for full screen elements
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT
CCueRenderer::RemoveFromFullWindowMediaRoot()
{
    ctl::ComPtr<xaml::IDependencyObject> spOwnerME;
    ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spChildren;
    ctl::ComPtr<xaml_controls::IPanel> spFullWindowMediaRoot;

    IFC_RETURN(m_wrOwner.As(&spOwnerME));
    if (spOwnerME.Get())
    {
        IFC_RETURN(VisualTreeHelper::GetFullWindowMediaRootStatic(spOwnerME.Get(), &spFullWindowMediaRoot));
        ASSERT(spFullWindowMediaRoot.Get());

        if (const auto xamlRoot = XamlRoot::GetImplementationForElementStatic(m_cueContainer.Get()))
        {   
            auto layoutBoundsHelper = xamlRoot->GetLayoutBoundsHelperNoRef();
            layoutBoundsHelper->RemoveLayoutBoundsChangedCallback(&m_tokLayoutBoundsChanged);
        }

        if (spFullWindowMediaRoot)
        {
            IFC_RETURN(spFullWindowMediaRoot.Cast<Panel>()->get_ChildrenInternal(&spChildren));

            UINT childrenSize = 0;
            IFC_RETURN(spChildren->get_Size(&childrenSize));
            for (XUINT32 i = 0; i < childrenSize; i++)
            {
                ctl::ComPtr<xaml::IUIElement> spUIE;
                IFC_RETURN(spChildren->GetAt(i, &spUIE));
                if (spUIE.Get() == m_cueContainer.Cast<Grid>())
                {
                    IFC_RETURN(spChildren->RemoveAt(i));
                    break;
                }
            }
        }
    }

    m_isInVisualTree = false;

    return S_OK;
}

// This does the actual call to update the layout bounds of the parent media element
// so that CC are correctly positioned in full screen. This has the same parent as MTC and
// therefore uses the same bounds callback as the MTC.
_Check_return_ HRESULT
CCueRenderer::UpdateBounds()
{
    ctl::ComPtr<DependencyObject> spOwnerME;

    IFC_RETURN(m_wrOwner.As(&spOwnerME));
    if (spOwnerME.Get())
    {
        auto* pFullWindowMediaRoot = spOwnerME->GetHandle()->GetContext()->GetMainFullWindowMediaRoot();
        if (pFullWindowMediaRoot)
        {
            pFullWindowMediaRoot->InvalidateArrange();
            pFullWindowMediaRoot->InvalidateMeasure();
        }
    }

    IFC_RETURN(ResetCues());

    return S_OK;
}

_Check_return_ HRESULT
CCueRenderer::GetNaturalVideoSize(_Out_ INT32* pHeight, _Out_ INT32* pWidth)
{
    *pHeight = 0;
    *pWidth = 0;
    if (m_pTimedTextSourceNoRef)
    {
        IFC_RETURN(m_pTimedTextSourceNoRef->GetNaturalVideoSize(pHeight, pWidth));

        if (*pHeight == 0 || *pWidth == 0)
        {
            ctl::ComPtr<FrameworkElement> spOwner;
            IFC_RETURN(m_wrOwner.As(&spOwner));
            if (spOwner.Get())
            {
                BOOLEAN isFullWindow = FALSE;
                IFC_RETURN(m_pTimedTextSourceNoRef->get_IsFullWindow(&isFullWindow));
                if (isFullWindow)
                {
                    wf::Rect layoutBounds = {};
                    IFC_RETURN(DXamlCore::GetCurrent()->GetContentLayoutBoundsForElement(spOwner->GetHandle(), &layoutBounds));
                    *pWidth = static_cast<INT32>(layoutBounds.Width);
                    *pHeight = static_cast<INT32>(layoutBounds.Height);
                }
                else
                {
                    double value = 0;
                    IFC_RETURN(spOwner->get_ActualWidth(&value));
                    *pWidth = static_cast<INT32>(value);

                    value = 0;
                    IFC_RETURN(spOwner->get_ActualHeight(&value));
                    *pHeight = static_cast<INT32>(value);
                }
            }
        }
    }
    return S_OK;
}

// Pass the natural video size to the styler
_Check_return_ HRESULT
CCueRenderer::SetNaturalVideoSize()
{
    INT32 height = 0;
    INT32 width = 0;
    IFC_RETURN(GetNaturalVideoSize(&height, &width));

    m_cueStyler.SetNaturalVideoSize(static_cast<int>(height), static_cast<int>(width));

    return S_OK;
}

// When the MTC is shown we need to shift the CC text up above the MTC
// this is given the height of the MTC when shown and 0 when hidden
_Check_return_ HRESULT
CCueRenderer::SetMTCOffset(_In_ double mtcHeight)
{
    m_mtcHeight = mtcHeight;

    // Only shift the viewport by the size of the MTC overlap with the video
    m_mtcVideoOverlapAmount = mtcHeight - m_videoYOffset;
    if (m_mtcVideoOverlapAmount < 0)
    {
        m_mtcVideoOverlapAmount = 0;
    }

    IFC_RETURN(UpdateClipping());

    // Because the MTC has changed the layout we need to re-render the current cues
    // The best way is to clear and reset the active cues
    Reset();
    if (m_pTimedTextSourceNoRef)
    {
        IFC_RETURN(m_pTimedTextSourceNoRef->ResetActiveCues());
    }

    return S_OK;
}

// When the MTC is shown, the viewport is shrunk by the amount of MTC overlap with the video
// This will allow for layouts to continue to look the same. I.e. font sizes and layout are adjusted
// based on the new viewport size
_Check_return_ HRESULT
CCueRenderer::UpdateClipping()
{
    ctl::ComPtr<RectangleGeometry> spClippingGeometry;
    wf::Rect clipRect = {};

    xaml::Thickness thickness = {};
    thickness.Top = m_videoYOffset;
    thickness.Left = m_videoXOffset;

    IFC_RETURN(ctl::make<RectangleGeometry>(&spClippingGeometry));

    clipRect.X = static_cast<float>(0);
    clipRect.Y = static_cast<float>(0);
    clipRect.Width = static_cast<float>(m_videoWidth);
    clipRect.Height = static_cast<float>(MAX(0, m_videoHeight - m_mtcVideoOverlapAmount));

    IFC_RETURN(spClippingGeometry->put_Rect(clipRect));
    IFC_RETURN(m_cueContainer->put_Clip(spClippingGeometry.Get()));

    IFC_RETURN(m_cueContainer->put_VerticalAlignment(xaml::VerticalAlignment::VerticalAlignment_Top));
    IFC_RETURN(m_cueContainer->put_HorizontalAlignment(xaml::HorizontalAlignment::HorizontalAlignment_Left));

    IFC_RETURN(m_cueContainer->put_Width(m_videoWidth));
    IFC_RETURN(m_cueContainer->put_Height(MAX(0, m_videoHeight - m_mtcVideoOverlapAmount)));
    IFC_RETURN(m_cueContainer->put_Margin(thickness));

    return S_OK;
}


_Check_return_ HRESULT
CCueRenderer::OnOrientationChanged(_In_ wgrd::IDisplayInformation* pSender, _In_ IInspectable* pArgs)
{
    UNREFERENCED_PARAMETER(pArgs);

    if (pSender)
    {
        IFC_RETURN(pSender->get_CurrentOrientation(&m_orientation));
    }

    IFC_RETURN(ResetCues());

    return S_OK;
}

// Size change handler, instead of trying to redo the layout math on each cue
// just delete all the cues and re-add the active cues
_Check_return_ HRESULT
CCueRenderer::OnSizeChanged(_In_ IInspectable* pSender, _In_ xaml::ISizeChangedEventArgs* pArgs)
{
    UNREFERENCED_PARAMETER(pSender);
    UNREFERENCED_PARAMETER(pArgs);

    IFC_RETURN(ResetCues());

    return S_OK;
}

_Check_return_ HRESULT
CCueRenderer::CalculateActualVideoSize()
{
    xaml_media::Stretch stretch = {};
    double naturalVideoHeight = 0;
    double naturalVideoWidth = 0;
    double parentHeight = m_parentHeight;
    double parentWidth = m_parentWidth;

    if (m_pTimedTextSourceNoRef)
    {
        // The method takes ints, but I want to do all the math with doubles
        int originalNaturalHeight = 0;
        int originalNaturalWidth = 0;

        IFC_RETURN(GetNaturalVideoSize(&originalNaturalHeight, &originalNaturalWidth));
        naturalVideoHeight = originalNaturalHeight;
        naturalVideoWidth = originalNaturalWidth;

        IFC_RETURN(m_pTimedTextSourceNoRef->get_Stretch(&stretch));
    }

    ASSERT(naturalVideoHeight != 0);
    ASSERT(naturalVideoWidth != 0);

    m_videoXOffset = 0;
    m_videoYOffset = 0;

    switch (stretch)
    {
    case Stretch_None:
        // Video is full size but may have letter or pillar boxing depending
        // on parent size
        m_videoHeight = std::min(parentHeight, naturalVideoHeight);
        m_videoWidth = std::min(parentWidth, naturalVideoWidth);

        if (parentWidth > naturalVideoWidth)
        {
            m_videoXOffset = (parentWidth - naturalVideoWidth) / 2.0;
        }

        if (parentHeight > naturalVideoHeight)
        {
            m_videoYOffset = (parentHeight - naturalVideoHeight) / 2.0;
        }
        break;

    case Stretch_Fill:
        // Video always fills the parent
        m_videoHeight = parentHeight;
        m_videoWidth = parentWidth;
        break;

    case Stretch_Uniform:
        {
            // Video is always fully shown, but may have letter of pillar boxing
            // depending on parent size and ratio of both parent and video
            double parentRatio = parentWidth / parentHeight;
            double videoRatio = naturalVideoWidth / naturalVideoHeight;

            if (parentRatio < videoRatio)
            {
                // Video will be Letterboxed
                m_videoHeight = parentWidth * (naturalVideoHeight / naturalVideoWidth);
                m_videoWidth = parentWidth;
                m_videoXOffset = 0;
                m_videoYOffset = (parentHeight - m_videoHeight) / 2.0;
            }
            else
            {
                // Video will be Pillarboxed
                m_videoHeight = parentHeight;
                m_videoWidth = parentHeight * (naturalVideoWidth / naturalVideoHeight);
                m_videoXOffset = (parentWidth - m_videoWidth) / 2.0;
                m_videoYOffset = 0;
            }
        }
        break;

    case Stretch_UniformToFill:
        // Video always fills the parent, but video will be clipped

        // TODO: for now the CC text will always be on screen. We need to determine if we want
        // the text to stay on screen or layout with the video frame and therefore be clipped
        m_videoHeight = parentHeight;
        m_videoWidth = parentWidth;
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT
CCueRenderer::ResetCues()
{
    m_needsParentSize = true;

    // The visuals have changed, we need to recalculate the MTC overlap
    SetMTCOffset(m_mtcHeight);

    if (m_pTimedTextSourceNoRef)
    {
        IFC_RETURN(m_pTimedTextSourceNoRef->ResetActiveCues());
    }

    return S_OK;
}

_Check_return_ HRESULT
CCueRenderer::OnCuePresentationModeChangedCallback(_In_ wmp::ITimedMetadataPresentationModeChangedEventArgs* pArgs)
{
    wmp::TimedMetadataTrackPresentationMode mode;
    IFC_RETURN(pArgs->get_NewPresentationMode(&mode));
    if (mode != wmp::TimedMetadataTrackPresentationMode_PlatformPresented)
    {
        IFC_RETURN(ClearCues());
    }

    return S_OK;
}
