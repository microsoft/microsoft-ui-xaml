// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageDecodeBoundsFinder.h"
#include <PixelFormat.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <MUX-ETWEvents.h>

using namespace RuntimeFeatureBehavior;

ImageDecodeBoundsFinder::ImageDecodeBoundsFinder(_In_ CImageSource* pImageSource)
: m_pImageSource(pImageSource),
  m_width(0),
  m_height(0),
  m_continueSearch(true),
  m_skipDecode(false)
{
}

// For each element/brush we find a result for, add the result to a collection of results for this image.
void ImageDecodeBoundsFinder::AddResult(Result result)
{
    m_results.push_back(result);

    if (result == Result::FallbackToNaturalSize)
    {
        // No need to continue trying to compute a bounds across more parents
        m_continueSearch = false;
    }
}

// At the end of the process, go through all the results for all parent element/brushes and determine
// the final result to base our decode size request on.
ImageDecodeBoundsFinder::Result ImageDecodeBoundsFinder::GetFinalResult()
{
    bool foundBounds = false;
    for (size_t i = 0; i < m_results.size(); i++)
    {
        if (m_results[i] == Result::FallbackToNaturalSize)
        {
            // Found at least one parent that requires natural size.
            // Fall back to natural size.
            return Result::FallbackToNaturalSize;
        }
        else if (m_results[i] == Result::FoundBounds)
        {
            // We found at least one parent that has non-empty bounds.
            // If we only find parents with empty bounds we'll avoid decoding till later.
            foundBounds = true;
        }
    }

    return foundBounds ? Result::FoundBounds : Result::EmptyBounds;
}

// Helper function to determine if this element requires the software rendering code path
bool ImageDecodeBoundsFinder::UsesSoftwareRendering(_In_ CUIElement* pUIElement)
{
    const KnownTypeIndex typeIndex = pUIElement->GetTypeIndex();

    return
        (typeIndex == KnownTypeIndex::ListViewItemPresenter) ||
        (typeIndex == KnownTypeIndex::GridViewItemPresenter) ||
        HWWalk::IsShapeThatRequiresSoftwareRendering(pUIElement) ||
        HWWalk::IsBorderLikeElementThatRequiresSoftwareRendering(pUIElement);
}

// Main function to run the various checks and compute a desired decode bounds
_Check_return_ HRESULT ImageDecodeBoundsFinder::ComputeDecodeBoundsForUIElement(
    _In_ CUIElement* pUIElement,
    _In_ CImageBrush* pImageBrush)
{
    // Don't bother if this element uses the software code path
    // TODO:  Add support for this case
    if (!UsesSoftwareRendering(pUIElement))
    {
        // Ask the element for the bounds it would use for the image in logical coordinates
        XRECTF_WH bounds;
        IFC_RETURN(pUIElement->GetBoundsForImageBrushVirtual(
            pImageBrush,
            &bounds
            ));

        // If the element is empty we avoid computing a bounds as this will end up empty too.
        if (bounds.Width != 0.0f &&
            bounds.Height != 0.0f)
        {
            // Incorporate the ImageBrush.Stretch mode
            IFC_RETURN(pImageBrush->AdjustDecodeRectForStretch(&bounds));

            // Convert the logical size to physical size.
            // This requires a walk up the tree incorporating scale factors along the way.
            CTransformToRoot realizationTransform;
            float scaleX;
            float scaleY;
            IFC_RETURN(pUIElement->ComputeRealizationTransform(&realizationTransform));
            IFC_RETURN(realizationTransform.GetScaleDimensions(pUIElement, &scaleX, &scaleY));

            // We now have the overall scale factor.  Convert from logical to physical size.
            UINT32 width = XcpRound(bounds.Width * scaleX);
            UINT32 height = XcpRound(bounds.Height * scaleY);
            XSIZE size = {width, height};

            // If the result requires image tiling, then we currently have to fall back to natural size.
            // TODO:  Add support for this case
            if (!CTiledSurface::NeedsToBeTiled(size, pUIElement->GetContext()->GetMaxTextureSize()))
            {
                // Take the max of the sizes we have computed for this parent chain.
                if (width > m_width || height > m_height)
                {
                    m_width = width;
                    m_height = height;
                    AddResult(Result::FoundBounds);
                }
            }
            else
            {
                m_pImageSource->TraceDecodeToRenderSizeDisqualified(UsesImageTiling);
                AddResult(Result::FallbackToNaturalSize);
            }
        }
        else
        {
            m_pImageSource->TraceDecodeToRenderSizeDisqualified(EmptyBoundsPostRenderWalk);
            AddResult(Result::EmptyBounds);
        }
    }
    else
    {
        m_pImageSource->TraceDecodeToRenderSizeDisqualified(SoftwareRendering);
        AddResult(Result::FallbackToNaturalSize);
    }

    return S_OK;
}

// Primary entry point, drives the overall process of finding the bounds to decode to
_Check_return_ HRESULT ImageDecodeBoundsFinder::FindReasonableDecodeBounds()
{
    // If the element isn't in the live tree then fallback to natural size.
    // This case can't be fixed as we can't compute a valid bounds.
    if(m_pImageSource->IsActive())
    {
        size_t parentCount = m_pImageSource->GetParentCount();
        for (size_t i = 0; i < parentCount && m_continueSearch; i++)
        {
            CDependencyObject* pParent = m_pImageSource->GetParentItem(i);

            // There are two cases:  Image and ImageBrush
            // Case 1:  Image
            if (pParent->OfTypeByIndex<KnownTypeIndex::Image>())
            {
                CImage *pImage = static_cast<CImage*>(pParent);

                // Don't bother if this element uses a NineGrid.
                // This case is already optimal - the image will use the NineGrid to stretch to fit.
                if (!HWWalk::ShouldImageUseNineGrid(pImage))
                {
                    if (pImage->GetBackgroundBrush() != nullptr)
                    {
                        ASSERT(pImage->GetBackgroundBrush()->m_pImageSource == m_pImageSource);
                        IFC_RETURN(ComputeDecodeBoundsForUIElement(pImage, pImage->GetBackgroundBrush()));
                    }
                }
                else
                {
                    m_pImageSource->TraceDecodeToRenderSizeDisqualified(NineGrid);
                    AddResult(Result::FallbackToNaturalSize);
                }
            }

            // Case 2:  ImageBrush
            else if (pParent->OfTypeByIndex<KnownTypeIndex::ImageBrush>())
            {
                // In this case we need to walk all the brush's parents
                size_t grandParentCount = pParent->GetParentCount();
                for (size_t j = 0; j < grandParentCount && m_continueSearch; j++)
                {
                    CDependencyObject* pGrandParent = pParent->GetParentItem(j);

                    // It's possible to put an ImageBrush under a ResourceDictionary.
                    // Catch that case by requiring that we walk UIElement parents.
                    if (pGrandParent->OfTypeByIndex<KnownTypeIndex::UIElement>())
                    {
                        CUIElement* pUIElement = static_cast<CUIElement*>(pGrandParent);
                        IFC_RETURN(ComputeDecodeBoundsForUIElement(pUIElement, static_cast<CImageBrush*>(pParent)));
                    }
                }
            }
        }
    }
    else
    {
        m_pImageSource->TraceDecodeToRenderSizeDisqualified(NotInLiveTree);
        AddResult(Result::FallbackToNaturalSize);
    }

    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if (!runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnableCulledFromRenderWalkOptimization))
    {
        AddResult(Result::FallbackToNaturalSize);
    }

    // Now that we're done  walking all possible parents, make a final decision
    switch (GetFinalResult())
    {
    case Result::FoundBounds:
        break;
    case Result::EmptyBounds:
        // TODO: MSFT: 4808968 - Skipping decode in the case of Empty Bounds will occassionally cause images not to be rendered.
        // The original intent was that if these images would be skipped the first render, but would be decoded in the second render.
        // A few bugs have cropped up (4794043, 4698270) where we never get this second render, so the images never decode/draw.
        // So for now, we will treat EmptyBounds the same as FallbackToNaturalSize, to ensure everything renders correctly.

        //m_skipDecode = true;
        //break;
    case Result::FallbackToNaturalSize:
        m_width = m_pImageSource->GetLayoutWidth();
        m_height = m_pImageSource->GetLayoutHeight();
        break;
    }

    return S_OK;
}
