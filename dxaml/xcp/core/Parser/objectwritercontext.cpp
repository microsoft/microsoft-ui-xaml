// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriterContext.h"

#include "XamlNamespace.h"

HRESULT ObjectWriterContext::Create(
            _In_ const std::shared_ptr<XamlSchemaContext>& inSchemaContext,
            _In_ std::shared_ptr<XamlQualifiedObject> spEventRoot,
            _In_ bool bExpandTemplates,
            _In_opt_ const std::shared_ptr<XamlQualifiedObject>& spParentXBindConnector,
            _Out_ std::shared_ptr<ObjectWriterContext>& rspObjectWriterContext)
{
    rspObjectWriterContext = std::make_shared<ObjectWriterContext>(
        inSchemaContext, 
        spEventRoot, 
        bExpandTemplates,
        false /* Indicates this is the top-level parse context. Used by ObjectWriter::ProcessError() */,
        spParentXBindConnector);
    rspObjectWriterContext->PushScope();

    return S_OK;
}

HRESULT ObjectWriterContext::Create(
    _In_ const std::shared_ptr<XamlSavedContext>& spXamlSavedContext,
    _In_ std::shared_ptr<XamlQualifiedObject> spEventRoot,
    _In_ bool bExpandTemplates,
    _In_opt_ const std::shared_ptr<XamlQualifiedObject>& spParentXBindConnector,
    _Out_ std::shared_ptr<ObjectWriterContext>& rspObjectWriterContext)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    IFC_RETURN(spXamlSavedContext->get_SchemaContext(spSchemaContext));

    std::shared_ptr<ObjectWriterContext> writerContext = std::make_shared<ObjectWriterContext>(
        spSchemaContext, 
        spEventRoot, 
        bExpandTemplates,
        true /* Indicates this is *not* the top-level parse context. Used by ObjectWriter::ProcessError() */,
        spParentXBindConnector);

    std::shared_ptr<ObjectWriterStack> spSavedStack;
    IFC_RETURN(spXamlSavedContext->get_Stack(spSavedStack));
    writerContext->m_ObjectWriterStack = spSavedStack->CopyStack();

    writerContext->m_uiSavedDepth = writerContext->m_ObjectWriterStack.size();

    // BaseUri is initially correct for XamlResourceUri but it gets lost later on.  Here we take the BaseUri if it's available; otherwise, we just
    // take the XamlResourceUri.  Ideally, we would just propagate the BaseUri here, but that has a high risk of regressions
    xref_ptr<IPALUri> spUri;
    spXamlSavedContext->get_BaseUri(spUri);
    if (!spUri)
    {
        spXamlSavedContext->get_XamlResourceUri(spUri);
    }
    writerContext->set_XamlResourceUri(spUri);

    writerContext->set_XbfHash(spXamlSavedContext->get_XbfHash());

    rspObjectWriterContext = std::move(writerContext);

    return S_OK;
}

HRESULT ObjectWriterContext::GetSavedContext( _Out_ std::shared_ptr<XamlSavedContext>& rspSavedContext )
{
    auto spNewStack = std::make_shared<ObjectWriterStack>(m_ObjectWriterStack.CopyStack());
    IFC_RETURN(XamlSavedContext::Create(shared_from_this(), spNewStack, rspSavedContext));
    return S_OK;
}

HRESULT ObjectWriterContext::AddNamespacePrefix( _In_ const xstring_ptr& inPrefix, _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace )
{
    RRETURN(m_ObjectWriterStack.AddNamespacePrefix(inPrefix, spXamlNamespace));
}

std::shared_ptr<XamlNamespace> ObjectWriterContext::FindNamespaceByPrefix( _In_ const xstring_ptr& inPrefix)
{
    return m_ObjectWriterStack.FindNamespaceByPrefix(inPrefix);
}

void ObjectWriterContext::PushScope()
{
    m_ObjectWriterStack.PushScope();
}

void ObjectWriterContext::PopScope()
{
    m_ObjectWriterStack.PopScope();
}

bool ObjectWriterContext::IsStackEmpty() const
{
    return m_ObjectWriterStack.empty();
}

bool ObjectWriterContext::IsInConditionalScope(bool ignoreInactiveScopes) const
{
    return m_ObjectWriterStack.IsInConditionalScope(ignoreInactiveScopes);
}

INT32 ObjectWriterContext::get_Depth() const
{
    return m_ObjectWriterStack.size();
}

INT32 ObjectWriterContext::get_SavedDepth() const
{
    return m_uiSavedDepth;
}

INT32 ObjectWriterContext::get_LiveDepth() const
{
    return (get_Depth() - get_SavedDepth());
}

ObjectWriterStack::iterator ObjectWriterContext::get_StackBegin()
{
    return m_ObjectWriterStack.begin();
}

ObjectWriterStack::iterator ObjectWriterContext::get_StackEnd()
{
    return m_ObjectWriterStack.end();
}

HRESULT ObjectWriterContext::ServiceProvider_GetAllAmbientValues(
            _In_ const std::shared_ptr<XamlType>& spCeilingType,
            _In_ const std::shared_ptr<XamlProperty>& spProperty,
            _In_ const std::shared_ptr<XamlType>& spTypeToFind,
            _In_ CompressedStackCacheHint eCacheHint,
            _Out_ AmbientValuesVector& vecValues)
 {
    bool fStoppedOnCeiling = false;
    ObjectWriterStack::iterator startingIterator = m_ObjectWriterStack.begin();
    std::shared_ptr<XamlType> spEmptyType;
    std::shared_ptr<XamlProperty> spEmptyProperty;

    while (!fStoppedOnCeiling && startingIterator != m_ObjectWriterStack.end())
    {
        AmbientValue value;
        IFC_RETURN(FindNextAmbientValue(   startingIterator,
                                    eCacheHint, /* Cache hint */
                                    spCeilingType, spEmptyType, 
                                    spProperty, spEmptyProperty,
                                    spTypeToFind,
                                    value, 
                                    &fStoppedOnCeiling));
        auto pdo = value.GetDo();
        if (pdo)
        {
            vecValues.m_vector.emplace_back(std::move(pdo));
        }
    }
    return S_OK;
 }

HRESULT ObjectWriterContext::ServiceProvider_GetAmbientValue(
            _In_ const std::shared_ptr<XamlType>& spCeilingType1,
            _In_ const std::shared_ptr<XamlType>& spCeilingType2,
            _In_ const std::shared_ptr<XamlProperty>& spProperty1,
            _In_ const std::shared_ptr<XamlProperty>& spProperty2,
            _In_ const std::shared_ptr<XamlType>& spTypeToFind,
            _In_ CompressedStackCacheHint cacheHint,
            _Out_ std::shared_ptr<XamlQualifiedObject>& rqoValue )
{
    bool fStoppedOnCeiling = false;
    ObjectWriterStack::iterator startingIterator = m_ObjectWriterStack.begin();

    AmbientValue value;
    IFC_RETURN(FindNextAmbientValue(startingIterator, 
                            cacheHint,
                            spCeilingType1, spCeilingType2, 
                            spProperty1, spProperty2, 
                            spTypeToFind,
                            value, &fStoppedOnCeiling));
    rqoValue = value.GetQoPtr();

    return S_OK;
}

// Find the next instance of a set of ambient properties given the current
// ObjectWriter stack.
// 
// (Note that WPF passes iterators for the ceilings and the properties, but we
// can't easily pass them on the stack so we've simplified things to include the
// maximum number of each that we need.  It's also worth calling out that WPF
// uses separate methods for looking up by properties or by types, but we've
// merged them into one.)
//
// NOTE: THIS METHOD IS UNABLE TO FIND AMBIENT VALUES ASSIGNED TO ATTACHED PROPERTIES
_Check_return_ 
HRESULT ObjectWriterContext::FindNextAmbientValue(
            // Iterator of the object writer stack frames
            _Inout_ ObjectWriterStack::iterator& startingIterator,
            _In_ CompressedStackCacheHint cacheHint,
            // Optional types that will end our search if walk over them
            _In_opt_ const std::shared_ptr<XamlType>& spCeilingType1,
            _In_opt_ const std::shared_ptr<XamlType>& spCeilingType2,
            // The properties whose values we are looking for
            _In_opt_ const std::shared_ptr<XamlProperty>& spProperty1,
            _In_opt_ const std::shared_ptr<XamlProperty>& spProperty2,
            // Optionally get the values of any properties of this type
            _In_opt_ const std::shared_ptr<XamlType>& spTypeToMatch,
            // The next value of the ambient property
            _Out_ AmbientValue& outValue,
            // Whether we stopped searching because we reached a ceiling type
            _Out_ bool *pfStoppedOnCeiling
    )
{
    bool fStop = false;
    AmbientValue value; // The last value found
    std::shared_ptr<XamlType> spDeclaringType1; // Declaring type of the first property to search for
    std::shared_ptr<XamlType> spDeclaringType2; // Declaring type of the first property to search for

    ObjectWriterStack::iterator frame;
    ObjectWriterStack::iterator lowerFrame;

    std::shared_ptr<XamlSchemaContext> schemaContext;
    IFC_RETURN(get_SchemaContext(schemaContext));

    *pfStoppedOnCeiling = false;

    // Get the declaring types of the properties (outside of the loop)
    ASSERT(spProperty1 || spTypeToMatch);
    if (spProperty1)
    {
        IFC_RETURN(spProperty1->get_DeclaringType(spDeclaringType1));
        ASSERT(!!spDeclaringType1);
    }
    if (spProperty2)
    {
        IFC_RETURN(spProperty2->get_DeclaringType(spDeclaringType2));
        ASSERT(!!spDeclaringType2);
    }

    if (startingIterator == m_ObjectWriterStack.end())
    {
        return S_OK;
    }

    // Start the search for ambient properties with the parent frame
    frame = startingIterator;
    while (!fStop && (++startingIterator != m_ObjectWriterStack.end()))
    {
        AmbientValue instance;
        AmbientValue lowerInstance;
        std::shared_ptr<XamlType> spType;
        std::shared_ptr<XamlType> spLowerType;
        std::shared_ptr<XamlProperty> spFrameProperty;
        bool bCanAssignTo = false;

        // Update the frames 
        lowerFrame = frame;
        frame = startingIterator;

        // Check if we have a compressed stack.
        if (frame->exists_CompressedStack())
        {
            // If we get something from the compressed stack we can stop.
            IFC_RETURN(frame->get_CompressedStack()->get_CachedItem(cacheHint, instance.m_pqo));
            if (instance)
            {
                value = std::move(instance);
                break;
            }
        }

        instance.m_pqo = frame->get_Instance();

        spType = frame->get_Type();
        spFrameProperty = frame->get_Member();
        lowerInstance.m_pqo = lowerFrame->get_Instance();
        spLowerType = lowerFrame->get_Type();

        // If the instance was null, check to see if it was stored as a weakref instead
        if (!instance)
        {
            if (frame->exists_WeakRefInstance())
            {
                instance.m_do = frame->get_WeakRefInstance().lock();
            }
        }

        // If the instance was null, check to see if it was stored as a weakref instead
        if (!lowerInstance)
        {
            if (lowerFrame->exists_WeakRefInstance())
            {
                lowerInstance.m_do = lowerFrame->get_WeakRefInstance().lock();
            }
        }

        if (spType)
        {
            // If a type was requested, check that first
            if (spTypeToMatch)
            {
                IFC_RETURN(spTypeToMatch->IsAssignableFrom(spType, bCanAssignTo));
                if (bCanAssignTo)
                {
                    value = std::move(instance);
                    fStop = true;
                }
            }

            // Otherwise check for the desired properties
            if (!fStop && spProperty1)
            {
                if (instance)
                {
                    // Note that WPF loops over a stream of properties, but
                    // since we've simplified that to just two parameters
                    // (because it's non-trivial to pass a list on the stack),
                    // we're simulating that with a constant for loop that any
                    // decent compiler will unroll.  This keeps the code closer
                    // to WPF and easier to maintain.
                    XUINT8 i;
                    for (i = 1; i <= 2; i++)
                    {
                        std::shared_ptr<XamlProperty> spProperty = (i == 1) ? spProperty1 : spProperty2;
                        std::shared_ptr<XamlType> spDeclaringType = (i == 1) ? spDeclaringType1 : spDeclaringType2;

                        // Ignore any properties/declaring types that aren't
                        // provided (since they're optional)
                        if (!spProperty || !spDeclaringType)
                        {
                            continue;
                        }

                        // If the frame contains a type that has the property
                        // we're checking for
                        // NOTE: THIS CHECK WILL FAIL IF THE DESIRED PROPERTY IS AN ATTACHED PROPERTY
                        IFC_RETURN(spDeclaringType->IsAssignableFrom(spType, bCanAssignTo));
                        if (bCanAssignTo)
                        {
                            // If we're searching from inside the target ambient
                            // property (like StaticResource inside a
                            // ResourceDictionary) it will be attached to the
                            // instance already and the normal path will handle
                            // things.
                            // 
                            // WPF also checks if spLowerType
                            // IsUsableDuringInitialization (but we don't have a
                            // notion of bottom up object creation).
                            if (XamlProperty::AreEqual(spProperty, spFrameProperty) &&
                                lowerInstance &&
                                spLowerType)
                            {
                                // If the object we're inside of is a markup
                                // extension, then we're inside a call to
                                // ProvideValue and don't want to return a
                                // reference to ourselves.
                                bool bIsMarkupExtension = false;

                                IFC_RETURN(spLowerType->IsMarkupExtension(bIsMarkupExtension));
                                if (!bIsMarkupExtension)
                                {
                                    value = std::move(lowerInstance);
                                    fStop = true;
                                }
                            }
                            else
                            {
                                IFC_RETURN(Runtime_GetAmbientValue(instance.GetQo(), spProperty, value.m_qo));
                                fStop = true;
                            }
                        }

                        // TODO: Should we break out as soon as we've found a value and possibly ignore spProperty2?  I'm leaving this as-is because it's what the old code did.
                        if (fStop)
                        {
                            break;
                        }
                    }
                }
            }

            // Stop looking if we've hit a ceiling
            if (spCeilingType1 &&
                (XamlType::AreEqual(spType, spCeilingType1) || XamlType::AreEqual(spType, spCeilingType2)))
            {
                *pfStoppedOnCeiling = true;
                fStop = true;
            }
        }
    }

    outValue = std::move(value);
    outValue.Validate();

    return S_OK;
}


HRESULT ObjectWriterContext::CheckAmbient( _In_ const std::shared_ptr<XamlProperty>& spXamlProperty )
{
    if (!spXamlProperty->IsAmbient())
    {
        RRETURN(E_FAIL);
        // TODO: Exception.
    }

    RRETURN(S_OK);
}

std::shared_ptr<XamlQualifiedObject> ObjectWriterContext::get_RootInstance() 
{
    if (!m_qoRootInstance)
    {
        m_qoRootInstance = m_ObjectWriterStack.Bottom().get_Instance();

        // Default to the event root if there's no other root instance
        if (!m_qoRootInstance)
        {
            m_qoRootInstance = m_spEventRoot;
        }
    }
    
    return m_qoRootInstance;
}

HRESULT ObjectWriterContext::ServiceProvider_ResolveXamlType( 
            _In_ const xstring_ptr_view& spName, 
            _Out_ std::shared_ptr<XamlType>& rspXamlType )
{
    std::shared_ptr<XamlTypeName> spXamlTypeName;
    XamlTypeNameParser typeNameParser(spName);

    IFC_RETURN(typeNameParser.ParseXamlTypeName(spXamlTypeName));
    std::shared_ptr<XamlNamespace> spXamlNamespace = FindNamespaceByPrefix(spXamlTypeName->get_Prefix());

    if (!spXamlNamespace)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(spXamlNamespace->GetXamlType(spXamlTypeName->get_Name(), rspXamlType));

    return S_OK;
}

HRESULT ObjectWriterContext::ServiceProvider_ResolveXamlType( 
            _In_ const XamlTypeToken& typeToken, 
            _Out_ std::shared_ptr<XamlType>& rspXamlType )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    IFC_RETURN(get_SchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetXamlType(typeToken, rspXamlType));

    return S_OK;
}

HRESULT ObjectWriterContext::EnsureTargetTypeCaches()
{
    HRESULT hr = S_OK;
    if (!m_spStyleXamlType)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;

        // We assume if we have the style type we also cached all these.
        ASSERT(!m_spStyleTargetTypeXamlProperty);
        ASSERT(!m_spControlTemplateXamlType);
        ASSERT(!m_spControlTemplateTargetTypeProperty);

        IFC(get_SchemaContext(spSchemaContext));

        IFC(spSchemaContext->GetXamlType(
            XamlTypeToken(tpkNative, KnownTypeIndex::Style),
            m_spStyleXamlType));
        IFC(spSchemaContext->GetXamlProperty(
            XamlPropertyToken(tpkNative, KnownPropertyIndex::Style_TargetType),
            XamlTypeToken(tpkNative, KnownTypeIndex::TypeName),
            m_spStyleTargetTypeXamlProperty));
        IFC(spSchemaContext->GetXamlType(
            XamlTypeToken(tpkNative, KnownTypeIndex::ControlTemplate),
            m_spControlTemplateXamlType));
        IFC(spSchemaContext->GetXamlProperty(
            XamlPropertyToken(tpkNative, KnownPropertyIndex::ControlTemplate_TargetType),
            XamlTypeToken(tpkNative, KnownTypeIndex::TypeName),
            m_spControlTemplateTargetTypeProperty));
    }

    ASSERT(!!m_spStyleXamlType);
    ASSERT(!!m_spStyleTargetTypeXamlProperty);
    ASSERT(!!m_spControlTemplateXamlType);
    ASSERT(!!m_spControlTemplateTargetTypeProperty);

Cleanup:
    if (FAILED(hr))
    {
        m_spStyleXamlType.reset();
        m_spStyleTargetTypeXamlProperty.reset();
        m_spControlTemplateXamlType.reset();
        m_spControlTemplateTargetTypeProperty.reset();
    }
    RRETURN(hr);
}

std::shared_ptr<XamlServiceProviderContext> ObjectWriterContext::get_TextSyntaxContext()
{
    return std::make_shared<XamlServiceProviderContext>(shared_from_this());
}

const std::shared_ptr<XamlServiceProviderContext> ObjectWriterContext::get_MarkupExtensionContext()
{
    return std::make_shared<XamlServiceProviderContext>(shared_from_this());
}

xref_ptr<CDependencyObject> ObjectWriterContext::AmbientValue::GetDo() const
{
    Validate();
    if (m_do)
    {
        return m_do;
    }
    auto pdo = m_qo.GetDependencyObject();
    if (!pdo && m_pqo)
    {
        pdo = m_pqo->GetDependencyObject();
    }
    return xref_ptr<CDependencyObject>(pdo);
}

XamlQualifiedObject ObjectWriterContext::AmbientValue::GetQo() const
{
    Validate();
    XamlQualifiedObject result;

    if (!m_qo.IsUnset())
    {
        VERIFYHR(m_qo.ConvertForManaged(result));
    }
    else if (m_pqo)
    {
        VERIFYHR(m_pqo->ConvertForManaged(result));
    }
    else
    {
        result.SetDependencyObject(m_do.get());
    }

    return result;
}

std::shared_ptr<XamlQualifiedObject> ObjectWriterContext::AmbientValue::GetQoPtr() const
{
    Validate();
    if (m_pqo)
    {
        return m_pqo;
    }

    std::shared_ptr<XamlQualifiedObject> result;
    if (!m_qo.IsUnset())
    {
        result = std::make_shared<XamlQualifiedObject>();
        VERIFYHR(m_qo.ConvertForManaged(*result));
    }
    else if(m_do)
    {
        result = std::make_shared<XamlQualifiedObject>();
        result->SetDependencyObject(m_do.get());
    }
    return result;
}
