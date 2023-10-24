// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ICustomWriter.h"
#include "ICustomWriterActivator.h"
#include <StreamOffsetToken.h>

class CCoreServices;
class DirectiveProperty;
class StreamOffsetToken;
class StyleCustomRuntimeData;
class StyleSetterEssence;
class XamlSchemaContext;
enum class KnownPropertyIndex : UINT16;
enum class KnownTypeIndex : UINT16;
struct ICustomWriterCallbacks;
struct XamlTypeToken;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// A CustomWriter for optimizing Style.
class StyleCustomWriter final
    : public ICustomWriter
{
private:
    StyleCustomWriter(
        _In_ _Notnull_ ICustomWriterCallbacks* callbacks,
        _In_ std::shared_ptr<XamlSchemaContext> context,
        _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext);

    _Check_return_ HRESULT Initialize();

public:
    static _Check_return_ HRESULT Create(
        _In_ _Notnull_ ICustomWriterCallbacks* callbacks,
        _In_ std::shared_ptr<XamlSchemaContext> context,
        _In_ std::shared_ptr<ObjectWriterContext> objectWriterContext,
        _Out_ std::unique_ptr<ICustomWriter>* ppValue);

    // ICustomWriter
    _Check_return_ HRESULT WriteObject(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ bool bFromMember,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteEndObject(_Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteMember(
        _In_ const std::shared_ptr<XamlProperty>& inProperty,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteEndMember(_Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteConditionalScope(_In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs, _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteEndConditionalScope(_Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT WriteNamespace(
        _In_ const xstring_ptr& spPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spXamlNamespace,
        _Out_ bool* pResult) override;

    bool IsCustomWriterFinished() const override;

    bool ShouldAllowNewCustomWriter(_In_ const std::shared_ptr<XamlType>& spXamlType) override
    {
        return m_allowNewCustomWriter;
    }

    static bool IsProperty(_In_ _Const_ const std::shared_ptr<XamlProperty>& spProperty, KnownPropertyIndex propIndex);
    static bool IsType(_In_ _Const_ const std::shared_ptr<XamlType>& spType, KnownTypeIndex typeIndex);
    static bool IsType(_In_ _Const_ const XamlTypeToken& typeToken, KnownTypeIndex typeIndex);

private:

    ICustomWriterCallbacks* m_customWriterCallbacks;
    std::unique_ptr<StyleCustomRuntimeData> m_runtimeData;
    std::shared_ptr<XamlSchemaContext> m_context;
    std::shared_ptr<ObjectWriterContext> m_objectWriterContext;
    CCoreServices* m_core;
    bool m_allowNewCustomWriter;

    std::shared_ptr<XamlType> m_targetXamlType;
    std::shared_ptr<XamlProperty> m_currentStyleXamlProperty;

    struct setterInfo
    {
        StreamOffsetToken offsetToken;
        bool shouldOptimize = true;
        bool isMutable = false;
        void reset() { shouldOptimize = true; isMutable = false; }
    } m_currentSetterInfo;

    // The current operation is pushed onto the pendingOperations vector below
    // along with the current depth in the node tree. As we encounter Begin/End
    // pairs we maintain a depth to sense how deep we are in the XAML parse tree.
    // As we encounter certain TypeTokens we change which operation we are in. The
    // current operation is the way we store state between XamlWriter calls.
    enum class Operation {
        WritingStyle,
        WritingStyleXKey,
        WritingStyleXName,
        WritingStyleTargetType,
        WritingStyleBasedOn,
        WritingStyleBasedOnObject,
        WritingStyleSetters,
        WritingStyleUnknownProperty,
        WritingSetterBaseCollection,
        WritingSetter,
        WritingSetterDirective,
        WritingSetterProperty,
        WritingSetterTarget,
        WritingSetterValue,
        WritingSetterValueObject,
    };

    void GetStringValue(_In_ _Const_ const std::shared_ptr<XamlQualifiedObject>& spValue, _In_ xstring_ptr& stringValue);
    _Check_return_ HRESULT ResolveSetterValue(StyleSetterEssence& setter);
    bool ShouldHandleOperation(_In_ Operation operation);

    std::vector<std::pair<unsigned int, Operation>> m_pendingOperations;

    // The number of objects deep we are relevative to the
    // first StartObject that was pushed into this writer.
    unsigned int m_stackDepth;

    std::shared_ptr<DirectiveProperty> m_spXKeyProperty;
    std::shared_ptr<DirectiveProperty> m_spXNameProperty;
};

class StyleCustomWriterActivator
    : public ICustomWriterActivator
{
public:
    StyleCustomWriterActivator(
        _In_ const std::shared_ptr<ObjectWriterContext>& spContext);

    _Check_return_ HRESULT ShouldActivateCustomWriter(
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ bool* pResult) override;

    _Check_return_ HRESULT CreateNodeStreamCollectingWriter(
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ std::shared_ptr<ObjectWriter>* pWriter) override;

    _Check_return_ HRESULT CreateCustomWriter(
        _In_ ICustomWriterCallbacks* pCallbacks,
        _In_ const CustomWriterActivatorState& activatorState,
        _Out_ std::unique_ptr<ICustomWriter>* pWriter) override;

private:
    std::shared_ptr<ObjectWriterContext> m_spContext;
};
