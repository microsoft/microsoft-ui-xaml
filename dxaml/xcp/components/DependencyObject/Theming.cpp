// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <corep.h>
#include <MetadataAPI.h>
#include <CDependencyObject.h>
#include <UIElement.h>
#include <dopointercast.h>
#include <TypeTableStructs.h>
#include <GridLength.h>
#include <UIAEnums.h>
#include <EnumDefs.h>
#include <primitives.h>
#include <CString.h>
#include <Point.h>
#include <Rect.h>
#include <Size.h>
#include <Double.h>
#include <InheritedProperties.h>
#include <DOCollection.h>
#include <Type.h>
#include <ModifiedValue.h>
#include <ThemeResourceExtension.h>
#include <ThemeResource.h>
#include <MetadataAPI.h>
#include <resources.h>
#include <framework.h>
#include <stack_allocator.h>
#include "theming\inc\Theme.h"
#include "resources\inc\ResourceResolver.h"
#include <FxCallbacks.h>
#include <FrameworkUdk/Containment.h>

using namespace DirectUI;
using namespace Theming;

// Bug 46010864: Explorer first frame - "CommandBarControlRootGrid" entering the tree causes 11k resource lookups
#define WINAPPSDK_CHANGEID_46010864 46010864

#pragma region Theme changes

// Notifies new property value of theme change that was applied to the property owner.
_Check_return_ HRESULT CDependencyObject::NotifyPropertyValueOfThemeChange(
    _In_ const CDependencyProperty* dp,
    _In_ CValue* effectiveValue)
{
    if (m_theme != Theme::None &&
        effectiveValue &&
        effectiveValue->AsObject() &&
        ShouldNotifyPropertyOfThemeChange(dp->GetIndex()))
    {
        IFC_RETURN(effectiveValue->AsObject()->NotifyThemeChanged(m_theme));
    }

    return S_OK;
}

// Should this property be notified of theme change?
bool CDependencyObject::ShouldNotifyPropertyOfThemeChange(_In_ KnownPropertyIndex propertyIndex)
{
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46010864>())
    {
        switch (propertyIndex)
        {
        case KnownPropertyIndex::ButtonBase_CommandParameter:
        case KnownPropertyIndex::MenuFlyoutItem_CommandParameter:

        // These can contain AppBarButtons that aren't in the visual tree yet (i.e. aren't in the child collection of the
        // CommandBar). These buttons won't render, so there's no reason to update their theme resources. We can do that
        // when they are parented to the CommandBar. This saves us from many resource lookups from those AppBarButtons.
        case KnownPropertyIndex::CommandBar_PrimaryCommands:
        case KnownPropertyIndex::CommandBar_SecondaryCommands:

            return false;

        default:
            // Don't notify property values that contain objects up the visual tree to prevent theme from propagating up the tree.
            return !IsDependencyPropertyBackReference(propertyIndex);
        }
    }
    else
    {
        // TODO: Consolidate this with IsDependencyPropertyBackReference.
        // Don't notify property values that contain objects up the visual tree to prevent theme from propagating up the tree.
        switch (propertyIndex)
        {
        case KnownPropertyIndex::ButtonBase_CommandParameter:
        case KnownPropertyIndex::MenuFlyoutItem_CommandParameter:
            return false;
        default:
            return !IsDependencyPropertyBackReference(propertyIndex);
        }
    }
}

// Tries to attach a Theme Resource if we have a Theme Resource as a property value.
_Check_return_ HRESULT CDependencyObject::TryProcessingThemeResourcePropertyValue(
    _In_ const CDependencyProperty* dp,
    _In_ CModifiedValue* pModifiedValue,
    _In_ CValue* pEffectiveValue,
    _In_ BaseValueSource baseValueSource,
    _Out_ bool* processed)
{
    xref_ptr<CThemeResource> themeResource;

    if (pModifiedValue && pModifiedValue->IsEffectiveValueThemeResource())
    {
        themeResource = pModifiedValue->GetEffectiveValueThemeResourceNoRef();
    }
    else if (pEffectiveValue && pEffectiveValue->GetType() == valueThemeResource)
    {
        themeResource = pEffectiveValue->AsThemeResource();
    }

    if (themeResource != nullptr)
    {
        // Set theme resource reference.
        IFC_RETURN(themeResource->SetThemeResourceBinding(this, dp, pModifiedValue, baseValueSource));
        *processed = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::NotifyThemeChanged(Theme theme, bool forceRefresh)
{
    // Make sure no funny business happens where someone tries to cast an unsigned int into
    // an invalid Theme value.
    ASSERT(theme < Theme::Unused);

    // If IsProcessingEnterLeave is true, then this element is already part of the
    // theme walk.  This can happen, for instance, if a custom DP's value has
    // been set to some ancestor of this node.
    if (IsProcessingThemeWalk())
    {
        return S_OK;
    }

    // If this is a framework element, then get the requested theme.
    auto thisAsFe = do_pointer_cast<CFrameworkElement>(this);
    if (thisAsFe)
    {
        theme = thisAsFe->GetRequestedThemeOverride(theme);
    }

    // Has theme changed?
    if (theme == m_theme && !forceRefresh)
    {
        return S_OK;
    }

    SetIsProcessingThemeWalk(TRUE);

    bool removeRequestedTheme = false;
    const auto oldRequestedThemeForSubTree = GetRequestedThemeForSubTreeFromCore();
    if (Theming::GetBaseValue(theme) != oldRequestedThemeForSubTree)
    {
        GetContext()->SetRequestedThemeForSubTree(theme);
        removeRequestedTheme = true;
    }
    auto themeguard = wil::scope_exit([&] {
        SetIsProcessingThemeWalk(FALSE);
        if (removeRequestedTheme) SetRequestedThemeForSubTreeOnCore(oldRequestedThemeForSubTree);
    });

    // Notify children and properties of theme change
    IFC_RETURN(NotifyThemeChangedCore(theme, forceRefresh));

    // Persist the theme on success
    m_theme = theme;
    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::NotifyThemeChangedCore(Theme theme, bool forceRefresh)
{
    return NotifyThemeChangedCoreImpl(theme, forceRefresh);
}

// ignoreGetValueFailures currently addresses [Blue Bug 637457]: For setter values which are invalid, the value may not be resolvable.
// This should only ever be set to true when being called from a Setter.
_Check_return_ HRESULT CDependencyObject::NotifyThemeChangedCoreImpl(Theme theme, bool forceRefresh, bool ignoreGetValueFailures)
{
    const CClassInfo* pClassInfo = GetClassInformation();

    ASSERT(!ignoreGetValueFailures || this->OfTypeByIndex<KnownTypeIndex::Setter>());

    // Update theme references first, and skip them below in the property value notifications.
    IFC_RETURN(UpdateAllThemeReferences());

    // Notify field-backed property values of theme change.
    for (const CPropertyBase* pProperty = pClassInfo->GetFirstProperty();
         pProperty->GetIndex() != KnownPropertyIndex::UnknownType_UnknownProperty;
         pProperty = pProperty->GetNextProperty())
    {
        if (ShouldNotifyPropertyOfThemeChange(pProperty->GetIndex()))
        {
            const CDependencyProperty* pDP = pProperty->AsOrNull<CDependencyProperty>();

            if (pDP && pDP->GetOffset() != 0 && !pDP->IsPropMethodCall())
            {
                CDependencyObject *pDONoRef = nullptr;

                // If property value is an object, notify it that theme has changed
                if (pDP->GetStorageType() == valueObject)
                {
                    pDONoRef = MapPropertyAndGroupOffsetToDO(pDP->GetOffset(), pDP->GetGroupOffset());
                }
                else if (pDP->GetStorageType() == valueAny)
                {
                    CValue value;
                    if (ignoreGetValueFailures)
                    {
                        if (SUCCEEDED(GetValueByIndex(pDP->GetIndex(), &value)))
                        {
                            pDONoRef = value.AsObject();
                        }
                    }
                    else
                    {
                        IFC_RETURN(GetValueByIndex(pDP->GetIndex(), &value));
                        pDONoRef = value.AsObject();
                    }
                }

                // Any DependencyObject property that happens to be a UIElement in the visual tree is skipped.
                // This is to avoid processing those live elements multiple times, as they are covered anyways via a full
                // traversal of the live visual tree starting from the root and going recursive in CUIElement::NotifyThemeChangedCore.
                // This in particular avoids processing an ItemsControl’s host panel multiple times since the ItemsControl.ItemsHost
                // property is a live UIElement (pProperty->GetIndex() is ItemsControl_ItemsHost for this case).

                // Note that when a UIElement enters the tree, it also applies the current theme through a call to UpdateAllThemeReferences
                // in CDependencyObject::EnterImpl.
                if (pDONoRef != nullptr && !(pDONoRef->OfTypeByIndex<KnownTypeIndex::UIElement>() && pDONoRef->IsActive()))
                {
                    IFC_RETURN(pDONoRef->NotifyThemeChanged(theme, forceRefresh));
                }
            }
        }
    }

    // Notify sparse property values of theme change.
    if (m_pValueTable != nullptr)
    {
        // To protect against reentrancy, copy into a temporary vector before iterating.
        Jupiter::arena<DefaultSparseArenaSize> localArena;
        auto tempValues = GetSparseValueEntries(localArena);
        for (auto& entry : tempValues)
        {
            if (ShouldNotifyPropertyOfThemeChange(entry.first))
            {
                // Notify objects set on properties of theme changes.
                auto pDONoRef = entry.second.value.AsObject();

                // Here too, any DependencyObject property that happens to be a UIElement in the visual tree is skipped.
                // This is to avoid processing those live elements multiple times, as they are covered anyways via a full
                // traversal of the live visual tree starting from the root and going recursive in CUIElement::NotifyThemeChangedCore.
                if (pDONoRef != nullptr && !(pDONoRef->OfTypeByIndex<KnownTypeIndex::UIElement>() && pDONoRef->IsActive()))
                {
                    IFC_RETURN(pDONoRef->NotifyThemeChanged(theme, forceRefresh));
                }
            }
        }
    }

    // Notify peer of theme change so expressions can be refreshed (e.g. Binding.TargetNullValue might be
    // a ThemeResource)
    IFC_RETURN(FxCallbacks::DependencyObject_RefreshExpressionsOnThemeChange(this, theme, forceRefresh));

    return S_OK;
}

// Resets this object's theme references. If Redstone or later app, will update
// reference's target dictionary where necessary to account for ancestor changes.
// Called during a theme change and if Redstone or later, during live enter.
_Check_return_ HRESULT CDependencyObject::UpdateAllThemeReferences()
{
    ThemeResourceMap* themeResourceMap = GetThemeResourcesStorage();

    if (themeResourceMap)
    {
        // Get the properties that have theme refs. The declared stack_vector size is set
        // high enough to handle the number of theme refs defined for any of the entries
        // in our global resources, e.g. some of the global templates are defined with
        // ListViewItemPresenter referencing at least 22 theme resources without WinUI and 41 with WinUI.
        // The CalendarView template also references at least 21 theme resources without WinUI and 49 with WinUI.
        Jupiter::stack_vector<KnownPropertyIndex, 50> properties;
        properties.m_vector.reserve(themeResourceMap->size()); // avoid reallocations
        for (const auto& entry : *themeResourceMap)
        {
            properties.m_vector.push_back(entry.first);
        }

        // Update the theme ref on each property.
        for (auto propertyIndex : properties.m_vector)
        {
            IFC_RETURN(UpdateThemeReference(propertyIndex));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CDependencyObject::UpdateThemeReference(KnownPropertyIndex propertyIndex)
{
    auto dp = MetadataAPI::GetDependencyPropertyByIndex(propertyIndex);

    // Clear the theme ref for the current property. Just continue to the next
    // one if the ref isn't in the map now, which means it no longer applies to
    // this element. That happens if it's removed while updating the ref for a
    // previous property. For example, a ref for the Style property could resolve
    // to a new style that applies different theme refs.
    xref_ptr<CThemeResource> themeResource;
    ClearThemeResource(dp, &themeResource);
    if (themeResource)
    {
        // Re-apply the theme ref.
        IFC_RETURN(SetThemeResourceBinding(
            dp,
            GetModifiedValue(dp).get(),
            themeResource.get(),
            GetBaseValueSource(dp)
        ));
    }

    return S_OK;
}

// Updates the theme reference's resolved value. Looks for the resource up the tree
// to account for ancestor changes during a theme walk or live enter.
_Check_return_ HRESULT CDependencyObject::UpdateThemeReference(_In_ CThemeResource* themeResource)
{
    bool refreshed = false;

    ASSERT(themeResource);

    if (IsActive())
    {
        // Look for the resource up the tree.
        // Update the ref's resolved value if found.
        CDependencyObject* valueNoRef;
        IFC_RETURN(Resources::ResourceResolver::FindNextResolvedValueNoRef(
            this,
            themeResource,
            ShouldCheckForResourceOverrides(),
            &valueNoRef));

        if (valueNoRef != nullptr)
        {
            IFC_RETURN(themeResource->SetLastResolvedValue(valueNoRef));
            refreshed = true; // skip refresh below
        }
    }
    // Call refresh if we're in a theme walk or the ref has been updated in the past
    // *and* the value wasn't updated already by the tree lookup above.
    if (!refreshed && (IsProcessingThemeWalk() || m_theme != Theme::None || !themeResource->IsValueFromInitialTheme()))
    {
        IFC_RETURN(themeResource->RefreshValue());
    }

    return S_OK;
}

// Sets theme value on property. Also stores the theme reference so we can refresh the value on theme changes.
_Check_return_ HRESULT CDependencyObject::SetThemeResourceBinding(
    _In_ const CDependencyProperty* pDP,
    _In_opt_ CModifiedValue* pModifiedValue,
    _In_ CThemeResource* pThemeResource,
    _In_ BaseValueSource baseValueSource)
{
    // Mark if a modified value is being set, so CDependencyObject::SetValue can differentiate
    // between a base value or a modified value being set.
    bool modiferValueBeingSet = (pModifiedValue && pModifiedValue->HasModifiers());
    if (modiferValueBeingSet) pModifiedValue->SetModifierValueBeingSet(true);
    auto modiferGuard = wil::scope_exit([&] {
        if (modiferValueBeingSet)
            pModifiedValue->SetModifierValueBeingSet(false);
    });

    auto prevTheme = Theme::None;
    bool popTheme = false;

    // Push theme that resource lookup should use to get the property value
    if (!IsProcessingThemeWalk() && m_theme != Theme::None)
    {
        prevTheme = GetRequestedThemeForSubTreeFromCore();
        if (prevTheme != Theming::GetBaseValue(m_theme))
        {
            SetRequestedThemeForSubTreeOnCore(m_theme);
            popTheme = true;
        }
    }
    auto themeguard = wil::scope_exit([&] {
        if (popTheme) GetContext()->SetRequestedThemeForSubTree(prevTheme);
    });

    // Refresh the resolved theme value.
    // Updates the target dictionary if necessary.
    IFC_RETURN(UpdateThemeReference(pThemeResource));

    {
        CValue value;
        IFC_RETURN(pThemeResource->GetLastResolvedThemeValue(&value));

        // Set value.
        bool wasFrozen = IsFrozen();
        if (wasFrozen) SimulateUnfreeze();
        auto freezeGuard = wil::scope_exit([&] { if (wasFrozen) SimulateFreeze(); });
        IFC_RETURN(SetValue(SetValueParams(pDP, value, baseValueSource)));
    }

    // Store the reference.
    SetThemeResource(pDP, pThemeResource);

    return S_OK;
}


#pragma endregion
