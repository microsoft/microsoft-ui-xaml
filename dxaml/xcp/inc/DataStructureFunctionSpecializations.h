// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DataStructureFunctionProvider.h>

class CDependencyObject;
class CUIElement;
class CTransition;
struct ID3D11Device;
class CSurfaceImageSource;
class DCompSurface;
struct IDirectManipulationViewport;
class CBitmapImage;
class CImageSource;

template<>
struct DataStructureFunctionProvider<XPOINT>
{
    static XUINT32 Hash(const XPOINT& pt)
    {
        return (pt.x ^ (pt.y << 16));
    }

    static bool AreEqual(const XPOINT& lhs, const XPOINT& rhs)
    {
        return ((lhs.x == rhs.x) && (lhs.y == rhs.y));
    }
};

template<>
struct DataStructureFunctionProvider<XSIZEF>
{
    static XUINT32 Hash(const XSIZEF& value)
    {
        return ((int)value.width ^ ((int)value.height << 16));
    }

    static bool AreEqual(const XSIZEF& lhs, const XSIZEF& rhs)
    {
        return ((lhs.width == rhs.width) && (lhs.height == rhs.height));
    }
};

// This macro defines a DataStructureFunctionProvider for a pointer to an object

#define DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(type)               \
template<>                                                                  \
struct DataStructureFunctionProvider<type*>                                 \
{                                                                           \
    static XUINT32 Hash(const type* const& data)                            \
    {                                                                       \
        return (XUINT32)(( (XUINT64)data >> 2) % 23);                       \
    }                                                                       \
    static bool AreEqual(const type* const& lhs, const type* const& rhs)   \
    {                                                                       \
        return lhs == rhs;                                                  \
    }                                                                       \
}

DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(CDependencyObject);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(CUIElement);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(CTransition);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(ID3D11Device);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(CSurfaceImageSource);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(DCompSurface);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(IDirectManipulationViewport);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(CBitmapImage);
DEFINE_DATA_STRUCTURE_FUNCTION_PROVIDER_POINTER(CImageSource);