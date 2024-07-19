// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ElevationHelper.h"

using namespace DirectUI;

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

// The initial Z offset applied to all elevated controls
static constexpr float s_elevationBaseDepth = 32.0f;
// This additional Z offset will be applied for each tier of logically parented controls
static constexpr float s_elevationIterativeDepth = 8.0f;

_Check_return_ HRESULT ApplyThemeShadow(_In_ xaml::IUIElement* target)
{
    // Construct a ThemeShadow
    wrl::ComPtr<xaml::Media::IThemeShadowFactory> spFactory;
    wrl::ComPtr<xaml::Media::IThemeShadow> themeShadow;
    wrl::ComPtr<IInspectable> spInspectable;
    IFC_RETURN(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Media_ThemeShadow).Get(),
        &spFactory));
    IFC_RETURN(spFactory->CreateInstance(
        nullptr,
        &spInspectable,
        &themeShadow));

    // Assign it to the target element
    wrl::ComPtr<xaml::Media::IShadow> shadow;
    IFC_RETURN(themeShadow.As(&shadow));

    IFC_RETURN(target->put_Shadow(shadow.Get()));

    return S_OK;
}

_Check_return_ HRESULT ApplyElevationEffect(_In_ xaml::IUIElement* target, unsigned int depth)
{
    // Calculate a Z translation offset based on the given depth level
    auto calculateZDepth = [](unsigned int depth)
    {
        return s_elevationBaseDepth + (depth * s_elevationIterativeDepth);
    };

    wfn::Vector3 endTranslation = { 0.0, 0.0, calculateZDepth(depth) };

    // Apply a translation facade value
    IFC_RETURN(target->put_Translation(endTranslation));

    // Apply a shadow to the element
    IFC_RETURN(ApplyThemeShadow(target));

    return S_OK;
}

} } } } XAML_ABI_NAMESPACE_END
