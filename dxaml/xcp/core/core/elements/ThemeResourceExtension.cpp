// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DOPointerCast.h"
#include "FrameworkTheming.h"
#include "ThemeResource.h"
#include "XamlQualifiedObject.h"
#include "XamlSchemaContext.h"
#include "XamlServiceProviderContext.h"
#include "theming\inc\Theme.h"
#include "resources\inc\ResourceResolver.h"
#include "resources\inc\ResourceLookupLogger.h"
#include <UriXStringGetters.h>

CThemeResourceExtension::CThemeResourceExtension(_In_ CCoreServices *pCore)
    : CMarkupExtensionBase(pCore)
    , m_strResourceKey()
    , m_isValueFromInitialTheme(FALSE)
    , m_themeWalkResourceCache(pCore->GetThemeWalkResourceCache())
{
}

void CThemeResourceExtension::Reset()
{
    m_lastResolvedThemeValue.SetNull();
    m_isValueFromInitialTheme = FALSE;
    m_strResourceKey.Reset();
}

//------------------------------------------------------------------------
//
//  Method:   ProvideValue
//
//  Synopsis:
//      Provide ThemeResourceExtension itself as value to parser, after
//  resolving the initial value and target dictionary. The property system
//  will use the ThemeResourceExtension's initial value, and will convert to
//  a ThemeResourceExpression if theme changes.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CThemeResourceExtension::ProvideValue(
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue)
{
    HRESULT hr = S_OK;
    CDependencyObject* pValue = NULL;
    CResourceDictionary* pTargetDictionary = NULL;
    CThemeResourceExtension *pThemeResourceExtension = NULL;
    auto core = GetContext();

    // Lookup theme resource value and target dictionary that should be searched when
    // theme is changed
    IFC(ResolveInitialValueAndTargetDictionary(
        m_strResourceKey,
        spServiceProviderContext,
        core,
        TRUE, //bShouldCheckThemeResources
        &pValue,
        &pTargetDictionary));

    if (pValue)
    {
        // If the value is already a ThemeResourceExtension (created during pre-resolve of
        // resources), return that. Else return this ThemeResourceExtension.
        if (pValue->OfTypeByIndex<KnownTypeIndex::ThemeResource>())
        {
            pThemeResourceExtension = static_cast<CThemeResourceExtension*>(pValue);
            pValue = NULL;
        }
        else
        {
            // Set inital value and target dictionary
            IFC(SetInitialValueAndTargetDictionary(pValue, pTargetDictionary));

            pThemeResourceExtension = this;
            AddRefInterface(pThemeResourceExtension);
        }

        // Provide ThemeResourceExtension as value to the parser.
        auto qo = std::make_shared<XamlQualifiedObject>(XamlTypeToken(tpkNative, KnownTypeIndex::ThemeResource));
        IFC(qo->SetDependencyObject(pThemeResourceExtension));
        qoValue = std::move(qo);
    }
    else
    {
        // Rerun the search with logging enabled
        xstring_ptr traceMessage;
        {
            Diagnostics::ResourceLookupLogger* loggerNoRef = core->GetResourceLookupLogger();
            auto cleanupGuard = wil::scope_exit([&]
            {
                TRACE_HR(loggerNoRef->Stop(m_strResourceKey, traceMessage));
            });

            xstring_ptr currentUri;
            if (const auto baseUri = spServiceProviderContext->GetBaseUri())
            {
                IGNOREHR(UriXStringGetters::GetPath(baseUri, &currentUri));
            }

            IFC(loggerNoRef->Start(m_strResourceKey, currentUri));
            IFC(ResolveInitialValueAndTargetDictionary(
                m_strResourceKey,
                spServiceProviderContext,
                core,
                TRUE, //bShouldCheckThemeResources
                &pValue,
                &pTargetDictionary));
        }

        // Record the message in an error context
        std::vector<std::wstring> extraInfo;
        extraInfo.push_back(std::wstring(traceMessage.GetBuffer()));

        // Not found. Throw error.
        IFC_EXTRA_INFO(CErrorService::OriginateInvalidOperationError(
                core,
                AG_E_PARSER_FAILED_RESOURCE_FIND,
                m_strResourceKey),
            &extraInfo);
    }

Cleanup:
    ReleaseInterface(pValue);
    ReleaseInterface(pTargetDictionary);
    ReleaseInterface(pThemeResourceExtension);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   LookupResource
//
//  Synopsis:
//      Find resource. Called to pre-resolve a resource
//
//------------------------------------------------------------------------

// static
_Check_return_ HRESULT
CThemeResourceExtension::LookupResource(
    _In_ const xstring_ptr& strResourceKey,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ CCoreServices *pCore,
    _In_ bool bShouldCheckThemeResources,
    _Outptr_result_maybenull_ CDependencyObject** ppValue,
    _In_opt_ CStyle* optimizedStyleParent,
    _In_ KnownPropertyIndex stylePropertyIndex)
{
    HRESULT hr = S_OK;
    CDependencyObject* pValue = NULL;
    CResourceDictionary* pTargetDictionary = NULL;
    CThemeResourceExtension *pThemeResourceExtension = NULL;

    *ppValue = NULL;

    // Lookup theme resource value and target dictionary that should be searched when
    // theme is changed
    IFC(ResolveInitialValueAndTargetDictionary(
        strResourceKey,
        spServiceProviderContext,
        pCore,
        bShouldCheckThemeResources,
        &pValue,
        &pTargetDictionary,
        optimizedStyleParent,
        stylePropertyIndex));

    // If value was found, return a corresponding ThemeResourceExtension
    if (pValue)
    {
        // If the value is already a ThemeResourceExtension (created during pre-resolve of
        // resources), return that. Else create and return a new ThemesourceExtension.
        if (pValue->OfTypeByIndex<KnownTypeIndex::ThemeResource>())
        {
            pThemeResourceExtension = static_cast<CThemeResourceExtension*>(pValue);
            pValue = NULL;
        }
        else
        {
            // Create ThemeResourceExtension
            CREATEPARAMETERS cp(pCore);
            IFC(CThemeResourceExtension::Create(
                reinterpret_cast<CDependencyObject **>(&pThemeResourceExtension),
                &cp));

            // Set key
            pThemeResourceExtension->m_strResourceKey = strResourceKey;

            // Set inital value and target dictionary
            IFC(pThemeResourceExtension->SetInitialValueAndTargetDictionary(
                pValue,
                pTargetDictionary));
        }

        *ppValue = pThemeResourceExtension;
        pThemeResourceExtension = NULL;
    }

Cleanup:
    ReleaseInterface(pValue);
    ReleaseInterface(pTargetDictionary);
    ReleaseInterface(pThemeResourceExtension);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ResolveInitialValueAndTargetDictionary
//
//  Synopsis:
//      Find inital resource value and target dictionary in which to
//  search for values when theme changes.
//
//------------------------------------------------------------------------

// static
_Check_return_ HRESULT
CThemeResourceExtension::ResolveInitialValueAndTargetDictionary(
    _In_ const xstring_ptr& strResourceKey,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ CCoreServices *pCore,
    _In_ bool bShouldCheckThemeResources,
    _Outptr_result_maybenull_ CDependencyObject** ppObject,
    _Outptr_result_maybenull_ CResourceDictionary** ppTargetDictionary,
    _In_opt_ CStyle* optimizedStyleParent,
    _In_ KnownPropertyIndex stylePropertyIndex)
{
    Resources::ResolvedResource resolved;
    IFC_RETURN(Resources::ResourceResolver::ResolveThemeResource(
        strResourceKey,
        spServiceProviderContext,
        pCore,
        !!bShouldCheckThemeResources,
        resolved,
        optimizedStyleParent,
        stylePropertyIndex));

    *ppObject = resolved.Value.detach();
    *ppTargetDictionary = resolved.DictionaryForThemeReference.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetValueAndTargetDictionary
//
//  Synopsis:
//      Set resolved initial value and target dictionary
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CThemeResourceExtension::SetInitialValueAndTargetDictionary(
    _In_ CDependencyObject* pValue,
    _In_ CResourceDictionary* targetDictionary)
{
    // Store weak reference to dictionary in which to look up value when theme changes.
    if (targetDictionary)
    {
        m_pTargetDictionaryWeakRef = xref::get_weakref(targetDictionary);

        // Store current value
        IFC_RETURN(SetLastResolvedValue(pValue));

        // Remember if value is from the Application's initial theme.
        IFC_RETURN(SetIsValueFromInitialTheme(targetDictionary));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetValue
//
//  Synopsis:
//      Resolve the theme resource, cache it locally and return the
//  resolved value
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CThemeResourceExtension::GetValue(_Out_ CValue* pValue)
{
    CResourceDictionary* pTargetDictionaryNoRef = m_pTargetDictionaryWeakRef.lock();

    // Get value from target dictionary.
    // If target dictionary has been released, which can occur during teardown, return
    // last resolved value.
    if (pTargetDictionaryNoRef)
    {
        CDependencyObject *pValueDO = nullptr;

        // Get value
        // GetKeyNoRef doesn't add-ref.
        IFC_RETURN(pTargetDictionaryNoRef->GetKeyNoRef(
            m_strResourceKey,
            &pValueDO));

        // Cache value
        IFC_RETURN(SetLastResolvedValue(pValueDO));

        // Remember if value is from the Application's initial theme.
        IFC_RETURN(SetIsValueFromInitialTheme(pTargetDictionaryNoRef));
   }

   IFC_RETURN(pValue->CopyConverted(m_lastResolvedThemeValue));

   return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetLastResolvedThemeValue
//
//  Synopsis:
//      Get the value to which ThemeResource was last resolved to.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CThemeResourceExtension::GetLastResolvedThemeValue(_Out_ CValue* pValue)
{
    RRETURN(pValue->CopyConverted(m_lastResolvedThemeValue));
}

//------------------------------------------------------------------------
//
//  Method:   SetLastResolvedValue
//
//  Synopsis:
//      Cache the value to which ThemeResource was last resolved to.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CThemeResourceExtension::SetLastResolvedValue(_In_ CDependencyObject* pValueDO)
{
    // Unwrap CDependencyObjectWrapper. The purpose of CDependencyObjectWrapper is to wrap a DO to work
    // around parenting issues when inserting into ResourceDictionary
    if (pValueDO->OfTypeByIndex<KnownTypeIndex::DependencyObjectWrapper>())
    {
        CDependencyObjectWrapper *pdoWrapper = static_cast<CDependencyObjectWrapper*>(pValueDO);
        pValueDO = pdoWrapper->WrappedDO();
        IFCPTR_RETURN(pValueDO);
    }

    m_lastResolvedThemeValue.ReleaseAndReset();

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
CThemeResourceExtension::SetThemeResourceBinding(
    _In_ CDependencyObject* pDependencyObject,
    _In_ const CDependencyProperty* pDP,
    _In_opt_ CModifiedValue* pModifiedValue,
    _In_ BaseValueSource baseValueSource)
{
    CValue valMarkupExtension;
    xref_ptr<CThemeResource> pThemeResource = make_xref<CThemeResource>(this);

    IFCEXPECT_RETURN(!m_strResourceKey.IsNull());

    // Don't store live expressions for certain properties like Setter.Value on a Style Setter
    // (live expressions OK on a VSM Setter's Setter.Value; if the VSM Setter value is a ThemeResource
    // and the base value is a TemplateBinding, then storing the markup extension would result in the
    // TemplateBinding being unexpectedly detached during a VSM state change).
    // We'll create live expressions when we apply the setter to a control.
    // Instead, store the markup extension itself.
    CSetter *pSetter = do_pointer_cast<CSetter>(pDependencyObject);
    if (   pDP->PreserveThemeResourceExtension()
        && (!pSetter || pSetter->IsStyleSetter()))
    {
        valMarkupExtension.WrapThemeResourceNoRef(pThemeResource.get());
        IFC_RETURN(pDependencyObject->SetValue(SetValueParams(pDP, valMarkupExtension, baseValueSource)));
    }
    else
    {
        IFC_RETURN(pDependencyObject->SetThemeResourceBinding(pDP, pModifiedValue, pThemeResource, baseValueSource));
    }

    return S_OK;
}

_Check_return_ HRESULT
CThemeResourceExtension::NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh)
{
    CValue value;

    IFC_RETURN(CMarkupExtensionBase::NotifyThemeChangedCore(theme, fForceRefresh));

    // Refresh the value.
    IFC_RETURN(GetValue(&value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetIsValueFromInitialTheme
//
//  Synopsis:
//      Remember if value is from the Application's initial theme. This is
//  needed for perf optimization to delay creation of ThemeResource binding
//  expressions. Even if the app theme has never changed, mixed themes in
//  the app can cause the target resource dictionary to use a different theme
//  than the app theme.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CThemeResourceExtension::SetIsValueFromInitialTheme(
    _In_ CResourceDictionary* pTargetDictionary)
{
    m_isValueFromInitialTheme = false;

    auto core = GetContext();
    auto resourceDictionaryTheme = pTargetDictionary->GetActiveTheme();
    auto appTheme = core->GetFrameworkTheming()->GetTheme();

    // If app's theme has never changed and app's theme is the same as the
    // the theme of the resource dictionary's ThemeDictionaries, the value is
    // from the initial theme.
    // If the resource dictionary does not have ThemeDictionaries (ThemeNone),
    // the value will not change when the theme changes, so can be treated as a
    // value from the the initial theme.
    if ((!core->HasThemeEverChanged() && (appTheme == resourceDictionaryTheme))
            || (resourceDictionaryTheme == Theming::Theme::None))
    {
        m_isValueFromInitialTheme = true;
    }

    RRETURN(S_OK);
}

KnownTypeIndex CThemeResourceExtension::GetTypeIndex() const
{
    return DependencyObjectTraits<CThemeResourceExtension>::Index;
}

