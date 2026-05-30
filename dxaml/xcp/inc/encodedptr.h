// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//      Warning: that this does not encode/decode anymore !!!

#if !defined(__ENCODEDPTR_H__)
#define __ENCODEDPTR_H__

// forward declare EncodedPtr so that we can declare extern gps
template<typename T> class EncodedPtr;

struct IPlatformServices;
extern EncodedPtr<IPlatformServices> gps;

// Note: The Naming is currently EncodedPtr but the encoding/decoding has been removed because
// it is not necessary anymore. The class itself is still present to avoid merging issues. It
// can be removed in the future (Tracking Bug 21457491).
template <typename T>
class EncodedPtr
{
public:
    EncodedPtr() : m_pTarget(0)
    {
    }

    void Set(T* pTarget)
    {
        m_pTarget = pTarget;
    }

    //
    // IsValid() will return TRUE only in cases where m_pTarget is usable (i.e.:  m_pTarget
    // has been set to some value and that value is not NULL);
    //
    bool IsValid()
    {
        return m_pTarget != nullptr;
    }

    T* Get()
    {
        return m_pTarget;
    }

    //
    // Operator overloads.
    //

    T* operator->()
    {
        return Get();
    }

private:
    T* m_pTarget;    
};

//
// An EncodedPtr that will delete the target when the
// instance of the EncodedPtr is deleted. This is used to protect
// the static instances of classes that other EncodedPtrs would point to.
//
template <typename T>
class EncodedPtrWithDelete : public EncodedPtr<T>
{
public:
    EncodedPtrWithDelete()              {}
    EncodedPtrWithDelete(T* pTarget)    { EncodedPtr<T>::Set(pTarget); }
    ~EncodedPtrWithDelete()             { delete EncodedPtr<T>::Get(); EncodedPtr<T>::Set((T*)0);  }
};

//
// An EncodedPtr that will delete the target when the
// instance of the EncodedPtr is deleted.
// It will also reset GPS
//
template <typename T>
class EncodedPtrWithDeleteAndgpsReset : public EncodedPtr<T>
{
public:
    EncodedPtrWithDeleteAndgpsReset()              {}
    EncodedPtrWithDeleteAndgpsReset(T* pTarget)    { EncodedPtr<T>::Set(pTarget); }
    ~EncodedPtrWithDeleteAndgpsReset()             { delete EncodedPtr<T>::Get(); EncodedPtr<T>::Set((T*)0); gps.Set(NULL); }
};


#endif
