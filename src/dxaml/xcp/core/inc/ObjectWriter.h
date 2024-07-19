// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <functional>

#include "XamlWriter.h"
#include "XamlNodeType.h"
#include "INamescope.h"
#include "ObjectWriterSettings.h"

class DeferringWriter;
struct XamlQualifiedObject;
class ObjectWriterContext;
class XamlSchemaContext;
class ObjectWriterCallbacksDelegate;
class XamlSavedContext;
class XamlType;
class XamlProperty;
class DirectiveProperty;
class XamlOptimizedNodeList;
class ParserErrorReporter;
class ObjectWriterCommonRuntime;
class ObjectWriterErrorService;
class ObjectWriterNodeList;
class XamlTextSyntax;
struct XamlTypeToken;

class CustomWriterManager;
class StreamOffsetToken;
class CustomWriterRuntimeData;

struct IError;
struct IErrorService;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// Converts a stream of XamlNodes into an object graph, using an
// ObjectWriterCommonRuntime to either create actual DOs or an encoder to
// create a new intermediate representation tree.
class ObjectWriter final
    : public XamlWriter
{
private:
    std::vector<std::tuple<XamlLineInfo, xstring_ptr, std::shared_ptr<XamlNamespace>>> m_namespacePrefixQueue;
    std::shared_ptr<XamlQualifiedObject> m_qoLastInstance;
    std::shared_ptr<XamlQualifiedObject> m_qoRootObjectInstance;
    std::shared_ptr<XamlQualifiedObject> m_qoEventRoot;
    std::shared_ptr<ObjectWriterContext> m_spContext;
    std::shared_ptr<ObjectWriterCallbacksDelegate> m_spObjectWriterCallbacks;
    std::shared_ptr<ObjectWriterCommonRuntime> m_spRuntime;
    std::shared_ptr<ObjectWriterErrorService> m_spErrorService;

    bool m_bCheckDuplicateProperty;
    bool m_bIsEncoding;
    bool m_allowCustomWriter;
    bool m_skipCustomWriterProcessingOfStartObject;
    XUINT32 m_skipDepth;

    // CTemplateContent and CDeferredKeys/Resource Dictionaries are currently
    // clumped together in DeferringWriter. Long-term they should move into
    // the CustomWriterManager below.
    std::shared_ptr<DeferringWriter> m_spDeferringWriter;
    std::shared_ptr<CustomWriterManager> m_spCustomWriterManager;

    XamlLineInfo m_rootLineInfo;

    enum SkipProcessingStage
    {
        spsNotSkipping = 0,
        spsSkipping,
    };
    SkipProcessingStage m_skipStage;

    // Forwards the action to both the DeferringWriter and the CustomerWriterManager using the following policy:
    // - If either of them are handling the nodestream, they alone will receive the action.
    // - If neither of them are handling the nodestream, CustomWriterManager will receive the action
    //   first, and then DeferringWriter will.
    // - We ASSERT that they both didn't try to start handling the nodestream simultaneously.
    // Because ObjectWriter will be undergoing a major refactoring soon, this is a good clean way
    // to unblock work on the new VisualStateGroupCollectionCustomWriter.
    _Check_return_ HRESULT ForwardActionToChildrenWriters(
        const std::function<HRESULT()>& deferringWriterAction,
        const std::function<HRESULT(bool&)>& customWriterManagerAction,
        _Out_ bool& isDeferred);

public:
    ObjectWriter()
        : m_skipDepth(0)
        , m_skipStage(spsNotSkipping)
        , m_bIsEncoding(false)
        , m_allowCustomWriter(true)
        , m_skipCustomWriterProcessingOfStartObject(false)
    {
    }

    ~ObjectWriter() override
    {
        // m_qoLastInstance is explicitly reset here because its CValue, m_value, may be pointing to a non-ref-counted CDependencyObject.
        // This prevents the XamlQualifiedObject accessing a destructed CDependencyObject in its destructor.
        m_qoLastInstance.reset();
    }

    static _Check_return_ HRESULT Create(
        _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
        _In_ const ObjectWriterSettings& settings,
        _Out_ std::shared_ptr<ObjectWriter>& rspObjectWriter)
    {
        std::shared_ptr<XamlSavedContext> spSavedContext;
        RRETURN(Create(spSchemaContext, spSavedContext, settings, rspObjectWriter));
    }

    static _Check_return_ HRESULT Create(
        _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
        _Out_ std::shared_ptr<ObjectWriter>& rspObjectWriter)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        RRETURN(Create(spSchemaContext, spSavedContext, ObjectWriterSettings(), rspObjectWriter));
    }

    static _Check_return_ HRESULT Create(
        _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
        _In_ const ObjectWriterSettings& settings,
        _Out_ std::shared_ptr<ObjectWriter>& rspObjectWriter)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        RRETURN(Create(spSchemaContext, spSavedContext, settings, rspObjectWriter));
    }

    static _Check_return_ HRESULT Create(
        _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
        _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
        _In_ const ObjectWriterSettings& settings,
        _Out_ std::shared_ptr<ObjectWriter>& rspObjectWriter);

    _Check_return_ HRESULT WriteObject(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ bool fromMember) override;
    _Check_return_ HRESULT WriteEndObject() override;
    _Check_return_ HRESULT WriteMember(_In_ const std::shared_ptr<XamlProperty>& spProperty) override;
    _Check_return_ HRESULT WriteEndMember() override;
    _Check_return_ HRESULT WriteConditionalScope(_In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) override;
    _Check_return_ HRESULT WriteEndConditionalScope() override;
    _Check_return_ HRESULT WriteValue(_In_ const std::shared_ptr<XamlQualifiedObject>& value) override;
    _Check_return_ HRESULT WriteNamespace(
        _In_ const xstring_ptr& spPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace) override;

    std::shared_ptr<XamlQualifiedObject> get_Result() const;

    _Check_return_ HRESULT GetSchemaContext(_Out_ std::shared_ptr<XamlSchemaContext>& rspOut) const override;

    _Check_return_ HRESULT ForceAssignmentToParentCollection(_In_ const std::shared_ptr<XamlQualifiedObject>& parentCollection);
    void DisableResourceDictionaryDefer();
    void EnableResourceDictionaryDefer();

    // inserts a marker in the stream and returns an index to the caller
    // that can be used to track the marker
    _Check_return_ HRESULT GetStreamOffsetToken(_Out_ StreamOffsetToken* pToken);

    // if there was an encoder enabled, we can retrieved the node list
    // from the object writer
    std::shared_ptr<ObjectWriterNodeList> GetNodeList();

    // Special workaround during CustomWriter bring-up: TemplateContent likes to try and
    // optimize things. CustomWriter isn't cool with that.
    bool IsCustomWriterActive();

    void SetActiveObject(_In_ std::shared_ptr<XamlQualifiedObject> object);

    // TODO: Hack - Figure out better way
    void SetSkipProcessingStartObjectByCustomWriter(bool skipCustomWriterProcessingOfStartObject)
    {
        m_skipCustomWriterProcessingOfStartObject = skipCustomWriterProcessingOfStartObject;
    }

protected:
    // This is called by WriteNode when it sees that an error has occurred.  It
    // should not be called by anyone else.
    _Check_return_ HRESULT ProcessError() override;

private:
    _Check_return_ HRESULT WriteObjectCore(
        const std::shared_ptr<XamlType>& spXamlType,
        bool fromMember);
    _Check_return_ HRESULT WriteEndObjectCore();
    _Check_return_ HRESULT WriteMemberCore(_In_ const std::shared_ptr<XamlProperty>& spProperty);
    _Check_return_ HRESULT WriteEndMemberCore();
    _Check_return_ HRESULT WriteValueCore(_In_ const std::shared_ptr<XamlQualifiedObject>& value);
    _Check_return_ HRESULT WriteNamespaceCore(
        _In_ const xstring_ptr& spPrefix,
        const std::shared_ptr<XamlNamespace>& spXamlNamespace);

    _Check_return_ HRESULT get_X_KeyProperty(_Out_ std::shared_ptr<DirectiveProperty>& rspOut);
    _Check_return_ HRESULT get_X_NameProperty(_Out_ std::shared_ptr<DirectiveProperty>& rspOut);

    _Check_return_ HRESULT Logic_ShouldWriteMemberCreateWithCtor(_Out_ bool& fOut);
    _Check_return_ HRESULT Logic_CreateWithCtor();
    _Check_return_ HRESULT Logic_CreateFromInitializationValue();
    _Check_return_ HRESULT Logic_CreatePropertyValueFromText();
    _Check_return_ HRESULT Logic_DuplicatePropertyCheck(_In_ const std::shared_ptr<XamlProperty>& spProperty);
    _Check_return_ HRESULT Logic_ApplyCurrentSavedDirectives();
    _Check_return_ HRESULT Logic_ProvideValue();
    _Check_return_ HRESULT Logic_AssignProvidedValue();
    _Check_return_ HRESULT Logic_DoAssignmentToParentCollection();
    _Check_return_ HRESULT Logic_DoAssignmentToParentProperty();
    _Check_return_ HRESULT Logic_SetTemplateProperty(_In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList);
    _Check_return_ HRESULT Logic_SetResourceDictionaryItemsProperty(_In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList, _In_ bool fIsDictionaryWithKeyProperty);
    _Check_return_ HRESULT Logic_SetupRootInstance(_In_ std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance);
    _Check_return_ HRESULT Logic_CheckAndSetCustomRuntimeData();

    // Create an association between the current instance and a name in a
    // template's namescope.
    _Check_return_ HRESULT Logic_RegisterName_OnCurrent(_In_ const std::shared_ptr<XamlQualifiedObject>& qoName);

    // Create an association between the parent instance and a name in a
    // template's namescope
    _Check_return_ HRESULT Logic_RegisterName_OnParent(_In_ const std::shared_ptr<XamlQualifiedObject>& qoName);

    _Check_return_ HRESULT RemapDirectiveToProperty(
        _In_ const std::shared_ptr<XamlType>& inParentType,
        _In_ const std::shared_ptr<DirectiveProperty>& inDirectiveProperty,
        _Out_ std::shared_ptr<XamlProperty>& outRemappedProperty);

    _Check_return_ HRESULT ExpandTemplateForInitialValidation(_In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList);

    void UpdateSkipDepthForResourceReplacement(XamlNodeType operationType);
    bool ShouldSkipForResourceReplacement() const   {  return m_skipStage != spsNotSkipping;  }
    _Check_return_ HRESULT InitiatePropertyReplacementIfNeeded(const std::shared_ptr<XamlProperty>& spXamlProperty);
    _Check_return_ HRESULT ApplyRemainingResourceProperties();
    _Check_return_ HRESULT GetResourcePropertyBag();

    bool IsSkippingForConditionalScope() const;

    _Check_return_ HRESULT Logic_InitializeCollectionFromString(_In_ const std::shared_ptr<XamlType>& memberType);
    _Check_return_ HRESULT TypeConvertStringAndAddToCollectionOnCurrentInstance(
        _In_ const std::shared_ptr<XamlType>& collectionItemType,
        _In_ const std::shared_ptr<XamlTextSyntax>& collectionspTextSyntax,
        _In_ xstring_ptr& trimmedObjectString);
};

