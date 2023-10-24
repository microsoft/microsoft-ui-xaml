// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CSizeUtil
{
public:
    static void Deflate(
        _Inout_ XSIZEF* pSize,
        _In_ const XTHICKNESS& thickness);

    static void Inflate(
        _Inout_ XSIZEF* pSize,
        _In_ const XTHICKNESS& thickness);

    static void Deflate(
        _Inout_ XSIZEF* pSize,
        _In_ const XSIZEF& size);

    static void Inflate(
        _Inout_ XSIZEF* pSize,
        _In_ const XSIZEF& size);

    static void Deflate(
        _Inout_ XRECTF* pRect,
        _In_ const XTHICKNESS& thickness);

    static void Inflate(
        _Inout_ XRECTF* pRect,
        _In_ const XTHICKNESS& thickness);

    static void CombineThicknesses(
        _Inout_ XTHICKNESS* pThickness,
        _In_ const XTHICKNESS& thickness);
};