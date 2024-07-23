// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DOPointerCast.h>
#include <FrameworkTheming.h>
#include <Setter.h>
#include <ThemeResource.h>
#include <ThemeResourceExtension.h>
#include <ThemeWalkResourceCache.h>
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"
#include "Resources.h"
#include "NoParentShareableDependencyObject.h"
#include "DependencyObjectWrapper.h"
#include "ManagedObjectReference.h"
#include "Corep.h"
#include "NullKeyedResource.h"
#include "resources\inc\ResourceLookupLogger.h"

CThemeResource::CThemeResource(_In_ CThemeResourceExtension* pThemeResourceExtension)
    : m_cRef(1)
    , m_strResourceKey(pThemeResourceExtension->m_strResourceKey)
    , m_isValueFromInitialTheme(pThemeResourceExtension->IsValueFromInitialTheme())
    , m_pTargetDictionaryWeakRef(pThemeResourceExtension->GetTargetDictionaryWeekRef())
    , m_themeWalkResourceCache(pThemeResourceExtension->m_themeWalkResourceCache)
{
    VERIFYHR(pThemeResourceExtension->GetLastResolvedThemeValue(&m_lastResolvedThemeValue));
}

CThemeResource::CThemeResource(_In_opt_ ThemeWalkResourceCache* resourceCache)
    : m_cRef(1)
    , m_themeWalkResourceCache(resourceCache)
    , m_isValueFromInitialTheme(true)
{
}

_Check_return_ HRESULT
CThemeResource::SetInitialValueAndTargetDictionary(
    _In_ CDependencyObject* pValue,
    _In_ CResourceDictionary* targetDictionary)
{
    // Store weak reference to dictionary in which to look up value when theme changes.
    m_pTargetDictionaryWeakRef = xref::get_weakref(targetDictionary);

    // Store current value
    IFC_RETURN(SetLastResolvedValue(pValue));

    return S_OK;
}

_Check_return_ HRESULT
CThemeResource::GetValue(_Out_ CValue* pValue)
{
   IFC_RETURN(RefreshValue());

   IFC_RETURN(pValue->CopyConverted(m_lastResolvedThemeValue));

   return S_OK;
}

_Check_return_ HRESULT
CThemeResource::RefreshValue()
{
    CDependencyObject *pValueDO = nullptr;
    CResourceDictionary* pTargetDictionaryNoRef = m_pTargetDictionaryWeakRef.lock();

    // Get value from target dictionary.
    // If target dictionary has been released, which can occur during teardown, return
    // last resolved value.
    if (pTargetDictionaryNoRef)
    {
        if (m_themeWalkResourceCache)
        {
            pValueDO = m_themeWalkResourceCache->TryGetCachedResource(pTargetDictionaryNoRef, m_strResourceKey);
        }

        if (pValueDO == nullptr)
        {
            // Get value
            // GetKeyNoRef doesn't add-ref.
            IFC_RETURN(pTargetDictionaryNoRef->GetKeyNoRef(
                m_strResourceKey,
                &pValueDO));

            // Cache this value so that we can skip the resource dictionary lookup
            // for other theme resources with the same key.
            if (m_themeWalkResourceCache)
            {
                m_themeWalkResourceCache->AddCachedResource(pTargetDictionaryNoRef, m_strResourceKey, pValueDO);
            }
        }

        if (pValueDO == nullptr)
        {
            // Rerun the search with logging enabled
            xstring_ptr traceMessage;
            {
                Diagnostics::ResourceLookupLogger* loggerNoRef = pTargetDictionaryNoRef->GetContext()->GetResourceLookupLogger();
                auto cleanupGuard = wil::scope_exit([&]
                {
                    TRACE_HR_NORETURN(loggerNoRef->Stop(m_strResourceKey, traceMessage));
                });

                IFC_RETURN(loggerNoRef->Start(m_strResourceKey, xstring_ptr{}));
                IFC_RETURN(pTargetDictionaryNoRef->GetKeyNoRef(
                    m_strResourceKey,
                    &pValueDO));
			}

            // Record the message in an error context
            std::vector<std::wstring> extraInfo;
            extraInfo.push_back(std::wstring(traceMessage.GetBuffer()));

            xephemeral_string_ptr parameters[1];
            m_strResourceKey.Demote(&parameters[0]);
            IFC_RETURN_EXTRA_INFO(pTargetDictionaryNoRef->SetAndOriginateError(
                    E_NER_INVALID_OPERATION,
                    RuntimeError,
                    AG_E_PARSER_FAILED_RESOURCE_FIND, 1, parameters),
                &extraInfo);
        }

        // Cache the value locally for this theme resource object.
        IFC_RETURN(SetLastResolvedValue(pValueDO));
   }
   return S_OK;
}

_Check_return_ HRESULT
CThemeResource::GetLastResolvedThemeValue(_Out_ CValue* pValue) const
{
    return pValue->CopyConverted(m_lastResolvedThemeValue);
}

_Check_return_ HRESULT
CThemeResource::SetLastResolvedValue(_In_ CDependencyObject* pValueDO)
{
    m_isValueFromInitialTheme = m_lastResolvedThemeValue.IsUnset();

    // Unwrap CDependencyObjectWrapper. The purpose of CDependencyObjectWrapper is to wrap a DO to work
    // around parenting issues when inserting into ResourceDictionary
    if (pValueDO->OfTypeByIndex<KnownTypeIndex::DependencyObjectWrapper>())
    {
        CDependencyObjectWrapper *pdoWrapper = static_cast<CDependencyObjectWrapper*>(pValueDO);
        pValueDO = pdoWrapper->WrappedDO();
        IFCPTR_RETURN(pValueDO);
    }

    // Unwrap MOR & store value
    CManagedObjectReference * pMOR = do_pointer_cast<CManagedObjectReference>(pValueDO);
    if ((pMOR != nullptr) && !pMOR->m_nativeValue.IsNull())
    {
        // MOR unwrapping is to handle ResourceDictionary::Insert's wrapping
        // using ExternalObjectReference (which is a peer of CManagedObjectReference).

        IFC_RETURN(m_lastResolvedThemeValue.CopyConverted(pMOR->m_nativeValue));
    }
    else if (do_pointer_cast<CNullKeyedResource>(pValueDO))
    {
        m_lastResolvedThemeValue.SetNull();
    }
    else
    {
        m_lastResolvedThemeValue.SetObjectAddRef(pValueDO);
    }
    return S_OK;
}

_Check_return_ HRESULT
CThemeResource::SetThemeResourceBinding(
    _In_ CDependencyObject* pDependencyObject,
    _In_ const CDependencyProperty* pDP,
    _In_opt_ CModifiedValue* pModifiedValue,
    _In_ BaseValueSource baseValueSource)
{
    CValue valMarkupExtension;
    xref_ptr<CThemeResource> pThemeResource(this);
    IFCEXPECT_RETURN(!m_strResourceKey.IsNull());

    // Don't store live expressions for certain properties like Setter.Value on a Style Setter
    // (live expressions OK on a VSM Setter's Setter.Value; if the VSM Setter value is a ThemeResource
    // and the base value is a TemplateBinding, then storing the markup extension would result in the
    // TemplateBinding being unexpectedly detached during a VSM state change).
    // We'll create live expressions when we apply the Style setter to a control.
    // Instead, store the markup extension itself.
    CSetter *pSetter = do_pointer_cast<CSetter>(pDependencyObject);
    if (   pDP->PreserveThemeResourceExtension()
        && (!pSetter || pSetter->IsStyleSetter()))
    {
        valMarkupExtension.SetThemeResourceAddRef(pThemeResource.get());
        IFC_RETURN(pDependencyObject->SetValue(SetValueParams(pDP, valMarkupExtension, baseValueSource)));
    }
    else
    {
        IFC_RETURN(pDependencyObject->SetThemeResourceBinding(pDP, pModifiedValue, this, baseValueSource));
    }
    return S_OK;
}

// static
_Check_return_ HRESULT
CThemeResource::LookupResource(
    _In_ const xstring_ptr& strResourceKey,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ CCoreServices *pCore,
    _In_ bool bShouldCheckThemeResources,
    _Outptr_result_maybenull_ CThemeResource** ppThemeResource,
    _In_opt_ CStyle* optimizedStyleParent,
    _In_ KnownPropertyIndex stylePropertyIndex
    )
{
    xref_ptr<CDependencyObject> pValue;
    xref_ptr<CResourceDictionary> pTargetDictionary;
    xref_ptr<CThemeResource> pThemeResource;

    *ppThemeResource = nullptr;

    // Lookup theme resource value and target dictionary that should be searched when
    // theme is changed
    IFC_RETURN(CThemeResourceExtension::ResolveInitialValueAndTargetDictionary(
        strResourceKey,
        spServiceProviderContext,
        pCore,
        bShouldCheckThemeResources,
        pValue.ReleaseAndGetAddressOf(),
        pTargetDictionary.ReleaseAndGetAddressOf(),
        optimizedStyleParent,
        stylePropertyIndex));

    // If value was found, return a corresponding ThemeResourceExtension
    if (pValue)
    {
        if(pValue.get()->OfTypeByIndex<KnownTypeIndex::ThemeResource>())
        {
            pThemeResource = make_xref<CThemeResource>(static_cast<CThemeResourceExtension*>(pValue.get()));
        }
        else
        {
            // Create ThemeResourceExtension
            pThemeResource = make_xref<CThemeResource>(pCore->GetThemeWalkResourceCache());

            // Set key
            pThemeResource->m_strResourceKey = strResourceKey;

            // Set inital value and target dictionary
            IFC_RETURN(pThemeResource->SetInitialValueAndTargetDictionary(
                pValue.get(),
                pTargetDictionary.get()));
        }
        *ppThemeResource = pThemeResource.detach();
    }

    return S_OK;
}
