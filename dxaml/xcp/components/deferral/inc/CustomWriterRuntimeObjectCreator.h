// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <MetadataAPI.h>

class CustomWriterRuntimeContext;
class StreamOffsetToken;
class XamlSavedContext;
class ObjectWriterNode;
class ObjectWriter;
class ObjectWriterSettings;
class XamlNode;
class CDependencyObject;
struct XamlQualifiedObject;
class BinaryFormatObjectWriter;
class XamlBinaryFormatSubReader2;
class CStyle;
class XamlLineInfo;
class CThemeResource;


// CustomWriterRuntimeObjectCreator supports two NameScope registration modes. We
// do this because in certain deferal scenarios we want to create objects in a way
// that ensures they aren't publically visible to third-party developers- e.g. 
// creating and using a storyboard internally but not placing it in the public visual
// tree. To ensure that the developer cannot find these elements using FindName we can
// disable name registration in the parser completely.
enum class NameScopeRegistrationMode
{
    RegisterEntries,
    SkipRegistration
};

// Provides services to instances passed CustomRuntimeData with StreamOffsetTokens
// that allow the instance to create an instance of that object without requiring
// deep knowledge of the parser or underlying representation.
class CustomWriterRuntimeObjectCreator
{
public:
    CustomWriterRuntimeObjectCreator(
        NameScopeRegistrationMode mode,
        _In_ const CustomWriterRuntimeContext* context);

    _Check_return_ HRESULT LookupStaticResourceValue(
        _In_ StreamOffsetToken token,
        _In_opt_ CStyle* optimizedStyleParent,
        _In_ KnownPropertyIndex stylePropertyIndex,
        _Out_ std::shared_ptr<CDependencyObject>* pResourceValue);

    _Check_return_ HRESULT LookupStaticResourceValue(
        _In_ StreamOffsetToken token,
        _Out_ std::shared_ptr<CDependencyObject>* pResourceValue);

    _Check_return_ HRESULT CreateThemeResourceInstance(
        _In_ StreamOffsetToken token,
        _In_opt_ CStyle* optimizedStyleParent,
        _In_ KnownPropertyIndex stylePropertyIndex,
        _Out_ std::shared_ptr<XamlQualifiedObject>* pResource);

    _Check_return_ HRESULT CreateThemeResourceInstance(_In_ StreamOffsetToken token,
        _Out_ std::shared_ptr<XamlQualifiedObject>* pResource);

    // When the entire stream is a single object instance we allow CustomWriterObjectCreator to
    // be invoked without a StreamOffsetToken.
    _Check_return_ HRESULT CreateInstance(
        _Out_ std::shared_ptr<CDependencyObject>* pInstance,
        _Out_ xref_ptr<CThemeResource>* resultAsThemeResource);

    _Check_return_ HRESULT CreateInstance(
        _In_ StreamOffsetToken token,
        _Out_ std::shared_ptr<CDependencyObject>* pInstance,
        _Out_ xref_ptr<CThemeResource>* resultAsThemeResource);

    // When we create an entry with NameScopeRegistrationMode::SkipRegistration they are
    // by excluded from TemplateNameScope registration. For cases where we're
    // only instantiating small parts of a larger element tree for specific
    // purposes that's fine. When we're trying to create these previously
    // created instances and make them part of a fully-expanded, faulted-in
    // tree we'll want to make sure they respond to FindName/GetTemplateChild
    // calls in the right way. This method will register them properly with
    // the TemplateNameScope table.
    _Check_return_ HRESULT RegisterTemplateNameScopeEntriesIfNeeded(_In_ CDependencyObject* instance);

    // Given a existing object, run the ObjectWriter over it and apply directives and property
    // sets to it. A vector of index ranges can be passed to this method to skip certain property
    // sets and other pieces of the XBF stream.
    _Check_return_ HRESULT ApplyStreamToExistingInstance(
        _In_ StreamOffsetToken token, _In_ CDependencyObject* instance,
        _In_ const std::vector<StreamOffsetToken>& indexRangesToSkip);

    // Returns the XamlLineInfo for the node corresponding to the input StreamOffsetToken
    XamlLineInfo GetLineInfoForToken(_In_ StreamOffsetToken token);

private:
    _Check_return_ HRESULT RunObjectWriter(
        _In_ StreamOffsetToken token,
        _In_opt_ CDependencyObject* instance, 
        _In_ const std::vector<StreamOffsetToken>& indexRangesToSkip);

    _Check_return_ HRESULT EnsureObjectWriter();
    std::shared_ptr<XamlSavedContext> BuildSavedContext();
    _Check_return_ HRESULT BuildObjectWriterSettings(_Out_ ObjectWriterSettings* pResult);
    std::shared_ptr<XamlBinaryFormatSubReader2> GetReaderAndSetIndex(unsigned int nodeIndex);
    void RestoreReaderIndex();

    static _Check_return_ HRESULT XamlQOFromCDOHelper(
        _In_ CDependencyObject* cdo,
        _Out_ std::shared_ptr<XamlQualifiedObject>* pQO);

    const CustomWriterRuntimeContext* m_context;
    std::shared_ptr<BinaryFormatObjectWriter> m_writer;
    bool m_pendingFirstNode;
    NameScopeRegistrationMode m_mode;
    unsigned int m_restoreIndex;
};


