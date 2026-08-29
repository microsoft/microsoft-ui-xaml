// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlParserContext;

// Specifies a set of features to support when parsing XAML.
class XamlTextReaderSettings
{
private:
    // A value indicating whether the XAML parser requires that the default
    // namespace is explicitly provided.
    bool m_bRequireDefaultNamespace;

    // value indicating whether the x:Uid attribute should be processed
    // or ignored.
    bool m_shouldProcessUid;

    // value indicating if the text fragment being processed should be 
    // treated as UTF-16 encoded
    bool m_bIsUtf16Encoded;
    
    
public:
    XamlTextReaderSettings(
        // A value indicating whether the XAML parser requires that the default
        // namespace is explicitly provided.
        bool requireDefaultNamespace,

        // Should x:Uid be processed
        bool shouldProcessUid,

        // Should force treat as UTF-16
        bool bIsUtf16Encoded
        )
        : m_bRequireDefaultNamespace(requireDefaultNamespace)
        , m_shouldProcessUid(shouldProcessUid)
        , m_bIsUtf16Encoded(bIsUtf16Encoded)
    {
    }

    // Gets value indicating whether the x:Uid attribute should be processed
    // or ignored.
    bool get_ShouldProcessUid() const { return m_shouldProcessUid; };

    // Gets value indicating whether the text fragment should be treated as UTF-16 encoded.
    bool get_IsUtf16Encoded() const { return m_bIsUtf16Encoded; };
    
    // Gets a value indicating whether the XAML parser requires that the default
    // namespace is explicitly provided.  In certain scenarios (like Silverlight
    // 1.0 compat, or Silverlight 2.0 XamlReader.Load("...") compat), the parser
    // will automatically add the default XML namespace for Silverlight
    // applications.
    bool get_RequireDefaultNamespace() const { return m_bRequireDefaultNamespace; }

    // Optionally add the default XML namespace prefixes for Silverlight
    // applications to the given parser context.  This method will be called by
    // the XamlScanner as it's reading the start of the first element.  If the
    // RequireDefaultNamespace property is false, it will associate the default
    // prefix ("") with the URI of the default Silverlight namespace
    // ("http://schemas.microsoft.com/winfx/2006/xaml/presentation").
    // 
    // Note that System.Xaml.XamlTextReaderSettings provides a public
    // XmlnsDictionary property that can be filled with any number of namespaces
    // to be included automatically by a XAML platform.  We only have a single
    // constant namespace, so a dictionary would be wasteful.  We've used a
    // method to add namespaces instead of properties for the single namespace
    // prefix and URI so that future namespaces can be added or quirked with
    // minimal impact.
    _Check_return_ HRESULT AddDefaultXmlNamespacePrefixesIfNotRequired(
        // The parser context which is used to both find the XamlNamespace
        // instances associated with our default XML namespace URIs and create
        // the association between our default XML namespace prefixes and URIs.
        _In_ const std::shared_ptr<XamlParserContext>& spParserContext) const;
};

