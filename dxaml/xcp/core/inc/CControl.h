// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpmath.h"
#include <framework.h>
#include <fwd/Microsoft.UI.Xaml.controls.h>

class CIsEnabledPropertyChangedEventArgs;
class CDependencyProperty;
class CDependencyObject;

namespace DirectUI
{
    enum class InputValidationVisualMode;
}
struct TargetPropertySubscription
{
    TargetPropertySubscription(
        const CDependencyProperty* const targetProperty,
        CDependencyObject* const targetObject,
        const bool runtimeCheck) noexcept
        : m_targetProperty(targetProperty)
        , m_targetObject(targetObject)
        , m_runtimeCheck(runtimeCheck)
    {}

    // The property that receives the updates
    const CDependencyProperty* m_targetProperty;

    // The object that will receive the notifications
    CDependencyObject*         m_targetObject;

    // Does this subscription requires checking the type before sending the data?
    bool                       m_runtimeCheck;

    // Used to distinguish between elements which were already updated during initial ApplyTemplate and deferred elements which were added after.
    bool                       m_hadInitialUpdate  = false;
};

struct PropertySubscriptions
{
    PropertySubscriptions() = default;

    PropertySubscriptions(const CDependencyProperty* const sourceProperty)
        : m_sourceProperty(sourceProperty)
    {}

    // The property to which the targets are subscribed
    const CDependencyProperty*              m_sourceProperty {};
    std::vector<TargetPropertySubscription> m_subscriptions;
};

// Indicates the scope for refreshing template binding.
enum class TemplateBindingsRefreshType
{
    // All of subscriptions should be refreshed.
    All,

    // Only ones, which have m_hadInitialUpdate flag set to false.
    WithoutInitialUpdate
};

// Native class to handle native side interactions for custom controls.
// At time of initial creation, custom controls can only be done via
// managed code by deriving from the managed code peer of this class.
class CControl
    : public CFrameworkElement
{
public:
    DECLARE_CREATE(CControl);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CControl>::Index;
    }

    // All Controls can have children via template expansion.
    // Some derived classes also expose element children directly (e.g. UserControl.Content).
    bool CanHaveChildren() const final
    {
        return true;
    }

    bool GetIsLayoutElement() const final { return true; }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // Inherited property support
    _Check_return_ HRESULT PullInheritedTextFormatting() final;

    // Get the implementation root by managed peer.
    _Check_return_ HRESULT GetImplementationRoot(_Outptr_result_maybenull_ CUIElement** ppResult);
    xref_ptr<CUIElement> GetImplementationRoot();

    _Check_return_ HRESULT GetBuiltInStyle(_Outptr_ CStyle **ppStyle);

    _Check_return_ HRESULT CreationComplete() override;
    _Check_return_ HRESULT CoerceIsEnabled(_In_ bool bIsEnabled, _In_ bool bCoerceChildren) final;

    void SuppressIsEnabled(_In_ bool bSuppress);
    xref_ptr<CDependencyObject> GetTemplateChild(_In_ const xstring_ptr_view& strChildName) const;
    xref_ptr<CControlTemplate> GetTemplate() const final;

    _Check_return_ HRESULT GetValueFromBuiltInStyle(
        _In_ const CDependencyProperty* dp,
        _Out_ CValue* pValue,
        _Out_ bool* gotValue);

    bool IsFocusableForFocusEngagement() final
    {
        return IsFocusEngagementEnabled() && LastInputGamepad();
    }

    bool IsFocusEngagementEnabled() const
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::Control_IsFocusEngagementEnabled, &result));
        return result.AsBool();
    }

    bool IsFocusEngaged() const
    {
        CValue result;
        VERIFYHR(GetValueByIndex(KnownPropertyIndex::Control_IsFocusEngaged, &result));
        return result.AsBool();
    }

    _Check_return_ HRESULT UpdateEngagementState(bool engaging);

    _Check_return_ HRESULT RemoveFocusEngagement();

    _Check_return_ HRESULT SetFocusEngagement();

    _Check_return_ HRESULT SubscribeToPropertyChanges(
        _In_ const CDependencyProperty* sourceProperty,
        _In_ CDependencyObject* target,
        _In_ const CDependencyProperty* targetProperty) final;

    void RemoveTemplateBinding(
        _In_ const CDependencyObject* const targetObject,
        _In_ const CDependencyProperty* const targetProperty);

    bool IsUpdatingBindings() final { return m_fIsUpdatingBindings; }
    _Check_return_ HRESULT RefreshTemplateBindings(_In_ TemplateBindingsRefreshType refreshType);
    void RequestTemplateBindingRefresh() { m_fRequestTemplateBindingRefresh = TRUE; }
    bool NeedsTemplateBindingRefresh() const { return m_fRequestTemplateBindingRefresh; }
    bool IsDefaultStyleApplying() const { return m_fIsDefaultStyleApplying; }
    _Check_return_ HRESULT OnApplyTemplate() override;

    _Check_return_ bool SupportsBuiltInStyles();

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Control peers have state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    static _Check_return_ HRESULT Enabled(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    XTHICKNESS GetPadding() const final {return m_padding;}

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;
    _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize) override;
    _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize) override;

    void SetIsUpdatingBindings(_In_ bool fIsUpdatingBinding) final { m_fIsUpdatingBindings = fIsUpdatingBinding; }

    _Check_return_ HRESULT SubscribeToCoreProperty(
        _In_ const CDependencyProperty* const sourceProperty,
        _In_ CDependencyObject* const targetObject,
        _In_ const CDependencyProperty* const targetProperty);

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    _Check_return_ HRESULT RefreshSubscriptions(
        _In_ PropertySubscriptions* const subscriptions,
        _In_ const CValue& value,
        _In_ TemplateBindingsRefreshType refreshType);

    PropertySubscriptions* LookupPropertySubscriptions(
        _In_ const CDependencyProperty* const sourceProperty);

    PropertySubscriptions* EnsurePropertySubscriptions(
        _In_ const CDependencyProperty* const sourceProperty);

    _Check_return_ HRESULT RaiseIsEnabledChangedEvent(_In_ CValue *pValue) final;

public:
    void RaiseValidationErrorEvent(
        _In_ const DirectUI::InputValidationErrorEventAction action,
        _In_ xaml_controls::IInputValidationError* error);

    bool HasErrors();
protected:
    CControl(_In_ CCoreServices *pCore)
        : CFrameworkElement(pCore)
        , m_fIsUpdatingBindings(FALSE)
        , m_fIsBuiltInStyleApplied(FALSE)
        , m_fSuppressIsEnabled(FALSE)
        , m_fRequestTemplateBindingRefresh(FALSE)
        , m_fIsDefaultStyleApplying(FALSE)
    {
        m_isTabStop = true;
    }

    ~CControl() override;

private:
    _Check_return_ HRESULT ApplyBuiltInStyle();

    _Check_return_ HRESULT PropagateInheritedProperty(_In_ CUIElement *pUIElement, _In_ const CDependencyProperty *pdp);

    void ClearPropertySubscriptions();

    static bool PropertyTypesCompatible(
        _In_ const CDependencyProperty* const sourceProperty,
        _In_ const CDependencyProperty* const targetProperty,
        _Out_ bool* pfRequiresTypeCheck);

    bool LastInputGamepad();

    KnownPropertyIndex GetTargetDescriptionProperty() const;

    bool EnsureHasErrors(bool* hadErrors = nullptr);
    bool IsValidationEnabled() const;

    bool ShowErrorsInline() const;

    DirectUI::InputValidationKind GetErrorVisualKind() const;
    DirectUI::InputValidationMode GetErrorVisualMode() const;

    xref_ptr<CDataTemplate> GetValidationErrorTemplate() const;

    KnownPropertyIndex GetTargetHasErrorsProperty() const;
    KnownPropertyIndex GetTargetErrorsProperty() const;
    KnownPropertyIndex GetTargetErrorTemplateProperty() const;
    KnownPropertyIndex GetTargetValidationModeProperty() const;
    KnownPropertyIndex GetTargetValidationKindProperty() const;
    KnownEventIndex GetTargetValidationErrorEvent() const;

    xref_ptr<CInputValidationCommand> TryGetInputValidationCommand() const;

    void RaiseHasValidationErrorsChanged(KnownEventIndex event, bool newValue);
public:
    void EnsureValidationVisuals();
    void EnsureErrors();
    void DeferErrors();
protected:
    KnownPropertyIndex GetTargetValidationCommandProperty() const;

public:
    void EnsureDescription();
    void ClearDescription();
public:

    // ------------------------------------------------------------------------
    // CControl Public Fields
    // ------------------------------------------------------------------------

    XTHICKNESS                          m_padding                       = {};
    XTHICKNESS                          m_borderThickness               = {};
    DirectUI::HorizontalAlignment       m_horizontalContentAlignment    = DirectUI::HorizontalAlignment::Center;
    DirectUI::VerticalAlignment         m_verticalContentAlignment      = DirectUI::VerticalAlignment::Center;

public:
    CControlTemplate                   *m_pTemplate                     = nullptr;
    CBrush                             *m_pBackground                   = nullptr;
    CBrush                             *m_pBorderBrush                  = nullptr;
    CDependencyObject                  *m_pDefaultStyleKey              = nullptr;

protected:
    // ------------------------------------------------------------------------
    // CControl Protected Fields
    // ------------------------------------------------------------------------
    bool m_fIsUpdatingBindings              : 1;

private:
    // Private fields placed here to pack better on x64
    bool   m_fIsBuiltInStyleApplied         : 1;
    bool   m_fSuppressIsEnabled             : 1;
    bool   m_fRequestTemplateBindingRefresh : 1;   // Flag used by deferred element to signal that template bindings should be reevaluated after an element was realized in control template.
    bool   m_fIsDefaultStyleApplying        : 1;

private:
    // Number of preallocated subscriptions in the vector.
    static constexpr std::size_t c_preallocatedSubscriptions = 4;

protected:
    std::unique_ptr<std::vector<PropertySubscriptions>> m_propertySubscriptions;

private:
    // ------------------------------------------------------------------------
    // CControl private Fields
    // ------------------------------------------------------------------------
    CStyle*                             m_pBuiltInStyle                 = nullptr;
};

class CUserControl : public CControl
{
public:
    DECLARE_CREATE(CUserControl);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CUserControl>::Index;
    }

    bool AreChildrenInLogicalTree() final { return true; }

    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    _Check_return_ HRESULT AddChild(_In_ CUIElement * pChild) override;

    _Check_return_ HRESULT static Content(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT SetContent(_In_ CUIElement* pContent);
    _Check_return_ HRESULT GetContent(_Outptr_ CUIElement** ppContent);


    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    void SetAllowForBackgroundRender(bool isAllowForBackgroundRender)
    {
        m_isAllowForBackgroundRender = isAllowForBackgroundRender;
        CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
    }

    bool IsAllowForBackgroundRender() const
    {
        return m_isAllowForBackgroundRender;
    }

protected:
    CUserControl(_In_ CCoreServices *pCore)
        : CControl(pCore)
    {
        m_isTabStop = false;
    }

    // We don't want to do anything to expand templates in a UserControl. This would
    // only happen if an empty UserControl (or subclass) was in the tree.
    _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals) final { fAddedVisuals = false; return S_OK; }

private:
    // Page is the only UserControl that renders its Background.
    bool m_isAllowForBackgroundRender = false;
};
