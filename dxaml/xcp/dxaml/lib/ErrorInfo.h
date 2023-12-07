// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines DirectUI::ErrorInfo, a helper class for working with
//      IErrorInfo and IRestrictedErrorInfo.

#pragma once

namespace DirectUI
{
    //-----------------------------------------------------------------------------
    //
    // BSTR wrapper.
    //     
    //-----------------------------------------------------------------------------
    class AutoBstr
    {
    public:
        AutoBstr() :
            m_value(NULL)
        {
        }
    
        ~AutoBstr()
        {
            Free();
        }
    
        BOOL IsEmpty()
        {
            return !m_value || SysStringLen(m_value) == 0;
        }
    
        _Check_return_ HRESULT ToXStringPtr(_Out_ xstring_ptr* pstrValue)
        {
            HRESULT hr = S_OK;    
            XUINT32 cchValue = SysStringLen(m_value);
    
            pstrValue->Reset();
            if (cchValue == 0)
            {
                goto Cleanup;
            }
            
            IFC(xstring_ptr::CloneBuffer(m_value, cchValue, pstrValue));
    
        Cleanup:
            RRETURN(hr);
        }
    
        void Free()
        {
            if (m_value)
            {
                SysFreeString(m_value);
                m_value = NULL;
            }
        }
    
        BSTR* FreeAndGetAddressOf() {
            if (m_value)
            {
                SysFreeString(m_value);
                m_value = NULL;
            }
            return &m_value;
        }    
    
    private:
        BSTR m_value;
        
        // uncopyable / unassignable    
        AutoBstr(const AutoBstr& other);
        AutoBstr& operator=(const AutoBstr& other);
    };
    
    //-----------------------------------------------------------------------------
    //
    // ErrorInfo is a helper class for working with error info objects.
    // 
    // An instance of ErrorInfo wraps an error info object. The wrapped error info
    // may be NULL or may be a valid instance. It may be an IRestrictedErrorInfo or
    // a regular IErrorInfo.
    //
    // This helper class is not thread safe.
    //
    // None of the functions of this class trace errors using IFC or related macros.
    // It's expected that this will be used in error handling code paths, and tracing
    // secondary errors is likely to mask the primary error being handled.
    //     
    //-----------------------------------------------------------------------------
    class ErrorInfo
    {
    public:
        ErrorInfo()
        {
            Clear();
        }

        //-----------------------------------------------------------------------------
        //
        // Clears all the state of this ErrorInfo instance. After calling Clear()
        // this instance will be holding a null error info object.
        //     
        //-----------------------------------------------------------------------------
        void Clear();

        //-----------------------------------------------------------------------------
        //
        // Resets this ErrorInfo instance to hold the specified error info object.
        // (Or clears the state of this ErrorInfo instance if NULL is passed).
        //     
        //-----------------------------------------------------------------------------
        void SetErrorInfo(_In_opt_ IErrorInfo* pErrorInfo);

        //-----------------------------------------------------------------------------
        //
        // Resets this ErrorInfo instance to hold the specified restricted error info 
        // object. (Or clears the state of this ErrorInfo instance if NULL is passed).
        //     
        //-----------------------------------------------------------------------------
        void SetRestrictedErrorInfo(_In_opt_ IRestrictedErrorInfo* pRestrictedErrorInfo);
        
        //-----------------------------------------------------------------------------
        //
        // Populates this ErrorInfo instance with the error info object from the calling
        // thread, if any. If the calling thread does not have any error info object,
        // this is equivalent to calling Clear().
        //
        // IMPORTANT: This function leaves the thread's error info intact. i.e., it is
        // unlike calling ::GetErrorInfo() in that the thread will still have an error
        // info object after calling this function.
        //     
        //-----------------------------------------------------------------------------
        _Check_return_ HRESULT GetFromThread();
        
        //-----------------------------------------------------------------------------
        //
        // Sets the error info object currently held by this ErrorInfo instance as
        // the thread's error info object.
        //
        // If this instance does not currently hold an error info (for example, after
        // calling Clear() or SetErrorInfo(NULL)), this function will clear the thread's
        // error info object.
        //
        //-----------------------------------------------------------------------------
        _Check_return_ HRESULT SetOnThread();

        //-----------------------------------------------------------------------------
        //
        // Initializes this ErrorInfo instance to hold a new error info object.
        //
        // The new object will be an IErrorInfo. If you want to create an 

        // IRestrictedErrorInfo, use CreateRestrictedErrorInfo() instead.
        //
        // If pstrDescription is non-NULL, it will be set as the description of the
        // new error info object.
        //     
        //-----------------------------------------------------------------------------
        _Check_return_ HRESULT CreateErrorInfo(_In_ const xstring_ptr& strDescription);

        //-----------------------------------------------------------------------------
        //
        // Initializes this ErrorInfo instance to hold a new error info object.
        //
        // The new object will be an IRestrictedErrorInfo.
        //
        // IRestrictedErrorInfo requires a failure code and non-empty description and
        // restricted description. If these requirements aren't satisfied, this function
        // will return E_INVALIDARG.
        //     
        //-----------------------------------------------------------------------------
        _Check_return_ HRESULT CreateRestrictedErrorInfo(HRESULT hrErrorCode, _In_ const xstring_ptr& strRestrictedDescription);

        //-----------------------------------------------------------------------------
        //
        // Returns TRUE if this ErrorInfo instance currently holds a non-NULL error
        // info object.
        //     
        //-----------------------------------------------------------------------------
        BOOL HasErrorInfo();

        //-----------------------------------------------------------------------------
        //
        // Returns a pointer to the error info object currently held by this ErrorInfo
        // instance. Returns NULL if this instance doesn't hold an error info object.
        //
        // The returned pointer is NOT addref'd. If the caller needs to retain the 
        // pointer after any subsequent calls to this ErrorInfo instance, including its 
        // destruction, the caller must first add a reference.
        //
        //-----------------------------------------------------------------------------
        IErrorInfo* GetErrorInfoNoRef();

        //-----------------------------------------------------------------------------
        //
        // Returns TRUE if this ErrorInfo instance currently holds an error info object
        // that is an IRestrictedErrorInfo, not just an IErrorInfo.
        //
        // If HasRestrictedErrorInfo() returns TRUE, GetRestrictedErrorInfoNoRef() will
        // return non-NULL. Otherwise it will return NULL.
        //     
        //-----------------------------------------------------------------------------
        BOOL HasRestrictedErrorInfo();

        //-----------------------------------------------------------------------------
        //
        // If this ErrorInfo instance currently holds an error info object that is an
        // IRestrictedErrorInfo (not just an IErrorInfo), returns a pointer to that
        // IRestrictedErrorInfo. Otherwise returns NULL (if this instance doesn't hold
        // an error info object or holds one that is not an IRestrictedErrorInfo).
        //
        // The returned pointer is NOT addref'd. If the caller needs to retain the 
        // pointer after any subsequent calls to this ErrorInfo instance, including its 
        // destruction, the caller must first add a reference.
        //
        //-----------------------------------------------------------------------------
        IRestrictedErrorInfo* GetRestrictedErrorInfoNoRef();

        //-----------------------------------------------------------------------------
        //
        // Returns TRUE if this ErrorInfo instance is currently holding an 
        // error info that has a non-empty description string.
        //     
        //-----------------------------------------------------------------------------
        BOOL HasDescription();

        //-----------------------------------------------------------------------------
        //
        // If this ErrorInfo instance is currently holding an error info object with
        // a non-empty description string, returns that string. Otherwise returns NULL.
        //     
        //-----------------------------------------------------------------------------
        _Check_return_ HRESULT GetDescription(_Out_ xstring_ptr* pstrDescription);

        //-----------------------------------------------------------------------------
        //
        // Returns TRUE if this ErrorInfo instance is currently holding an 
        // IRestrictedErrorInfo that has a non-empty restricted description string.
        //     
        //-----------------------------------------------------------------------------
        BOOL HasRestrictedDescription();
        
        //-----------------------------------------------------------------------------
        //
        // If this ErrorInfo instance is currently holding an IRestrictedErrorInfo,
        // with a non-empty restricted description string, returns that string. 
        // Otherwise returns NULL.
        //     
        //-----------------------------------------------------------------------------
        _Check_return_ HRESULT GetRestrictedDescription(_Out_ xstring_ptr* pstrRestrictedDescription);

        //-----------------------------------------------------------------------------
        //
        // The "message" of an error info is a synthetic property that is meant to 
        // represent the single best (most informative) description string that's available.
        //
        // HasMessage() will return TRUE if this ErrorInfo currently has a non-empty
        // restricted description or description. i.e., HasMessage() ==
        // HasRestrictedDescription() || HasDescription().
        //
        //-----------------------------------------------------------------------------
        BOOL HasMessage();

        //-----------------------------------------------------------------------------
        //
        // Returns the "message" of this error info. (See HasMessage()).
        //
        // If this ErrorInfo instance has a non-empty restricted description, returns 
        // that. Otherwise, if this has a non-empty description, returns that.
        // Otherwise, returns NULL.
        //
        //-----------------------------------------------------------------------------
        _Check_return_ HRESULT GetMessage(_Out_ xstring_ptr* pstrMessage);

        //-----------------------------------------------------------------------------
        //
        // Returns TRUE if this ErrorInfo instance is currently holding an error info
        // that has a true failure code.
        //
        // Will return FALSE if:
        //     - this instance is currently holding a null error info object (e.g. if
        //       Clear() was called)
        //
        //     - this instance is holding an error info object that is an IErrorInfo 
        //       but not an IRestrictedErrorInfo (IErrorInfo doesn't have an error 
        //       code)
        //
        //     - this instance is holding an IRestrictedErrorInfo but the error code
        //       is not a failure code (this should never happen, as the system 
        //       implementation of IRestrictedErrorInfo validates against it)
        //
        // If HasErrorCode() returns TRUE, then FAILED(GetErrorCode()) will be TRUE.
        //
        //-----------------------------------------------------------------------------
        BOOL HasErrorCode();

        //-----------------------------------------------------------------------------
        //
        // Returns the error code of the error info currently held by this instance.
        // See HasErrorCode() for important notes.
        //
        //-----------------------------------------------------------------------------
        HRESULT GetErrorCode();

    private:
        ctl::ComPtr<IErrorInfo> m_spErrorInfo;
        BOOL m_ErrorInfoInitialized;
        
        ctl::ComPtr<IRestrictedErrorInfo> m_spRestrictedErrorInfo;                    
        BOOL m_RestrictedErrorInfoInitialized;

        AutoBstr m_description;
        AutoBstr m_restrictedDescription;
        HRESULT m_ErrorCode;
        BOOL m_ErrorDetailsInitialized;

        void EnsureErrorInfoInitialized();
        void EnsureRestrictedErrorInfoInitialized();
        _Check_return_ HRESULT EnsureErrorDetailsInitialized();
    };
}
