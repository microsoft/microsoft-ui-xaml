// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DWrite based implementation of IFssFontFamily.

#pragma once

namespace PALText
{
    struct IFontFace;
}

//---------------------------------------------------------------------------
//
//  DWriteFontFamily
//
//  Wraps a DWrite Font Collection.
//
//---------------------------------------------------------------------------
class DWriteFontFamily : public CXcpObjectBase<PALText::IFontFamily, CXcpObjectAddRefPolicy>
{
public:

    // Initializes a new instance of the DWriteFontFamily class.
    DWriteFontFamily(
        _In_ IDWriteFontFamily *pDWriteFontFamily,
        _In_ bool isShapingOptimizable
        );

    // For a given physical font typeface determines which glyph typeface to
    // use for rendering the missing glyph.
    HRESULT LookupNominalFontFace(
        _In_ XUINT32 weight,
        _In_ XUINT32 style,
        _In_ XUINT32 stretch,
        _Outptr_ PALText::IFontFace **ppFontFace
        ) override;

    // Gets DWrite's peer associated with this object.
    IDWriteFontFamily* GetFontFamily() const;

    // Returns whether we can bypass full shaping.
    // This currently only applies to Segoe UI.
    bool CanOptimizeShaping() const;

private:

    // DWrite's peer associated with this object.
    IDWriteFontFamily *m_pDWriteFontFamily;

    // A flag to indicate whether we can bypass full shaping.
    // This currently only applies to Segoe UI.
    bool m_canOptimizeShaping;

    // Release resources associated with the DWriteFontFamily.
    ~DWriteFontFamily() override;

};

inline IDWriteFontFamily* DWriteFontFamily::GetFontFamily() const
{
    return m_pDWriteFontFamily;
}

inline bool DWriteFontFamily::CanOptimizeShaping() const
{
    return m_canOptimizeShaping;
}
