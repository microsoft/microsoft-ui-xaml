// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Specifies a set of features to support when parsing XAML.

#include "precomp.h"

// Optionally add the default XML namespace prefixes for Silverlight
// applications to the given parser context.  This method will be called by the
// XamlScanner as it's reading the start of the first element.  If the
// RequireDefaultNamespace property is false, it will associate the default
// prefix ("") with the URI of the default Silverlight namespace
// ("http://schemas.microsoft.com/winfx/2006/xaml/presentation").
// 
// Note that System.Xaml.XamlTextReaderSettings provides a public
// XmlnsDictionary property that can be filled with any number of namespaces to
// be included automatically by a XAML platform.  We only have a single constant
// namespace, so a dictionary would be wasteful.  We've used a method to add
// namespaces instead of properties for the single namespace prefix and URI so
// that future namespaces can be added or quirked with minimal impact.
_Check_return_ HRESULT XamlTextReaderSettings::AddDefaultXmlNamespacePrefixesIfNotRequired(
    // The parser context which is used to both find the XamlNamespace instances
    // associated with our default XML namespace URIs and create the association
    // between our default XML namespace prefixes and URIs.
    _In_ const std::shared_ptr<XamlParserContext>& spParserContext) const
{
    // Check if we should optionally use the default Silverlight XML namespace
    if (!m_bRequireDefaultNamespace)
    {
        // Make sure the the default Silverlight XML namespace prefix ("") hasn't
        // already been associated with a namespace
        std::shared_ptr<XamlNamespace> spDefaultXmlNamespace = spParserContext->FindNamespaceByPrefix(xstring_ptr::EmptyString());
        if (!spDefaultXmlNamespace)
        {
            std::shared_ptr<XamlSchemaContext> spSchemaContext;

            xstring_ptr spDefaultXmlNsUri(c_strDefaultXmlNsUri);

            // Turn the URI into a XAML namespace (Note: We're assuming we
            // already have an entry for the default namespace in the schema
            // context which will be returned by GetXamlNamespace.  This is
            // currently the case becasuse the shared XamlSchemaContext is
            // initialized with the standard XAML namespaces.)
            IFC_RETURN(spParserContext->get_SchemaContext(spSchemaContext));

            // Get the XAML namespace for the default Silverlight XML namespace URI
            // ("http://schemas.microsoft.com/winfx/2006/xaml/presentation")
            IFC_RETURN(spSchemaContext->GetXamlXmlNamespace(c_strDefaultXmlNsUri, spDefaultXmlNamespace));

            // Associate the default XML namespace prefix with the XamlNamespace
            IFC_RETURN(spParserContext->AddNamespacePrefix(xstring_ptr::EmptyString(), spDefaultXmlNamespace));
        }
    }

    return S_OK;
}


