// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ScopedResources_Cloning.h"
#include "ScopedResources_Util.h"
#include "ct_hash.h"
#include <ManagedObjectReference.h>
#include <CValueBoxer.h>
#include <CustomDependencyProperty.h>
#include <DependencyObject.h>
#include <ActivationAPI.h>
#include <xstring_ptr.h>
#include <DOPointerCast.h>
#include <ThemeResourceExtension.h>
#include <MetadataAPI.h>
#include <Resources.h>

namespace Resources { namespace ScopedResources
{
    // Until we have a public mechanism for cloning objects, we restrict types which can be cloned to the ones below.
    // In addition, only framework defined brushes participate in resource overriding, so we can be certain
    // we will not try to clone types we do not know how to clone (e.g. we do not support overrides on redefined brushes).
    // These restrictions might be lifted in future releases.

    static constexpr CloneableType s_cloneableTypes[] =
    {
        { KnownTypeIndex::SolidColorBrush },
        { KnownTypeIndex::XamlCompositionBrushBase, HASHED_XSTRING_PTR_STORAGE(L"Microsoft.UI.Xaml.Media.AcrylicBrush") },
        { KnownTypeIndex::XamlCompositionBrushBase, HASHED_XSTRING_PTR_STORAGE(L"Microsoft.UI.Xaml.Media.RevealBackgroundBrush") },
        { KnownTypeIndex::XamlCompositionBrushBase, HASHED_XSTRING_PTR_STORAGE(L"Microsoft.UI.Xaml.Media.RevealBorderBrush") },
    };

    const gsl::span<const CloneableType> GetCloneableTypes()
    {
        return gsl::span<const CloneableType>(
            &s_cloneableTypes[0],
            ARRAY_SIZE(s_cloneableTypes));
    }

    static _Check_return_ HRESULT CreateInstanceFromPrototype(
        _In_ const CDependencyObject* const prototype,
        _Outptr_ CDependencyObject** coreInstance,
        _Outptr_result_maybenull_ xaml::IDependencyObject** fxInstance)
    {
        *coreInstance = nullptr;
        *fxInstance = nullptr;

        bool createCoreOnly = true;

        xstring_ptr customRuntimeClassName;
        uint32_t customRuntimeClassNameHash = 0;

        if (prototype->HasManagedPeer() && prototype->IsCustomType())
        {
            IFC_RETURN(GetRuntimeClassNameFromCoreObject(prototype, &customRuntimeClassName));
            customRuntimeClassNameHash = ct_hash::hash(customRuntimeClassName.GetBuffer(), customRuntimeClassName.GetCount());
            createCoreOnly = false;
        }

        const CClassInfo* prototypeCoreClass = prototype->GetClassInformation();
        const KnownTypeIndex prototypeTypeIndex = prototypeCoreClass->GetIndex();

        xref_ptr<CDependencyObject> newCoreInstance;
        ctl::ComPtr<xaml::IDependencyObject> newFxInstance;

        for (const auto& item : GetCloneableTypes())
        {
            if (prototypeTypeIndex == item.index &&
                customRuntimeClassNameHash == item.customTypeNameHash &&
                customRuntimeClassName.Equals(xstring_ptr(item.customTypeName)))
            {
                if (createCoreOnly)
                {
                    IFC_RETURN(DirectUI::ActivationAPI::ActivateCoreInstance(
                        prototypeCoreClass,
                        newCoreInstance.ReleaseAndGetAddressOf()));
                }
                else
                {
                    const CClassInfo* prototypeCustomClass = nullptr;
                    IFC_RETURN(DirectUI::MetadataAPI::GetClassInfoByFullName(
                        customRuntimeClassName,
                        &prototypeCustomClass));

                    ctl::ComPtr<IInspectable> spNewInst;
                    IFC_RETURN(DirectUI::ActivationAPI::ActivateInstance(
                        prototypeCustomClass,
                        nullptr,
                        spNewInst.ReleaseAndGetAddressOf()));

                    IFC_RETURN(spNewInst.As(&newFxInstance));

                    newCoreInstance = newFxInstance.Cast<DirectUI::DependencyObject>()->GetHandle();
                }

                break;
            }
        }

        *coreInstance = newCoreInstance.detach();
        *fxInstance = newFxInstance.Detach();

        return S_OK;
    }

    static void EnumerateLocalCoreDependencyProperties(
        _In_ const CDependencyObject* const obj,
        _Out_ stack_vector_t<KnownPropertyIndex>& indices)
    {
        for (auto propertyBase = obj->GetClassInformation()->GetFirstProperty();
            propertyBase->GetIndex() != KnownPropertyIndex::UnknownType_UnknownProperty;
            propertyBase = propertyBase->GetNextProperty())
        {
            if (const CDependencyProperty* dp = propertyBase->AsOrNull<CDependencyProperty>())
            {
                if (dp->IsPublic() &&
                    obj->GetBaseValueSource(dp) != BaseValueSource::BaseValueSourceDefault)
                {
                    indices.push_back(dp->GetIndex());
                }
            }
        }
    }

    static _Check_return_ HRESULT EnumerateLocalCustomAndAttachedDependencyProperties(
        _In_ const CDependencyObject* const obj,
        _Out_ stack_vector_t<KnownPropertyIndex>& indices)
    {
        auto& sparseValues = obj->GetValueTable();

        if (sparseValues)
        {
            for (auto& keyValue : *sparseValues)
            {
                const CPropertyBase* propertyBase = DirectUI::MetadataAPI::GetPropertyBaseByIndex(keyValue.first);

                if (propertyBase->Is<CCustomProperty>() ||
                    propertyBase->Is<CCustomDependencyProperty>())
                {
                    indices.push_back(propertyBase->GetIndex());
                }
                else if (const CDependencyProperty* dp = propertyBase->AsOrNull<CDependencyProperty>())
                {
                    if (dp->IsPublic() && dp->IsAttached())
                    {
                        indices.push_back(dp->GetIndex());
                    }
                }
            }
        }

        return S_OK;
    }

    static _Check_return_ HRESULT EnumerateLocalProperties(
        _In_ const CDependencyObject* const obj,
        _Out_ stack_vector_t<KnownPropertyIndex>& indices)
    {
        indices.clear();
        EnumerateLocalCoreDependencyProperties(obj, indices);
        IFC_RETURN(EnumerateLocalCustomAndAttachedDependencyProperties(obj, indices));
        return S_OK;
    }

    static _Check_return_ HRESULT RemovePropertiesWithOverrides(
        const stack_vector_t<FoundOverride>& overrides,
        _Inout_ stack_vector_t<KnownPropertyIndex>& indices)
    {
        for (const auto& item : overrides)
        {
            const CDependencyProperty* property = nullptr;

            IFC_RETURN(DirectUI::MetadataAPI::TryGetUnderlyingDependencyProperty(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(item.propertyIndex),
                &property));

            indices.erase(
                std::remove(
                    std::begin(indices),
                    std::end(indices),
                    property->GetIndex()),
                std::end(indices));
        }

        return S_OK;
    }

    static _Check_return_ HRESULT ClonePropertyValues(
        _In_ const CDependencyObject* const from,
        _Inout_ CDependencyObject* const to,
        const stack_vector_t<KnownPropertyIndex>& indices)
    {
        for (KnownPropertyIndex index : indices)
        {
            auto property = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(index);

            if (auto customProperty = property->AsOrNull<CCustomProperty>())
            {
                auto xamlProperty = customProperty->GetXamlPropertyNoRef();
                ctl::ComPtr<IInspectable> value;

                IFC_RETURN(xamlProperty->GetValue(ctl::iinspectable_cast(from->GetDXamlPeer()), &value));
                IFC_RETURN(xamlProperty->SetValue(ctl::iinspectable_cast(to->GetDXamlPeer()), value.Get()));
            }
            else
            {
                CValue value;
                IFC_RETURN(const_cast<CDependencyObject*>(from)->GetValue(property, &value));
                IFC_RETURN(to->SetValue(property, value));
            }
        }

        return S_OK;
    }

    static _Check_return_ HRESULT UnwrapMOR(
        _In_ CDependencyObject* const inValue,
        _Out_ CValue& out)
    {
        auto mor = do_pointer_cast<CManagedObjectReference>(inValue);

        if (mor &&
            !mor->m_nativeValue.IsNull())
        {
            IFC_RETURN(out.CopyConverted(mor->m_nativeValue));
        }
        else
        {
            out.Wrap<valueObject>(inValue);
        }

        return S_OK;
    }

    static _Check_return_ HRESULT SetValueHelper(
        const FoundOverride& foundOverride,
        _In_ CDependencyObject* const coreObj)
    {
        CValue propertyValue;

        IFC_RETURN(UnwrapMOR(
            foundOverride.value,
            propertyValue));

        auto property = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(foundOverride.propertyIndex);

        if (auto customProperty = property->AsOrNull<CCustomProperty>())
        {
            ctl::ComPtr<IInspectable> spValue;
            IFC_RETURN(DirectUI::CValueBoxer::UnboxObjectValue(&propertyValue, property->GetPropertyType(), &spValue));
            IFC_RETURN(customProperty->GetXamlPropertyNoRef()->SetValue(ctl::iinspectable_cast(coreObj->GetDXamlPeer()), spValue.Get()));
        }
        else
        {
            IFC_RETURN(coreObj->SetValue(
                property,
                propertyValue));
        }

        return S_OK;
    }

    static _Check_return_ HRESULT SetupThemeResourceBinding(
        const FoundOverride& foundOverride,
        const OverrideInfo& overrideInfo,
        _In_ CDependencyObject* const coreObj)
    {
        CREATEPARAMETERS cp(coreObj->GetContext());
        xref_ptr<CThemeResourceExtension> themeResourceExt;

        IFC_RETURN(CThemeResourceExtension::Create(
            reinterpret_cast<CDependencyObject**>(themeResourceExt.ReleaseAndGetAddressOf()),
            &cp));

        themeResourceExt->m_strResourceKey = overrideInfo.Get(foundOverride.propertyIndex);

        IFC_RETURN(themeResourceExt->SetInitialValueAndTargetDictionary(
            foundOverride.value,
            foundOverride.dictionary));

        const CDependencyProperty* property = nullptr;

        IFC_RETURN(DirectUI::MetadataAPI::TryGetUnderlyingDependencyProperty(
            DirectUI::MetadataAPI::GetDependencyPropertyByIndex(foundOverride.propertyIndex),
            &property));

        IFC_RETURN(themeResourceExt->SetThemeResourceBinding(
            coreObj,
            property));

        return S_OK;
    }

    static _Check_return_ HRESULT SetOverrideValues(
        const stack_vector_t<FoundOverride>& overrides,
        _In_ CDependencyObject* const coreObj)
    {
        for (const auto& item : overrides)
        {
            IFC_RETURN(SetValueHelper(
                item,
                coreObj));
        }

        return S_OK;
    }

    _Check_return_ HRESULT CreateOverrideNoRef(
        _In_ const CDependencyObject* const referencingResource,
        const stack_vector_t<FoundOverride>& overrides,
        _Outptr_ CDependencyObject** resultCore,
        _Outptr_ xaml::IDependencyObject** resultFx)
    {
        *resultCore = nullptr;
        *resultFx = nullptr;

        xref_ptr<CDependencyObject> clonedCoreObj;
        ctl::ComPtr<xaml::IDependencyObject> clonedFxObj;

        IFC_RETURN(CreateInstanceFromPrototype(
            referencingResource,
            clonedCoreObj.ReleaseAndGetAddressOf(),
            clonedFxObj.ReleaseAndGetAddressOf()));

        Jupiter::stack_vector<KnownPropertyIndex, DefaultVectorSize> propertiesToCloneWithArena;
        auto& propertiesToClone = propertiesToCloneWithArena.m_vector;

        IFC_RETURN(EnumerateLocalProperties(
            referencingResource,
            propertiesToClone));

        IFC_RETURN(RemovePropertiesWithOverrides(
            overrides,
            propertiesToClone));

        IFC_RETURN(ClonePropertyValues(
            referencingResource,
            clonedCoreObj,
            propertiesToClone));

        IFC_RETURN(SetOverrideValues(
            overrides,
            clonedCoreObj));

        // Propagate override key name to short-circuit lookups.
        clonedCoreObj->EnsureOverrideResourceKeyStorage() = *referencingResource->GetOverrideResourceKeyStorage();

        *resultCore = clonedCoreObj.detach();
        *resultFx = clonedFxObj.Detach();

        return S_OK;
    }
} }