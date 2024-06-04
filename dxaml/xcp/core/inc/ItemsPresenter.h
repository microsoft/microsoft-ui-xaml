// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CItemsPresenter final : public CFrameworkElement
{
public:
    DECLARE_CREATE(CItemsPresenter);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CItemsPresenter>::Index;
    }

    bool CanHaveChildren() const final { return true; }
    bool GetIsLayoutElement() const final { return true; }
    _Check_return_ HRESULT AddChild(_In_ CUIElement *pChild) override;

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;
    XTHICKNESS GetPadding() const final {return m_padding;}

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peer has state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

protected:
    CItemsPresenter(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
    {}

    ~CItemsPresenter() override;

    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;
    _Check_return_ HRESULT OnApplyTemplate() override;

private:
    _Check_return_ HRESULT AddItemsHost(_In_ CItemsControl* pItemsControl);

public:
    CItemsPanelTemplate*    m_pItemsPanelTemplate   = nullptr;
    XTHICKNESS              m_padding               = {};       // ItemsPresenter.Padding property storage.
};
