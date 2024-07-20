// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <nserror.h>
#include <runtimeErrorEventArgs.h>
#include <xmllite.h>

#define  NS_E_MEDIA_WSAENOTSOCK     0x80072736     // winerror.h keeps only Low Word of the error code, so define the full error code here.

//
// Keep a table of the known mapping between HRESULT and AG ErrorCode.
//

const HResultToECodePair eResult_eCode_Table[] =
{
    {E_UNEXPECTED,                     AG_E_UNKNOWN_ERROR},
    {E_NOTIMPL,                        AG_E_UNKNOWN_ERROR},
    {E_OUTOFMEMORY,                    AG_E_UNKNOWN_ERROR},
    {E_INVALIDARG,                     AG_E_INVALID_ARGUMENT},
    {E_NOINTERFACE,                    AG_E_UNKNOWN_ERROR},
    {E_POINTER,                        AG_E_UNKNOWN_ERROR},
    {E_HANDLE,                         AG_E_UNKNOWN_ERROR},
    {E_ABORT,                          AG_E_UNKNOWN_ERROR},
    {E_FAIL,                           AG_E_UNKNOWN_ERROR},
    {E_ACCESSDENIED,                   AG_E_UNKNOWN_ERROR},
    {E_PENDING,                        AG_E_UNKNOWN_ERROR},
    {E_NOT_SUPPORTED,                  AG_E_UNKNOWN_ERROR}, 

    {E_HTMLACCESS_DENIED,              AG_E_RUNTIME_HTML_ACCESS_RESTRICTED},

    // Broadcast error mapping
    {NS_E_SERVER_NOT_FOUND,            AG_E_NOT_FOUND},
    {NS_E_SERVER_DNS_TIMEOUT,          AG_E_NOT_FOUND},
    {NS_E_SERVER_UNAVAILABLE,          AG_E_NOT_FOUND},
    {NS_E_NOCONNECTION,                AG_E_NOT_FOUND},
    {NS_E_CANNOTCONNECT,               AG_E_NOT_FOUND},
    {NS_E_TIMEOUT,                     AG_E_NOT_FOUND},
    {NS_E_BAD_MULTICAST_ADDRESS,       AG_E_NOT_FOUND},
    {NS_E_INVALID_STREAM,              AG_E_NOT_FOUND},
    {NS_E_INVALID_PORT,                AG_E_NOT_FOUND},
    {NS_E_INSUFFICIENT_BANDWIDTH,      AG_E_UNABLE_TO_PLAY},      
    {NS_E_PROTOCOL_MISMATCH,           AG_E_UNABLE_TO_PLAY},
    {NS_E_NETWORK_SERVICE_FAILURE,     AG_E_UNABLE_TO_PLAY},
    {NS_E_NETWORK_RESOURCE_FAILURE,    AG_E_UNABLE_TO_PLAY},
    {NS_E_CONNECTION_FAILURE,          AG_E_UNABLE_TO_PLAY},
    {NS_E_SHUTDOWN,                    AG_E_UNABLE_TO_PLAY},
    {NS_E_INVALID_REQUEST,             AG_E_UNABLE_TO_PLAY},
    {NS_E_INVALID_DATA,                AG_E_UNABLE_TO_PLAY},
    {NS_E_STRIDE_REFUSED,              AG_E_UNABLE_TO_PLAY},
    {NS_E_MAX_BITRATE,                 AG_E_UNABLE_TO_PLAY},
    {NS_E_MAX_CLIENTS,                 AG_E_UNABLE_TO_PLAY},
    {NS_E_UNRECOGNIZED_STREAM_TYPE,    AG_E_INVALID_FILE_FORMAT},
    {NS_E_INCOMPATIBLE_FORMAT,         AG_E_INVALID_FILE_FORMAT},
    {NS_E_MEDIA_WSAENOTSOCK,           AG_E_MEDIA_DISCONNECTED},

    // DO error codes
    {E_DO_RESOURCE_NAMENOTSET,         AG_E_PARSER_RESOURCE_NAMENOTSET},
    {E_DO_RESOURCE_KEYCONFLICT,        AG_E_PARSER_RESOURCE_KEYCONFLICT},
    {E_DO_RESOURCE_KEYANDNAMESET,      AG_E_PARSER_RESOURCE_KEYANDNAMESET},

    // CLR error codes
    {CLR_E_RUNTIME_MANAGED_UNKNOWN_ERROR,       AG_E_RUNTIME_MANAGED_UNKNOWN_ERROR},
    {CLR_E_RUNTIME_MANAGED_ACTIVATION,          AG_E_RUNTIME_MANAGED_ACTIVATION},
    {CLR_E_RUNTIME_MANAGED_ASSEMBLY_DOWNLOAD,   AG_E_RUNTIME_MANAGED_ASSEMBLY_DOWNLOAD},
    {CLR_E_RUNTIME_MANAGED_ASSEMBLY_LOAD,       AG_E_RUNTIME_MANAGED_ASSEMBLY_LOAD},
    {CLR_E_RUNTIME_MANAGED_ASSEMBLY_NOT_FOUND,  AG_E_RUNTIME_MANAGED_ASSEMBLY_NOT_FOUND},
    {CLR_E_PARSER_UNKNOWN_TYPE,                 AG_E_PARSER_UNKNOWN_TYPE},
    {CLR_E_PARSER_BAD_TYPE,                     AG_E_PARSER_BAD_TYPE},
    {CLR_E_PARSER_BAD_NATIVE_TYPE,              AG_E_PARSER_BAD_NATIVE_TYPE},
    {CLR_E_PARSER_CREATE_OBJECT_FAILED,         AG_E_PARSER_CREATE_OBJECT_FAILED},
    {CLR_E_PARSER_PROPERTY_NOT_FOUND,           AG_E_PARSER_PROPERTY_NOT_FOUND},
    {CLR_E_PARSER_BAD_PROPERTY_TYPE,            AG_E_PARSER_BAD_PROPERTY_TYPE},
    {CLR_E_PARSER_BAD_PROPERTY_VALUE,           AG_E_PARSER_BAD_PROPERTY_VALUE},
    {CLR_E_PARSER_ROOT_NOT_CUSTOM,              AG_E_PARSER_ROOT_NOT_CUSTOM},
    {CLR_E_PARSER_NAMESPACE_NOT_SUPPORTED,      AG_E_PARSER_NAMESPACE_NOT_SUPPORTED},
    {CLR_E_PARSER_MISSING_DEFAULT_NAMESPACE,    AG_E_PARSER_MISSING_DEFAULT_NAMESPACE},
    {CLR_E_PARSER_INVALID_XMLNS,                AG_E_PARSER_INVALID_XMLNS},
    {CLR_E_PARSER_INVALID_CLASS,                AG_E_PARSER_INVALID_CLASS},
    {CLR_E_PARSER_INVALID_CONTENT,              AG_E_PARSER_INVALID_CONTENT},

    // XmlLite parser codes

    {MX_E_INPUTEND,            XMLLITE_MX_E_INPUTEND},
    {MX_E_ENCODING,            XMLLITE_MX_E_ENCODING},
    {MX_E_ENCODINGSWITCH,      XMLLITE_MX_E_ENCODINGSWITCH},
    {MX_E_ENCODINGSIGNATURE,   XMLLITE_MX_E_ENCODINGSIGNATURE},
    {WC_E_WHITESPACE,          XMLLITE_WC_E_WHITESPACE},
    {WC_E_SEMICOLON,           XMLLITE_WC_E_SEMICOLON},
    {WC_E_GREATERTHAN,         XMLLITE_WC_E_GREATERTHAN},
    {WC_E_QUOTE,               XMLLITE_WC_E_QUOTE},
    {WC_E_EQUAL,               XMLLITE_WC_E_EQUAL},
    {WC_E_LESSTHAN,            XMLLITE_WC_E_LESSTHAN},
    {WC_E_HEXDIGIT,            XMLLITE_WC_E_HEXDIGIT},
    {WC_E_DIGIT,               XMLLITE_WC_E_DIGIT},
    {WC_E_LEFTBRACKET,         XMLLITE_WC_E_LEFTBRACKET},
    {WC_E_LEFTPAREN,           XMLLITE_WC_E_LEFTPAREN},
    {WC_E_XMLCHARACTER,        XMLLITE_WC_E_XMLCHARACTER},
    {WC_E_NAMECHARACTER,       XMLLITE_WC_E_NAMECHARACTER},
    {WC_E_SYNTAX,              XMLLITE_WC_E_SYNTAX},
    {WC_E_CDSECT,              XMLLITE_WC_E_CDSECT},
    {WC_E_COMMENT,             XMLLITE_WC_E_COMMENT},
    {WC_E_CONDSECT,            XMLLITE_WC_E_CONDSECT},
    {WC_E_DECLATTLIST,         XMLLITE_WC_E_DECLATTLIST},
    {WC_E_DECLDOCTYPE,         XMLLITE_WC_E_DECLDOCTYPE},
    {WC_E_DECLELEMENT,         XMLLITE_WC_E_DECLELEMENT},
    {WC_E_DECLENTITY,          XMLLITE_WC_E_DECLENTITY},
    {WC_E_DECLNOTATION,        XMLLITE_WC_E_DECLNOTATION},
    {WC_E_NDATA,               XMLLITE_WC_E_NDATA},
    {WC_E_PUBLIC,              XMLLITE_WC_E_PUBLIC},
    {WC_E_SYSTEM,              XMLLITE_WC_E_SYSTEM},
    {WC_E_NAME,                XMLLITE_WC_E_NAME},
    {WC_E_ROOTELEMENT,         XMLLITE_WC_E_ROOTELEMENT},
    {WC_E_ELEMENTMATCH,        XMLLITE_WC_E_ELEMENTMATCH},
    {WC_E_UNIQUEATTRIBUTE,     XMLLITE_WC_E_UNIQUEATTRIBUTE},
    {WC_E_TEXTXMLDECL,         XMLLITE_WC_E_TEXTXMLDECL},
    {WC_E_LEADINGXML,          XMLLITE_WC_E_LEADINGXML},
    {WC_E_TEXTDECL,            XMLLITE_WC_E_TEXTDECL},
    {WC_E_XMLDECL,             XMLLITE_WC_E_XMLDECL},
    {WC_E_ENCNAME,             XMLLITE_WC_E_ENCNAME},
    {WC_E_PUBLICID,            XMLLITE_WC_E_PUBLICID},
    {WC_E_PESINTERNALSUBSET,   XMLLITE_WC_E_PESINTERNALSUBSET},
    {WC_E_PESBETWEENDECLS,     XMLLITE_WC_E_PESBETWEENDECLS},
    {WC_E_NORECURSION,         XMLLITE_WC_E_NORECURSION},
    {WC_E_ENTITYCONTENT,       XMLLITE_WC_E_ENTITYCONTENT},
    {WC_E_UNDECLAREDENTITY,    XMLLITE_WC_E_UNDECLAREDENTITY},
    {WC_E_PARSEDENTITY,        XMLLITE_WC_E_PARSEDENTITY},
    {WC_E_NOEXTERNALENTITYREF, XMLLITE_WC_E_NOEXTERNALENTITYREF},
    {WC_E_PI,                  XMLLITE_WC_E_PI},
    {WC_E_SYSTEMID,            XMLLITE_WC_E_SYSTEMID},
    {WC_E_QUESTIONMARK,        XMLLITE_WC_E_QUESTIONMARK},
    {WC_E_CDSECTEND,           XMLLITE_WC_E_CDSECTEND},
    {WC_E_MOREDATA,            XMLLITE_WC_E_MOREDATA},
    {WC_E_DTDPROHIBITED,       XMLLITE_WC_E_DTDPROHIBITED},
    {NC_E_QNAMECHARACTER,      XMLLITE_NC_E_QNAMECHARACTER},
    {NC_E_QNAMECOLON,          XMLLITE_NC_E_QNAMECOLON},
    {NC_E_NAMECOLON,           XMLLITE_NC_E_NAMECOLON},
    {NC_E_DECLAREDPREFIX,      XMLLITE_NC_E_DECLAREDPREFIX},
    {NC_E_UNDECLAREDPREFIX,    XMLLITE_NC_E_UNDECLAREDPREFIX},
    {NC_E_EMPTYURI,            XMLLITE_NC_E_EMPTYURI},
    {NC_E_XMLPREFIXRESERVED,   XMLLITE_NC_E_XMLPREFIXRESERVED},
    {NC_E_XMLNSPREFIXRESERVED, XMLLITE_NC_E_XMLNSPREFIXRESERVED},
    {NC_E_XMLURIRESERVED,      XMLLITE_NC_E_XMLURIRESERVED},
    {NC_E_XMLNSURIRESERVED,    XMLLITE_NC_E_XMLNSURIRESERVED},
    {XML_E_INVALID_DECIMAL,    XMLLITE_XML_E_INVALID_DECIMAL},
    {XML_E_INVALID_HEXIDECIMAL,XMLLITE_XML_E_INVALID_HEXIDECIMAL},
    {XML_E_INVALID_UNICODE,    XMLLITE_XML_E_INVALID_UNICODE},
    {XML_E_INVALIDENCODING,    XMLLITE_XML_E_INVALIDENCODING},

    {(HRESULT)WC_E_DTDPROHIBITED,      AG_E_PARSER_NO_DTDS},
    {(HRESULT)WC_E_UNDECLAREDENTITY,   AG_E_PARSER_INVALID_ENTITY_REF},
    {(HRESULT)MX_E_ENCODING,           AG_E_PARSER_INVALID_ENCODING},
    {(HRESULT)WC_E_UNIQUEATTRIBUTE,    AG_E_PARSER_MULT_PROP_VALUES},

    {E_CAPTURE_DEVICE_IN_USE,        AG_E_CAPTURE_DEVICE_IN_USE},
    {E_CAPTURE_DEVICE_REMOVED,       AG_E_CAPTURE_DEVICE_REMOVED},
    {E_CAPTURE_DEVICE_ACCESS_DENIED, AG_E_CAPTURE_DEVICE_ACCESS_DENIED},
    {E_CAPTURE_SOURCE_NOT_STOPPED,   AG_E_CAPTURE_SOURCE_NOT_STOPPED},
    {E_CAPTURE_DEVICE_NOT_AVAILABLE, AG_E_CAPTURE_DEVICE_NOT_AVAILABLE},
    {E_INVALID_APP_SIGNATURE,        AG_E_INVALID_APP_SIGNATURE},
};

//
// Table mapping specific media errors to generic equivalents.  Allows
// us to expose new media errors in MSS scenario without changing others.
//
const ECodePair Media_Specific_to_Generic_Table[] =
{
    {AG_E_ATTRIBUTENOTFOUND,           AG_E_UNABLE_TO_PLAY},
    {AG_E_END_OF_STREAM,               AG_E_UNABLE_TO_PLAY},
    {AG_E_INVALIDINDEX,                AG_E_UNABLE_TO_PLAY},
    {AG_E_INVALIDSTREAMNUMBER,         AG_E_UNABLE_TO_PLAY},
    {AG_E_NO_SAMPLE_DURATION,          AG_E_UNABLE_TO_PLAY},
    {AG_E_NO_SAMPLE_TIMESTAMP,         AG_E_UNABLE_TO_PLAY},
    {AG_E_SHUTDOWN,                    AG_E_UNABLE_TO_PLAY},
    {AG_E_INVALIDMEDIATYPE,            AG_E_INVALID_FILE_FORMAT},
    {AG_E_INVALIDTYPE,                 AG_E_INVALID_FILE_FORMAT},
    {AG_E_INVALID_FORMAT,              AG_E_INVALID_FILE_FORMAT},
    {AG_E_UNSUPPORTED_REPRESENTATION,  AG_E_INVALID_FILE_FORMAT},
    {AG_E_INVALIDREQUEST,              AG_E_NOT_FOUND},
};


//------------------------------------------------------------------------
//  Method:   Create
//
//  Synopsis:
//      Static method to create an instance of ErrorSerivce.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CErrorService::Create(_In_ CCoreServices *pCore, _Outptr_ IErrorService** ppErrorService)
{

    if (!pCore || !ppErrorService)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *ppErrorService = NULL;

    xref_ptr<CErrorService> pErrorService;
    pErrorService.attach(new CErrorService(pCore));

    *ppErrorService = pErrorService.detach();

    return S_OK;
}


// CErrorService Constructor
CErrorService::CErrorService(CCoreServices *pCore)
{
    XCP_WEAK(&m_pCore);
    m_pCore = pCore;
    m_cRef = 1;
    m_pErrorList = NULL;
    m_pListenerList = NULL;
}


// CErrorService Destructor
CErrorService::~CErrorService( )
{
    CleanupErrors();
    CleanupListeners();
}

//------------------------------------------------------------------------
//  Method:   AddRef
//
//  Synopsis:
//      Raises the reference count on the object.
//
//------------------------------------------------------------------------
XUINT32
CErrorService::AddRef()
{
    return PAL_InterlockedIncrement((XINT32 *) &m_cRef);
}


//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Lowers the reference count on the object.  Will delete it when there
// are no remaining references.
//
//------------------------------------------------------------------------
XUINT32
CErrorService::Release()
{
    XUINT32 cRef = PAL_InterlockedDecrement((XINT32 *) &m_cRef);

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}

//-----------------------------------------------------------------------------
//
//  Function: CErrorService::AddListener
//
//  Synopsis:
//        Add a listener to this error service.

//-----------------------------------------------------------------------------
_Check_return_ HRESULT CErrorService::AddListener(_In_ IErrorServiceListener* pListener)
{
    auto lock = m_CSError.lock();

    if(!m_pListenerList)
    {
        m_pListenerList = new CXcpList<IErrorServiceListener>();
    }

    IFC_RETURN(m_pListenerList->Add(pListener));
    AddRefInterface(pListener);

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Function: CErrorService::RemoveListener
//
//  Synopsis:
//        Removes a previously added listener from this error service.

//-----------------------------------------------------------------------------
_Check_return_ HRESULT CErrorService::RemoveListener(_In_ IErrorServiceListener* pListener)
{
    HRESULT hr = S_OK;
    auto lock = m_CSError.lock();

    if(!m_pListenerList)
    {
        goto Cleanup;
    }

    IFC(m_pListenerList->Remove(pListener, FALSE /* bDoDelete */));
    if (S_OK == hr)
    {
        ReleaseInterface(pListener);
    }
    else
    {
        hr = S_OK;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
//  Function: CErrorService::CleanupErrors
//
//  Synopsis:
//      Cleanup the list of error objects.
//
//-----------------------------------------------------------------------------
void CErrorService::CleanupErrors( )
{
    auto lock = m_CSError.lock();

    if (m_pErrorList)
    {   
        m_pErrorList->Clean();
        delete m_pErrorList;
        m_pErrorList = NULL;
    }
}

//-----------------------------------------------------------------------------
//
//  Function: CErrorService::CleanupListeners
//
//  Synopsis:
//      Cleanup the list of error service listeners.
//
//-----------------------------------------------------------------------------
void CErrorService::CleanupListeners( )
{
    auto lock = m_CSError.lock();

    if (m_pListenerList)
    {   
        m_pListenerList->Clean();
        delete m_pListenerList;
        m_pListenerList = NULL;
    }
}

//-----------------------------------------------------------------------------
//
//  Function: CErrorService::CoreResetCleanup
//
//  Synopsis:
//      On resetting core, browserhost will cleanup everything in error
//      service that holds a pointer to core.
//
//-----------------------------------------------------------------------------
void CErrorService::CoreResetCleanup( _In_ CCoreServices* pCore )
{
    auto lock = m_CSError.lock();

    m_pCore = pCore;
    CleanupErrors();
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::GetLastReportedError
//
//  Synopsis:
//       Get the last error in the list of errors.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT  CErrorService::GetLastReportedError(_Outptr_ IError **ppError)
{
    CXcpList<IError>::XCPListNode *pTail = NULL;

    auto lock = m_CSError.lock();

    if(m_pErrorList)
    {
        pTail = m_pErrorList->GetHead();
    }
    
    if(pTail)
    {
        IError *pErrorObject = static_cast<IError *>(pTail->m_pData);

        if(pErrorObject)
        {
            *ppError = pErrorObject;
        }
        else
        {
            IFC_NOTRACE_RETURN(E_FAIL);
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::GetFirstError
//
//  Synopsis:
//       Get the first error in the list of errors.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT  CErrorService::GetFirstError(_Outptr_ IError **ppError)
{
    CXcpList<IError>::XCPListNode *pHead = NULL;

    auto lock = m_CSError.lock();

    if(m_pErrorList)
    {
        pHead = m_pErrorList->GetTail();
    }
    
    if(pHead)
    {
        IError *pErrorObject = static_cast<IError *>(pHead->m_pData);

        if(pErrorObject)
        {
            *ppError = pErrorObject;
        }
        else
        {
            IFC_NOTRACE_RETURN(E_FAIL);
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::CleanupLastError
//
//  Synopsis:
//       Cleanup the first error in the list of errors.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT  CErrorService::CleanupLastError(_In_ IError *pError)
{
    HRESULT hr = S_OK;

    auto lock = m_CSError.lock();

    if(m_pErrorList && pError)
    {
        IFC(m_pErrorList->Remove(pError, 0));
        if (hr == S_OK)
        {
            // only release if found ... S_FALSE means it was not found...
            ReleaseInterface(pError);
        }
    }   

Cleanup:

    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::FormatErrorMessage
//
//  Synopsis:
//      Format error message with parameters.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CErrorService::FormatErrorMessage(
            _In_ const xstring_ptr& strOriginalErrorMessage,
            _Out_ xstring_ptr* pstrFormattedErrorMessage,
            _In_reads_(cParams) const xephemeral_string_ptr* pParams,
            _In_ XUINT32 cParams
            )
{
            HRESULT hr = S_OK;
            WCHAR   *pLeftSplitMessage = NULL;
            XUINT32 cLeftStringLen = 0; 
            WCHAR   *pUpdatedLeftSplitMessage = NULL;  // This is the new LeftSplitMessage which holds the original LeftSplitMessage plus pParams[i]
            XUINT32 cUpdatedLeftStringLen = 0;
            WCHAR   *pWholeMessage = NULL;
            XUINT32 cWholeStringLen = 0;
            WCHAR   *pRemainingMessagePtr = NULL; // A pointer pointing to other text string, there is no need to release this pointer when cleanup.
            WCHAR   *pRemainingMessage = NULL;
            XUINT32  cRemainingStringLen = 0;
    const   WCHAR   *pTempParameter = L"Unknown";
            XUINT32 cTempParameter = 7;
            WCHAR   pSearchString[3] = { L'%', L'0', L'\0' };  // keep string L"%0" in an array, so that it can be updated later.
            XUINT32 i = 0;

    // Check for null values
    if (strOriginalErrorMessage.IsNullOrEmpty())
    {
        IFC(E_INVALIDARG);
    }

    // check for null for output parameters.
    if (pstrFormattedErrorMessage == NULL)
    {
        IFC(E_INVALIDARG);
    }

    //
    // We don't support more than 9 parameters. Bail out if that is not the case.
    //
    IFCEXPECT(cParams < 9);

    // Find the %0 in the string if it exists
    pRemainingMessagePtr = xstrstr(strOriginalErrorMessage.GetBuffer(), pSearchString);

    // If we don't find the string, then bail
    // NOTRACE because this can legitimately happen if the error string doesn't include
    // any placeholders. The caller of FormatErrorMessage handles the failure and just
    // uses the un-formatted version of the string.
    if (!pRemainingMessagePtr)
    {
        IFC_NOTRACE(E_UNEXPECTED);
    }

    // Find the offset of the %0
    cLeftStringLen = (XUINT32)(pRemainingMessagePtr - strOriginalErrorMessage.GetBuffer());
    cRemainingStringLen = strOriginalErrorMessage.GetCount() - cLeftStringLen;

    //
    // The above pRemainingMessagePtr is just a pointer pointing to the remaining part of the origninal message.
    // We need to create a copy of the remaining part and make it independent of the original message.
    // So that the remaining message can be re-used and released independently.
    //
    pRemainingMessage = xstralloc(pRemainingMessagePtr,  cRemainingStringLen);

    // Create a substring from the original error message, starting at the beginning and ending at the index of the %0
    pLeftSplitMessage = xsubstr(strOriginalErrorMessage.GetBuffer(), strOriginalErrorMessage.GetCount(), 0, cLeftStringLen);

    i = 0;

    while((i < cParams) && 
        (pRemainingMessage != NULL))
    {
        // Free the previous allocated memory for pUpdatedLeftSplitMessage.
        xstrfree(pUpdatedLeftSplitMessage);
        pUpdatedLeftSplitMessage = NULL;

        // Concatenate the parameter onto the end of the temporary string
        if(pParams == NULL || pParams[i].IsNullOrEmpty())
        {
            xstrconcat(&pUpdatedLeftSplitMessage,
                       &cUpdatedLeftStringLen,
                       pLeftSplitMessage,
                       (XUINT32)cLeftStringLen,
                       pTempParameter, cTempParameter);
        }
        else
        {
            xstrconcat(&pUpdatedLeftSplitMessage,
                       &cUpdatedLeftStringLen,
                       pLeftSplitMessage,
                       (XUINT32)cLeftStringLen,
                       pParams[i].GetBuffer(),
                       pParams[i].GetCount());
        }



        // Concatenate the remaining message onto the end of the temporary string
        // We substract 2 for the length of %0
        xstrfree(pWholeMessage);
        pWholeMessage = NULL;

        xstrconcat(&pWholeMessage,
                   &cWholeStringLen,
                   pUpdatedLeftSplitMessage,
                   cUpdatedLeftStringLen,
                   pRemainingMessage + 2,
                   cRemainingStringLen - 2);


        // Look for the next parameter
        i++;

        // Break out of the loop if past last params
        if (i >= cParams)
        {
            break;
        }

        pSearchString[1] ++;     // Update the search string to %i

        pRemainingMessagePtr = xstrstr(pWholeMessage, pSearchString);

        if(pRemainingMessagePtr == NULL)
        {
            break;
        }

        // Get new LeftStringLen and RemaingStringLen.
        cLeftStringLen = (XUINT32)(pRemainingMessagePtr - pWholeMessage);
        cRemainingStringLen = cWholeStringLen - cLeftStringLen;

        //
        // The above pRemainingMessagePtr is just a pointer pointing to the remaining part of pWholeMessage.
        // We need to create a copy of the remaining part and make it independent of the pWholeMessage,
        // So that the remaining message can be re-used and released independently.
        //
        xstrfree(pRemainingMessage);
        pRemainingMessage = NULL;

        pRemainingMessage = xstralloc(pRemainingMessagePtr,  cRemainingStringLen);

        // Create a substring from pWholeMessage, starting at the beginning and ending at the index of the %i
        xstrfree(pLeftSplitMessage);
        pLeftSplitMessage = xsubstr(pWholeMessage, cWholeStringLen, 0, cLeftStringLen);
    }

    // Copy the string over to the outgoing parameter (xstralloc Zero Terminates the string)
    IFC(xstring_ptr::CloneBuffer(pWholeMessage, cWholeStringLen, pstrFormattedErrorMessage));

Cleanup:

    xstrfree(pLeftSplitMessage);
    xstrfree(pUpdatedLeftSplitMessage);
    xstrfree(pWholeMessage);
    xstrfree(pRemainingMessage);

    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::GetMessageFromErrorCode
//
//  Synopsis:
//        a helper method, extract error message based on Error code.
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CErrorService::GetMessageFromErrorCode(XUINT32 iErrorID, _Out_ xstring_ptr* pstrMessage)
{
    IXcpBrowserHost *pHost = NULL;

    pstrMessage->Reset();

    if(m_pCore)
    {
        pHost = m_pCore->GetBrowserHost();
        
        if(pHost)
        {
            IFC_NOTRACE_RETURN(pHost->GetNonLocalizedErrorString(iErrorID, pstrMessage));
        }
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
//
//  Function: CErrorService::UpdateErrorCodeFromHResult
//
//  Synopsis:
//        a static helper method, get the corresponding error code from HRESULT.
//-----------------------------------------------------------------------------

_Check_return_ HRESULT
CErrorService::UpdateErrorCodeFromHResult(_In_ HRESULT hResult, _Out_ XUINT32 *pErrorCode)
{
    XUINT32 cr2cm = sizeof(eResult_eCode_Table) / sizeof(HResultToECodePair);
    const HResultToECodePair *pr2cm = eResult_eCode_Table;

    if (pErrorCode == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *pErrorCode = AG_E_UNKNOWN_ERROR;

    // Search the Code_To_Message mapping table to get the error message.

    while (cr2cm)
    {

        if (pr2cm->m_hrErr == hResult)
        {
            // Found a mapping
            *pErrorCode = pr2cm->m_iErrorCode;
            break;
        }

        pr2cm++;
        cr2cm--;
    }
    return S_OK;
}

_Check_return_ HRESULT
CErrorService::UnspecifyMediaError(_In_ XUINT32 iSpecificCode, _Out_ XUINT32 *pGenericCode)
{
    XUINT32 cecp = sizeof(Media_Specific_to_Generic_Table) / sizeof(ECodePair);
    const ECodePair *pecp = Media_Specific_to_Generic_Table;

    if (pGenericCode == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *pGenericCode = iSpecificCode;

    // Search the Specific_to_Generic mapping table to get the original code.

    while (cecp)
    {
        if (pecp->m_iKey == iSpecificCode)
        {
            // Found a mapping
            *pGenericCode = pecp->m_iEquivalent;
            break;
        }

        pecp++;
        cecp--;
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Function: CErrorService::AddError
//
//  Synopsis:
//       Add Error object to the list of errors.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CErrorService::AddError(HRESULT hrToOriginate, _In_ IError *pErrorObject)
{
    HRESULT hr = S_OK;
    CXcpList<IErrorServiceListener>* pListenersToNotify = NULL;

    if (!pErrorObject)
    {
        IFC(E_INVALIDARG);
    }

    {
        auto lock = m_CSError.lock();

        if(!m_pErrorList)
        {
            m_pErrorList = new CXcpList<IError>;
        }

        IFC(m_pErrorList->Add(pErrorObject));

        if (m_pListenerList)
        {
            // copy listeners into a local list - so we can call into them after releasing the error service lock
        
            pListenersToNotify = new CXcpList<IErrorServiceListener>();

            CXcpList<IErrorServiceListener>::XCPListNode* pNode = m_pListenerList->GetHead();
            while (pNode)
            {
                IFC(pListenersToNotify->Add(pNode->m_pData));
                AddRefInterface(pNode->m_pData);

                pNode = pNode->m_pNext;
            }            
        }
    }

    if (pListenersToNotify)
    {
        // notify the listeners
    
        CXcpList<IErrorServiceListener>::XCPListNode* pNode = pListenersToNotify->GetHead();
        while (pNode)
        {
            pNode->m_pData->NotifyErrorAdded(hrToOriginate, this);

            pNode = pNode->m_pNext;
        }            
    }

Cleanup:
    if (pListenersToNotify)
    {
        pListenersToNotify->Clean();
        delete pListenersToNotify;
    }

    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::ReportGenericError
//
//  Synopsis:
//       This is used everywhere else in the code to report errors. A CError
//          or deriving class is created and added to the ErrorService's list
//          of errors to be processed.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CErrorService::ReportGenericError(
                HRESULT hrToOriginate,
                ::ErrorType eType, 
                XUINT32 iErrorCode, 
                XUINT32 bRecoverable, 
                XUINT32 uLineNumber,
                XUINT32 uCharPosition, 
                _In_reads_(cParams) const xephemeral_string_ptr* pParams, 
                _In_ XUINT32 cParams,
                _In_opt_ CEventArgs *pErrorEventArgs,
                _In_opt_ CDependencyObject *pSender
                )
{
    switch(eType)
    {
        case ParserError:

            IFC_RETURN(ReportParserError(
                hrToOriginate,
                iErrorCode,
                bRecoverable,
                xstring_ptr::NullString(),
                xstring_ptr::NullString(),
                xstring_ptr::NullString(),
                uLineNumber,
                uCharPosition, 
                xstring_ptr::NullString(),
                pParams,
                cParams
                ));
            break;

        case RuntimeError:

            IFC_RETURN(ReportRuntimeError(
                hrToOriginate,
                iErrorCode, 
                bRecoverable,
                uLineNumber,
                uCharPosition,
                xstring_ptr::NullString(),
                pParams,
                cParams,
                pErrorEventArgs,
                pSender,
                xstring_ptr::NullString()
                ));
            break;

        default:
            IError *pError = NULL;

            IFC_RETURN(CError::Create(
                eType,
                iErrorCode, 
                bRecoverable, 
                uLineNumber, 
                uCharPosition, 
                pParams,
                cParams,
                &pError,
                this));


            if(pErrorEventArgs)
            {
                IFC_RETURN(pError->SetErrorEventArgs(pErrorEventArgs))
            }
            if(pSender)
            {
                pError->SetErrorEventSender(pSender);
            }

            IFC_RETURN(AddError(hrToOriginate, pError));
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::ReportParserError
//
//  Synopsis:
//       This is used everywhere else in the code to report errors. A 
//          CParserError or deriving class is created and added to the 
//          ErrorService's list of errors to be processed.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CErrorService::ReportParserError(
            _In_ HRESULT hrToOriginate,
            _In_ XUINT32 iErrorCode,
            _In_ XUINT32 bRecoverable,
            _In_ const xstring_ptr& strXamlFileName,
            _In_ const xstring_ptr& strXmlElement,
            _In_ const xstring_ptr& strXmlAttribute,
            _In_ XUINT32 uLineNumber,
            _In_ XUINT32 uCharPosition, 
            _In_ const xstring_ptr& strMessage, 
            _In_reads_(cParams) const xephemeral_string_ptr* pParams, 
            _In_ XUINT32 cParams
            )
{
    IError *pParserError = NULL;

    IFC_RETURN(CParserError::Create(
        iErrorCode, 
        bRecoverable, 
        strXamlFileName,
        strXmlElement,
        strXmlAttribute,
        uLineNumber, 
        uCharPosition, 
        pParams,
        cParams,
        &pParserError,
        this));


    // In case a localized error message cannot be retrieved,
    // use a hard coded error message string provided by
    // the caller.
    if (!strMessage.IsNullOrEmpty())
    {
        IFC_RETURN(pParserError->SetErrorMessage(strMessage));
    }

    IFC_RETURN(AddError(hrToOriginate, pParserError));

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CErrorService::ReportRuntimeError
//
//  Synopsis:
//       This is used everywhere else in the code to report errors. A 
//          CRuntimeError or deriving class is created and added to the 
//          ErrorService's list of errors to be processed.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CErrorService::ReportRuntimeError(
            HRESULT hrToOriginate,
            XUINT32 iErrorCode, 
            XUINT32 bRecoverable,
            XUINT32 uLineNumber,
            XUINT32 uCharPosition,
            _In_ const xstring_ptr& strMethodName, 
            _In_reads_(cParams) const xephemeral_string_ptr* pParams, 
            _In_ XUINT32 cParams,
            _In_opt_ CEventArgs* pErrorEventArgs,
            _In_opt_ CDependencyObject *pSender,
            _In_ const xstring_ptr& strErrorMessage
            )
{
    IError *pRuntimeError = NULL;

    IFC_RETURN(CRuntimeError::Create(
                iErrorCode, 
                bRecoverable, 
                uLineNumber, 
                uCharPosition, 
                strMethodName, 
                strErrorMessage,
                pParams,
                cParams,
                &pRuntimeError,
                this));
    

    if(pErrorEventArgs)
    {
        IFC_RETURN(pRuntimeError->SetErrorEventArgs(pErrorEventArgs))
    }
    if(pSender)
    {
        pRuntimeError->SetErrorEventSender(pSender);
    }

    IFC_RETURN(AddError(hrToOriginate, pRuntimeError));

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Function: CErrorService::OriginateInvalidOperationError
//
//  Synopsis:
//       Report an invalid operation error (via a managed error) from anywhere
//       in the code base that has a reference to the core.  Returns E_FAIL
//       (unless some other error is hit within this function, in which case
//       that error is returned).
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT CErrorService::OriginateInvalidOperationError(
    _In_ CCoreServices* pCore,
    _In_ XUINT32 uiErrorCode,
    _In_ const xstring_ptr_view& strParameter1,
    _In_ const xstring_ptr_view& strParameter2,
    _In_ const xstring_ptr_view& strParameter3)
{
    IErrorService *pErrorService = NULL;

    const HRESULT hrToOriginate = E_FAIL; // This will be the OriginateError code, and will be passed back to callers

    IFC_RETURN(pCore->getErrorService(&pErrorService));
    if (pErrorService)
    {
        xephemeral_string_ptr parameters[3];

        strParameter1.Demote(&parameters[0]);
        strParameter2.Demote(&parameters[1]);
        strParameter3.Demote(&parameters[2]);

        XUINT32 cParameters = 0;

        for (XUINT32 i = 0; i < ARRAY_SIZE(parameters); ++i)
        {
            if (parameters[i].IsNullOrEmpty())
            {
                break;
            }

            ++cParameters;
        }

        // Report the error
        IFC_RETURN(pErrorService->ReportGenericError(
            hrToOriginate, ManagedError, uiErrorCode, 0, 0, 0, parameters, cParameters, NULL, NULL));
        
#if DBG
        {
            IError* pError = NULL;
            xstring_ptr strErrorDescription;
            
            IFC_RETURN(pErrorService->GetFirstError(&pError));
            IFC_RETURN(pError->GetErrorMessage(&strErrorDescription));
            if (!strErrorDescription.IsNullOrEmpty())
            {
                TRACE(TraceAlways, L"Reporting Invalid Operation:  %s\n", strErrorDescription.GetBuffer());
            }
        }
#endif
    }

    // Return the OriginateError code to callers.  (If we hit some other error before we got here,
    // we let that error be returned to minimize any compat issues if any code actually expected
    // those other errors.)
    return hrToOriginate;
}


//------------------------------------------------------------------------
//  Method:   CError::Create
//
//  Synopsis:
//      Static method to create an instance of CError.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CError::Create(::ErrorType eType, 
        XUINT32 iErrorCode, 
        XUINT32 bRecoverable, 
        XUINT32 uLineNumber,
        XUINT32 uCharPosition, 
        _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams, 
        _In_ XUINT32 cParams,
        _Outptr_ IError** ppError,
        _In_opt_ IErrorService *pErrorService )
{

    if (!ppError)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *ppError = NULL;

    xref_ptr<CError> pError;
    pError.attach(new CError(eType, iErrorCode, bRecoverable, uLineNumber, uCharPosition, pErrorService));

    // Initialize the CError.
    IFC_RETURN(pError->Initialize(iErrorCode, pParams, cParams));

    *ppError = pError.detach();

    return S_OK;
}


//------------------------------------------------------------------------
//  Method:   AddRef
//
//  Synopsis:
//      Raises the reference count on the object.
//
//------------------------------------------------------------------------
XUINT32
CError::AddRef()
{
    return PAL_InterlockedIncrement((XINT32 *) &m_cRef);
}


//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Lowers the reference count on the object.  Will delete it when there
// are no remaining references.
//
//------------------------------------------------------------------------
XUINT32
CError::Release()
{
    XUINT32 cRef = PAL_InterlockedDecrement((XINT32 *) &m_cRef);

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}


//-------------------------------------------------------------------------
//
//  Function:   CError::GetErrorType
//
//  Synopsis:
//     Get Error Type.
//
//-------------------------------------------------------------------------
::ErrorType CError::GetErrorType()
{
    auto lock = m_CSError.lock();

    return m_eType;
}


//-----------------------------------------------------------------------------
//
//  Function: CError::GetErrorResult
//
//  Synopsis:
//      Get error Result. ( HRESULT)
//
//-----------------------------------------------------------------------------

HRESULT   CError::GetErrorResult( )
{
    auto lock = m_CSError.lock();

    return m_hrErr;
}


//-----------------------------------------------------------------------------
//
//  Function: CError::GetLineNumber
//
//  Synopsis:
//      Return the line number in xaml file when error occurs.
//-----------------------------------------------------------------------------
XUINT32   CError::GetLineNumber(  )
{
    auto lock = m_CSError.lock();

    return m_uLineNumber;
}


//-----------------------------------------------------------------------------
//
//  Function: CError::GetCharPosition
//
//  Synopsis:
//      Return char position that has error in xaml file.
//-----------------------------------------------------------------------------

XUINT32   CError::GetCharPosition(  )
{
    auto lock = m_CSError.lock();

    return m_uCharPosition;
}


//-------------------------------------------------------------------------
//
//  Function:   CError::GetIsRecoverable
//
//  Synopsis:
//     Get whether or not the error is recoverable.
//
//-------------------------------------------------------------------------
XUINT32 CError::GetIsRecoverable()
{
    auto lock = m_CSError.lock();

    return m_bRecoverable;
}


//-------------------------------------------------------------------------
//
//  Function:   CError::SetIsRecoverable
//
//  Synopsis:
//     Set whether or not the error is recoverable.
//
//-------------------------------------------------------------------------
XUINT32 CError::SetIsRecoverable(_In_ XUINT32 bIsRecoverable)
{
    auto lock = m_CSError.lock();

    return m_bRecoverable = bIsRecoverable;
}

//-----------------------------------------------------------------------------
//
//  Function: CError::GetErrorCode
//
//  Synopsis:
//      Get error code.
//
//-----------------------------------------------------------------------------
XUINT32  CError::GetErrorCode(  )
{
    auto lock = m_CSError.lock();

    return m_iErrorCode;
}


//-----------------------------------------------------------------------------
//
//  Function: CError::CError
//
//  Synopsis:
//      Constructor
//
//-----------------------------------------------------------------------------
CError::CError(
        ::ErrorType eType, 
        XUINT32 iErrorCode, 
        XUINT32 bRecoverable, 
        XUINT32 uLineNumber,
        XUINT32 uCharPosition,
        _In_opt_ IErrorService *pErrorService 
        )
    {
        m_eType = eType;
        m_iErrorCode = iErrorCode;
        m_bRecoverable = bRecoverable;
        m_uLineNumber = uLineNumber;
        m_uCharPosition = uCharPosition;
        m_pSender = NULL;
        m_hrErr = 0;
        m_cRef = 1;

        m_pErrorService = pErrorService;
    }

//-----------------------------------------------------------------------------
//
//  Function: CError::~CError
//
//  Synopsis:
//      Destructor
//
//-----------------------------------------------------------------------------
CError::~CError( )
{
    ReleaseInterface(m_pSender);
}


//-----------------------------------------------------------------------------
//
//  Function: CError::Initialize
//
//  Synopsis:
//        Initialize instance of CError instance.
//-----------------------------------------------------------------------------
_Check_return_ 
HRESULT 
CError::Initialize(_In_ XUINT32 iErrorCode, _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams, _In_ XUINT32 cParams)
{
    // HRESULT errors have high bit set, error codes don't. 
    if(iErrorCode & 0x80000000)
    {
        m_iErrorCode = 0;  // SetErrorResult will update it appropriately.
        SetErrorResult(iErrorCode, pParams, cParams);
    }
    else
    {
        SetErrorCode(iErrorCode, pParams, cParams);
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CError::SetErrorResult
//
//  Synopsis:
//      Get error result. (HRESULT)
//
//-----------------------------------------------------------------------------

void CError::SetErrorResult(HRESULT hrErr, _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams, _In_ XUINT32 cParams)
{
    auto lock = m_CSError.lock();

    // AG Error codes can be encoded as HRESULTS by setting the high order bits.
    XUINT32 iErrorCode = AgCodeFromHResult(hrErr);
    
    if (!iErrorCode)
    {
        m_hrErr = hrErr;

        if (m_iErrorCode == 0 || m_iErrorCode == AG_E_UNKNOWN_ERROR)
        {
            // Try to get more meaningful error code.
            IGNOREHR(CErrorService::UpdateErrorCodeFromHResult(hrErr, &iErrorCode));
        }
    }
    
    SetErrorCode(iErrorCode, pParams, cParams);
}


//-----------------------------------------------------------------------------
//
//  Function: CError::GetErrorMessage
//
//  Synopsis:
//      Get error message based on the error settings.
//      Caller is responsbile to clean up the memory.
//
//-----------------------------------------------------------------------------

HRESULT   CError::GetErrorMessage(_Out_ xstring_ptr* pstrErrorMessage)
{
    auto lock = m_CSError.lock();

    *pstrErrorMessage = m_strErrorDescription;

    if (   pstrErrorMessage->IsNullOrEmpty()
        && NULL != m_pErrorService)
    {
        IFC_RETURN(m_pErrorService->GetMessageFromErrorCode(m_iErrorCode, pstrErrorMessage));

        if (pstrErrorMessage->IsNullOrEmpty())
        {
            // Cannot find the error message, try to get message with AG_E_GENERAL_ERROR
            IGNOREHR(m_pErrorService->GetMessageFromErrorCode(AG_E_UNKNOWN_ERROR, pstrErrorMessage));
        }
    }

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CError::SetErrorMessage
//
//  Synopsis:
//      Set error message.
//      If caller knows the error message, and is sure there is no easy
//      mapping between the message and error code.
//
//-----------------------------------------------------------------------------

HRESULT   CError::SetErrorMessage(_In_ const xstring_ptr& strErrorMessage)
{
    auto lock = m_CSError.lock();

    m_strErrorDescription = strErrorMessage;

    RRETURN(S_OK);
}


//-----------------------------------------------------------------------------
//
//  Function: CError::SetErrorCode
//
//  Synopsis:
//      Set error code.
//
//-----------------------------------------------------------------------------
void CError::SetErrorCode(_In_ XUINT32 iErrorCode, _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams, _In_ XUINT32 cParams)
{
    auto lock = m_CSError.lock();

    xstring_ptr strErrorMessage;
    xstring_ptr strFormattedErrorMessage;

    m_iErrorCode = iErrorCode;

    if (iErrorCode != 0)
    {
        if (m_strErrorDescription.IsNullOrEmpty())
        {
            if (m_pErrorService)
            {
                IGNOREHR(m_pErrorService->GetMessageFromErrorCode(m_iErrorCode, &strErrorMessage));
            }
        }

        if (!strErrorMessage.IsNullOrEmpty())
        {
            if (SUCCEEDED(CErrorService::FormatErrorMessage(strErrorMessage, &strFormattedErrorMessage, pParams, cParams)))
            {
                m_strErrorDescription = strFormattedErrorMessage;
            }
            else
            {
                m_strErrorDescription = strErrorMessage;
            }
        }
    }
}


//-----------------------------------------------------------------------------
//
//  Function: CError::SetErrorEventSender
//
//  Synopsis:
//       Set sender object. This can be called by the script plugin object.
//
//-----------------------------------------------------------------------------
void CError::SetErrorEventSender(_In_ CDependencyObject *pSender)
{
    auto lock = m_CSError.lock();

    // Add reference new pointer
    AddRefInterface(pSender);

    // Release the old object if it exists.
    ReleaseInterface(m_pSender);

    // Update the new Sender.
    m_pSender = pSender;
}


//-----------------------------------------------------------------------------
//
//  Function: CError::SetErrorEventArgs
//
//  Synopsis:
//       Set ErrorEventArgs to this service.
//       This is useful for async failure report.
//       such as when MediaFailed, ImageFailed occurs, but there is event handler
//       is registerted,  such async error will then fall back to plugin's
//       global error handler, the code needs to set current error event args
//       to the service.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CError::SetErrorEventArgs(_In_ CEventArgs* pErrorEventArgs)
{
    auto lock = m_CSError.lock();

    xstring_ptr strMessage;
    CErrorEventArgs *pErrEventArgs = NULL;

    // Save the necessary information from the error event args
    pErrEventArgs = static_cast<CErrorEventArgs *> (pErrorEventArgs);
    if (pErrEventArgs != NULL)
    {
        m_eType = pErrEventArgs->m_eType;
        m_iErrorCode = pErrEventArgs->m_iErrorCode;
        m_hrErr = pErrEventArgs->m_hResult;

        if (!pErrEventArgs->m_strErrorMessage.IsNullOrEmpty())
        {
            m_strErrorDescription = pErrEventArgs->m_strErrorMessage;
        }
        else
        {
            if (m_pErrorService)
            {
                IFC_RETURN(m_pErrorService->GetMessageFromErrorCode(pErrEventArgs->m_iErrorCode, &strMessage));
            }

            pErrEventArgs->m_strErrorMessage = strMessage;
        }
    }

    return S_OK;
}



//-----------------------------------------------------------------------------
//
//  Function: CError::GetErrorEventSender
//
//  Synopsis:
//       Get Sender object.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CError::GetErrorEventSender(_Outptr_ CDependencyObject **ppSender)
{
    auto lock = m_CSError.lock();

    HRESULT hr = S_OK;

    *ppSender = m_pSender;

    if(m_pSender)
    {
        AddRefInterface(m_pSender);
    }

    RRETURN(hr);
}


//------------------------------------------------------------------------
//  Method:   CParserError::Create
//
//  Synopsis:
//      Static method to create an instance of CParserError.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CParserError::Create(
         XUINT32 iErrorCode,
         XUINT32 bRecoverable,
         _In_ const xstring_ptr& strXamlFileName,
         _In_ const xstring_ptr& strXmlElement,
         _In_ const xstring_ptr& strXmlAttribute,
         XUINT32 uLineNumber,
         XUINT32 uCharPosition,
        _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams, 
        _In_ XUINT32 cParams,
        _Outptr_ IError** ppError,
        _In_opt_ IErrorService *pErrorService )
{

    if (!ppError)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *ppError = NULL;

    xref_ptr<CParserError> pError;
    pError.attach(new CParserError(iErrorCode, bRecoverable, strXamlFileName, strXmlElement, strXmlAttribute, uLineNumber, uCharPosition, pErrorService));

    // Initialize the CError.
    IFC_RETURN(pError->Initialize(iErrorCode, pParams, cParams));

    *ppError = pError.detach();

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CParserError::~CParserError
//
//  Synopsis:
//      Destructor
//
//-----------------------------------------------------------------------------
CParserError::~CParserError()
{
    ReleaseInterface(m_pSender);
}


//-----------------------------------------------------------------------------
//
//  Function: CParserError::SetErrorEventArgs
//
//  Synopsis:
//       Set ErrorEventArgs to this service.
//       This is useful for async failure report.
//       such as when MediaFailed, ImageFailed occurs, but there is event handler
//       is registerted,  such async error will then fall back to plugin's
//       global error handler, the code needs to set current error event args
//       to the service.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CParserError::SetErrorEventArgs(_In_ CEventArgs* pErrorEventArgs)
{
    auto lock = m_CSError.lock();

    CErrorEventArgs *pErrEventArgs = NULL;

    // Save the necessary information from the error event args
    pErrEventArgs = static_cast<CErrorEventArgs *> (pErrorEventArgs);
    if (pErrEventArgs != NULL)
    {
        CParserErrorEventArgs  *pParser_EEA = NULL;

        // For Parser Error, ParserErrorEventArgs is created;
        pParser_EEA = static_cast<CParserErrorEventArgs *>(pErrorEventArgs);
        IFCPTR_RETURN(pParser_EEA);

        m_uLineNumber = pParser_EEA->m_uLineNumber;
        m_uCharPosition = pParser_EEA->m_uCharPosition;

        if (!pParser_EEA->m_strXamlFileName.IsNullOrEmpty())
        {
            m_strXamlFileName = pParser_EEA->m_strXamlFileName;
        }

        if (!pParser_EEA->m_strXmlElement.IsNullOrEmpty())
        {
            m_strXmlElement = pParser_EEA->m_strXmlElement;
        }

        if (pParser_EEA->m_strXmlAttribute.IsNullOrEmpty())
        {
            m_strXmlAttribute = pParser_EEA->m_strXmlAttribute;
        }

        IFC_RETURN(CError::SetErrorEventArgs(pErrorEventArgs));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//  Method:   CRuntimeError::Create
//
//  Synopsis:
//      Static method to create an instance of CRuntimeError.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRuntimeError::Create(
        XUINT32 iErrorCode, 
        XUINT32 bRecoverable, 
        XUINT32 uLineNumber,
        XUINT32 uCharPosition, 
        _In_ const xstring_ptr& strMethodName, 
        _In_ const xstring_ptr& strErrorMessage,
        _In_reads_(cParams) const xephemeral_string_ptr* pParams, 
        _In_ XUINT32 cParams,
        _Outptr_ IError** ppError,
        _In_opt_ IErrorService *pErrorService )
{

    if (!ppError)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    *ppError = NULL;

    xref_ptr<CRuntimeError> pError;
    pError.attach(new CRuntimeError(iErrorCode, bRecoverable, uLineNumber, uCharPosition, strMethodName, pErrorService));

    // Initialize the CError.
    IFC_RETURN(pError->Initialize(iErrorCode, pParams, cParams));

    *ppError = pError.detach();

    return S_OK;
}


//-----------------------------------------------------------------------------
//
//  Function: CRuntimeError::~CRuntimeError
//
//  Synopsis:
//      Destructor
//
//-----------------------------------------------------------------------------
CRuntimeError::~CRuntimeError()
{
}

//-----------------------------------------------------------------------------
//
//  Function: CRuntimeError::SetErrorEventArgs
//
//  Synopsis:
//       Set ErrorEventArgs to this service.
//       This is useful for async failure report.
//       such as when MediaFailed, ImageFailed occurs, but there is event handler
//       is registerted,  such async error will then fall back to plugin's
//       global error handler, the code needs to set current error event args
//       to the service.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT
CRuntimeError::SetErrorEventArgs(_In_ CEventArgs* pErrorEventArgs)
{
    auto lock = m_CSError.lock();

    CErrorEventArgs *pErrEventArgs = NULL;

    // Save the necessary information from the error event args
    pErrEventArgs = static_cast<CErrorEventArgs *> (pErrorEventArgs);
    if (pErrEventArgs != NULL)
    {
        CRuntimeErrorEventArgs  *pRuntime_EEA = NULL;
 
        // For Runtime Error, CRuntimeErrorEventArgs is created;
        pRuntime_EEA = static_cast<CRuntimeErrorEventArgs *>(pErrorEventArgs);
        IFCPTR_RETURN(pRuntime_EEA);

        if (!pRuntime_EEA->m_strMethodName.IsNullOrEmpty())
        {
            m_strMethodName = pRuntime_EEA->m_strMethodName;
        }

        m_uLineNumber = pRuntime_EEA->m_uLineNumber;
        m_uCharPosition = pRuntime_EEA->m_uCharPosition;

        IFC_RETURN(CError::SetErrorEventArgs(pErrorEventArgs));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   CXcpList::Clean
//
//  Synopsis:
//      This is a specialized Clean function for an XcpList of IErrors.
//      It is necessary because the IErrors need to be Released, not
//      deleted on a Clean.
//
//------------------------------------------------------------------------
template<>
void CXcpList<IError>::Clean(XUINT8 bDoDelete)
{
    XCPListNode *pTemp;
    IError* pError = NULL;

    while (m_pHead)
    {
        pTemp = m_pHead;
        m_pHead = m_pHead->m_pNext;
        
        pError = reinterpret_cast<IError *>(pTemp->m_pData);
        ReleaseInterface(pError);

        delete pTemp;
    }

    m_pHead = NULL;
}

//------------------------------------------------------------------------
//
//  Function:   CXcpList<IErrorServiceListener>::Clean
//
//  Synopsis:
//      Template specialization to call IErrorServiceListener::Release.
//
//------------------------------------------------------------------------
template<>
void CXcpList<IErrorServiceListener>::Clean(XUINT8 bDoDelete)
{
    XCPListNode *pTemp;
    
    while (m_pHead)
    {
        pTemp = m_pHead;
        m_pHead = m_pHead->m_pNext;
    
        if (bDoDelete)
        {
            ReleaseInterface(pTemp->m_pData);
        }
        pTemp->m_pData = NULL;        

        delete pTemp;
    }
    
    m_pHead = NULL;
    m_pTail = NULL;
}



