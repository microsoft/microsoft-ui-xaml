// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;
class CCoreServices;
class CError;
struct IError;

//------------------------------------------------------------------------
//
//  class:  CErrorService
//
//  Synopsis:
//      Keep all possible error information in the CoreService and manipuate
//      the error information, such as generating the ErrorEventArgs for report.
//
//      Control Host can also update some error information appropriately.
//
//------------------------------------------------------------------------
class CErrorService final : public IErrorService
{

public:

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    _Check_return_ HRESULT AddListener(_In_ IErrorServiceListener* pListener) override;
    _Check_return_ HRESULT RemoveListener(_In_ IErrorServiceListener* pListener) override;

    _Check_return_ HRESULT  GetLastReportedError(_Outptr_ IError **ppError) override;
    _Check_return_ HRESULT  GetFirstError(_Outptr_ IError **ppError) override;

    _Check_return_ HRESULT AddError(HRESULT hrToOriginate, _In_ IError *pErrorObject) override;

    _Check_return_ HRESULT  CleanupLastError(_In_ IError *pError) override;

    void CleanupErrors( ) override;

    void CoreResetCleanup( _In_ CCoreServices* pCore ) override;

    static _Check_return_ HRESULT UpdateErrorCodeFromHResult(_In_ HRESULT hResult, _Out_ XUINT32 *pErrorCode);

    _Check_return_ HRESULT GetMessageFromErrorCode(XUINT32 iErrorID, _Out_ xstring_ptr* pstrMessage) override;

    static _Check_return_ HRESULT FormatErrorMessage(_In_ const xstring_ptr& strOriginalErrorMessage,
                                                    _Out_ xstring_ptr* pstrFormattedErrorMessage,
                                                    _In_reads_(cParams) const xephemeral_string_ptr* pParams,
                                                    _In_ XUINT32 cParams);


    _Check_return_ HRESULT ReportGenericError(
                HRESULT hrToOriginate,
                ::ErrorType eType,
                XUINT32 iErrorCode,
                XUINT32 bRecoverable,
                XUINT32 uLineNumber,
                XUINT32 uCharPosition,
                _In_reads_(cParams) const xephemeral_string_ptr* pParams,
                _In_ XUINT32 cParams = 0,
                _In_opt_ CEventArgs *pErrorEventArgs = nullptr,
                _In_opt_ CDependencyObject *pSender = nullptr
                ) override;


    _Check_return_ HRESULT ReportParserError(
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
                ) override;


    virtual _Check_return_ HRESULT ReportRuntimeError(
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
                );

    // Report an invalid operation error (via a managed error) from anywhere in
    // the code base that has a reference to the core, and returns failure.
    static _Check_return_ HRESULT OriginateInvalidOperationError(
        _In_ CCoreServices* pCore,
        _In_ XUINT32 uiErrorCode,
        _In_ const xstring_ptr_view& strParameter1 = xstring_ptr::NullString(),
        _In_ const xstring_ptr_view& strParameter2 = xstring_ptr::NullString(),
        _In_ const xstring_ptr_view& strParameter3 = xstring_ptr::NullString());

    static _Check_return_ HRESULT Create(_In_ CCoreServices *pCore, _Outptr_ IErrorService** ppErrorService);

protected:

    //
    // Make ctor protected, the instance should be created through the static method
    // CErrorService::Create( ).
    //
    CErrorService(CCoreServices *pCore);

    // Destructor
    ~CErrorService();

private :

    void CleanupListeners();

    wil::critical_section m_CSError;

    _Guarded_by_(m_CSError) CCoreServices *m_pCore;             //
    XUINT32  m_cRef;                    // Ref count on ErrorService
    _Guarded_by_(m_CSError) CXcpList<IError> *m_pErrorList;     // List of error object
    _Guarded_by_(m_CSError) CXcpList<IErrorServiceListener>* m_pListenerList;  // List of error service listeners, lazily initialized
};

template<>
void CXcpList<IError>::Clean(XUINT8 bDoDelete);

template<>
void CXcpList<IErrorServiceListener>::Clean(XUINT8 bDoDelete);

//------------------------------------------------------------------------
//
//  class:  CError
//
//  Synopsis:
//      Keep all error information in the error object. Deriving from this
//      class is CParserError and CRuntimeError. CError is instantiated
//      for media, image, and download errors.
//
//------------------------------------------------------------------------
class CError : public IError
{
public:

    static _Check_return_ HRESULT Create(
        ::ErrorType eType,
        XUINT32 iErrorCode,
        XUINT32 bRecoverable,
        XUINT32 uLineNumber,
        XUINT32 uCharPosition,
        _In_reads_(cParams) const xephemeral_string_ptr* pParams,
        _In_ XUINT32 cParams,
        _Outptr_ IError** ppError,
        _In_opt_ IErrorService *pErrorService);

protected:

    // Constructors
    CError(
        ::ErrorType eType,
        XUINT32 iErrorCode,
        XUINT32 bRecoverable,
        XUINT32 uLineNumber = 0,
        XUINT32 uCharPosition = 0,
        _In_opt_ IErrorService *pErrorService = NULL
        );

    // Destructor
    virtual ~CError();

    _Check_return_ HRESULT Initialize(_In_ XUINT32 iErrorCode, _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams, _In_ XUINT32 cParams);

public:

    XUINT32 AddRef() override;
    XUINT32 Release() override;

    void SetErrorEventSender(_In_ CDependencyObject *pSender) override;
    _Check_return_ HRESULT GetErrorEventSender(_Outptr_ CDependencyObject **ppSender) override;

    _Check_return_ HRESULT SetErrorEventArgs(_In_ CEventArgs* pErrorEventArgs) override;

    HRESULT   GetErrorMessage(_Out_ xstring_ptr* pstrErrorMessage  ) override;

    ::ErrorType GetErrorType() override;
    XUINT32   GetErrorCode() override;
    HRESULT   GetErrorResult() override;
    XUINT32   GetLineNumber() override;
    XUINT32   GetCharPosition() override;
    XUINT32   GetIsRecoverable() override;

    HRESULT SetErrorMessage(_In_ const xstring_ptr& strErrorMessage) override;
    void    SetErrorCode(_In_ XUINT32 iErrorCode, _In_reads_opt_(cParams) const xephemeral_string_ptr* pParams, _In_ XUINT32 cParams);
    void    SetErrorResult(HRESULT hrErr, _In_reads_(cParams) const xephemeral_string_ptr* pParams, _In_ XUINT32 cParams);

    XUINT32   SetIsRecoverable(_In_ XUINT32 bIsRecoverable) override;

public:

    _Guarded_by_(m_CSError) ::ErrorType m_eType;                    // Error Type.
    _Guarded_by_(m_CSError) XUINT32 m_iErrorCode;                   // Error Code.
    _Guarded_by_(m_CSError) XUINT32 m_bRecoverable;                 // Determines whether or not the error is recoverable
    _Guarded_by_(m_CSError) XUINT32 m_uLineNumber;                  // Line number in file where error happens.
    _Guarded_by_(m_CSError) XUINT32 m_uCharPosition;                // Charactor position in the file where the error occurs.

    _Guarded_by_(m_CSError) HRESULT m_hrErr;                        // HResult for the error. There should be a mapping between iErrorID and hrErr.
    _Guarded_by_(m_CSError) xstring_ptr m_strErrorDescription;      // Error description.
    XUINT32  m_cRef;                        // Ref count on Error

protected :

    _Guarded_by_(m_CSError) CDependencyObject *m_pSender;           // Dependent object on which the error occurs.

    IErrorService *m_pErrorService;
    wil::critical_section m_CSError;
};



//------------------------------------------------------------------------
//
//  class:  CParserError
//
//  Synopsis:
//      Keep all error information in the parser error class for parser
//      errors.
//
//------------------------------------------------------------------------
class CParserError : public CError
{

public:

    static _Check_return_ HRESULT Create(
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
        _In_opt_ IErrorService *pErrorService);

protected:

    // Constructor
    CParserError(
         XUINT32 iErrorCode,
         XUINT32 bRecoverable,
         _In_ const xstring_ptr& strXamlFileName,
         _In_ const xstring_ptr& strXmlElement,
         _In_ const xstring_ptr& strXmlAttribute,
         XUINT32 uLineNumber,
         XUINT32 uCharPosition,
        _In_opt_ IErrorService *pErrorService)
         : CError(ParserError, iErrorCode, bRecoverable, uLineNumber, uCharPosition, pErrorService)
         , m_strXamlFileName(strXamlFileName)
         , m_strXmlElement(strXmlElement)
         , m_strXmlAttribute(strXmlAttribute)
    {
    }

    // Destructor
    ~CParserError() override;

public:

    _Check_return_ HRESULT SetErrorEventArgs(_In_ CEventArgs* pErrorEventArgs) override;

public:

    // Parser error information
    /*_Guarded_by_(m_CSError)*/ xstring_ptr m_strXamlFileName;         // Xaml file name.
    /*_Guarded_by_(m_CSError)*/ xstring_ptr m_strXmlElement;           // The name of the element tag in the xaml file.
    /*_Guarded_by_(m_CSError)*/ xstring_ptr m_strXmlAttribute;         // The Attribute name in the xaml file.
};


//------------------------------------------------------------------------
//
//  class:  CRuntimeError
//
//  Synopsis:
//      Keep all error information in the runtime error class for runtime
//      errors.
//
//------------------------------------------------------------------------
class CRuntimeError : public CError
{
public :

    static _Check_return_ HRESULT Create(
        XUINT32 iErrorCode,
        XUINT32 bRecoverable,
        XUINT32 uLineNumber,
        XUINT32 uCharPosition,
        _In_ const xstring_ptr& strMethodName,
        _In_ const xstring_ptr& strErrorMessage,
        _In_reads_(cParams) const xephemeral_string_ptr* pParams,
        _In_ XUINT32 cParams,
        _Outptr_ IError** ppError,
        _In_opt_ IErrorService *pErrorService);

protected:

    CRuntimeError(
        XUINT32 iErrorCode,
        XUINT32 bRecoverable,
        XUINT32 uLineNumber,
        XUINT32 uCharPosition,
        const xstring_ptr& strMethodName,
        _In_opt_ IErrorService *pErrorService
        ) : CError(RuntimeError, iErrorCode, bRecoverable, uLineNumber, uCharPosition, pErrorService)
          , m_strMethodName(strMethodName)
    {
    }


    // Destructor
    ~CRuntimeError() override;

public:

    _Check_return_ HRESULT SetErrorEventArgs(_In_ CEventArgs* pErrorEventArgs) override;

public:

    // Runtime error information.
    /*_Guarded_by_(m_CSError)*/ xstring_ptr m_strMethodName;           // The name of method when error occurs. this is for synch method call inside javaScript.
};

