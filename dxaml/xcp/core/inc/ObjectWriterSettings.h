// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;
class ObjectWriterCallbacksDelegate;

#include "IObjectWriterCallbacks.h"
#include "INamescope.h"
#include "XamlBinaryMetadata.h"

class ObjectWriterSettings
{
private:
    std::shared_ptr<ObjectWriterCallbacksDelegate> m_spObjectWriterCallbacks;
    std::shared_ptr<XamlQualifiedObject> m_qoRootObjectInstance;
    std::shared_ptr<XamlQualifiedObject> m_qoEventRoot;
    std::shared_ptr<XamlQualifiedObject> m_qoXBindConnector;
    std::shared_ptr<XamlQualifiedObject> m_qoParentXBindConnector;
    xref_ptr<INameScope> m_spNameScope;

    // NOTE on the System.Xaml parser, the BaseUri is set on the XamlTextReaderSettings,
    // and is then passed and is then passed from the Parser to the ObjectWriter through a special
    // property.  For our current use, setting the BaseUri on the ObjectWriterSettings is more
    // direct, but can be changed (and the special directive property, x:Base, added) if necessary.
    xref_ptr<IPALUri> m_spBaseUri;

    // Used by Binding Xaml debugging for the generic.xaml case in order to store the ResourceUri.
    // This is a separate field so we do not conflict with the baseUri.
    xref_ptr<IPALUri> m_spXamlResourceUri;

    xstring_ptr m_xbfHash;

    bool m_bExpandTemplates;
    bool m_bCheckDuplicateProperty;
    bool m_bEnableEncoding;

public:
    ObjectWriterSettings()
        : m_bExpandTemplates(false)
        , m_bCheckDuplicateProperty(false)
        , m_bEnableEncoding(false)
    {
    }

    void set_ObjectWriterCallbacks(_In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spObjectWriterCallbacks)
    {
        m_spObjectWriterCallbacks = spObjectWriterCallbacks;
    }

    const std::shared_ptr<ObjectWriterCallbacksDelegate>& get_ObjectWriterCallbacks() const
    {
        return m_spObjectWriterCallbacks;
    }

    void set_RootObjectInstance(_In_ const std::shared_ptr<XamlQualifiedObject>& qoRootObjectInstance)
    {
        m_qoRootObjectInstance = qoRootObjectInstance;
    }

    const std::shared_ptr<XamlQualifiedObject>& get_RootObjectInstance() const
    {
        return m_qoRootObjectInstance;
    }

    void set_EventRoot(_In_ const std::shared_ptr<XamlQualifiedObject>& qoEventRoot)
    {
        m_qoEventRoot = qoEventRoot;
    }

    const std::shared_ptr<XamlQualifiedObject>& get_EventRoot() const
    {
        return m_qoEventRoot;
    }

    void set_EnableEncoding(bool bValue)
    {
        m_bEnableEncoding = bValue;
    }

    bool get_EnableEncoding() const
    {
        return m_bEnableEncoding;
    }

    void set_ExpandTemplates(bool bExpandTemplates)
    {
        m_bExpandTemplates = bExpandTemplates;
    }

    bool get_ExpandTemplates() const
    {
        return m_bExpandTemplates;
    }

    void set_CheckDuplicateProperty(bool bCheckDuplicateProperty)
    {
        m_bCheckDuplicateProperty = bCheckDuplicateProperty;
    }

    bool get_CheckDuplicateProperty() const
    {
        return m_bCheckDuplicateProperty;
    }

    void set_NameScope(xref_ptr<INameScope> spValue)
    {
        m_spNameScope = std::move(spValue);
    }

    const xref_ptr<INameScope>& get_NameScope() const
    {
        return m_spNameScope;
    }

    void set_XbfHash(_In_ const xstring_ptr& strHash)
    {
        m_xbfHash = strHash;
    }

    const xstring_ptr& get_XbfHash() const
    {
        return m_xbfHash;
    }

    //
    // NOTE on the System.Xaml parser, the BaseUri is set on the XamlTextReaderSettings,
    // and is then passed and is then passed from the Parser to the ObjectWriter through a special
    // property.  For our current use, setting the BaseUri on the ObjectWriterSettings is more
    // direct, but can be changed (and the special directive property, x:Base, added) if necessary.
    //
    void set_BaseUri(xref_ptr<IPALUri> spBaseUri)
    {
        m_spBaseUri = std::move(spBaseUri);
    }

    const xref_ptr<IPALUri>& get_BaseUri() const
    {
        return m_spBaseUri;
    }

    void set_XamlResourceUri(xref_ptr<IPALUri> spXamlResourceUri)
    {
        m_spXamlResourceUri = std::move(spXamlResourceUri);
    }

    const xref_ptr<IPALUri>& get_XamlResourceUri() const
    {
        return m_spXamlResourceUri;
    }

    bool get_AllowCustomWriter() const
    {
        return m_bEnableEncoding;
    }

    void set_XBindConnector(_In_ const std::shared_ptr<XamlQualifiedObject>& qoXBindConnector)
    {
        m_qoXBindConnector = qoXBindConnector;
    }

    const std::shared_ptr<XamlQualifiedObject>& get_XBindConnector() const
    {
        return m_qoXBindConnector;
    }

    void set_XBindParentConnector(_In_ const std::shared_ptr<XamlQualifiedObject>& qoParentXBindConnector)
    {
        m_qoParentXBindConnector = qoParentXBindConnector;
    }

    const std::shared_ptr<XamlQualifiedObject>& get_XBindParentConnector() const
    {
        return m_qoParentXBindConnector;
    }
};