// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include <DOPointerCast.h>
#include <TemplateBindingData.h>
#include <FrameworkTheming.h>
#include <CVisualStateManager2.h>
#include <stack_vector.h>
#include "XamlTraceLogging.h"
#include <DependencyLocator.h>
#include <FloatUtil.h>
#include "theming\inc\Theme.h"
#include <RuntimeEnabledFeatures.h>
#include "DiagnosticsInterop.h"
#include "ResourceGraph.h"
#include "DXamlServices.h"
#include "RootScale.h"

using namespace Theming;
using namespace DirectUI;

CFrameworkElement::CFrameworkElement(_In_ CCoreServices *pCore)
    : CUIElement(pCore)
    , m_eImplicitStyleProvider(ImplicitStyleProvider::None)
    , m_eWidth(static_cast<XFLOAT>(XDOUBLE_NAN))
    , m_eHeight(static_cast<XFLOAT>(XDOUBLE_NAN))
    , m_eMouseCursor((MouseCursor)0)
    // If this can't be created due to OOM, it will return NULL, but EnsureLayoutProperties will return a failure HRESULT later.
    , m_pLayoutProperties(const_cast<FrameworkElementGroupStorage*>(&DefaultLayoutProperties))
    , m_pLogicalParent(nullptr)
    , m_setByStyle()
    , m_strClassName()
    , m_firedLoadingEvent(false)
{
    ASSERT(0 == m_setByStyle.bits);
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for framework element object
//
//------------------------------------------------------------------------

CFrameworkElement::~CFrameworkElement()
{
#if DBG
    InheritedProperties::RecordTextPropertyUsage(this);
#endif

    // Don't try to delete the default properties.
    if (m_pLayoutProperties != &DefaultLayoutProperties)
    {
        delete m_pLayoutProperties;
    }

    auto resources = GetResourcesNoCreate();
    if (resources)
    {
        // Clear the ResourceDictionary's owner weak ref to avoid a dangling pointer
        resources->SetResourceOwner(nullptr);
    }

    IGNOREHR(DisconnectFromTemplatedParent());
    if(IsBitFieldValidPointer())
    {
        delete [] m_setByStyle.pointer;
        // We can use the same ValidPointer check for m_styleValueSourceHasBinding as well because if there's
        // no Binding set through a style, it will be NULL.  If there is a BindingSet through a style,
        // both pointers will be valid and both will be released.
    }
}

//------------------------------------------------------------------------
//
//  Method:   CreationComplete
//
//  Synopsis:
//      called when the parser has finished parsing the tag associated
//      with this element.  All properties will have been set, and all
//      children will have been added by this point.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CFrameworkElement::CreationComplete()
{
    // Apply style
    if (m_eImplicitStyleProvider == ImplicitStyleProvider::None || GetStyle())
    {
        IFC_RETURN(ApplyStyle());
    }

    // call base implementation.
    IFC_RETURN(CUIElement::CreationComplete());
    return S_OK;
}

// Removes the weak reference back from this child DO to this DO, its parent.
_Check_return_ HRESULT CFrameworkElement::ResetReferenceFromChild(_In_ CDependencyObject* child)
{
    IFC_RETURN(CDependencyObject::ResetReferenceFromChild(child));

    // Make sure logical parent cleans up references to itself before going away.
    RemoveLogicalChild(child);

    return S_OK;
}

_Check_return_ HRESULT CFrameworkElement::EnsureGroupStorage(
    _In_     CDependencyObject   *pObject,
    _In_opt_ const CDependencyProperty *pDp,
    _In_     bool forGetValue // If this is a GetValue call
)
{
    static_cast<CFrameworkElement*>(pObject)->EnsureLayoutProperties(forGetValue);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: SetValue
//
//  Synopsis:
//      Override to base SetValue for some Framework specific property settings
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CFrameworkElement::SetValue(_In_ const SetValueParams& args)
{
    XUINT32 uOldValue = 0;
    XINT32 bGridValueChanged = FALSE;

    xref_ptr<CStyle> oldStyle;
    bool oldStylePegged = false;
    auto styleGuard = wil::scope_exit([&oldStyle, &oldStylePegged]
    {
        if (oldStyle && oldStylePegged) {
            oldStyle->UnpegManagedPeer();
        }
    });

    xref_ptr<CResourceDictionary> oldResources;
    bool oldResourcesPegged = false;
    auto resourceGuard = wil::scope_exit([&oldResources, &oldResourcesPegged]
    {
        if (oldResources && oldResourcesPegged) {
            oldResources->UnpegManagedPeer();
        }
    });

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::FrameworkElement_Language:
            {
                // validate the XmlLanguage string
                if (!ValidateXmlLanguage(&args.m_value))
                {
                    IFC_RETURN(static_cast<HRESULT>(E_DO_INVALID_CONTENT));
                }

                break;
            }
        case KnownPropertyIndex::FrameworkElement_Style:
            {
                CStyle *pStyle = nullptr;
                IFC_RETURN(DoPointerCast(pStyle, args.m_value.AsObject()));
                if (pStyle)
                {
                    HRESULT validateResult = ValidateTargetType(pStyle->GetTargetType(), KnownPropertyIndex::FrameworkElement_Style);
                    IFC_RETURN(validateResult);
                    if (validateResult == S_OK)
                    {
                        IFC_RETURN(pStyle->Seal());
                    }
                    else
                    {
                        ASSERT(validateResult == S_FALSE);
                        // In the designer, we won't cause the app to crash, we just won't change the style. We'll
                        // notify to the user that something went wrong.
                        return S_OK;
                    }
                }

                // Keep the old style so that we can send a change notification after the
                // new one is set.  Since it will no longer be in the tree, we need to peg it.

                oldStyle = GetActiveStyle();
                if (oldStyle)
                {
                    IFC_RETURN(oldStyle->PegManagedPeer());
                    oldStylePegged = true;
                }

                break;
            }
        case KnownPropertyIndex::Grid_Row:
        case KnownPropertyIndex::Grid_Column:
        case KnownPropertyIndex::Grid_RowSpan:
        case KnownPropertyIndex::Grid_ColumnSpan:
            {
                IFC_RETURN(GetGridPropertyValue(static_cast<XUINT32>(args.m_pDP->GetIndex()), &uOldValue));
                break;
            }
        case KnownPropertyIndex::FrameworkElement_Resources:
            {
                auto resources = GetResourcesNoCreate();
                if (resources)
                {
                    if (resources->HasImplicitStyle())
                    {
                        // Keep the old ResourceDictionary so that we can send a change notification after the
                        // new one is set.  Since it will no longer be in the tree, we need to peg it.
                        oldResources = resources;
                        IFC_RETURN(oldResources->PegManagedPeer());
                        oldResourcesPegged = true;
                    }
                    else
                    {
                        resources->SetResourceOwner(nullptr);
                    }
                }
                break;
            }
    }

    IFC_RETURN(CUIElement::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::FrameworkElement_Language:
            {
                auto core = GetContext();
                ASSERT(m_pTextFormatting != NULL);
                IFC_RETURN(m_pTextFormatting->ResolveLanguageString(core));
                IFC_RETURN(m_pTextFormatting->ResolveLanguageListString(core));
                break;
            }
        case KnownPropertyIndex::FrameworkElement_HorizontalAlignment:
        case KnownPropertyIndex::FrameworkElement_VerticalAlignment:
            {
                IFC_RETURN(CUIElement::OnAlignmentChanged(
                    args.m_pDP->GetIndex() == KnownPropertyIndex::FrameworkElement_HorizontalAlignment /*fIsForHorizontalAlignment*/,
                    FALSE /*fIsForStretchAlignment*/,
                    FALSE /*fIsStretchAlignmentTreatedAsNear*/));
                break;
            }

        case KnownPropertyIndex::FrameworkElement_Style:
            {
                if (!GetStyle() && !m_pImplicitStyle)
                {
                    IFC_RETURN(EnsureImplicitStyle(false /*isLeavingParentStyle*/));
                }
                // If we're not parsing, then we don't
                // have to defer the application of the style
                CStyle * pNewStyle = GetActiveStyle();
                if (!IsParsing() && oldStyle != pNewStyle)
                {
                    IFC_RETURN(OnStyleChanged(oldStyle.get(), pNewStyle, BaseValueSourceStyle));
                }
                break;
            }

        case KnownPropertyIndex::Control_IsTemplateFocusTarget:
            {
                // If we're setting IsTemplateFocusTarget, then we'll want to find our ancestor and set its descendant pointer to us
                // if we've set IsTemplateFocusTarget to true, or clear its value if we've set it to false and it's currently equal to us.
                // Note that we can't use the value of args.m_value, because it might be a string if the value was set
                // in the control template of this control.
                const CDependencyProperty *pFocusDescendantDP = GetPropertyByIndexInline(KnownPropertyIndex::Control_IsTemplateFocusTarget);
                CValue isTemplateFocusTargetValue;

                IFC_RETURN(GetValue(pFocusDescendantDP, &isTemplateFocusTargetValue));
                IFC_RETURN(UpdateFocusAncestorsTarget(isTemplateFocusTargetValue.AsBool()));
                break;
            }

        case KnownPropertyIndex::Control_IsTemplateKeyTipTarget:
            {
                // If this element is a control's TemplateKeyTipTarget, find the control that owns this template expansion and set the
                // internal TemplateKeyTipTarget to point to this element, so we know what element to use to calculate the KeyTip bounds
                // if the user enters AccessKey mode.
                const CDependencyProperty* isKeyTipTargetProp = GetPropertyByIndexInline(KnownPropertyIndex::Control_IsTemplateKeyTipTarget);

                CValue isTemplateKeyTipTargetValue;
                IFC_RETURN(GetValue(isKeyTipTargetProp, &isTemplateKeyTipTargetValue));

                CDependencyObject* templateParent = GetTemplatedParent();
                if (templateParent && templateParent->OfTypeByIndex<KnownTypeIndex::Control>())
                {
                    const CDependencyProperty* keyTipTargetProp = templateParent->GetPropertyByIndexInline(KnownPropertyIndex::Control_TemplateKeyTipTarget);

                    CValue keyTipTargetValue;
                    IFC_RETURN(templateParent->GetValue(keyTipTargetProp, &keyTipTargetValue));

                    if (isTemplateKeyTipTargetValue.AsBool())
                    {
                        // Clobber the control's previous TemplateKeyTipTarget if needed, the last element to set the property wins.
                        keyTipTargetValue.SetObjectAddRef(this);
                        IFC_RETURN(templateParent->SetValue(keyTipTargetProp, keyTipTargetValue));
                    }
                    else if (keyTipTargetValue.AsObject() == this)
                    {
                        // The control's KeyTipTargetValue is set to this element, and the element has unset IsTemplateKeyTipTarget.
                        // Clear out the value.
                        IFC_RETURN(templateParent->ClearValue(keyTipTargetProp));
                    }
                }
                break;
            }


        case KnownPropertyIndex::Grid_Row:
        case KnownPropertyIndex::Grid_Column:
        case KnownPropertyIndex::Grid_RowSpan:
        case KnownPropertyIndex::Grid_ColumnSpan:
            {
                XUINT32 uNewValue;

                // Has value changed?
                IFC_RETURN(GetGridPropertyValue(static_cast<XUINT32>(args.m_pDP->GetIndex()), &uNewValue));
                bGridValueChanged = (uOldValue != uNewValue);
                break;
            }
        case KnownPropertyIndex::FrameworkElement_Resources:
            {
                auto resources = GetResourcesNoCreate();

                if (oldResources != resources)
                {
                    bool stylesInvalidated = false;
                    // if we have implicit style in new ResourceDictionary
                    // then we want to invalidate all implicit styles from root
                    // to be sure that correct style will picked up by the element
                    if (resources)
                    {
                        resources->SetResourceOwner(this);

                        if (resources->HasImplicitStyle())
                        {
                            // if we got new ResourceDictionary with implicit styles then invalidate all implicit styles
                            IFC_RETURN(CFrameworkElement::InvalidateImplicitStyles(this, nullptr));
                            stylesInvalidated = true;
                        }

                        resources->InvalidateNotFoundCache(true);
                    }

                    if (oldResources)
                    {
                        if (!stylesInvalidated)
                        {
                            // otherwise invalidate only implicit styles that came from the old ResourceDictionary.
                            IFC_RETURN(CFrameworkElement::InvalidateImplicitStyles(this, oldResources.get()));
                        }
                        oldResources->SetResourceOwner(nullptr);
                    }
                }
                break;
            }

        case KnownPropertyIndex::FrameworkElement_RequestedTheme:
            {
                IFC_RETURN(OnRequestedThemeChanged());
                break;
            }

        case KnownPropertyIndex::AutomationProperties_LandmarkType:
        case KnownPropertyIndex::AutomationProperties_LocalizedLandmarkType:
            {
                // We want to force any FrameworkElement which sets the LandmarkType or LocalizedLandmarkType property into the UIA tree; this will
                // make sure there is an AutoamtionPeer for the element if there otherwise wouldn't be.
                if ((args.m_value.IsEnum() && args.m_value.AsEnum() != UIAXcp::AutomationLandmarkType::AutomationLandmarkType_None) ||
                    (args.m_value.GetType() == valueString && !args.m_value.AsString().IsNullOrEmpty()))
                {
                    CValue automationPeerFactory;
                    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::FrameworkElement_AutomationPeerFactoryIndex, &automationPeerFactory));
                    if (automationPeerFactory.AsSigned() == 0)
                    {
                        automationPeerFactory.SetSigned(static_cast<XINT32>(KnownTypeIndex::LandmarkTargetAutomationPeer));
                        IFC_RETURN(SetValueByIndex(KnownPropertyIndex::FrameworkElement_AutomationPeerFactoryIndex, automationPeerFactory));
                    }
                }
            }
            break;

        case KnownPropertyIndex::FrameworkElement_AutomationPeerFactoryIndex:
            {
                // This bitflag saves CPU cycles checking AutomationPeerFactoryIndex in the sparse storage
                m_fIsAutomationPeerFactorySet = (args.m_value.AsSigned() != 0);
                break;
            }

        case KnownPropertyIndex::AutomationProperties_Name:
        case KnownPropertyIndex::AutomationProperties_LabeledBy:
            {
                // We want to force any FrameworkElement which sets the Name or LabeledBy automation property into the UIA tree; this will
                // make sure there is an AutoamtionPeer for the element if there otherwise wouldn't be.
                if ((args.m_value.GetType() == valueString && !args.m_value.AsString().IsNullOrEmpty()) ||
                    (args.m_value.GetType() == valueObject && args.m_value.AsObject() != nullptr))
                {
                    CValue automationPeerFactory;
                    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::FrameworkElement_AutomationPeerFactoryIndex, &automationPeerFactory));
                    if (automationPeerFactory.AsSigned() == 0)
                    {
                        automationPeerFactory.SetSigned(static_cast<XINT32>(KnownTypeIndex::NamedContainerAutomationPeer));
                        IFC_RETURN(SetValueByIndex(KnownPropertyIndex::FrameworkElement_AutomationPeerFactoryIndex, automationPeerFactory));
                    }
                }
            }
            break;

        case KnownPropertyIndex::Border_CornerRadius:
        case KnownPropertyIndex::ContentPresenter_CornerRadius:
        case KnownPropertyIndex::Grid_CornerRadius:
        case KnownPropertyIndex::RelativePanel_CornerRadius:
        case KnownPropertyIndex::StackPanel_CornerRadius:
            {
                UpdateRequiresCompNodeForRoundedCorners();
            }
            break;
    }

    if (IsPropertyTemplateBound(args.m_pDP) && GetTemplatedParent() != nullptr && !GetTemplatedParent()->IsUpdatingBindings())
    {
        auto modifiedValue = GetModifiedValue(args.m_pDP);

        if (modifiedValue &&
            // REVIEW: In our next MQ, we should get rid of the ModifierValueBeingSet flag.
            (!modifiedValue->IsModifierValueBeingSet() && args.m_modifierValueBeingSet == nullptr) &&
            modifiedValue->HasModifiers())
        {
            // a local value is being set during animation.
            // so note that templatebinding can be removed after animation has ended.
            modifiedValue->StopTemplateBinding(true);
        }

        if (!modifiedValue || (!modifiedValue->HasModifiers() && modifiedValue->ShouldTemplateBindingBeStopped()))
        {
            // do the actual removal of the template when we have a templated parent and it is not updating us
            // or we detected a local set during animation
            CControl* templatedParent = do_pointer_cast<CControl>(GetTemplatedParent());
            if (templatedParent)
            {
                templatedParent->RemoveTemplateBinding(this, args.m_pDP);
            }
            m_pTemplateBindingData->m_pTemplateBindings->Remove(args.m_pDP, false);    // Remove but do not do a cleanup

            if (modifiedValue)
            {
                // stopped templatebinding, so reset switch
                modifiedValue->StopTemplateBinding(false);
            }
        }
    }

    // Invalidate the grid when grid values change, so that grid
    // can re-arrange its children.
    if (bGridValueChanged)
    {
        CUIElement* pParent = GetUIElementParentInternal();

        if (pParent && pParent->OfTypeByIndex<KnownTypeIndex::Grid>())
        {
            pParent->InvalidateMeasure();
        }
    }

    RRETURN(S_OK);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Pulls all non-locally set inherited properties from the parent.
//
//  Since the only text core property that FrameworkElement exposes is
//  Language, all other properties are simply copied form the parent,
//  which will most likely be the default values.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFrameworkElement::PullInheritedTextFormatting()
{
    HRESULT         hr                    = S_OK;
    TextFormatting *pParentTextFormatting = NULL;

    IFCEXPECT_ASSERT(m_pTextFormatting != NULL);
    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC(GetParentTextFormatting(&pParentTextFormatting));

        // Process each TextElement text core property one by one.

        IFC(m_pTextFormatting->SetFontFamily(this, pParentTextFormatting->m_pFontFamily));

        if (!m_pTextFormatting->m_freezeForeground)
        {
            IFC(m_pTextFormatting->SetForeground(this, pParentTextFormatting->m_pForeground));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Language))
        {
            m_pTextFormatting->SetLanguageString(pParentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(pParentTextFormatting->GetResolvedLanguageStringNoRef());
            m_pTextFormatting->SetResolvedLanguageListString(pParentTextFormatting->GetResolvedLanguageListStringNoRef());
        }

        m_pTextFormatting->m_eFontSize         = pParentTextFormatting->m_eFontSize;
        m_pTextFormatting->m_nFontWeight       = pParentTextFormatting->m_nFontWeight;
        m_pTextFormatting->m_nFontStyle        = pParentTextFormatting->m_nFontStyle;
        m_pTextFormatting->m_nFontStretch      = pParentTextFormatting->m_nFontStretch;
        m_pTextFormatting->m_nCharacterSpacing = pParentTextFormatting->m_nCharacterSpacing;
        m_pTextFormatting->m_nTextDecorations  = pParentTextFormatting->m_nTextDecorations;

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection  = pParentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal))
        {
            m_pTextFormatting->m_isTextScaleFactorEnabled = pParentTextFormatting->m_isTextScaleFactorEnabled;
        }

        m_pTextFormatting->SetIsUpToDate();
    }

Cleanup:
    ReleaseInterface(pParentTextFormatting);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Fast get of RightToLeft flag
//
//  Needed for e.g. animation scenarios where rendering needs the RTL flag
//  without any other text properties.
//
//------------------------------------------------------------------------
void CFrameworkElement::EvaluateIsRightToLeft()
{
    bool isRightToLeft = false;

    // The Silverlight rasterizer accesses the flow direction of ui elements
    // very frequently, so we avoid the relatively slow IsPropertyDefaultById
    // and use the faster IsPropertySetBySlot.

    // Fast result if FlowDirection is set locally, or if inherited properties
    // are up to date.

    if (m_pTextFormatting != NULL
        && (!m_pTextFormatting->IsOld()
        || IsPropertySetBySlot(MetadataAPI::GetPropertySlot(KnownPropertyIndex::FrameworkElement_FlowDirection))))
    {
        isRightToLeft = m_pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft;
    }
    else
    {
        // We'll need to get direction from our parent.

        CDependencyObject *pParent = GetInheritanceParentInternal();

        if (pParent != NULL)
        {
            isRightToLeft = pParent->IsRightToLeft();
        }
    }

    m_isRightToLeft = isRightToLeft;
    m_isRightToLeftGeneration = GetContext()->m_cIsRightToLeftGenerationCounter;
}

//------------------------------------------------------------------------
//
//  Method:   ApplyStyle
//
//  Synopsis: Get and apply style
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFrameworkElement::ApplyStyle()
{
    CStyle *pOldStyle = nullptr;
    CStyle *pNewStyle = GetStyle();

    if (!pNewStyle)
    {
        pOldStyle = m_pImplicitStyle;
        IFC_RETURN(EnsureImplicitStyle(false /*isLeavingParentStyle*/));
        pNewStyle = m_pImplicitStyle;
    }

    // Apply style
    if (pOldStyle != pNewStyle)
    {
        IFC_RETURN(OnStyleChanged(pOldStyle, pNewStyle, BaseValueSourceStyle));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets implicit style local value
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFrameworkElement::UpdateImplicitStyle(
    _In_opt_ CStyle *pOldStyle,
    _In_opt_ CStyle *pNewStyle,
    bool bForceUpdate,
    bool bUpdateChildren,
    bool isLeavingParentStyle
    )
{
    xref_ptr<CStyle> pOldImplicitStyle;

    if (bForceUpdate || (pOldStyle && m_pImplicitStyle == pOldStyle) || pNewStyle)
    {
        pOldImplicitStyle = m_pImplicitStyle;
        m_pImplicitStyle.reset();

        // if default Style property is null apply implicit style
        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Style))
        {
            auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

            // XamlDiagnostics signals it modified the implicit style by passing in the same style as pOldStyle and pNewStyle.
            const bool xamlDiagRefresh = bForceUpdate && (pOldStyle == pNewStyle) && runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics);
            if (!xamlDiagRefresh || pNewStyle)
            {
                // pNewStyle may not be applicable to this item, but one of the children, so re-ensure this implicit style if the one
                // was passed in.
                IFC_RETURN(EnsureImplicitStyle(!!isLeavingParentStyle));
            }

            if (pOldImplicitStyle != m_pImplicitStyle || xamlDiagRefresh)
            {
                // Apply local style
                IFC_RETURN(OnStyleChanged(pOldImplicitStyle, m_pImplicitStyle, BaseValueSourceStyle));
            }
        }
    }

    IFC_RETURN(CUIElement::UpdateImplicitStyle(pOldStyle, pNewStyle, bForceUpdate, bUpdateChildren, isLeavingParentStyle));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   InvalidateImplicitStyles
//
//  Synopsis: Resets implicit style value and force it to update
//            If pOldResources is NULL we we want to invalidate all implicit
//            styles to be sure that correct style will picked up by the element
//            Otherwise we invalidate only those styles which were in the old
//            ResourceDictionary
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFrameworkElement::InvalidateImplicitStyles(
        _In_ CUIElement *pElement,
        _In_opt_ CResourceDictionary *pOldResources)
{
    if (pElement)
    {
        if (pOldResources)
        {
            ASSERT(pOldResources->HasImplicitStyle());

            Jupiter::stack_vector<CStyle*, 8> implicitStyles;
            IFC_RETURN(pOldResources->GetUndeferredImplicitStyles(&implicitStyles));

            for (auto& pStyle : implicitStyles.m_vector)
            {
                IFC_RETURN(pElement->UpdateImplicitStyle(pStyle, NULL, /*bForceUpdate*/ FALSE));
            }

            if (pOldResources->m_pMergedDictionaries)
            {
                IFC_RETURN(pOldResources->m_pMergedDictionaries->InvalidateImplicitStyles());
            }
        }
        else
        {
            IFC_RETURN(pElement->UpdateImplicitStyle(NULL, NULL, /*bForceUpdate*/TRUE));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   OnStyleChanged
//
//  Synopsis: Process change of style. Invalidate properties which
//  are not in the new and old style, so those property values can be
//  updated.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CFrameworkElement::OnStyleChanged(
    _In_ CStyle *pOldStyle,
    _In_ CStyle *pNewStyle,
    _In_ BaseValueSource baseValueSource)
{
    HRESULT hr = S_OK;
    unsigned int oldStylePropertyCount = 0;
    const bool isSettingFromManaged = GetContextInterface()->IsSettingValueFromManaged(this);
    const bool shouldStoreSourceInfo = DXamlServices::ShouldStoreSourceInformation();

    TraceElementStyleChangedInfo((XUINT64)this, (XUINT64)pNewStyle);

    if (isSettingFromManaged)
    {
        // although this style might have been set from managed, all the properties
        // that are being invalidated are being set from native.
        GetContextInterface()->SetValueFromManaged(NULL);
    }

    if (pNewStyle)
    {
        // Seal style so it cannot be changed after it is applied
        IFC(pNewStyle->Seal());
    }

    // Invalidate properties in the old style, which are not
    // in the new style
    if (pOldStyle)
    {
        if (pOldStyle->HasMutableSetters() || shouldStoreSourceInfo)
        {
            auto thisWeakRef = xref::get_weakref(this);
            GetContext()->RemoveMutableStyleValueChangedListener(pOldStyle, thisWeakRef);
        }

        // Get count of properties in old style
        oldStylePropertyCount = pOldStyle->GetSetterCount();

        // Iterate over old style's properties
        for (auto i=0U; i < oldStylePropertyCount; i++)
        {
            bool bInBothStyles = false;
            KnownPropertyIndex oldPropIndex = KnownPropertyIndex::UnknownType_UnknownProperty;

            IFC(pOldStyle->GetPropertyAtSetterIndex(i, &oldPropIndex));

            if (pNewStyle)
            {
                IFC(pNewStyle->HasPropertySetter(oldPropIndex, &bInBothStyles));
            }

            // Invalidate property if it is in the old style, but not in the new style,
            // because that property is being cleared.
            if (!bInBothStyles)
            {
                const CDependencyProperty* pDP = MetadataAPI::GetDependencyPropertyByIndex(oldPropIndex);
                IGNOREHR(InvalidateProperty(pDP, baseValueSource));
            }
        }
    }

    // Invalidate properties in the new style
    if (pNewStyle)
    {
        KnownPropertyIndex propIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
        unsigned int newStylePropertyCount = 0;

        // Get count of properties in new style
        newStylePropertyCount = pNewStyle->GetSetterCount();

        for (auto i = 0U; i < newStylePropertyCount; i++)
        {
            IFC(pNewStyle->GetPropertyAtSetterIndex(i, &propIndex));
            IGNOREHR(InvalidateProperty(MetadataAPI::GetDependencyPropertyByIndex(propIndex), baseValueSource));

            // Let the style know that the property setter has been applied.
            // If the value has a new peer after InvalidateProperty, an optimized style will add a
            // peer reference to it to ensure the life of the peer through the lifetime of the style.
            IFC(pNewStyle->NotifySetterApplied(i));
        }

        if (pNewStyle->HasMutableSetters() || shouldStoreSourceInfo)
        {
            auto thisWeakRef = xref::get_weakref(this);
            GetContext()->AddMutableStyleValueChangedListener(pNewStyle, thisWeakRef);
        }
    }

Cleanup:
    if (isSettingFromManaged)
    {
        // return to the correct state
        GetContextInterface()->SetValueFromManaged(this);
    }

    RRETURN(hr);
}


//------------------------------------------------------------------------
//  Method:   UpdateFocusAncestorsTarget
//
//  Synopsis: used by the focus chrome target property,
//  set bool to true when you are adding the descendant.
//------------------------------------------------------------------------
_Check_return_ HRESULT CFrameworkElement::UpdateFocusAncestorsTarget(_In_ bool shouldSet)
{
    // By design, we need to ensure that the descendant is never its own ancestor.
    CDependencyObject* pParent = static_cast<CDependencyObject*>(this)->GetParent();

    while (pParent != nullptr && !pParent->OfTypeByIndex<KnownTypeIndex::Control>())
    {
        pParent = pParent->GetParent();
    }
    if (pParent != nullptr)
    {
        const CDependencyProperty *pFocusDescendantDP = pParent->GetPropertyByIndexInline(KnownPropertyIndex::Control_FocusTargetDescendant);

        CValue focusTargetDescendantValue;
        IFC_RETURN(pParent->GetValue(pFocusDescendantDP, &focusTargetDescendantValue));

        bool valueChanged = false;

        if (shouldSet && focusTargetDescendantValue.AsObject() != this)
        {
            focusTargetDescendantValue.SetObjectAddRef(this);
            valueChanged = true;
        }
        else if (!shouldSet && focusTargetDescendantValue.AsObject() == this)
        {
            focusTargetDescendantValue.SetObjectAddRef(nullptr);
            valueChanged = true;
        }

        if (valueChanged)
        {
            IFC_RETURN(pParent->SetValue(pFocusDescendantDP, focusTargetDescendantValue));

            // We want the focus rect to be redrawn, so we'll need to mark this element as being render dirty.
            NWSetContentDirty(this, DirtyFlags::Render);
        }
    }
    return S_OK;
}

// Get property value from style.
_Check_return_ HRESULT CFrameworkElement::GetValueFromStyle(
    _In_ const CDependencyProperty* dp,
    _Out_ CValue *pValue,
    _Out_ bool* gotValue)
{
    CStyle* pActiveStyle = GetActiveStyle();

    *gotValue = false;

    if (pActiveStyle)
    {
        IFC_RETURN(pActiveStyle->GetPropertyValue(
            dp->GetIndex(),
            pValue,
            gotValue));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   InvalidateProperty
//
//  Synopsis: Invalidate property, so it can be re-evaluated.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CFrameworkElement::InvalidateProperty(
    _In_ const CDependencyProperty* pDP,
    _In_ BaseValueSource baseValueSource)
{
    if (pDP->IsSparse())
    {
        // Peer needs to start participating in framework tree because framework property
        // is being changed.
        IFC_RETURN(SetParticipatesInManagedTreeDefault());
    }

    IFC_RETURN(__super::InvalidateProperty(pDP, baseValueSource));

    return S_OK;
}

_Check_return_ HRESULT CFrameworkElement::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CUIElement::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::ToolTipService_ToolTip:
        {
            // If we're setting the value of ToolTipService.ToolTip, then we need to have a managed peer to notify,
            // since otherwise the tool tip won't get registered with the tool tip service.
            // So let's create one now if we don't have one already, and mark this object as participating in the
            // managed tree since a ref to the peer will now be in the ToolTipService
            IFC_RETURN(SetParticipatesInManagedTreeDefault());
            IFC_RETURN(EnsurePeer());

            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CFrameworkElement::GetGridPropertyValue
//
//  Parameters:
//      uIndex - Property whose value is to be obtained
//      puValue - Returns value
//
//  Synopsis:
//      Get value for grid property
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CFrameworkElement::GetGridPropertyValue(
    _In_ XUINT32 uIndex,
    _Out_ XUINT32 *puValue)
{
    switch (static_cast<KnownPropertyIndex>(uIndex))
    {
        case KnownPropertyIndex::Grid_Row:
        {
            *puValue = m_pLayoutProperties->m_nGridRow;
            break;
        }

        case KnownPropertyIndex::Grid_Column:
        {
            *puValue = m_pLayoutProperties->m_nGridColumn;
            break;
        }

        case KnownPropertyIndex::Grid_RowSpan:
        {
            *puValue = m_pLayoutProperties->m_nGridRowSpan;
            break;
        }

        case KnownPropertyIndex::Grid_ColumnSpan:
        {
            *puValue = m_pLayoutProperties->m_nGridColumnSpan;
            break;
        }

        default:
        {
            // Unknown property value
            ASSERT(0);
            IFC_RETURN(E_FAIL);
        }
     }

    return S_OK;
}

_Check_return_ HRESULT CFrameworkElement::ValidateTargetType(_In_opt_ const CClassInfo* targetType, _In_ KnownPropertyIndex index)
{
    if (targetType)
    {
        bool isTypeValid = false;
        if (HasManagedPeer() && IsCustomType())
        {
            wrl::ComPtr<IInspectable> peer;
            IFC_RETURN(DirectUI::DXamlServices::GetPeer(this, IID_PPV_ARGS(&peer)));
            IFC_RETURN(DirectUI::MetadataAPI::IsInstanceOfType(peer.Get(), targetType, &isTypeValid));
        }
        else
        {
            isTypeValid = OfTypeByIndex(targetType->GetIndex());
        }

        if (isTypeValid)
        {
            return S_OK;
        }
    }

    return NotifyTargetTypeError(targetType, index);
}

_Check_return_ HRESULT CFrameworkElement::NotifyTargetTypeError(_In_opt_ const CClassInfo* targetType, _In_ KnownPropertyIndex index)
{
    const auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    const bool ignoreForDesigner = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics);

    // We need to throw an InvalidOperationException with a specific exception string on the managed side.
    // Set the error in errorservice and this will be retrieved by managed code while constructing the exception.
    HRESULT hrToOriginate = E_NER_INVALID_OPERATION;

    if (targetType == nullptr && !ignoreForDesigner)
    {
        // ignore the error code returned from ReportError as we should propagate
        // the actual error code(AG_E_STYLE_BASEDON_TARGETTYPE_NULL).
        IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AgError(AG_E_STYLE_BASEDON_TARGETTYPE_NULL), 0, NULL));
    }
    else if (!ignoreForDesigner)
    {
        // ignore the error code returned from ReportError as we should propagate
        // the actual error code(AG_E_RUNTIME_STYLE_TARGETTYPE).
        if (SUCCEEDED(EnsureClassName()))
        {
            xephemeral_string_ptr parameters[2];

            m_strClassName.Demote(&parameters[0]);
            targetType->GetFullName().Demote(&parameters[1]);

            IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AgError(AG_E_RUNTIME_STYLE_TARGETTYPE), 2, parameters));
        }
    }
    else
    {
        // Don't crash the designer, just report out to the callback so the user can know of their mistake
        hrToOriginate = S_FALSE;

        const auto graph = Diagnostics::GetResourceGraph();
        auto dependency = graph->TryGetResourceDependency(this, index);
        if (!dependency)
        {
            if (auto parent = do_pointer_cast<CItemsControl>(m_pLogicalParent))
            {
                // Note we aren't catching every possible Style type that a parent could apply to a child element. For now,
                // this is the most common scenario and if we get future bugs about "Resource 'Unknown' being invalid" then we could
                // look into a more general solution.
                dependency = graph->TryGetResourceDependency(parent, KnownPropertyIndex::ItemsControl_ItemContainerStyle);
            }
        }

        if (dependency)
        {
            dependency->MarkInvalid(VisualElementState::ErrorInvalidResource);
        }
        else
        {
            // Report something went wrong. Right now, this will just report "Resource 'Unknown' being invalid" in VS.
            const auto interop = Diagnostics::GetDiagnosticsInterop(true);
            interop->OnElementStateChanged(VisualElementState::ErrorInvalidResource, this, DirectUI::MetadataAPI::GetPropertyByIndex(index));
        }
    }

    return hrToOriginate;
}

_Check_return_
HRESULT
CFrameworkElement::ActualWidth(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    if (!cArgs && !pArgs && pResult)
    {
        // We are getting the value
        XFLOAT value;
        CFrameworkElement* pElement = (CFrameworkElement*) pObject;
        IFC_RETURN(pElement->GetActualWidth(&value));
        pResult->SetFloat(value);
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_
HRESULT
CFrameworkElement::ActualHeight(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    if (!cArgs && !pArgs && pResult)
    {
       // We are getting the value
        XFLOAT value;
        CFrameworkElement* pElement = (CFrameworkElement*) pObject;
        IFC_RETURN(pElement->GetActualHeight(&value));
        pResult->SetFloat(value);
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
// LAYOUT Methods
//-----------------------------------------------------------------------------

void
CFrameworkElement::GetMinMax(XFLOAT& minWidth, XFLOAT& maxWidth, XFLOAT& minHeight, XFLOAT& maxHeight)
{
    XINT32 isDefaultHeight = IsDefaultHeight();
    XINT32 isDefaultWidth = IsDefaultWidth();

    maxHeight = m_pLayoutProperties->m_eMaxHeight;
    minHeight = m_pLayoutProperties->m_eMinHeight;
    XFLOAT userValue  = m_eHeight;

    XFLOAT height = isDefaultHeight ? XFLOAT_INF : userValue;
    maxHeight = MAX(MIN(height, maxHeight), minHeight);

    height = (isDefaultHeight ? 0 : userValue);
    minHeight = MAX(MIN(maxHeight, height), minHeight);

    maxWidth = m_pLayoutProperties->m_eMaxWidth;
    minWidth = m_pLayoutProperties->m_eMinWidth;
    userValue = m_eWidth;

    XFLOAT width = (isDefaultWidth ? XFLOAT_INF : userValue);
    maxWidth = MAX(MIN(width, maxWidth), minWidth);

    width = (isDefaultWidth ? 0 : userValue);
    minWidth = MAX(MIN(maxWidth, width), minWidth);

    if (GetUseLayoutRounding())
    {
        // It is possible for max vars to be INF so be don't want to round those.

        minWidth = LayoutRound(minWidth);

        if (IsFiniteF(maxWidth))
            maxWidth = LayoutRound(maxWidth);

        minHeight = LayoutRound(minHeight);

        if (IsFiniteF(maxHeight))
            maxHeight = LayoutRound(maxHeight);
    }

}

//-------------------------------------------------------------------------
//
//  Function:   CFrameworkElement::OnApplyTemplate()
//
//  Synopsis:   Default overridable implementation. Does nothing.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CFrameworkElement::OnApplyTemplate()
{
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   InvokeApplyTemplate()
//
//  Synopsis:   Ensure that all template expansion has occurred before refreshing
//              the bound data and notifying OnApplyTemplate() listeners.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CFrameworkElement::InvokeApplyTemplate(_Out_ BOOLEAN* bAddedVisuals)
{
    HRESULT hr = S_OK;
    bool fAddedVisuals = false;

    // Even though ApplyTemplate is defined at the CFrameworkElement level, only CControl has template bindings/overloads.
    // You also must be a CControl to receive OnApplyTemplate notification.
    CControl* pControl = do_pointer_cast<CControl>(this);

    if (EventEnabledApplyTemplateBegin())
    {
        IFC(EnsureClassName());
        TraceApplyTemplateBegin((UINT64)this, m_strClassName.GetBuffer());
    }

    IFC(ApplyTemplate(fAddedVisuals));

    if (auto visualTree = VisualTree::GetForElementNoRef(pControl))
    {
        // Create VisualState StateTriggers and perform evaulation to determine initial state,
        // if we're in the visual tree (since we need it to get our qualifier context).
        // If we're not in the visual tree, we'll do this when we enter it.
        IFC(CVisualStateManager2::InitializeStateTriggers(this));
    }

    if (fAddedVisuals)
    {
        if (pControl)
        {
            // Run all of the bindings that were created and set the
            // properties to the values from this control
            IFC(pControl->RefreshTemplateBindings(TemplateBindingsRefreshType::All));
        }
        // If the object has a managed peer that is a custom type, then it might have
        // an overloaded OnApplyTemplate. Reverse P/Invoke to get that overload, if any.
        // If there's no overload, the default Control.OnApplyTemplate will be invoked,
        // which will just P/Invoke back to the native CControl::OnApplyTemplate.
        if (HasManagedPeer() && IsCustomType())
        {
            IFC(FxCallbacks::FrameworkElement_OnApplyTemplate(this));
        }
        else
        {
            IFC(OnApplyTemplate());
        }
    }

    // Update template bindings of realized element in the template.
    // This will update if element was realized after template was applied (no visuals added, hence it would not be updated earlier)
    // and if element was realized in OnApplyTemplate.
    // We should not refresh all of controls template bindings, as it could potentially overwrite values set by other ways (e.g. VSM)
    if (pControl &&
        pControl->NeedsTemplateBindingRefresh())
    {
        IFC(pControl->RefreshTemplateBindings(TemplateBindingsRefreshType::WithoutInitialUpdate));
    }

    TraceApplyTemplateEnd();

Cleanup:
    *bAddedVisuals = fAddedVisuals;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CFrameworkElement::InvokeFocus
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CFrameworkElement::InvokeFocus(
    _In_ DirectUI::FocusState focusState,
    _Out_ BOOLEAN* returnValue)
{
    bool focusInvoked = false;

    *returnValue = FALSE;
    IFC_RETURN(Focus(focusState, false /*animateIfBringIntoView*/, &focusInvoked));
    *returnValue = focusInvoked;
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CFrameworkElement::CompareForCircularReference()
//
//  Synopsis:   Called during template instantiation for all the templated
//  parents of an element. Should return TRUE if a cycle is detected.
//
//  Looks for circumstances that indicate a cycle when instantiating
//  a template. Sample markup that will be caught:
//     <Style>
//         <Button />
//         <Setter for Template>
//             <Button />
//         </Setter for Template>
//     </Style>
//
//-------------------------------------------------------------------------
bool CFrameworkElement::CompareForCircularReference(_In_ CFrameworkElement *pTreeChild)
{
    if (!pTreeChild)
    {
        return false;
    }

    auto pTemplate = GetTemplate();
    auto pTemplateChild = pTreeChild->GetTemplate();

    // Circular reference is found if pTemplate and pTemplateChild are the same and they are not both NULL
    return pTemplate == pTemplateChild && pTemplate != nullptr;
}

xref_ptr<CControlTemplate> CFrameworkElement::GetTemplate() const
{
    return nullptr;
}

_Check_return_ HRESULT CFrameworkElement::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr = S_OK;
    CDependencyObject* pChild = NULL;

    // Applying the template will not delete existing visuals. This will be done conditionally
    // when the template is invalidated.
    if (!HasTemplateChild())
    {
        auto pTemplate = GetTemplate();
        if(pTemplate)
        {
            // do circular check
            CFrameworkElement* pTemplatedParent = do_pointer_cast<CFrameworkElement>(GetTemplatedParent());
            const CDependencyProperty* pdp = GetPropertyByIndexInline(KnownPropertyIndex::Control_Template);

            while (pTemplatedParent != NULL)
            {
                if (pdp)
                {
                    // If along the parent tree, we encounter a parent that has a local template, the circular reference rule
                    // no longer apply.
                    if (!pTemplatedParent->OfTypeByIndex<KnownTypeIndex::Control>() || pTemplatedParent->HasLocalOrModifierValue(pdp))
                    {
                        break;
                    }
                }

                if (pTemplatedParent->CompareForCircularReference(this))
                {
                    IFC(E_INVALIDARG);
                }
                pTemplatedParent = do_pointer_cast<CFrameworkElement>(pTemplatedParent->GetTemplatedParent());
            }


            // Ensure that the types of the template and the control
            // match before expanding it
            if (pTemplate->HasTargetType())
            {
                // Ensure that the types match. When under the designer, we'll no-op and pretend like the template
                // had no content. ValidateTargetType will notify the user something went wrong
                HRESULT validateResult = ValidateTargetType(pTemplate->GetTargetType(), KnownPropertyIndex::Control_Template);
                IFC_RETURN(validateResult);
                if (validateResult == S_FALSE)
                {
                    goto Cleanup;
                }
            }


            // Do not do the lookup of subscriptions when expanding the template
            SetIsUpdatingBindings(TRUE);
            IFC(pTemplate->LoadContent(&pChild, this));
            SetIsUpdatingBindings(FALSE);

            // If no content is returned from the template stop here
            if (pChild == NULL)
            {
                goto Cleanup;
            }

            fAddedVisuals = TRUE;

            // Add new ImplementationRoot as one and only child of this object.
            ASSERT(pChild->OfTypeByIndex<KnownTypeIndex::UIElement>());
            IFC(AddTemplateChild(static_cast<CUIElement*>(pChild)));
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        SetIsUpdatingBindings(FALSE);
    }
    ReleaseInterface(pChild);
    RRETURN(hr);
}

bool CFrameworkElement::HasTemplateChild()
{
    return GetFirstChildNoAddRef() != nullptr;
}

_Check_return_
HRESULT
CFrameworkElement::AddTemplateChild(_In_ CUIElement* pUI)
{
    RRETURN(AddChild(pUI));
}

_Check_return_
HRESULT
CFrameworkElement::RemoveTemplateChild()
{
    // FE doesn't support this.
    RRETURN(E_UNEXPECTED);
}

//------------------------------------------------------------------------
//
//  Method:   MeasureOverrideForPInvoke
//
//  Synopsis:
//      This is the public entry point to get the platform MeasureOverride implementation.
//      This should be only called from FrameworkElement_MeasureOverride, the entry point
//      for the P/Invoke. DO NOT CALL THIS METHOD FROM ANYWHERE ELSE.
//------------------------------------------------------------------------
_Check_return_
 HRESULT
 CFrameworkElement::MeasureOverrideForPInvoke(
    _In_ XSIZEF availableSize,
    _Out_ XSIZEF* pDesiredSize
)
{
    IFC_RETURN(MeasureOverride(availableSize, *pDesiredSize));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ArrangeOverrideForPInvoke
//
//  Synopsis:
//      This is the public entry point to get the platform ArrangeOverride implementation.
//      This should be only called from FrameworkElement_ArrangeOverride, the entry point
//      for the P/Invoke. DO NOT CALL THIS METHOD FROM ANYWHERE ELSE.
//------------------------------------------------------------------------
_Check_return_
 HRESULT
 CFrameworkElement::ArrangeOverrideForPInvoke(
    _In_ XSIZEF finalSize,
    _Out_ XSIZEF* pNewFinalSize
)
{
    IFC_RETURN(ArrangeOverride(finalSize, *pNewFinalSize));

    return S_OK;
}


_Check_return_ HRESULT
CFrameworkElement::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    desiredSize.width = desiredSize.height = 0;
    return S_OK;
}

_Check_return_
HRESULT
CFrameworkElement::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    newFinalSize = finalSize;

    return S_OK;
}

_Check_return_
HRESULT
CFrameworkElement::MeasureCore(XSIZEF availableSize, XSIZEF& desiredSize)
{
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
    bool bInLayoutTransition = pLayoutManager ? pLayoutManager->GetTransitioningElement() == this : false;

    XSIZEF frameworkAvailableSize;
    XFLOAT minWidth = 0.0f;
    XFLOAT maxWidth = 0.0f;
    XFLOAT minHeight = 0.0f;
    XFLOAT maxHeight = 0.0f;

    XFLOAT clippedDesiredWidth;
    XFLOAT clippedDesiredHeight;

    XFLOAT roundedMarginWidth = 0.0f;
    XFLOAT roundedMarginHeight = 0.0f;

    BOOLEAN bTemplateApplied = FALSE;

    RaiseLoadingEventIfNeeded();

    if (!bInLayoutTransition)
    {
        // Templates should be applied here.
        IFC_RETURN(InvokeApplyTemplate(&bTemplateApplied));

        roundedMarginWidth = m_pLayoutProperties->m_margin.left + m_pLayoutProperties->m_margin.right;
        roundedMarginHeight = m_pLayoutProperties->m_margin.top + m_pLayoutProperties->m_margin.bottom;

        // Layout round the margins too. This corresponds to the behavior in ArrangeCore, where we check the unclipped desired
        // size against available space minus the rounded margin. This also prevents a bug where MeasureCore adds the unrounded
        // margin (e.g. 14) to the desired size (e.g. 55.56) and rounds the final result (69.56 rounded to 69.33 under 2.25x scale),
        // then ArrangeCore takes that rounded result (i.e. 69.33), subtracts the unrounded margin (i.e. 14) and ends up with a
        // size smaller than the desired size (69.33 - 14 = 55.33 < 55.56). This ends up putting a layout clip on an element that
        // doesn't need one, and causes big problems if the element is the scrollable extent of a carousel panel.
        if (GetUseLayoutRounding())
        {
            // Note: Both MeasureCore and ArrangeCore round the total margin width/height together,
            // rather than round left, right, top, and and bottom individually. This could result in
            // reserving more availableSize for margin than is necessary: for example, Margin="1" at
            // 125% should be properly rounded to Round(1 * 1.25) = 1 physical pixel on all sides,
            // but rounding together is Round((1 + 1) * 1.25) = Round(2.5) = 3 physical pixels in each
            // direction. That's one extra pixel of margin, and also means the content is no longer
            // exactly centered. This is an opportunity for future improvement if there is an important
            // rendering complaint about too much margin and this slight mis-centering.
            // In contrast, BorderThickness values are properly rounded individually (see
            // CBorder::GetLayoutRoundedThickness).
            roundedMarginWidth = LayoutRound(roundedMarginWidth);
            roundedMarginHeight = LayoutRound(roundedMarginHeight);
        }

        // Subtract the margins from the available size
        // We check to see if availableSize.width and availableSize.height are finite since that will
        // also protect against NaN getting in.
        frameworkAvailableSize.width = IsFiniteF(availableSize.width) ? MAX(availableSize.width - roundedMarginWidth, 0) : XFLOAT_INF;
        frameworkAvailableSize.height = IsFiniteF(availableSize.height) ? MAX(availableSize.height - roundedMarginHeight, 0) : XFLOAT_INF;

        if (GetUseLayoutRounding())
        {
            // The parent panel may not have passed in an availableSize which was layout rounded. This
            // can happen with Padding, which currently doesn't get layout rounded (see
            // CBorder::HelperGetInnerRect and CBorder::HelperGetCombinedThickness, which most panels use).
            //   Example (from ScrollBarMarginRoundingNoLayoutCycle test):
            //     A ScrollViewer has height 654.222229 running in 2.25x scale is properly layout rounded
            //     (654.222229 * 2.25 = 1472). The Grid above the VerticalScrollBar has Padding="1,1,1,1"
            //     which it subtracts off without rounding to produce 652.222229, which is not rounded
            //     (652.222229 * 2.25 = 1467.5).
            // If we proceed with the non-rounded size, this element might use that full availableSize
            // which may layout round at the end beyond the availableSize (in the above example,
            // 652.222229 will round up to 652.444444, bigger than the availableSize).
            // Layout round with Floor the frameworkAvailableSize to ensure we don't end up measuring and
            // rounding up beyond the real available space.
            if (IsFiniteF(frameworkAvailableSize.width))
            {
                frameworkAvailableSize.width = LayoutRoundFloor(frameworkAvailableSize.width);
            }
            if (IsFiniteF(frameworkAvailableSize.height))
            {
                frameworkAvailableSize.height = LayoutRoundFloor(frameworkAvailableSize.height);
            }
        }

        // Layout transforms would get processed here.

        // Adjust available size by Min/Max Width/Height

        GetMinMax(minWidth, maxWidth, minHeight, maxHeight);

        frameworkAvailableSize.width  = MAX(minWidth,  MIN(frameworkAvailableSize.width, maxWidth));
        frameworkAvailableSize.height = MAX(minHeight, MIN(frameworkAvailableSize.height, maxHeight));
    }
    else
    {
        // when in a transition, just take the passed in constraint without considering the above
        frameworkAvailableSize = availableSize;
    }

    // If the object has a managed peer and that is not one of the platform types, then there
    // might be an implementation of MeasureOverride defined by that type. Reverse P/Invoke to get that
    // implementation. Otherwise call the native virtual.
    if (HasManagedPeer() && IsCustomType())
    {
        if (EventEnabledMeasureOverrideBegin())
        {
            TraceMeasureOverrideBegin(reinterpret_cast<XUINT64>(this));
        }

        IFC_RETURN(FxCallbacks::FrameworkElement_MeasureOverride(this, frameworkAvailableSize.width,
            frameworkAvailableSize.height,
            &(desiredSize.width),
            &(desiredSize.height)));

        if (EventEnabledMeasureOverrideEnd())
        {
            TraceMeasureOverrideEnd(reinterpret_cast<XUINT64>(this));
        }
    }
    else
    {
        if (EventEnabledMeasureOverrideBegin())
        {
            TraceMeasureOverrideBegin(reinterpret_cast<XUINT64>(this));
        }

        IFC_RETURN(MeasureOverride(frameworkAvailableSize, desiredSize));

        if (EventEnabledMeasureOverrideEnd())
        {
            TraceMeasureOverrideEnd(reinterpret_cast<XUINT64>(this));
        }
    }



    if (!bInLayoutTransition)
    {
        // Maximize desired size with user provided min size. It's also possible that MeasureOverride returned NaN for either
        // width or height, in which case we should use the min size as well.
        desiredSize.width = std::max(desiredSize.width, minWidth);
        if (DirectUI::FloatUtil::IsNaN(desiredSize.width))
        {
            desiredSize.width = minWidth;
        }
        desiredSize.height = std::max(desiredSize.height, minHeight);
        if (DirectUI::FloatUtil::IsNaN(desiredSize.height))
        {
            desiredSize.height = minHeight;
        }

        // We need to round now since we save the values off, and use them to determine
        // if a layout clip will be applied.

        if (GetUseLayoutRounding())
        {
            desiredSize.width = LayoutRound(desiredSize.width);
            desiredSize.height = LayoutRound(desiredSize.height);
        }

        // Here is the "true minimum" desired size - the one that is
        // for sure enough for the control to render its content.
        IFC_RETURN(EnsureLayoutStorage());
        UnclippedDesiredSize = desiredSize;

        // More layout transforms processing here.

        if (desiredSize.width > maxWidth)
        {
            desiredSize.width = maxWidth;
        }

        if (desiredSize.height > maxHeight)
        {
            desiredSize.height = maxHeight;
        }

        // Transform desired size to layout slot space (placeholder for when we do layout transforms)

        //  Because of negative margins, clipped desired size may be negative.
        //  Need to keep it as XFLOATS for that reason and maximize with 0 at the
        //  very last point - before returning desired size to the parent.
        clippedDesiredWidth = desiredSize.width + roundedMarginWidth;
        clippedDesiredHeight = desiredSize.height + roundedMarginHeight;

        // only clip and constrain if the tree wants that.
        // currently only listviewitems do not want clipping
        if (!pLayoutManager->GetIsInNonClippingTree())
        {
            // In overconstrained scenario, parent wins and measured size of the child,
            // including any sizes set or computed, can not be larger then
            // available size. We will clip the guy later.
            if (clippedDesiredWidth > availableSize.width)
            {
                clippedDesiredWidth = availableSize.width;
            }

            if (clippedDesiredHeight > availableSize.height)
            {
                clippedDesiredHeight = availableSize.height;
            }
        }

        //  Note: unclippedDesiredSize is needed in ArrangeCore,
        //  because due to the layout protocol, arrange should be called
        //  with constraints greater or equal to child's desired size
        //  returned from MeasureOverride. But in most circumstances
        //  it is possible to reconstruct original unclipped desired size.

        desiredSize.width = MAX(0, clippedDesiredWidth);
        desiredSize.height = MAX(0, clippedDesiredHeight);
    }
    else
    {
        // in LT, need to take precautions
        desiredSize.width = MAX(desiredSize.width, 0.0f);
        desiredSize.height = MAX(desiredSize.height, 0.0f);
    }


    // We need to round again in case the desired size has been modified since we originally
    // rounded it.
    if (GetUseLayoutRounding())
    {
        desiredSize.width = LayoutRound(desiredSize.width);
        desiredSize.height = LayoutRound(desiredSize.height);
    }
    return S_OK;
}

void
CFrameworkElement::RaiseLoadingEventIfNeeded()
{
    if (!m_firedLoadingEvent &&
        ShouldRaiseEvent(EventHandle(KnownEventIndex::FrameworkElement_Loading)))
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        ASSERT(pEventManager);

        TraceFrameworkElementLoadingBegin();

        pEventManager->Raise(
            EventHandle(KnownEventIndex::FrameworkElement_Loading),
            FALSE /* bRefire */,
            this /* pSender */,
            NULL /* pArgs */,
            TRUE /* fRaiseSync */);

        TraceFrameworkElementLoadingEnd();

        m_firedLoadingEvent = true;
    }
}

_Check_return_
HRESULT
CFrameworkElement::ArrangeCore(XRECTF finalRect)
{
    CLayoutManager* pLayoutManager = VisualTree::GetLayoutManagerForElement(this);
    bool bInLayoutTransition = pLayoutManager ? pLayoutManager->GetTransitioningElement() == this : false;

    XUINT32 needsClipBounds = FALSE;

    XSIZEF arrangeSize = finalRect.Size();

    XFLOAT roundedMarginWidth = m_pLayoutProperties->m_margin.left + m_pLayoutProperties->m_margin.right;
    XFLOAT roundedMarginHeight = m_pLayoutProperties->m_margin.top + m_pLayoutProperties->m_margin.bottom;

    DirectUI::HorizontalAlignment ha = m_pLayoutProperties->m_horizontalAlignment;
    DirectUI::VerticalAlignment va = m_pLayoutProperties->m_verticalAlignment;

    XSIZEF unclippedDesiredSize = {0,0};
    XFLOAT minWidth = 0, maxWidth = 0, minHeight = 0, maxHeight = 0;
    XFLOAT effectiveMaxWidth = 0, effectiveMaxHeight = 0;

    XSIZEF oldRenderSize = {0,0};
    XSIZEF innerInkSize = {0,0};
    XSIZEF clippedInkSize = {0,0};
    XSIZEF clientSize = {0,0};
    XFLOAT offsetX = 0, offsetY = 0;
    XPOINTF oldOffset = {0,0};

    IFC_RETURN(EnsureLayoutStorage());

    unclippedDesiredSize = UnclippedDesiredSize;
    oldRenderSize = RenderSize;

    if (!bInLayoutTransition)
    {
        // Round the margins if necessary
        if (GetUseLayoutRounding())
        {
            roundedMarginWidth = LayoutRound(roundedMarginWidth);
            roundedMarginHeight = LayoutRound(roundedMarginHeight);
        }

        XFLOAT arrangeWidthWithoutRoundedMargin = MAX(arrangeSize.width - roundedMarginWidth, 0);
        arrangeSize.width = arrangeWidthWithoutRoundedMargin;

        XFLOAT arrangeHeightWithoutRoundedMargin = MAX(arrangeSize.height - roundedMarginHeight, 0);
        arrangeSize.height = arrangeHeightWithoutRoundedMargin;

        if (IsLessThanReal(arrangeSize.width, unclippedDesiredSize.width))
        {
            needsClipBounds = TRUE;
            arrangeSize.width = unclippedDesiredSize.width;
        }

        if (IsLessThanReal(arrangeSize.height, unclippedDesiredSize.height))
        {
            needsClipBounds = TRUE;
            arrangeSize.height = unclippedDesiredSize.height;
        }

        // Alignment==Stretch --> arrange at the slot size minus margins
        // Alignment!=Stretch --> arrange at the unclippedDesiredSize
        if (ha != DirectUI::HorizontalAlignment::Stretch)
        {
            arrangeSize.width = unclippedDesiredSize.width;
        }

        if (va != DirectUI::VerticalAlignment::Stretch)
        {
            arrangeSize.height = unclippedDesiredSize.height;
        }

        GetMinMax(minWidth, maxWidth, minHeight, maxHeight);

        // Layout transforms processed here

        // We have to choose max between UnclippedDesiredSize and Max here, because
        // otherwise setting of max property could cause arrange at less then unclippedDS.
        // Clipping by Max is needed to limit stretch here

        effectiveMaxWidth = MAX(unclippedDesiredSize.width, maxWidth);
        if (IsLessThanReal(effectiveMaxWidth, arrangeSize.width))
        {
            needsClipBounds = TRUE;
            arrangeSize.width = effectiveMaxWidth;
        }

        effectiveMaxHeight = MAX(unclippedDesiredSize.height, maxHeight);
        if (IsLessThanReal(effectiveMaxHeight, arrangeSize.height))
        {
            needsClipBounds = TRUE;
            arrangeSize.height = effectiveMaxHeight;
        }
    }

    // If the object has a managed peer and that is not one of the platform types, then there
    // might be an implementation of ArrangeOverride defined by that type. Reverse P/Invoke to get that
    // implementation. Otherwise call the native virtual.
    if (HasManagedPeer() && IsCustomType())
    {
        if (EventEnabledArrangeOverrideBegin())
        {
            TraceArrangeOverrideBegin(reinterpret_cast<XUINT64>(this));
        }

        IFC_RETURN(FxCallbacks::FrameworkElement_ArrangeOverride(this, arrangeSize.width,
            arrangeSize.height,
            &(innerInkSize.width),
            &(innerInkSize.height)));

        if (EventEnabledArrangeOverrideEnd())
        {
            TraceArrangeOverrideEnd(reinterpret_cast<XUINT64>(this));
        }
    }
    else
    {
        if (EventEnabledArrangeOverrideBegin())
        {
            TraceArrangeOverrideBegin(reinterpret_cast<XUINT64>(this));
        }

        IFC_RETURN(ArrangeOverride(arrangeSize, innerInkSize));

        if (EventEnabledArrangeOverrideEnd())
        {
            TraceArrangeOverrideEnd(reinterpret_cast<XUINT64>(this));
        }
    }

    // Here we use un-clipped InkSize because element does not know that it is
    // clipped by layout system and it shoudl have as much space to render as
    // it returned from its own ArrangeOverride
    // Inner ink size is not guaranteed to be rounded, but should be.
    // TODO: inner ink size currently only rounded if plateau > 1 to minimize impact in RC,
    // but should be consistently rounded in all plateaus.
    const auto scale = RootScale::GetRasterizationScaleForElement(this);
    if ((scale != 1.0f) && GetUseLayoutRounding())
    {
        innerInkSize.width = LayoutRound(innerInkSize.width);
        innerInkSize.height = LayoutRound(innerInkSize.height);
    }
    RenderSize = innerInkSize;

    if (!IsSameSize(oldRenderSize, innerInkSize))
    {
        OnActualSizeChanged();
    }

    if (!IsSameSize(oldRenderSize, innerInkSize))
    {
        (VisualTree::GetLayoutManagerForElement(this))->EnqueueForSizeChanged(this, oldRenderSize);
    }

    if (!bInLayoutTransition)
    {
        // ClippedInkSize differs from InkSize only what MaxWidth/Height explicitly clip the
        // otherwise good arrangement. For ex, DS<clientSize but DS>MaxWidth - in this
        // case we should initiate clip at MaxWidth and only show Top-Left portion
        // of the element limited by Max properties. It is Top-left because in case when we
        // are clipped by container we also degrade to Top-Left, so we are consistent.
        clippedInkSize.width = MIN(innerInkSize.width, maxWidth);
        clippedInkSize.height = MIN(innerInkSize.height, maxHeight);

        // remember we have to clip if Max properties limit the inkSize
        needsClipBounds |=
            IsLessThanReal(clippedInkSize.width, innerInkSize.width)
            ||  IsLessThanReal(clippedInkSize.height, innerInkSize.height);

        // Transform stuff here

        // Note that inkSize now can be bigger then layoutSlotSize-margin (because of layout
        // squeeze by the parent or LayoutConstrained=true, which clips desired size in Measure).

        // The client size is the size of layout slot decreased by margins.
        // This is the "window" through which we see the content of the child.
        // Alignments position ink of the child in this "window".
        // Max with 0 is necessary because layout slot may be smaller then unclipped desired size.
        clientSize.width = MAX(0, finalRect.Width - roundedMarginWidth);
        clientSize.height = MAX(0, finalRect.Height - roundedMarginHeight);

        // Remember we have to clip if clientSize limits the inkSize
        needsClipBounds |=
            IsLessThanReal(clientSize.width, clippedInkSize.width)
            ||  IsLessThanReal(clientSize.height, clippedInkSize.height);

        bool isAlignedByDirectManipulation = false;
        IFCFAILFAST(IsAlignedByDirectManipulation(&isAlignedByDirectManipulation));

        if (isAlignedByDirectManipulation)
        {
            // Skip the layout engine's contribution to the element's offsets when it is already aligned by DirectManipulation.

            if (m_pLayoutProperties->m_horizontalAlignment == DirectUI::HorizontalAlignment::Stretch)
            {
                // Check if the Stretch alignment needs to be overridden with a Left alignment.
                // The "IsStretchHorizontalAlignmentTreatedAsLeft" case corresponds to CFrameworkElement::ComputeAlignmentOffset's "degenerate Stretch to Top-Left" branch.
                // The "IsFinalArrangeSizeMaximized()" case is for text controls CTextBlock, CRichTextBlock and CRichTextBlockOverflow which stretch their desired width to the finalSize argument in their ArrangeOverride method.
                // The "(clippedInkSize.width == clientSize.width && unclippedDesiredSize.width < clientSize.width)" case is for 3rd party controls that stretch their desired width to the final arrange width too.
                bool isStretchAlignmentTreatedAsNear_New =
                    IsStretchHorizontalAlignmentTreatedAsLeft(DirectUI::HorizontalAlignment::Stretch, clientSize, clippedInkSize) ||
                    (clippedInkSize.width == clientSize.width && unclippedDesiredSize.width < clientSize.width) ||
                    IsFinalArrangeSizeMaximized();

                // Check if the overriding needs are changing by accessing the current status from the owning ScrollViewer control.
                bool isStretchAlignmentTreatedAsNear_Old = false;
                IFCFAILFAST(IsStretchAlignmentTreatedAsNear(true /*isForHorizontalAlignment*/, &isStretchAlignmentTreatedAsNear_Old));
                if (isStretchAlignmentTreatedAsNear_New != isStretchAlignmentTreatedAsNear_Old)
                {
                    // The overriding needs are changing - push the new status to the owning ScrollViewer control.
                    IFCFAILFAST(OnAlignmentChanged(TRUE /*fIsForHorizontalAlignment*/, TRUE /*fIsForStretchAlignment*/, isStretchAlignmentTreatedAsNear_New));
                }
            }

            if (m_pLayoutProperties->m_verticalAlignment == DirectUI::VerticalAlignment::Stretch)
            {
                // Check if the Stretch alignment needs to be overridden with a Top alignment.
                // The "IsStretchVerticalAlignmentTreatedAsTop" case corresponds to CFrameworkElement::ComputeAlignmentOffset's "degenerate Stretch to Top-Left" branch.
                // The "IsFinalArrangeSizeMaximized()" case is for text controls CTextBlock, CRichTextBlock and CRichTextBlockOverflow which stretch their desired height to the finalSize argument in their ArrangeOverride method.
                // The "(clippedInkSize.height == clientSize.height && unclippedDesiredSize.height < clientSize.height)" case is for 3rd party controls that stretch their desired height to the final arrange height too.
                bool isStretchAlignmentTreatedAsNear_New =
                    IsStretchVerticalAlignmentTreatedAsTop(DirectUI::VerticalAlignment::Stretch, clientSize, clippedInkSize) ||
                    (clippedInkSize.height == clientSize.height && unclippedDesiredSize.height < clientSize.height) ||
                    IsFinalArrangeSizeMaximized();

                // Check if the overriding needs are changing by accessing the current status from the owning ScrollViewer control.
                bool isStretchAlignmentTreatedAsNear_Old = false;
                IFCFAILFAST(IsStretchAlignmentTreatedAsNear(false /*isForHorizontalAlignment*/, &isStretchAlignmentTreatedAsNear_Old));
                if (isStretchAlignmentTreatedAsNear_New != isStretchAlignmentTreatedAsNear_Old)
                {
                    // The overriding needs are changing - push the new status to the owning ScrollViewer control.
                    IFCFAILFAST(OnAlignmentChanged(FALSE /*fIsForHorizontalAlignment*/, TRUE /*fIsForStretchAlignment*/, isStretchAlignmentTreatedAsNear_New));
                }
            }
        }
        else
        {
            ComputeAlignmentOffset(clientSize, clippedInkSize, offsetX, offsetY);
        }

        oldOffset = VisualOffset;

        VisualOffset.x = offsetX + finalRect.X + m_pLayoutProperties->m_margin.left;
        VisualOffset.y = offsetY + finalRect.Y + m_pLayoutProperties->m_margin.top;

        if (GetUseLayoutRounding())
        {
            VisualOffset.x = LayoutRound(VisualOffset.x);
            VisualOffset.y = LayoutRound(VisualOffset.y);
        }
    }
    else
    {
        VisualOffset.x = finalRect.X;
        VisualOffset.y = finalRect.Y;
    }
    // If the VisualOffset or RenderSize has changed, this element should be redrawn.
    {
        // Note: a similar check occurs in CUIElement::ArrangeInternal which calls this method, but the check there is between
        // the size passed in and the final size. The comparison here is between the previous size and the final size.

        if (oldRenderSize.width != RenderSize.width || oldRenderSize.height != RenderSize.height)
        {
            // Mark this element's rendering content as dirty.
            CUIElement::NWSetContentDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);

            // If this element has a render transform and a nonzero render transform origin, then the render transform
            // will need to be updated for the new element size. Mark it dirty. If it doesn't have either of these then no-op.
            // The transform will be marked dirty when either the RenderTransform or the RenderTransformOrigin changes.
            if (GetRenderTransform())
            {
                XPOINTF renderTransformOrigin = CUIElement::GetRenderTransformOrigin();
                if (renderTransformOrigin.x != 0 || renderTransformOrigin.y != 0)
                {
                    CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);
                }
            }
        }

        bool actualOffsetChanged = (VisualOffset.x != oldOffset.x) || (VisualOffset.y != oldOffset.y);
        if (actualOffsetChanged)
        {
            OnActualOffsetChanged();
        }
        if (actualOffsetChanged
            || ((oldRenderSize.width != RenderSize.width) && this->IsRightToLeft()))
        {
            // Mark this element's transform as dirty.
            CUIElement::NWSetTransformDirty(this, DirtyFlags::Render | DirtyFlags::Bounds);

        }
    }
    SetRequiresClip(needsClipBounds);

    return S_OK;
}

// Retrieves the current Stretch-to-Top/Left alignment overriding status from the owning ScrollViewer control.
// When isForHorizontalAlignment is True, the current Stretch-to-Left alignment overriding status is returned.
// The Stretch-to-Top alignment overriding status is returned otherwise.
_Check_return_ HRESULT
CFrameworkElement::IsStretchAlignmentTreatedAsNear(_In_ bool isForHorizontalAlignment, _Out_ bool* pIsStretchAlignmentTreatedAsNear)
{
    HRESULT hr = S_OK;

    *pIsStretchAlignmentTreatedAsNear = false;

    CInputServices* inputServices = GetContext()->GetInputServices();
    if (inputServices)
    {
        inputServices->AddRef();
        IFC(inputServices->IsStretchAlignmentTreatedAsNear(
            this,
            isForHorizontalAlignment,
            pIsStretchAlignmentTreatedAsNear));
    }

Cleanup:
    ReleaseInterface(inputServices);
    return hr;
}

// Returns true when an explicit horizontal Stretch alignment needs to actually be treated like a Left alignment by either our layout engine or DManip.
bool
CFrameworkElement::IsStretchHorizontalAlignmentTreatedAsLeft(DirectUI::HorizontalAlignment ha, XSIZEF& clientSize, XSIZEF& inkSize)
{
    return ha == DirectUI::HorizontalAlignment::Stretch && inkSize.width > clientSize.width;
}

// Returns true when an explicit vertical Stretch alignment needs to actually be treated like a Top alignment by either our layout engine or DManip.
bool
CFrameworkElement::IsStretchVerticalAlignmentTreatedAsTop(DirectUI::VerticalAlignment va, XSIZEF& clientSize, XSIZEF& inkSize)
{
    return va == DirectUI::VerticalAlignment::Stretch && inkSize.height > clientSize.height;
}

void
CFrameworkElement::ComputeAlignmentOffset(DirectUI::HorizontalAlignment ha, DirectUI::VerticalAlignment va, XSIZEF& clientSize, XSIZEF& inkSize, XFLOAT& offsetX, XFLOAT& offsetY)
{
    //this is to degenerate Stretch to Top-Left in case when clipping is about to occur
    //if we need it to be Center instead, simply remove these 2 ifs
    if (IsStretchHorizontalAlignmentTreatedAsLeft(ha, clientSize, inkSize))
    {
        ha = DirectUI::HorizontalAlignment::Left;
    }

    if (IsStretchVerticalAlignmentTreatedAsTop(va, clientSize, inkSize))
    {
        va = DirectUI::VerticalAlignment::Top;
    }
    //end of degeneration of Stretch to Top-Left

    if (ha == DirectUI::HorizontalAlignment::Center
        || ha == DirectUI::HorizontalAlignment::Stretch)
    {
        offsetX = (clientSize.width - inkSize.width) / 2;
    }
    else if (ha == DirectUI::HorizontalAlignment::Right)
    {
        offsetX = clientSize.width - inkSize.width;
    }
    else
    {
        offsetX = 0;
    }

    if (va == DirectUI::VerticalAlignment::Center
        || va == DirectUI::VerticalAlignment::Stretch)
    {
        offsetY = (clientSize.height - inkSize.height) / 2;
    }
    else if (va == DirectUI::VerticalAlignment::Bottom)
    {
        offsetY = clientSize.height - inkSize.height;
    }
    else
    {
        offsetY = 0;
    }
}

void
CFrameworkElement::ComputeAlignmentOffset(XSIZEF& clientSize, XSIZEF& inkSize, XFLOAT& offsetX, XFLOAT& offsetY)
{
    ComputeAlignmentOffset(
        m_pLayoutProperties->m_horizontalAlignment,
        m_pLayoutProperties->m_verticalAlignment,
        clientSize,
        inkSize,
        offsetX,
        offsetY);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the layout clip, if needed, based on the layout slot size.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFrameworkElement::UpdateLayoutClip(bool forceClipToRenderSize)
{
    if (GetRequiresClip() || forceClipToRenderSize)
    {
        XRECTF clipRect = {0,0,0,0};

        // TODO: Clip rect currently only rounded in plateau > 1 to minimize impact, but should be consistently rounded in all plateaus.
        const auto scale = RootScale::GetRasterizationScaleForElement(this);
        const bool roundClipRect = (scale != 1.0f) && GetUseLayoutRounding();

        // see if  MaxWidth/MaxHeight limit the element
        XFLOAT minWidth, maxWidth, minHeight, maxHeight;
        GetMinMax(minWidth, maxWidth, minHeight, maxHeight);

        XSIZEF inkSize = {0,0};
        XSIZEF layoutSlotSize = {0, 0};
        XFLOAT maxWidthClip = 0, maxHeightClip = 0;
        bool needToClipLocally = false;
        bool needToClipSlot = false;
        XFLOAT marginWidth = 0, marginHeight = 0;
        XSIZEF clippingSize = {0,0};
        CUIElement * pParent = NULL;

        IFC_RETURN(EnsureLayoutStorage());

        // this is in element's local rendering coord system
        inkSize = RenderSize;
        layoutSlotSize = FinalRect.Size();

        maxWidthClip = IsInfiniteF(maxWidth) ? inkSize.width : maxWidth;
        maxHeightClip = IsInfiniteF(maxHeight) ? inkSize.height : maxHeight;

        // If clipping is forced, ensure the clip is at least as small as the RenderSize.
        if (forceClipToRenderSize)
        {
            maxWidthClip = MIN(inkSize.width, maxWidthClip);
            maxHeightClip = MIN(inkSize.height, maxHeightClip);
            needToClipLocally = TRUE;
        }
        else
        {
            // need to clip if the computed sizes exceed MaxWidth/MaxHeight/Width/Height
            needToClipLocally = IsLessThanReal(maxWidthClip, inkSize.width)
                             || IsLessThanReal(maxHeightClip, inkSize.height);
        }

        // now lets say we already clipped by MaxWidth/MaxHeight, lets see if further clipping is needed
        inkSize.width = MIN(inkSize.width, maxWidth);
        inkSize.height = MIN(inkSize.height, maxHeight);

        //now see if layout slot should clip the element
        marginWidth = m_pLayoutProperties->m_margin.left + m_pLayoutProperties->m_margin.right;
        marginHeight = m_pLayoutProperties->m_margin.top + m_pLayoutProperties->m_margin.bottom;

        clippingSize.width = MAX(0, layoutSlotSize.width  - marginWidth);
        clippingSize.height = MAX(0, layoutSlotSize.height - marginHeight);

        // With layout rounding, MinMax and RenderSize are rounded. Clip size should be rounded as well.
        if (roundClipRect)
        {
            clippingSize.width = LayoutRound(clippingSize.width);
            clippingSize.height = LayoutRound(clippingSize.height);
        }

        needToClipSlot = IsLessThanReal(clippingSize.width, inkSize.width)
                      || IsLessThanReal(clippingSize.height, inkSize.height);

        if (needToClipSlot)
        {
            // The layout clip is created from the slot size determined in the parent's coordinate space,
            // but is set on the child, meaning it's affected by the child's transform/offset and is applied in
            // the child's coordinate space.  The inverse of the offset is applied to the clip to prevent the clip's
            // position from shifting as a result of the change in coordinates.
            XFLOAT offsetX, offsetY;
            ComputeAlignmentOffset(clippingSize, inkSize, offsetX, offsetY);

            if (roundClipRect)
            {
                offsetX = LayoutRound(offsetX);
                offsetY = LayoutRound(offsetY);
            }
            clipRect.X = -offsetX;
            clipRect.Y = -offsetY;
            clipRect.Width = clippingSize.width;
            clipRect.Height = clippingSize.height;

            if(needToClipLocally)
            {
                XRECTF tempRect = {0, 0, maxWidthClip, maxHeightClip};
                IntersectRect(&clipRect, &tempRect);
            }
        }
        else if (needToClipLocally)
        {
            // In this case clipRect starts at 0, 0 and max width/height clips are rounded due
            // to RenderSize and MinMax being rounded. So clipRect is already rounded.
            clipRect.Width = maxWidthClip;
            clipRect.Height = maxHeightClip;
        }

        // if we have difference between child and parent FlowDirection
        // then we have to change origin of Clipping rectangle
        // which allows us to visually keep it at the same place
        pParent = GetUIElementAdjustedParentInternal(FALSE /*fPublicParentsOnly*/);
        if (pParent && pParent->IsRightToLeft() != IsRightToLeft())
        {
            clipRect.X = RenderSize.width - (clipRect.X + clipRect.Width);
        }

        if (ShouldApplyLayoutClipAsAncestorClip())
        {
            // LayoutClipParentRect is in the parent coordinate space and is applied above any RenderTransform.
            // This is a behavior change starting with Redstone, the previous behavior was to apply the LayoutClip below RenderTransforms.
            // We think this behavior makes more sense - the LayoutClip should ideally act as an ancestor clip, not a self-clip.
            clipRect.X += GetActualOffsetX();
            clipRect.Y += GetActualOffsetY();
        }

        // If the rectangle has not changed, then bail out.
        if ((needToClipLocally || needToClipSlot) &&
            LayoutClipGeometry != NULL &&
            LayoutClipGeometry->m_rc.X == clipRect.X &&
            LayoutClipGeometry->m_rc.Y == clipRect.Y &&
            LayoutClipGeometry->m_rc.Width == clipRect.Width &&
            LayoutClipGeometry->m_rc.Height == clipRect.Height)
        {
            return S_OK;
        }

        if (LayoutClipGeometry)
        {
            // Release old clip geometry since we will either have a new one or none
            LayoutClipGeometry->Release();
        }

        if (needToClipLocally || needToClipSlot)
        {
            // We have a new one
            CREATEPARAMETERS cp(GetContext());
            IFC_RETURN(CRectangleGeometry::Create(reinterpret_cast<CDependencyObject**>(&LayoutClipGeometry), &cp));

            LayoutClipGeometry->m_rc = clipRect;
        }
        else
        {
            // We don't need to clip anymore
            LayoutClipGeometry = nullptr;
        }

        SetLayoutClipDirty();
    }
    else
    {
        IFC_RETURN(CUIElement::UpdateLayoutClip(forceClipToRenderSize));
    }

    return S_OK;
}

// Checks if the given dp is is the list of template bindings or not.
bool CFrameworkElement::IsPropertyTemplateBound(_In_ const CDependencyProperty* dp) const
{
    if (m_pTemplateBindingData != nullptr && m_pTemplateBindingData->m_pTemplateBindings != nullptr)
    {
        CXcpList<const CDependencyProperty>::XCPListNode* node = m_pTemplateBindingData->m_pTemplateBindings->GetHead();
        while (node != nullptr)
        {
            if (dp == node->m_pData)
            {
                return true;
            }

            node = node->m_pNext;
        }
    }

    // Nothing found.
    return false;
}

//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CFrameworkElement::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    auto core = GetContext();

    if (params.fIsLive &&
        params.fCheckForResourceOverrides == FALSE)
    {
        xref_ptr<CResourceDictionary> resources = GetResourcesNoCreate();

        if (resources &&
            resources->HasPotentialOverrides())
        {
            params.fCheckForResourceOverrides = TRUE;
        }
    }

    IFC_RETURN(CUIElement::EnterImpl(pNamescopeOwner, params));

    //Check for focus chrome property.
    if (params.fIsLive)
    {
        CValue isTemplateFocusTargetValue;
        IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::Control_IsTemplateFocusTarget), &isTemplateFocusTargetValue));
        if (isTemplateFocusTargetValue.AsBool())
        {
            IFC_RETURN(UpdateFocusAncestorsTarget(true /*shouldSet*/)); //Add pointer to the Descendant
        }
    }

    IFCPTR_RETURN(core);

    // Walk the list of events (if any) to keep watch of loaded events.
    if (params.fIsLive && m_pEventList)
    {
        CXcpList<REQUEST>::XCPListNode *pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
            if (pRequest && pRequest->m_hEvent.index != KnownEventIndex::UnknownType_UnknownEvent)
            {
                if (pRequest->m_hEvent.index == KnownEventIndex::FrameworkElement_Loaded)
                {
                    // Take note of the fact we added a loaded event to the event manager.
                    core->KeepWatch(WATCH_LOADED_EVENTS);
                }
            }
            pTemp = pTemp->m_pNext;
        }
     }

    // Apply style when element is live in the tree
    if (params.fIsLive)
    {
        if (m_eImplicitStyleProvider == ImplicitStyleProvider::None)
        {
            if (!GetStyle())
            {
                IFC_RETURN(ApplyStyle());
            }
        }
        else if (m_eImplicitStyleProvider == ImplicitStyleProvider::AppWhileNotInTree)
        {
            IFC_RETURN(UpdateImplicitStyle(m_pImplicitStyle, NULL, /*bForceUpdate*/FALSE, /*bUpdateChildren*/FALSE));
        }
    }

    m_firedLoadingEvent = false;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: LeaveImpl
//
//  Synopsis:
//      Causes the object and its "children" to leave scope.
//      If we have implicit style from parent then we should unapply it.
//      If we got implicit style from application resources while element was
//      in the live tree we should change the implicit style provider from
//      AppWhileInTree to AppWhileNotInTree
//
//------------------------------------------------------------------------
 _Check_return_ HRESULT CFrameworkElement::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
 {
    CDependencyObject *pImplicitStyleParent = NULL;
    auto core = GetContext();

    if (params.fIsLive)
    {
        //Check for focus chrome property.
        CValue isTemplateFocusTargetValue;
        IFC_RETURN(GetValue(GetPropertyByIndexInline(KnownPropertyIndex::Control_IsTemplateFocusTarget), &isTemplateFocusTargetValue));
        if (isTemplateFocusTargetValue.AsBool())
        {
            IFC_RETURN(UpdateFocusAncestorsTarget(false /*shouldSet*/)); //Remove the pointer to the descendant
        }

        if (params.fCheckForResourceOverrides == FALSE)
        {
            xref_ptr<CResourceDictionary> resources = GetResourcesNoCreate();

            if (resources &&
                resources->HasPotentialOverrides())
            {
                params.fCheckForResourceOverrides = TRUE;
            }
        }
    }

    if (params.fIsLive && !core->IsShuttingDown())
    {
        // Raise unloaded event on element if being removed from live tree

        CEventManager* pEventManager = core->GetEventManager();
        if (pEventManager)
        {
            xref_ptr<CEventArgs> spUnloadedArgs;
            spUnloadedArgs = make_xref<CRoutedEventArgs>();
            pEventManager->Raise(EventHandle(KnownEventIndex::FrameworkElement_Unloaded), FALSE, this, spUnloadedArgs);
        }
    }

    IFC_RETURN(CUIElement::LeaveImpl(pNamescopeOwner, params));

    // Update style when element has implicit style from parent and it is leave the tree
    if (m_eImplicitStyleProvider == ImplicitStyleProvider::Parent)
    {
        ASSERT(m_pImplicitStyleParentWeakRef);
        pImplicitStyleParent = m_pImplicitStyleParentWeakRef.lock_noref();

        // Don't update the implicit style if the element from which we got it is either in the middle of an enter/leave,
        // or in its destructor.

        if (pImplicitStyleParent
            && !pImplicitStyleParent->IsProcessingEnterLeave()
            && !pImplicitStyleParent->IsDestructing() )
        {
            // We are being disconnected from the implicit style parent so we need to clear out the parent style
            IFC_RETURN(UpdateImplicitStyle(
                m_pImplicitStyle,
                NULL /*pNewStyle*/,
                FALSE /*bForceUpdate*/,
                FALSE /*bUpdateChildren*/,
                TRUE /*isLeavingParentStyle*/
                ));
        }
    }
    // if implicit style was set from application resources mark it as app style while element is not in the tree
    else if (m_eImplicitStyleProvider == ImplicitStyleProvider::AppWhileInTree)
    {
        m_eImplicitStyleProvider = ImplicitStyleProvider::AppWhileNotInTree;
    }

    return S_OK;
 }

//------------------------------------------------------------------------
//
//  Method: GetActualOffsetX
//
//  Synopsis:
//  Overrides UIElement::GetActualOffsetX to account for margin.
//------------------------------------------------------------------------
_Check_return_ XFLOAT CFrameworkElement::GetActualOffsetX()
{
    return HasLayoutStorage() && GetIsParentLayoutElement() ? GetLayoutStorage()->m_offset.x : GetOffsetX() + m_pLayoutProperties->m_margin.left;
}

//------------------------------------------------------------------------
//
//  Method: GetActualOffsetY
//
//  Synopsis:
//  Overrides UIElement::GetActualOffsetY to account for margin.
//------------------------------------------------------------------------
_Check_return_ XFLOAT CFrameworkElement::GetActualOffsetY()
{
    return HasLayoutStorage() && GetIsParentLayoutElement() ? GetLayoutStorage()->m_offset.y : GetOffsetY() + m_pLayoutProperties->m_margin.top;
}

xref_ptr<CBrush> CFrameworkElement::GetBackgroundBrush() const
{
    return nullptr;
}

xref_ptr<CBrush> CFrameworkElement::GetBorderBrush() const
{
    return nullptr;
}

void CFrameworkElement::GetIndependentlyAnimatedBrushes(
    _Outptr_ CSolidColorBrush **ppFillBrush,
    _Outptr_ CSolidColorBrush **ppStrokeBrush)
{
    const auto& backgroundBrush = GetBackgroundBrush();
    if (backgroundBrush && backgroundBrush->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppFillBrush, static_cast<CSolidColorBrush *>(backgroundBrush.get()));
    }

    const auto& borderBrush = GetBorderBrush();
    if (borderBrush && borderBrush->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppStrokeBrush, static_cast<CSolidColorBrush *>(borderBrush.get()));
    }
}

XTHICKNESS CFrameworkElement::GetBorderThickness() const
{
    XTHICKNESS thickness = {0};
    return thickness;
}

XTHICKNESS CFrameworkElement::GetPadding() const
{
    XTHICKNESS padding = {0};
    return padding;

}

XCORNERRADIUS CFrameworkElement::GetCornerRadius() const
{
    XCORNERRADIUS cornerRadius = {0};
    return cornerRadius;
}

// If a non-zero corner radius is being used, and this element has children, we need to create
// a CompNode for this element so we can correctly apply rounded corner clipping to this element's
// children.
//
// If it's just this element itself with a rounded corner, then we'll draw the rounded corner via
// an alpha mask, and we don't need to apply a rounded corner clip via DComp. We don't want to pay
// for the perf cost of the rounded corner clip, so we avoid it if there are no children.
void CFrameworkElement::UpdateRequiresCompNodeForRoundedCorners()
{
    XCORNERRADIUS cornerRadius = GetCornerRadius();

    const bool hasRoundedCorner =
        cornerRadius.topLeft != 0
        || cornerRadius.topRight != 0
        || cornerRadius.bottomLeft != 0
        || cornerRadius.bottomRight != 0;

    const bool hasChildren =
        GetChildren() != nullptr
        && GetChildren()->GetCount() > 0;

    if (hasRoundedCorner && hasChildren)
    {
        if (!RequiresCompNodeForRoundedCorners())
        {
            SetRequiresComposition(CompositionRequirement::HasRoundedCorners, IndependentAnimationType::None);
        }
    }
    else
    {
        if (RequiresCompNodeForRoundedCorners())
        {
            UnsetRequiresComposition(CompositionRequirement::HasRoundedCorners, IndependentAnimationType::None);
        }
    }
}

DirectUI::BackgroundSizing CFrameworkElement::GetBackgroundSizing() const
{
    return DirectUI::BackgroundSizing::InnerBorderEdge;
}

bool CFrameworkElement::IsAutomationPeerFactorySet() const
{
    return m_fIsAutomationPeerFactorySet;
}

//------------------------------------------------------------------------
//
//  Method:   ComputeWidthInMinMaxRange
//
//  Synopsis:
//      Adjust the passed in width for minWidth/maxWidth attributes
//
//------------------------------------------------------------------------
_Check_return_ XCP_FORCEINLINE XFLOAT CFrameworkElement::ComputeWidthInMinMaxRange(XFLOAT width)
{
    return MAX(MIN(width, m_pLayoutProperties->m_eMaxWidth), m_pLayoutProperties->m_eMinWidth);
}

//------------------------------------------------------------------------
//
//  Method:   ComputeHeightInMinMaxRange
//
//  Synopsis:
//      Adjust the passed in height for minHeight/maxHeight attributes
//
//------------------------------------------------------------------------
_Check_return_ XCP_FORCEINLINE XFLOAT CFrameworkElement::ComputeHeightInMinMaxRange(XFLOAT height)
{
    return MAX(MIN(height, m_pLayoutProperties->m_eMaxHeight), m_pLayoutProperties->m_eMinHeight);
}


//------------------------------------------------------------------------
//
//  Method:   GetActualWidth
//
//  Synopsis:
//      RenderWidth if layout is present, else max of width and minwidth
//
//------------------------------------------------------------------------
_Check_return_ XFLOAT CFrameworkElement::GetActualWidth()
{
    XFLOAT width = m_eWidth;

    if (_isnan(width))
    {
        // If the Width is not set, but MinWidth is set
        // we set width to MinWidth, This way the next line would return
        // the proper value
        width = MIN(m_pLayoutProperties->m_eMinWidth, XFLOAT_INF);
    }

    // If the element is measure dirty and at the same time, doesn't
    // have a layout storage, this means that it has not been measured
    // due to its parent invisibility.
    // In such case, we should return 0.0f as the actual width.
    // Note that this condition can hold only if parent has never been
    // visible (Initially, Its Visibility is set to Collapsed).
    if (GetIsMeasureDirty() && !HasLayoutStorage())
    {
        width = 0.0f;
    }
    else if (HasLayoutStorage())
    {
        width = RenderSize.width;
    }
    else
    {
        width = ComputeWidthInMinMaxRange(width);
    }

    return width;
}

// Returns True when either the Width, MinWidth or MaxWidth property was set to a non-default value.
bool CFrameworkElement::IsWidthSpecified()
{
    return !_isnan(m_eWidth) ||
        (m_pLayoutProperties != nullptr && (m_pLayoutProperties->m_eMinWidth != 0.0 || _finite(m_pLayoutProperties->m_eMaxWidth)));
}

// Returns True when either the Height, MinHeight or MaxHeight property was set to a non-default value.
bool CFrameworkElement::IsHeightSpecified()
{
    return !_isnan(m_eHeight) ||
        (m_pLayoutProperties != nullptr && (m_pLayoutProperties->m_eMinHeight != 0.0 || _finite(m_pLayoutProperties->m_eMaxHeight)));
}

//------------------------------------------------------------------------
//
//  Method:   GetSpecifiedWidth
//
//  Synopsis:
//      Resolve the specified width regardless of layout. NaN if unspecified.
//
//------------------------------------------------------------------------
_Check_return_ XCP_FORCEINLINE XFLOAT CFrameworkElement::GetSpecifiedWidth()
{
    XFLOAT width = m_eWidth;

    if (_isnan(width))
    {
        // If the Width is not set, but MinWidth is set
        // we set width to MinWidth, This way the next line would return
        // the proper value
        width = MIN(m_pLayoutProperties->m_eMinWidth, XFLOAT_INF);
    }
    width = ComputeWidthInMinMaxRange(width);

    if (width == XFLOAT_INF)
    {
        width = 0.0f;
    }

    return width;
}

//------------------------------------------------------------------------
//
//  Method:   GetSpecifiedHeight
//
//  Synopsis:
//      Resolve the specified height regardless of layout. NaN if unspecified.
//
//------------------------------------------------------------------------
_Check_return_ XCP_FORCEINLINE XFLOAT CFrameworkElement::GetSpecifiedHeight()
{
    XFLOAT height = m_eHeight;

    if (_isnan(height))
    {
        height = MIN(m_pLayoutProperties->m_eMinHeight, XFLOAT_INF);
    }
    height = ComputeHeightInMinMaxRange(height);

    if (height == XFLOAT_INF)
    {
        height = 0.0f;
    }

    return height;
}


//------------------------------------------------------------------------
//
//  Method:   GetActualWidth
//
//  Synopsis:
//      RenderWidth if layout is present, else max of width and minwidth
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFrameworkElement::GetActualWidth(_Out_ XFLOAT* peWidth)
{
    *peWidth = GetActualWidth();
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   GetActualHeight
//
//  Synopsis:
//      RenderHeight if layout is present, else max of height and minheight
//
//------------------------------------------------------------------------
_Check_return_ XFLOAT CFrameworkElement::GetActualHeight()
{
    XFLOAT height = m_eHeight;

    if (_isnan(height))
    {
        // If the Height is not set, but MinHeight is set
        // we set height to MinHeight, This way the next line would return
        // the proper value
        height = MIN(m_pLayoutProperties->m_eMinHeight, XFLOAT_INF);
    }

    // If the element is measure dirty and at the same time, doesn't
    // have a layout storage, this means that it has not been measured
    // due to its parent invisibility.
    // In such case, we should return 0.0f as the actual height.
    // Note that this condition can hold only if parent has never been
    // visible (Initially, Its Visibility is set to Collapsed).
    if (GetIsMeasureDirty() && !HasLayoutStorage())
    {
        height = 0.0f;
    }
    else if (HasLayoutStorage())
    {
        height = RenderSize.height;
    }
    else
    {
        height = ComputeHeightInMinMaxRange(height);
    }

    return height;
}


//------------------------------------------------------------------------
//
//  Method:   GetActualHeight
//
//  Synopsis:
//      RenderHeight if layout is present, else max of height and minheight
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CFrameworkElement::GetActualHeight(_Out_ XFLOAT* peHeight)
{
    *peHeight = GetActualHeight();
     return S_OK;
}


// Sets the templated parent of this object, it should only be set once
// as the templated parent should not change during the life of an FE
void CFrameworkElement::SetTemplatedParentImpl(_In_ CDependencyObject* parent)
{
    ASSERT(m_pTemplateBindingData == nullptr);
    m_pTemplateBindingData.reset(new TemplateBindingData);
    // Create a WeakRef to the TemplatedParent.  It could be released and at that time, it would
    // be too tedious to unhook all the nested children.
    m_pTemplateBindingData->m_pTemplatedParentWeakRef = xref::get_weakref(parent);
}

_Check_return_ HRESULT CFrameworkElement::SetTemplateBinding(
    _In_ const CDependencyProperty* targetProperty,
    _In_ const CDependencyProperty* sourceProperty)
{
    HRESULT hr = S_OK;
    CDependencyObject* pTemplatedParent = GetTemplatedParent();

    if (pTemplatedParent == nullptr)
    {
        IFC(CErrorService::OriginateInvalidOperationError(GetContext(), AG_E_TEMPLATEBINDING_TEMPLATEDPARENT_NULL));
    }

    IFC(pTemplatedParent->SubscribeToPropertyChanges(sourceProperty, this, targetProperty));

    // Remember that we added a template binding for this property
    // (but only if necessary, we don't need to track custom properties)
    if (hr != S_FALSE)
    {
        if (m_pTemplateBindingData->m_pTemplateBindings == nullptr)
        {
            m_pTemplateBindingData->m_pTemplateBindings = new CXcpList<const CDependencyProperty>();
        }
        IFC(m_pTemplateBindingData->m_pTemplateBindings->Add(targetProperty));
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Method: IsPropertyBitSet
//
//  Synopsis:
//      Checks if the specified per property bit has been set for a given BitField
//
//  Parameters:
//      pdp                     --  The DependencyProperty, the flag of which is to be checked.
//      field                    --  The BitField to check
//
//------------------------------------------------------------------------
bool CFrameworkElement::IsPropertyBitSet(_In_ const CDependencyProperty *pdp, const BitField& field) const
{
    if (pdp->IsSparse())
    {
        ASSERT(false);
        return false;
    }

    // Return the bit
    return GetPropertyBitField(field, MetadataAPI::GetPropertySlotCount(GetTypeIndex()), MetadataAPI::GetPropertySlot(pdp->GetIndex()));
}

//------------------------------------------------------------------------
//
//  Method: SetIsPropertyBitSet
//
//  Synopsis:
//      Mark/clear the property bit for a specified BitField
//
//  Parameters:
//      pdp                     --  The DependencyProperty, the flag of which is to be set.
//      field                    --  The BitField containing the bit to set/clear
//      fSet                    --   Flag specifying whether or not to Set or Clear
//
//------------------------------------------------------------------------
void CFrameworkElement::SetIsPropertyBitSet(_In_ const CDependencyProperty* pdp, BitField& field, bool fSet)
{
    if (pdp->IsSparse())
    {
        ASSERT(false);
        return;
    }

    if(fSet)
    {
        SetPropertyBitField(field, MetadataAPI::GetPropertySlotCount(GetTypeIndex()), MetadataAPI::GetPropertySlot(pdp->GetIndex()));
    }
    else
    {
        ClearPropertyBitField(field, MetadataAPI::GetPropertySlotCount(GetTypeIndex()), MetadataAPI::GetPropertySlot(pdp->GetIndex()));
    }
}

//------------------------------------------------------------------------
//
//  Method: IsPropertySetByStyle
//
//  Synopsis:
//      Checks if the specified property has been set by a style
//
//  Parameters:
//      pdp                     --  The DependencyProperty, the flag of which is to be checked.
//
//------------------------------------------------------------------------
bool CFrameworkElement::IsPropertySetByStyle(_In_ const CDependencyProperty *pdp) const
{
    if (pdp->IsSparse())
    {
        if (m_pValueTable != nullptr)
        {
            SparseValueTable::iterator it = m_pValueTable->find(pdp->GetIndex());
            if (it != m_pValueTable->end())
            {
                return it->second.IsSetByStyle();
            }
        }
        return false;
    }
    else if (pdp->IsInheritedAttachedPropertyInStorageGroup())
    {
        // The flags for inherited attached properties in storage groups are in
        // the storage group.
        if (m_pInheritedProperties == nullptr)
        {
            // If there's no local m_pInheritedProperties then we can never have
            // set a local flag.
            return false;
        }
        else
        {
            // If the local m_pInheritedProperties belongs to another DO then we
            // haven't set a local value.
            return (m_pInheritedProperties->m_pWriter == this)
                && (m_pInheritedProperties->IsPropertyFlagSet(pdp, InheritedPropertyFlag::IsSetByStyle));
        }
    }
    else
    {
        return IsPropertyBitSet(pdp, m_setByStyle);
    }
}

//------------------------------------------------------------------------
//
//  Method: SetIsSetByStyle
//
//  Synopsis:
//      Mark/clear the specified property as having been set by a style
//
//  Parameters:
//      pdp   --  The DependencyProperty, the flag of which is to be set.
//      fSet  --  Flag specifying whether or not to Set or Clear
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CFrameworkElement::SetIsSetByStyle(_In_ const CDependencyProperty* pdp, bool fSet)
{
    if (pdp->IsSparse())
    {
        ASSERT(m_pValueTable != nullptr);
        (*m_pValueTable)[pdp->GetIndex()].SetIsSetByStyle(!!fSet);
    }
    else if (pdp->IsInheritedAttachedPropertyInStorageGroup())
    {
        // The flags for inherited attached properties in storage groups are in
        // the storage group.
        // We would normally expect the property to have been written prior to
        // calling SetIsSetByStyle, which would have caused m_pInheritedProperties
        // to exist and be owned by this DO.
        ASSERT(    m_pInheritedProperties != NULL
               &&  m_pInheritedProperties->m_pWriter == this);
        IFC_RETURN(EnsureInheritedProperties(this, pdp, FALSE));
        m_pInheritedProperties->SetPropertyFlag(pdp, InheritedPropertyFlag::IsSetByStyle, fSet);
    }
    else
    {
        SetIsPropertyBitSet(pdp, m_setByStyle, fSet);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   DisconnectFromTemplatedParent
//
//  Synopsis:
//      Removes all template bindings and clears template binding data
//------------------------------------------------------------------------
_Check_return_ HRESULT CFrameworkElement::DisconnectFromTemplatedParent()
{
    if (m_pTemplateBindingData)
    {
        if (m_pTemplateBindingData->m_pTemplateBindings)
        {
            CXcpList<const CDependencyProperty>::XCPListNode *pNode = m_pTemplateBindingData->m_pTemplateBindings->GetHead();

            if (pNode)
            {
                CControl* templatedParent = do_pointer_cast<CControl>(GetTemplatedParent());
                IFCEXPECT_NOTRACE_RETURN(templatedParent)

                while (pNode)
                {
                    if(templatedParent)
                    {
                        templatedParent->RemoveTemplateBinding(this, pNode->m_pData);
                    }
                    pNode = pNode->m_pNext;
                }
            }
        }

        ClearTemplateBindingData();
    }

    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   Parent
//
//  Synopsis:   property method implementation for FrameworkElement.Parent
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CFrameworkElement::Parent(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
)
{
    HRESULT hr = S_OK;
    CFrameworkElement* pThis = NULL;
    CDependencyObject* result = NULL;

    IFC(DoPointerCast(pThis, pObject));

    if (pThis && !cArgs && !pArgs && pResult)
    {
        // We are getting the value
        IFC(CFrameworkElement::get_Parent(pThis, &result));
        pResult->SetObjectNoRef(result);
        result = NULL;
    }
    else
    {
        //readonly property
        IFC(E_INVALIDARG);
    }

Cleanup:
    ReleaseInterface(result);
    RRETURN(hr);
}

_Check_return_
HRESULT
CFrameworkElement::get_Parent(
    _In_ CFrameworkElement* pObject,
    _Outptr_ CDependencyObject** ppReturnValue)
{
    SetInterface(*ppReturnValue, pObject->GetLogicalParentNoRef());
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Return the DO from which properties should inherit.
//
//  If fLogicalParent is passed as FALSE:
//
//      Returns the visual parent unless it is a popup root, when it returns
//      the value of the framework element dependency property 'Parent'.
//
//  If fLogicalParent is passed as TRUE:
//
//      Returns the value of the framework element dependency property 'Parent'
//      unless it's null, when it returns the visual parent.
//
//------------------------------------------------------------------------

_Check_return_ CDependencyObject* CFrameworkElement::GetInheritanceParentInternal(
    _In_opt_ bool fLogicalParent
) const
{
    CDependencyObject *pdoVisualParent      = CUIElement::GetInheritanceParentInternal();
    CDependencyObject *pdoInheritanceParent = pdoVisualParent; // By default inheritance parent is visual parent.

    // If this object's visual parent is PopupRoot or this object is a Popup
    // we inherit properties from the logical parent.
    if (fLogicalParent
        || (pdoVisualParent != NULL && pdoVisualParent->GetTypeIndex() == KnownTypeIndex::PopupRoot)
        || (pdoVisualParent == NULL && GetTypeIndex() == KnownTypeIndex::Popup))
    {
        // Get the framework element logical parent property, if set.

        // Since we're in a highly performance sensitive codepath, we use the
        // relatively fast IsPropertySetBySlot reather than
        // IsPropertyDefaultById.

        if (IsPropertySetBySlot(MetadataAPI::GetPropertySlot(KnownPropertyIndex::FrameworkElement_Parent)))
        {
            // Use the FrameworkElement logical parent property
            pdoInheritanceParent = m_pLogicalParent;
        }
    }

    // If logical parent was requested and is null, fall back to visual parent for inheritance.
    if (fLogicalParent && pdoInheritanceParent == NULL)
    {
        pdoInheritanceParent = pdoVisualParent;
    }

    return pdoInheritanceParent;
}


//-------------------------------------------------------------------------
//
//  Function:   AddLogicalChild
//
//  Synopsis:   sets the logical parent property on the element passed in.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CFrameworkElement::AddLogicalChild(_In_ CDependencyObject* pNewLogicalChild, _In_ bool bContentTemplateboundOnManagedSide)
{
    CFrameworkElement* pChild = NULL;

    pChild = do_pointer_cast<CFrameworkElement>(pNewLogicalChild);
    if(pChild && (pChild->m_pLogicalParent != this))
    {
        const CDependencyProperty* pdp = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::FrameworkElement_Parent);

        //if logical parent is already set, then content should be template bound. otherwise, this is an error.
        if(pChild->IsPropertyDefault(pdp))
        {
            pChild->EnsureLayoutProperties(false);
            pChild->m_pLogicalParent = this;

            //set the flags to indicate the property value is now set.
            IFC_RETURN(pChild->SetPropertyIsLocal(pdp));
        }
        else if(!GetTemplatedParent() || (!IsPropertyTemplateBound(GetContentProperty()) && !bContentTemplateboundOnManagedSide))
        {
            //it is OK to have logical parent already set if the value is coming via template binding.
            //currently only content properties have logical links. in future, if a non-content property
            //has a logical link with the property owner, then GetContentProperty should be replaced
            //by the actual property metadata.
            IFC_RETURN(this->SetAndOriginateError(E_NER_INVALID_OPERATION, ManagedError, AG_E_MANAGED_ELEMENT_ASSOCIATED));
        }
    }

    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   GetNamescopeParent
//
//  Synopsis:   returns the logical parent unless the logical parent is null.
//                   In that case, it returns the visual parent.
//-------------------------------------------------------------------------
CDependencyObject* CFrameworkElement::GetStandardNameScopeParent()
{
    CDependencyObject* pNamescopeParent = GetLogicalParentNoRef();

    // give preference to logical parent. if that is null,
    // then the namescope parent is the visual parent. logical parent
    // will be null at template boundaries, for generated ItemsControl
    // containers and for direct children of itemspresenter panel
    // unless those children are the items in itemscontrol.
    if(!pNamescopeParent)
    {
       pNamescopeParent = GetParentInternal(false);
    }
    return pNamescopeParent;
}


//-------------------------------------------------------------------------
//
//  Function:   GetBaseUri
//
//  Synopsis:   Walks up the tree looking for a base uri.
//
//-------------------------------------------------------------------------
IPALUri* CFrameworkElement::GetBaseUri()
{
    IPALUri* preferredBaseURI = GetPreferredBaseUri();

    // if this has a value, then return that
    if (preferredBaseURI)
    {
        AddRefInterface(preferredBaseURI);
        return preferredBaseURI;
    }

    CDependencyObject* pParent = GetParentInternal(false);

    // check the visual parent's base uri unless the visual parent is
    // the popup root. In the case, check the logical parent if popup
    // is in the live tree. If popup is in either, then the element gets the uri
    // from the root of the live tree. This is to ensure that Glyphs work inside
    // popup even when popup is not in the live tree.
    if(NULL != do_pointer_cast<CPopupRoot>(pParent))
    {
        CDependencyObject* pLogicalParent = GetLogicalParentNoRef();
        if(pLogicalParent && pLogicalParent->IsActive())
        {
            pParent = pLogicalParent;
        }
        else
        {
            pParent = pParent->GetPublicRootVisual();
        }
    }

    if (pParent)
    {
        return pParent->GetBaseUri();
    }

    return NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the cursor for this framework element.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT CFrameworkElement::SetCursor(_In_ MouseCursor cursor)
{
    m_eMouseCursor = cursor;
    if (IsActive())
    {
        if (CInputServices* inputServices = GetContext()->GetInputServices())
        {
            IFC_RETURN(inputServices->UpdateCursor(this));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get theme to set for this element during Theme Walk
//------------------------------------------------------------------------
Theming::Theme
CFrameworkElement::GetRequestedThemeOverride(_In_ Theming::Theme theme)
{
    auto highContrastTheme = theme & Theme::HighContrastMask;

    // If RequestedTheme is set, it takes precedence.
    auto requestedTheme = GetRequestedTheme();
    if (requestedTheme != DirectUI::ElementTheme::Default)
    {
        if (requestedTheme == DirectUI::ElementTheme::Dark)
        {
            theme = (Theme::Dark | highContrastTheme);
        }
        else if (requestedTheme == DirectUI::ElementTheme::Light)
        {
            theme = (Theme::Light | highContrastTheme);
        }
    }

    return theme;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notify element that theme has changed
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFrameworkElement::NotifyThemeChangedCore(
    _In_ Theming::Theme theme,
    _In_ bool fForceRefresh)
{
    // If RequestedTheme is set, freeze of theme related inherited
    // properties at the root of the subtree, so they don't inherit from this
    // element's parent, which may have a different theme
    if (GetRequestedTheme() != DirectUI::ElementTheme::Default)
    {
        IFC_RETURN(NotifyThemeChangedForInheritedProperties(theme,
            /* freezeInheritedPropertiesAtSubTreeRoot */ true));
    }

    // Notify base class that theme has changed
    IFC_RETURN(CUIElement::NotifyThemeChangedCore(theme, fForceRefresh));

    // Raise event if the effective theme is changing.
    RaiseActiveThemeChangedEventIfChanging(theme);

    return S_OK;
}

// Called when there are default (system/app) theme changes. This enables us to notify
// ActualThemeChanged listeners on this element when it's outside the live tree.
void CFrameworkElement::NotifyThemeChangedListeners(_In_ Theming::Theme theme)
{
    if (!IsActive())
    {
        // Raise event if the effective theme is changing.
        RaiseActiveThemeChangedEventIfChanging(theme);
    }
}

void CFrameworkElement::RaiseActiveThemeChangedEventIfChanging(_In_ Theming::Theme theme)
{
    // The actual/effective theme value on an element may change due to theme changes
    // in several places. The local theme value starts as None, which means default,
    // and doesn't change unless one of the following changes occurs. Though we may be
    // notified by these changes, the actual/effective theme value may remain the same.
    // For example, if the initial actual value is Dark by default from the system theme,
    // then setting RequestedTheme=Dark would trigger a theme walk here but wouldn't
    // change the actual value.
    // Possible causes of theme change notifications:
    //  - RequestedTheme change on this element or an ancestor.
    //  - RequestedTheme change on application.
    //  - System theme change.
    //  - System high contrast change.

    auto theming = GetContext()->GetFrameworkTheming();
    auto eventManager = GetContext()->GetEventManager();

    // Raise HighContrastChanged event if it's high contrast that's changing.
    if (theming->IsHighContrastChanging())
    {
        eventManager->Raise(EventHandle(KnownEventIndex::FrameworkElement_HighContrastChanged), TRUE /*refire*/, this, nullptr);

        // For backwards compatibility of ActualThemeChanged, we return only if
        // we are changing from HC to HC, or non-HC to HC. If we are changing
        // from HC to non-HC, we want to fire ActualThemeChanged as well.
        if (theming->HasHighContrastTheme())
        {
            return;
        }
    }

    // Raise ActualThemeChanged event if effective base (non-HighContrast) theme value is changing.
    auto oldTheme = GetTheme();
    auto oldBase = oldTheme == Theme::None ? theming->GetBaseTheme() : Theming::GetBaseValue(oldTheme);
    auto newBase = Theming::GetBaseValue(theme);
    if ((oldBase != newBase) || (oldTheme == Theme::None && theming->IsBaseThemeChanging()))
    {
        eventManager->Raise(EventHandle(KnownEventIndex::FrameworkElement_ActualThemeChanged), TRUE /*refire*/, this, nullptr);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Freeze inherited properties that are affected by Theme at this
//  element's level in the inheritance chain if
//  freezeInheritedPropertiesAtSubTreeRoot is TRUE. Otherwise unfreeze
//  these properties at this level so the element can start inheriting
//  again. Freeze is done when RequestedTheme is set for the element,
//  and UnFreeze is done when RequestedTheme is cleared. Currently
//  this is done only for the Foreground property.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CFrameworkElement::NotifyThemeChangedForInheritedProperties(
    _In_ Theming::Theme theme,
    _In_ bool freezeInheritedPropertiesAtSubTreeRoot)
{
    HRESULT hr = S_OK;
    TextFormatting **ppTextFormatting = NULL;
    TextFormatting *pTextFormatting = NULL;
    CBrush *pBrushNoRef = NULL;
    CDependencyObject *pBrushDO = NULL;
    CValue brushValue;
    const CDependencyProperty *pForegroundProperty = NULL;
    const CDependencyProperty *pControlForegroundProperty = NULL;
    auto oldRequestedThemeForSubTree = Theme::None;
    bool removeRequestedTheme = false;
    auto core = GetContext();

    // Get Foreground property for this element
    pControlForegroundProperty = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Control_Foreground);
    pForegroundProperty = InheritedProperties::GetCorrespondingInheritedProperty(this,
                                pControlForegroundProperty);

    // If this element has a Foreground property and it is set locally, by style or animated,
    // there is nothing to do, because that value will be used.
    // Otherwise set the new theme's foreground value as the value to inherit from.
    if (pForegroundProperty && (HasLocalOrModifierValue(pForegroundProperty) || IsPropertySetByStyle(pForegroundProperty)))
    {
        goto Cleanup;
    }

    if (freezeInheritedPropertiesAtSubTreeRoot)
    {
        // Freeze

        // Get storage for inherited property
        IFC(EnsureTextFormatting(this, NULL, FALSE));
        ppTextFormatting = GetTextFormattingMember();
        pTextFormatting = *ppTextFormatting;

        // Push theme that Resource lookup should use to get the property value
        oldRequestedThemeForSubTree = core->GetRequestedThemeForSubTree();
        if (oldRequestedThemeForSubTree != Theming::GetBaseValue(theme))
        {
            core->SetRequestedThemeForSubTree(theme);
            removeRequestedTheme = true;
        }

        // Get Foreground property value for theme
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strBrush, L"DefaultTextForegroundThemeBrush");
        IFC(core->LookupThemeResource(c_strBrush, &pBrushDO));
        pBrushNoRef = do_pointer_cast<CBrush>(pBrushDO);

        // Set new inherited foreground brush for the element whose theme is being changed,
        // into the group storage for the property at this element's level.
        // Also freeze the brush at this level, so that values from ancestors are not
        // inherited.
        if (pBrushNoRef)
        {
            IFC(pTextFormatting->SetForeground(this, pBrushNoRef));
            pTextFormatting->SetFreezeForeground(true);

            // Mark inheritance chain as dirty
            core->m_cInheritedPropGenerationCounter++;

            brushValue.WrapObjectNoRef(pTextFormatting->m_pForeground);
            IFC(MarkInheritedPropertyDirty(pControlForegroundProperty, &brushValue));
        }
    }
    else
    {
        // Unfreeze
        ppTextFormatting = GetTextFormattingMember();
        pTextFormatting = *ppTextFormatting;
        if (pTextFormatting)
        {
            pTextFormatting->SetFreezeForeground(false);

            // Mark inheritance chain as dirty
            core->m_cInheritedPropGenerationCounter++;
        }
    }

Cleanup:
    // Pop theme
    if (removeRequestedTheme)
    {
        core->SetRequestedThemeForSubTree(oldRequestedThemeForSubTree);
    }
    ReleaseInterface(pBrushDO);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process change in RequestedTheme property value
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFrameworkElement::OnRequestedThemeChanged()
{
    HRESULT hr = S_OK;
    auto theme = Theme::None;
    bool setIsSwitchingTheme = false;
    auto core = GetContext();

    switch (GetRequestedTheme())
    {
        case DirectUI::ElementTheme::Dark:
           theme = Theme::Dark;
           break;

        case DirectUI::ElementTheme::Light:
           theme = Theme::Light;
           break;

        case DirectUI::ElementTheme::Default:
        {
            // Use parent's requested theme if this property was cleared
            CDependencyObject *pParent = GetParentInternal();
            if (pParent)
            {
                theme = pParent->GetTheme();
            }
            // If there is no parent, or parent's theme has
            // not been set, default to the application's requested theme.
            if (theme == Theme::None)
            {
                theme = core->GetFrameworkTheming()->GetBaseTheme();
            }

            // UnFreeze values of theme related inherited properties at
            // this element's level because RequestedTheme was cleared.
            IFC(NotifyThemeChangedForInheritedProperties(theme,
                /* freezeInheritedPropertiesAtSubTreeRoot */ FALSE));

            break;
        }
    }

    if (!core->IsSwitchingTheme())
    {
       core->SetIsSwitchingTheme(true);
       setIsSwitchingTheme = true;
    }

    // Apply theme to this element and its subtree
    theme |= core->GetFrameworkTheming()->GetHighContrastTheme();
    IFC(NotifyThemeChanged(theme));

Cleanup:
    if (setIsSwitchingTheme)
    {
         core->SetIsSwitchingTheme(false);
    }

    RRETURN(hr);
}

namespace CoreImports
{
    _Check_return_ HRESULT CFrameworkElement_GetLayoutInformation(
        _In_ CFrameworkElement* pElement,
        _Out_ XUINT32* pcFloats,
        _Out_writes_(*pcFloats) XFLOAT** ppFloats)
    {
        HRESULT hr = S_OK;
        CLayoutStorage* pLS = pElement->GetLayoutStorage();

        if (pLS)
        {
            // We add one to accommodate the flags
            *pcFloats = (sizeof(CLayoutStorage) / sizeof(XFLOAT)) + 1;

            // A temp buffer, This will hold the LayoutStorage info PLUS one more XFLOAT to
            // hold the LayoutFlags
            XFLOAT* tempResult = new XFLOAT[*pcFloats];

            // Step 1: Copy LayoutStorage into the first (cFloats - 1) elements of the temp buffer
            memcpy(tempResult, &pLS->m_previousAvailableSize, (*pcFloats - 1) * sizeof(XFLOAT));

            // Step 2: Copy LayoutFlags into the last element of the buffer
            XUINT32 flags = pElement->GetLayoutFlagsRaw();
            XUINT32* pFlags = (XUINT32*)(&tempResult[*pcFloats-1]);
            *pFlags = flags;

            *ppFloats = tempResult;
        }
        else
        {
            // We add one to accommodate the flags
            *pcFloats = 1;
            XUINT32 flags = pElement->GetLayoutFlagsRaw();
            XFLOAT* pFlags = reinterpret_cast<XFLOAT*>(&flags);
            *ppFloats = new XFLOAT[*pcFloats];
            *ppFloats[0] = *pFlags;
        }

        RRETURN(hr);//RRETURN_REMOVAL
    }
}

XINT32 CFrameworkElement::IsDefaultHeight()
{
    return (_isnan(m_eHeight) || IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Height));
}

XINT32 CFrameworkElement::IsDefaultWidth()
{
    return (_isnan(m_eWidth) || IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Width));
}

xref_ptr<CVisualStateGroupCollection> CFrameworkElement::GetVisualStateGroupsNoCreate() const
{
    CValue result = CheckOnDemandProperty(KnownPropertyIndex::VisualStateManager_VisualStateGroups);
    return checked_sp_cast<CVisualStateGroupCollection>(result.DetachObject());
}

xref_ptr<CStyle> CFrameworkElement::GetStyle() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::FrameworkElement_Style, &result));
    return checked_sp_cast<CStyle>(result.DetachObject());
}

xref_ptr<CStyle> CFrameworkElement::GetActiveStyle() const
{
    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Style))
    {
        return GetStyle();
    }

    auto style = GetStyle();
    return style ? style : m_pImplicitStyle;
}

DirectUI::ElementTheme CFrameworkElement::GetRequestedTheme() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::FrameworkElement_RequestedTheme, &result));
    ASSERT(result.IsEnum());
    return static_cast<DirectUI::ElementTheme>(result.AsEnum());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders the border as per provided background and border geometries
//      This method is used by both D2D render walk and the print walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CFrameworkElement::AcceleratedBorderRenderCommon(
    _In_ const SharedRenderParams& sharedRP,
    _In_ const D2DRenderParams& d2dRP,
    _In_opt_ IPALAcceleratedGeometry *pPALBackgroundGeometry,
    _In_opt_ IPALAcceleratedBrush *pPALBackgroundBrush,
    _In_opt_ AcceleratedBrushParams *pBackgroundBrushParams,
    _In_opt_ IPALAcceleratedGeometry *pPALBorderGeometry,
    _In_opt_ IPALAcceleratedBrush *pPALBorderBrush,
    _In_opt_ AcceleratedBrushParams *pBorderBrushParams)
{
    ASSERT(OfTypeByIndex<KnownTypeIndex::Panel>()
        || OfTypeByIndex<KnownTypeIndex::Border>()
        || OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

    // If this element is not under layout control, then then width and height must
    // be specified.
    if (HasLayoutStorage() || (!IsDefaultWidth() && !IsDefaultHeight()))
    {
        if (pPALBackgroundBrush != nullptr && pPALBackgroundGeometry != nullptr)
        {
            IFC_RETURN(CGeometry::DrawAccelerated(
                pPALBackgroundGeometry,
                d2dRP,
                sharedRP.pCurrentTransform,
                0.0f,   // rStrokeThickness
                NULL,   // pStrokeStyle
                1.0f,   // opacity
                pPALBackgroundBrush,
                pBackgroundBrushParams));
        }

        if (pPALBorderBrush != nullptr && pPALBorderGeometry != nullptr)
        {
            IFC_RETURN(CGeometry::DrawAccelerated(
                pPALBorderGeometry,
                d2dRP,
                sharedRP.pCurrentTransform,
                0.0f,   // rStrokeThickness
                NULL,   // pStrokeStyle
                1.0f,   // opacity
                pPALBorderBrush,
                pBorderBrushParams));
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------
//
//  Synopsis:
//      Creates the D2D geometries and brush clips.
//
//---------------------------------------------------------------------
_Check_return_ HRESULT
CFrameworkElement::CreateBorderGeometriesAndBrushClipsCommon(
    const bool renderCollapsedMask,
    _In_ const D2DPrecomputeParams &cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform,
    _In_ AcceleratedBrushParams *pPALBackgroundBrushParams,
    _In_ AcceleratedBrushParams *pPALBorderBrushParams,
    _Outptr_opt_ IPALAcceleratedGeometry **ppPALBackgroundGeometry,
    _Outptr_opt_ IPALAcceleratedGeometry **ppPALBorderGeometry)
{
    ASSERT(OfTypeByIndex<KnownTypeIndex::Panel>()
        || OfTypeByIndex<KnownTypeIndex::Border>()
        || OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

    const bool generateBackground = (ppPALBackgroundGeometry != nullptr);
    const bool generateBorder = (ppPALBorderGeometry != nullptr);

    xref_ptr<IPALAcceleratedGeometry> backgroundGeometry;
    xref_ptr<IPALAcceleratedGeometry> borderGeometry;

    // If this element is not under layout control, then the width and height
    // must be specified.
    if (HasLayoutStorage() || (!IsDefaultWidth() && !IsDefaultHeight()))
    {
        const XCORNERRADIUS cornerRadius = GetCornerRadius();
        const bool extendBackgroundUnderBorder =
            (GetBackgroundSizing() == DirectUI::BackgroundSizing::OuterBorderEdge);

        // If we have rounded corners, we are going for a "complex drawing."
        // One of the very important things that a complex drawing handles is
        // making sure that the border thickness--if any--is calculated
        // correctly. For more info, refer to the DrawRoundedCornersRectangle
        // method, and check the 0.5 factor.
        const bool useComplexDrawing = CBorder::HasNonZeroCornerRadius(cornerRadius);

        XRECTF outerBounds = { 0.0f, 0.0f, GetActualWidth(), GetActualHeight() };
        XTHICKNESS borderThickness = GetBorderThickness();

        const bool useLayoutRounding = GetUseLayoutRounding();

        if (useLayoutRounding)
        {
            outerBounds.Width = LayoutRound(outerBounds.Width);
            outerBounds.Height = LayoutRound(outerBounds.Height);
            borderThickness = CBorder::GetLayoutRoundedThickness(this);
        }

        if (renderCollapsedMask)
        {
            //
            // Memory-saving optimization. We can collapse a rounded corner mask down to its four corners plus an extra
            // pixel in each dimension, then nine grid stretch that small mask back up to the full border.
            //
            // Use the same math that sets up the nine grid insets for the mask to generate the mask. Intuitively this
            // makes sense - we're setting up nine grid insets to keep the rounded corners unstretched when using the
            // mask, so we should use the same math to calculate how big those corners are when drawing the mask.
            //
            // Always ask for layout rounding here regardless of the UseLayoutRounding property set on the border. We want
            // nine grid insets that turn into whole numbers once multiplied by the rasterization scale. These insets
            // determine how much of the mask is mapped into the nine grid, not how much of the mask contains the border
            // stroke. Always mapping an integer number of pixels on all four sides guarantees that we don't introduce
            // any unnecessary blurriness. The border thickness itself is layout rounded above iff the border has layout
            // rounding turned on, and that's what contributes to the size of the border stroke.
            //
            XTHICKNESS ninegrid;
            HWWalk::GetNinegridForBorderParameters(borderThickness, cornerRadius, true /* useLayoutRounding */, this, &ninegrid);

            float onePixel = LayoutRound(1.0f); // Always layout round. We want the full pixel(s) to be available in the mask.

            outerBounds.Width = MIN(outerBounds.Width, ninegrid.left + onePixel + ninegrid.right);
            outerBounds.Height = MIN(outerBounds.Height, ninegrid.top + onePixel + ninegrid.bottom);
        }

        auto buildGeometry = [&borderThickness, &cornerRadius, &useComplexDrawing, &cp](
            const bool isOuter,
            const XRECTF& bounds,
            xref_ptr<IPALAcceleratedGeometry>& geometry)
            -> HRESULT
        {
            if (useComplexDrawing)
            {
                IFC_RETURN(CGeometryBuilder::DrawRoundedCornersRectangle(
                    geometry.ReleaseAndGetAddressOf(),
                    cp.GetFactory(),
                    bounds,
                    cornerRadius,
                    &borderThickness,
                    isOuter));
            }
            else
            {
                IFC_RETURN(cp.GetFactory()->CreateRectangleGeometry(
                    bounds,
                    geometry.ReleaseAndGetAddressOf()));
            }

            return S_OK;
        };

        if (outerBounds.Width != 0.0f && outerBounds.Height != 0.0f)
        {
            XRECTF innerBounds = {};
            CBorder::HelperDeflateRect(outerBounds, borderThickness, innerBounds);

            if (innerBounds.Width != 0.0f && innerBounds.Height != 0.0f)
            {
                // Building the border geometry is tricky. This geometry is the
                // difference between the outer rect and the inner rect. We can
                // calculate it by building a geometry based on the outer rect
                // and a second geometry based on the inner rect. We can then
                // combine them using the "exclude" mode, in a way that
                // effectively "cuts out" the inner from the outer.
                xref_ptr<IPALAcceleratedGeometry> outerGeometry;
                xref_ptr<IPALAcceleratedGeometry> innerGeometry;

                IFC_RETURN(buildGeometry(true /* isOuter */, outerBounds, outerGeometry));
                IFC_RETURN(buildGeometry(false /* isOuter */, innerBounds, innerGeometry));

                if (generateBorder)
                {
                    xref_ptr<IPALAcceleratedGeometry> combinedGeometry;

                    IFC_RETURN(cp.GetFactory()->CombineGeometry(
                        outerGeometry,
                        innerGeometry,
                        GeometryCombineMode::Exclude,
                        combinedGeometry.ReleaseAndGetAddressOf()));

                    borderGeometry = combinedGeometry;
                }

                if (generateBackground)
                {
                    backgroundGeometry = extendBackgroundUnderBorder ? outerGeometry : innerGeometry;
                }
            }
            else
            {
                // If the inner rect is empty, that means the border geometry is a solid rectangle.
                if (generateBorder)
                {
                    IFC_RETURN(buildGeometry(true /* isOuter */, outerBounds, borderGeometry));
                }

                // We might also need a geometry for the background, but only if this one extends under the border,
                // e.g. in case of a semi-transparent border.
                if (generateBackground && extendBackgroundUnderBorder)
                {
                    IFC_RETURN(buildGeometry(true /* isOuter */, outerBounds, backgroundGeometry));
                }
            }

            // Update the background brush transform & clip. This depends on the
            // bounds of the geometry, so they may need to be updated even if the
            // brush itself did not change.
            if (backgroundGeometry && GetBackgroundBrush())
            {
                const XRECTF& bounds = extendBackgroundUnderBorder ? outerBounds : innerBounds;
                XRECTF_RB backgroundBrushBounds = ToXRectFRB(bounds);

                IFC_RETURN(GetBackgroundBrush().get()->D2DEnsureDeviceIndependentResources(
                    cp,
                    pMyAccumulatedTransform,
                    &backgroundBrushBounds,
                    pPALBackgroundBrushParams));
            }

            // Update the border brush transform & clip. This depends on the bounds
            // of the geometry, so they may need to be updated even if the brush
            // itself did not change.
            if (borderGeometry && GetBorderBrush())
            {
                XRECTF_RB borderBrushBounds = ToXRectFRB(outerBounds);
                IFC_RETURN(GetBorderBrush()->D2DEnsureDeviceIndependentResources(
                    cp,
                    pMyAccumulatedTransform,
                    &borderBrushBounds,
                    pPALBorderBrushParams));
            }
        }
    }

    if (generateBackground)
    {
        *ppPALBackgroundGeometry = backgroundGeometry.detach();
    }

    if (generateBorder)
    {
        *ppPALBorderGeometry = borderGeometry.detach();
    }

    return S_OK;
}

// Provides the information required to render the focus rectangles using the 5 FE.FocusVisualXXX properties.
// If this FrameworkElement is a Control, then ppFocusTargetDescendant is set to the UIElement child which has
// the IsTemplateFocusTarget attached property set to True, if any.  Otherwise it's set to null.
// If such a ppFocusTargetDescendant UIElement exists, and it's a FrameworkElement, then its 5 FocusVisualXXX
// properties take precedence over this FrameworkElement's 5 properties.
// If neither the ppFocusTargetDescendant nor this FrameworkElement have explicitly set a property, then this
// FrameworkElement's default property value is used.
_Check_return_ HRESULT CFrameworkElement::GetFocusVisualProperties(
    _In_opt_ CUIElement*  pFocusTargetDescendant,
    _Outptr_ CBrush** ppPrimaryBrush,
    _Outptr_ CBrush** ppSecondaryBrush,
    _Out_ XTHICKNESS* pPrimaryThickness,
    _Out_ XTHICKNESS* pSecondaryThickness)
{
    *ppSecondaryBrush = nullptr;
    *ppPrimaryBrush = nullptr;
    memset(pSecondaryThickness, 0, sizeof(XTHICKNESS));
    memset(pPrimaryThickness, 0, sizeof(XTHICKNESS));

    // This code path is not expected to be hit when Application::FocusVisualKind returns FocusVisualKind::DottedLine
    ASSERT(CApplication::GetFocusVisualKind() != DirectUI::FocusVisualKind::DottedLine);

    auto pFocusTargetDescendantFE = do_pointer_cast<CFrameworkElement>(pFocusTargetDescendant);
    IFC_RETURN(GetPropertiesForFocusVisualType(pFocusTargetDescendantFE, FocusVisualType::Secondary, ppSecondaryBrush, pSecondaryThickness));
    IFC_RETURN(GetPropertiesForFocusVisualType(pFocusTargetDescendantFE, FocusVisualType::Primary, ppPrimaryBrush, pPrimaryThickness));

    return S_OK;
}

const CFrameworkElement* CFrameworkElement::FixValueGetterForPropertyNoRef(
    _In_opt_ const CFrameworkElement* const focusTargetDescendantFE,
    _In_ KnownPropertyIndex index) const
{
    return (focusTargetDescendantFE == nullptr || focusTargetDescendantFE->IsPropertyDefaultByIndex(index)) ? this :  focusTargetDescendantFE;
}

_Check_return_ HRESULT CFrameworkElement::GetPropertiesForFocusVisualType(
    _In_opt_ const CFrameworkElement* const focusTargetDescendantFE,
    _In_ FocusVisualType type,
    _Outptr_ CBrush** brush,
    _Out_ XTHICKNESS* thickness)
{
    const auto focusVisualBrushIndex = type == FocusVisualType::Secondary ?
            KnownPropertyIndex::FrameworkElement_FocusVisualSecondaryBrush :
            KnownPropertyIndex::FrameworkElement_FocusVisualPrimaryBrush;

    const auto focusVisualThicknessIndex = type == FocusVisualType::Secondary ?
        KnownPropertyIndex::FrameworkElement_FocusVisualSecondaryThickness :
        KnownPropertyIndex::FrameworkElement_FocusVisualPrimaryThickness;

    auto valueFENoRef = FixValueGetterForPropertyNoRef(focusTargetDescendantFE,focusVisualBrushIndex);

    CValue value;
    IFC_RETURN(valueFENoRef->GetValueByIndex(focusVisualBrushIndex, &value));
    auto focusBrush = value.DetachObject();
    ASSERT(focusBrush != nullptr);

    valueFENoRef = FixValueGetterForPropertyNoRef(focusTargetDescendantFE, focusVisualThicknessIndex);
    IFC_RETURN(valueFENoRef->GetValueByIndex(focusVisualThicknessIndex, &value));

    *thickness = *(value.AsThickness());
    *brush = checked_cast<CBrush>(focusBrush.detach());
    return S_OK;
}

_Check_return_ HRESULT CFrameworkElement::ActualTheme(
    _In_ CDependencyObject* object,
    _In_ UINT32 cArgs,
    _Inout_updates_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    ASSERT(cArgs == 0);

    if (cArgs > 0 || pArgs != nullptr || pResult == nullptr)
    {
        //readonly property
        IFC_RETURN(E_INVALIDARG);
    }

    // Get base (non-HighContrast) theme.
    // Fall back to default (system or app) theme if the local theme isn't set.
    auto baseTheme = Theming::GetBaseValue(object->GetTheme());
    if (baseTheme == Theme::None)
    {
        baseTheme = object->GetContext()->GetFrameworkTheming()->GetBaseTheme();
    }
    pResult->Set((DirectUI::ElementTheme)baseTheme, KnownTypeIndex::ElementTheme);

    return S_OK;
}

_Check_return_ HRESULT CFrameworkElement::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue* pValue,
    _In_ INT32 iListenerType,
    _Out_opt_ CValue* pResult,
    _In_ bool fHandledEventsToo)
{
    if (hEvent.index == KnownEventIndex::FrameworkElement_ActualThemeChanged)
    {
        // Register to receive theme change notifications so we can raise the event
        // for default (system/app) theme changes when the element happens to be outside
        // the live tree (i.e. not included in the theme walk from root).
        GetContext()->AddThemeChangedListener(this);
    }

    IFC_RETURN(CUIElement::AddEventListener(hEvent, pValue, iListenerType, pResult, fHandledEventsToo));

    return S_OK;
}

_Check_return_ HRESULT CFrameworkElement::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue* pValue)
{
    if (hEvent.index == KnownEventIndex::FrameworkElement_ActualThemeChanged)
    {
        GetContext()->RemoveThemeChangedListener(this);
    }

    IFC_RETURN(CUIElement::RemoveEventListener(hEvent, pValue));

    return S_OK;
}

void CFrameworkElement::NotifyMutableStyleValueChanged(KnownPropertyIndex propertyIndex)
{
    auto property = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(propertyIndex);

    IGNOREHR(InvalidateProperty(property, BaseValueSourceStyle));
}

// Stores a WarningContext with the provided type and info. This FrameworkElement's class name and instance name are added to the string array
// included in the memory dump, should it be created.
bool CFrameworkElement::StoreWarningContext(WarningContextLog::WarningContextType type, _In_ std::vector<std::wstring>& warningInfo, size_t framesToSkip)
{
    std::wstring warningInfoClassName(L"ClassName: ");
    warningInfoClassName.append(GetClassName().GetBuffer());
    warningInfo.push_back(std::move(warningInfoClassName));

    CValue valueName;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::DependencyObject_Name, &valueName));

    xstring_ptr name = valueName.AsString();

    if (!name.IsNullOrEmpty())
    {
        std::wstring warningInfoName(L"Name: ");
        warningInfoName.append(name.GetBuffer());
        warningInfo.push_back(std::move(warningInfoName));
    }

    return CUIElement::StoreWarningContext(type, warningInfo, framesToSkip + 1);
}
