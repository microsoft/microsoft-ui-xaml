// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <ContentPresenter.h>
#include "CControl.h"
class CContentControl : public CControl
{
protected:
    CContentControl(_In_ CCoreServices *pCore);
    ~CContentControl() override;

public:
    DECLARE_CREATE(CContentControl);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CContentControl>::Index;
    }

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

public:
    static _Check_return_ HRESULT Content(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    static _Check_return_ HRESULT CreateDefaultVisuals(_In_ CContentControl* pParent, _In_ CDependencyObject* pContent, _In_ const xstring_ptr& strBindingPath);
    static _Check_return_ HRESULT CreateDefaultTemplate(_Outptr_ CControlTemplate** ppTemplate, _In_ CDependencyObject* pTemplatedParent, _In_ const xstring_ptr& strBindingPath);

    void SetContentIsLogical(bool flag) { m_bContentIsLogical = flag; }

    bool IsContentLogical() const { return m_bContentIsLogical; }

    void SetContentIsTemplateboundManaged(bool bValue) { m_bContentIsTemplateboundManaged = bValue; }

    bool IsContentTemplateboundManaged() const { return m_bContentIsTemplateboundManaged; }

    _Check_return_ HRESULT DeferredContentRemovalLogic(_In_ CDependencyObject* pTarget);

    // Sets the Grid/ListViewItem-specific chrome. If we add CSelectorItem, move this and the associated field
    // there. This method takes and releases refs as required. Passing in NULL clears the chrome.
    _Check_return_ HRESULT SetGridViewItemChrome(_In_opt_ CListViewBaseItemChrome* pChrome);

    // Gets the Grid/ListViewItem-specific chrome.
    CListViewBaseItemChrome* GetGridViewItemChromeNoRef();

    void ConsiderContentPresenterForContentTemplateRoot(_In_ CContentPresenter* pCandidate, _In_ CValue& value);
    xref_ptr<CUIElement> GetContentTemplateRoot() const;

    CTextBlock *GetTextBlockNoRef();

protected:
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) override;
    bool CompareForCircularReference(_In_ CFrameworkElement *pTreeChild) final;

private:
    _Check_return_ HRESULT Invalidate(XUINT32 bClearChildren);
    // Fetches the child TextBlock of the default template if we are using the default template; null otherwise.
    xref_ptr<CTextBlock> GetTextBlockChildOfDefaultTemplate(_In_ bool fAllowNullContent);

private:
    xref::weakref_ptr<CContentPresenter> m_pTemplatePresenter;
    bool m_bContentIsLogical;
    bool m_bContentIsTemplateboundManaged;

public:
    bool m_bInOnApplyTemplate;
    CTransitionCollection* m_pContentTransitions;
    CValue m_content;
    CDataTemplate* m_pContentTemplate;  // Not really used by ContentControl it is just something for ContentPresenter to bind to.
    CDataTemplate* m_pSelectedContentTemplate;  // Not really used by ContentControl it is just something for ContentPresenter to bind to.

    //Minor hack: ListViewItems and GridViewItems support a special kind of rendering known as "chrome".
    // Ideally we'd define a chain of core types all the way up to ListViewBaseItem and store this field
    // there, but that would require quite a few extra classes (CSelectorItem, CGridViewItem, CListViewItem), so
    // for now we'll just put this here. Reconsider if we add chrome to other UIElements.
    // If this field is non-null, we can assume the object is a chromed ListViewBaseItem.
    // It's set in ListViewBaseItem::OnApplyTemplate.
    CListViewBaseItemChrome* m_pListViewBaseItemChrome;

};

