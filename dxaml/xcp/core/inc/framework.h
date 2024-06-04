// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <enumdefs.h>
#include <Indexes.g.h>
#include <FeatureFlags.h>
#include <uielement.h>

class CCoreServices;
struct FrameworkElementGroupStorage;
class CRectangleGeometry;
class CControlTemplate;
struct TemplateBindingData;
class CStyle;
struct AcceleratedBrushParams;

// Specified who was an implicit style provider.
// None - no implicit style has been picked up
// Self - implicit style has been picked up from element's resources
// Parent - implicit style has been picked up from one of visual parents resources
// AppWhileNotInTree - implicit style has been picked up from the Application's resources while element was out of the Visual Tree
// AppWhileNotInTree - implicit style has been picked up from the Application's resources while element was it the Visual Tree
enum class ImplicitStyleProvider : uint8_t
{
    None,
    Self,
    Parent,
    AppWhileNotInTree,
    AppWhileInTree,
};

enum class FocusVisualType : uint8_t
{
    Primary,
    Secondary,
};

namespace DirectUI
{
    class DxamlCoreTestHooks;
}

class CFrameworkElement : public CUIElement
{
    friend class DirectUI::DxamlCoreTestHooks;

public:
#if defined(__XAML_UNITTESTS__)
    CFrameworkElement()  // !!! FOR UNIT TESTING ONLY !!!
        : CFrameworkElement(nullptr)
    {}
#endif

private:
    _Check_return_ HRESULT ApplyStyle();
    _Check_return_ HRESULT EnsureImplicitStyle(bool isLeavingParentStyle);
    _Check_return_ HRESULT EnsureClassName();

    //Used in EnterImpl, LeaveImpl and SetValue for the IsTemplateFocusTarget attached property
    _Check_return_ HRESULT UpdateFocusAncestorsTarget(_In_ bool shouldSet);

protected:
    CFrameworkElement(_In_ CCoreServices *pCore);

    ~CFrameworkElement() override;

    _Check_return_ HRESULT ResetReferenceFromChild(_In_ CDependencyObject* child) override;

    _Check_return_ HRESULT OnStyleChanged(
        _In_ CStyle *pStyleOld,
        _In_ CStyle *pStyleNew,
        _In_ BaseValueSource baseValueSource);

    _Check_return_ HRESULT AcceleratedBorderRenderCommon(
        _In_ const SharedRenderParams& sharedRP,
        _In_ const D2DRenderParams& d2dRP,
        _In_opt_ IPALAcceleratedGeometry *pPALBackgroundGeometry,
        _In_opt_ IPALAcceleratedBrush *pPALBackgroundBrush,
        _In_opt_ AcceleratedBrushParams *pBackgroundBrushParams,
        _In_opt_ IPALAcceleratedGeometry *pPALBorderGeometry,
        _In_opt_ IPALAcceleratedBrush *pPALBorderBrush,
        _In_opt_ AcceleratedBrushParams *pBorderBrushParams);

    _Check_return_ HRESULT CreateBorderGeometriesAndBrushClipsCommon(
        const bool renderCollapsedMask,
        _In_ const D2DPrecomputeParams &cp,
        _In_ const CMILMatrix *pMyAccumulatedTransform,
        _In_ AcceleratedBrushParams *pPALBackgroundBrushParams,
        _In_ AcceleratedBrushParams *pPALBorderBrushParams,
        _Outptr_opt_ IPALAcceleratedGeometry **ppPALBackgroundGeometry,
        _Outptr_opt_ IPALAcceleratedGeometry **ppPALBorderGeometry);

public:
    DECLARE_CREATE(CFrameworkElement);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CFrameworkElement>::Index;
    }

    xref_ptr<CStyle> GetActiveStyle() const;
    xstring_ptr GetClassName();

    // called to inform the FrameworkElement that the mutable Style Setter providing the value
    // for the property 'propertyIndex' has a new value
    void NotifyMutableStyleValueChanged(KnownPropertyIndex propertyIndex);

    // called when the parser has finished parsing the tag associated
    // with this element.  All properties will have been set, and all
    // children will have been added by this point.
    _Check_return_ HRESULT CreationComplete() override;

    // The Mentor of an FE is always itself
    _Ret_maybenull_ CDependencyObject* GetMentor() final
    {
        return this;
    }

    _Check_return_ CDependencyObject* GetInheritanceParentInternal(
        _In_opt_ bool fLogicalParent = false
    ) const final;


// CUIElement overrides
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

    // Inherited property support
    _Check_return_ HRESULT PullInheritedTextFormatting() override;
    void EvaluateIsRightToLeft() final;

    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

    _Check_return_ CDependencyObject* GetLogicalParentNoRef() final;

// CFrameworkElement fields

// Group storage creation method for layout properties. Very important to not actually set any properties unless
// EnsureLayoutProperties has been called, or else the defaults will be changed.

    void EnsureLayoutProperties(bool fDefaultOK);
    CREATE_GROUP_FN static EnsureGroupStorage;
    _Check_return_ static HRESULT InvalidateImplicitStyles(_In_ CUIElement *pElement, _In_opt_ CResourceDictionary *pOldResources);

// Getters for ActualWidth/Height properties. (Would be setters, too, but these are R/O.)

    _Check_return_
    HRESULT static
    ActualWidth(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
    );

    _Check_return_
    HRESULT static
    ActualHeight(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
    );

    _Check_return_
    HRESULT static
    Parent(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _In_reads_(cArgs) CValue *pArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult
    );

    _Check_return_
    HRESULT static get_Parent(
    _In_ CFrameworkElement* pNativeInstance,
    _Outptr_ CDependencyObject** ppReturnValue);

    _Check_return_ XFLOAT GetActualOffsetX() override;
    _Check_return_ XFLOAT GetActualOffsetY() override;

    // Returns True when either the Width, MinWidth or MaxWidth property was set to a non-default value.
    bool IsWidthSpecified();
    // Returns True when either the Height, MinHeight or MaxHeight property was set to a non-default value.
    bool IsHeightSpecified();

    _Check_return_ XFLOAT GetSpecifiedWidth();
    _Check_return_ XFLOAT GetSpecifiedHeight();

    _Check_return_ XFLOAT GetActualWidth() override;
    virtual _Check_return_ HRESULT GetActualWidth(_Out_ XFLOAT* peWidth);
    _Check_return_ XFLOAT GetActualHeight() override;
    virtual _Check_return_ HRESULT GetActualHeight(_Out_ XFLOAT* peHeight);

    _Check_return_ HRESULT SetTemplateBinding(_In_ const CDependencyProperty* targetProperty, _In_ const CDependencyProperty* sourceProperty) final;

    CDependencyObject *GetTemplatedParent() final;

    _Check_return_ HRESULT DisconnectFromTemplatedParent();

    CDependencyObject* GetStandardNameScopeParent() final;

    IPALUri* GetBaseUri() final;

    void ClearTemplateBindingData();

    virtual _Check_return_ HRESULT OnApplyTemplate();

    // Template child manipulation.

    // Returns true if this FE already has an expanded template in its tree.
    virtual bool HasTemplateChild();

    // Adds the given CUIElement to the children collection as the designated templated child.
    // Should only be used if !HasTemplateChild.
    virtual _Check_return_ HRESULT AddTemplateChild(_In_ CUIElement* pUI);

    // Removes the templated child from the children collection of this FE. Only use if
    // HasTemplateChild is TRUE.
    virtual _Check_return_ HRESULT RemoveTemplateChild();

    _Check_return_ HRESULT InvokeApplyTemplate(_Out_ BOOLEAN* bAddedVisuals);

    _Check_return_ HRESULT InvokeFocus(
        _In_ DirectUI::FocusState focusState,
        _Out_ BOOLEAN* returnValue);

    _Check_return_ HRESULT MeasureOverrideForPInvoke(_In_ XSIZEF availableSize, _Out_ XSIZEF* desiredSize);

    _Check_return_ HRESULT ArrangeOverrideForPInvoke(_In_ XSIZEF finalSize, _Out_ XSIZEF* pNewFinalSize);

    _Check_return_ bool IsPropertySetByStyle(_In_ const CDependencyProperty *pdp) const final;

    _Check_return_ HRESULT SetIsSetByStyle(_In_ const CDependencyProperty* pdp, bool fSet = true) final;

    _Check_return_ HRESULT UpdateImplicitStyle(
        _In_opt_ CStyle *pOldStyle,
        _In_opt_ CStyle *pNewStyle,
        bool bForceUpdate,
        bool bUpdateChildren = true,
        bool isLeavingParentStyle = false
        ) override;

    bool IsPropertyTemplateBound(_In_ const CDependencyProperty* dp) const final;

    XINT32 IsDefaultHeight();
    XINT32 IsDefaultWidth();

    _Check_return_ HRESULT AddLogicalChild(_In_ CDependencyObject* pNewLogicalChild, _In_ bool bContentTemplateboundOnManagedSide = false);
    void RemoveLogicalChild(_Inout_opt_ CDependencyObject* pOldLogicalChild);

    _Check_return_ HRESULT GetValueFromStyle(
        _In_ const CDependencyProperty* dp,
        _Out_ CValue* pValue,
        _Out_ bool* gotValue);

    static  _Check_return_ HRESULT FlowDirection(
        _In_ CDependencyObject *pObject,
        _In_ XUINT32 cArgs,
        _Inout_updates_(cArgs) CValue *ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue *pResult);

    _Check_return_ HRESULT SetCursor(_In_ MouseCursor cursor);

    _Check_return_ HRESULT NotifyThemeChangedForInheritedProperties(_In_ Theming::Theme theme, _In_ bool freezeInheritedPropertiesAtSubTreeRoot);

    xref_ptr<CVisualStateGroupCollection> GetVisualStateGroupsNoCreate() const;
    xref_ptr<CResourceDictionary> GetResourcesNoCreate() const;

    virtual xref_ptr<CBrush> GetBackgroundBrush() const;
    virtual xref_ptr<CBrush> GetBorderBrush() const;
    virtual XTHICKNESS GetBorderThickness() const;
    virtual XTHICKNESS GetPadding() const;
    virtual XCORNERRADIUS GetCornerRadius() const;
    virtual DirectUI::BackgroundSizing GetBackgroundSizing() const;

    void UpdateRequiresCompNodeForRoundedCorners();

    void GetIndependentlyAnimatedBrushes(
        _Outptr_ CSolidColorBrush **ppFillBrush,
        _Outptr_ CSolidColorBrush **ppStrokeBrush
        ) override;

    bool IsAutomationPeerFactorySet() const;
    bool AllowFocusWhenDisabled();

    // Provides the information required to render the focus rectangles using the 5 FE.FocusVisualXXX properties.
    // If this FrameworkElement is a Control, then ppFocusTargetDescendant is set to the UIElement child which has
    // the IsTemplateFocusTarget attached property set to True, if any.  Otherwise it's set to null.
    // If such a ppFocusTargetDescendant UIElement exists, and it's a FrameworkElement, then its 5 FocusVisualXXX
    // properties take precedence over this FrameworkElement's 5 properties.
    // If neither the ppFocusTargetDescendant nor this FrameworkElement have explicitly set a property, then this
    // FrameworkElement's default property value is used.
    _Check_return_ HRESULT GetFocusVisualProperties(
        _In_opt_ CUIElement*  pFocusTargetDescendant,
        _Outptr_ CBrush** ppPrimaryBrush,
        _Outptr_ CBrush** ppSecondaryBrush,
        _Out_ XTHICKNESS* pPrimaryThickness,
        _Out_ XTHICKNESS* pSecondaryThickness);

    _Check_return_ HRESULT GetPropertiesForFocusVisualType(
        _In_opt_ const CFrameworkElement* const focusTargetDescendantFE,
        _In_ FocusVisualType type,
        _Outptr_ CBrush** brush,
        _Out_ XTHICKNESS* thickness);

    const CFrameworkElement* FixValueGetterForPropertyNoRef(
        _In_opt_ const CFrameworkElement* const focusTargetDescendant,
        _In_ KnownPropertyIndex index) const;

//-----------------------------------------------------------------------------
// LAYOUT Methods
//-----------------------------------------------------------------------------
public:
    static void ComputeAlignmentOffset(DirectUI::HorizontalAlignment ha, DirectUI::VerticalAlignment va, XSIZEF& clientSize, XSIZEF& inkSize, XFLOAT& offsetX, XFLOAT& offsetY);

    Theming::Theme GetRequestedThemeOverride(_In_ Theming::Theme theme);

    XRECTF GetEffectiveViewport() const;
    XRECTF GetMaxViewport() const;
    DOUBLE GetBringIntoViewDistanceX() const;
    DOUBLE GetBringIntoViewDistanceY() const;
    void SetEffectiveViewport(XRECTF value);
    void SetMaxViewport(XRECTF value);
    void SetBringIntoViewDistanceX(DOUBLE value);
    void SetBringIntoViewDistanceY(DOUBLE value);
    virtual xref_ptr<CControlTemplate> GetTemplate() const;

protected:
    virtual _Check_return_ HRESULT ApplyTemplate(_Out_ bool& fAddedVisuals);

    void SetTemplatedParentImpl(_In_ CDependencyObject* parent) final;

    virtual _Check_return_ HRESULT MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize);
    virtual _Check_return_ HRESULT ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize);

    virtual bool CompareForCircularReference(_In_ CFrameworkElement *pTreeChild);

    virtual void SetIsUpdatingBindings(_In_ bool fIsUpdatingBinding) { /*do nothing*/ }

    _Check_return_ HRESULT MeasureCore(XSIZEF availableSize, XSIZEF& desiredSize) final;
    _Check_return_ HRESULT ArrangeCore(XRECTF finalRect) override;

    _Check_return_ HRESULT UpdateLayoutClip(bool forceClipToRenderSize) override;

    _Check_return_ HRESULT NotifyThemeChangedCore(_In_ Theming::Theme theme, _In_ bool fForceRefresh = false) override;

protected:
    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;

    // Returns 'true' when the ArrangeOverride method always returns a size at least as large as its finalSize parameter.
    // The RichTextBlock is an example.
    virtual bool IsFinalArrangeSizeMaximized() { return false; }

    // Stores a WarningContext with the provided type and info. This FrameworkElement's class name and instance name are added to
    // the string array included in the memory dump, should it be created.
    bool StoreWarningContext(WarningContextLog::WarningContextType type, _In_ std::vector<std::wstring>& warningInfo, size_t framesToSkip) override;

private:
    void RaiseLoadingEventIfNeeded();
    void GetMinMax(XFLOAT& minWidth, XFLOAT& maxWidth, XFLOAT& minHeight, XFLOAT& maxHeight);
    void ComputeAlignmentOffset(XSIZEF& clientSize, XSIZEF& inkSize, XFLOAT& offsetX, XFLOAT& offsetY);

    // Retrieves the current Stretch-to-Top/Left alignment overriding status from the owning ScrollViewer control.
    _Check_return_ HRESULT IsStretchAlignmentTreatedAsNear(_In_ bool isForHorizontalAlignment, _Out_ bool* pIsStretchAlignmentTreatedAsNear);

    // Return true when an explicit Stretch alignment needs to actually be treated like a Top/Left alignment by either our layout engine or DManip.
    static bool IsStretchHorizontalAlignmentTreatedAsLeft(DirectUI::HorizontalAlignment ha, XSIZEF& clientSize, XSIZEF& inkSize);
    static bool IsStretchVerticalAlignmentTreatedAsTop(DirectUI::VerticalAlignment va, XSIZEF& clientSize, XSIZEF& inkSize);

    _Check_return_ XFLOAT ComputeWidthInMinMaxRange(XFLOAT width);
    _Check_return_ XFLOAT ComputeHeightInMinMaxRange(XFLOAT height);

    // Validates the TargetType passed in. In the designer, this returns S_FALSE if the target type is invalid and/or null.
    // This way we don't crash the designer. If it's invalid and not in the desiger, an error result will be returned, otherwise
    // it returns S_OK;
    _Check_return_ HRESULT ValidateTargetType(_In_opt_ const CClassInfo* targetType, _In_ KnownPropertyIndex index);

    // Reports the invalid target type. Returns E_NER_INVALID_OPERATION when not in the designer and S_FALSE when it is.
    // If this returned S_FALSE, then it notified the VisualTreeServiceCallback of the error.
    _Check_return_ HRESULT NotifyTargetTypeError(_In_opt_ const CClassInfo* targetType, _In_ KnownPropertyIndex index);

    _Check_return_ HRESULT GetGridPropertyValue(_In_ XUINT32 uIndex, _Out_ XUINT32 *puValue);

    _Check_return_ HRESULT InvalidateProperty(_In_ const CDependencyProperty* pDP, _In_ BaseValueSource baseValueSource) final;

    bool IsPropertyBitSet(_In_ const CDependencyProperty* pDP, const BitField& field) const;

    void SetIsPropertyBitSet(_In_ const CDependencyProperty* pdp, BitField& field, bool fSet = true);

    _Check_return_ HRESULT OnRequestedThemeChanged();

    xref_ptr<CStyle> GetStyle() const;
    DirectUI::ElementTheme GetRequestedTheme() const;

    void RaiseActiveThemeChangedEventIfChanging(_In_ Theming::Theme theme);

// CFrameworkElement fields

public:

    XFLOAT      m_eWidth;
    XFLOAT      m_eHeight;

    MouseCursor m_eMouseCursor;        // FrameworkElement.Cursor
private:
    // Allow 8 byte alignment on amd64
    ImplicitStyleProvider m_eImplicitStyleProvider : 4;
    bool                  m_firedLoadingEvent      : 1;
    // unsigned int                                : 27;    // 6 - 32 Unused on amd64


public:
    FrameworkElementGroupStorage* m_pLayoutProperties;
    CDependencyObject *m_pLogicalParent;

    BitField m_setByStyle;

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue* pValue,
        _In_ INT32 iListenerType,
        _Out_opt_ CValue* pResult,
        _In_ bool fHandledEventsToo = false) override;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue* pValue) override;

    static _Check_return_ HRESULT ActualTheme(
        _In_ CDependencyObject* pObject,
        _In_ UINT32 cArgs,
        _Inout_updates_(cArgs) CValue* ppArgs,
        _In_opt_ IInspectable* pValueOuter,
        _Out_ CValue* pResult
    );

    void NotifyThemeChangedListeners(_In_ Theming::Theme theme);

private:

    static const FrameworkElementGroupStorage DefaultLayoutProperties;

    std::unique_ptr<TemplateBindingData> m_pTemplateBindingData;

    xref_ptr<CStyle> m_pImplicitStyle;

    // This field is populated just often enough (1/3 of the time) that it's not really worth putting into sparse storage
    // However, someday, we may be able to efficiently cache our property index so that we can quickly retrieve this
    // from MetadataAPI without storing it here...
    xstring_ptr m_strClassName;

    xref::weakref_ptr<CUIElement> m_pImplicitStyleParentWeakRef;
};

struct FrameworkElementGroupStorage
{
    XFLOAT m_eMinWidth;
    XFLOAT m_eMaxWidth;
    XFLOAT m_eMinHeight;
    XFLOAT m_eMaxHeight;
    DirectUI::HorizontalAlignment m_horizontalAlignment;
    DirectUI::VerticalAlignment m_verticalAlignment;
    XTHICKNESS m_margin;
    XUINT32 m_nGridRow;
    XUINT32 m_nGridColumn;
    XUINT32 m_nGridRowSpan;
    XUINT32 m_nGridColumnSpan;
};
