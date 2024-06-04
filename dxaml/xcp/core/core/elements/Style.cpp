// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include <OptimizedStyle.h>
#include <StyleCustomRuntimeData.h>
#include <ICustomWriterRuntimeDataReceiver.h>
#include <CustomWriterRuntimeContext.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <DXamlServices.h>
#include "MUX-ETWEvents.h"
#include <DXamlServices.h>
#include "diagnosticsInterop\inc\ResourceGraph.h"
#include "unsealer.h"
using namespace RuntimeFeatureBehavior;

//------------------------------------------------------------------------
//
//  Method:   CStyle::Create
//
//  Synopsis:
//      Create CStyle instance
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CStyle::Create(
            _Outptr_ CDependencyObject **ppObject,
            _In_ CREATEPARAMETERS *pCreate
        )
{
    if (pCreate->m_value.GetType() == valueString) {
        IFC_RETURN(E_NOTIMPL);
    }
    *ppObject = new CStyle(pCreate->m_pCore);
    return S_OK;
}

CStyle::CStyle(_In_ CCoreServices *pCore)
    : CNoParentShareableDependencyObject(pCore)
{
    m_requiresReleaseOverride = true;
}

CStyle::~CStyle()
{
    UnsubscribeFromMutableSetters();
    ReleaseInterface(m_pSetters);
    ReleaseInterface(m_pBasedOn);
    ReleaseInterface(m_pBasedOnSetters);
}

const CClassInfo* CStyle::GetTargetType() const
{
    return DirectUI::MetadataAPI::GetClassInfoByIndex(m_targetTypeIndex);
}

// Seal the Style
_Check_return_ HRESULT CStyle::Seal()
{
    if(m_bIsSealed)
    {
        return S_OK;
    }

    const bool shouldStoreSourceInformation = DirectUI::DXamlServices::ShouldStoreSourceInformation();

    if (m_pBasedOn)
    {
        // check if the basedon style's target type is compatible with this style's target type.
        IFC_RETURN(ValidateBasedOnTargetType());

        // Will return an error if we find a loop of BasedOn references.
        // (A.BasedOn = B, B.BasedOn = C, C.BasedOn = A)
        m_cBasedOnCircularRefCount = 0;
        IFC_RETURN(CheckForBasedOnCircularReferences());

        // Seal the BasedOn chain of styles.
        IFC_RETURN(m_pBasedOn->Seal());

        //If XamlDiagnostics is enabled, register this Style as using its initial BasedOn value.
        //RegisterBasedOnStyleDependency will ensure it doesn't add duplicate entries to the map in the case
        //we are resealing after a property change.
        if (shouldStoreSourceInformation)
        {
            const auto resourceGraph = Diagnostics::GetResourceGraph();
            resourceGraph->RegisterBasedOnStyleDependency(this);
        }
    }

    if (m_pSetters)
    {
        m_pSetters->SetIsSealed();
    }

    // Force optimization if the EnableStyleOptimization feature is on.
    // Otherwise, optimization occurs only via XBF2 when the parser calls
    // this type's SetCustomWriterRuntimeData() override.
    auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    if(runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnableStyleOptimization))
    {
        IFC_RETURN(EnsureOptimizedStyle());
    }

    // Seal the optimized style if it exists, otherwise fall back
    // to seal the non-optimized setters.
    if (m_optimizedStyle)
    {
        IFC_RETURN(m_optimizedStyle->Seal());
    }
    else if (m_pBasedOn)
    {
        // Create merged setters collection.
        IFC_RETURN(CreateMergedBasedOnSetterCollection());
    }

    SubscribeToMutableSetters();

    m_bIsSealed = true;

    if (shouldStoreSourceInformation)
    {
        if (m_pSetters)
        {
            const auto resourceGraph = Diagnostics::GetResourceGraph();
            resourceGraph->RegisterStyleAndSetterCollection(this, m_pSetters);
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CStyle::RefreshSetterCollection()
{
    if (m_pBasedOn != nullptr)
    {
        Diagnostics::StyleUnsealer unseal(this);
        IFC_RETURN(CreateMergedBasedOnSetterCollection());
    }
    return S_OK;
}

// Creates a new setter collection containing this style's setters plus any
// BasedOn setters for properties that aren't in this style's setters.
_Check_return_ HRESULT CStyle::CreateMergedBasedOnSetterCollection()
{
    CREATEPARAMETERS cp(GetContext());
    std::vector<bool> selfSettersAdded;
    CSetterBaseCollection* pBasedOnSetters = nullptr;

    ASSERT(m_pBasedOn != nullptr);

    // Fault in based on setters if necessary
    if (m_pBasedOn->m_optimizedStyle)
    {
        IFC_RETURN(m_pBasedOn->FaultInOptimizedStyle());
    }

    // creating a collection of setters which will hold the union of
    // self-setters and setters from the BasedOn style
    // If XamlDiagnostics modifies a Style, this method may get called multiple times - don't
    // reallocate m_pBasedOnSetters if we already have a collection for it
    if (m_pBasedOnSetters == nullptr)
    {
        IFC_RETURN(CBasedOnSetterCollection::Create((CDependencyObject**)&m_pBasedOnSetters, &cp));
    }
    else
    {
        //If m_pBasedOnSetters already exists, this should be from XamlDiagnostics modifying
        //a Style this Style is based on - clear the existing setter union
        ASSERT(DirectUI::DXamlServices::ShouldStoreSourceInformation());
        m_pBasedOnSetters->clear();
    }

    if (m_pSetters && m_pSetters->GetCount() > 0)
    {
        selfSettersAdded.insert(selfSettersAdded.begin(), m_pSetters->GetCount(), false);
    }

    pBasedOnSetters = m_pBasedOn->GetSetterCollection();
    if (pBasedOnSetters)
    {
        // merge the basedon setters and self-setters, giving priority to self-setters
        // when both contain a setter for the same property.
        bool bFound;
        XUINT32 uIndex;
        KnownPropertyIndex nItrPropertyId = KnownPropertyIndex::UnknownType_UnknownProperty;

        for (XUINT32 nItrSetterIndex = 0; nItrSetterIndex < pBasedOnSetters->GetCount(); nItrSetterIndex++)
        {
            CSetter* pSetter = do_pointer_cast<CSetter>((*pBasedOnSetters)[nItrSetterIndex]);

            IFCPTR_RETURN(pSetter);

            IFC_RETURN(pSetter->GetProperty(m_targetTypeIndex, &nItrPropertyId));

            bFound = FALSE;
            if (m_pSetters)
            {
                IFC_RETURN(m_pSetters->GetSetterIndexByPropertyId(nItrPropertyId, m_targetTypeIndex, uIndex, bFound));
                if (bFound)
                {
                    // For the given basedon style setter, a setter for the same property is also there
                    // in this style. Setter from this style wins and is added to the union set. Also, set
                    // the byte in the lookup array to indicate that this setter has been added to the
                    // union set.
                    IFC_RETURN(m_pBasedOnSetters->Append((*m_pSetters)[uIndex]));
                    selfSettersAdded[uIndex] = true;
                }
            }

            if (!bFound)
            {
                // For the given basedon setter, no setter for the same property is present in
                // this style. Add the basedon setter to the union set.
                IFC_RETURN(m_pBasedOnSetters->Append(pSetter));
            }
        }
    }

    // Add the self-setters to the union set which have not been added yet.
    if (m_pSetters)
    {
        for (XUINT32 nItrSetterIndex = 0; nItrSetterIndex < m_pSetters->GetCount(); nItrSetterIndex++)
        {
            if (!selfSettersAdded[nItrSetterIndex])
            {
                IFC_RETURN(m_pBasedOnSetters->Append((*m_pSetters)[nItrSetterIndex]));
            }
        }
    }

    return S_OK;
}

// Returns true if the style has a setter for the given property index.
_Check_return_ HRESULT CStyle::HasPropertySetter(_In_ const KnownPropertyIndex propIndex, _Out_ bool* hasProperty)
{
    ASSERT(m_bIsSealed || DirectUI::DXamlServices::ShouldStoreSourceInformation());

    *hasProperty = false;

    if (m_optimizedStyle)
    {
        *hasProperty = m_optimizedStyle->HasPropertySetter(propIndex);
        return S_OK;
    }
    else
    {
        int cSetters = 0;
        CSetterBaseCollection* pSetterCollection = GetSetterCollection();
        KnownPropertyIndex index = KnownPropertyIndex::UnknownType_UnknownProperty;

        if (pSetterCollection == nullptr)
            return S_OK;

        cSetters = pSetterCollection->GetCount();

        for (int i = 0; i < cSetters; i++)
        {
            CSetter* pSetter = nullptr;
            IFC_RETURN(DoPointerCast(pSetter, (*pSetterCollection)[i]));

            IFC_RETURN(pSetter->GetProperty(m_targetTypeIndex, &index));

            if (index == propIndex)
            {
                *hasProperty = true;
                break;
            }
        }
    }

    return S_OK;
}

// Returns the count of the style's setters.
unsigned int CStyle::GetSetterCount()
{
    ASSERT(m_bIsSealed || DirectUI::DXamlServices::ShouldStoreSourceInformation());

    if (m_optimizedStyle)
    {
        return m_optimizedStyle->GetSetterCount();
    }
    else
    {
        CSetterBaseCollection* pSetterCollection = GetSetterCollection();

        if (pSetterCollection == nullptr)
        {
            return 0;
        }
        else
        {
            return pSetterCollection->GetCount();
        }
    }
}

// Returns the KnownPropertyIndex at the given setter index.
_Check_return_ HRESULT CStyle::GetPropertyAtSetterIndex(_In_ unsigned int setterIndex, _Out_ KnownPropertyIndex* propIndex)
{
    ASSERT(m_bIsSealed || DirectUI::DXamlServices::ShouldStoreSourceInformation());

    if (m_optimizedStyle)
    {
        *propIndex = m_optimizedStyle->GetPropertyAtSetterIndex(setterIndex);
        return S_OK;
    }
    else
    {
        CSetter* pSetter = nullptr;
        CSetterBaseCollection* pSetterCollection = GetSetterCollection();

        ASSERT(pSetterCollection);

        IFC_RETURN(DoPointerCast(pSetter, (*pSetterCollection)[setterIndex]));

        IFC_RETURN(pSetter->GetProperty(m_targetTypeIndex, propIndex));
    }

    return S_OK;
}

// Gets the full name of the TargetType.  This is used as an implicit
// key in the ResourceDictionary if no other key is provided.
_Check_return_ HRESULT CStyle::GetTargetTypeName(_Out_ xstring_ptr* pstrFullName)
{
    *pstrFullName = GetTargetType()->GetFullName();
    return S_OK;
}

_Check_return_ HRESULT CStyle::GetPropertyValue(
    _In_ KnownPropertyIndex uPropertyId,
    _Out_ CValue* pValue,
    _Out_ bool* gotValue)
{
    return GetPropertyValue(uPropertyId, true /*getFromBasedOn*/, pValue, gotValue);
}

// Get value of property in the style. Return null CValue if property is not found in the style.
// If getFromBasedOn is true, will try to get the property in a based on style
_Check_return_ HRESULT CStyle::GetPropertyValue(
    _In_ KnownPropertyIndex uPropertyId,
    _In_ bool getFromBasedOn,
    _Out_ CValue* pValue,
    _Out_ bool* gotValue)
{
    *gotValue = false;

    // We should always be getting from the basedOnSetters. The only reason you can not get the property from basedOnSetters
    // is for xaml diagnostics
    ASSERT(getFromBasedOn || DirectUI::DXamlServices::ShouldStoreSourceInformation());

    if (m_optimizedStyle)
    {
        CValue* foundValue = nullptr;

        // Get property value.
        // Catch any exception and return a failure code if necessary
        // -- InvalidateProperties will ignore the failure.
        IFC_RETURN(m_optimizedStyle->GetPropertyValue(uPropertyId, getFromBasedOn, &foundValue));
        if (foundValue == nullptr)
            return S_OK;

        *gotValue = true;
        return pValue->CopyConverted(*foundValue);
    }
    else
    {
        KnownPropertyIndex uCurrentPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
        int cSetters;

        CSetterBaseCollection* pSetterCollection = getFromBasedOn && m_pBasedOn ? m_pBasedOnSetters : m_pSetters;
        if (!pSetterCollection)
        {
            return S_OK;
        }

        cSetters = pSetterCollection->GetCount();

        // Search in reverse-order because if a property is duplicated, the last setter wins
        for (int i = (cSetters - 1); i >= 0; i--)
        {
            CSetter* pSetter = nullptr;
            IFC_RETURN(DoPointerCast(pSetter, (*pSetterCollection)[i]));

            IFC_RETURN(pSetter->GetProperty(m_targetTypeIndex, &uCurrentPropertyIndex));
            if (uCurrentPropertyIndex == uPropertyId)
            {
                IFC_RETURN(pSetter->GetSetterValue(pValue));
                if (!pValue->IsUnset())
                {
                    *gotValue = true;
                }
                break;
            }
        }
    }

    return S_OK;
}

// Returns the effective setter collection.
_Check_return_ CSetterBaseCollection* CStyle::GetSetterCollection() const
{
    // if there is a basedon style, then return the list of combined setters.
    // Else, return the self-setters.
    if (m_pBasedOn)
    {
        return m_pBasedOnSetters;
    }
    else
    {
        return m_pSetters;
    }
}

// Checks for circular references in the BasedOn hierarchy.
_Check_return_ HRESULT CStyle::CheckForBasedOnCircularReferences()
{
    HRESULT hr = S_OK;

    ++m_cBasedOnCircularRefCount;

    // if count is greater than 1, then we have seen this style before and hence, there is a cycle.
    IFC(m_cBasedOnCircularRefCount == 1 ? S_OK : AgError(AG_E_STYLE_BASEDON_CIRCULAR_REF));

    // if the BasedOn style is sealed, then we have already verified that
    // there it is not involved in a BasedOn circular reference
    if(m_pBasedOn)
    {
        IFC(m_pBasedOn->CheckForBasedOnCircularReferences());
    }

    --m_cBasedOnCircularRefCount;

Cleanup:
    // We need to throw an InvalidOperationException with a specific exception string on the managed side.
    // Set the error in errorservice and this will be retrieved by managed code while constructing the exception.
    if(hr == AgError(AG_E_STYLE_BASEDON_CIRCULAR_REF))
    {
        // we are already in an error state. ignore any further errors from the ErrorService and
        // propagate the original error.
        HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
        IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, hr));
        hr = hrToOriginate;
    }
    RRETURN(hr);
}


// Verifies that the targettype of the BasedOn style is the same as or a base type
// of the targettype of this style.
_Check_return_ HRESULT CStyle::ValidateBasedOnTargetType()
{
    HRESULT hr = S_OK;

    // We are checking for null TargetType at time of sealing only if
    // there is a BasedOn style. This is due to v2 compatibility issues as in v2
    // it was possible to Seal a style with null TargetType. This should be removed
    // if we introduce a quirks mode fix to check for null targettype in all cases.
    IFC((m_targetTypeIndex != KnownTypeIndex::UnknownType && m_pBasedOn->m_targetTypeIndex != KnownTypeIndex::UnknownType) ? S_OK : AgError(AG_E_STYLE_BASEDON_TARGETTYPE_NULL));

    if (!DirectUI::MetadataAPI::IsAssignableFrom(m_pBasedOn->GetTargetType(), GetTargetType()))
    {
        IFC(AgError(AG_E_STYLE_BASEDON_INVALID_TARGETTYPE));
    }

Cleanup:
    // We need to throw an InvalidOperationException with a specific exception string on the managed side.
    // Set the error in errorservice and this will be retrieved by managed code while constructing the exception.
    if(hr == AgError(AG_E_STYLE_BASEDON_INVALID_TARGETTYPE)
        || hr == AgError(AG_E_STYLE_BASEDON_TARGETTYPE_NULL))
    {
        // ignore the error code returned from ReportError as we should propagate
        // the original error code(hr).
        HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
        IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, hr));
        hr = hrToOriginate;
    }

    RRETURN(hr);
}


// SetValue override to return error on SetValue attempts after Style has been sealed.
_Check_return_ HRESULT CStyle::SetValue(_In_ const SetValueParams& args)
{
    HRESULT hr = S_OK;

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Style_BasedOn || args.m_pDP->GetIndex() == KnownPropertyIndex::Style_Setters)
    {
        IFC(m_bIsSealed ? AgError(AG_E_STYLE_CHANGE_AFTER_SEALED) : S_OK);
    }

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Style_BasedOn)
    {
        const CStyle* pStyle = nullptr;
        IFC(DoPointerCast(pStyle, args.m_value));
        IFC(pStyle == this ? AgError(AG_E_STYLE_BASEDON_SELF) : S_OK);
    }

    IFC(__super::SetValue(args));

Cleanup:
    if(hr == AgError(AG_E_STYLE_CHANGE_AFTER_SEALED))
    {
        // ignore the error code returned from ReportError as we should propagate
        // the original error code(hr).
        HRESULT hrToOriginate = E_NER_INVALID_OPERATION;
        IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, hr));
        hr = E_NER_INVALID_OPERATION;
    }
    if(hr == AgError(AG_E_STYLE_BASEDON_SELF))
    {
        HRESULT hrToOriginate = E_NER_ARGUMENT_EXCEPTION;
        IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, hr));
        hr = hrToOriginate;
    }
    return hr;
}

// Creates an optimized style and clears the Setters property. Optimization occurs
// only via XBF2 when the parser calls this type's SetCustomWriterRuntimeData() override,
// or within Seal() if the EnableStyleOptimization feature is on.
_Check_return_ HRESULT CStyle::EnsureOptimizedStyle()
{
    if (this->m_optimizedStyle)
        return S_OK;

    ASSERT(m_targetTypeIndex != KnownTypeIndex::UnknownType);

    // Create optimized style from non-optimized setters.
    IFC_RETURN(OptimizedStyle::Create(this, &m_optimizedStyle));

    if (m_pSetters)
    {
        IFC_RETURN(ClearValue(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Style_Setters)));
    }

    ASSERT(m_pSetters == nullptr);

    return S_OK;
}

// SetCustomWriterRuntimeData override to create optimized from style runtime data.
_Check_return_ HRESULT
CStyle::SetCustomWriterRuntimeData(std::shared_ptr<CustomWriterRuntimeData> data, std::unique_ptr<CustomWriterRuntimeContext> context)
{
    // Depending on how the ObjectWriter is configured these invarients might not always be true.
    // It's better to capture this condition on origination than allow it to blow up later.
    ASSERT(data && context && context->GetReader());

    ASSERT(m_optimizedStyle == nullptr);
    ASSERT(m_pSetters == nullptr);

    ASSERT(m_targetTypeIndex != KnownTypeIndex::UnknownType);

    // Create optimized style from runtime data.
    std::shared_ptr<StyleCustomRuntimeData> styleData = std::static_pointer_cast<StyleCustomRuntimeData>(data);
    IFC_RETURN(OptimizedStyle::Create(this, std::move(styleData), context, &m_optimizedStyle));

    // For styles, cache the runtime context. This is needed for IVisualTreeService3.ResolveResource which is used during
    // Edit & Continue for resolving resources at runtime. Currently, Style's (nor the OptimizedStyle) cache the runtime data
    // so we have to explicitly do this.
    if (DirectUI::DXamlServices::ShouldStoreSourceInformation())
    {
        const auto resourceGraph = Diagnostics::GetResourceGraph();
        resourceGraph->CacheRuntimeContext(this, std::move(context));
    }

    return S_OK;
}

// GetValue override to fault in the setter collection if the style is optimized.
_Check_return_ HRESULT CStyle::GetValue(_In_ const CDependencyProperty *pdp, _Inout_ CValue *pValue)
{
    if (m_optimizedStyle)
    {
        if (pdp->GetIndex() == KnownPropertyIndex::Style_Setters)
        {
            IFC_RETURN(FaultInOptimizedStyle());
        }

        IFC_RETURN(__super::GetValue(pdp, pValue));
    }
    else
    {
        if (   pdp->GetIndex() == KnownPropertyIndex::Style_Setters
            && CheckOnDemandProperty(KnownPropertyIndex::Style_Setters).IsNull())
        {
            // Temporarily ensure style is unsealed to allow changes.
            // This is because Style.Setters is a create-on-demand property
            // so if the Style is already sealed, but Style.Setters wasn't already set,
            // then the act of retrieving its value would be forbidden.
            const bool wasSealed = m_bIsSealed;
            m_bIsSealed = false;

            auto scopeGuard = wil::scope_exit([&]
            {
                // Reseal if necessary.
                if (wasSealed)
                {
                    IGNOREHR(Seal());
                }
            });

            IFC_RETURN(__super::GetValue(pdp, pValue));
        }
        else
        {
            IFC_RETURN(__super::GetValue(pdp, pValue));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CStyle::FaultInOptimizedStyle()
{
    ASSERT(m_pSetters == nullptr);
    ASSERT(m_optimizedStyle != nullptr);

    // Temporarily ensure style is unsealed to allow changes.
    bool wasSealed = m_bIsSealed;
    m_bIsSealed = false;

    TraceFaultInBehaviorBegin(L"Style");
    auto scopeGuard = wil::scope_exit([&]
    {
        // Reseal if necessary.
        if (wasSealed)
        {
            IGNOREHR(Seal());
        }

        TraceFaultInBehaviorEnd();
    });

    // Create setter collection.
    xref_ptr<CDependencyObject> depObj;
    xref_ptr<CSetterBaseCollection> setters;
    CREATEPARAMETERS cp(GetContext());
    IFC_RETURN(CSetterBaseCollection::Create(depObj.ReleaseAndGetAddressOf(), &cp));
    setters = static_cast<CSetterBaseCollection*>(depObj.get());

    // Set Style_Setters property.
    CValue settersVal;
    settersVal.SetObjectAddRef(setters.get());
    IFC_RETURN(SetValueByIndex(KnownPropertyIndex::Style_Setters, settersVal));
    ASSERT(m_pSetters != nullptr);

    // Populate setter collection.
    std::unique_ptr<OptimizedStyle> optimizedStyle = std::move(m_optimizedStyle);
    IFC_RETURN(optimizedStyle->FaultInOwnedSetters(setters));

    return S_OK;
}

// NotifyThemeChangedCore override to forward notification to optimized style.
_Check_return_ HRESULT CStyle::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    if (m_optimizedStyle)
    {
        IFC_RETURN(m_optimizedStyle->NotifyThemeChanged(theme, fForceRefresh));
        return S_OK;
    }
    else
    {
        return __super::NotifyThemeChangedCore(theme, fForceRefresh);
    }
}

// GC's ReferenceTrackerWalk. Walk children
// Called on GC's thread
bool CStyle::ReferenceTrackerWalkCore(
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _In_ bool shouldWalkPeer)
{
    bool walked = __super::ReferenceTrackerWalkCore(walkType, isRoot, shouldWalkPeer);

    if (walked)
    {
        if (m_optimizedStyle)
        {
            m_optimizedStyle->ReferenceTrackerWalkCore(
                walkType,
                false,  //isRoot
                true);  //shouldWalkPeer
        }
    }

    return walked;
}

_Check_return_ HRESULT CStyle::NotifySetterApplied(_In_ unsigned int setterIndex)
{
    if (m_optimizedStyle)
    {
        IFC_RETURN(m_optimizedStyle->NotifySetterApplied(setterIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT CStyle::NotifyMutableSetterValueChanged(_In_ CSetter* const sender)
{
    if (m_optimizedStyle)
    {
        IFC_RETURN(m_optimizedStyle->NotifyMutableSetterValueChanged(sender));
    }

    KnownPropertyIndex propertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
    IFC_RETURN(sender->GetProperty(m_targetTypeIndex, &propertyIndex));
    // Setters added through the xaml diagnostics API aren't always guaranteed to have Setter.Property set
    MICROSOFT_TELEMETRY_ASSERT_DISABLED(propertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty || (m_pSetters && m_pSetters->AllowsInvalidSetter()));
    if (propertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        GetContext()->NotifyMutableStyleValueChangedListeners(this, propertyIndex);
    }

    return S_OK;
}

bool CStyle::HasMutableSetters(bool considerBasedOnStyle) const
{
    if (m_optimizedStyle)
    {
        if (m_optimizedStyle->HasMutableSetters())
        {
            return true;
        }
    }
    else if (m_pSetters)
    {
        auto result = std::find_if(m_pSetters->begin(), m_pSetters->end(),
            [&](const auto& setter)
            {
                return do_pointer_cast<CSetter>(setter)->GetIsValueMutable();
            });
        if (result != m_pSetters->end())
        {
            return true;
        }
    }

    // If the Style itself doesn't have mutable setters, we may need to check its
    // BasedOn Style.
    if (considerBasedOnStyle && m_pBasedOn != nullptr)
    {
        return m_pBasedOn->HasMutableSetters(considerBasedOnStyle);
    }
    else
    {
        return false;
    }
}

void CStyle::SubscribeToMutableSetters()
{
    if (m_optimizedStyle)
    {
        m_optimizedStyle->SubscribeToMutableSetters();
    }
    else if (m_pSetters)
    {
        for (auto& setterBase : *m_pSetters)
        {
            auto setter = do_pointer_cast<CSetter>(setterBase);
            if (setter && (setter->GetIsValueMutable() || DirectUI::DXamlServices::ShouldStoreSourceInformation()))
            {
                setter->SubscribeToValueChangedNotification(this);
            }
        }
    }

    if (m_pBasedOn != nullptr)
    {
        m_pBasedOn->SubscribeToMutableSetters();
    }
}

void CStyle::UnsubscribeFromMutableSetters()
{
    if (m_optimizedStyle)
    {
        m_optimizedStyle->UnsubscribeFromMutableSetters();
    }
    else if (m_pSetters)
    {
        for (auto& setterBase : *m_pSetters)
        {
            auto setter = do_pointer_cast<CSetter>(setterBase);
            if (setter)
            {
                setter->UnsubscribeFromValueChangedNotification(this);
            }
        }
    }
}