// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//------------------------------------------------------------------------
//
//  Template:  CorePeggedPtr<T>
//
//  Synopsis:
//      Used as a reference to keep both a COM reference and a peg on an CDependencyObject.
//
//      NOTE:  This should only be used for roots, where a peer reference (AddPeerReferenceToItem,
//      SetPeerReferenceToProperty) isn't appropriate.
//
//------------------------------------------------------------------------

// Prevent AddRef/Release from being callable from an operator->

template <typename T>
class RemoveIObject
: public T
{
    private:
        XUINT32 AddRef() override;
        XUINT32 Release() override;
};

template<class T>
class CorePeggedPtr
{
private:

    T *m_pValue;

public:

    CorePeggedPtr()
    {
        m_pValue = nullptr;
    }

    void Clear()
    {
        if( m_pValue )
        {
            m_pValue->UnpegManagedPeer();
            m_pValue->Release();
            m_pValue = nullptr;
        }

    }

    ~CorePeggedPtr()
    {
        Clear();
    }


    T* Get() const
    {
        return m_pValue;
    }

    HRESULT Set( _In_ T* pValue )
    {
        Clear();

        if( pValue )
        {
            IFC_RETURN( pValue->PegManagedPeer() );
            m_pValue = pValue;
            AddRefInterface(pValue);

        }

        return S_OK;
    }

    // Set with a TryPegPeer(), rather than PegManagedPeer().  If we can't get the peg,
    // then 'operator bool' will be false.
    void TrySet( _In_ T* pValue )
    {
        Clear();

        if( pValue )
        {
            bool pegged = false;
            bool isPendingDelete = false;

            pValue->TryPegPeer(&pegged, &isPendingDelete);
            if( !pegged )
                return;

            m_pValue = pValue;
            AddRefInterface( pValue );
        }
    }


    operator bool () const
    {
        return m_pValue != nullptr;
    }

    RemoveIObject<T>* operator-> () const
    {
        return static_cast<RemoveIObject<T>*>( Get() );
    }
};
