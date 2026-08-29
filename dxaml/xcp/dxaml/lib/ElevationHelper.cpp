// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ElevationHelper.h"
#include "ThemeShadow.g.h"
#include "CompositeTransform3D.g.h"
#include "DoubleAnimation.g.h"
#include "Storyboard.g.h"
#include "ProjectedShadowManager.h"
#include "DCompTreeHost.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

namespace DirectUI {

// The initial Z offset applied to all elevated controls
static constexpr float s_elevationBaseDepth = 32.0f;
// This additional Z offset will be applied for each tier of logically parented controls
static constexpr float s_elevationIterativeDepth = 8.0f;

static constexpr INT64 s_durationTime = 1250000L; // 125 ms.

_Check_return_ HRESULT ApplyThemeShadow(_In_ xaml::IUIElement* target)
{
    ctl::ComPtr<ThemeShadow> themeShadow;
    IFC_RETURN(ctl::make(&themeShadow));

    ctl::ComPtr<IShadow> shadow;
    IFC_RETURN(themeShadow.As(&shadow));

    IFC_RETURN(target->put_Shadow(shadow.Get()));

    return S_OK;
}

_Check_return_ HRESULT ApplyElevationEffect(_In_ xaml::IUIElement* target, unsigned int depth, std::optional<int> baseElevation)
{
    // Calculate a Z translation offset based on the given depth level
    auto calculateZDepth = [baseElevation](unsigned int depth)
    {
        return baseElevation.value_or(s_elevationBaseDepth) + (depth * s_elevationIterativeDepth);
    };

    // Apply a translation facade value
    wfn::Vector3 currentTranslation {};
    IFC_RETURN(target->get_Translation(&currentTranslation));

    // Don't modify the X or Y component from what was already set
    wfn::Vector3 startTranslation = { currentTranslation.X, currentTranslation.Y, 0.0 };

    // If this is a multi-tiered depth situation we need to start at the previous depth level
    // otherwise we'll visibly cross from behind that "parent" to in front of it (which is an
    // awkward pop).
    if (depth > 0)
    {
        // Push the child just _slighty_ in front of the parent tier so that there's no Z fighting
        // for that first frame of the animation when they're at the same Z value.
        startTranslation.Z = calculateZDepth(depth - 1) + 0.001f;
    }

    // Don't modify the X or Y component from what was already set
    wfn::Vector3 endTranslation = { currentTranslation.X, currentTranslation.Y, calculateZDepth(depth) };

    IFC_RETURN(target->put_Translation(endTranslation));

    // Apply a shadow to the element
    IFC_RETURN(ApplyThemeShadow(target));

    // Animate the translation offset
    bool isAnimationEnabled = true;
    IFC_RETURN(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));
    if (!CThemeShadow::IsDropShadowMode()   // If we're in drop shadows mode, then we don't support animating Z translation.
        && isAnimationEnabled)
    {
        ctl::ComPtr<WUComp::ICompositor> compositor;
        compositor = DXamlCore::GetCurrent()->GetHandle()->GetCompositor();

        ctl::ComPtr<ixp::ICompositionEasingFunctionStatics> easingFunctionStatics;
        easingFunctionStatics = DXamlCore::GetCurrent()->GetHandle()->GetDCompTreeHost()->GetEasingFunctionStatics();

        // Create a vector keyframe animation
        ctl::ComPtr<WUComp::IVector3KeyFrameAnimation> vector3Animation;
        {
            // Use a linear easing
            ctl::ComPtr<WUComp::ICompositionEasingFunction> easingFunction;
            {
                ctl::ComPtr<WUComp::ILinearEasingFunction> linearEasingFunction;
                IFC_RETURN(easingFunctionStatics->CreateLinearEasingFunction(compositor.Get(), &linearEasingFunction));
                IFC_RETURN(linearEasingFunction.As(&easingFunction));
            }

            IFC_RETURN(compositor->CreateVector3KeyFrameAnimation(&vector3Animation));
            IFC_RETURN(vector3Animation->InsertKeyFrame(0.0f, startTranslation));
            IFC_RETURN(vector3Animation->InsertKeyFrameWithEasingFunction(1.0f, endTranslation, easingFunction.Get()));
        }

        // Set the duration
        {
            wf::TimeSpan timeSpan;
            timeSpan.Duration = s_durationTime;

            ctl::ComPtr<WUComp::IKeyFrameAnimation> keyFrameAnimation;
            IFC_RETURN(vector3Animation.As(&keyFrameAnimation));
            IFC_RETURN(keyFrameAnimation->put_Duration(timeSpan));
        }

        // Target it as a Translation animation
        {
            ctl::ComPtr<WUComp::ICompositionAnimation2> animationForTarget;
            IFC_RETURN(vector3Animation.As(&animationForTarget));
            IFC_RETURN(animationForTarget->put_Target(wrl_wrappers::HStringReference(FacadeProperty_Translation).Get()));
        }

        // Actually play it
        {
            ctl::ComPtr<WUComp::ICompositionAnimationBase> animation;
            IFC_RETURN(vector3Animation.As(&animation));

            IFC_RETURN(target->StartAnimation(animation.Get()));
        }
    }

    return S_OK;
}


_Check_return_ HRESULT ClearElevationEffect(_In_ xaml::IUIElement* target)
{
    IFC_RETURN(target->put_Shadow(nullptr));

    return S_OK;
}

_Check_return_ HRESULT IsDefaultShadowEnabled(_In_ xaml::IFrameworkElement* resourceTarget, _Inout_ bool *enabled)
{
    if (!resourceTarget)
    {
        return S_OK;
    }

    ctl::ComPtr<xaml::IResourceDictionary> resources;
    IFC_RETURN(resourceTarget->get_Resources(&resources));

    ctl::ComPtr<wfc::IMap<IInspectable*, IInspectable*>> resourcesMap;
    IFC_RETURN(resources.As(&resourcesMap));

    ctl::ComPtr<IInspectable> boxedResourceKey;
    IFC_RETURN(PropertyValue::CreateFromString(wrl_wrappers::HStringReference(L"IsDefaultShadowEnabled").Get(), boxedResourceKey.ReleaseAndGetAddressOf()));

    BOOLEAN hasKey = FALSE;
    IFC_RETURN(resourcesMap->HasKey(boxedResourceKey.Get(), &hasKey));
    if (hasKey)
    {
        ctl::ComPtr<IInspectable> boxedResource;
        IFC_RETURN(resourcesMap->Lookup(boxedResourceKey.Get(), &boxedResource));

        BOOLEAN isDefaultShadowEnabled;
        auto booleanReference = ctl::query_interface_cast<wf::IReference<bool>>(boxedResource.Get());
        IFC_RETURN(booleanReference->get_Value(&isDefaultShadowEnabled));

        *enabled = !!isDefaultShadowEnabled;
    }

    return S_OK;
}

}
