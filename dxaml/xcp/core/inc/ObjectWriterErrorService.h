// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xstring_ptr.h"
#include "LineInfo.h"

class ParserErrorReporter;
class XamlSchemaContext;
class XamlType;
class XamlProperty;
struct IErrorService;
struct IError;

// An error service for the ObjectWriter that provides
// helpers for reporting parse specific errors with rich
// error information and context.
class ObjectWriterErrorService
{
public:
    ObjectWriterErrorService(_In_ const std::shared_ptr<ObjectWriterContext>& spContext, _In_ const bool isEncoding) 
        : m_spContext(spContext)
        , m_isEncoding(isEncoding)
    {}

    _Check_return_ HRESULT GetErrorService(
        _In_ std::shared_ptr<ParserErrorReporter>& outErrorService) const;

    _Check_return_ HRESULT ReportError(
        _In_ const HRESULT errorCode, 
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ReportError(
        _In_ const HRESULT errorCode, 
        _In_ const XamlLineInfo& inLineInfo, 
        _In_ const xstring_ptr& inssParam1);

    _Check_return_ HRESULT ReportError(
        _In_ const HRESULT errorCode, 
        _In_ const XamlLineInfo& inLineInfo, 
        _In_ const xstring_ptr& inssParam1, 
        _In_ const xstring_ptr& inssParam2);

    _Check_return_ HRESULT GetReportedError(
        _Out_ std::shared_ptr<ParserErrorReporter>& spParserErrorReporter,
        _Outptr_result_maybenull_ IErrorService **ppErrorService,
        _Outptr_result_maybenull_ IError **ppLastError);

    _Check_return_ HRESULT WrapErrorWithParserErrorAndRethrow(
        _In_ const HRESULT errorCode,
        _In_ const XamlLineInfo& lineInfo);

    _Check_return_ HRESULT ReportSetValueError(
        _In_ const HRESULT errorResult,
        _In_ const XamlLineInfo& inLineInfo,
        _In_ const xstring_ptr& inssPropertyName,
        _In_ const xstring_ptr& inssValue);

    _Check_return_ HRESULT ReportBadObjectWriterState(
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ReportDictionaryItemMustHaveKey(
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ReportDeferredElementMustHaveName(
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ReportDeferredElementCannotBeRoot(
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ReportInvalidDeferLoadStrategyValue(
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ReportInvalidRealizeValue(
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ReportInvalidUseOfSetter_Property(
        _In_ const XamlLineInfo& lineInfo);

    // Returns a failing HRESULT for an unknown type or property. This
    // possibly could be made into an ASSERT in the future once we
    // verify that this is caught elsewhere in the stack.
    // We make this a template class because even though a lot of the
    // Xaml schema objects have a IsUnknown property it's not part
    // of a common interface... =(
    template<class T>
    _Check_return_ HRESULT ValidateIsKnown(         
        _In_ const std::shared_ptr<T>& spValue,
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ValidateFrameForWriteValue(
        _In_ const ObjectWriterFrame& frame,
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ValidateFrameForConstructType(
        _In_ const ObjectWriterFrame& frame,
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ValidateFrameForPropertySet(
        _In_ const ObjectWriterFrame& frame,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ValidateFrameForCollectionAdd(
        _In_ const ObjectWriterFrame& frame,
        _In_ const XamlLineInfo& inLineInfo);

    _Check_return_ HRESULT ValidateXClassMustBeOnRoot(
        _In_ const XINT32 liveDepth,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spRootInstance,
        _In_ const XamlLineInfo& inLineInfo);

private:
    std::shared_ptr<ObjectWriterContext> m_spContext;
    bool m_isEncoding;
};

template<>
_Check_return_ HRESULT ObjectWriterErrorService::ValidateIsKnown<XamlType>(    
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const XamlLineInfo& inLineInfo);

template<>
_Check_return_ HRESULT ObjectWriterErrorService::ValidateIsKnown<XamlProperty>(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const XamlLineInfo& inLineInfo);

template<>
_Check_return_ HRESULT ObjectWriterErrorService::ValidateIsKnown<ImplicitProperty>(
    _In_ const std::shared_ptr<ImplicitProperty>& spImplicitProperty,
    _In_ const XamlLineInfo& inLineInfo);

