// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//------------------------------------------------------------------------
//  Abstract:
//
// Contains the PAL Notification Interfaces
//------------------------------------------------------------------------

#ifndef __PAL__NOTIFY__
#define __PAL__NOTIFY__


//------------------------------------------------------------------------
//
//  Interface:  INotifyOnDeleteCallback
//
//  Synopsis:
//      Used to notify when a object is deleted so weakreference containers can clean up
// This interface is implemented by the container.
//
//------------------------------------------------------------------------
struct INotifyOnDeleteCallback
{
    virtual void OnDelete(_In_ const xstring_ptr& token) = 0;
};

//------------------------------------------------------------------------
//
//  Mixin Class:  CNotifyOnDelete
//
//  Synopsis:
//      Designed to be mixed in as a base or multiple-base class for a type that
// wants to notify a container when it is being deleted.
//
//------------------------------------------------------------------------
struct CNotifyOnDelete
{

    CNotifyOnDelete()
    {
        m_pNotifyCallback = NULL;
    }

    virtual  ~CNotifyOnDelete(){};

    void FireOnDelete()
    {
        if (m_pNotifyCallback)
        {
            m_pNotifyCallback->OnDelete(m_token);
            m_pNotifyCallback = NULL;
        }
    }


    void AddOnDeleteCallback(
        _In_ INotifyOnDeleteCallback *pNotifyCallback,
        _In_ const xstring_ptr& token
        )
    {
        // We only support one listener at a time
        FAIL_FAST_ASSERT(m_pNotifyCallback == NULL);
    
        m_pNotifyCallback = pNotifyCallback;
        m_token = token;
    }

    void RemoveOnDeleteCallback(
        _In_ INotifyOnDeleteCallback *pNotifyCallback
        )
    {
        FAIL_FAST_ASSERT(m_pNotifyCallback == pNotifyCallback);
    
        m_pNotifyCallback = NULL;
        m_token.Reset();
    }

    const xstring_ptr& GetNotifyOnDeleteToken() const
    {
        return m_token;
    }


protected:
    INotifyOnDeleteCallback *m_pNotifyCallback;
    xstring_ptr m_token;

};




#endif // #ifndef __PAL__NOTIFYONDELETE__
