// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "CIconSource.g.h"

//------------------------------------------------------------------------
//
//  Class:  CIconElement
//
//  Synopsis:
//      The base class for all icon types.
//
//------------------------------------------------------------------------

class CIconElement : public CFrameworkElement
{
protected:
    CIconElement(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
    {
    }

    // Inherited property support
    _Check_return_ HRESULT PullInheritedTextFormatting() override;

    _Check_return_ HRESULT MeasureOverride(
        XSIZEF availableSize,
        XSIZEF& desiredSize
        ) final;

    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) final;

    _Check_return_ HRESULT AddChildElement(CUIElement* pChildElement);
    _Check_return_ HRESULT RemoveChildElement();
    _Check_return_ CUIElement* GetChildElementNoRef();

    // Fetches the value of a specified property owned by this IconElement,
    // and sets it as the value of a property owned by our first (and only)
    // child. If fClearParentAssociation is true and the value is a
    // DependencyObject, its existing association with the IconElement
    // will be cleared so that it can be re-associated with the child
    // element.
    _Check_return_ HRESULT UpdateLinkedChildProperty(
        KnownPropertyIndex parentPropertyIndex,
        KnownPropertyIndex childPropertyIndex,
        bool fClearParentAssociation = 0
    );

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIconElement>::Index;
    }

    bool CanHaveChildren() const final { return true; }
    bool GetIsLayoutElement() const final { return true; }
};


//------------------------------------------------------------------------
//
//  Class:  CSymbolIcon
//
//  Synopsis:
//      An icon that displays a member of the Symbol enum.
//
//------------------------------------------------------------------------

class CSymbolIcon final : public CIconElement
{
private:
    CSymbolIcon(_In_ CCoreServices *pCore)
        : CIconElement(pCore)
        , m_nSymbol(DirectUI::Symbol::Emoji)
    {
    }

   _Check_return_ HRESULT UpdateChildTextBlockText();

   static WCHAR ConvertSymbolValueToGlyph(const XUINT32 symbol);

protected:
    // Overridden to set default values for font properties
    _Check_return_ HRESULT InitInstance() override;

    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSymbolIcon>::Index;
    }

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    void SetFontSize(_In_ XFLOAT fontSize);

    DirectUI::Symbol m_nSymbol; // The symbol that this icon will display
    XFLOAT m_fontSize;
};


//------------------------------------------------------------------------
//
//  Class:  CFontIcon
//
//  Synopsis:
//      An icon that displays a character glyph from a specified font.
//
//------------------------------------------------------------------------

class CFontIcon final : public CIconElement
{
private:
    CFontIcon(_In_ CCoreServices *pCore)
        : CIconElement(pCore)
        , m_pScaleTransform(nullptr)
    {
    }

   ~CFontIcon() override;
   
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;

protected:
    // Inherited property support
    _Check_return_ HRESULT PullInheritedTextFormatting() override;

    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;

public:
    DECLARE_CREATE(CFontIcon);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CFontIcon>::Index;
    }

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    xstring_ptr m_strGlyph; // The font glyph that this icon will display

private:
    _Check_return_ HRESULT ApplyScaleTransformForFlowDirection();

    CScaleTransform* m_pScaleTransform;
};


//------------------------------------------------------------------------
//
//  Class:  CPathIcon
//
//  Synopsis:
//      An icon that displays a shape specified using Path geometry.
//
//------------------------------------------------------------------------

class CPathIcon final : public CIconElement
{
private:
    CPathIcon(_In_ CCoreServices *pCore)
        : CIconElement(pCore)
        , m_pData(NULL)
    {
    }

   ~CPathIcon() override;

protected:
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPathIcon>::Index;
    }

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    CGeometry* m_pData; // The path data that this icon will display
};


//------------------------------------------------------------------------
//
//  Class:  CBitmapIcon
//
//  Synopsis:
//      An icon that displays an image mask.
//
//------------------------------------------------------------------------

class CBitmapIcon : public CIconElement
{
private:
    CBitmapIcon(_In_ CCoreServices *pCore)
        : CIconElement(pCore)
        , m_strSource()
        , m_showAsMonochrome(true)
        , m_pWriteableBitmap(NULL)
        , m_pPixelBuffer(NULL)
        , m_nWidth(0)
        , m_nHeight(0)
        , m_pBitmapImage(NULL)
        , m_pBitmapImageReportCallback(NULL)
    {
    }

   ~CBitmapIcon() override;

protected:
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;

    _Check_return_ HRESULT ReloadSourceImage();

    _Check_return_ HRESULT CreateWritableBitmapFromSource();

    _Check_return_ HRESULT ApplyForegroundColorToImage(_In_opt_ const CSolidColorBrush* pBrush);

    void ReleaseBitmapImageResources();
    void ReleaseWriteableBitmapResources();

public:
    DECLARE_CREATE(CBitmapIcon);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBitmapIcon>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    void FireImageDownloadEvent(_In_ XFLOAT downloadProgress);

    _Check_return_ HRESULT FireImageFailed(_In_ XUINT32 iErrorCode);

    _Check_return_ HRESULT FireImageOpened();

    // Public Fields
    xstring_ptr           m_strSource;
    bool                  m_showAsMonochrome;

private:
    CBitmapImage*               m_pBitmapImage;
    CBitmapImageReportCallback* m_pBitmapImageReportCallback;

    CWriteableBitmap*           m_pWriteableBitmap; // The image with the current foreground color applied to it.

    XUINT32*            m_pPixelBuffer;
    XUINT32             m_nWidth;
    XUINT32             m_nHeight;
};


//------------------------------------------------------------------------
//
//  Class:  CIconSourceElement
//
//  Synopsis:
//      An icon that puts the contents of its IconSource into the visual tree.
//
//------------------------------------------------------------------------

class CIconSourceElement final : public CIconElement
{
private:
    CIconSourceElement(_In_ CCoreServices *pCore)
        : CIconElement(pCore)
    {
    }

    ~CIconSourceElement() override;

protected:
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;

public:
    DECLARE_CREATE(CIconSourceElement);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CIconSourceElement>::Index;
    }

    _Check_return_ HRESULT SwapIconElements();
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    xref_ptr<CIconSource> m_iconSource;
    xref_ptr<CIconElement> m_iconElement;
};