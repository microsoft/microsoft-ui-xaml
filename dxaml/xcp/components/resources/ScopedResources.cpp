// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ScopedResources.h>
#include <ResourceResolver.h>
#include "ScopedResources_Cloning.h"
#include "ScopedResources_Util.h"
#include "ScopedResources_Types.h"
#include "ct_hash.h"
#include <CDependencyObject.h>
#include <framework.h>
#include <XamlSchemaContext.h>
#include <XamlQualifiedObject.h>
#include <XamlProperty.h>
#include <corep.h>
#include <DOPointerCast.h>
#include <OverrideInfo.h>

namespace Resources { namespace ScopedResources
{
    struct OverridableResource
    {
        KnownPropertyIndex  index   {};
        uint32_t            hash    {};
        xstring_ptr_storage name    {};
    };

    static constexpr OverridableResource s_overridableResources[] =
    {
        // Base colors from generic.xaml

        { KnownPropertyIndex::ColorPaletteResources_AltHigh, HASHED_XSTRING_PTR_STORAGE(L"SystemAltHighColor") },
        { KnownPropertyIndex::ColorPaletteResources_AltLow, HASHED_XSTRING_PTR_STORAGE(L"SystemAltLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_AltMedium, HASHED_XSTRING_PTR_STORAGE(L"SystemAltMediumColor") },
        { KnownPropertyIndex::ColorPaletteResources_AltMediumHigh, HASHED_XSTRING_PTR_STORAGE(L"SystemAltMediumHighColor") },
        { KnownPropertyIndex::ColorPaletteResources_AltMediumLow, HASHED_XSTRING_PTR_STORAGE(L"SystemAltMediumLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_BaseHigh, HASHED_XSTRING_PTR_STORAGE(L"SystemBaseHighColor") },
        { KnownPropertyIndex::ColorPaletteResources_BaseLow, HASHED_XSTRING_PTR_STORAGE(L"SystemBaseLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_BaseMedium, HASHED_XSTRING_PTR_STORAGE(L"SystemBaseMediumColor") },
        { KnownPropertyIndex::ColorPaletteResources_BaseMediumHigh, HASHED_XSTRING_PTR_STORAGE(L"SystemBaseMediumHighColor") },
        { KnownPropertyIndex::ColorPaletteResources_BaseMediumLow, HASHED_XSTRING_PTR_STORAGE(L"SystemBaseMediumLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeAltLow, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeAltLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeBlackHigh, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeBlackHighColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeBlackLow, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeBlackLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeBlackMediumLow, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeBlackMediumLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeBlackMedium, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeBlackMediumColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeDisabledHigh, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeDisabledHighColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeDisabledLow, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeDisabledLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeHigh, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeHighColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeLow, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeMedium, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeMediumColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeMediumLow, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeMediumLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeWhite, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeWhiteColor") },
        { KnownPropertyIndex::ColorPaletteResources_ChromeGray, HASHED_XSTRING_PTR_STORAGE(L"SystemChromeGrayColor") },
        { KnownPropertyIndex::ColorPaletteResources_ListLow, HASHED_XSTRING_PTR_STORAGE(L"SystemListLowColor") },
        { KnownPropertyIndex::ColorPaletteResources_ListMedium, HASHED_XSTRING_PTR_STORAGE(L"SystemListMediumColor") },
        { KnownPropertyIndex::ColorPaletteResources_ErrorText, HASHED_XSTRING_PTR_STORAGE(L"SystemErrorTextColor") },

        // Accent colors, a.k.a. Immersive colors

        { KnownPropertyIndex::ColorPaletteResources_Accent, HASHED_XSTRING_PTR_STORAGE(L"SystemAccentColor") },
    };

    static const gsl::span<const OverridableResource> GetOverridableResources()
    {
        return gsl::span<const OverridableResource>(
            &s_overridableResources[0],
            ARRAY_SIZE(s_overridableResources));
    }

    static bool ShouldOverrideKeyName(
        const xstring_ptr_view& keyName)
    {
        uint32_t keyNameHash = ct_hash::hash(keyName.GetBuffer(), keyName.GetCount());

        for (const auto& item : GetOverridableResources())
        {
            if (keyNameHash == item.hash &&
                keyName == xstring_ptr(item.name))
            {
                return true;
            }
        }

        return false;
    }

    xstring_ptr GetOverrideKey(
        KnownPropertyIndex propertyIndex)
    {
        for (const auto& item : GetOverridableResources())
        {
            if (propertyIndex == item.index)
            {
                return xstring_ptr(item.name);
            }
        }

        return xstring_ptr::NullString();
    }

    static _Check_return_ HRESULT ShouldOverrideForType(
        _In_ const CDependencyObject* const target,
        _Out_ bool& result)
    {
        result = false;

        xstring_ptr customRuntimeClassName;
        uint32_t customRuntimeClassNameHash = 0;

        if (target->HasManagedPeer() && target->IsCustomType())
        {
            IFC_RETURN(GetRuntimeClassNameFromCoreObject(target, &customRuntimeClassName));
            customRuntimeClassNameHash = ct_hash::hash(customRuntimeClassName.GetBuffer(), customRuntimeClassName.GetCount());
        }

        const KnownTypeIndex targetTypeIndex = target->GetTypeIndex();

        for (const auto& item : GetCloneableTypes())
        {
            if (targetTypeIndex == item.index &&
                customRuntimeClassNameHash == item.customTypeNameHash &&
                customRuntimeClassName.Equals(xstring_ptr(item.customTypeName)))
            {
                result = true;
                return S_OK;
            }
        }

        return S_OK;
    }

    static _Check_return_ HRESULT TraverseVisualTreeResources(
        _In_ const CDependencyObject* const startObject,
        const std::function<HRESULT(const CFrameworkElement*, CResourceDictionary*, bool&)>& visitor)
    {
        const CDependencyObject* current = startObject;

        while (current != nullptr)
        {
            if (const CFrameworkElement* currentAsFe = do_pointer_cast<CFrameworkElement>(current))
            {
                if (xref_ptr<CResourceDictionary> resources = currentAsFe->GetResourcesNoCreate())
                {
                    bool done = false;

                    IFC_RETURN(visitor(currentAsFe, resources, done));

                    if (done)
                    {
                        return S_OK;
                    }
                }

                // this is here on purpose, to avoid runtime type check.
                current = const_cast<CFrameworkElement*>(currentAsFe)->GetParentFollowPopups();
            }
            else if (const CUIElement* currentAsUIE = do_pointer_cast<CUIElement>(current))
            {
                current = const_cast<CUIElement*>(currentAsUIE)->GetParentFollowPopups();
            }
            else
            {
                current = current->GetParentInternal(false /* public */);
            }
        }

        return S_OK;
    }

    static _Check_return_ HRESULT GetOrCreateOverrideNoRef(
        const stack_vector_t<FoundOverride>& overrides,
        const xstring_ptr_view& referencingResourceKey,
        _In_ const CDependencyObject* const referencingResource,
        _In_opt_ const CResourceDictionary* const referencingResourceDictionary,
        _Outptr_result_maybenull_ CDependencyObject** resultObj,
        _Out_opt_ xref_ptr<CResourceDictionary>* resultDict)
    {
        CResourceDictionary* dictionary = overrides[0].dictionary;

        // This check is an optimization to avoid re-checking dictionaries if overridden
        // value exists.

        if (dictionary != referencingResourceDictionary)
        {
            CResourceDictionary* existingDict = nullptr;

            // See if it already exists.

            IFC_RETURN(dictionary->GetLocalOverrideNoRef(
                referencingResourceKey,
                resultObj,
                &existingDict));

            if (!*resultObj)
            {
                // If not, clone and insert into dictionary.

                xref_ptr<CDependencyObject> clonedCoreObj;
                ctl::ComPtr<xaml::IDependencyObject> clonedFxObj;

                IFC_RETURN(CreateOverrideNoRef(
                    referencingResource,
                    overrides,
                    clonedCoreObj.ReleaseAndGetAddressOf(),
                    clonedFxObj.ReleaseAndGetAddressOf()));

                CValue clonedCoreObjValue;
                clonedCoreObjValue.Wrap<valueObject>(clonedCoreObj.get());

                xstring_ptr keyCopy;
                IFC_RETURN(referencingResourceKey.Promote(&keyCopy));

                IFC_RETURN(dictionary->AddOverride(
                    keyCopy,
                    &clonedCoreObjValue,
                    nullptr));

                // Dictionary owns the clone now...

                *resultObj = clonedCoreObj.get();
            }

            if (resultDict)
            {
                *resultDict = dictionary;
            }
        }

        return S_OK;
    }

    static _Check_return_ HRESULT ResolveReferencingResource(
        const xstring_ptr_view& keyName,
        _In_ CCoreServices* core,
        _Outptr_result_maybenull_ const CDependencyObject** resultObj,
        _Outptr_result_maybenull_ const OverrideInfo** resultInfo)
    {
        *resultObj = nullptr;
        *resultInfo = nullptr;

        CDependencyObject* referenceValue = nullptr;
        const OverrideInfo* overrideInfo = nullptr;

        if (CResourceDictionary* themeResources = core->GetThemeResources())
        {
            IFC_RETURN(themeResources->GetKeyNoRef(
                    keyName,
                    LookupScope::LocalOnly,
                    &referenceValue));
        }

        if (referenceValue)
        {
            overrideInfo = referenceValue->GetOverrideResourceKeyStorage();
        }

        *resultObj = referenceValue;
        *resultInfo = overrideInfo;

        return S_OK;
    }

    static _Check_return_ HRESULT GetParentResourceDictionaryNoRef(
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _Outptr_result_maybenull_ CResourceDictionary** dictionary)
    {
        *dictionary = nullptr;

        std::shared_ptr<XamlSchemaContext> schemaContext;
        IFC_RETURN(spServiceProviderContext->GetSchemaContext(schemaContext));

        XamlPropertyToken propertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::FrameworkElement_Resources);
        XamlTypeToken typeToken(XamlTypeInfoProviderKind::tpkNative, KnownTypeIndex::ResourceDictionary);

        std::shared_ptr<XamlProperty> xamlProperty;
        IFC_RETURN(schemaContext->GetXamlProperty(
            propertyToken,
            typeToken,
            xamlProperty));

        std::shared_ptr<XamlType> xamlType;
        IFC_RETURN(schemaContext->GetXamlType(
            typeToken,
            xamlType));

        std::shared_ptr<XamlQualifiedObject> resourceDictionaryObject;
        IFC_RETURN(spServiceProviderContext->GetAmbientValue(
            std::shared_ptr<XamlType>(),
            std::shared_ptr<XamlType>(),
            xamlProperty,
            std::shared_ptr<XamlProperty>(),
            xamlType,
            CompressedStackCacheHint::Resources,
            resourceDictionaryObject));

        if (resourceDictionaryObject)
        {
            *dictionary = do_pointer_cast<CResourceDictionary>(resourceDictionaryObject->GetDependencyObject());
        }

        return S_OK;
    }

    _Check_return_ HRESULT MarkAsOverride(
        const xstring_ptr_view& keyName,
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext)
    {
        CDependencyObject* target = nullptr;
        KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;

        {
            std::shared_ptr<XamlQualifiedObject> spXamlTargetObject;
            spServiceProviderContext->GetMarkupExtensionTargetObject(spXamlTargetObject);

            if (spXamlTargetObject)
            {
                target = spXamlTargetObject->GetDependencyObject();

                std::shared_ptr<XamlProperty> spXamlTargetProperty;
                spServiceProviderContext->GetMarkupExtensionTargetProperty(spXamlTargetProperty);

                if (spXamlTargetProperty &&
                    spXamlTargetProperty->get_PropertyToken().GetProviderKind() != tpkParser)
                {
                    propertyIndex = spXamlTargetProperty->get_PropertyToken().GetHandle();
                }
            }
        }

        // If there's no target -- don't mark.

        if (!target ||
            propertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty)
        {
            return S_OK;
        }

        auto overrideInfo = target->GetOverrideResourceKeyStorage();

        // Existence of overrideInfo means it has been marked before, so don't re-check.

        if (!overrideInfo)
        {
            // Is it overrideable key name?

            if (!ShouldOverrideKeyName(keyName))
            {
                return S_OK;
            }

            // Is it overrideable type?

            bool overrideType = false;
            IFC_RETURN(ShouldOverrideForType(target, overrideType));

            if (!overrideType)
            {
                return S_OK;
            }

            // Is it a framework brush?

            CResourceDictionary* dictionary = nullptr;

            IFC_RETURN(GetParentResourceDictionaryNoRef(
                spServiceProviderContext,
                &dictionary));

            if (!dictionary ||
                !dictionary->IsGlobal())
            {
                return S_OK;
            }

            overrideInfo = &target->EnsureOverrideResourceKeyStorage();
        }

        ASSERT(overrideInfo);

        xstring_ptr keyCopy;
        IFC_RETURN(keyName.Promote(&keyCopy));
        overrideInfo->Add(propertyIndex, keyCopy);

        return S_OK;
    }

    static _Check_return_ HRESULT CheckDictionaryForOverrides(
        _Inout_ OverrideInfo& overrideInfo,
        _In_ CResourceDictionary* dictionary,
        uint16_t distance,
        _Out_ stack_vector_t<FoundOverride>& overrides)
    {
        for (size_t i = 0; i < OverrideInfo::c_maxOverridesPerType; ++i)
        {
            auto current = overrideInfo.At(i);

            if (current.first != KnownPropertyIndex::UnknownType_UnknownProperty)
            {
                CDependencyObject* overrideValue = nullptr;
                CResourceDictionary* overrideDictionary = nullptr;

                IFC_RETURN(dictionary->GetLocalOverrideNoRef(
                    current.second,
                    &overrideValue,
                    &overrideDictionary));

                if (overrideValue)
                {
                    ASSERT(overrideDictionary);
                    overrides.push_back({ overrideValue, overrideDictionary, distance, current.first });
                    overrideInfo.Clear(i);
                }
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT TryCreateOverrideForContext(
        const xstring_ptr_view& referencingResourceKey,
        _In_ const CDependencyObject* const referencingResource,
        _In_ const CResourceDictionary* const referencingResourceDictionary,
        const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _Outptr_result_maybenull_ CDependencyObject** resultObj,
        _Out_opt_ xref_ptr<CResourceDictionary>* resultDict)
    {
        *resultObj = nullptr;

        if (resultDict)
        {
            *resultDict = nullptr;
        }

        const OverrideInfo* const overrideInfo = referencingResource->GetOverrideResourceKeyStorage();

        if (overrideInfo)
        {
            // Get all resource dictionaries from parser context.

            AmbientValuesVector ambientValues;
            CResourceDictionary* ambientDictionary = nullptr;

            IFC_RETURN(ResourceResolver::GetAmbientValues(
                spServiceProviderContext,
                ambientValues));

            Jupiter::stack_vector<FoundOverride, DefaultVectorSize> overridesWithArena;
            auto& overrides = overridesWithArena.m_vector;

            OverrideInfo overrideInfoCopy = *overrideInfo;
            uint16_t distance = 0;

            for (auto& ambientValue : ambientValues.m_vector)
            {
                IFC_RETURN(DoPointerCast(ambientDictionary, ambientValue));

                IFC_RETURN(CheckDictionaryForOverrides(
                    overrideInfoCopy,
                    ambientDictionary,
                    distance,
                    overrides));

                if (overrideInfoCopy.Count() == 0)
                {
                    // If we found all possible overrides, exit early.
                    break;
                }

                ++distance;
            }

            if (!overrides.empty())
            {
                IFC_RETURN(GetOrCreateOverrideNoRef(
                    overrides,
                    referencingResourceKey,
                    referencingResource,
                    referencingResourceDictionary,
                    resultObj,
                    resultDict));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT GetOrCreateOverrideForVisualTree(
        _In_ const CDependencyObject* const object,
        const xstring_ptr_view& keyName,
        bool searchForOverrides,
        _Out_ CDependencyObject** resultObj,
        _Out_opt_ xref_ptr<CResourceDictionary>* resultDict)
    {
        *resultObj = nullptr;

        if (resultDict)
        {
            *resultDict = nullptr;
        }

        // First look for alias of resource -- RS1 behavior.

        const CFrameworkElement* whereFound = nullptr;

        IFC_RETURN(TraverseVisualTreeResources(
            object,
            [&keyName, resultObj, resultDict, &whereFound](const CFrameworkElement* fe, CResourceDictionary* dict, bool& done) -> HRESULT
            {
                IFCFAILFAST(dict->GetKeyForResourceResolutionNoRef(
                            keyName,
                            Resources::LookupScope::LocalOnly,
                            resultObj,
                            resultDict));

                if (*resultObj)
                {
                    whereFound = fe;
                    done = true;
                }

                return S_OK;
            }
        ));

        // If there are no override dictionaries, save some work and bail early.

        if (!searchForOverrides)
        {
            return S_OK;
        }

        const CDependencyObject* referencingResource = nullptr;
        const OverrideInfo* overrideInfo = nullptr;

        // Figure out the override key, either from propagated value or by looking into global dictionaries.

        if (*resultObj)
        {
            referencingResource = *resultObj;
            overrideInfo = (*resultObj)->GetOverrideResourceKeyStorage();
        }

        if (!overrideInfo)
        {
            IFC_RETURN(ResolveReferencingResource(
                keyName,
                object->GetContext(),
                &referencingResource,
                &overrideInfo));
        }

        if (overrideInfo)
        {
            ASSERT(referencingResource);

            Jupiter::stack_vector<FoundOverride, DefaultVectorSize> overridesWithArena;
            auto& overrides = overridesWithArena.m_vector;

            // Look for override keys up to, but excluding alias found earlier.

            OverrideInfo overrideInfoCopy = *overrideInfo;
            uint16_t distance = 0;

            IFC_RETURN(TraverseVisualTreeResources(
                object,
                [&overrideInfoCopy, &overrides, whereFound, &distance](const CFrameworkElement* fe, CResourceDictionary* dict, bool& done) -> HRESULT
                {
                    if (fe != whereFound)
                    {
                        IFC_RETURN(CheckDictionaryForOverrides(
                            overrideInfoCopy,
                            dict,
                            distance,
                            overrides));

                        ++distance;

                        // Done when all possible overrides were found.
                        done = overrideInfoCopy.Count() == 0;
                    }
                    else
                    {
                        // Or we reached the node which had alias defined.
                        done = true;
                    }

                    return S_OK;
                }
            ));

            if (!overrides.empty())
            {
                IFC_RETURN(GetOrCreateOverrideNoRef(
                    overrides,
                    keyName,
                    referencingResource,
                    nullptr,
                    resultObj,
                    resultDict));
            }
        }

        return S_OK;
    }

    _Check_return_ HRESULT TryGetOrCreateOverrideForDictionary(
        _In_ CResourceDictionary* const thisDictionary,
        _In_opt_ const CDependencyObject* const foundValue,
        _In_opt_ const CResourceDictionary* const foundValueDictionary,
        const xstring_ptr_view& keyName,
        _Out_ CDependencyObject** resultObj,
        _Out_opt_ xref_ptr<CResourceDictionary>* resultDict)
    {
        *resultObj = nullptr;

        if (resultDict)
        {
            *resultDict = nullptr;
        }

        const CDependencyObject* referencingResource = nullptr;
        const OverrideInfo* overrideInfo = nullptr;

        // Figure out the override key, either from propagated value or by looking into global dictionaries.

        if (foundValue)
        {
            referencingResource = foundValue;
            overrideInfo = foundValue->GetOverrideResourceKeyStorage();
        }

        if (!overrideInfo)
        {
            IFC_RETURN(ResolveReferencingResource(
                keyName,
                thisDictionary->GetContext(),
                &referencingResource,
                &overrideInfo));
        }

        if (overrideInfo)
        {
            ASSERT(referencingResource);

            Jupiter::stack_vector<FoundOverride, DefaultVectorSize> overridesWithArena;
            auto& overrides = overridesWithArena.m_vector;

            OverrideInfo overrideInfoCopy = *overrideInfo;

            IFC_RETURN(CheckDictionaryForOverrides(
                        overrideInfoCopy,
                        thisDictionary,
                        0,
                        overrides));

            if (!overrides.empty())
            {
                IFC_RETURN(GetOrCreateOverrideNoRef(
                    overrides,
                    keyName,
                    referencingResource,
                    foundValueDictionary,
                    resultObj,
                    resultDict));
            }
        }

        return S_OK;
    }
} }