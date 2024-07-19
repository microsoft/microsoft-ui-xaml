// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ICustomWriter.h"
#include <ICustomWriterActivator.h>
#include <XamlWriter.h>

struct ICustomWriterCallbacks;
class DeferredElementCustomRuntimeData;
class ObjectWriterContext;
class ObjectWriterStack;
class ObjectWriterErrorService;

namespace Parser
{
struct XamlPredicateAndArgs;
}

class DeferredElementCustomWriter
    : public ICustomWriter
{
public:
    DeferredElementCustomWriter(
        _In_ ICustomWriterCallbacks* callbacks,
        _In_ std::shared_ptr<ObjectWriterContext> spContext);

    // XamlWriter
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

    // ICustomWriter
    bool IsCustomWriterFinished() const override;

    bool ShouldAllowNewCustomWriter(_In_ const std::shared_ptr<XamlType>& spXamlType) override
    {
        return m_allowNewCustomWriter;
    }

private:
    static bool IsNonDeferredProperty(
        _In_ const std::shared_ptr<XamlProperty>& prop);

    ICustomWriterCallbacks* m_customWriterCallbacks;
    std::shared_ptr<ObjectWriterContext> m_spContext;
    std::shared_ptr<XamlProperty> m_currentDeferredElementXamlProperty;
    std::unique_ptr<DeferredElementCustomRuntimeData> m_runtimeData;
    unsigned m_stackDepth;
    bool m_allowNewCustomWriter;

    enum class Operation
    {
        None,
        WritingDeferredElement,
        WritingDeferredElementXName,
        WritingNonDeferredProperty,
        WritingDeferLoadStrategyDirective,
        WritingRealizeDirective
    };

    std::vector<std::pair<unsigned int, Operation>> m_pendingOperations;
};

class DeferredElementCustomWriterActivator
    : public ICustomWriterActivator
{
public:
    DeferredElementCustomWriterActivator(
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
    _Check_return_ HRESULT ValidateAndGetDeferLoadStrategyValue(
        _In_ const xstring_ptr& strValue,
        _Out_ bool* pResult);

    _Check_return_ HRESULT ValidateAndGetRealizeValue(
        _In_ const xstring_ptr& strValue,
        _Out_ bool* pResult);

    _Check_return_ HRESULT ValidateNotRootElement();

    std::shared_ptr<ObjectWriterContext> m_spContext;
};
