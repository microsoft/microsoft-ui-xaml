// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDataTemplate;
class CItemCollection;
class CItemsPanelTemplate;
class CItemsPresenter;
class CPanel;
class CStyle;
class CTransitionCollection;

class CItemsControl : public CControl
{
protected:
    CItemsControl(_In_ CCoreServices *pCore)
        : CControl(pCore)
    {}

// private destructor prevents inheritance from CItemsControl
    ~CItemsControl() override;

public:
    // Creation method
    DECLARE_CREATE(CItemsControl);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CItemsControl>::Index;
    }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT CreationComplete() override;

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, _In_ LeaveParams params) override;

// CFrameworkElement overrides
protected:
    _Check_return_ HRESULT MeasureOverride(_In_ XSIZEF availableSize, _Inout_ XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final;
    void CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp) override;

// CItemsControl methods
public:
    _Check_return_ HRESULT static GetItems(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

private:
    friend class CItemCollection;
    _Check_return_ HRESULT InvalidateItemsHost(_In_ bool bHostIsReplaced);

    friend class CItemsPresenter;
    _Check_return_ CItemsPresenter* RegisterItemsPresenter(
        _In_ CItemsPresenter* pItemsPresenter);

    _Check_return_ HRESULT ValidateItemsHost();

    _Check_return_ HRESULT EnsureItemCollection( _In_ CDependencyObject *pNamescopeOwner = nullptr, _In_ EnterParams *pParams = nullptr );

// CItemsControl public fields
public:

    // The store for the Items property
    // Ensure this is allocated by calling EnsureItemCollection
    CItemCollection*        m_pItemCollection       = nullptr;

    // The panel that displays the element trees which wrap the items
    CPanel*                 m_pItemsHost            = nullptr;

    // The template to use to display an item
    CDataTemplate*          m_pItemTemplate         = nullptr;

    // The path into the ItemsSource of the item to be displayed
    xstring_ptr             m_strDisplayMemberPath;

    // The template that knows how to create the ItemHost panel.
    CItemsPanelTemplate*    m_pItemsPanelTemplate   = nullptr;

    CTransitionCollection*  m_pItemContainerTransitions = nullptr;

private:
    // The child that hosts the panel, that hosts the visual items.
    CItemsPresenter*        m_pItemsPresenter       = nullptr;

    // This is set when ItemsHost has been set from the managed side.
    // Setter of ItemsHost property on ItemsControl has internal access
    // and we use it only for replacing a standard SL2 ListBox host panel
    // which was StackPanel to VirtualizingStackPanel
    bool                    m_bItemsHostIsSetFromManaged    = false;

public:
    // This is set if we need to regenerate item visual trees
    bool                    m_bItemsHostInvalid     = true;

};

// Piggybacking in this file for now. This should be a codegen'ed class, except we need to make a check in the dtor.
class CMenuFlyoutPresenter: public CItemsControl
{
protected:
    CMenuFlyoutPresenter(_In_ CCoreServices *pCore)
        : CItemsControl(pCore)
    {
        SetIsCustomType();
    }

    ~CMenuFlyoutPresenter() override;

public:
    DECLARE_CREATE(CMenuFlyoutPresenter);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::MenuFlyoutPresenter;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

};

