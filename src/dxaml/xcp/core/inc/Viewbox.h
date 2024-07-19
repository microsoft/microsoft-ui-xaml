// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Synopsis:
//      Base class for objects that contain only one child.

#pragma once

class CViewbox final : public CFrameworkElement
{
private:
    CViewbox(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
    {}

   ~CViewbox() override;

    _Check_return_ XSIZEF ComputeScaleFactor(XSIZEF availableSize, XSIZEF contentSize);

    CScaleTransform* m_pScaleTransform = nullptr;

public:
    // Creation method
    DECLARE_CREATE(CViewbox);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CViewbox>::Index;
    }

    bool GetIsLayoutElement() const final { return true; }

    bool CanHaveChildren() const final { return true; }

    _Check_return_ HRESULT AddChild(_In_ CUIElement* pChild) override;

    _Check_return_ HRESULT InitInstance() override;

    // Method to set the Child property
    static _Check_return_ HRESULT Child(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    virtual _Check_return_ HRESULT SetChild(
        _In_ CUIElement* pContent);

    virtual _Check_return_ HRESULT GetChild(
        _Outptr_ CUIElement** ppContent);

    DirectUI::StretchDirection m_stretchDirection   = DirectUI::StretchDirection::Both;
    DirectUI::Stretch m_stretch                     = DirectUI::Stretch::Uniform;
    CBorder* m_pContainerVisual                     = nullptr;

protected:
    _Check_return_ HRESULT MeasureOverride(
        XSIZEF availableSize,
        XSIZEF& desiredSize) override;

    _Check_return_ HRESULT ArrangeOverride(
        XSIZEF finalSize,
        XSIZEF& newFinalSize) override;
};