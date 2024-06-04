// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


void CSizeUtil::Deflate(
    _Inout_ XSIZEF* pSize,
    _In_ const XTHICKNESS& thickness)
{
    pSize->width = MAX(0.0f, pSize->width - (thickness.left + thickness.right));
    pSize->height = MAX(0.0f, pSize->height - (thickness.top + thickness.bottom));
}

void CSizeUtil::Inflate(
    _Inout_ XSIZEF* pSize,
    _In_ const XTHICKNESS& thickness)
{
    pSize->width = MAX(0.0f, pSize->width + (thickness.left + thickness.right));
    pSize->height = MAX(0.0f, pSize->height + (thickness.top + thickness.bottom));
}

void CSizeUtil::Deflate(
    _Inout_ XSIZEF* pSize,
    _In_ const XSIZEF& size)
{
    pSize->width = MAX(0.0f, pSize->width - size.width);
    pSize->height = MAX(0.0f, pSize->height - size.height);
}

void CSizeUtil::Inflate(
    _Inout_ XSIZEF* pSize,
    _In_ const XSIZEF& size)
{
    pSize->width = MAX(0.0f, pSize->width + size.width);
    pSize->height = MAX(0.0f, pSize->height + size.height);
}

void CSizeUtil::Deflate(
    _Inout_ XRECTF* pRect,
    _In_ const XTHICKNESS& thickness)
{
    pRect->X += thickness.left;
    pRect->Y += thickness.top;
    pRect->Width = MAX(0.0f, pRect->Width - (thickness.left + thickness.right));
    pRect->Height = MAX(0.0f, pRect->Height - (thickness.top + thickness.bottom));
}

void CSizeUtil::Inflate(
    _Inout_ XRECTF* pRect,
    _In_ const XTHICKNESS& thickness)
{
    pRect->X -= thickness.left;
    pRect->Y -= thickness.top;
    pRect->Width = MAX(0.0f, pRect->Width + (thickness.left + thickness.right));
    pRect->Height = MAX(0.0f, pRect->Height + (thickness.top + thickness.bottom));
}

void CSizeUtil::CombineThicknesses(
    _Inout_ XTHICKNESS* pThickness,
    _In_ const XTHICKNESS& thickness)
{
    pThickness->top += thickness.top;
    pThickness->bottom += thickness.bottom;
    pThickness->left += thickness.left;
    pThickness->right += thickness.right;
}