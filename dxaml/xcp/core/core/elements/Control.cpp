// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ThemeResource.h"
#include "FocusProperties.h"
#include <CValueBoxer.h>
#include "ValidationErrorsCollection.h"
#include "ValidationErrorEventArgs.h"
#include "HasValidationErrorsChangedEventArgs.h"
#include "VisualStateManager.h"
#include "ValidationCommand.h"
#include "DXamlServices.h"
#include "CVisualStateManager2.h"
#include <Theme.h>
#include <FrameworkUdk/Containment.h>

// Bug 47229546: [1.4 servicing] The File Explorer's Details pane steals focus unexpectedly
#define WINAPPSDK_CHANGEID_47229546 47229546

//  Class:  CControl
//
//  Synopsis:
//      Native class to handle native side interactions for custom controls.
//  At time of initial creation, custom controls can only be done via
//  managed code by deriving from the managed code peer of this class.
//
//  Note: The original Roger's custom control implementation will be moved
//  to other place. CControl will be a base class for all controls.
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Method: SetValue
//
//  Synopsis:
//      Override to clear visual children when template property is changed
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CControl::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP != nullptr)
    {
        switch (args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::Control_Template:
            {
                auto pOldTemplate = GetTemplate();

                // If template has not changed, don't unapply current template, for
                // better perf and to prevent duplicate OnApplyTemplate calls.
                if (args.m_value.AsObject() != pOldTemplate)
                {
                    // Reset the template bindings for this control
                    ClearPropertySubscriptions();

                    // When the control template property is set, we clear the visual children
                    CUIElement* pUIElement = this->GetFirstChildNoAddRef();
                    if (pUIElement)
                    {
                        CFrameworkTemplate* pNewTemplate = NULL;
                        if (args.m_value.GetType() == valueObject)
                        {
                            IFC_RETURN(DoPointerCast(pNewTemplate, args.m_value.AsObject()));
                        }
                        else if (args.m_value.GetType() != valueNull)
                        {
                            IFC_RETURN(E_INVALIDARG);
                        }
                        IFC_RETURN(this->RemoveChild(pUIElement));
                        IFC_RETURN(GetContext()->RemoveNameScope(this, Jupiter::NameScoping::NameScopeType::TemplateNameScope));
                    }
                }
                break;
            }

            case KnownPropertyIndex::Control_DefaultStyleKey:
            {
                if (m_fIsBuiltInStyleApplied)
                {
                    IFC_RETURN(E_FAIL);
                }
                break;
            }

            case KnownPropertyIndex::Control_FontFamily:
            {
                if (args.m_value.IsNull() &&
                    (GetContext()->IsSettingValueFromManaged(this) || ParserOwnsParent()))
                {
                    HRESULT hrToOriginate = E_NER_ARGUMENT_EXCEPTION;
                    xephemeral_string_ptr parameters[2];

                    ASSERT(parameters[0].IsNull());
                    args.m_pDP->GetName().Demote(&parameters[1]);

                    IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_PROPERTY_INVALID, 2, parameters));
                    IFC_RETURN(hrToOriginate);
                }
                break;
            }
        }
    }

    IFC_RETURN(CFrameworkElement::SetValue(args));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pulls all non-locally set inherited properties from the parent.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CControl::PullInheritedTextFormatting()
{
    HRESULT         hr = S_OK;
    TextFormatting *pParentTextFormatting = NULL;

    IFCEXPECT_ASSERT(m_pTextFormatting != NULL);
    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC(GetParentTextFormatting(&pParentTextFormatting));

        // Process each TextElement text core property one by one.

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_FontFamily))
        {
            IFC(m_pTextFormatting->SetFontFamily(this, pParentTextFormatting->m_pFontFamily));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_Foreground)
            && !m_pTextFormatting->m_freezeForeground)
        {
            IFC(m_pTextFormatting->SetForeground(this, pParentTextFormatting->m_pForeground));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Language))
        {
            m_pTextFormatting->SetLanguageString(pParentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(pParentTextFormatting->GetResolvedLanguageStringNoRef());
            m_pTextFormatting->SetResolvedLanguageListString(pParentTextFormatting->GetResolvedLanguageListStringNoRef());
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_FontSize))
        {
            m_pTextFormatting->m_eFontSize = pParentTextFormatting->m_eFontSize;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_FontWeight))
        {
            m_pTextFormatting->m_nFontWeight = pParentTextFormatting->m_nFontWeight;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_FontStyle))
        {
            m_pTextFormatting->m_nFontStyle = pParentTextFormatting->m_nFontStyle;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_FontStretch))
        {
            m_pTextFormatting->m_nFontStretch = pParentTextFormatting->m_nFontStretch;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_CharacterSpacing))
        {
            m_pTextFormatting->m_nCharacterSpacing = pParentTextFormatting->m_nCharacterSpacing;
        }

        m_pTextFormatting->m_nTextDecorations = pParentTextFormatting->m_nTextDecorations;

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection = pParentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::Control_IsTextScaleFactorEnabled) &&
            IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal))
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
//  Method: PropagateInheritedProperty
//
//  Synopsis:
//      This is called from SetValue when inherited properties
//      change for this control. This method recursively walks all children
//      and invalidates measure on any nodes that have a cached inherited
//      property bag.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CControl::PropagateInheritedProperty(_In_ CUIElement *pUIElement, _In_ const CDependencyProperty *pdp)
{
    HRESULT hr = S_OK;
    CUIElement *pChild = NULL;
    CUIElementCollection *pChildren = NULL;

    pChildren = static_cast<CUIElementCollection*>(pUIElement->GetChildren());

    if (pChildren && pChildren->GetCount() > 0)
    {
        for (XUINT32 i = 0; i < pChildren->GetCount(); i++)
        {
            pChild = static_cast<CUIElement*>(pChildren->GetItemWithAddRef(i));
            if (pChild)
            {
                if (pChild->HasInheritedProperties())
                {
                    // If current element has a local value for this property, we can stop the walk here.
                    if (pChild->OfTypeByIndex(pdp->GetDeclaringType()->m_nIndex) && !pChild->IsPropertyDefault(pdp))
                    {
                        break;
                    }
                    // Otherwise, invalidate measure on the current node.
                    pChild->InvalidateMeasure();
                }

                // And continue walking the tree down...
                IFC(PropagateInheritedProperty(pChild, pdp));
            }
            ReleaseInterface(pChild);
        }
    }

Cleanup:
    ReleaseInterface(pChild);
    RRETURN(hr);
}

_Check_return_ HRESULT
CControl::GetImplementationRoot(_Outptr_result_maybenull_ CUIElement** ppResult)
{
    *ppResult = GetImplementationRoot().detach();
    return S_OK;
}

xref_ptr<CUIElement> CControl::GetImplementationRoot()
{
    return xref_ptr<CUIElement>(GetFirstChildNoAddRef());
}

CControl::~CControl()
{
    VERIFYHR(RemoveFocusEngagement());
    ReleaseInterface(m_pTemplate);
    ReleaseInterface(m_pBorderBrush);
    ReleaseInterface(m_pDefaultStyleKey);

    // If we still have any template children that haven't been deleted,
    // clear their back-references, so that they don't reference this
    // object during their cleanup.
    ClearPropertySubscriptions();

    ReleaseInterface(m_pBackground);
    ReleaseInterface(m_pBuiltInStyle);
    VERIFYHR(GetContext()->RemoveNameScope(this, Jupiter::NameScoping::NameScopeType::TemplateNameScope));
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::GetTemplateChild()
//
//  Synopsis:   This will return a named child of the template, which is not
//              otherwise accessible.
//
//-------------------------------------------------------------------------
xref_ptr<CDependencyObject> CControl::GetTemplateChild(_In_ const xstring_ptr_view& strChildName) const
{
    return xref_ptr<CDependencyObject>(GetContext()->GetNamedObject(strChildName, this, Jupiter::NameScoping::NameScopeType::TemplateNameScope));
}

xref_ptr<CControlTemplate> CControl::GetTemplate() const
{
    return xref_ptr<CControlTemplate>(m_pTemplate);
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::SupportsBuiltInStyles
//
//  Synopsis:   Check whether this control supports built-in styles. All
//              Control-derived controls support built-in styles, except
//              UserControl.
//
//-------------------------------------------------------------------------
_Check_return_
bool
CControl::SupportsBuiltInStyles()
{
    return !OfTypeByIndex<KnownTypeIndex::UserControl>();
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::EnterImpl
//
//  Synopsis:   Called when the control enters the tree. If the builtin
//              style hasn't been applied yet, do so now, but only if this
//              is not a subclass.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CControl::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(CFrameworkElement::EnterImpl(pNamescopeOwner, params));

    // Apply any built-in styles.
    if (params.fIsLive)
    {
        if (SupportsBuiltInStyles() && !m_fIsBuiltInStyleApplied)
        {
            // When we apply the built-in style, we may resolve theme resources in doing so
            // that haven't yet been resolved - for example, if a property value references
            // a resource that then references another resource.
            // We need to make sure we're operating under the correct theme during that resolution.
            bool removeRequestedTheme = false;
            const auto theme = GetTheme();
            const auto oldRequestedThemeForSubTree = GetRequestedThemeForSubTreeFromCore();

            if (theme != Theming::Theme::None && Theming::GetBaseValue(theme) != oldRequestedThemeForSubTree)
            {
                SetRequestedThemeForSubTreeOnCore(theme);
                removeRequestedTheme = true;
            }

            auto themeGuard = wil::scope_exit([&] {
                if (removeRequestedTheme)
                {
                    SetRequestedThemeForSubTreeOnCore(oldRequestedThemeForSubTree);
                }
            });

            IFC_RETURN(ApplyBuiltInStyle());
        }

        // Initialize StateTriggers at this time.  We need to wait for this to enter a visual tree
        // since we need for it to be part of the main visual tree to know which visual tree's
        // qualifier context to use.  We also need to force an update because our visual tree root
        // may have changed since the last enter.
        IFC_RETURN(CVisualStateManager2::InitializeStateTriggers(this, true /* forceUpdate */));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::LeaveImpl
//
//  Synopsis:   Called when the control leaves the tree.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CControl::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(RemoveFocusEngagement());

    if (IsFocused() && GetContext())
    {
        CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this);

        if(pFocusManager)
        {
            // Don't move focus to next focusable element when proofing menu is dismissed, CPopup::Close will restore the focus back to the original floatie element
            if (!FxCallbacks::TextControlFlyout_IsElementChildOfProofingFlyout(this))
            {
                // Set the focus on the next focusable element.
                // If we remove the currently focused element from the live tree, inside a GettingFocus or LosingFocus handler,
                // we failfast. This is being tracked by Bug 9840123
                IFCFAILFAST(pFocusManager->SetFocusOnNextFocusableElement(GetFocusState(), !params.fVisualTreeBeingReset));
            }
        }

        // Ensure this control doesn't have a focus
        UpdateFocusState(DirectUI::FocusState::Unfocused);
    }

    IFC_RETURN(CFrameworkElement::LeaveImpl(pNamescopeOwner, params));


    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   CControl::MeasureOverride()
//
//  Synopsis:   This will return the desired size of the first (and only)
//              child.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CControl::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    CUIElement* pChild = (CUIElement*) GetFirstChildNoAddRef();
    if (pChild)
    {
        IFC_RETURN(pChild->Measure(availableSize));
        IFC_RETURN(pChild->EnsureLayoutStorage());
        desiredSize = pChild->DesiredSize;
    }
    else
    {
        desiredSize.height = desiredSize.width = 0.0f;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::ArrangeOverride()
//
//  Synopsis:   This will arrange the first (and only) child.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CControl::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;

    CUIElement* pChild = (CUIElement*) GetFirstChildNoAddRef();
    if (pChild)
    {
        XRECTF arrangeRect = {0, 0, finalSize.width, finalSize.height};
        IFC(pChild->Arrange(arrangeRect));
    }

Cleanup:
    newFinalSize = finalSize;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::SubscribeToPropertyChanges()
//
//  Synopsis:   Adds a subscription record for this property, the
//              pDOTarget object will be updated every time the ptrSource property
//              changes
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CControl::SubscribeToPropertyChanges(
    _In_ const CDependencyProperty* sourceProperty,
    _In_ CDependencyObject* target,
    _In_ const CDependencyProperty* targetProperty)
{
    HRESULT hr = S_OK;

    // TFS#836261:Template bindings behave differently in the core than in the framework.
    if (sourceProperty->IsSparse() || targetProperty->IsSparse())
    {
        // Peer needs to start participating in framework tree because it is used in
        // template binding.
        IFC(target->SetParticipatesInManagedTreeDefault());

        // If the target object does not have a managed peer it is time to create one.
        IFC(target->EnsurePeer());

        IFC(FxCallbacks::FrameworkCallbacks_SetTemplateBinding(this, sourceProperty, target, targetProperty));
    }
    else
    {
        IFC(SubscribeToCoreProperty(sourceProperty, target, targetProperty));
        // Returns S_FALSE if the subscription failed due to type mismatch
    }

Cleanup:
    // Indicate if this was a custom property; if so, we don't need to
    // keep track of it.
    if (hr == S_OK && sourceProperty->IsSparse())
    {
        hr = S_FALSE;
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::RemoveTemplateBinding()
//
//  Synopsis:   Removes a template binding subscription for this control
//
//-------------------------------------------------------------------------

void CControl::RemoveTemplateBinding(
    _In_ const CDependencyObject* const targetObject,
    _In_ const CDependencyProperty* const targetProperty)
{
    if (m_propertySubscriptions)
    {
        for (auto& propertySubscription : *m_propertySubscriptions)
        {
            // If we find a subscription for this target do and target dp then remove it

            auto found = std::find_if(
                propertySubscription.m_subscriptions.begin(),
                propertySubscription.m_subscriptions.end(),
                [targetObject, targetProperty](const TargetPropertySubscription& tps)
                {
                    return tps.m_targetObject == targetObject && tps.m_targetProperty == targetProperty;
                });

            if (found != propertySubscription.m_subscriptions.end())
            {
                if ((found + 1) != propertySubscription.m_subscriptions.end())
                {
                    std::swap(
                        *found,
                        propertySubscription.m_subscriptions.back());
                }

                propertySubscription.m_subscriptions.pop_back();
                break;
            }
        }
    }
}

_Check_return_ HRESULT CControl::UpdateEngagementState(bool engaging)
{
    IFC_RETURN(FxCallbacks::Control_UpdateEngagementState(this, engaging));
    return S_OK;
}

//-------------------------------------------------------------------------
//  Removes Engagement state (if already set) on the control
//-------------------------------------------------------------------------
_Check_return_ HRESULT CControl::RemoveFocusEngagement()
{
    if (IsFocusEngaged())
    {
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::Control_IsFocusEngaged, false));
    }

    return S_OK;
}

//-----------------------------------------------------------------------------------
//  Sets Focus Engagement on a control, if
//      1. The control (or one of its descendants) already has focus
//      2. Control has IsEngagementEnabled set to true
//------------------------------------------------------------------------------------
_Check_return_ HRESULT CControl::SetFocusEngagement()
{
    CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this);
    if (pFocusManager)
    {
        if (IsFocusEngaged())
        {
            const bool hasFocusedElement = FocusProperties::HasFocusedElement<CDependencyObject>(this);
            //Check to see if the element or any of it's descendants has focus
            if (!hasFocusedElement)
            {
                IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::Control_IsFocusEngaged, false));
                IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_FOCUSENGAGEMENT_CANT_ENGAGE_WITHOUT_FOCUS));
            }
            if (!IsFocusEngagementEnabled())
            {
                IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::Control_IsFocusEngaged, false));
                IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_FOCUSENGAGEMENT_CANT_ENGAGE_WITHOUT_ENGAGEMENT_ENABLED));
            }

            //Control is focused and has IsFocusEngagementEnabled set to true
            pFocusManager->SetEngagedControl(this);
            IFC_RETURN(UpdateEngagementState(true /*engaging*/));
        }
        else if(pFocusManager->GetEngagedControlNoRef() != nullptr) //prevents re-entrancy because we set the property to false above in error cases.
        {
            pFocusManager->SetEngagedControl(nullptr /*No control is now engaged*/);
            IFC_RETURN(UpdateEngagementState(false /*Disengage*/));

            CPopupRoot* popupRoot = nullptr;
            IFC_RETURN(VisualTree::GetPopupRootForElementNoRef(this, &popupRoot));
            popupRoot->ClearWasOpenedDuringEngagementOnAllOpenPopups();
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::LookupPropertySubscriptions()
//
//  Synopsis:   Looks up the PropertySubscriptions struct associated with the
//              given core DP
//
//-------------------------------------------------------------------------

PropertySubscriptions* CControl::LookupPropertySubscriptions(
    _In_ const CDependencyProperty* const sourceProperty)
{
    if (!m_propertySubscriptions)
    {
        return nullptr;
    }

    auto found = std::find_if(
        m_propertySubscriptions->begin(),
        m_propertySubscriptions->end(),
        [sourceProperty](const PropertySubscriptions& ps)
        {
            return ps.m_sourceProperty == sourceProperty;
        });

    return (found != m_propertySubscriptions->end()) ? &(*found) : nullptr;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::EnsurePropertySubscriptions()
//
//  Synopsis:   Returns the property subscriptions for the given DP if it exsits
//              allocates a new one if it does not exist
//
//-------------------------------------------------------------------------

PropertySubscriptions* CControl::EnsurePropertySubscriptions(
    _In_ const CDependencyProperty* const sourceProperty)
{
    PropertySubscriptions* subscriptions = LookupPropertySubscriptions(sourceProperty);

    // If nothing was found then we need to add it to the list and return it
    if (!subscriptions)
    {
        if (!m_propertySubscriptions)
        {
            m_propertySubscriptions = std::make_unique<std::vector<PropertySubscriptions>>();
            m_propertySubscriptions->reserve(c_preallocatedSubscriptions);
        }

        m_propertySubscriptions->emplace_back(sourceProperty);
        subscriptions = &m_propertySubscriptions->back();
    }

    return subscriptions;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::SubscribeToCoreProperty()
//
//  Synopsis:   Adds a subscription record for this core property, the
//              target object and dp will be updated
//              every time the pdpSource property changes
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CControl::SubscribeToCoreProperty(
    _In_ const CDependencyProperty* const sourceProperty,
    _In_ CDependencyObject* const targetObject,
    _In_ const CDependencyProperty* const targetProperty)
{
    bool fRequiresTypeCheck = false;

    // Check that we have compatible types
    if (PropertyTypesCompatible(sourceProperty, targetProperty, &fRequiresTypeCheck))
    {
        // Get the list of subscriptions for the core dp
        PropertySubscriptions* subscriptions = EnsurePropertySubscriptions(sourceProperty);
        subscriptions->m_subscriptions.emplace_back(targetProperty, targetObject, fRequiresTypeCheck);
        return S_OK;
    }
    else
    {
        // We couldn't add the subscription because the types don't match
        // return S_FALSE so the caller will not try to remember this
        // core to core binding. FEs do remember their side of the TemplateBinding
        // and we don't want that to happen
        return S_FALSE;
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::OnPropertyChanged()
//
//  Synopsis:
//      Notifies the subscriptions for a given property of the new value
//      of the property
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CControl::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    PropertySubscriptions *pSubscriptions = NULL;

    pSubscriptions = LookupPropertySubscriptions(args.m_pDP);
    if (pSubscriptions != NULL)
    {
        IFC_RETURN(RefreshSubscriptions(
            pSubscriptions,
            *args.m_pNewValue,
            TemplateBindingsRefreshType::All));
    }

    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::TextBox_Description:
        case KnownPropertyIndex::AutoSuggestBox_Description:
        case KnownPropertyIndex::ComboBox_Description:
        case KnownPropertyIndex::PasswordBox_Description:
        case KnownPropertyIndex::RichEditBox_Description:
        case KnownPropertyIndex::CalendarDatePicker_Description:
        {
            if (!args.m_pNewValue->IsNullOrUnset())
            {
                EnsureDescription();
            }
            else
            {
                ClearDescription();
            }
            break;
        }
        case KnownPropertyIndex::TextBox_ValidationErrors:
        case KnownPropertyIndex::AutoSuggestBox_ValidationErrors:
        case KnownPropertyIndex::ComboBox_ValidationErrors:
        case KnownPropertyIndex::PasswordBox_ValidationErrors:
        {
            auto errors = checked_cast<CValidationErrorsCollection>(args.m_pNewValue->AsObject());
            errors->SetOwner(this);
            break;
        }
        case KnownPropertyIndex::TextBox_InputValidationMode:
        case KnownPropertyIndex::AutoSuggestBox_InputValidationMode:
        case KnownPropertyIndex::ComboBox_InputValidationMode:
        case KnownPropertyIndex::PasswordBox_InputValidationMode:
        {
            if (IsValidationEnabled() && HasErrors())
            {
                EnsureErrors();
            }
            else if (HasErrors())
            {
                DeferErrors();
            }
        }
        case KnownPropertyIndex::TextBox_InputValidationKind:
        case KnownPropertyIndex::AutoSuggestBox_InputValidationKind:
        case KnownPropertyIndex::ComboBox_InputValidationKind:
        case KnownPropertyIndex::PasswordBox_InputValidationKind:
        {
            EnsureValidationVisuals();
            break;
        }
        case KnownPropertyIndex::TextBox_ErrorTemplate:
        case KnownPropertyIndex::AutoSuggestBox_ErrorTemplate:
        case KnownPropertyIndex::ComboBox_ErrorTemplate:
        case KnownPropertyIndex::PasswordBox_ErrorTemplate:
        {
            if (IsValidationEnabled() && HasErrors())
            {
                EnsureErrors();
            }
            break;
        }
        case KnownPropertyIndex::TextBox_HasValidationErrors:
            RaiseHasValidationErrorsChanged(KnownEventIndex::TextBox_HasValidationErrorsChanged, args.m_pNewValue->As<valueBool>());
            break;
        case KnownPropertyIndex::AutoSuggestBox_HasValidationErrors:
            RaiseHasValidationErrorsChanged(KnownEventIndex::AutoSuggestBox_HasValidationErrorsChanged, args.m_pNewValue->As<valueBool>());
            break;
        case KnownPropertyIndex::ComboBox_HasValidationErrors:
            RaiseHasValidationErrorsChanged(KnownEventIndex::ComboBox_HasValidationErrorsChanged, args.m_pNewValue->As<valueBool>());
            break;
        case KnownPropertyIndex::PasswordBox_HasValidationErrors:
            RaiseHasValidationErrorsChanged(KnownEventIndex::PasswordBox_HasValidationErrorsChanged, args.m_pNewValue->As<valueBool>());
            break;
        default:
            break;
    }

    // If any inherited font properties were set, invalidate measure on any children
    // (that have a cached inherited property bag), so that they get re-measured.
    // This is necessary so that any TextBlock/TextBox elements in the control's visual
    // tree get measured during the layout pass that will follow.
    if (!ParserOwnsParent() &&
        args.m_pDP != nullptr &&
        (args.m_pDP->IsInherited()) &&
        (args.m_pDP->AffectsMeasure()))
    {
        IFC_RETURN(PropagateInheritedProperty(this, args.m_pDP));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::RaiseIsEnabledChangedEvent()
//
//  Synopsis:
//      Raise IsEnabledChanged event.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CControl::RaiseIsEnabledChangedEvent(_In_ CValue *pValue)
{
    HRESULT hr = S_OK;
    CEventManager *pEventManager = GetContext()->GetEventManager();
    CIsEnabledChangedEventArgs *pEventArgs = NULL;

    pEventArgs = new CIsEnabledChangedEventArgs();

    ASSERT(pValue->GetType() == valueBool);
    pEventArgs->m_fNewValue = pValue->AsBool();
    pEventArgs->m_fOldValue = !pEventArgs->m_fNewValue;
    pEventManager->Raise(EventHandle(KnownEventIndex::Control_IsEnabledChanged), TRUE, this, pEventArgs);

    ReleaseInterface(pEventArgs);
    RRETURN(hr);//RRETURN_REMOVAL
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::RaiseValidationErrorEvent()
//
//  Synopsis:
//      Raises Validation.Error attached event.
//
//------------------------------------------------------------------------
void CControl::RaiseValidationErrorEvent(
    _In_ const DirectUI::InputValidationErrorEventAction action,
    _In_ xaml_controls::IInputValidationError* error)
{
    bool hadErrors = true;
    const bool hasErrors = EnsureHasErrors(&hadErrors);

    if (action == DirectUI::InputValidationErrorEventAction::Added && IsValidationEnabled() && hasErrors && !hadErrors)
    {
        EnsureErrors();
    }
    else if (!hasErrors)
    {
        DeferErrors();
    }

    xref_ptr<CInputValidationErrorEventArgs> eventArgs;
    eventArgs.attach(new CInputValidationErrorEventArgs(action, error));

    GetContext()->GetEventManager()->Raise(EventHandle(GetTargetValidationErrorEvent()),TRUE, this, eventArgs.get());
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::RefreshSubscriptions()
//
//  Synopsis:
//      Refresh all of the subscriptions for the given property
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CControl::RefreshSubscriptions(
    _In_ PropertySubscriptions* const subscriptions,
    _In_ const CValue& value,
    _In_ TemplateBindingsRefreshType refreshType)
{
    // Flag that we're updating the bindings so the objects that are
    // being updated do not try to remove the bindings when their values
    // are being touched

    bool wasUpdatingBindings = m_fIsUpdatingBindings;

    auto onExit = wil::scope_exit([this, wasUpdatingBindings]()
    {
        m_fIsUpdatingBindings = wasUpdatingBindings;
    });

    m_fIsUpdatingBindings = TRUE;

    for (auto& targetSubscription : subscriptions->m_subscriptions)
    {
        HRESULT hr = S_OK;

        // Refresh if either refreshing all or if ones requiring initial update
        if (refreshType == TemplateBindingsRefreshType::All ||
            (refreshType == TemplateBindingsRefreshType::WithoutInitialUpdate && !targetSubscription.m_hadInitialUpdate))
        {
            hr = targetSubscription.m_targetObject->SetValue(targetSubscription.m_targetProperty, value);

            // If setting the value of the property failed it could be ok if a runtime check was necessary
            if (FAILED(hr))
            {
                // The value is not appropiate for the target property, we will use the
                // default value instead, we know that it could fail because a runtime check
                // was necessary
                if (targetSubscription.m_runtimeCheck)
                {
                    CValue defaultValue;
                    IFC_RETURN(targetSubscription.m_targetObject->GetDefaultValue(targetSubscription.m_targetProperty, &defaultValue));

                    // Set the property to the default value
                    IFC_RETURN(targetSubscription.m_targetObject->SetValue(targetSubscription.m_targetProperty, defaultValue));

                    defaultValue.SetNull();
                }
                else
                {
                    // The refresh failed but no runtime check was suposed to be done this means that
                    // this is an actual error
                    IFC_RETURN(hr);
                }
            }
        }

        targetSubscription.m_hadInitialUpdate = true;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::RefreshTemplateBindings()
//
//  Synopsis:
//      Refreshes all of the subscriptions with the current values
//      for the properties of the control
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CControl::RefreshTemplateBindings(
    _In_ TemplateBindingsRefreshType refreshType)
{
    auto onExit = wil::scope_exit([this]()
    {
        m_fRequestTemplateBindingRefresh = FALSE;
        TraceRefreshTemplateBindingsEnd();
    });

    TraceRefreshTemplateBindingsBegin();

    if (m_propertySubscriptions)
    {
        for (auto& propertySubscription : *m_propertySubscriptions)
        {
            CValue currentValue;

            if (!propertySubscription.m_sourceProperty->ShouldBindingGetValueUseCheckOnDemandProperty())
            {
                // Get the current value of the property
                IFC_RETURN(GetValue(propertySubscription.m_sourceProperty, &currentValue));
            }
            else
            {
                currentValue = CheckOnDemandProperty(propertySubscription.m_sourceProperty);
            }

            // Refresh the target core properties, the managed
            // properties are already refreshed, when the template binding
            // is created a getvalue is issued
            IFC_RETURN(RefreshSubscriptions(&propertySubscription, currentValue, refreshType));
        }
    }

    return S_OK;
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
_Check_return_
HRESULT
CControl::CreationComplete()
{
    // Call base implementation. This will apply any explicit styles.
    IFC_RETURN(CFrameworkElement::CreationComplete());

    // Only do this if it is a Control that has been subclassed in user code.
    // ASSERT(!m_fIsBuiltInStyleApplied);
    if (!m_fIsBuiltInStyleApplied && SupportsBuiltInStyles())
    {
        IFC_RETURN(ApplyBuiltInStyle());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ApplyBuiltInStyle
//
//  Synopsis:
//      Applies the builtin style, if it exists.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CControl::ApplyBuiltInStyle()
{
    HRESULT hr = S_OK;
    CStyle *pStyle = NULL;
    CStyle *pOldStyle = NULL;

    ASSERT(!m_fIsBuiltInStyleApplied);

    // Get Style
    IFC(GetBuiltInStyle(&pStyle));
    if (!pStyle)
    {
        goto Cleanup;
    }

    // Set style
    pOldStyle = m_pBuiltInStyle;
    m_pBuiltInStyle = pStyle;
    pStyle = NULL;

    m_fIsDefaultStyleApplying = TRUE;

    // Apply style
    IFC(OnStyleChanged(pOldStyle, m_pBuiltInStyle, BaseValueSourceBuiltInStyle));

Cleanup:
    ReleaseInterface(pStyle);
    ReleaseInterface(pOldStyle);

    m_fIsBuiltInStyleApplied = TRUE;
    m_fIsDefaultStyleApplying = FALSE;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   GetBuiltInStyle
//
//  Synopsis:
//      Retrieves the builtin style for this control
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CControl::GetBuiltInStyle(_Outptr_ CStyle** ppStyle)
{
    HRESULT hr = S_OK;
    CStyle* pStyle = NULL;

    IFCPTR(ppStyle);

    TraceGetBuiltInStyleBegin();

    // If the CLR is initialized, then get the builtin style from the managed side.
    // else, retrieve the native builtin style.

    // If there's no managed peer, create one. This is necessary for retrieval of
    // built-in styles
    IFC(EnsurePeer());

    IFC(FxCallbacks::Control_GetBuiltInStyle(this, &pStyle));

    if (EventEnabledGetBuiltInStyleEnd())
    {
        if (pStyle)
        {
            xstring_ptr strStyle;
            IFC(pStyle->GetTargetTypeName(&strStyle));
            TraceGetBuiltInStyleEnd(strStyle.GetBuffer());
        }
        else
        {
            TraceGetBuiltInStyleEnd(L"None");
        }
    }

    *ppStyle = pStyle;
    pStyle = NULL;

Cleanup:

    ReleaseInterface(pStyle);

    RRETURN(hr);
}

// Get property value from built-in style.
_Check_return_ HRESULT CControl::GetValueFromBuiltInStyle(
    _In_ const CDependencyProperty* dp,
    _Out_ CValue* pValue,
    _Out_ bool* gotValue)
{
    *gotValue = false;

    if (m_pBuiltInStyle)
    {
        IFC_RETURN(m_pBuiltInStyle->GetPropertyValue(
            dp->GetIndex(),
            pValue,
            gotValue));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:   This is overridden so that we can return an error if the
//              parser adds more than one child to the UserControl.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CUserControl::AddChild(_In_ CUIElement * pChild)
{
    HRESULT hr = S_OK;
    CUIElement* pExistingContent = NULL;

    IFCEXPECT(pChild != NULL);

    // Can only have one child!
    IFC(GetContent(&pExistingContent));
    IFCEXPECT(pExistingContent == NULL);

    IFC(CControl::AddChild(pChild));

Cleanup:
    ReleaseInterface(pExistingContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CUserControl::Content()
//
//  Synopsis:   This is the content property getter and setter method. Note
//              that the storage for this property is actually this children
//              collection.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CUserControl::Content(
                      _In_ CDependencyObject *pObject,
                      _In_ XUINT32 cArgs,
                      _Inout_updates_(cArgs) CValue *ppArgs,
                      _In_opt_ IInspectable* pValueOuter,
                      _Out_ CValue *pResult)
{
    HRESULT hr = S_OK;
    CUserControl* pUserControl = NULL;

    // Validate parameters
    if (!pObject || !pObject->OfTypeByIndex<KnownTypeIndex::Control>())
    {
        IFC(E_INVALIDARG);
    }

    pUserControl = (CUserControl*) pObject;

    if (cArgs == 0)
    {
        // Getting the content
        CUIElement* pContent = NULL;
        hr = pUserControl->GetContent(&pContent);
        if (SUCCEEDED(hr))
        {
            pResult->SetObjectNoRef(pContent);
        }
        else
        {
            pResult->SetNull();
        }
    }
    else if (cArgs == 1 && ppArgs->GetType() == valueObject && ppArgs->AsObject()->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        // Setting the content
        IFC(pUserControl->SetContent((CUIElement*)ppArgs->AsObject()));
    }
    else if (cArgs == 1 && ppArgs->GetType() == valueNull)
    {
        IFC(pUserControl->SetContent(NULL));
    }
    else
    {
        IFC(E_INVALIDARG);
    }
Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:   Remove any existing content and set the new child tree.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CUserControl::SetContent(_In_ CUIElement* pContent)
{
    HRESULT hr = S_OK;

    CUIElement* pExistingContent = NULL;
    IFC(GetContent(&pExistingContent));

    if (pExistingContent)
    {
        IFC(RemoveChild(pExistingContent));
    }

    if (pContent)
    {
        IFC(AddChild(pContent));
    }

Cleanup:
    ReleaseInterface(pExistingContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CUserControl::GetContent()
//
//  Synopsis:   This will return the first (and only) child, or NULL if
//              the there is no Content yet.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CUserControl::GetContent(_Outptr_ CUIElement** ppContent)
{
    IFCPTR_RETURN(ppContent);

    // Get First Child will not return a Addref'ed interface
    // so we need to add ref it.
    *ppContent = (CUIElement*) GetFirstChildNoAddRef();
    AddRefInterface(*ppContent);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: SetValue
//
//  Synopsis:
//      Override to disallow setting the template property on a UserControl.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CUserControl::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP != nullptr)
    {
        switch (args.m_pDP->GetIndex())
        {
            case KnownPropertyIndex::Control_Template:
            {
                // Do not allow template properties to be set on UserControls.
                IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, ManagedError, AG_E_USER_CONTROL_OP_UNSUPPORTED));
                break;
            }
        }
    }

    IFC_RETURN(CControl::SetValue(args));

    return S_OK;
}

_Check_return_ HRESULT
CUserControl::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    IFC_RETURN(CControl::EnterImpl(pNamescopeOwner, params));

    // If this object has a managed peer, it's possible this is a Page object with appbars. Make sure
    // the Page peer has an opportunity to register the appbars now.
    if (params.fIsLive && HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::UserControl_RegisterAppBars(this));
    }

    return S_OK;
}

_Check_return_ HRESULT
CUserControl::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CControl::LeaveImpl(pNamescopeOwner, params));

    // If this object has a managed peer, it's possible this is a Page object with appbars. Make sure
    // the Page peer has an opportunity to unregister the appbars now.
    if (params.fIsLive && HasManagedPeer())
    {
        IFC_RETURN(FxCallbacks::UserControl_UnregisterAppBars(this));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CControl::ClearPropertySubscriptions()
//
//  Synopsis:
//      Notifies all of the CFrameworkElement instances that have
//      subcriptions that they should remove those
//
//------------------------------------------------------------------------

void CControl::ClearPropertySubscriptions()
{
    if (m_propertySubscriptions)
    {
        for (auto& propertySubscription : *m_propertySubscriptions)
        {
            for (auto& targetSubscription : propertySubscription.m_subscriptions)
            {
                checked_cast<CFrameworkElement>(targetSubscription.m_targetObject)->ClearTemplateBindingData();
            }
        }

        m_propertySubscriptions.reset();
    }
}


//-------------------------------------------------------------------------
//
//  Function:   CControl::PropertyTypesCompatible()
//
//  Synopsis:
//      Checks the source and target type of a template binding, determines if
//      those types are compatible and if they will require a runtime check
//
//------------------------------------------------------------------------

bool CControl::PropertyTypesCompatible(
    _In_ const CDependencyProperty* const sourceProperty,
    _In_ const CDependencyProperty* const targetProperty,
    _Out_ bool* pfRequiresTypeCheck)
{
    auto sourceIndex = sourceProperty->GetPropertyType()->m_nIndex;
    auto targetIndex = targetProperty->GetPropertyType()->m_nIndex;

    // If the target type is a base class or the same class as the source type then
    // the types are fully compatible and will not require a type check
    if (sourceIndex == targetIndex ||
        DirectUI::MetadataAPI::IsAssignableFrom(targetIndex, sourceIndex))
    {
        *pfRequiresTypeCheck = false;
        return true;
    }

    // If the source type is a base class of the target type then
    // the types are compatible, but will eventually require a type check
    if (DirectUI::MetadataAPI::IsAssignableFrom(sourceIndex, targetIndex))
    {
        *pfRequiresTypeCheck = true;
        return true;
    }

    // The types are not related, such as string and double, and therefore are not compatible
    return false;
}

//------------------------------------------------------------------------
//
//  Method: Enabled
//
//  Synopsis:
//      Property method for IsEnabled property.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CControl::Enabled(
                  _In_ CDependencyObject *pObject,
                  _In_ XUINT32 cArgs,
                  _Inout_updates_(cArgs) CValue *ppArgs,
                  _In_opt_ IInspectable* pValueOuter,
                  _Out_ CValue *pResult
                  )
{
    CControl *pControl = NULL;
    CUIElement *pParent = NULL;
    bool fIsEnabled = false;
    bool fParentIsEnabled = false;
    CCoreServices *pCore = NULL;
    CFocusManager *pFocusManager = NULL;
    CControl *pFocusedControl = NULL;

    IFC_RETURN(DoPointerCast(pControl, pObject));
    IFCPTR_RETURN(pCore = pControl->GetContext());

    pFocusManager = VisualTree::GetFocusManagerForElement(pControl, LookupOptions::NoFallback);
    IFCPTR_RETURN(pFocusManager);

    if (cArgs == 1 && ppArgs)
    {
        ASSERT(ppArgs->GetType() == valueBool || ppArgs->GetType() == valueObject);

        // We are setting the value...
        if (ppArgs->GetType() == valueBool)
        {
            fIsEnabled = (ppArgs->AsBool() != 0);
        }
        else if (ppArgs->GetType() == valueObject)
        {
            if (ppArgs->AsObject()->GetClassInformation()->IsEnum())
            {
                CEnumerated *pDO = checked_cast<CEnumerated>(ppArgs->AsObject());
                fIsEnabled = (pDO->m_nValue != 0) ? TRUE : FALSE;
            }
            else
            {
                IFC_RETURN(E_INVALIDARG);
            }
        }
        else
        {
            IFC_RETURN(E_INVALIDARG);
        }

        pControl->SetIsEnabled(fIsEnabled);

        if (pControl->ParserOwnsParent())
        {
            // If value is being set while parsing, coersion will be performed later during enter walk.
            // Just update coerced value to local value now...
            pControl->SetCoercedIsEnabled(fIsEnabled);
        }
        else if (fIsEnabled != pControl->IsEnabled()) // If value changed...
        {
            if (!fIsEnabled)
            {
                IFC_RETURN(pControl->CoerceIsEnabled(FALSE, /*bCoerceChildren*/TRUE));

            }
            else
            {
                // Can coerce to true only if visual parent is not disabled.
                pParent = pControl->GetUIElementParentInternal();
                if (pParent)
                {
                    fParentIsEnabled = pParent->IsEnabled();
                }

                // only coerce to true if we are not suppressing.
                if  ((!pParent || fParentIsEnabled) && !pControl->m_fSuppressIsEnabled )
                {
                    IFC_RETURN(pControl->CoerceIsEnabled(TRUE, /*bCoerceChildren*/TRUE));
                }

                // Set focus if this control is the first focusable control in case of
                // no having focus yet
                if (pControl->IsFocusable())
                {
                    if (pFocusManager->GetFocusedElementNoRef() == NULL)
                    {
                        // No focused control here, so try to check the control is
                        // the first focusable control
                        CDependencyObject *pFocusable = pFocusManager->GetFirstFocusableElement();
                        if (pControl == pFocusable)
                        {
                            bool focusUpdated = false;

                            if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_47229546>())
                            {
                                // In 1.4 servicing we started specifying NoActivate here.  In this function, we're setting focus just to avoid the state where no
                                // focus is set, not because the user is interacting with it.  We don't want to activate the window for this kind of situation.
                                IFCFAILFAST(pControl->Focus(
                                    DirectUI::FocusState::Programmatic,
                                    false /*animateIfBringIntoView*/,
                                    &focusUpdated,
                                    DirectUI::FocusNavigationDirection::None, 
                                    InputActivationBehavior::NoActivate));
                            }
                            else
                            {
                                // If we are trying to set focus in a changing focus event handler, we will end up leaving focus on the disabled control.
                                // As a result, we fail fast here. This is being tracked by Bug 9840123
                                IFCFAILFAST(pControl->Focus(DirectUI::FocusState::Programmatic, false /*animateIfBringIntoView*/, &focusUpdated));
                            }
                            
                        }
                    }
                }
            }
        }

        const bool shouldReevaluateFocus = !fIsEnabled && !pControl->ParserOwnsParent()
                                            && !pControl->AllowFocusWhenDisabled()
                                            // We just disabled this control, find if this control
                                            //or one of its children had focus.
                                            && FocusProperties::HasFocusedElement<CDependencyObject>(pControl);

        if (shouldReevaluateFocus)
        {
            pFocusedControl = do_pointer_cast<CControl>(pFocusManager->GetFocusedElementNoRef());

            // Set the focus on the next focusable control.
            IFC_RETURN(pFocusManager->SetFocusOnNextFocusableElement(pFocusedControl ? pFocusedControl->GetFocusState() : DirectUI::FocusState::Programmatic, true));
        }
    }
    else if (pResult)
    {
        // We are getting the value
        pResult->SetBool(!!pControl->GetCoercedIsEnabled());
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }




    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: SuppressIsEnabled
//
//  Synopsis:
//      Called when there is a reason to coerce IsEnabled to false.
//      Used by controls that carry a Command with corresponding CanExecute value
//
//------------------------------------------------------------------------
void CControl::SuppressIsEnabled(_In_ bool fSuppress)
{
    if (m_fSuppressIsEnabled != fSuppress)
    {
        m_fSuppressIsEnabled = fSuppress;
        IGNOREHR(CoerceIsEnabled(GetIsEnabled() && !fSuppress, TRUE));
    }
}

//------------------------------------------------------------------------
//
//  Method: CoerceIsEnabled
//
//  Synopsis:
//      Coerces an element (and potentially) its children into a
//      state of either enabled or disabled.
//------------------------------------------------------------------------

_Check_return_
HRESULT
CControl::CoerceIsEnabled(_In_ bool fIsEnabled, _In_ bool fCoerceChildren)
{
    if (fIsEnabled && m_fSuppressIsEnabled)
    {
        return S_OK;
    }

    IFC_RETURN(CUIElement::CoerceIsEnabled(fIsEnabled, fCoerceChildren));

    return S_OK;
}

bool CControl::LastInputGamepad()
{
    CContentRoot* contentRoot = VisualTree::GetContentRootForElement(this);
    return contentRoot->GetInputManager().GetLastInputDeviceType() == DirectUI::InputDeviceType::GamepadOrRemote;
}

_Check_return_ HRESULT CControl::OnApplyTemplate()
{
    IFC_RETURN(__super::OnApplyTemplate());
    if (GetTargetDescriptionProperty() != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        EnsureDescription();
    }

    if (GetTargetValidationCommandProperty() != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        EnsureValidationVisuals();
    }
    return S_OK;
}

void CControl::EnsureDescription()
{
    if (auto descriptionPresenter = do_pointer_cast<CContentPresenter>(GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"DescriptionPresenter"))))
    {
        CValue visibleValue;
        visibleValue.SetEnum(static_cast<XUINT32>(DirectUI::Visibility::Visible));
        IFCFAILFAST(descriptionPresenter->SetValueByIndex(KnownPropertyIndex::UIElement_Visibility, visibleValue));
    }
}

void CControl::ClearDescription()
{
    if (auto descriptionPresenter = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"DescriptionPresenter")))
    {
        // Resetting, try to defer the error list if we don't have errors. TryDefer returns E_INVALIDARG if the element
        // doesn't have a realizing proxy, which could be expected if someone overrides the template
        // and changes it to not use x:Load or x:DeferLoadStrategy
        IFCFAILFAST_ALLOW_INVALIDARG(CDeferredElement::TryDefer(descriptionPresenter.get()));
    }
}

KnownPropertyIndex CControl::GetTargetDescriptionProperty() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownPropertyIndex::AutoSuggestBox_Description;
    case KnownTypeIndex::TextBox:
        return KnownPropertyIndex::TextBox_Description;
    case KnownTypeIndex::PasswordBox:
        return KnownPropertyIndex::PasswordBox_Description;
    case KnownTypeIndex::ComboBox:
        return KnownPropertyIndex::ComboBox_Description;
    case KnownTypeIndex::CalendarDatePicker:
        return KnownPropertyIndex::CalendarDatePicker_Description;
    case KnownTypeIndex::RichEditBox:
        return KnownPropertyIndex::RichEditBox_Description;
    default:
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }
}

bool CControl::EnsureHasErrors(bool* hadErrors)
{
    CValue hasErrorsValue;
    IFCFAILFAST(GetValueByIndex(GetTargetHasErrorsProperty(), &hasErrorsValue));
    const bool hasErrors = hasErrorsValue.As<valueBool>();

    if (hadErrors) *hadErrors = hasErrors;

    bool hasErrorsInCollection = false;
    if (auto errors = do_pointer_cast<CValidationErrorsCollection>(CheckOnDemandProperty(GetTargetErrorsProperty())))
    {
        hasErrorsInCollection = !errors->empty();
    }

    // The two don't match, the two possible scenarios are:
    //  a) just got an error, in which case hasErrors is false and hasErrorsInCollection is true
    //  b) just cleared last error, in which hasErrors is true and hasErrorsInCollection is false
    // Either way, the existence of items in the collection takes precedence.
    if (hasErrorsInCollection != hasErrors)
    {
        CValue hasErrorsResult;
        hasErrorsResult.Set<valueBool>(hasErrorsInCollection);
        IFCFAILFAST(SetValueByIndex(GetTargetHasErrorsProperty(), hasErrorsResult));
    }

    return hasErrorsInCollection;
}

bool CControl::HasErrors()
{
    return EnsureHasErrors();
}

void CControl::EnsureErrors()
{
    // When an error occurs, we want to undefer the error presenter and load the error template. We want to hook up
    // the appropriate data context so that controls can use {Binding} in their ErrorTemplates
    if (auto errorPresenter = do_pointer_cast<CContentPresenter>(GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"ErrorPresenter"))))
    {
        EnsureValidationVisuals();
        if (auto errorTemplate = GetValidationErrorTemplate())
        {
            xref_ptr<CDependencyObject> loadedContent;
            IFCFAILFAST(errorTemplate->LoadContent(loadedContent.ReleaseAndGetAddressOf()));
            if (loadedContent)
            {
                CValue dataContextValue;
                dataContextValue.WrapObjectNoRef(this);
                IFCFAILFAST(loadedContent->SetValueByIndex(KnownPropertyIndex::FrameworkElement_DataContext, dataContextValue));

                CValue contentValue;
                contentValue.WrapObjectNoRef(loadedContent.get());

                if (!ShowErrorsInline())
                {
                    // We aren't showing errors inline, get the default compact template and set the errors as the content of the
                    // tooltip. We then use the tree created from the compact template as the content for the presenter
                    xref_ptr<CDependencyObject> dataTemplate;
                    IFCFAILFAST(GetContext()->LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"DefaultCompactErrorIconTemplate"), dataTemplate.ReleaseAndGetAddressOf()));
                    if (!dataTemplate)
                    {
                        // Temporary workaround for build demo and so we don't have a dependence on the resource being present in the resources dll.
                        // Removal tracked by Bug 21221714: remove workaround for inputvalidation demo in control.cpp
                        CDependencyObject* dataTemplateNoRef = nullptr;
                        IFCFAILFAST(GetContext()->GetApplicationResourceDictionary()->GetKeyNoRef(XSTRING_PTR_EPHEMERAL(L"DefaultCompactErrorIconTemplate"), &dataTemplateNoRef));
                        dataTemplate = dataTemplateNoRef;
                    }

                    if (auto compactTemplate = do_pointer_cast<CDataTemplate>(dataTemplate))
                    {
                        xref_ptr<CDependencyObject> iconContent;
                        IFCFAILFAST(compactTemplate->LoadContent(iconContent.ReleaseAndGetAddressOf()));

                        CValue toolTipValue;
                        IFCFAILFAST(iconContent->GetValueByIndex(KnownPropertyIndex::ToolTipService_ToolTip, &toolTipValue));
                        if (auto toolTip = toolTipValue.As<valueObject>())
                        {
                            IFCFAILFAST(toolTip->SetValueByIndex(KnownPropertyIndex::ContentControl_Content, contentValue));
                        }

                        contentValue.SetObjectAddRef(iconContent.get());
                    }
                }

                IFCFAILFAST(errorPresenter->SetValueByIndex(KnownPropertyIndex::ContentPresenter_Content, contentValue));
            }
        }
    }
}

xref_ptr<CInputValidationCommand> CControl::TryGetInputValidationCommand() const
{
    xref_ptr<CInputValidationCommand> returnValue;
    auto commandProperty = GetTargetValidationCommandProperty();
    if (commandProperty != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        CValue validationCommandValue;
        IFCFAILFAST(GetValueByIndex(commandProperty, &validationCommandValue));
        returnValue = checked_sp_cast<CInputValidationCommand>(validationCommandValue.DetachObject());
    }
    return returnValue;
}

xref_ptr<CDataTemplate> CControl::GetValidationErrorTemplate() const
{
    CValue errorTemplateValue;
    IFCFAILFAST(GetValueByIndex(GetTargetErrorTemplateProperty(), &errorTemplateValue));
    return checked_sp_cast<CDataTemplate>(errorTemplateValue.DetachObject());
}

DirectUI::InputValidationMode CControl::GetErrorVisualMode() const
{
    DirectUI::InputValidationMode returnValue = DirectUI::InputValidationMode::Default;
    if (auto validationCommand = TryGetInputValidationCommand())
    {
        returnValue = validationCommand->m_inputValidationMode;
    }

    // If we don't have a validation command, or it's InputValidationMode is Default, then use what we have set
    if (returnValue == DirectUI::InputValidationMode::Default)
    {
        auto targetValidationModeProperty = GetTargetValidationModeProperty();

        if (targetValidationModeProperty != KnownPropertyIndex::UnknownType_UnknownProperty)
        {
            CValue visualModeValue;
            IFCFAILFAST(GetValueByIndex(GetTargetValidationModeProperty(), &visualModeValue));
            returnValue = static_cast<DirectUI::InputValidationMode>(visualModeValue.AsEnum());
        }
        else
        {
            returnValue = DirectUI::InputValidationMode::Disabled;
        }
    }
    return returnValue;
}

DirectUI::InputValidationKind CControl::GetErrorVisualKind() const
{
    DirectUI::InputValidationKind returnValue = DirectUI::InputValidationKind::Auto;
    if (auto validationCommand = TryGetInputValidationCommand())
    {
        returnValue = validationCommand->m_inputValidationKind;
    }

    // If we don't have a validation command, or it's InputValidationKind is Auto, then use what we have set
    if (returnValue == DirectUI::InputValidationKind::Auto)
    {
        CValue visualKindValue;
        IFCFAILFAST(GetValueByIndex(GetTargetValidationKindProperty(), &visualKindValue));
        returnValue = static_cast<DirectUI::InputValidationKind>(visualKindValue.AsEnum());
    }
    return returnValue;
}

bool CControl::ShowErrorsInline() const
{
    return GetErrorVisualKind() == DirectUI::InputValidationKind::Inline;
}

bool CControl::IsValidationEnabled() const
{
    return GetErrorVisualMode() != DirectUI::InputValidationMode::Disabled;
}

void CControl::DeferErrors()
{
    if (auto errorPresenter = GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"ErrorPresenter")))
    {
        // Resetting, try to defer the error list. TryDefer returns E_INVALIDARG if the element
        // doesn't have a realizing proxy, which could be expected if someone overrides the template
        // and changes it to not use x:Load or x:DeferLoadStrategy
        IFCFAILFAST_ALLOW_INVALIDARG(CDeferredElement::TryDefer(errorPresenter.get()));

        // EnsureValidationVisuals will reset settings so we don't affect layout when there are no error visuals present
        EnsureValidationVisuals();
    }
}

void CControl::EnsureValidationVisuals()
{
    const bool isValidationEnabled = IsValidationEnabled();
    if (isValidationEnabled && ShowErrorsInline())
    {
        IGNOREHR(CVisualStateManager::GoToState(this, L"InlineValidationEnabled", false));
        if (HasErrors())
        {
           IGNOREHR(CVisualStateManager::GoToState(this, L"InlineErrors", false));
        }
        else
        {
            IGNOREHR(CVisualStateManager::GoToState(this, L"ErrorsCleared", false));
        }
    }
    else if (isValidationEnabled)
    {
        IGNOREHR(CVisualStateManager::GoToState(this, L"CompactValidationEnabled", false));
        if (HasErrors())
        {
           IGNOREHR(CVisualStateManager::GoToState(this, L"CompactErrors", false));
        }
        else
        {
            IGNOREHR(CVisualStateManager::GoToState(this, L"ErrorsCleared", false));
        }
    }
    else
    {
        IGNOREHR(CVisualStateManager::GoToState(this, L"ValidationDisabled", false));
    }
}

KnownPropertyIndex CControl::GetTargetHasErrorsProperty() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownPropertyIndex::AutoSuggestBox_HasValidationErrors;
    case KnownTypeIndex::TextBox:
        return KnownPropertyIndex::TextBox_HasValidationErrors;
    case KnownTypeIndex::PasswordBox:
        return KnownPropertyIndex::PasswordBox_HasValidationErrors;
    case KnownTypeIndex::ComboBox:
        return KnownPropertyIndex::ComboBox_HasValidationErrors;
    default:
        XAML_FAIL_FAST();
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }
}

KnownPropertyIndex CControl::GetTargetErrorsProperty() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownPropertyIndex::AutoSuggestBox_ValidationErrors;
    case KnownTypeIndex::TextBox:
        return KnownPropertyIndex::TextBox_ValidationErrors;
    case KnownTypeIndex::PasswordBox:
        return KnownPropertyIndex::PasswordBox_ValidationErrors;
    case KnownTypeIndex::ComboBox:
        return KnownPropertyIndex::ComboBox_ValidationErrors;
    default:
        // We should only every get this property when trying to display errors,
        // so we should know what we are doing.
        XAML_FAIL_FAST();
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }
}

KnownPropertyIndex CControl::GetTargetValidationCommandProperty() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownPropertyIndex::AutoSuggestBox_ValidationCommand;
    case KnownTypeIndex::TextBox:
        return KnownPropertyIndex::TextBox_ValidationCommand;
    case KnownTypeIndex::PasswordBox:
        return KnownPropertyIndex::PasswordBox_ValidationCommand;
    case KnownTypeIndex::ComboBox:
        return KnownPropertyIndex::ComboBox_ValidationCommand;
    default:
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }
}

KnownPropertyIndex CControl::GetTargetErrorTemplateProperty() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownPropertyIndex::AutoSuggestBox_ErrorTemplate;
    case KnownTypeIndex::TextBox:
        return KnownPropertyIndex::TextBox_ErrorTemplate;
    case KnownTypeIndex::PasswordBox:
        return KnownPropertyIndex::PasswordBox_ErrorTemplate;
    case KnownTypeIndex::ComboBox:
        return KnownPropertyIndex::ComboBox_ErrorTemplate;
    default:
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }
}

KnownPropertyIndex CControl::GetTargetValidationModeProperty() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownPropertyIndex::AutoSuggestBox_InputValidationMode;
    case KnownTypeIndex::TextBox:
        return KnownPropertyIndex::TextBox_InputValidationMode;
    case KnownTypeIndex::PasswordBox:
        return KnownPropertyIndex::PasswordBox_InputValidationMode;
    case KnownTypeIndex::ComboBox:
        return KnownPropertyIndex::ComboBox_InputValidationMode;
    default:
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }
}

KnownPropertyIndex CControl::GetTargetValidationKindProperty() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownPropertyIndex::AutoSuggestBox_InputValidationKind;
    case KnownTypeIndex::TextBox:
        return KnownPropertyIndex::TextBox_InputValidationKind;
    case KnownTypeIndex::PasswordBox:
        return KnownPropertyIndex::PasswordBox_InputValidationKind;
    case KnownTypeIndex::ComboBox:
        return KnownPropertyIndex::ComboBox_InputValidationKind;
    default:
        return KnownPropertyIndex::UnknownType_UnknownProperty;
    }
}

KnownEventIndex CControl::GetTargetValidationErrorEvent() const
{
    switch(GetTypeIndex())
    {
    case KnownTypeIndex::AutoSuggestBox:
        return KnownEventIndex::AutoSuggestBox_ValidationError;
    case KnownTypeIndex::TextBox:
        return KnownEventIndex::TextBox_ValidationError;
    case KnownTypeIndex::PasswordBox:
        return KnownEventIndex::PasswordBox_ValidationError;
    case KnownTypeIndex::ComboBox:
        return KnownEventIndex::ComboBox_ValidationError;
    default:
        return KnownEventIndex::UnknownType_UnknownEvent;
    }
}

void CControl::RaiseHasValidationErrorsChanged(KnownEventIndex errorsChangedIndex, bool newValue)
{
    xref_ptr<CHasValidationErrorsChangedEventArgs> eventArgs;
    eventArgs.attach(new CHasValidationErrorsChangedEventArgs(newValue));
    GetContext()->GetEventManager()->Raise(EventHandle(errorsChangedIndex), TRUE, this, eventArgs.get());
}