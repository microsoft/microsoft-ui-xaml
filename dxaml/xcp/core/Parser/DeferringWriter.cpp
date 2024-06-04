// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Stores the stream of XamlNodes between a <Foo.Template> start member and
//      a </Foo.Template> end member or <ResourceDictionary.__implicit_items>
//      start member and a </ResourceDictionary.__implicit_items> end member.

#include "precomp.h"

#include <ObjectWriterContext.h>
#include "XamlPredicateHelpers.h"

// Initializes a new instance of the DeferringWriter class.
DeferringWriter::DeferringWriter(
    _In_ std::shared_ptr<ObjectWriterContext>& spContext,
    _In_ bool encoding)
{
    m_spContext = spContext;
    m_eMode = dmOff;
    m_fHandled = FALSE;
    m_iDeferredTreeDepth = 0;
    m_encoding = encoding;
    // disable resource dictionary deferral if ExpandTemplatesWithInitialValidation is TRUE
    // this is required for APIs like XamlReader.LoadTemplateWithInitialValidation() to
    // be able to report parse errors immediately
    m_fDisableResourceDictionaryDefer = spContext->get_ExpandTemplates();
    m_fIsDictionaryWithKeyProperty = FALSE;
}


// Gets the XamlNodeList that was stored while deferring the XamlNodes in a
// template.  This should only be called when the mode is dmTemplateReady.  It
// will clear the stored list and changed the mode to dmOff.
_Check_return_ HRESULT DeferringWriter::CollectTemplateList(
    _Out_ std::shared_ptr<XamlOptimizedNodeList>& spXamlNodeList)
{
    // A XamlNodeList should have been deferred before we collect it
    ASSERT(m_eMode == dmTemplateReady);

    spXamlNodeList = m_spDeferredList;
    m_spDeferredList.reset();
    m_eMode = dmOff;

    RRETURN(S_OK);
}

// Gets the XamlNodeList that was stored while deferring the XamlNodes in a
// resource dictionary.  This should only be called when the mode is dmResourceDictionaryReady.  It
// will clear the stored list and changed the mode to dmOff.
_Check_return_ HRESULT DeferringWriter::CollectResourceDictionaryList(
    _Out_ std::shared_ptr<XamlOptimizedNodeList>& spXamlNodeList,
    _Out_ bool *pfIsDictionaryWithKeyProperty)
{
    // A XamlNodeList should have been deferred before we collect it
    ASSERT(m_eMode == dmResourceDictionaryReady);

    spXamlNodeList = m_spDeferredList;
    *pfIsDictionaryWithKeyProperty = m_fIsDictionaryWithKeyProperty;

    m_spDeferredList.reset();
    m_eMode = dmOff;
    m_fIsDictionaryWithKeyProperty = FALSE;

    RRETURN(S_OK);
}

// Start writing an object.
_Check_return_ HRESULT DeferringWriter::WriteObject(
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ bool fFromMember)
{
    m_fHandled = FALSE;
    switch (m_eMode)
    {
    case dmOff:
        break;

    case dmTemplateReady:
    case dmResourceDictionaryReady:
        ASSERT(FALSE);
        break;

    case dmTemplateDeferring:
    case dmResourceDictionaryDeferring:
        {
            m_spDeferredWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_spDeferredWriter->WriteObject(spXamlType, fFromMember));
            m_iDeferredTreeDepth++;
            m_fHandled = TRUE;
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return S_OK;
}


// Finish writing an object.
_Check_return_ HRESULT DeferringWriter::WriteEndObject()
{
    m_fHandled = FALSE;
    switch (m_eMode)
    {
    case dmOff:
        break;

    case dmTemplateReady:
    case dmResourceDictionaryReady:
        ASSERT(FALSE);
        break;

    case dmTemplateDeferring:
    case dmResourceDictionaryDeferring:
        {
            m_spDeferredWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_spDeferredWriter->WriteEndObject());
            m_fHandled = TRUE;
            m_iDeferredTreeDepth--;

            // We shouldn't find ourselves deferring more than the template/resource dictionary
            ASSERT(m_iDeferredTreeDepth >= 0);
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return S_OK;
}


// Start writing a member.
_Check_return_ HRESULT DeferringWriter::WriteMember(
    _In_ const std::shared_ptr<XamlProperty>& spProperty)
{
    m_fHandled = FALSE;

    switch (m_eMode)
    {
    case dmOff:
        {
            std::shared_ptr<XamlType>          spPropertyType;

            bool bIsTemplate = false;
            bool bIsResourceDictionary = false;

            IFC_RETURN(spProperty->get_Type(spPropertyType));

            const bool isVisualStateDeferredProperty =
                   !!spProperty->get_PropertyToken().Equals(XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualState___DeferredStoryboard))
                || !!spProperty->get_PropertyToken().Equals(XamlPropertyToken(XamlTypeInfoProviderKind::tpkNative, KnownPropertyIndex::VisualState___DeferredSetters));

            // Check if the type of the property we're starting to write is for
            // a template
            if (spPropertyType && !(isVisualStateDeferredProperty && m_encoding))
            {
                IFC_RETURN(spPropertyType->IsTemplate(bIsTemplate));
            }
            // Check if the type of the property is the implicit items property for
            // a resource dictionary
            else if ((!m_fDisableResourceDictionaryDefer) &&
                     (spProperty->IsImplicit() && std::static_pointer_cast<ImplicitProperty>(spProperty)->get_ImplicitPropertyType() == iptItems))
            {
                auto& tokenType = m_spContext->Current().get_Type()->get_TypeToken();

                if (tokenType == XamlTypeToken(tpkNative, KnownTypeIndex::ResourceDictionary) ||
                    tokenType == XamlTypeToken(tpkNative, KnownTypeIndex::ColorPaletteResources))
                {
                    std::shared_ptr<XamlSchemaContext> spSchemaContext;
                    std::shared_ptr<DirectiveProperty> spKeyProperty;

                    // Matched the type to a ResourceDictionary
                    bIsResourceDictionary = TRUE;

                    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
                    IFC_RETURN(spSchemaContext->get_X_KeyProperty(spKeyProperty));

                    m_fIsDictionaryWithKeyProperty = m_spContext->Current().contains_Directive(spKeyProperty);
                }
            }

            if (bIsTemplate || bIsResourceDictionary)
            {
                std::shared_ptr<XamlSchemaContext> spSchemaContext;

                // Start deferring XamlNodes. Note that we don't defer the start member.
                if (bIsResourceDictionary)
                {
                    m_eMode = dmResourceDictionaryDeferring;
                }
                else
                {
                    m_eMode = dmTemplateDeferring;
                }

                IFC_RETURN(GetSchemaContext(spSchemaContext));
                m_spDeferredList = std::make_shared<XamlOptimizedNodeList>(spSchemaContext);
                IFC_RETURN(m_spDeferredList->get_Writer(m_spDeferredWriter));
                m_iDeferredTreeDepth = 0;
            }
        }
        break;

    case dmTemplateReady:
    case dmResourceDictionaryReady:
        ASSERT(FALSE);
        break;

    case dmTemplateDeferring:
    case dmResourceDictionaryDeferring:
        {
            m_spDeferredWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_spDeferredWriter->WriteMember(spProperty));
            m_fHandled = TRUE;
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return S_OK;
}


// Finish writing a member.
_Check_return_ HRESULT DeferringWriter::WriteEndMember()
{
    m_fHandled = FALSE;
    switch (m_eMode)
    {
    case dmOff:
        break;

    case dmTemplateReady:
    case dmResourceDictionaryReady:
        ASSERT(FALSE);
        break;

    case dmTemplateDeferring:
        {
            // If we finish deferring the template we started with, change to
            // the dmTemplateReady state
            if (m_iDeferredTreeDepth == 0)
            {
                m_spDeferredWriter->Close();
                m_spDeferredWriter.reset();
                m_eMode = dmTemplateReady;
            }
            else
            {
                m_spDeferredWriter->SetLineInfo(GetLineInfo());
                IFC_RETURN(m_spDeferredWriter->WriteEndMember());
            }

            m_fHandled = TRUE;
        }
        break;

    case dmResourceDictionaryDeferring:
        {
            // If we finish deferring the resource dictionary we started with, change to
            // the dmResourceDictionaryDeferring state
            if (m_iDeferredTreeDepth == 0)
            {
                m_spDeferredWriter->Close();
                m_spDeferredWriter.reset();
                m_eMode = dmResourceDictionaryReady;
            }
            else
            {
                m_spDeferredWriter->SetLineInfo(GetLineInfo());
                IFC_RETURN(m_spDeferredWriter->WriteEndMember());
            }

            m_fHandled = TRUE;
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return S_OK;
}

// Start writing a conditional XAML section
_Check_return_ HRESULT DeferringWriter::WriteConditionalScope(const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs)
{
    m_fHandled = FALSE;

    switch (m_eMode)
    {
        case dmOff:
            break;
        case dmTemplateReady:
        case dmResourceDictionaryReady:
            ASSERT(false);
            break;
        case dmTemplateDeferring:
        case dmResourceDictionaryDeferring:
        {
            m_spDeferredWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_spDeferredWriter->WriteConditionalScope(xamlPredicateAndArgs));
            m_fHandled = TRUE;
        }
        break;

        default:
            ASSERT(false);
            break;
    }

    return S_OK;
}

// Finish writing a conditional XAML section
_Check_return_ HRESULT DeferringWriter::WriteEndConditionalScope()
{
    m_fHandled = FALSE;

    switch (m_eMode)
    {
        case dmOff:
            break;
        case dmTemplateReady:
        case dmResourceDictionaryReady:
            ASSERT(false);
            break;
        case dmTemplateDeferring:
        case dmResourceDictionaryDeferring:
        {
            m_spDeferredWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_spDeferredWriter->WriteEndConditionalScope());
            m_fHandled = TRUE;
        }
        break;

        default:
            ASSERT(false);
            break;
    }

    return S_OK;
}


// Write the value of a member.
_Check_return_ HRESULT DeferringWriter::WriteValue(
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    m_fHandled = FALSE;
    switch (m_eMode)
    {
    case dmOff:
        break;

    case dmTemplateReady:
    case dmResourceDictionaryReady:
        ASSERT(FALSE);
        break;

    case dmTemplateDeferring:
    case dmResourceDictionaryDeferring:
        if (m_iDeferredTreeDepth == 0)
        {
            // We have a case where we have just started deferring, and the first thing
            // we see is a value, then is a node-replacement.  we transition back to dmOff, and
            // let the objectwriter handle this.


            ASSERT(spValue != NULL);

            #if DEBUG
            {
                std::shared_ptr<XamlSchemaContext> spSchemaContext;
                std::shared_ptr<XamlType> spValueType;
                bool bIsTemplate = false;

                IFC_RETURN(GetSchemaContext(spSchemaContext));
                IFC_RETURN(spSchemaContext->GetXamlType(spValue->GetTypeToken(), spValueType));
                ASSERT(spValueType != NULL);
                IFC_RETURN(spValueType->IsTemplate(bIsTemplate));
                ASSERT(bIsTemplate);
            }
            #endif

            m_fHandled = FALSE;
            m_eMode = dmOff;
        }
        else
        {
            m_spDeferredWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_spDeferredWriter->WriteValue(spValue));
            m_fHandled = TRUE;
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return S_OK;
}


// Write a namespace.
_Check_return_ HRESULT DeferringWriter::WriteNamespace(
    _In_ const xstring_ptr& spPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace)
{
    switch (m_eMode)
    {
    case dmOff:
        break;

    case dmTemplateReady:
    case dmResourceDictionaryReady:
        ASSERT(FALSE);
        break;

    case dmTemplateDeferring:
    case dmResourceDictionaryDeferring:
        {
            m_spDeferredWriter->SetLineInfo(GetLineInfo());
            IFC_RETURN(m_spDeferredWriter->WriteNamespace(spPrefix, spXamlNamespace));
            m_fHandled = TRUE;
        }
        break;

    default:
        ASSERT(FALSE);
        break;
    }

    return S_OK;
}


// Get the XamlSchemaContext associated with the writer.
_Check_return_ HRESULT DeferringWriter::GetSchemaContext(
    _Out_ std::shared_ptr<XamlSchemaContext>& spXamlSchemaContext ) const
{
    RRETURN(m_spContext->get_SchemaContext(spXamlSchemaContext));
}


