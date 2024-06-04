// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <CColor.h>
#include "FrameworkTheming.h"
#include "CustomDependencyProperty.h"
#include "DurationVO.h"
#include "RepeatBehaviorVO.h"
#include "KeyTimeVO.h"
#include <FeatureFlags.h>
#include "SystemThemingInterop.h"
#include "RuntimeEnabledFeatures.h"
#include <DXamlCore.h>

using namespace RuntimeFeatureBehavior;

_Check_return_ HRESULT CDependencyProperty::GetDefaultInheritedPropertyValue(
    _In_ CCoreServices* core,
    _Out_ CValue* defaultValue) const
{
    return core->GetDefaultInheritedPropertyValue(m_nIndex, defaultValue);
}

_Check_return_ HRESULT CDependencyProperty::GetDefaultValueFromPeer(
    _In_opt_ CDependencyObject* referenceObject,
    _Out_ CValue* defaultValue) const
{
    if (referenceObject)
    {
        return FxCallbacks::DependencyObject_GetDefaultValue(referenceObject, this, defaultValue);
    }
    else
    {
        defaultValue->Unset();
        return S_OK;
    }
}

_Check_return_ HRESULT CDependencyProperty::ValidateType(
    _In_ const CClassInfo* type) const
{
    // For now we skip validation on custom DPs, because otherwise we require an expensive GetTypeFromObject() call.
    // If you see this ASSERT while running a test it's likely that loose XAML is being applied in a LoadComponent
    // call to an incorrectly-typed instance. For example: newing up a UserControl and calling LoadComponent with
    // XAML for a Page that sets the Page's AppBar. This behavior doesn't cause a failure for appcompat reasons.
    // During the Threshold timeframe we're looking at the possibility of adding quirked validation here.
    ASSERT(Is<CCustomDependencyProperty>() || DirectUI::MetadataAPI::IsAssignableFrom(GetTargetType()->m_nIndex, type->m_nIndex));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CreateDefaultValueObject
//
//  Synopsis:
//      When default values are registered, object types are not copied into
//  the Property class because there is no generic object cloning.  Usually
//  we can get away with creating an instance of the property type (m_hType),
//  but sometimes we need to hard code special treatment to duplicate the
//  default value creation in the Create() method.
//
//  If switch() statements below get too long, consider using a different
//  property system mechanism to better handle object defaults.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CDependencyProperty::CreateDefaultValueObject(
    _In_ CCoreServices *pCore,
    _Inout_ CValue *pDefaultValue) const
{
    CREATEPARAMETERS cp(pCore);
    const CREATEPFN pfnCreate = GetPropertyType()->GetCoreConstructor();
    xref_ptr<CDependencyObject> value;

    IFC_RETURN(pfnCreate(value.ReleaseAndGetAddressOf(), &cp));

    switch (GetIndex())
    {
        case KnownPropertyIndex::ResourceDictionary_ThemeDictionaries:
        {
            CResourceDictionary* pThemeDictionaries = nullptr;
            IFC_RETURN(DoPointerCast(pThemeDictionaries, value.get()));
            pThemeDictionaries->MarkIsThemeDictionaries();

            if (pCore->IsLoadingGlobalThemeResources())
            {
                pThemeDictionaries->MarkIsGlobal();
            }
            break;
        }
    }

    pDefaultValue->Set<valueObject>(value.detach());

    return S_OK;
}

_Check_return_ HRESULT CDependencyProperty::CreateDefaultVO(
    _In_ CCoreServices *pCore,
    _Inout_ CValue *pDefaultValue) const
{
    switch (GetPropertyType()->GetIndex())
    {
        case KnownTypeIndex::Duration:
            pDefaultValue->Set<valueVO>(DurationVOHelper::Create(pCore).detach());
            break;

        case KnownTypeIndex::RepeatBehavior:
            pDefaultValue->Set<valueVO>(RepeatBehaviorVOHelper::Create(pCore).detach());
            break;

        case KnownTypeIndex::KeyTime:
            pDefaultValue->Set<valueVO>(KeyTimeVOHelper::Create(pCore).detach());
            break;

        default:
            return E_FAIL;
    }

    return S_OK;
}

// Returns the default FocusVisualSecondaryBrush or FocusVisualSecondaryBrush brush for the provided targetObject
// FrameworkElement, according to the forFocusVisualSecondaryBrush flag.
// There are two successive fallback mechanisms in case the SystemControlFocusVisualSecondary/OuterBrush brush
// cannot be found in the current resources dictionary: First the SystemAltMediumColor/SystemBaseHighColor
// resource is retrieved respectively. In the unexpected case that cannot be accessed either, the default
// black/white color is used. All those colors are flipped as needed in light themes to maintain a good
// contrast.
/*static*/
_Check_return_ HRESULT CDependencyProperty::GetDefaultFocusVisualBrush(
    _In_ FocusVisualType forType,
    _In_ CCoreServices* core,
    _In_opt_ CDependencyObject* targetObject,
    _Outptr_ CDependencyObject** ppBrush)
{
    *ppBrush = nullptr;

    if (*ppBrush == nullptr)
    {
        IFC_RETURN(GetDefaultFocusVisualBrush(
            forType,
            core,
            targetObject,
            GetFocusVisualResourceData(forType, do_pointer_cast<CFrameworkElement>(targetObject)),
            ppBrush));
    }

    return S_OK;
}

/*static*/
_Check_return_ HRESULT CDependencyProperty::GetDefaultFontIconFontFamily(
    _In_ CCoreServices* core,
    _Outptr_ CDependencyObject** fontFamily)
{
    CREATEPARAMETERS cp(core);

    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_fluentIcons, L"Segoe Fluent Icons,Segoe MDL2 Assets");
    cp.m_value.SetString(c_fluentIcons);

    IFC_RETURN(CFontFamily::Create(fontFamily, &cp));

    return S_OK;
}

/*static*/
_Check_return_ HRESULT CDependencyProperty::GetBooleanThemeResourceValue(
    _In_ CCoreServices* core,
    _In_ const xstring_ptr_view& key,
    _Out_ bool* value,
    _Out_opt_ bool* resourceExists /* = nullptr */)
{
    *value = false;
    if (resourceExists)
    {
        *resourceExists = false;
    }

    // Same as for GetTextControlFlyoutResource below:
    // TODO 17579502: In general, LookupThemeResource as a whole should return null when we're shutting down
    // in order to avoid the creation of new objects during shutdown.
    if (!DirectUI::DXamlCore::IsShuttingDownStatic())
    {
        xref_ptr<CDependencyObject> resource = nullptr;

        IFC_RETURN(core->LookupThemeResource(key, resource.ReleaseAndGetAddressOf()));

        if (resource)
        {
            if (resourceExists)
            {
                *resourceExists = true;
            }

            if (resource->GetClassInformation()->IsEnum())
            {
                CEnumerated* enumDO = checked_cast<CEnumerated>(resource);

                if (enumDO->GetEnumTypeIndex() == KnownTypeIndex::Boolean)
                {
                    *value = !!enumDO->m_nValue;
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT GetTextControlFlyoutResource(
    _In_ CCoreServices* core,
    _In_ const xstring_ptr_view& themeResourceName,
    _Outptr_ CDependencyObject** ppFlyout)
{
    *ppFlyout = nullptr;

    // TODO 17579502: In general, LookupThemeResource as a whole should return null when we're shutting down
    // in order to avoid the creation of new objects during shutdown.  However, for now, we're making a targeted fix
    // by not calling it during shutdown here to avoid an RI-blocking crash.
    if (!DirectUI::DXamlCore::IsShuttingDownStatic())
    {
        xref_ptr<CDependencyObject> flyout;
        CResourceDictionary* applicationResourceDictionary = core->GetApplicationResourceDictionary();
        
        if (applicationResourceDictionary)
        {
            IFC_RETURN(applicationResourceDictionary->GetKeyNoRef(themeResourceName, flyout.ReleaseAndGetAddressOf()));
            
            // ResourceDictionary doesn't AddRef the value it returns from GetKeyNoRef, so we need to do that ourselves.
            AddRefInterface(flyout.get());
        }
        
        if (flyout == nullptr)
        {
            IFC_RETURN(core->LookupThemeResource(themeResourceName, flyout.ReleaseAndGetAddressOf()));
        }

        if (flyout != nullptr)
        {
            *ppFlyout = flyout.detach();
        }
    }

    return S_OK;
}

/*static*/
_Check_return_ HRESULT CDependencyProperty::GetDefaultTextControlContextFlyout(
    _In_ CCoreServices* core,
    _Outptr_ CDependencyObject** ppFlyout)
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strTextControlCommandBarFlyout, L"TextControlCommandBarContextFlyout");
    
    if (!SUCCEEDED(GetTextControlFlyoutResource(core, c_strTextControlCommandBarFlyout, ppFlyout)))
    {
        *ppFlyout = nullptr;
    }
    
    return S_OK;
}

/*static*/
_Check_return_ HRESULT CDependencyProperty::GetDefaultTextControlSelectionFlyout(
    _In_ CCoreServices* core,
    _Outptr_ CDependencyObject** ppFlyout)
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strTextControlCommandBarFlyout, L"TextControlCommandBarSelectionFlyout");
    
    if (!SUCCEEDED(GetTextControlFlyoutResource(core, c_strTextControlCommandBarFlyout, ppFlyout)))
    {
        *ppFlyout = nullptr;
    }
    
    return S_OK;
}

CDependencyProperty::FocusVisualResourceData CDependencyProperty::GetFocusVisualResourceData(
    _In_ FocusVisualType forType,
    _In_opt_ CFrameworkElement* targetObject)
{
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemControlFocusVisualPrimaryBrush, L"SystemControlFocusVisualPrimaryBrush");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemControlFocusVisualSecondaryBrush, L"SystemControlFocusVisualSecondaryBrush");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemControlRevealFocusBrush, L"SystemControlRevealFocusVisualBrush");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemBaseHighColor, L"SystemBaseHighColor");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemAltMediumColor, L"SystemAltMediumColor");
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSystemAccentColor, L"SystemAccentColor");

    if (forType == FocusVisualType::Secondary)
    {
        return FocusVisualResourceData{c_strSystemControlFocusVisualSecondaryBrush, c_strSystemAltMediumColor, static_cast<uint32_t>(KnownColors::Black) };
    }
    // For primary, check if reveal focus is enabled and use that if so
    else if (CFocusRectManager::AreRevealFocusRectsEnabled())
    {
        return FocusVisualResourceData{c_strSystemControlRevealFocusBrush, c_strSystemAccentColor, SystemThemingInterop().GetSystemAccentColor() };
    }
    else
    {
        return FocusVisualResourceData{c_strSystemControlFocusVisualPrimaryBrush, c_strSystemBaseHighColor, static_cast<uint32_t>(KnownColors::White) };
    }
}

// Returns a brush picked in this order:
// 1. Based on the provided targetObject element and provided brush resource key in its current theme dictionary.
// 2. Based on the provided targetObject element and provided color resource key in its current theme dictionary.
// 3. Based on the provided fallback color.
/*static*/
_Check_return_ HRESULT CDependencyProperty::GetDefaultFocusVisualBrush(
    _In_ FocusVisualType forFocusVisualType,
    _In_ CCoreServices* core,
    _In_opt_ CDependencyObject* targetObject,
    _In_ const FocusVisualResourceData& data,
    _Outptr_ CDependencyObject** ppBrush)
{
    xref_ptr<CDependencyObject> spFocusVisualResource;
    xref_ptr<CSolidColorBrush>  spSolidColorBrush;
    CColor* pCColor = nullptr;

    *ppBrush = nullptr;

    const auto theme = (targetObject != nullptr) ? targetObject->GetTheme() : core->GetFrameworkTheming()->GetTheme();

    IFC_RETURN(core->LookupThemeResource(
        theme,
        data.BrushThemeKey,
        spFocusVisualResource.ReleaseAndGetAddressOf()));
    if (spFocusVisualResource != nullptr)
    {
        *ppBrush = spFocusVisualResource.detach();
        return S_OK;
    }

    IFC_RETURN(core->LookupThemeResource(
        theme,
        data.ColorThemeKey,
        spFocusVisualResource.ReleaseAndGetAddressOf()));
    if (spFocusVisualResource != nullptr)
    {
        IFC_RETURN(DoPointerCast(pCColor, spFocusVisualResource));
    }

    // This is an edge case where the strBrushThemeKey resource could not be found in the resource dictionary.
    // Create a solid color brush on the fly with a fallback color, or retrieve the previously cached one with the same color.
    // pCColor == nullptr means the strColorThemeKey resource could not point to a color. Use the fallback color instead.
    IFC_RETURN(core->GetDefaultFocusVisualSolidColorBrush(
        forFocusVisualType == FocusVisualType::Secondary,
        pCColor == nullptr ? data.FallbackColor : pCColor->m_rgb,
        spSolidColorBrush.ReleaseAndGetAddressOf()));
    *ppBrush = spSolidColorBrush.detach();

    return S_OK;
}
