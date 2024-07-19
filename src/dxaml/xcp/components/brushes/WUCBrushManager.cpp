// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WUCBrushManager.h"
#include <DCompSurface.h>
#include <HWTextureMgr.h>
#include <SolidColorBrush.h>
#include <LinearGradientBrush.h>
#include <XamlCompositionBrush.h>
#include <dcomp.h>
#include <windows.ui.composition.interop.h>
#include "ColorUtil.h"
#include "ExpressionHelper.h"
#include "DOPointerCast.h"
#include "UIElement.h"
#include "Corep.h"
#include "PropertyTransitions.h"
#include "SharedTransitionAnimations.h"
#include "DropShadowRecipe.h"

SolidColorBrushTransitionState::~SolidColorBrushTransitionState()
{
    // Unsubscribe to the completed event if we're still subscribed. This relies on this struct living only in the
    // brush map, which is enforced by blocking the copy operation.
    if (m_wucCompletedEventToken.value)
    {
        ASSERT(m_wucScopedBatch != nullptr);
        IFCFAILFAST(m_wucScopedBatch->remove_Completed(m_wucCompletedEventToken));
    }
}

WUCBrushManager::WUCBrushManager()
{
    m_wucColorBrushTransitions = std::make_shared<std::vector<SolidColorBrushTransitionState>>();
}

WUCBrushManager::~WUCBrushManager()
{
    m_wucColorBrushTransitions->clear();
}

void WUCBrushManager::EnsureResources(
    _In_ SharedTransitionAnimations* sharedTransitionAnimations,
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ WUComp::ICompositor* compositor,
    _In_ WUComp::ICompositionGraphicsDevice* graphicsDevice)
{
    m_sharedTransitionAnimationsNoRef = sharedTransitionAnimations;

    if (m_easingFunctionStatics == nullptr)
    {
        m_easingFunctionStatics = easingFunctionStatics;
    }
    else
    {
        ASSERT(m_easingFunctionStatics.Get() == easingFunctionStatics);
    }

    if (m_compositor == nullptr)
    {
        m_compositor = compositor;
        IFCFAILFAST(m_compositor.As(&m_compositor2));
        IFCFAILFAST(m_compositor.As(&m_compositor4));
    }
    else
    {
        ASSERT(m_compositor.Get() == compositor);
    }

    if (m_compositionGraphicsDevice == nullptr)
    {
        m_compositionGraphicsDevice = graphicsDevice;
    }
    else
    {
        ASSERT(m_compositionGraphicsDevice.Get() == graphicsDevice);
    }
}

void WUCBrushManager::ReleaseDCompResources()
{
    m_sharedTransitionAnimationsNoRef = nullptr;
    m_compositor.Reset();
    m_compositor2.Reset();
    m_compositor4.Reset();
    ReleaseGraphicsDevice();
    m_wucColorBrushTransitions->clear();
    m_dropShadowBrushCache.clear();
}

void WUCBrushManager::ReleaseGraphicsDevice()
{
    m_compositionGraphicsDevice.Reset();
}

wrl::ComPtr<WUComp::ICompositionBrush> WUCBrushManager::GetMaskBrush(
    _In_ WUComp::ICompositionBrush* existingBrush,
    _In_ HWTexture* maskTexture,
    _In_ CBrush* xamlBrush,
    const BrushParams& brushParams,
    _In_opt_ HWTexture* brushTexture,
    _In_ const CMILMatrix* brushTransform,
    _In_opt_ const XTHICKNESS* nineGrid,
    float realizationScaleX,
    float realizationScaleY,
    bool isCenterHollow,
    _In_ const XRECTF& brushBounds,
    _In_ const CUIElement* element,
    _In_ WUComp::ICompositor* compositor)
{
    wrl::ComPtr<WUComp::ICompositionMaskBrush> maskBrush;
    if (nineGrid && maskTexture)
    {
        maskBrush = GetMaskBrush(existingBrush, maskTexture, nineGrid, realizationScaleX, realizationScaleY, isCenterHollow);
    }
    else
    {
        // If the ninegrid wasn't meant for the mask texture, explicitly pass in a null ninegrid to avoid creating a WUC nine grid brush.
        maskBrush = GetMaskBrush(existingBrush, maskTexture, nullptr /* nineGrid */, 1.0 /* realizationScaleX */, 1.0 /* realizationScaleY */, false /* isCenterHollow */);
    }

    wrl::ComPtr<WUComp::ICompositionBrush> existingBrushSource;
    IFCFAILFAST(maskBrush->get_Source(existingBrushSource.ReleaseAndGetAddressOf()));

    // Currently Xaml uses a nine grid to stretch either a brush or a shape texture, but not both. IXP has no such limitations.
    // If we get both textures, it's for a scenario like a rounded corner border (with nine grid and mask for the border portion)
    // with the border brush set as an ImageBrush (with a brush texture for the image). In these cases the nine grid stretches
    // the mask and not the brush. The only nine grid that stretches a brush texture is for an Image element that has a NineGrid
    // property set, and there's no mask in that case.
    wrl::ComPtr<WUComp::ICompositionBrush> newBrushSource;
    if (nineGrid && brushTexture && !maskTexture)
    {
        // This should be a textured brush (image), not a solid color or gradient. Ignore the brush that was passed in.
        newBrushSource = GetNineGridBrush(
            existingBrushSource.Get(),
            nineGrid,
            realizationScaleX,
            realizationScaleY,
            isCenterHollow,
            xamlBrush,
            brushTexture,
            brushTransform,
            &brushBounds,
            element,
            compositor);
    }
    else
    {
        // If the ninegrid wasn't meant for the brush texture, avoid creating a WUC nine grid brush.

        if (xamlBrush != nullptr && KnownTypeIndex::SolidColorBrush == xamlBrush->GetTypeIndex())
        {
            // Rounded corner borders can use a mask brush for the rounded background. Pass the brush params along to handle
            // Border.Background brush transitions.
            newBrushSource = GetColorBrush(static_cast<CSolidColorBrush*>(xamlBrush), brushParams);
        }
        else if (xamlBrush != nullptr && KnownTypeIndex::LinearGradientBrush == xamlBrush->GetTypeIndex())
        {
            newBrushSource = GetLinearGradientBrush(static_cast<CLinearGradientBrush*>(xamlBrush), brushBounds, brushTransform);
        }
        else if (xamlBrush != nullptr && KnownTypeIndex::XamlCompositionBrushBase == xamlBrush->GetTypeIndex())
        {
            wrl::ComPtr<WUComp::ICompositionBrush> compositionBrush;
            xref_ptr<CXamlCompositionBrush> xamlCompositionBrush(static_cast<CXamlCompositionBrush *>(xamlBrush));
            IFCFAILFAST(xamlCompositionBrush->GetCompositionBrush(element, compositor, &compositionBrush));

            if (compositionBrush)
            {
                IFCFAILFAST(compositionBrush.As(&newBrushSource));
            }
        }
        else
        {
            newBrushSource = GetSurfaceBrush(existingBrushSource.Get(), brushTexture, brushTransform);
        }
    }

    if (newBrushSource != existingBrushSource)
    {
        IFCFAILFAST(maskBrush->put_Source(newBrushSource.Get()));
    }

    wrl::ComPtr<WUComp::ICompositionBrush> maskBrushAsBrush;
    IFCFAILFAST(maskBrush.As(&maskBrushAsBrush));
    return maskBrushAsBrush;
}

wrl::ComPtr<WUComp::ICompositionMaskBrush> WUCBrushManager::GetMaskBrush(
    _In_ WUComp::ICompositionBrush* existingBrush,
    _In_ HWTexture * maskTexture,
    _In_opt_ const XTHICKNESS* nineGrid,
    float realizationScaleX,
    float realizationScaleY,
    bool isCenterHollow)
{
    wrl::ComPtr<WUComp::ICompositionMaskBrush> maskBrush;
    wrl::ComPtr<WUComp::ICompositionBrush> existingBrushMask;
    wrl::ComPtr<WUComp::ICompositionBrush> newBrushMask;

    if (existingBrush != nullptr)
    {
        existingBrush->QueryInterface(IID_PPV_ARGS(maskBrush.ReleaseAndGetAddressOf()));
        if (maskBrush != nullptr)
        {
            IFCFAILFAST(maskBrush->get_Mask(&existingBrushMask));
        }
    }

    if (maskBrush == nullptr)
    {
        IFCFAILFAST(m_compositor2->CreateMaskBrush(&maskBrush));
    }

    if (nineGrid != nullptr)
    {
        newBrushMask = GetNineGridBrush(
            existingBrushMask.Get(),
            nineGrid,
            realizationScaleX,
            realizationScaleY,
            isCenterHollow,
            nullptr /* xamlBrush - we want a NineGrid from just the texture */,
            maskTexture,
            nullptr /* transform */,
            nullptr /* brushBounds */,
            nullptr /* element */,
            nullptr /* compositor */);
    }
    else
    {
        newBrushMask = GetSurfaceBrush(existingBrushMask.Get(), maskTexture, nullptr /* transform */);
    }

    if (newBrushMask != existingBrushMask)
    {
        IFCFAILFAST(maskBrush->put_Mask(newBrushMask.Get()));
    }

    return maskBrush;
}

wrl::ComPtr<WUComp::ICompositionBrush> WUCBrushManager::GetNineGridBrush(
    _In_ WUComp::ICompositionBrush* existingBrush,
    _In_ const XTHICKNESS* nineGrid,
    float maskRealizationScaleX,
    float maskRealizationScaleY,
    bool isCenterHollow,
    _In_ CBrush* xamlBrush,
    _In_opt_ HWTexture* brushTexture,
    _In_ const CMILMatrix* brushTransform,
    _In_opt_ const XRECTF* brushBounds,
    _In_opt_ const CUIElement* element,
    _In_opt_ WUComp::ICompositor* compositor)
{
    wrl::ComPtr<WUComp::ICompositionNineGridBrush> nineGridBrush = GetNineGridBrush(existingBrush, nineGrid, maskRealizationScaleX, maskRealizationScaleY, isCenterHollow);

    wrl::ComPtr<WUComp::ICompositionBrush> existingBrushSource;
    IFCFAILFAST(nineGridBrush->get_Source(existingBrushSource.ReleaseAndGetAddressOf()));

    wrl::ComPtr<WUComp::ICompositionBrush> newBrushSource;
    if (xamlBrush != nullptr && KnownTypeIndex::SolidColorBrush == xamlBrush->GetTypeIndex())
    {
        BrushParams emptyBrushParams;
        newBrushSource = GetColorBrush(static_cast<CSolidColorBrush*>(xamlBrush), emptyBrushParams);
    }
    else if (xamlBrush != nullptr && KnownTypeIndex::LinearGradientBrush == xamlBrush->GetTypeIndex())
    {
        newBrushSource = GetLinearGradientBrush(static_cast<CLinearGradientBrush*>(xamlBrush), *brushBounds, brushTransform);
    }
    else if (xamlBrush != nullptr && KnownTypeIndex::XamlCompositionBrushBase == xamlBrush->GetTypeIndex())
    {
        wrl::ComPtr<WUComp::ICompositionBrush> compositionBrush;
        xref_ptr<CXamlCompositionBrush> xamlCompositionBrush(static_cast<CXamlCompositionBrush *>(xamlBrush));
        IFCFAILFAST(xamlCompositionBrush->GetCompositionBrush(element, compositor, &compositionBrush));

        if (compositionBrush)
        {
            IFCFAILFAST(compositionBrush.As(&newBrushSource));
        }
    }
    else
    {
        newBrushSource = GetSurfaceBrush(existingBrushSource.Get(), brushTexture, brushTransform);
    }

    if (newBrushSource != existingBrushSource)
    {
        IFCFAILFAST(nineGridBrush->put_Source(newBrushSource.Get()));
    }

    wrl::ComPtr<WUComp::ICompositionBrush> nineGridBrushAsBrush;
    IFCFAILFAST(nineGridBrush.As(&nineGridBrushAsBrush));
    return nineGridBrushAsBrush;
}

wrl::ComPtr<WUComp::ICompositionNineGridBrush> WUCBrushManager::GetNineGridBrush(
    _In_ WUComp::ICompositionBrush* existingBrush,
    _In_ const XTHICKNESS* nineGrid,
    float maskRealizationScaleX,
    float maskRealizationScaleY,
    bool isCenterHollow)
{
    wrl::ComPtr<WUComp::ICompositionNineGridBrush> nineGridBrush;

    if (existingBrush != nullptr)
    {
        existingBrush->QueryInterface(IID_PPV_ARGS(nineGridBrush.ReleaseAndGetAddressOf()));
    }

    if (nineGridBrush == nullptr)
    {
        IFCFAILFAST(m_compositor2->CreateNineGridBrush(&nineGridBrush));
    }

    float inverseRealizationScaleX = (maskRealizationScaleX == 0 ? 0 : 1.0f / maskRealizationScaleX);
    float inverseRealizationScaleY = (maskRealizationScaleY == 0 ? 0 : 1.0f / maskRealizationScaleY);

    nineGridBrush->put_TopInset(nineGrid->top * maskRealizationScaleY);
    nineGridBrush->put_TopInsetScale(inverseRealizationScaleY);
    nineGridBrush->put_BottomInset(nineGrid->bottom * maskRealizationScaleY);
    nineGridBrush->put_BottomInsetScale(inverseRealizationScaleY);
    nineGridBrush->put_LeftInset(nineGrid->left * maskRealizationScaleX);
    nineGridBrush->put_LeftInsetScale(inverseRealizationScaleX);
    nineGridBrush->put_RightInset(nineGrid->right * maskRealizationScaleX);
    nineGridBrush->put_RightInsetScale(inverseRealizationScaleX);
    nineGridBrush->put_IsCenterHollow(isCenterHollow);

    return nineGridBrush;
}

wrl::ComPtr<WUComp::ICompositionBrush> WUCBrushManager::GetSurfaceBrush(
    _In_opt_ WUComp::ICompositionBrush* existingBrush,
    _In_ HWTexture* texture,
    _In_opt_ const CMILMatrix* transform)
{
    wrl::ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;
    WUComp::ICompositionSurface* surfaceNoRef = nullptr;

    if (existingBrush != nullptr)
    {
        existingBrush->QueryInterface(IID_PPV_ARGS(surfaceBrush.ReleaseAndGetAddressOf()));
    }

    if (surfaceBrush == nullptr)
    {
        IFCFAILFAST(m_compositor->CreateSurfaceBrush(&surfaceBrush));

        // Reset the default alignment properties to what we need them to be
        IFCFAILFAST(surfaceBrush->put_HorizontalAlignmentRatio(0));
        IFCFAILFAST(surfaceBrush->put_VerticalAlignmentRatio(0));
    }

    if (texture)
    {
        surfaceNoRef = texture->GetCompositionSurface()->GetWinRTSurface();

        CMILMatrix combinedTransform;
        if (transform != nullptr)
        {
            combinedTransform = *transform;
            texture->AddTextureSpaceToLocalSpaceTransform(&combinedTransform);
            IFCFAILFAST(surfaceBrush->put_Stretch(WUComp::CompositionStretch::CompositionStretch_None));
        }
        else
        {
            // Have the brush scale the texture to the visual automatically
            combinedTransform.SetToIdentity();
            IFCFAILFAST(surfaceBrush->put_Stretch(WUComp::CompositionStretch::CompositionStretch_Fill));
        }

        // The two transform types have the same internal layout.  This will soon break anyway once
        // ICompositionSurfaceBrush2 takes component transform values instead of a matrix.
        C_ASSERT(sizeof(wfn::Matrix3x2) == sizeof(combinedTransform));
        wfn::Matrix3x2* pwfnTransform = reinterpret_cast<wfn::Matrix3x2*>(&combinedTransform);

        wrl::ComPtr<WUComp::ICompositionSurfaceBrush2> surfaceBrush2;
        IFCFAILFAST(surfaceBrush.As(&surfaceBrush2));
        IFCFAILFAST(surfaceBrush2->put_TransformMatrix(*pwfnTransform));
    }

    IFCFAILFAST(surfaceBrush->put_Surface(surfaceNoRef));

    wrl::ComPtr<WUComp::ICompositionBrush> surfaceBrushAsBrush;
    IFCFAILFAST(surfaceBrush.As(&surfaceBrushAsBrush));
    return surfaceBrushAsBrush;
}

wrl::ComPtr<WUComp::ICompositionBrush> WUCBrushManager::GetLinearGradientBrush(
    _In_ CLinearGradientBrush* xamlBrush,
    _In_ const XRECTF& brushBounds,
    _In_ const CMILMatrix* brushTransform)
{
    wrl::ComPtr<WUComp::ICompositionBrush> linearGradientBrush;
    linearGradientBrush = xamlBrush->GetWUCBrush(brushBounds, brushTransform, m_compositor4.Get());
    return linearGradientBrush;
}

wrl::ComPtr<WUComp::ICompositionBrush> WUCBrushManager::GetColorBrush(_In_ CSolidColorBrush* xamlBrush, const BrushParams& brushParams)
{
    if (brushParams.m_element != nullptr)
    {
        // Look for an entry for this element/property combination in our map. If we found one, then it was set up previously
        // as a SolidColorBrush transition. If we didn't find one, then there was no SCB transition, and we should use the
        // default WUC color brush.
        for (auto& transition : *m_wucColorBrushTransitions)
        {
            if (transition.m_xamlBrush == xamlBrush
                && transition.m_element == brushParams.m_element
                && transition.m_brushProperty == brushParams.m_brushProperty)
            {
                if (transition.m_wucBrush == nullptr)
                {
                    wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
                    IFCFAILFAST(m_compositor->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
                    transition.m_wucBrush = colorBrush;
                }

                wrl::ComPtr<WUComp::ICompositionBrush> brush;
                IFCFAILFAST(transition.m_wucBrush.As(&brush));

                if (transition.m_wucScopedBatch == nullptr)
                {
                    std::optional<wu::Color> fromColor = transition.m_fromARGB ? ColorUtils::GetWUColor(transition.m_fromARGB.value()) : std::optional<wu::Color>();
                    transition.m_toColor = ColorUtils::GetWUColor(xamlBrush->m_rgb);

                    WUComp::ICompositionAnimation* brushAnimation = m_sharedTransitionAnimationsNoRef->GetBrushAnimationNoRef(m_easingFunctionStatics.Get(), m_compositor.Get(), fromColor, transition.m_toColor, transition.m_duration);

                    // Start the animation
                    wrl::ComPtr<WUComp::ICompositionObject> brushICO;
                    IFCFAILFAST(brush.As(&brushICO));

                    wrl::ComPtr<WUComp::ICompositionScopedBatch> scopedBatch;
                    IFCFAILFAST(m_compositor->CreateScopedBatch(WUComp::CompositionBatchTypes_Animation, &scopedBatch));
                    IFCFAILFAST(brushICO->StartAnimation(wrl_wrappers::HStringReference(ExpressionHelper::sc_propertyName_Color).Get(), brushAnimation));
                    scopedBatch->End();

                    CCoreServices* context = transition.m_element->GetContext();
                    context->IncrementActiveBrushTransitionCount();

                    // Make a copy of the shared pointer to the current transitions vector, and capture that by value.
                    // That will ensure the vector stays alive in the meantime.
                    std::shared_ptr<std::vector<SolidColorBrushTransitionState>> transitions = m_wucColorBrushTransitions;
                    auto callback = WRLHelper::MakeAgileCallback<wf::ITypedEventHandler<IInspectable*, WUComp::CompositionBatchCompletedEventArgs*>>(
                        [transitions](IInspectable* sender, WUComp::ICompositionBatchCompletedEventArgs* pEventArgs) -> HRESULT
                    {
                        WUCBrushManager::OnBrushTransitionCompleted(transitions, sender);
                        return S_OK;
                    });

                    IFCFAILFAST(scopedBatch->add_Completed(
                        callback.Get(),
                        &transition.m_wucCompletedEventToken));

                    transition.m_wucScopedBatch = scopedBatch;
                }

                return brush.Get(); // The ICompositionColorBrush in the map is still holding a ref.
            }
        }
    }

    // No brush transition, or one wasn't found.
    return xamlBrush->GetWUCBrush(m_compositor.Get());
}

// Returns a drop shadow ICompositionBrush from the cache if available.
wrl::ComPtr<WUComp::ICompositionBrush> WUCBrushManager::GetDropShadowBrushFromCache(_In_ const DropShadowRecipe& recipe)
{
    auto kv = m_dropShadowBrushCache.find(recipe);
    if (kv != m_dropShadowBrushCache.end())
    {
        return kv->second;
    }
    else
    {
        return nullptr;
    }
}

void WUCBrushManager::PutDropShadowBrushInCache(_In_ const DropShadowRecipe& recipe, _In_ wrl::ComPtr<WUComp::ICompositionBrush> brush)
{
    ASSERT(GetDropShadowBrushFromCache(recipe) == nullptr);
    m_dropShadowBrushCache[recipe] = brush;
}

wrl::ComPtr<ID2D1Factory1> WUCBrushManager::EnsureDropShadowD2DFactory()
{
    if (!m_dropShadowD2DFactory)
    {
        IFCFAILFAST(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, m_dropShadowD2DFactory.ReleaseAndGetAddressOf()));
    }

    return m_dropShadowD2DFactory;
}

void WUCBrushManager::SetUpBrushTransitionIfAllowed(
    const CBrush* from,
    const CBrush* to,
    const CUIElement& element,
    ElementBrushProperty elementBrushProperty,
    const CBrushTransition& brushTransition)
{
    if (from != to)
    {
        wf::TimeSpan duration = brushTransition.GetDuration();

        SetUpBrushTransitionIfAllowedHelper(from, to, element, elementBrushProperty, duration);
    }
}

void WUCBrushManager::SetUpBrushTransitionIfAllowedHelper(
    const CBrush* from,
    const CBrush* to,
    const CUIElement& element,
    ElementBrushProperty elementBrushProperty,
    wf::TimeSpan duration)
{
    const CSolidColorBrush* oldBrush = from != nullptr? do_pointer_cast<CSolidColorBrush>(from) : nullptr;
    const CSolidColorBrush* newBrush = to != nullptr ? do_pointer_cast<CSolidColorBrush>(to) : nullptr;

    const bool oldBrushIsNullOrSolidColorBrush = oldBrush != nullptr || from == nullptr;
    const bool oldBrushIsNullOrStatic = oldBrush == nullptr || !oldBrush->IsEffectiveValueInSparseStorage(KnownPropertyIndex::SolidColorBrush_ColorAnimation);

    if (// The new brush must exist and must be different from the old brush. The old brush can be either a SolidColorBrush
        // or null (in which case it will fade in from transparent). The old brush can't be a non-null brush of some other type.
        // TODO: If we want to allow null new brushes (fade to transparent), that's some unloading storage level of work.
        newBrush != nullptr
        && oldBrushIsNullOrSolidColorBrush
        && oldBrush != newBrush
        // SolidColorBrush animations work only on static brushes. Neither brush can be animating.
        && oldBrushIsNullOrStatic
        && !newBrush->IsEffectiveValueInSparseStorage(KnownPropertyIndex::SolidColorBrush_ColorAnimation))
    {
        for (auto& transition : *m_wucColorBrushTransitions)
        {
            // Look for hand off scenarios. One example of hand off is when a green brush is changed to a yellow
            // brush, then while that animation is still in progress, it gets changed to a red brush.
            //
            // In these cases, we'll detect the hand off when making the second transition (i.e. yellow to red).
            // There will already be an entry for the element+property combination, and it will be for the old
            // brush of the second transition (i.e. yellow). We want to reuse the WUC color brush and put a new
            // animation on it that goes to the new color (i.e. red).
            if (transition.m_xamlBrush == oldBrush
                && transition.m_element == &element
                && transition.m_brushProperty == elementBrushProperty)
            {
                transition.m_xamlBrush = newBrush;
                transition.m_duration = duration;

                // There's some special behavior here if the brush implicit animation hasn't started yet. In that
                // case the app changed brushes back-to-back (e.g. setting green to yellow, then immediately yellow
                // to red), and we just need to update the new value. We can reuse the old color from the existing
                // transition entry (i.e. we want to go from green directly to red).
                if (transition.m_wucScopedBatch)
                {
                    // If there's an animation playing already, clean up the scoped batch and prepare for a hand off.
                    // Note that this doesn't stop the currently playing animation.
                    // Also note that if there's a completed event currently pending, we'll no-op when get to it because
                    // we won't be able to find the scoped batch in m_wucColorBrushTransitions anymore.
                    ASSERT(transition.m_wucCompletedEventToken.value);
                    IFCFAILFAST(transition.m_wucScopedBatch->remove_Completed(transition.m_wucCompletedEventToken));
                    transition.m_wucCompletedEventToken = {0};
                    transition.m_wucScopedBatch.Reset();

                    CCoreServices* context = transition.m_element->GetContext();
                    context->DecrementActiveBrushTransitionCount();

                    // Remove the old start value. We don't want to explicitly specify the animation's value at time 0,
                    // because we want it to be the current animated value of the brush.
                    transition.m_fromARGB.reset();
                }

                return;
            }
        }

        // If the old brush exists, use its color. Otherwise fade in the new brush's color from transparent.
        XUINT32 oldRgb = oldBrush != nullptr ? oldBrush->m_rgb : (newBrush->m_rgb & 0x00ffffff);

        m_wucColorBrushTransitions->emplace_back(newBrush, &element, elementBrushProperty, oldRgb, duration);
    }
}

void WUCBrushManager::CleanUpBrushTransition(
    const CUIElement& element,
    ElementBrushProperty elementBrushProperty)
{
    for (auto iterator = m_wucColorBrushTransitions->begin(); iterator != m_wucColorBrushTransitions->end(); iterator++)
    {
        if (iterator->m_element == &element
            && iterator->m_brushProperty == elementBrushProperty)
        {
            // It's possible that the animation was set up but was never started. In those cases there won't be a scoped batch.
            if (iterator->m_wucScopedBatch.Get())
            {
                CCoreServices* context = iterator->m_element->GetContext();
                context->DecrementActiveBrushTransitionCount();
            }

            m_wucColorBrushTransitions->erase(iterator);
            break;
        }
    }
}

/* static */ void WUCBrushManager::OnBrushTransitionCompleted(std::shared_ptr<std::vector<SolidColorBrushTransitionState>> transitions, _In_ IInspectable* sender)
{
    wrl::ComPtr<WUComp::ICompositionScopedBatch> scopedBatch;
    IFCFAILFAST(sender->QueryInterface(IID_PPV_ARGS(&scopedBatch)));

    for (auto iterator = transitions->begin(); iterator != transitions->end(); ++iterator)
    {
        if (iterator->m_wucScopedBatch.Get() == scopedBatch.Get())
        {
            // Revert back to a static color. Otherwise the brush will still have an animated color that's holding its final value.
            iterator->m_wucBrush->put_Color(iterator->m_toColor);

            CCoreServices* context = iterator->m_element->GetContext();
            context->DecrementActiveBrushTransitionCount();

            transitions->erase(iterator);
            break;
        }
    }
}

void WUCBrushManager::CleanUpBrushTransitions(const CUIElement& element)
{
    for (auto iterator = m_wucColorBrushTransitions->begin(); iterator != m_wucColorBrushTransitions->end(); /* manually advance */)
    {
        if (iterator->m_element == &element)
        {
            // It's possible that the animation was set up but was never started. In those cases there won't be a scoped batch.
            if (iterator->m_wucScopedBatch.Get())
            {
                CCoreServices* context = iterator->m_element->GetContext();
                context->DecrementActiveBrushTransitionCount();
            }

            m_wucColorBrushTransitions->erase(iterator);
        }
        else
        {
            ++iterator;
        }
    }
}

bool WUCBrushManager::HasActiveBrushTransitions(const CUIElement* element)
{
    for (auto iterator = m_wucColorBrushTransitions->begin(); iterator != m_wucColorBrushTransitions->end(); ++iterator)
    {
        if (iterator->m_element == element)
        {
            return true;
        }
    }
    return false;
}

bool WUCBrushManager::HasActiveBrushTransition(const CUIElement* element, ElementBrushProperty elementBrushProperty)
{
    for (auto iterator = m_wucColorBrushTransitions->begin(); iterator != m_wucColorBrushTransitions->end(); ++iterator)
    {
        if (iterator->m_element == element && iterator->m_brushProperty == elementBrushProperty)
        {
            return true;
        }
    }
    return false;
}
