// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BrushParams.h"
#include <optional>
#include <fwd/windows.ui.composition.h>

class HWTexture;
class CBrush;
class CSolidColorBrush;
class CLinearGradientBrush;
class CUIElement;
class SharedTransitionAnimations;
class CBrushTransition;
struct DropShadowRecipe;
struct ID2D1Factory1;

namespace DirectUI { class BrushTransition; }
namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Brushes { class SolidColorBrushUnitTests; } } } } } }

struct SolidColorBrushTransitionState
{
    // Allow only the default constructor. This struct will be constructed in-place in a map.
    SolidColorBrushTransitionState(
        const CSolidColorBrush* brush,
        const CUIElement* element,
        ElementBrushProperty brushProperty,
        XUINT32 fromARGB,
        wf::TimeSpan duration)
        : m_xamlBrush(brush)
        , m_element(element)
        , m_brushProperty(brushProperty)
        , m_fromARGB(fromARGB)
        , m_duration(duration)
        , m_toColor({0,0,0,0})
    { }
    ~SolidColorBrushTransitionState();

    // Block copy construction
    SolidColorBrushTransitionState(const SolidColorBrushTransitionState&) = delete;
    SolidColorBrushTransitionState& operator=(const SolidColorBrushTransitionState&) = delete;

    // Assignment ctor
    SolidColorBrushTransitionState(SolidColorBrushTransitionState&& other) noexcept
        : m_xamlBrush(std::move(other.m_xamlBrush))
        , m_element(std::move(other.m_element))
        , m_brushProperty(std::move(other.m_brushProperty))
        , m_fromARGB(std::move(other.m_fromARGB))
        , m_duration(std::move(other.m_duration))
        , m_wucBrush(std::move(other.m_wucBrush))
        , m_wucScopedBatch(std::move(other.m_wucScopedBatch))
        , m_wucCompletedEventToken(std::move(other.m_wucCompletedEventToken))
        , m_toColor(std::move(other.m_toColor))
    {
        other.m_wucCompletedEventToken = {0};
    }

    // Assignment
    SolidColorBrushTransitionState& operator=(SolidColorBrushTransitionState&& other) noexcept
    {
        m_xamlBrush = std::move(other.m_xamlBrush);
        m_element = std::move(other.m_element);
        m_brushProperty = std::move(other.m_brushProperty);
        m_fromARGB = std::move(other.m_fromARGB);
        m_duration = std::move(other.m_duration);
        m_wucBrush = std::move(other.m_wucBrush);
        m_wucScopedBatch = std::move(other.m_wucScopedBatch);
        m_wucCompletedEventToken = std::move(other.m_wucCompletedEventToken);
        m_toColor = std::move(other.m_toColor);

        other.m_wucCompletedEventToken = {0};

        return *this;
    }

    // TODO: If the transition animation runs long and the element using this brush exits the tree, clean up the transition
    // before the animation completes. Otherwise we can wait for the animation.
    const CSolidColorBrush* m_xamlBrush;
    const CUIElement* m_element;
    ElementBrushProperty m_brushProperty;

    std::optional<XUINT32> m_fromARGB;
    wf::TimeSpan m_duration;
    wrl::ComPtr<WUComp::ICompositionColorBrush> m_wucBrush;
    wrl::ComPtr<WUComp::ICompositionScopedBatch> m_wucScopedBatch;
    EventRegistrationToken m_wucCompletedEventToken = {0};

    // When the transition completes, we want to revert m_wucBrush back to a static color as opposed to an animation
    // that's holding a final value. This field is set when we generate the CompositionColorBrush with the brush
    // transition animation in it.
    wu::Color m_toColor;
};

class WUCBrushManager
{
    // Xaml uses the following kinds of WUC brushes:
    //
    //  CompositionColorBrush
    //    - The simplest kind of brush, corresponding to a Xaml SolidColorBrush.
    //    - Created, updated, and kept alive by the SolidColorBrush.
    //
    //  CompositionLinearGradientBrush
    //    - Corresponding to a Xaml LinearGradientBrush.
    //    - Created, updated, and kept alive by the LinearGradientBrush.
    //
    //  CompositionSurfaceBrush
    //    - Corresponding to a Xaml ImageBrush.
    //    - Not kept on the Xaml brush, since an ImageBrush shared between elements of different sizes will require
    //      different WUC surface brushes with different transforms.
    //    - Managed by this class.
    //
    //  CompositionNineGridBrush
    //    - No Xaml counterpart.
    //    - Xaml has the Image.NineGrid property (and nothing on ImageBrush).
    //    - Xaml can also use nine grids to draw borders (with a border and a background) and shapes (with a stroke and
    //      a fill) to reduce overdraw.
    //    - Managed by this class.
    //
    //  CompositionMaskBrush
    //    - No Xaml counterpart.
    //    - Xaml can use alpha masks to draw text, shapes, and rounded borders.
    //    - Managed by this class.
    //
    //  General CompositionBrush
    //    - Corresponding to a XamlCompositionBrush.
    //    - The XamlCompositionBrush exists primarily to wrap the CompositionEffectBrush so that composition effects can be applied in Xaml
    //      wherever a Brush is expected. In particular, it serves as the base class for Neon "materials" such as Acrylic. For additional flexibility,
    //      it takes a generic WUC CompositionBrush allowing any kind of comp brush to be used in Xaml.
    //
    // Notes:
    //
    // There's a hierarchy to WUC brushes. Mask brushes can accept NineGridBrushes, SurfaceBrushes, LinearGradientBrushes, and
    // ColorBrushes as inputs. NineGridBrushes can accept a SurfaceBrush or a ColorBrush as an input.
    //
    // The VisualContentRenderer uses a concept of "recycling" when updating sprite visuals in a given render data list. The entire
    // list, along with all visuals inside it generated on the previous frame, is set on the content renderer. We then iterate through
    // each one of them, updating them to render the current frame. This means we often already have a brush set on a visual, and in
    // those cases we want to update the existing brush if possible, rather than setting a new one. This also means we should walk
    // down from the top of the brush hierarchy (e.g. first check that there's a mask brush set, then check that there's a nine grid
    // brush inside that mask brush, then check that there's a surface brush inside that nine grid brush).
    //

    friend class ::Windows::UI::Xaml::Tests::Foundation::Brushes::SolidColorBrushUnitTests;

public:
    WUCBrushManager();
    virtual ~WUCBrushManager();

    void EnsureResources(
        _In_ SharedTransitionAnimations* sharedTransitionAnimations,
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ WUComp::ICompositor* compositor,
        _In_ WUComp::ICompositionGraphicsDevice* graphicsDevice);

    void ReleaseDCompResources();

    // Releases device-dependent resources and holds on to the WUC compositor.
    void ReleaseGraphicsDevice();

    // Returns a mask brush with the Mask and Source properties both set. The source is either a color brush (if solidColorBrush
    // is provided) or a surface brush (otherwise).
    wrl::ComPtr<WUComp::ICompositionBrush> GetMaskBrush(
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
        _In_ WUComp::ICompositor* compositor);

    // Returns a mask brush with the Mask property set. The caller is responsible for taking care of the Source property.
    wrl::ComPtr<WUComp::ICompositionMaskBrush> GetMaskBrush(
        _In_ WUComp::ICompositionBrush* existingBrush,
        _In_ HWTexture* maskTexture,
        _In_opt_ const XTHICKNESS* nineGrid,
        float realizationScaleX,
        float realizationScaleY,
        bool isCenterHollow);

    // Returns a nine grid brush with the insets and the Source set.
    wrl::ComPtr<WUComp::ICompositionBrush> GetNineGridBrush(
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
        _In_opt_ WUComp::ICompositor* compositor);

    // Returns a nine grid brush with the insets set. The caller is responsible for taking care of the Source property.
    wrl::ComPtr<WUComp::ICompositionNineGridBrush> GetNineGridBrush(
        _In_ WUComp::ICompositionBrush* existingBrush,
        _In_ const XTHICKNESS* nineGrid,
        float maskRealizationScaleX,
        float maskRealizationScaleY,
        bool isCenterHollow);

    // Returns an ICompositionBrush for convenience. The surface has already been updated, and an ICompositionBrush can be used directly
    // with WUC methods without a QI.
    wrl::ComPtr<WUComp::ICompositionBrush> GetSurfaceBrush(
        _In_opt_ WUComp::ICompositionBrush* existingBrush,
        _In_ HWTexture* texture,
        _In_opt_ const CMILMatrix* transform);

    // Returns an ICompositionBrush for convenience. The gradient has already been updated, and an ICompositionBrush can be used directly
    // with WUC methods without a QI.
    wrl::ComPtr<WUComp::ICompositionBrush> GetLinearGradientBrush(
        _In_ CLinearGradientBrush* xamlBrush,
        _In_ const XRECTF& brushBounds,
        _In_ const CMILMatrix* brushTransform);

    // Returns an ICompositionBrush for convenience. The color has already been updated, and an ICompositionBrush can be used directly
    // with WUC methods without a QI.
    wrl::ComPtr<WUComp::ICompositionBrush> GetColorBrush(_In_ CSolidColorBrush* xamlBrush, const BrushParams& brushParams);

    // Returns a drop shadow ICompositionBrush from the cache if available.
    wrl::ComPtr<WUComp::ICompositionBrush> GetDropShadowBrushFromCache(_In_ const DropShadowRecipe& recipe);

    // Places the given drop shadow brush in the cache with its associated recipe as the key.
    void PutDropShadowBrushInCache(_In_ const DropShadowRecipe& recipe, _In_ wrl::ComPtr<WUComp::ICompositionBrush> brush);

    // Returns the saved D2D1 Factory if initialized, otherwise create it and return it.
    wrl::ComPtr<ID2D1Factory1> EnsureDropShadowD2DFactory();

    // Setting up a SolidColorBrush transition
    void SetUpBrushTransitionIfAllowed(
        const CBrush* from,
        const CBrush* to,
        const CUIElement& element,
        ElementBrushProperty elementBrushProperty,
        const CBrushTransition& brushTransition);

    // Tearing down a SolidColorBrush transition
    static void OnBrushTransitionCompleted(std::shared_ptr<std::vector<SolidColorBrushTransitionState>> transitions, _In_ IInspectable* sender);

    void CleanUpBrushTransition(
        const CUIElement& element,
        ElementBrushProperty elementBrushProperty);

    void CleanUpBrushTransitions(const CUIElement& element);

    bool HasActiveBrushTransitions(_In_ const CUIElement* element);
    bool HasActiveBrushTransition(_In_ const CUIElement* element, ElementBrushProperty elementBrushProperty);

private:
    // Setting up a SolidColorBrush transition
    void SetUpBrushTransitionIfAllowedHelper(
        const CBrush* from,
        const CBrush* to,
        const CUIElement& element,
        ElementBrushProperty elementBrushProperty,
        wf::TimeSpan duration);

private:
    wrl::ComPtr<ixp::ICompositionEasingFunctionStatics> m_easingFunctionStatics;
    wrl::ComPtr<WUComp::ICompositor> m_compositor;
    wrl::ComPtr<WUComp::ICompositor2> m_compositor2;
    wrl::ComPtr<WUComp::ICompositor4> m_compositor4;
    wrl::ComPtr<WUComp::ICompositionGraphicsDevice> m_compositionGraphicsDevice;

    SharedTransitionAnimations* m_sharedTransitionAnimationsNoRef{};

    containers::vector_map<DropShadowRecipe, wrl::ComPtr<WUComp::ICompositionBrush>> m_dropShadowBrushCache;
    wrl::ComPtr<ID2D1Factory1> m_dropShadowD2DFactory;

    // Used for SolidColorBrush transitions
    //
    // Normally a Xaml SCB corresponds to a single WUC color brush, but with SCB transitions this is no longer the
    // case. A red Xaml SCB can contain multiple WUC color brushes - one that's transitioning from blue to red,
    // another that's transitioning from green to red, and another that's static red. Which WUC brush gets used
    // depends on what element+property combination is using the brush. We store everything in this map.
    //
    // An entry is placed in this map when a brush property changes. Then, when we get a WUC color brush for this
    // Xaml SCB, we'll create and start the color transition animation. When the animation is completed, we take
    // the entry back out of this map, so that the element+property combination reverts to the default WUC color
    // brush.
    //
    // This is a shared_ptr because we register for completed handlers on the scoped batch of the color animation,
    // and we need to keep this vector alive in the meantime. Normally DCompTreeHost controls the lifetime of the
    // WUCBrushManager, and the WUCBrushManager is not a ref-counted class.
    // TODO: Handoff
    // TODO: Starting an animation on this SCB while a transition is in progress
    std::shared_ptr<std::vector<SolidColorBrushTransitionState>> m_wucColorBrushTransitions;
};
