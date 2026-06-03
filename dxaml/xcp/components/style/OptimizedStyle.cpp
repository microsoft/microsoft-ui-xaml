// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CValue.h>
#include "OptimizedStyle.h"
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>
#include <collectionbase.h>
#include <DOCollection.h>
#include <SetterBaseCollection.h>
#include <NoParentShareableDependencyObject.h>
#include <DependencyPropertyProxy.h>
#include <DoPointerCast.h>
#include <ICustomWriterRuntimeDataReceiver.h>
#include <Style.h>
#include <Setter.h>
#include <TypeTableStructs.h>
#include <MetadataAPI.h>
#include <StyleCustomRuntimeData.h>
#include <CustomWriterRuntimeContext.h>
#include <CustomWriterRuntimeObjectCreator.h>
#include <XamlOptimizedNodeList.h>
#include <EnumDefs.h>
#include <MarkupExtension.h>
#include <Binding.h>
#include <DeferredKeys.h>
#include <Resources.h>
#include <Type.h>
#include <XamlProperty.h>
#include <ManagedObjectReference.h>
#include <ThemeResourceExtension.h>
#include <ThemeResource.h>
#include <ValueBuffer.h>
#include <AutoReentrantReferenceLock.h>
#include <XamlQualifiedObject.h>
#include "theming\inc\Theme.h"
#include <LineInfo.h>
#include <ParserErrorService.h>
#include <xcperrorresource.h>
#include <PerfOptIn.h>

using namespace DirectUI;

OptimizedStyle::OptimizedStyle(_In_ CStyle* const style)
    : m_style(style)
    , m_isSealed(false)
    , m_targetType(KnownTypeIndex::UnknownType)
    , m_basedOnBegin(0)
{
}

// Initialize setter info from non-optimized style's setters.
_Check_return_ HRESULT OptimizedStyle::Initialize()
{
    ASSERT(m_style->m_optimizedStyle == nullptr);

    m_targetType = m_style->m_targetTypeIndex;

    ASSERT(m_targetType != KnownTypeIndex::UnknownType);

    // Get setter info from the non-optimized style's setters.
    if (m_style->m_pSetters)
    {
        IFC_RETURN(OptimizeSetterCollection(m_style->m_pSetters, false));
    }

    m_basedOnBegin = m_properties.size();

    if (m_style->m_bIsSealed)
    {
        IFC_RETURN(Seal());
    }

    return S_OK;
}

// Initialize setter info from custom XBF data.
// Property indices are resolved eagerly, but values are deferred until the value is needed (e.g. via GetPropertyValue).
// Conditional setters (SetterHasTokenForSelf) are still evaluated eagerly since the conditional predicate determines
// whether the setter is included at all.
_Check_return_ HRESULT OptimizedStyle::Initialize(
    _In_ std::shared_ptr<StyleCustomRuntimeData> data,
    _In_ std::shared_ptr<CustomWriterRuntimeContext> context)
{
    ASSERT(m_style->m_optimizedStyle == nullptr);
    ASSERT(!m_style->m_bIsSealed);

    m_targetType = m_style->m_targetTypeIndex;
    // Pessimistically assume that every Setter is mutable and reserve() space accordingly
    // to avoid lots of potential reallocations
    m_mutableSetters.reserve(data->GetSetterCount());

    ASSERT(m_targetType != KnownTypeIndex::UnknownType);

    // Store runtime data and context for deferred value realization.
    m_runtimeData = std::move(data);
    m_runtimeContext = std::move(context);

    m_spObjectCreator = std::make_unique<CustomWriterRuntimeObjectCreator>(
        NameScopeRegistrationMode::RegisterEntries,
        m_runtimeContext.get(),
        CustomWriterRuntimeObjectCreator::ContextReference::Weak);

    auto setterCount = m_runtimeData->GetSetterCount();
    m_properties.reserve(setterCount);
    m_values.reserve(setterCount);
    m_dataIndices.reserve(setterCount);

    // Store this style's setter info.
    // We try to defer the Setter.Value property. In case an app applies multiple styles on the same element that all
    // try to set the Control.Template property, we don't want to fully load all templates only to use the final one.
    for (unsigned int i = 0; i < setterCount; i++)
    {
        KnownPropertyIndex prop = KnownPropertyIndex::UnknownType_UnknownProperty;
        CValue val;

        if (m_runtimeData->SetterHasTokenForSelf(i))
        {
            // This Setter.Value can't be deferred because it's needed for conditionals and mutable setters. Eagerly
            // create the full Setter object.

            if (m_runtimeData->IsTokenForIgnoredConditionalObject(m_runtimeData->GetSetterValueToken(i)))
            {
                // Nothing to do if any predicate associated with conditional setter evaluates to false
                continue;
            }
            else
            {
                xref_ptr<CThemeResource> unused;
                xref_ptr<CDependencyObject> depObj;
                xref_ptr<CSetter> setter;
                IFC_RETURN(m_spObjectCreator->CreateInstance(m_runtimeData->GetSetterValueToken(i), &depObj, &unused));

                // Extract information
                setter.attach(do_pointer_cast<CSetter>(depObj.detach()));
                IFC_RETURN(setter->GetProperty(m_targetType, &prop));
                IFC_RETURN(setter->GetValueByIndex(KnownPropertyIndex::Setter_Value, &val));

                if (m_runtimeData->SetterIsMutable(i))
                {
                    setter->SetIsValueMutable(true);
                    m_mutableSetters.emplace_back(std::move(setter), m_properties.size());
                }

                IFC_RETURN(AddSetterInfo(prop, std::move(val)));
                m_realizedStates.push_back(true);
                m_dataIndices.push_back(UINT_MAX);
            }
        }
        else
        {
            // Resolve the property index but defer Setter.Value realization. This avoids costs of loading a full
            // Control.Template value if it's just going to be replaced by another style's Setter later.

            if (m_runtimeData->IsSetterPropertyResolved(i))
            {
                prop = m_runtimeData->GetSetterPropertyIndex(i);
            }
            else
            {
                const xstring_ptr& propertyName = m_runtimeData->GetSetterPropertyString(i);
                ASSERT(!propertyName.IsNullOrEmpty());

                std::shared_ptr<XamlProperty> xamlProperty;
                std::shared_ptr<XamlType> xamlType = m_runtimeData->GetSetterPropertyOwnerType(i);

                if (xamlType)
                {
                    IFC_RETURN(xamlType->GetDependencyProperty(propertyName, xamlProperty));
                }

                if (xamlProperty == nullptr)
                {
                    IFC_RETURN(E_FAIL);
                }

                prop = xamlProperty->get_PropertyToken().GetHandle();
            }

            ASSERT(prop != KnownPropertyIndex::UnknownType_UnknownProperty);

            // Store property index and data index; value will be realized on demand.
            AddDeferredSetterInfo(prop, i);
        }
    }

    m_basedOnBegin = m_properties.size();

    if (!IsPerfOptInEnabled())
    {
        IFC_RETURN(EnsureAllValuesRealized());
    }

    return S_OK;
}

OptimizedStyle::~OptimizedStyle()
{
    // TODO: Incorporate pegging behavior into xref_ptr so it will be handled
    // TODO: automatically in this class and others like CustomWriterRuntimeObjectCreator.

    // Unpeg any DO's that we pegged
    std::size_t index = 0;

    for (auto iterVal = m_values.begin(); iterVal < m_values.end(); ++iterVal)
    {
        // Skip unrealized deferred values — they have no DO to unpeg
        if (m_runtimeData && index < m_realizedStates.size() && !m_realizedStates[index])
        {
            ++index;
            continue;
        }

        // Unpeg peer if we pegged it when we stored the value
        CDependencyObject* depObj = iterVal->AsObject();

        if (depObj != nullptr && depObj->HasManagedPeer())
        {
            if (m_peggedStates[index])
            {
                VERIFYHR(m_style->RemovePeerReferenceToItem(depObj));
            }
        }

        ++index;
    }
}

// Get the info from the given setters, and add it to our optimized storage.
_Check_return_ HRESULT OptimizedStyle::OptimizeSetterCollection(_In_ CSetterBaseCollection* setters, bool isBasedOn)
{
    xref_ptr<CDependencyObject> depObj;
    KnownPropertyIndex propertyId;
    CSetter* setter = nullptr;

    ASSERT(setters != nullptr);

    auto setterCount = setters->GetCount();

    // Store each setter property and value.
    // We don't need to store the BasedOn setter values, because we just call to the
    // BasedOn style to get values when necessary.
    for (auto i = 0U; i < setterCount; i++)
    {
        depObj.attach(static_cast<CDependencyObject*>(setters->GetItemWithAddRef(i)));
        setter = do_pointer_cast<CSetter>(depObj.get());

        // Get property ID and value
        IFC_RETURN(setter->GetProperty(m_targetType, &propertyId));

        // Store the setter info
        if (isBasedOn)
        {
            AddBasedOnSetterInfo(propertyId);
        }
        else
        {
            IFC_RETURN(AddSetterInfo(propertyId, CValue(setter->m_vValue)));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT OptimizedStyle::Seal()
{
    CStyle* basedOnStyle = m_style->m_pBasedOn;

    if (m_isSealed)
        return S_OK;

    m_basedOnBegin = m_properties.size();

    // Add BasedOn style's setter info to storage.
    // Only store the affected properties since we'll just call to the BasedOn
    // style to get values.
    if (basedOnStyle)
    {
        ASSERT((basedOnStyle->m_optimizedStyle == nullptr) != (basedOnStyle->m_pSetters == nullptr));

        if (basedOnStyle->m_optimizedStyle)
        {
            // BasedOn style is optimized -- get property info from optimized property vector
            std::vector<KnownPropertyIndex>& basedOnProperties = basedOnStyle->m_optimizedStyle->m_properties;

            for (auto basedOn = basedOnProperties.begin(); basedOn != basedOnProperties.end(); ++basedOn)
            {
                AddBasedOnSetterInfo(*basedOn);
            }
        }
        else if (basedOnStyle->GetSetterCollection())
        {
            // BasedOn style is not optimized -- get property info from setter collection
            IFC_RETURN(OptimizeSetterCollection(basedOnStyle->GetSetterCollection(), true /* isBasedOn */));
        }
    }

    m_isSealed = true;

    return S_OK;
}

// Stores property, value, and related info in our vectors.
_Check_return_ HRESULT OptimizedStyle::AddSetterInfo(_In_ KnownPropertyIndex propertyId, _In_ CValue&& value)
{
    ASSERT(!value.IsUnset());
    ASSERT(m_basedOnBegin == 0);

    m_properties.push_back(propertyId);

    bool pegged = false;
    IFC_RETURN(AddPeerRefIfNecessary(value.AsObject(), &pegged));
    {
        // m_values are used in GC walk, so take GC lock
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

        m_values.push_back(std::move(value));
    }
    m_resolvedStates.push_back(false);
    m_peggedStates.push_back(pegged);

    return S_OK;
}

// Stores BasedOn property. We only store the properties from BasedOn styles.
// When we get the value later, we just call to the BasedOn style to get it.
void OptimizedStyle::AddBasedOnSetterInfo(_In_ KnownPropertyIndex propertyId)
{
    ASSERT(m_basedOnBegin <= m_properties.size());
    m_properties.push_back(propertyId);
}

// Stores property index for a deferred setter. The value will be realized
// on demand via EnsureValueRealized using the stored runtime data/context.
void OptimizedStyle::AddDeferredSetterInfo(_In_ KnownPropertyIndex propertyId, _In_ unsigned int dataIndex)
{
    ASSERT(m_basedOnBegin == 0);

    m_properties.push_back(propertyId);
    m_values.emplace_back();    // To be realized later. No AutoReentrantReferenceLock when accessing m_values because this function is only called during init
    m_resolvedStates.push_back(false);
    m_peggedStates.push_back(false);    // Compare with AddSetterInfo - we'll set this when realizing the Setter value.
    m_realizedStates.push_back(false);
    m_dataIndices.push_back(dataIndex);
}

// Realizes a single deferred setter value from the stored XBF data and context.
_Check_return_ HRESULT OptimizedStyle::EnsureValueRealized(_In_ size_t index)
{
    ASSERT(index < m_realizedStates.size());

    if (m_realizedStates[index])
    {
        return S_OK;
    }

    unsigned int i = m_dataIndices[index];
    ASSERT(i != UINT_MAX);

    KnownPropertyIndex prop = m_properties[index];
    CValue val;

    ASSERT(m_runtimeData != nullptr);
    if (m_runtimeData->SetterHasContainerValue(i))
    {
        val = std::move(m_runtimeData->GetSetterValueContainer(i));
    }
    else
    {
        xref_ptr<CDependencyObject> depObj;
        std::shared_ptr<XamlQualifiedObject> xqObj;
        bool resourceFound = false;

        if (m_runtimeData->SetterHasStaticResourceValue(i))
        {
            // Get the object, and pass the parent CStyle and property index down.  XamlDiagnostics
            // needs the original CStyle object and property index that this
            // setter would have set when this function eventually leads to
            // CStaticResourceExtension::LookupResourceNoRef
            IFC_RETURN(m_spObjectCreator->LookupStaticResourceValue(m_runtimeData->GetSetterValueToken(i), m_style, prop, &depObj));
            resourceFound = true;
        }
        else if (m_runtimeData->SetterHasThemeResourceValue(i))
        {
            // Create the object
            IFC_RETURN(m_spObjectCreator->CreateThemeResourceInstance(m_runtimeData->GetSetterValueToken(i), m_style, prop, &xqObj));
            resourceFound = true;
        }
        else if (m_runtimeData->SetterHasObjectValue(i))
        {
            // Create the object
            xref_ptr<CThemeResource> unused;
            IFC_RETURN(m_spObjectCreator->CreateInstance(m_runtimeData->GetSetterValueToken(i), &depObj, &unused));
        }

        if (depObj == nullptr && xqObj == nullptr && !resourceFound)
        {
            IFC_RETURN(E_FAIL);
        }

        // If the object is a custom resource, get its value
        if (depObj && depObj->GetTypeIndex() == KnownTypeIndex::CustomResource)
        {
            CustomResourceExtension* customResource = do_pointer_cast<CustomResourceExtension>(depObj.get());
            ASSERT(customResource != nullptr);
            IFC_RETURN(customResource->ProvideValueByPropertyIndex(KnownPropertyIndex::Setter_Value, val));
        }
        // Null
        else if (depObj && depObj->GetTypeIndex() == KnownTypeIndex::NullExtension)
        {
            val.SetNull();
        }
        // Binding
        else if (depObj && depObj->GetTypeIndex() == KnownTypeIndex::Binding)
        {
            // We support only limited, one-time bindings on style setters.
            // To get the resolved value, create a temp setter, set the binding on
            // its Value property, then get the resolved value from Setter.Value.
            CBinding* binding = do_pointer_cast<CBinding>(depObj.get());
            ASSERT(binding != nullptr);

            xref_ptr<CDependencyObject> tempSetter;
            CREATEPARAMETERS cp(m_style->GetContext());
            IFC_RETURN(CSetter::Create(tempSetter.ReleaseAndGetAddressOf(), &cp));

            IFC_RETURN(binding->SetBinding(tempSetter.get(), KnownPropertyIndex::Setter_Value));
            IFC_RETURN(tempSetter->GetValueByIndex(KnownPropertyIndex::Setter_Value, &val));
        }
        // ThemeResource
        else if (xqObj)
        {
            auto* pDO = xqObj->GetAndTransferDependencyObjectOwnership();
            if (pDO)
            {
                CThemeResourceExtension* pThemeResourceExtension = do_pointer_cast<CThemeResourceExtension>(pDO);
                xref_ptr<CThemeResource> pThemeResource = make_xref<CThemeResource>(pThemeResourceExtension);
                val.SetThemeResourceNoRef(pThemeResource.detach());
            }
            else
            {
                val = std::move(xqObj.get()->GetValue());
            }
        }
        // If the object is an external reference, get its value
        else if (depObj && depObj->GetTypeIndex() == KnownTypeIndex::ExternalObjectReference)
        {
            CManagedObjectReference* ref = do_pointer_cast<CManagedObjectReference>(depObj.get());
            ASSERT(ref != nullptr);
            if (!ref->m_nativeValue.IsNull())
            {
                IFC_RETURN(val.CopyConverted(ref->m_nativeValue));
            }
            else
            {
                val.SetObjectAddRef(depObj.get());
            }
        }
        else
        {
            if (depObj)
            {
                auto dp = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(prop);
                if (dp && dp->RequiresMultipleAssociationCheck() && !depObj->DoesAllowMultipleAssociation())
                {
                    std::shared_ptr<ParserErrorReporter> parserErrorReporter;
                    IFC_RETURN(m_runtimeContext->GetSchemaContext()->GetErrorService(parserErrorReporter));

                    auto lineInfo = m_spObjectCreator->GetLineInfoForToken(m_runtimeData->GetSetterValueToken(i));
                    IFC_RETURN(parserErrorReporter->SetError(AG_E_PARSER2_NONSHAREABLE_OBJECT_NOT_ALLOWED_ON_STYLE_SETTER, lineInfo.LineNumber(), lineInfo.LinePosition()));
                    IFC_RETURN(E_FAIL);
                }
                else
                {
                    val.SetObjectAddRef(depObj.get());
                }
            }
            else
            {
                val.SetNull();
            }
        }
    }

    // Store the realized value
    bool pegged = false;
    IFC_RETURN(AddPeerRefIfNecessary(val.AsObject(), &pegged));
    {
        // m_values are used in GC walk, so take GC lock
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

        m_values[index] = std::move(val);
    }
    m_peggedStates.set(index, pegged);
    m_realizedStates.set(index, true);

    return S_OK;
}

// Realizes all deferred setter values.
_Check_return_ HRESULT OptimizedStyle::EnsureAllValuesRealized()
{
    if (!m_runtimeData)
    {
        return S_OK;
    }

    for (size_t i = 0; i < m_basedOnBegin; i++)
    {
        IFC_RETURN(EnsureValueRealized(i));
    }

    m_spObjectCreator.reset();
    m_runtimeData.reset();
    m_runtimeContext.reset();
    return S_OK;
}

// Returns true if the style has a setter for the given property index.
bool OptimizedStyle::HasPropertySetter(_In_ const KnownPropertyIndex propertyId) const
{
    return std::find(m_properties.begin(), m_properties.end(), propertyId) != m_properties.end();
}

// Get the value of a property in the style. Return null CValue if property is not found.
_Check_return_ HRESULT OptimizedStyle::GetPropertyValue(_In_ KnownPropertyIndex propertyId, _In_ bool getFromBasedOn, _Outptr_opt_ CValue** ppValue)
{
    *ppValue = nullptr;

    // Find the property/value.
    // If there are multiple setters for the same property, the last one wins, so search backwards.

    // First look in the range of this style's owned setters, which are at the front
    // of the vector.
    auto revIter = std::find(m_properties.rend() - m_basedOnBegin, m_properties.rend(), propertyId);
    auto found = revIter != m_properties.rend();
    auto foundIndex = revIter.base() - m_properties.begin() - 1;

    // If the property wasn't in this style's setters, look in the BasedOn style's setters.
    if (!found && m_style->m_pBasedOn && getFromBasedOn)
    {
        bool gotValue = false;
        IFC_RETURN(m_style->m_pBasedOn->GetPropertyValue(propertyId, &m_latestFoundBasedOnValue, &gotValue));

        // If we found the property/value in the BasedOn style, just return it immediately,
        // because the found value is already resolved.
        if (gotValue)
        {
            *ppValue = &m_latestFoundBasedOnValue;
            return S_OK;
        }
    }

    // Not found in this style or the BasedOn style -- just return null
    if (!found)
        return S_OK;

    // Found the value in this style's setters
    ASSERT(m_properties[foundIndex] == propertyId);

    // Realize the value if it was deferred
    if (m_runtimeData && !m_realizedStates[foundIndex])
    {
        IFC_RETURN(EnsureValueRealized(foundIndex));
    }

    auto& foundVal = m_values[foundIndex];

    // Resolve the value if we found the value but it isn't resolved yet (it's still in string form).
    if (!m_resolvedStates[foundIndex])
    {
        bool resolved = false;
        IFC_RETURN(ResolveValue(m_style->GetContext(), DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyId), foundVal, &resolved));
        if (resolved)
        {
            bool result = false;
            IFC_RETURN(AddPeerRefIfNecessary(foundVal.AsObject(), &result));
            m_peggedStates.set(foundIndex, result);
        }
        m_resolvedStates.set(foundIndex, true);
    }

    *ppValue = &foundVal;
    return S_OK;
}

// Populates a setter collection from the optimized setter info.
_Check_return_ HRESULT OptimizedStyle::FaultInOwnedSetters(_In_ const xref_ptr<CSetterBaseCollection>& setters)
{
    // Realize all deferred values before creating setter objects
    IFC_RETURN(EnsureAllValuesRealized());

    xref_ptr<CDependencyObject> depObj;
    xref_ptr<CSetter> setter;
    xref_ptr<CDependencyPropertyProxy> proxy;
    CREATEPARAMETERS cp(m_style->GetContext());

    CValue propertyVal;
    CValue valueVal;
    std::size_t mutableSetterIndex = 0;

    for (auto i = 0U; i < m_basedOnBegin; i++)
    {
        const KnownPropertyIndex propIndex = m_properties[i];

        if (mutableSetterIndex < m_mutableSetters.size() && i == m_mutableSetters[mutableSetterIndex].Index)
        {
            // If there is an existing mutable Setter, preserve and use it
            setter = m_mutableSetters[mutableSetterIndex].MutableSetter;
            ++mutableSetterIndex;
        }
        else
        {
            // Otherwise, create a new Setter object
            // Create property proxy
            proxy = make_xref<CDependencyPropertyProxy>(m_style->GetContext());
            proxy->SetPropertyIndex(propIndex);

            propertyVal.SetObjectAddRef(proxy.get());

            // Get copy of value
            IFC_RETURN(valueVal.CopyConverted(m_values[i]));

            // Create setter
            IFC_RETURN(CSetter::Create(depObj.ReleaseAndGetAddressOf(), &cp));
            setter = static_cast<CSetter*>(depObj.get());

            IFC_RETURN(setters->AddPeerReferenceToItem(setter));

            IFC_RETURN(setter->SetValueByIndex(KnownPropertyIndex::Setter_Property, propertyVal));
            IFC_RETURN(setter->SetValueByIndex(KnownPropertyIndex::Setter_Value, valueVal));
        }

        // Add setter
        IFC_RETURN(setters->Append(setter));

        IFC_RETURN(setters->RemovePeerReferenceToItem(setter));
    }

    return S_OK;
}

// Resolve the value if necessary (it's still in string form).
// Return true if we create a new object.
_Check_return_ HRESULT OptimizedStyle::ResolveValue(
    _In_ CCoreServices* core,
    _In_ const CDependencyProperty* depProp,
    _Inout_ CValue& val,
    _Out_ bool* createdObject)
{
    const CClassInfo* pPropertyType = nullptr;
    *createdObject = false;

    if (depProp->IsSparse())
    {
        pPropertyType = depProp->GetPropertyType();
        if (!pPropertyType->IsBuiltinType() || pPropertyType->GetIndex() == KnownTypeIndex::String)
        {
            // Managed code should've resolved using its type converters. Assume we're done.
            pPropertyType = nullptr;
        }
    }
    else
    {
        // If the target type is not a string/DO, then we need to do a conversion.
        if (depProp->GetStorageType() != valueString && depProp->GetPropertyType()->m_nIndex != KnownTypeIndex::DependencyObject)
        {
            pPropertyType = depProp->GetPropertyType();
        }
    }

    if (pPropertyType != nullptr)
    {
        // Convert the value if its type doesn't match the property storage type.
        if (val.GetType() != depProp->GetStorageType()
            && val.GetType() != valueThemeResource
            && pPropertyType->GetIndex() != KnownTypeIndex::Object)
        {
            if (pPropertyType->HasTypeConverter())
            {
                ValueBuffer buffer(core);
                IFC_RETURN(buffer.RepackageValueAndCopy(depProp, const_cast<CValue*>(&val), val));
                *createdObject = val.AsObject() != nullptr;
            }
            else
            {
                // We're trying to assign a value to a different property type that doesn't have a type converter.
                IFC_RETURN(E_FAIL);
            }
        }
        else
        {
            // Validate the value matches the property type. We only do this for sparse properties
            // because that's all we only did in Windows 8.1. Consider expanding this to all
            // properties moving forward.
            if (depProp->IsSparse() && !depProp->IsAssignable(val))
            {
                IFC_RETURN(E_FAIL);
            }
        }
    }

    return S_OK;
}

// Forward the notification to the stored DependencyObject values.
_Check_return_ HRESULT OptimizedStyle::NotifyThemeChanged(_In_ Theming::Theme theme, _In_ bool forceRefresh) const
{
    CDependencyObject* depObj = nullptr;
    std::size_t index = 0;

    for (auto& val : m_values)
    {
        // Skip unrealized deferred values — they haven't been applied to any
        // elements yet, so there's nothing to notify about a theme change.
        if (m_runtimeData && index < m_realizedStates.size() && !m_realizedStates[index])
        {
            ++index;
            continue;
        }

        depObj = val.AsObject();

        if (depObj != nullptr)
        {
            IFC_RETURN(depObj->NotifyThemeChanged(theme, forceRefresh));
        }

        ++index;
    }

    return S_OK;
}

bool OptimizedStyle::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    CDependencyObject* pDO = nullptr;
    std::size_t index = 0;

    for (auto& val : m_values)
    {
        // Skip unrealized deferred values — they hold no DO references
        if (m_runtimeData && index < m_realizedStates.size() && !m_realizedStates[index])
        {
            ++index;
            continue;
        }

       pDO = val.AsObject();

       if (pDO != nullptr)
       {
           pDO->ReferenceTrackerWalk(
                walkType,
                false,  //isRoot
                true);  //shouldWalkPeer
       }

       ++index;
    }

    return true;
}

// Called for each setter applied during a style change.  We add a peer ref
// to the value if it's a DO with a peer and we haven't pegged it yet.
_Check_return_ HRESULT OptimizedStyle::NotifySetterApplied(_In_ unsigned int setterIndex)
{
    ASSERT(setterIndex < m_properties.size());

    if (setterIndex < m_basedOnBegin)
    {
        // The index is within the range of this style's owned setters.

        // Realize the value if it was deferred
        if (m_runtimeData && !m_realizedStates[setterIndex])
        {
            IFC_RETURN(EnsureValueRealized(setterIndex));
        }

        if (!m_peggedStates[setterIndex])
        {
            bool result = false;
            IFC_RETURN(AddPeerRefIfNecessary(m_values[setterIndex].AsObject(), &result));
            m_peggedStates.set(setterIndex, result);
        }
    }
    else
    {
        // The index is within the range of the BasedOn style's setters.

        ASSERT(m_style->m_pBasedOn != nullptr);

        // Adjust index to the BasedOn setters' range,
        // i.e. subtract the count of this style's setters.
        setterIndex -= static_cast<unsigned int>(m_basedOnBegin);

        IFC_RETURN(m_style->m_pBasedOn->NotifySetterApplied(setterIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT OptimizedStyle::AddPeerRefIfNecessary(_In_opt_ CDependencyObject* depObj, _Out_ bool* pResult) const
{
    *pResult = false;
    if (depObj != nullptr && depObj->HasManagedPeer())
    {
        IFC_RETURN(m_style->AddPeerReferenceToItem(depObj));
        *pResult = true;
    }

    return S_OK;
}

_Check_return_ HRESULT OptimizedStyle::NotifyMutableSetterValueChanged(_In_ CSetter* const sender)
{
    std::size_t index = 0;
    // Find the index of the changed Setter's property, so we can update m_values appropriately
    auto result = std::find_if(m_mutableSetters.begin(), m_mutableSetters.end(), [&sender](const auto& setter) { return setter.MutableSetter == sender; });
    if (result != m_mutableSetters.end())
    {
        index = result->Index;
    }

    // Unpeg old value's peer if we pegged it when we stored the value
    CDependencyObject* oldDepObj = m_values[index].AsObject();
    if (oldDepObj != nullptr && oldDepObj->HasManagedPeer())
    {
        if (m_peggedStates[index])
        {
            IFC_RETURN(m_style->RemovePeerReferenceToItem(oldDepObj));
            m_peggedStates.set(index, false);
        }
    }

    // Add the new value, and peg if necessary
    CValue newValue;
    bool pegged = false;
    IFC_RETURN(sender->GetSetterValue(&newValue));
    IFC_RETURN(AddPeerRefIfNecessary(newValue.AsObject(), &pegged));
    {
        // m_values are used in GC walk, so take GC lock
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

        m_values[index] = std::move(newValue);
    }
    m_resolvedStates.set(index, false);
    m_peggedStates.set(index, pegged);

    return S_OK;
}

void OptimizedStyle::SubscribeToMutableSetters() const
{
    for (auto& setterIndexPair : m_mutableSetters)
    {
        auto& setter = setterIndexPair.MutableSetter;
        setter->SubscribeToValueChangedNotification(m_style);
    }
}

void OptimizedStyle::UnsubscribeFromMutableSetters() const
{
    for (auto& setterIndexPair : m_mutableSetters)
    {
        auto& setter = setterIndexPair.MutableSetter;
        setter->UnsubscribeFromValueChangedNotification(m_style);
    }
}