// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ObjectWriterErrorService.h"
#include <ObjectWriterContext.h>

_Check_return_ HRESULT ObjectWriterErrorService::GetErrorService(
    _In_ std::shared_ptr<ParserErrorReporter>& outErrorService) const
{
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(m_spContext->get_SchemaContext(schemaContext));
    IFC_RETURN(schemaContext->GetErrorService(outErrorService));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportError(
    _In_ const HRESULT errorCode,
    _In_ const XamlLineInfo& inLineInfo)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, inLineInfo.LineNumber(), inLineInfo.LinePosition()));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportError(
    _In_ const HRESULT errorCode,
    _In_ const XamlLineInfo& inLineInfo,
    _In_ const xstring_ptr& inssParam1)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, inLineInfo.LineNumber(), inLineInfo.LinePosition(), inssParam1));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportError(
    _In_ const HRESULT errorCode,
    _In_ const XamlLineInfo& inLineInfo,
    _In_ const xstring_ptr& inssParam1,
    _In_ const xstring_ptr& inssParam2)
{
    std::shared_ptr<ParserErrorReporter> errorService;

    IFC_RETURN(GetErrorService(errorService));
    IFC_RETURN(errorService->SetError(errorCode, inLineInfo.LineNumber(), inLineInfo.LinePosition(), inssParam1, inssParam2));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::GetReportedError(
    _Out_ std::shared_ptr<ParserErrorReporter>& spParserErrorReporter,
    _Outptr_result_maybenull_ IErrorService **ppErrorService,
    _Outptr_result_maybenull_ IError **ppLastError)
{
    std::shared_ptr<XamlSchemaContext> spSchemaContext;

    IFC_RETURN(m_spContext->get_SchemaContext(spSchemaContext));
    CCoreServices* pCore = spSchemaContext->GetCore();

    IFC_RETURN(pCore->getErrorService(ppErrorService));
    IFC_RETURN(GetErrorService(spParserErrorReporter));
    if (*ppErrorService && spParserErrorReporter)
    {
        // Get the last error raised
        IFC_RETURN((*ppErrorService)->GetFirstError(ppLastError));
    }

    return S_OK;
}

// Check for an inner error created by CErrorService::RaiseInvalidOperation in
// other code outisde the parser and wrap it in a parser error with the
// appropriate context.  This will return E_FAIL if it's propagating the error.
_Check_return_ HRESULT ObjectWriterErrorService::WrapErrorWithParserErrorAndRethrow(
    _In_ const HRESULT errorCode,
    _In_ const XamlLineInfo& lineInfo)
{
    // Only try to rethrow if there was a failure
    if (FAILED(errorCode))
    {
        IErrorService *pErrorService = NULL;
        std::shared_ptr<ParserErrorReporter> spParserErrorReporter;
        IError* pLastError = NULL;

        IFC_RETURN(GetReportedError(spParserErrorReporter, &pErrorService, &pLastError));
        if (spParserErrorReporter && pErrorService && pLastError)
        {
            xstring_ptr spErrorMessage;
            XUINT32 uiErrorCode = 0;

            // Get the message and error code from the last error
            IFC_RETURN(pLastError->GetErrorMessage(&spErrorMessage));
            uiErrorCode = pLastError->GetErrorCode();

            // Make sure there's an error message to wrap
            if (!spErrorMessage.IsNullOrEmpty())
            {
                XUINT32 bRecoverable = FALSE;
                bRecoverable = pLastError->GetIsRecoverable();

                // Delete the last error
                IFC_RETURN(pErrorService->CleanupLastError(pLastError));

                // Since we just removed the last error, we're going to clear
                // the IsErrorRecorded flag
                spParserErrorReporter->SetIsErrorRecorded(FALSE);

                // Report a new parse error using the info from the last
                // error
                IFC_RETURN(spParserErrorReporter->SetErrorWithMessage(uiErrorCode, lineInfo.LineNumber(), lineInfo.LinePosition(), bRecoverable, spErrorMessage));
                spParserErrorReporter->SetIsErrorRecorded(TRUE);

                // Propagate the failure and prevent from stamping another stowed using notrace
                IFC_NOTRACE_RETURN(errorCode);
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportSetValueError(
    _In_ const HRESULT errorResult,
    _In_ const XamlLineInfo& inLineInfo,
    _In_ const xstring_ptr& inssPropertyName,
    _In_ const xstring_ptr& inssValue)
{
    switch (errorResult)
    {
    case E_SV_NO_EVENT_ROOT:
        IFC_RETURN(ReportError(AG_E_PARSER2_NO_EVENT_ROOT, inLineInfo, inssPropertyName));
        break;
    default:
        IFC_RETURN(ReportError(AG_E_PARSER2_OW_CANT_SET_VALUE_GENERIC, inLineInfo, inssPropertyName));
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportBadObjectWriterState(
    _In_ const XamlLineInfo& inLineInfo)
{
    // Raise a generic error like "Parser internal error: Object
    // writer '%0'."
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorType, L"BadStateObjectWriter");
    IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_GENERIC, inLineInfo, c_strErrorType));
    IFC_RETURN(E_UNEXPECTED);

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportDictionaryItemMustHaveKey(
    _In_ const XamlLineInfo& inLineInfo)
{
    IFC_RETURN(ReportError(AG_E_PARSER2_OW_ITEMS_IN_DICTIONARY_MUST_HAVE_KEY, inLineInfo));
    IFC_RETURN(E_FAIL);

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportDeferredElementMustHaveName(
    _In_ const XamlLineInfo& inLineInfo)
{
    IFC_RETURN(ReportError(AG_E_PARSER2_DEFERRED_ELEMENT_MUST_HAVE_NAME, inLineInfo));
    IFC_RETURN(E_FAIL);

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportDeferredElementCannotBeRoot(
    _In_ const XamlLineInfo& inLineInfo)
{
    IFC_RETURN(ReportError(AG_E_PARSER2_DEFERRED_ELEMENT_CANNOT_BE_ROOT, inLineInfo));
    IFC_RETURN(E_FAIL);

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportInvalidDeferLoadStrategyValue(
    _In_ const XamlLineInfo& inLineInfo)
{
    IFC_RETURN(ReportError(AG_E_PARSER2_INVALID_DEFERLOADSTRATEGY_VALUE, inLineInfo));
    IFC_RETURN(E_FAIL);
    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportInvalidRealizeValue(
    _In_ const XamlLineInfo& inLineInfo)
{
    IFC_RETURN(ReportError(AG_E_PARSER2_INVALID_REALIZE_VALUE, inLineInfo));
    IFC_RETURN(E_FAIL);
    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ReportInvalidUseOfSetter_Property(
    _In_ const XamlLineInfo& lineInfo)
{
    IFC_RETURN(ReportError(AG_E_PARSER2_INVALID_USE_OF_SETTER_PROPERTY, lineInfo));
    IFC_RETURN(E_FAIL);
    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ValidateFrameForPropertySet(
    _In_ const ObjectWriterFrame& frame,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const XamlLineInfo& inLineInfo)
{
    if (spProperty->IsDirective())
    {
        auto directiveProperty = std::static_pointer_cast<DirectiveProperty>(spProperty);
        if (frame.contains_Directive(directiveProperty))
        {
            // Raise a generic error like "Parser internal error:
            // Object writer '%0'."
            DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorType, L"ParentContainsKey");
            IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_GENERIC, inLineInfo, c_strErrorType));
            IFC_RETURN(E_UNEXPECTED);
        }
    }
    else
    {
        bool isPropertyAssigned = false;
        isPropertyAssigned = frame.get_IsPropertyAssigned(spProperty);
        if (isPropertyAssigned)
        {
            // TODO: Future versions of Windows should fail when events are used in markup at runtime.
            // We don't support events at runtime anymore, so just skip.
            if (!spProperty->IsEvent())
            {
                // If the property is being set twice, raise an error like "The property
                // '%0' is set more than once."
                xstring_ptr ssName;
                IFC_RETURN(spProperty->get_FullName(&ssName));
                IFC_RETURN(ReportError(AG_E_PARSER2_OW_DUPLICATE_MEMBER, inLineInfo, ssName));
                IFC_RETURN(E_FAIL);
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ValidateXClassMustBeOnRoot(
    _In_ const XINT32 liveDepth,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spRootInstance,
    _In_ const XamlLineInfo& inLineInfo)
{
    if (m_isEncoding)
    {
        // we cannot validate this scenario during encoding because
        // spRootInstance is not set. The encoder tries to cater to
        // the case where the fragment is loaded in without a provided root
        // to ensure we emit a CreateType node.
        return S_OK;
    }

    if (!spRootInstance)
    {
        // Raise a generic error like "Parser internal
        // error: Object writer '%0'."
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorType, L"xClassCanOnlyBeUsedOnLoadComponent");
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_GENERIC, inLineInfo, c_strErrorType));
        IFC_RETURN(E_UNEXPECTED);
    }
    else if (liveDepth != 2)
    {
        // Raise a generic error like "The attribute
        // 'Class' from the XAML namespace is only
        // accepted on the root element."
        IFC_RETURN(ReportError(AG_E_PARSER2_OW_X_CLASS_MUST_BE_ON_ROOT, inLineInfo));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ValidateFrameForWriteValue(
    _In_ const ObjectWriterFrame& frame,
    _In_ const XamlLineInfo& inLineInfo)
{
    if (!frame.exists_Member())
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"PropertyNullInWriteValue");
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_GENERIC, inLineInfo, c_strErrorMessage));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ValidateFrameForConstructType(
    _In_ const ObjectWriterFrame& frame,
    _In_ const XamlLineInfo& inLineInfo)
{
    if (frame.get_IsObjectFromMember())
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorType, L"ConstructImplicitType");
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_GENERIC, inLineInfo, c_strErrorType));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterErrorService::ValidateFrameForCollectionAdd(
    _In_ const ObjectWriterFrame& frame,
    _In_ const XamlLineInfo& inLineInfo)
{
    if (m_isEncoding)
    {
        // since the encoder will insert a dummy XQO object, the IsNull check will
        // succeed and make the object writer encoder think that we are in a bad
        // state and cannot continue. However, the encoder will not really perform
        // a collection item add so we are safe to let the collection itself be
        // a fake one.
        return S_OK;
    }

    // ObjectWriter will store the collection in the 'Collection' property of the frame, but
    // BinaryFormatObjectWriter sticks it into the 'Instance' property
    const auto& collection = frame.exists_Collection() ? frame.get_Collection() : frame.get_Instance();

    if (collection->GetValue().IsNull())
    {
        // Raise an error like "Collection property '%0' is null."
        xstring_ptr spMemberName;
        IFC_RETURN(frame.get_Member()->get_FullName(&spMemberName));
        IFC_RETURN(ReportError(AG_E_PARSER2_OW_COLLECTION_NULL, inLineInfo, spMemberName));
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

template<>
_Check_return_ HRESULT ObjectWriterErrorService::ValidateIsKnown<XamlType>(
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const XamlLineInfo& inLineInfo)
{
    if (!spType)
    {
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_UNKNOWN_TYPE, inLineInfo, xstring_ptr::EmptyString()));
        IFC_RETURN(E_UNEXPECTED);
    }
    else if (spType->IsUnknown())
    {
        xstring_ptr ssName;
        IFC_RETURN(spType->get_Name(&ssName));
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_UNKNOWN_TYPE, inLineInfo, ssName));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

template<>
_Check_return_ HRESULT ObjectWriterErrorService::ValidateIsKnown<XamlProperty>(
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const XamlLineInfo& inLineInfo)
{
    if (!spProperty)
    {
        IFC_RETURN(ReportError(AG_E_PARSER2_UNKNOWN_PROP_ON_TYPE, inLineInfo, xstring_ptr::EmptyString(), xstring_ptr::EmptyString()));
        IFC_RETURN(E_UNEXPECTED);
    }
    else if (spProperty->IsUnknown())
    {
        xstring_ptr propertyName;
        IFC_RETURN(spProperty->get_Name(&propertyName));

        std::shared_ptr<XamlType> declaringType;
        IFC_RETURN(spProperty->get_DeclaringType(declaringType));
        xstring_ptr declaringTypeName;
        IFC_RETURN(declaringType->get_Name(&declaringTypeName));

        IFC_RETURN(ReportError(AG_E_PARSER2_UNKNOWN_PROP_ON_TYPE, inLineInfo, propertyName, declaringTypeName));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

template<>
_Check_return_ HRESULT ObjectWriterErrorService::ValidateIsKnown<ImplicitProperty>(
    _In_ const std::shared_ptr<ImplicitProperty>& spImplicitProperty,
    _In_ const XamlLineInfo& inLineInfo)
{
    // we aren't really validating the unknown flag

    if (!spImplicitProperty ||
        (spImplicitProperty->get_ImplicitPropertyType() != iptInitialization
        && spImplicitProperty->get_ImplicitPropertyType() != iptItems))
    {
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strErrorMessage, L"MissingImplicitPropertyTypeCase");
        IFC_RETURN(ReportError(AG_E_PARSER2_INTERNAL_OW_GENERIC, inLineInfo, c_strErrorMessage));
        IFC_RETURN(E_UNEXPECTED);
    }

    return S_OK;
}

