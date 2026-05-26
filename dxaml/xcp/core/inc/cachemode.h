// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Use of this source code is subject to the terms of the Microsoft
// premium shared source license agreement under which you licensed
// this source code. If you did not accept the terms of the license
// agreement, you are not authorized to use this source code.
// For the terms of the license, please see the license agreement
// signed by you and Microsoft.
// THE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//  Abstract:
//      CacheMode and derived classes
//
//  Class:  CCacheMode
//
//  Synopsis:
//      A class representing a means for caching a realization of an
//      element.

enum CacheModeType
{
    CMT_BitmapCache
};

class CCacheMode : public CMultiParentShareableDependencyObject
{
public:
    CCacheMode(_In_ CCoreServices* pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

    CCacheMode() = delete;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCacheMode>::Index;
    }

    virtual CacheModeType GetCacheModeType() = 0;
};

//------------------------------------------------------------------------
//
//  Class:  CBitmapCache
//
//  Synopsis:
//      A cache mode requesting that the element be cached as a bitmap
//      realization.
//
//------------------------------------------------------------------------
class CBitmapCache final : public CCacheMode
{
protected:
    CBitmapCache(_In_ CCoreServices* pCore)
        : CCacheMode(pCore)
    {}

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBitmapCache>::Index;
    }

    CacheModeType GetCacheModeType() override
    {
        return CMT_BitmapCache;
    }
};