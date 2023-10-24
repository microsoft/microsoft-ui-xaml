// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CIconSource.g.h"
#include "CBitmapIconSource.g.h"
#include "CFontIconSource.g.h"
#include "CPathIconSource.g.h"
#include "CSymbolIconSource.g.h"
#include <ImageDecodeBoundsFinder.h>

const XFLOAT g_ClientCoreFontSize = 20.f;
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strSegoeFluentIconsStorage, L"Segoe Fluent Icons,Segoe MDL2 Assets");

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CIconElement::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    // We want to create a symbol icon if we're given a parse string

    if (pCreate->m_value.GetType() == valueString)
    {
        return CSymbolIcon::Create(ppObject, pCreate);
    }

    // If we can't return an object, fail
    return E_FAIL;
}

// Pulls all non-locally set inherited properties from the parent.
_Check_return_ HRESULT CIconElement::PullInheritedTextFormatting()
{
    xref_ptr<TextFormatting> parentTextFormatting;

    IFCEXPECT_ASSERT_RETURN(m_pTextFormatting != nullptr);

    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC_RETURN(GetParentTextFormatting(parentTextFormatting.ReleaseAndGetAddressOf()));

        // Process each TextElement text core property one by one.

        IFC_RETURN(m_pTextFormatting->SetFontFamily(this, parentTextFormatting->m_pFontFamily));

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::IconElement_Foreground)
            && !m_pTextFormatting->m_freezeForeground)
        {
            IFC_RETURN(m_pTextFormatting->SetForeground(this, parentTextFormatting->m_pForeground));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Language))
        {
            m_pTextFormatting->SetLanguageString(parentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(parentTextFormatting->GetResolvedLanguageStringNoRef());
            m_pTextFormatting->SetResolvedLanguageListString(parentTextFormatting->GetResolvedLanguageListStringNoRef());
        }

        m_pTextFormatting->m_eFontSize = parentTextFormatting->m_eFontSize;
        m_pTextFormatting->m_nFontWeight = parentTextFormatting->m_nFontWeight;
        m_pTextFormatting->m_nFontStyle = parentTextFormatting->m_nFontStyle;
        m_pTextFormatting->m_nFontStretch = parentTextFormatting->m_nFontStretch;
        m_pTextFormatting->m_nCharacterSpacing = parentTextFormatting->m_nCharacterSpacing;
        m_pTextFormatting->m_nTextDecorations = parentTextFormatting->m_nTextDecorations;

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection = parentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal))
        {
            m_pTextFormatting->m_isTextScaleFactorEnabled = parentTextFormatting->m_isTextScaleFactorEnabled;
        }

        m_pTextFormatting->SetIsUpToDate();
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Measures the first (and only) child.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CIconElement::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    CUIElement* pChild = GetFirstChildNoAddRef();
    if (pChild)
    {
        // Ensure that our inherited Foreground property is up to date if it hasn't
        // been set locally.
        if (IsPropertyDefault(DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::IconElement_Foreground)))
        {
            EnsureTextFormattingForRead();
            PullInheritedTextFormatting();
        }

        // Measure the child
        IFC_RETURN(pChild->Measure(availableSize));
        IFC_RETURN(pChild->EnsureLayoutStorage());
        desiredSize = pChild->DesiredSize;
    }
    else
    {
        desiredSize.width = 0;
        desiredSize.height = 0;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Arranges the first (and only) child.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CIconElement::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;
    CUIElement* pChild;
    IFC(DoPointerCast(pChild, GetFirstChildNoAddRef()));
    if (pChild)
    {
        XRECTF arrangeRect = {0, 0, finalSize.width, finalSize.height};
        IFC(pChild->Arrange(arrangeRect));
    }

Cleanup:
    newFinalSize = finalSize;
    RRETURN(hr);
}

_Check_return_
HRESULT
CIconElement::AddChildElement(CUIElement* pChildElement)
{
    xref_ptr<CGrid> grid;
    xref_ptr<CSolidColorBrush> backgroundBrush;

    if (!GetFirstChildNoAddRef())
    {
        CREATEPARAMETERS cp(GetContext());
        CValue backgroundColor;

        IFC_RETURN(CGrid::Create(reinterpret_cast<CDependencyObject**>(grid.ReleaseAndGetAddressOf()), &cp));

        IFC_RETURN(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(backgroundBrush.ReleaseAndGetAddressOf()), &cp));
        backgroundColor.SetColor(0x00000000 /* Transparent */);
        IFC_RETURN(backgroundBrush->SetValueByKnownIndex(KnownPropertyIndex::SolidColorBrush_Color, backgroundColor));
        IFC_RETURN(grid->SetValueByKnownIndex(KnownPropertyIndex::Panel_Background, backgroundBrush.get()));

        IFC_RETURN(AddChild(grid.get()));
    }

    // We should have a child by this point.
    IFC_RETURN(GetFirstChildNoAddRef()->AddChild(pChildElement));
    return S_OK;
}

_Check_return_
HRESULT
CIconElement::RemoveChildElement()
{
    // If we don't have a child, then there's no element for us to remove.
    if (GetFirstChildNoAddRef())
    {
        IFC_RETURN(GetFirstChildNoAddRef()->RemoveChild(GetChildElementNoRef()));
    }

    return S_OK;
}

_Check_return_
CUIElement*
CIconElement::GetChildElementNoRef()
{
    CGrid* pGrid = do_pointer_cast<CGrid>(GetFirstChildNoAddRef());
    CUIElement* pResult = NULL;

    if (pGrid && pGrid->GetChildren())
    {
        pResult = static_cast<CUIElement*>(pGrid->GetChildren()->GetItemWithAddRef(0));
        ReleaseInterfaceNoNULL(pResult);
    }

    return pResult;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fetches the value of a specified property owned by this IconElement,
//      and sets it as the value of a property owned by our first (and only)
//      child. If fClearParentAssociation is true and the value is a
//      DependencyObject, its existing association with the IconElement
//      will be cleared so that it can be re-associated with the child
//      element.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CIconElement::UpdateLinkedChildProperty(
KnownPropertyIndex parentPropertyIndex,
    KnownPropertyIndex childPropertyIndex,
    bool fClearParentAssociation /* = false */)
{
    HRESULT hr = S_OK;
    CUIElement* pChild = NULL;
    CValue value;

    // Our child elements should all be under a root grid.
    if (GetFirstChildNoAddRef() && GetFirstChildNoAddRef()->GetChildren())
    {
        pChild = static_cast<CUIElement*>(GetFirstChildNoAddRef()->GetChildren()->GetItemWithAddRef(0));
    }

    if (pChild)
    {
        // Get the value from the parent's property
        IFC(GetValue(GetPropertyByIndexInline(parentPropertyIndex), &value));

        // Clear the value object's existing association with us, if requested
        if (fClearParentAssociation && value.AsObject())
        {
            value.AsObject()->SetAssociated(false, nullptr);
        }

        // Set the value on the child's property
        IFC(pChild->SetValueByKnownIndex(childPropertyIndex, value));
    }

Cleanup:
    ReleaseInterface(pChild);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSymbolIcon::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
   HRESULT hr = S_OK;
    CSymbolIcon* _this = new CSymbolIcon(pCreate->m_pCore);
    CDependencyObject *pSymbol = NULL;
    CDependencyObject *pTemp = NULL;
    IFC(ValidateAndInit(_this, &pTemp));

    if (pCreate->m_value.GetType() == valueString || pCreate->m_value.IsEnum())
    {
        // Create a Symbol instance using the same parameters as our icon
        IFC(CSymbol::Create(&pSymbol, pCreate));

        // Set the Symbol's enum value on our icon's "Symbol" property
        CValue value;
        value.SetEnum(static_cast<CSymbol*>(pSymbol)->m_nValue);
        IFC(_this->SetValueByKnownIndex(KnownPropertyIndex::SymbolIcon_Symbol, value));
    }

    *ppObject = pTemp;
    _this = NULL;

Cleanup:
    if (pTemp) delete _this;
    ReleaseInterface(pSymbol);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overrides InitInstance to set the default values of our
//      font size.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CSymbolIcon::InitInstance()
{
    // For now, font size is internal to allow AutoSuggestBox to set it
    // in order to get properly-sized SymbolIcon symbols for its purposes.
    // In the future, this may become a public property, in which case
    // it will need to be treated as FontIcon.FontSize is.
    m_fontSize = g_ClientCoreFontSize;

    return S_OK;
}

_Check_return_ HRESULT CSymbolIcon::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr = S_OK;
    CTextBlock* pTextBlock = NULL;
    CDependencyObject* pFontFamily = NULL;
    const xstring_ptr c_strSegoeFluentIcons = XSTRING_PTR_FROM_STORAGE(c_strSegoeFluentIconsStorage);

    if (!GetChildElementNoRef())
    {
        CValue horizontalAlignment;
        CValue verticalAlignment;
        CValue textAlignment;
        CValue fontSize;
        CValue fontStyle;
        CValue fontWeight;
        CValue accessibilityView;
        CValue isTextScaleFactorEnabled;
        CValue style;

        // Create a new TextBlock
        CREATEPARAMETERS cp(GetContext());
        IFC(CTextBlock::Create(reinterpret_cast<CDependencyObject**>(&pTextBlock), &cp));
        IFC(AddChildElement(pTextBlock));

        // Set the TextBlock's style to null so an implicit style doesn't affect it.
        style.SetNull();
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Style, style));

        // Set the TextBlock properties that aren't linked to our properties
        horizontalAlignment.Set(DirectUI::HorizontalAlignment::Stretch);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, horizontalAlignment));
        verticalAlignment.Set(DirectUI::VerticalAlignment::Center);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, verticalAlignment));
        textAlignment.Set(DirectUI::TextAlignment::Center);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextAlignment, textAlignment));

        // When SetFontSize() is used by AutoSuggestBox to set FontSize to zero (RS2+), do set FontSize on TextBlock
        // to allow for FontSize to be inherited from parent.
        if (m_fontSize > 0)
        {
            fontSize.SetFloat(m_fontSize);
            IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontSize, fontSize));
        }

        fontStyle.Set(DirectUI::FontStyle::Normal);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontStyle, fontStyle));
        fontWeight.Set(DirectUI::CoreFontWeight::Normal);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontWeight, fontWeight));
        isTextScaleFactorEnabled.SetBool(FALSE);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_IsTextScaleFactorEnabled, isTextScaleFactorEnabled));

        // Create a CFontFamily object for the "Segoe Fluent Icons" font and set it
        // as the TextBlock's FontFamily
        cp.m_value.SetString(c_strSegoeFluentIcons);
        IFC(CFontFamily::Create(&pFontFamily, &cp));
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_FontFamily, pFontFamily));

        // Set the TextBlock's Text property to a string containing our symbol
        IFC(UpdateChildTextBlockText());

        // Set the TextBlock's attached AccessibilityView property to "Raw" so that its text
        // won't be picked up by automation/accessibility tools
        accessibilityView.SetEnum(UIAXcp::AccessibilityView_Raw);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, accessibilityView));

        fAddedVisuals = true;
    }

Cleanup:
    ReleaseInterface(pFontFamily);
    ReleaseInterface(pTextBlock);
    RRETURN(hr);
}

//  Overrides base OnPropertyChanged to "push" the updated value
//  of our Foreground property to our child TextBlock's Foreground property
//  when it changes.
_Check_return_ HRESULT CSymbolIcon::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    // Call to base first
    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    // If the symbol or foreground has changed, update our child TextBlock
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::SymbolIcon_Symbol:
        {
            IFC_RETURN(UpdateChildTextBlockText());
            break;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Converts our Symbol value to a one-character string, and sets it
//      as the value of our child TextBlock's Text property.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CSymbolIcon::UpdateChildTextBlockText()
{
    CUIElement* pTextBlock = GetChildElementNoRef();

    if (pTextBlock)
    {
        xstring_ptr strSymbolString;
        CValue symbolAsEnum;
        CValue symbolAsString;

        // Convert our symbol to a string
        IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::SymbolIcon_Symbol), &symbolAsEnum));
        XUINT32 uSymbol = symbolAsEnum.AsEnum();

        // Allocation doesn't include space for NULL terminator as the string
        // builder takes care of adding an additional slot for the NULL.
        XStringBuilder symbolStringBuilder;

        IFC_RETURN(symbolStringBuilder.Initialize(1));
        IFC_RETURN(symbolStringBuilder.AppendChar(static_cast<const WCHAR>(uSymbol)));

        IFC_RETURN(symbolStringBuilder.DetachString(&strSymbolString));

        // Set the symbol string as the text of the TextBlock
        symbolAsString.SetString(strSymbolString);

        IFC_RETURN(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Text, symbolAsString));
    }

    return S_OK;
}

void
CSymbolIcon::SetFontSize(_In_ XFLOAT fontSize)
{
    m_fontSize = fontSize;
    InvalidateMeasure();
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for icon object
//
//------------------------------------------------------------------------
CFontIcon::~CFontIcon()
{
    ReleaseInterface(m_pScaleTransform);
}

_Check_return_ HRESULT CFontIcon::EnterImpl(_In_ CDependencyObject* pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(__super::EnterImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        IFC_RETURN(ApplyScaleTransformForFlowDirection());
    }

    return S_OK;
}

// Pulls all non-locally set inherited properties from the parent.
_Check_return_ HRESULT CFontIcon::PullInheritedTextFormatting()
{
    xref_ptr<TextFormatting> parentTextFormatting;

    IFCEXPECT_ASSERT_RETURN(m_pTextFormatting != NULL);

    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC_RETURN(GetParentTextFormatting(parentTextFormatting.ReleaseAndGetAddressOf()));

        // Process each TextElement text core property one by one.

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FontIcon_FontFamily))
        {
            // Default to the proper FontIcon FontFamily.
            xref_ptr<CDependencyObject> fontFamily;
            IFC_RETURN(CDependencyProperty::GetDefaultFontIconFontFamily(GetContext(), fontFamily.ReleaseAndGetAddressOf()));
            IFC_RETURN(m_pTextFormatting->SetFontFamily(this, do_pointer_cast<CFontFamily>(fontFamily.get())));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::IconElement_Foreground)
            && !m_pTextFormatting->m_freezeForeground)
        {
            IFC_RETURN(m_pTextFormatting->SetForeground(this, parentTextFormatting->m_pForeground));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Language))
        {
            m_pTextFormatting->SetLanguageString(parentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(parentTextFormatting->GetResolvedLanguageStringNoRef());
            m_pTextFormatting->SetResolvedLanguageListString(parentTextFormatting->GetResolvedLanguageListStringNoRef());
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FontIcon_FontSize))
        {
            // Default to the proper FontIcon FontSize.
            m_pTextFormatting->m_eFontSize = g_ClientCoreFontSize;
        }

        m_pTextFormatting->m_nFontStretch = parentTextFormatting->m_nFontStretch;
        m_pTextFormatting->m_nCharacterSpacing = parentTextFormatting->m_nCharacterSpacing;
        m_pTextFormatting->m_nTextDecorations = parentTextFormatting->m_nTextDecorations;

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection = parentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FontIcon_IsTextScaleFactorEnabled) &&
            IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal))
        {
            m_pTextFormatting->m_isTextScaleFactorEnabled = parentTextFormatting->m_isTextScaleFactorEnabled;
        }

        m_pTextFormatting->SetIsUpToDate();
    }

    return S_OK;
}

_Check_return_ HRESULT CFontIcon::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr = S_OK;
    CTextBlock* pTextBlock = NULL;

    if (!GetChildElementNoRef())
    {
        CValue horizontalAlignment;
        CValue verticalAlignment;
        CValue textAlignment;
        CValue accessibilityView;
        CValue isTextScaleFactorEnabled;
        CValue style;

        // Create a new TextBlock
        CREATEPARAMETERS cp(GetContext());
        IFC(CTextBlock::Create(reinterpret_cast<CDependencyObject**>(&pTextBlock), &cp));
        IFC(AddChildElement(pTextBlock));

        // Set the TextBlock's style to null so an implicit style doesn't affect it.
        style.SetNull();
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Style, style));

        // Set the TextBlock properties that aren't linked to our properties
        horizontalAlignment.Set(DirectUI::HorizontalAlignment::Stretch);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, horizontalAlignment));
        verticalAlignment.Set(DirectUI::VerticalAlignment::Center);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, verticalAlignment));
        textAlignment.Set(DirectUI::TextAlignment::Center);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextAlignment, textAlignment));
        isTextScaleFactorEnabled.SetBool(FALSE);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_IsTextScaleFactorEnabled, isTextScaleFactorEnabled));

        // Set our glyph string as the text of the TextBlock
        IFC(UpdateLinkedChildProperty(KnownPropertyIndex::FontIcon_Glyph, KnownPropertyIndex::TextBlock_Text));

        // Set the TextBlock's attached AccessibilityView property to "Raw" so that its text
        // won't be picked up by automation/accessibility tools
        accessibilityView.SetEnum(UIAXcp::AccessibilityView_Raw);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, accessibilityView));

        fAddedVisuals = true;

        // Apply a scale transform in x axis for flipping the visuals if flowdirection is set to RTL.
        IFC(ApplyScaleTransformForFlowDirection());
    }

Cleanup:
    ReleaseInterface(pTextBlock);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: OnPropertyChanged
//
//  Synopsis:
//      Overrides base OnPropertyChanged to "push" the updated value
//      of our Foreground property to our child TextBlock's Foreground property
//      when it changes.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CFontIcon::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    // Call to base first
    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    // Now check if the value needs to be pushed to one of our TextBlock's properties
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::FontIcon_Glyph:
            {
                IFC_RETURN(UpdateLinkedChildProperty(KnownPropertyIndex::FontIcon_Glyph, KnownPropertyIndex::TextBlock_Text));
                break;
            }
        case KnownPropertyIndex::FrameworkElement_FlowDirection:
        case KnownPropertyIndex::FontIcon_MirroredWhenRightToLeft:
            {
                IFC_RETURN(ApplyScaleTransformForFlowDirection());
                break;
            }
    }

    return S_OK;
}

_Check_return_ HRESULT
CFontIcon::ApplyScaleTransformForFlowDirection()
{
    CValue val;
    CValue mirroredWhenRightToLeftAsCValue;
    bool mirroredWhenRightToLeft = false;

    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::FontIcon_MirroredWhenRightToLeft, &mirroredWhenRightToLeftAsCValue));

    if (mirroredWhenRightToLeftAsCValue.GetType() == valueBool)
    {
        mirroredWhenRightToLeft = (mirroredWhenRightToLeftAsCValue.AsBool() != 0);
    }

    if (m_pScaleTransform == nullptr && IsRightToLeft() && mirroredWhenRightToLeft)
    {
        // Initialize the scale transform that will be used for mirroring and the
        // render transform origin as center in order to have the icon mirrored in place.
        CREATEPARAMETERS cp(GetContext());
        CValue renderTransformOriginAsCValue;
        XPOINTF* pRenderTransformOrigin = new XPOINTF;

        pRenderTransformOrigin->x = 0.5f;
        pRenderTransformOrigin->y = 0.5f;
        renderTransformOriginAsCValue.SetPoint(pRenderTransformOrigin);
        IFC_RETURN(CScaleTransform::Create((CDependencyObject**)(&m_pScaleTransform), &cp));
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransformOrigin, renderTransformOriginAsCValue));
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_RenderTransform, m_pScaleTransform));
    }

    if (m_pScaleTransform)
    {
        // Scale to 1 or -1 on X axis only if the RenderTransfrom is created before.
        // Otherwise it means we have never switched to RightToLeft at any time, so do nothing.
        val.SetFloat((IsRightToLeft() && mirroredWhenRightToLeft) ? -1.0f : 1.0f);
        IFC_RETURN(m_pScaleTransform->SetValueByKnownIndex(KnownPropertyIndex::ScaleTransform_ScaleX, val));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for icon object
//
//------------------------------------------------------------------------
CPathIcon::~CPathIcon()
{
    ReleaseInterface(m_pData);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathIcon::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;
    CPathIcon* _this = new CPathIcon(pCreate->m_pCore);
    CDependencyObject *pPath = NULL;
    CDependencyObject *pTemp = NULL;
    IFC(ValidateAndInit(_this, &pTemp));

    if ( pCreate->m_value.GetType() == valueString || pCreate->m_value.GetType() == valueObject)
    {
        IFC(CPath::Create(&pPath, pCreate));

        CValue value;
        value.WrapObjectNoRef(pPath);
        IFC(_this->SetValueByKnownIndex(KnownPropertyIndex::PathIcon_Data, value));
    }
   *ppObject = pTemp;
    _this = NULL;

Cleanup:
    if (pTemp) delete _this;
    ReleaseInterface(pPath);
    RRETURN(hr);
}


_Check_return_ HRESULT CPathIcon::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr = S_OK;
    CPath* pPath = NULL;

    if (!GetChildElementNoRef())
    {
        CValue horizontalAlignment;
        CValue verticalAlignment;

        // Create a new Path
        CREATEPARAMETERS cp(GetContext());
        IFC(CPath::Create(reinterpret_cast<CDependencyObject**>(&pPath), &cp));
        IFC(AddChildElement(pPath));

        // Set the Path properties that aren't linked to our properties
        horizontalAlignment.Set(DirectUI::HorizontalAlignment::Stretch);
        IFC(pPath->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, horizontalAlignment));
        verticalAlignment.Set(DirectUI::VerticalAlignment::Stretch);
        IFC(pPath->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, verticalAlignment));

        // Set the Path's Fill property to match our Foreground property
        IFC(UpdateLinkedChildProperty(KnownPropertyIndex::IconElement_Foreground, KnownPropertyIndex::Shape_Fill));

        // Set the Path's Data property to match ours
        IFC(UpdateLinkedChildProperty(KnownPropertyIndex::PathIcon_Data, KnownPropertyIndex::Path_Data, TRUE));

        fAddedVisuals = true;
    }

Cleanup:
    ReleaseInterface(pPath);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: OnPropertyChanged
//
//  Synopsis:
//      Overrides base OnPropertyChanged to "push" the updated value
//      of our Foreground property to our child Path's Fill property
//      when it changes.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CPathIcon::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    // Call to base first
    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    // Now check if the value needs to be pushed to one of our Path's properties
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::IconElement_Foreground:
            {
                IFC_RETURN(UpdateLinkedChildProperty(KnownPropertyIndex::IconElement_Foreground, KnownPropertyIndex::Shape_Fill));
                break;
            }
        case KnownPropertyIndex::PathIcon_Data:
            {
                IFC_RETURN(UpdateLinkedChildProperty(KnownPropertyIndex::PathIcon_Data, KnownPropertyIndex::Path_Data, TRUE));
                break;
            }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for icon object
//
//------------------------------------------------------------------------
CBitmapIcon::~CBitmapIcon()
{
    ReleaseBitmapImageResources();
    ReleaseWriteableBitmapResources();
}


_Check_return_ HRESULT CBitmapIcon::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr = S_OK;
    CImage* pImage = NULL;
    CValue foregroundBrushValue = {};

    if (!GetChildElementNoRef())
    {
        // Create the new Image object.
        CREATEPARAMETERS cp(GetContext());
        IFC(CImage::Create(reinterpret_cast<CDependencyObject**>(&pImage), &cp));
        IFC(AddChildElement(pImage));

        if (m_pWriteableBitmap != NULL)
        {
            CValue value;
            value.WrapObjectNoRef(m_pWriteableBitmap);

            // Set the value on the image's source property
            IFC(GetChildElementNoRef()->SetValueByKnownIndex(KnownPropertyIndex::Image_Source, value));
        }

        fAddedVisuals = true;
    }

    // Make sure our writeable bitmap is up-to-date with the foreground color value (inherited
    // or not).
    if (m_pWriteableBitmap != NULL &&
        SUCCEEDED(GetValueByIndex(KnownPropertyIndex::IconElement_Foreground, &foregroundBrushValue)))
    {
        if (const CSolidColorBrush* pSolidColorBrush = do_pointer_cast<CSolidColorBrush>(foregroundBrushValue))
        {
            IFC(ApplyForegroundColorToImage(pSolidColorBrush));
        }
    }


Cleanup:
    ReleaseInterface(pImage);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Responds to property changes to update the state of the icon,
//      such as when the foreground brush changes, or the uri source
//      changes.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CBitmapIcon::SetValue(_In_ const SetValueParams& args)
{
    IFC_RETURN(CFrameworkElement::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::IconElement_Foreground:
            {
                IFC_RETURN(ApplyForegroundColorToImage(do_pointer_cast<CSolidColorBrush>(args.m_value)));
                break;
            }
        case KnownPropertyIndex::BitmapIcon_UriSource:
            {
                IFC_RETURN(ReloadSourceImage());
                break;
            }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: OnPropertyChanged
//
//  Synopsis:
//      Overrides base OnPropertyChanged to apply our new foreground
//      property value to our image mask.  This allows us to pick up
//      inherited foreground values.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CBitmapIcon::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::IconElement_Foreground:
            {
                IFC_RETURN(ApplyForegroundColorToImage(args.m_pNewValue ? do_pointer_cast<CSolidColorBrush>(*args.m_pNewValue) : nullptr));
                break;
            }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Reloads the source image when our source URI changes.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CBitmapIcon::ReloadSourceImage()
{
    // Release our previous bitmap image resources.  These might be valid if
    // our source changed while we were downloading/decoding the previous image.
    ReleaseBitmapImageResources();

    // Release our writeable bitmap and unset it as our child image's source
    // in preparation for our new source.
    ReleaseWriteableBitmapResources();

    // If our new source was set to NULL, the icon will just be blank.
    if (!m_strSource.IsNull())
    {
        CValue value;
        value.SetString(m_strSource);

        CREATEPARAMETERS cp(GetContext(), value);

        // Create a new bitmap image object whose source will be set to the image
        // icon's source Uri.
        IFC_RETURN(CBitmapImage::Create(reinterpret_cast<CDependencyObject**>(&m_pBitmapImage), &cp));

        // Create and register the callback object that we'll use to get notifcations for
        // when image decoding completes.
        IFC_RETURN(CBitmapImageReportCallback::Create(this, &m_pBitmapImageReportCallback));
        IFC_RETURN(m_pBitmapImage->AddCallback(m_pBitmapImageReportCallback));

        // DecodeToRenderSize requires the element be placed in the live tree and rendered
        // in order to kick off the decoding process.  Since this BitmapImage is not put in
        // the live tree, it will never decode.  The fix is to disable DecodeToRenderSize.
        m_pBitmapImage->TraceDecodeToRenderSizeDisqualified(ImageDecodeBoundsFinder::BitmapIcon);
        m_pBitmapImage->SetDecodeToRenderSizeForcedOff();

        // Trigger a download of the image.  When this completes, our FireImageOpened method
        // should get invoked, which is where we'll create the writeable bitmap and set it as
        // the source of our child image.
        IFC_RETURN(m_pBitmapImage->ReloadImage(true /*uriChanged*/, false /*retainPlaybackState*/));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a writeable bitmap from our source image source object,
//      preserving the source's alpha values but overwriting the RGB values.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CBitmapIcon::CreateWritableBitmapFromSource()
{
    HRESULT hr = S_OK;

    CREATEPARAMETERS cp(GetContext());
    CWriteableBitmap* pWriteableBitmap = NULL;
    CUIElement* pChildImage = NULL;

    CValue foregroundBrushValue;
    XUINT32 defaultRgb = 0x00FFFFFF;

    IPALSurface *pSurface = NULL;
    bool surfaceLocked = false;

    XUINT32* pPixelBuffer = NULL;
    XUINT32 nWidth = 0;
    XUINT32 nHeight = 0;
    XUINT32 nLength = 0;
    XUINT32 nBufferSize = 0;

    // We should only be creating a new writeable bitmap when the previous one has been released or
    // never created in the first place.
    ASSERT(m_pWriteableBitmap == NULL);

    // Should have been created during init.
    ASSERT(m_pBitmapImage != NULL);

    // We should only be creating a new writeable bitmap when our bitmap image has a software surface.
    IFCEXPECT(m_pBitmapImage->GetSoftwareSurface() != NULL);

    if (SUCCEEDED(GetValueByIndex(KnownPropertyIndex::IconElement_Foreground, &foregroundBrushValue)))
    {
        if (const CSolidColorBrush* pSolidColorBrush = do_pointer_cast<CSolidColorBrush>(foregroundBrushValue))
        {
            defaultRgb = pSolidColorBrush->m_rgb & 0x00FFFFFF;
        }
    }

    // If our source is null or has no dimensions, then we're effectively unsetting the
    // source for our child image, which will cause the icon to be blank.
    if (m_pBitmapImage->GetWidth() > 0 && m_pBitmapImage->GetHeight() > 0)
    {
        XUINT32 surfaceWidth = 0;
        XUINT32 surfaceHeight = 0;
        XINT32 surfaceStride = 0;
        XUINT32* pSurfacePixels = NULL;

        nWidth = m_pBitmapImage->GetWidth();
        nHeight = m_pBitmapImage->GetHeight();

        IFC(UInt32Mult(nWidth, nHeight, &nLength));
        IFC(UInt32Mult(nLength, sizeof(XUINT32), &nBufferSize));

        // Create a new writeable bitmap object and a buffer to along with it.
        IFC(CWriteableBitmap::Create(reinterpret_cast<CDependencyObject**>(&pWriteableBitmap), &cp));

        pPixelBuffer = new XUINT32[nLength];
        memset(pPixelBuffer, 0x00, nBufferSize);

        IFC(pWriteableBitmap->Create(pPixelBuffer, nWidth, nHeight));

        // Lock our source's software surface so that we can copy pixel data from it.
        pSurface = m_pBitmapImage->GetSoftwareSurface();
        IFCEXPECT(pSurface != NULL);
        IFC(pSurface->Lock(
            reinterpret_cast<void**>(&pSurfacePixels),
            &surfaceStride,
            &surfaceWidth,
            &surfaceHeight
            ));
        surfaceLocked = TRUE;

        // We expect the software surface dimensions to match the values reported
        // by the source object.
        IFCEXPECT(surfaceWidth == nWidth);
        IFCEXPECT(surfaceHeight == nHeight);

        // Copy pixel data from our source image, preserving the alpha values but
        // overwriting the RGB values with our current foreground color.
        for (XUINT32 nRow = 0; nRow < nHeight; ++nRow)
        {
            for (XUINT32 nCol = 0; nCol < nWidth; ++nCol)
            {
                if (m_showAsMonochrome)
                {
                    pPixelBuffer[nRow * nWidth + nCol] = Premultiply((pSurfacePixels[nCol] & 0xFF000000) | defaultRgb);
                }
                else
                {
                    pPixelBuffer[nRow * nWidth + nCol] = pSurfacePixels[nCol];
                }
            }

            // Advance to the next line.
            pSurfacePixels = reinterpret_cast<XUINT32*>(reinterpret_cast<XUINT8*>(pSurfacePixels) + surfaceStride);
        }
    }

    // Update our child image's source to point to our new writeable bitmap.
    pChildImage = GetChildElementNoRef();
    if (pChildImage != NULL)
    {
        CValue value;
        value.WrapObjectNoRef(pWriteableBitmap);
        IFC(pChildImage->SetValueByKnownIndex(KnownPropertyIndex::Image_Source, value));
    }

    // Save off new state.
    m_nWidth = nWidth;
    m_nHeight = nHeight;

    m_pPixelBuffer = pPixelBuffer;
    pPixelBuffer = NULL;

    m_pWriteableBitmap = pWriteableBitmap;
    pWriteableBitmap = NULL;

Cleanup:
    if (surfaceLocked)
    {
        VERIFYHR(pSurface->Unlock());
    }

    // Clean up our pixel buffer if there was a failure.
    if (pPixelBuffer != NULL)
    {
        delete [] pPixelBuffer;
        pPixelBuffer = NULL;
    }

    // Clean up our writeable bitmap if there was a failure.
    if (pWriteableBitmap != NULL)
    {
        ReleaseInterface(pWriteableBitmap);
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Applies the color of the foreground brush to our image.  If the
//      brush is not a SolidColorBrush, then it applies White to the image.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CBitmapIcon::ApplyForegroundColorToImage(_In_opt_ const CSolidColorBrush* pBrush)
{
    if (m_pWriteableBitmap != NULL && m_showAsMonochrome)
    {
        XUINT32 nLength = 0;
        XUINT32 rgb = 0x00FFFFFF;
        XUINT8 alpha = 0xFF;

        // If a valid brush has been set, then we'll use its RGB values, otherwise
        // we'll use white.
        if (pBrush != NULL)
        {
            rgb = pBrush->m_rgb & 0x00FFFFFF;
            alpha = static_cast<XUINT8>((pBrush->m_rgb >> 24) & 0xFF);
        }

        IFC_RETURN(UInt32Mult(m_nWidth, m_nHeight, &nLength));

        // Apply the new foreground color to the buffer.
        for (XUINT32 i = 0; i < nLength; ++i)
        {
            m_pPixelBuffer[i] = Premultiply((m_pPixelBuffer[i] & 0xFF000000) | rgb);
        }

        // Update the child image's opacity based on foreground brush's
        // alpha value.
        if (GetChildElementNoRef() != NULL)
        {
            CValue value;
            value.SetFloat(static_cast<XFLOAT>(alpha) / 255.f);
            IFC_RETURN(GetChildElementNoRef()->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, value));
        }

        IFC_RETURN(m_pWriteableBitmap->Invalidate());
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Cleans up resources related to our bitmap image.
//
//-------------------------------------------------------------------------
void
CBitmapIcon::ReleaseBitmapImageResources()
{
    // Make sure our callback is unregistered.
    if (m_pBitmapImage && m_pBitmapImageReportCallback)
    {
        m_pBitmapImage->RemoveCallback(m_pBitmapImageReportCallback);
    }

    ReleaseInterface(m_pBitmapImageReportCallback);
    ReleaseInterface(m_pBitmapImage);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Cleans up resources related to our writeable bitmap.
//
//-------------------------------------------------------------------------
void
CBitmapIcon::ReleaseWriteableBitmapResources()
{
    // Unset the source of our child image.
    CUIElement* pChildImage = GetChildElementNoRef();
    if (pChildImage != NULL)
    {
        CValue value;
        value.SetNull();
        IGNOREHR(pChildImage->SetValueByKnownIndex(KnownPropertyIndex::Image_Source, value));
    }

    ReleaseInterface(m_pWriteableBitmap);

    if (m_pPixelBuffer != NULL)
    {
        delete [] m_pPixelBuffer;
        m_pPixelBuffer = NULL;
    }

    m_nWidth = 0;
    m_nHeight = 0;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fired to report progress of an image download.  This is currently
//      a NO-OP for BitmapIcon.
//
//-------------------------------------------------------------------------
void
CBitmapIcon::FireImageDownloadEvent(_In_ XFLOAT /*downloadProgress*/)
{
    // No-op
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fired to report and error during image download/opening.
//      If we fail to open the image, the icon will just be blank.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CBitmapIcon::FireImageFailed(_In_ XUINT32 /*iErrorCode*/)
{
    // We no longer need these resources.
    ReleaseBitmapImageResources();
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fired to report completion of image download & opening.  We cue off
//      this event to create our writeable bitmap.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CBitmapIcon::FireImageOpened()
{
    HRESULT hr = S_OK;

    IFC(CreateWritableBitmapFromSource());

Cleanup:
    // We no longer need these resources.
    ReleaseBitmapImageResources();

    RRETURN(hr);
}

CIconSourceElement::~CIconSourceElement()
{
    if (m_iconElement)
    {
        m_iconElement->UnpegManagedPeer();
    }
}

_Check_return_ HRESULT CIconSourceElement::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    if (!GetChildElementNoRef() && m_iconSource)
    {
        IFC_RETURN(SwapIconElements());
        fAddedVisuals = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CIconSourceElement::SwapIconElements()
{
    if (m_iconElement)
    {
        m_iconElement->UnpegManagedPeer();
        m_iconElement.reset();
    }

    IFC_RETURN(RemoveChildElement());

    if (m_iconSource)
    {
        CREATEPARAMETERS cp(GetContext());

        if (auto fontIconSource = do_pointer_cast<CFontIconSource>(m_iconSource.get()))
        {
            IFC_RETURN(CFontIcon::Create(reinterpret_cast<CDependencyObject**>(m_iconElement.ReleaseAndGetAddressOf()), &cp));
            IFC_RETURN(m_iconElement->PegManagedPeer());

            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"Glyph").Get(), m_iconElement.get(), KnownPropertyIndex::FontIcon_Glyph));
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"FontFamily").Get(), m_iconElement.get(), KnownPropertyIndex::FontIcon_FontFamily));
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"FontSize").Get(), m_iconElement.get(), KnownPropertyIndex::FontIcon_FontSize));
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"FontStyle").Get(), m_iconElement.get(), KnownPropertyIndex::FontIcon_FontStyle));
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"FontWeight").Get(), m_iconElement.get(), KnownPropertyIndex::FontIcon_FontWeight));
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"IsTextScaleFactorEnabled").Get(), m_iconElement.get(), KnownPropertyIndex::FontIcon_IsTextScaleFactorEnabled));
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"MirroredWhenRightToLeft").Get(), m_iconElement.get(), KnownPropertyIndex::FontIcon_MirroredWhenRightToLeft));
        }
        else if (auto symbolIconSource = do_pointer_cast<CSymbolIconSource>(m_iconSource.get()))
        {
            IFC_RETURN(CSymbolIcon::Create(reinterpret_cast<CDependencyObject**>(m_iconElement.ReleaseAndGetAddressOf()), &cp));
            IFC_RETURN(m_iconElement->PegManagedPeer());

            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"Symbol").Get(), m_iconElement.get(), KnownPropertyIndex::SymbolIcon_Symbol));
        }
        else if (auto bitmapIconSource = do_pointer_cast<CBitmapIconSource>(m_iconSource.get()))
        {
            IFC_RETURN(CBitmapIcon::Create(reinterpret_cast<CDependencyObject**>(m_iconElement.ReleaseAndGetAddressOf()), &cp));
            IFC_RETURN(m_iconElement->PegManagedPeer());

            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"UriSource").Get(), m_iconElement.get(), KnownPropertyIndex::BitmapIcon_UriSource));
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"ShowAsMonochrome").Get(), m_iconElement.get(), KnownPropertyIndex::BitmapIcon_ShowAsMonochrome));
        }
        else if (auto pathIconSource = do_pointer_cast<CPathIconSource>(m_iconSource.get()))
        {
            IFC_RETURN(CPathIcon::Create(reinterpret_cast<CDependencyObject**>(m_iconElement.ReleaseAndGetAddressOf()), &cp));
            IFC_RETURN(m_iconElement->PegManagedPeer());

            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"Data").Get(), m_iconElement.get(), KnownPropertyIndex::PathIcon_Data));
        }

        // Here is a typical visualtree structure when we are using XamlUICommand in AppBarButton
        //  AppBarButton
        //      - IconElement
        //          - IconSource
        // If IconSource is default Foreground, we assume the foreground should be inherited from it's parent AppBarButton, the disabled foreground should apply to icon too.
        // But if we bind iconSource.Foreground with iconElement.Foreground in this situation, any visualstate change to the icon would be override by iconSource.Foreground
        // So we only SetBinding when IconSource.Foreground is not the default one.
        if (m_iconElement && !m_iconSource->IsPropertyDefaultByIndex(KnownPropertyIndex::IconSource_Foreground))
        {
            IFC_RETURN(FxCallbacks::DXamlCore_SetBinding(m_iconSource.get(), wrl_wrappers::HStringReference(L"Foreground").Get(), m_iconElement.get(), KnownPropertyIndex::IconElement_Foreground));
        }

        IFC_RETURN(AddChildElement(m_iconElement.get()));
    }

    return S_OK;
}

_Check_return_ HRESULT CIconSourceElement::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    // If the icon source has changed, swap elements.
    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::IconSourceElement_IconSource:
        {
            IFC_RETURN(SwapIconElements());
            break;
        }
    }

    return S_OK;
}
