// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xref_ptr.h"
#include <weakref_ptr.h>

class CUIElement;
class CRectangleGeometry;
class CSplitViewTemplateSettings;

class CSplitView final : public CControl
{
private:
    CSplitView(_In_ CCoreServices *pCore)
        : CControl(pCore)
    {
    }

    ~CSplitView() override;

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

public:
    DECLARE_CREATE(CSplitView);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSplitView>::Index;
    }

    _Check_return_ HRESULT InitInstance() override;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    // CFrameworkElement overrides.
    _Check_return_ HRESULT OnApplyTemplate() override;

    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;

    // Implementation
    bool CanLightDismiss();
    bool IsLightDismissible();
    _Check_return_ HRESULT TryCloseLightDismissiblePane();

    _Check_return_ HRESULT OnPaneOpening();
    _Check_return_ HRESULT OnPaneClosing();
    _Check_return_ HRESULT OnPaneClosed();

    void OnCancelClosing();

    CDependencyObject* ProcessTabStop(
        _In_ bool isForward,
        _In_opt_ CDependencyObject* pFocusedElement,
        _In_opt_ CDependencyObject* pCandidateTabStopElement
        );

    void GetFirstFocusableElementFromPane(_Outptr_ CDependencyObject** firstFocusable);
    void GetLastFocusableElementFromPane(_Outptr_ CDependencyObject** lastFocusable);

public:
    // Public fields
    bool m_isPaneOpen{ false };
    CSplitViewTemplateSettings* m_pTemplateSettings{ nullptr };

private:

    double GetOpenPaneLength();

    _Check_return_ HRESULT UpdateTemplateSettings();

    _Check_return_ HRESULT RegisterEventHandlers();
    _Check_return_ HRESULT UnregisterEventHandlers();

    _Check_return_ HRESULT SetFocusToPane();
    _Check_return_ HRESULT RestoreSavedFocusElement();

    static _Check_return_ HRESULT OnKeyDown(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);
    static _Check_return_ HRESULT OnLightDismissLayerPointerReleased(_In_ CDependencyObject* sender, _In_ CEventArgs* eventArgs);

    xref_ptr<CRectangleGeometry> m_spPaneClipRectangle;

    xref_ptr<CUIElement> m_spPaneRoot;
    xref_ptr<CUIElement> m_spContentRoot;
    xref_ptr<CUIElement> m_spLightDismissLayer;

    xref::weakref_ptr<CDependencyObject> m_spPrevFocusedElementWeakRef;
    DirectUI::FocusState m_prevFocusState{ DirectUI::FocusState::Unfocused };

    bool m_isPaneClosingByLightDismiss{ false };
    double m_paneMeasuredLength{ 0.0 };

}; // class CSplitView
