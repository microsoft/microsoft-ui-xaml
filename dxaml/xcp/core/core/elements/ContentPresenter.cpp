// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MetadataAPI.h"
#include "DeferredMapping.h"

using namespace DirectUI;

CContentPresenter::~CContentPresenter()
{
    if (m_pChildFromDefaultTemplate)
    {
        IGNOREHR(RemovePeerReferenceToItem(m_pChildFromDefaultTemplate));
    }
    ReleaseInterface(m_pContentTransitions);
    if (m_cachedContent.GetType() == valueObject)
    {
        m_cachedContent.AsObject()->UnpegManagedPeer();
    }
    ReleaseInterface(m_pContentTemplate);
    VERIFYHR(GetContext()->RemoveNameScope(this, Jupiter::NameScoping::NameScopeType::TemplateNameScope));
}

CContentPresenter::CContentPresenter(_In_ CCoreServices *pCore)
    : CFrameworkElement(pCore)
    , m_bInOnApplyTemplate(false)
    , m_bDataContextInvalid(true)
    , m_fNWBackgroundDirty(false)
    , m_fNWBorderBrushDirty(false)
    , m_contentPresenterUsingDefaultTemplate(false)
{
    m_content.SetNull();
    m_cachedContent.SetNull();
}

//------------------------------------------------------------------------
//
//  Method: SetValue
//
//  Synopsis:
//      Detect changes in key values.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CContentPresenter::SetValue(_In_ const SetValueParams& args)
{
    if (args.m_pDP->GetIndex() == KnownPropertyIndex::ContentPresenter_ContentTransitions)
    {
        // when this elements receives a transition meant for its child,
        // proactively ensure storage on that child so it starts to
        // take snapshots.
        if (args.m_value.AsObject() || args.m_value.AsIInspectable())
        {
            IFC_RETURN(CUIElement::EnsureLayoutTransitionStorage(m_content.AsObject(), NULL, TRUE));
        }
        // lifetime of transitioncollection is manually handled since it has no parent
        IFC_RETURN(SetPeerReferenceToProperty(args.m_pDP, args.m_value));
    }

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::ContentPresenter_FontFamily && args.m_value.IsNull() &&
       (GetContext()->IsSettingValueFromManaged(this) || ParserOwnsParent()))
    {
        HRESULT hrToOriginate = E_NER_ARGUMENT_EXCEPTION;
        xephemeral_string_ptr parameters[2];

        ASSERT(parameters[0].IsNull());
        args.m_pDP->GetName().Demote(&parameters[1]);

        IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_PROPERTY_INVALID, 2, parameters));
        IFC_RETURN(hrToOriginate);
    }

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ContentPresenter_Content:
        {
            CContentControl* pContentControlOwner = NULL;

            // ContentPresenter is unusual in that it sets its DataContext (used for data binding) to be its Content.  If the Content
            // is itself bound (uncommon but legal) this leads to an infinite recursion.  We break the recursion here with !m_bInOnApplyTemplate.

            // CDependencyObject::SetValue will fail if we attempt to set the content property to itself.
            if (!m_bInOnApplyTemplate && !(args.m_value.IsNull() && m_content.IsNull()) &&
                (args.m_value != m_content))
            {
                // Let the base do most of the work.
                IFC_RETURN(CFrameworkElement::SetValue(args));
            }
            else if (HasDeferred())
            {
                // We rely on base class being called to clear deferred element proxies.  If the above if condition evaluates to false, it will not happen.
                // In such case notify the value has been set here.
                IFC_RETURN(CDeferredMapping::NotifyParentValueSet(this, args.m_pDP->GetIndex()));
            }

            // register ourselves with our contentcontrol so that the contentcontrol knows how to get to
            // its 'controltemplateroot': the first visual child of us.
            pContentControlOwner = do_pointer_cast<CContentControl>(GetTemplatedParent());
            if (pContentControlOwner)
            {
                pContentControlOwner->ConsiderContentPresenterForContentTemplateRoot(this, m_content);
            }
        }
        break;
    case KnownPropertyIndex::ContentPresenter_ContentTemplate:
        {
            if (args.m_value.GetType() != valueObject || args.m_value.AsObject() != m_pContentTemplate)
            {
                xref_ptr<CDataTemplate> oldContentTemplate;

                auto cleanupGuard = wil::scope_exit([&]
                {
                    if (oldContentTemplate)
                    {
                        oldContentTemplate->UnpegManagedPeer();
                    }
                });

                if (m_pContentTemplate)
                {
                    // Momentarily pegging the DataTemplate's DXaml peer so it does not get discarded before the CContentPresenter::OnContentTemplateChanged notification completes.
                    IFC_RETURN(m_pContentTemplate->PegManagedPeer());
                    oldContentTemplate = m_pContentTemplate;
                }

                // Let the base do most of the work.
                IFC_RETURN(CFrameworkElement::SetValue(args));
                IFC_RETURN(Invalidate(TRUE));
            }
        }
        break;
    case KnownPropertyIndex::ContentPresenter_SelectedContentTemplate:
        {
            if (args.m_value.GetType() != valueObject || args.m_value.AsObject() != GetSelectedContentTemplate())
            {
                // Let the base do most of the work.
                IFC_RETURN(CFrameworkElement::SetValue(args));
                if (!m_pContentTemplate)
                {
                    IFC_RETURN(Invalidate(TRUE));
                }
            }
        }
        break;

    case KnownPropertyIndex::ContentPresenter_Background:
    {
        bool isAnimationEnabled = true;
        IGNOREHR(FxCallbacks::FrameworkCallbacks_IsAnimationEnabled(&isAnimationEnabled));

        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();

        if (dcompTreeHost != nullptr && isAnimationEnabled)
        {
            CBrushTransition* const backgroundTransition = CUIElement::GetBrushTransitionNoRef(*this, KnownPropertyIndex::ContentPresenter_BackgroundTransition);

            if (backgroundTransition != nullptr)
            {
                dcompTreeHost->GetWUCBrushManager()->SetUpBrushTransitionIfAllowed(
                    GetBackgroundBrush().get() /* from */,
                    do_pointer_cast<CSolidColorBrush>(args.m_value) /* to */,
                    *this,
                    ElementBrushProperty::Fill,
                    *backgroundTransition);
            }
            else
            {
                dcompTreeHost->GetWUCBrushManager()->CleanUpBrushTransition(*this, ElementBrushProperty::Fill);
            }
        }

        IFC_RETURN(CFrameworkElement::SetValue(args));
        break;
    }

    default:
        {
            IFC_RETURN(CFrameworkElement::SetValue(args));
        }
        break;
    }

    return S_OK;
}

_Check_return_ HRESULT CContentPresenter::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    KnownPropertyIndex textBlockPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;

    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ContentPresenter_OpticalMarginAlignment:
        textBlockPropertyIndex = KnownPropertyIndex::TextBlock_OpticalMarginAlignment;
        break;
    case KnownPropertyIndex::ContentPresenter_TextLineBounds:
        textBlockPropertyIndex = KnownPropertyIndex::TextBlock_TextLineBounds;
        break;
    case KnownPropertyIndex::ContentPresenter_TextWrapping:
        textBlockPropertyIndex = KnownPropertyIndex::TextBlock_TextWrapping;
        break;
    case KnownPropertyIndex::ContentPresenter_LineStackingStrategy:
        textBlockPropertyIndex = KnownPropertyIndex::TextBlock_LineStackingStrategy;
        break;
    case KnownPropertyIndex::ContentPresenter_MaxLines:
        textBlockPropertyIndex = KnownPropertyIndex::TextBlock_MaxLines;
        break;
    case KnownPropertyIndex::ContentPresenter_LineHeight:
        textBlockPropertyIndex = KnownPropertyIndex::TextBlock_LineHeight;
        break;
    case KnownPropertyIndex::ContentPresenter_ContentTemplate:
        IFC_RETURN(OnContentTemplateChanged(args));
        break;
    case KnownPropertyIndex::ContentPresenter_ContentTemplateSelector:
        IFC_RETURN(OnContentTemplateSelectorChanged(args));
        break;
    }

    if (textBlockPropertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        xref_ptr<CTextBlock> textBlockChildOfDefaultTemplate = GetTextBlockChildOfDefaultTemplate(false /* fAllowNullContent */);
        if (textBlockChildOfDefaultTemplate)
        {
            IFC_RETURN(textBlockChildOfDefaultTemplate->SetValueByIndex(textBlockPropertyIndex, *args.m_pNewValue));
        }
    }

    // If any inherited font properties were set, invalidate measure on any children
    // (that have a cached inherited property bag), so that they get re-measured.
    // This is necessary so that any TextBlock/TextBox elements in the control's visual
    // tree get measured during the layout pass that will follow.
    if (!ParserOwnsParent() &&
        args.m_pDP->IsInherited() &&
        args.m_pDP->AffectsMeasure())
    {
        IFC_RETURN(PropagateInheritedProperty(this, args.m_pDP));
    }
    return S_OK;
}

_Check_return_ HRESULT CContentPresenter::OnContentTemplateChanged(_In_ const PropertyChangedParams& args)
{
    return FxCallbacks::ContentPresenter_OnContentTemplateChanged(this, args);
}

_Check_return_ HRESULT CContentPresenter::OnContentTemplateSelectorChanged(_In_ const PropertyChangedParams& args)
{
    return FxCallbacks::ContentPresenter_OnContentTemplateSelectorChanged(this, args);
}

bool CContentPresenter::ParticipatesInUnloadingContentTransition()
{
    if (m_pContentTransitions && !m_pContentTransitions->empty())
    {
        for (auto transition : *m_pContentTransitions)
        {
            bool participate = false;

            static_cast<CTransition*>(transition)->ParticipateInTransitions(this, DirectUI::TransitionTrigger::Unload, &participate);

            if (participate)
            {
                return true;
            }
        }
    }

    return false;
}


//-------------------------------------------------------------------------
//
//  Function:   Content()
//
//  Synopsis:   This is the Content property getter and setter method.  This
//              is really no different than a normal property but we can't
//              use the normal property machinery because that would end up
//              adding the children to the live tree twice.  Once under the
//              Content property and once under the visual children.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CContentPresenter::Content(
    _In_ CDependencyObject* pObject,
    _In_ XUINT32 cArgs,
    _Inout_updates_(cArgs) CValue* pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue* pResult)
{
    HRESULT hr;
    CContentPresenter* pThis;
    bool fNeedToUpdateManaged = false;

    IFC(DoPointerCast(pThis, pObject));
    IFC(pThis && cArgs <= 1 ? S_OK : E_INVALIDARG);

    if (cArgs == 0)
    {
        IFCEXPECT(pResult);
        IFC(pResult->CopyConverted(pThis->m_content));
    }
    else
    {
         // The Setter
        bool fInvalidationNeeded = false;
        bool participatesInUnloading = pThis->ParticipatesInUnloadingContentTransition();

        // Invalidating the tree is only "worthwhile" if we don't have a locally defined content template or selected template,
        // Checking this early prevents unnecessary calls to FrameworkCallbacks_AreObjectsOfSameType, which is expensive,
        // and causes a performance hit in long list (Xbox store results) scrolling scenarios
        const bool fInvalidationWorthwhile = pThis->m_pContentTemplate == nullptr && pThis->GetSelectedContentTemplate() == nullptr;

        IFCEXPECT(pArgs);

        // 1. content (old) is a uielement
        // 2. content (new) is a uielement

        // 3. content is a string and this contentpresenter has contenttransitions
        // in these cases we also need to invalidate our first child in order to show that
        // the transitions on our child should seemingly 'bubble up' to this cp.
        if (pThis->m_content.AsObject() && pThis->m_content.AsObject()->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            fInvalidationNeeded = fInvalidationWorthwhile;
        }
        else if (pArgs->AsObject() && pArgs->AsObject()->OfTypeByIndex<KnownTypeIndex::UIElement>())
        {
            fInvalidationNeeded = fInvalidationWorthwhile;
        }
        else if (participatesInUnloading)
        {
            // For this case we also validate if there are ContentTransitions since the tree has to be invalidated in order for them to be processed.
            fInvalidationNeeded = fInvalidationWorthwhile || (pThis->m_pContentTransitions != nullptr && !pThis->m_pContentTransitions->empty());
        }
        // 4. Content change should trigger template invalidation if the types of
        // old and new content do not match. Exception to this rule is when the
        // old and new content are both of type valueObject, in which case some special
        // rules apply -
        // 4a. Content(MOR) -> NULL.
        // 4b. NULL -> Content(MOR) where cached content is MOR with same type object.
        // 4c. If neither the old nor the new content is UIElement, compare the types of the two objects.
        //     If the types do not match, invalidate the template.
        else
        {
            fInvalidationNeeded = fInvalidationWorthwhile;

            if (pArgs->IsNull())
            {
                CUIElement* pChild = nullptr;
                IFC(DoPointerCast(pChild, pThis->GetTemplateChildNoRef()));

                if (pChild)
                {
                    if (!pThis->m_content.IsNull() && (pThis->m_content.GetType() != valueObject || pThis->m_content.AsObject()->GetTypeIndex() == KnownTypeIndex::ExternalObjectReference))
                    {
                        CValue valueCollapsed;

                        valueCollapsed.Set(DirectUI::Visibility::Collapsed);

                        // Cache reference to old content value.
                        IFC(pThis->m_cachedContent.CopyConverted(pThis->m_content));
                        if (pThis->m_cachedContent.GetType() == valueObject)
                        {
                            IFC(pThis->m_cachedContent.AsObject()->PegManagedPeer());
                        }
                        IFC(pChild->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, valueCollapsed));

                        fInvalidationNeeded = false;
                    }
                }
                else
                {
                    fInvalidationNeeded = false;
                }
            }
            else if (pThis->m_content.IsNull() && !pArgs->IsNull() && !pThis->m_cachedContent.IsNull())
            {
                CValue valueVisible;
                CUIElement* pChild = nullptr;
                IFC(DoPointerCast(pChild, pThis->GetTemplateChildNoRef()));

                valueVisible.Set(DirectUI::Visibility::Visible);
                IFC(pChild->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, valueVisible));

                if (pArgs->GetType() == pThis->m_cachedContent.GetType())
                {
                    fInvalidationNeeded = false;

                    if (pArgs->GetType() == valueObject)
                    {
                        fInvalidationNeeded = fInvalidationWorthwhile;

                        if (fInvalidationWorthwhile && pArgs->AsObject()->GetTypeIndex() == KnownTypeIndex::ExternalObjectReference)
                        {
                            bool fAreEqual = false;
                            IFC(FxCallbacks::FrameworkCallbacks_AreObjectsOfSameType(pArgs->AsObject(), pThis->m_cachedContent.AsObject(), &fAreEqual));
                            fInvalidationNeeded = !fAreEqual;
                        }
                    }
                }

                if (pThis->m_cachedContent.GetType() == valueObject)
                {
                    pThis->m_cachedContent.AsObject()->UnpegManagedPeer();
                }

                pThis->m_cachedContent.SetNull();
            }
            else
            {
                if(pArgs->GetType() == pThis->m_content.GetType())
                {
                    fInvalidationNeeded = false;

                    if(fInvalidationWorthwhile && pThis->m_content.GetType() == valueObject)
                    {
                        // Check if the objects are of the same type. These could be managed types represented by
                        // ManagedObjectReference on the native side and, therefore, we have to reverse pinvoke to
                        // compare the types.
                        bool fAreEqual = false;
                        IFC(FxCallbacks::FrameworkCallbacks_AreObjectsOfSameType(pArgs->AsObject(), pThis->m_content.AsObject(), &fAreEqual));
                        fInvalidationNeeded = !fAreEqual;
                    }
                }
            }

            // If we got all the way here, this means that there are no content transitions
            // that participate in unloading. However, if we do have other content transitions,
            // one of these might participate in loading. If we are not invalidating our visual
            // child, though, we won't create a new tree and instead we'll just reuse the old one
            // which has already being loaded, so the load trigger will never fire. Given that, we
            // will lie to the layout manager and make it look like the visual child just entered
            // during this tick. When CTransition::OnLayoutChanged gets called as part of the arrange
            // pass, we will detect this and process the load trigger for this element.
            if (!fInvalidationNeeded && pThis->m_pContentTransitions && !pThis->m_pContentTransitions->empty())
            {
                CUIElement* pChild = nullptr;
                IFC(DoPointerCast(pChild, pThis->GetTemplateChildNoRef()));

                if (pChild)
                {
                    CLayoutManager* pLayoutManager = nullptr;
                    pLayoutManager = VisualTree::GetLayoutManagerForElement(pChild);

                    if (pLayoutManager)
                    {
                        pChild->m_enteredTreeCounter = pLayoutManager->GetLayoutCounter();
                    }
                }
            }
        }

        // If the old value was an object we will need to notify
        // the managed layer of the change
        fNeedToUpdateManaged = pThis->m_content.AsObject() != nullptr;

        // Release existing memory/reference as applicable.
        pThis->m_content.ReleaseAndReset();

        // Wholly contained CValue: copied directly.  (Color, Enum, Double, Int.)
        // CValue with pointer to memory: make our own copy.  (String, Point, Rect.)
        // CValue with DO: AddRef().
        IFC(pThis->m_content.CopyConverted(*pArgs));

        // Determine if we need to invalidate the visual children.  This will create
        //  a new data template and is unnecessary cost if we can reuse it.
        // We can reuse the same template if both old and new content are non-UIElements,
        //  so we invalidate if either the old or the new are UIElement.
        IFC(pThis->Invalidate(fInvalidationNeeded));

        // If we have an object remember its managed peer on the managed side
        // to avoid losing state
        if (fNeedToUpdateManaged || pThis->m_content.AsObject())
        {
            IFC(pThis->SetPeerReferenceToProperty(
                MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ContentPresenter_Content),
                pThis->m_content,
                false, // bPreservePegNoRef
                pValueOuter));
        }

    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: Invalidate
//
//  Synopsis:
//      Remove all the visual children and mark the node as layout dirty.
//      During the next pass all the visual children will be recreated.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CContentPresenter::Invalidate(_In_ bool bClearChildren)
{
    if (bClearChildren)
    {
        IFC_RETURN(RemoveTemplateChild());

        // Clear cached reference to the old content value.
        if (m_cachedContent.GetType() == valueObject)
        {
            m_cachedContent.AsObject()->UnpegManagedPeer();
        }
        m_cachedContent.SetNull();

        m_contentPresenterUsingDefaultTemplate = false;
    }
    else
    {
        m_bDataContextInvalid = true;
    }

    InvalidateMeasure();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: SetDataContext
//
//  Synopsis:
//      Set my binding DataContext to be the new contents.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CContentPresenter::SetDataContext()
{
    CDependencyObject *pObject = m_content.AsObject();

    // Start participating because DataContext state is being set/cleared in peer
    IFC_RETURN(SetParticipatesInManagedTreeDefault());

    // If the content of the content presenter is NULL or UI element
    // clear the DataContext
    if (m_content.IsNull() || pObject && pObject->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        // Need to clear the data context on this object to allow the content
        // to inherit from the parent
        IFC_RETURN(FxCallbacks::FrameworkCallbacks_ClearDataContext(this));
    }
    else
    {
        IFC_RETURN(EnsurePeer());

        if (pObject)
        {
            IFC_RETURN(pObject->EnsurePeer());
        }

        IFC_RETURN(FxCallbacks::FrameworkCallbacks_SetDataContext(this, &m_content));
    }

    return S_OK;
}

_Check_return_ HRESULT CContentPresenter::ApplyTemplate(_Out_ bool& fAddedVisuals)
{
    HRESULT hr = S_OK;
    CValue value;
    UIAXcp::AccessibilityView contentPresentersUIAView = UIAXcp::AccessibilityView_Content;

    if (m_bInOnApplyTemplate)
    {
        IFC(CFrameworkElement::ApplyTemplate(fAddedVisuals));
        goto Cleanup;
    }

    // Applying the template will not delete existing visuals. This will be done conditionally
    // when the template is invalidated.
    if (!HasTemplateChild())
    {
        CContentControl* pTemplatedParent = do_pointer_cast<CContentControl>(GetTemplatedParent());

        // Only ContentControl has the two properties below.  Other parents would just fail to bind since they don't have these
        // two content related properties.
        if (pTemplatedParent != nullptr)
        {
            bool needsRefresh = false;
            const CDependencyProperty* pdpTarget;

            // By default Content and ContentTemplate are template are bound.
            // If no template binding exists already then hook them up now
            pdpTarget = GetPropertyByIndexInline(KnownPropertyIndex::ContentPresenter_SelectedContentTemplate);
            IFCEXPECT(pdpTarget);
            if (IsPropertyDefault(pdpTarget) && !IsPropertyTemplateBound(pdpTarget))
            {
                const CDependencyProperty* pdpSource = pTemplatedParent->GetPropertyByIndexInline(KnownPropertyIndex::ContentControl_SelectedContentTemplate);
                IFCEXPECT(pdpSource);

                IFC(SetTemplateBinding(pdpTarget, pdpSource));
                needsRefresh = true;
            }

            pdpTarget = GetPropertyByIndexInline(KnownPropertyIndex::ContentPresenter_ContentTemplate);
            IFCEXPECT(pdpTarget);
            if (IsPropertyDefault(pdpTarget) && !IsPropertyTemplateBound(pdpTarget))
            {
                const CDependencyProperty* pdpSource = pTemplatedParent->GetPropertyByIndexInline(KnownPropertyIndex::ContentControl_ContentTemplate);
                IFCEXPECT(pdpSource);

                IFC(SetTemplateBinding(pdpTarget, pdpSource));
                needsRefresh = true;
            }

            pdpTarget = GetPropertyByIndexInline(KnownPropertyIndex::ContentPresenter_Content);
            IFCEXPECT(pdpTarget);
            if (IsPropertyDefault(pdpTarget) && !IsPropertyTemplateBound(pdpTarget))
            {
                const CDependencyProperty* pdpSource = pTemplatedParent->GetPropertyByIndexInline(KnownPropertyIndex::ContentControl_Content);
                IFCEXPECT(pdpSource);

                IFC(SetTemplateBinding(pdpTarget, pdpSource));
                needsRefresh = true;
            }

            // Setting up the binding doesn't get you the values.  We need to call refresh to get the latest value
            // for m_pContentTemplate, SelectedContentTemplate and/or m_pContent for the tests below.
            if (needsRefresh)
            {
                IFC(pTemplatedParent->RefreshTemplateBindings(TemplateBindingsRefreshType::All));
            }
        }

        m_bInOnApplyTemplate = true;
        IFC(SetDataContext());

        if (m_pContentTemplate != nullptr || GetSelectedContentTemplate() != nullptr)
        {
            // Expand the template.
            IFC(CFrameworkElement::ApplyTemplate(fAddedVisuals));
        }
        // if ContentTemplate is empty control template
        // we don't want ContentPresenter to create visuals
        else if (!m_content.IsNull())
        {
            // If the content is a UIElement then show it.
            CUIElement* pUI = do_pointer_cast<CUIElement>(m_content.AsObject());
            if (pUI)
            {
                if (pUI->IsAssociated() && !pUI->DoesAllowMultipleAssociation())
                {
                    IFC(E_INVALIDARG);
                }

                IFC(AddTemplateChild(pUI));
            }
            else
            {
                xref_ptr<CTextBlock> pTextBlockChildOfDefaultTemplate;
                IFC(CFrameworkElement::ApplyTemplate(fAddedVisuals));

                // We have a default(secret) Data template for ContentPresenter that should have its TextBlock present in all the UIA views by default.
                // But at the same time we want to mitigate this behavior specifically for Controls like Button where the TextBlock would represent redundant data.
                // At the same time we want to provide a mechanism for other controls if they want to opt-in this behavior. So if any control doesn't want these
                // secret TextBlocks to be present in a certain view they can set AutomationProperties.AccessibilityView="Raw" on the corresponding ContentPresenter,
                // we here exceptionally make sure the set property gets reflected on the secret TextBlock if the default template is getting used.
                IFC(this->GetValue(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView), &value));

                if (!value.IsEnum())
                {
                    IFC(E_FAIL);
                }

                pTextBlockChildOfDefaultTemplate = GetTextBlockChildOfDefaultTemplate(FALSE /* fAllowNullContent */);
                if (pTextBlockChildOfDefaultTemplate)
                {
                    contentPresentersUIAView = static_cast<UIAXcp::AccessibilityView>(value.AsEnum());
                    if (contentPresentersUIAView != UIAXcp::AccessibilityView_Content)
                    {
                        IFC(pTextBlockChildOfDefaultTemplate->SetValueByKnownIndex(KnownPropertyIndex::AutomationProperties_AccessibilityView, value));
                    }
                    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_OpticalMarginAlignment))
                    {
                        CValue tempValue;
                        VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_OpticalMarginAlignment, &tempValue));
                        ASSERT(tempValue.IsEnum());
                        if (static_cast<DirectUI::OpticalMarginAlignment>(tempValue.AsEnum()) != DefaultOpticalMarginAlignment())
                        {
                            IFC(pTextBlockChildOfDefaultTemplate->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_OpticalMarginAlignment, tempValue));
                        }
                    }
                    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_TextLineBounds))
                    {
                        CValue tempValue;
                        VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_TextLineBounds, &tempValue));
                        ASSERT(tempValue.IsEnum());
                        if (static_cast<DirectUI::TextLineBounds>(tempValue.AsEnum()) != DefaultTextLineBounds())
                        {
                            IFC(pTextBlockChildOfDefaultTemplate->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextLineBounds, tempValue));
                        }
                    }
                    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_TextWrapping))
                    {
                        CValue tempValue;
                        VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_TextWrapping, &tempValue));
                        ASSERT(tempValue.IsEnum());
                        if (static_cast<DirectUI::TextWrapping>(tempValue.AsEnum()) != DefaultTextWrapping())
                        {
                            IFC(pTextBlockChildOfDefaultTemplate->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextWrapping, tempValue));
                        }
                    }
                    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_LineStackingStrategy))
                    {
                        CValue tempValue;
                        VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_LineStackingStrategy, &tempValue));
                        ASSERT(tempValue.IsEnum());
                        if (static_cast<DirectUI::LineStackingStrategy>(tempValue.AsEnum()) != DefaultLineStackingStrategy())
                        {
                            IFC(pTextBlockChildOfDefaultTemplate->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_LineStackingStrategy, tempValue));
                        }
                    }
                    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_MaxLines))
                    {
                        CValue tempValue;
                        VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_MaxLines, &tempValue));
                        ASSERT(tempValue.GetType() == valueSigned);
                        if (tempValue.AsSigned() != 0)
                        {
                            IFC(pTextBlockChildOfDefaultTemplate->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_MaxLines, tempValue));
                        }
                    }
                    if (!IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_LineHeight))
                    {
                        CValue tempValue;
                        VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_LineHeight, &tempValue));
                        ASSERT(tempValue.GetType() == valueFloat);
                        if (tempValue.AsFloat() > 0)
                        {
                            IFC(pTextBlockChildOfDefaultTemplate->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_LineHeight, tempValue));
                        }
                    }
                }
            }
        }

        fAddedVisuals = GetFirstChildNoAddRef() != nullptr;
    }
    else if (m_bDataContextInvalid)
    {
        IFC(SetDataContext());
    }

Cleanup:
    if (m_pChildFromDefaultTemplate)
    {
        HRESULT hr2 = RemovePeerReferenceToItem(m_pChildFromDefaultTemplate);
        hr = FAILED(hr) ? hr : hr2;
        m_pChildFromDefaultTemplate.reset();
    }

    m_bDataContextInvalid = false;
    m_bInOnApplyTemplate = false;
    RRETURN(hr);
}

// Fetches the child TextBlock of the default template if we are using the default template; null otherwise.
xref_ptr<CTextBlock> CContentPresenter::GetTextBlockChildOfDefaultTemplate(_In_ bool fAllowNullContent)
{
    xref_ptr<CTextBlock> textBlock;

    // Make sure we are indeed using the default template (i.e. content is non-null and is not a UIElement).
    if (fAllowNullContent || (!m_content.IsNull() && !do_pointer_cast<CUIElement>(m_content.AsObject())))
    {
        CUIElementCollection* children = this->GetChildren();
        if (children)
        {
            auto cKids = children->GetCount();
            if (cKids >= 1)
            {
                xref_ptr<CUIElement> child;
                child.attach(do_pointer_cast<CUIElement>(children->GetItemDOWithAddRef(0)));
                if (child)
                {
                    // The TextBlock can now be the first child of the ContentPresenter
                    if (child->GetTypeIndex() == KnownTypeIndex::TextBlock)
                    {
                        textBlock = static_sp_cast<CTextBlock>(std::move(child));
                    }
                    else
                    {
                        // Old template with the Grid
                        children = child->GetChildren();
                        if (children)
                        {
                            cKids = children->GetCount();
                            if (cKids == 1)
                            {
                                child.attach(do_pointer_cast<CUIElement>(children->GetItemDOWithAddRef(0)));
                                if (child && child->GetTypeIndex() == KnownTypeIndex::TextBlock)
                                {
                                    textBlock = static_sp_cast<CTextBlock>(std::move(child));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return textBlock;
}

// Gets the applicable data template. The order of precedence in decreasing priority is -
// ContentPresenter.ContentTemplate, implicit data template, default template.
xref_ptr<CControlTemplate> CContentPresenter::GetTemplate() const
{
    xref_ptr<CControlTemplate> result(m_pContentTemplate);
    if (!result) {
        result = GetSelectedContentTemplate();
        if (!result) {
            result = GetContext()->GetDefaultContentPresenterTemplate();
        }
    }
    return result;
}


//-------------------------------------------------------------------------
//
//  Function:   CContentPresenter::MeasureOverride()
//
//  Synopsis:   This will return the desired size of the first (and only)
//              child.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CContentPresenter::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    XSIZEF combined = CBorder::HelperGetCombinedThickness(this);

    CUIElement* pChild;
    IFC_RETURN(DoPointerCast(pChild, GetFirstChildNoAddRef()));
    if (pChild)
    {
        XSIZEF childAvailableSize;
        childAvailableSize.width = MAX(0.0f, availableSize.width - combined.width);
        childAvailableSize.height = MAX(0.0f, availableSize.height - combined.height);

        IFC_RETURN( pChild->Measure(childAvailableSize) );
        IFC_RETURN( pChild->EnsureLayoutStorage() );
        desiredSize.width = pChild->DesiredSize.width + combined.width;
        desiredSize.height = pChild->DesiredSize.height + combined.height;
    }
    else
    {
        desiredSize.width = combined.width;
        desiredSize.height = combined.height;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CContentPresenter::ArrangeOverride()
//
//  Synopsis:   This will arrange the first (and only) child.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CContentPresenter::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)
{
        CUIElement* pChild;
        IFC_RETURN(DoPointerCast(pChild, GetFirstChildNoAddRef()));
        if (pChild)
        {
            XRECTF innerRect = CBorder::HelperGetInnerRect(this,finalSize);

            XSIZEF contentSize;
            XSIZEF contentAvailableSize;
            XRECTF contentArrangedBounds = { };
            DirectUI::HorizontalAlignment horizontalContentAlignment = GetHorizontalContentAlignment();
            DirectUI::VerticalAlignment verticalContentAlignment = GetVerticalContentAlignment();

            contentAvailableSize.height = innerRect.Height;
            contentAvailableSize.width = innerRect.Width;

            // If alignment is Stretch, use entire available size, otherwise control's desired size.
            IFC_RETURN( pChild->EnsureLayoutStorage() );
            contentSize.width  = (horizontalContentAlignment == DirectUI::HorizontalAlignment::Stretch) ? contentAvailableSize.width : pChild->DesiredSize.width;
            contentSize.height = (verticalContentAlignment == DirectUI::VerticalAlignment::Stretch) ? contentAvailableSize.height : pChild->DesiredSize.height;

            CFrameworkElement::ComputeAlignmentOffset(
                horizontalContentAlignment,
                verticalContentAlignment,
                contentAvailableSize,
                contentSize,
                contentArrangedBounds.X,
                contentArrangedBounds.Y);

            contentArrangedBounds.X += innerRect.X;
            contentArrangedBounds.Y += innerRect.Y;
            contentArrangedBounds.Width = contentSize.width;
            contentArrangedBounds.Height = contentSize.height;

            IFC_RETURN( pChild->Arrange(contentArrangedBounds) );
        }

        newFinalSize = finalSize;
        return S_OK;
}

_Check_return_ HRESULT
CContentPresenter::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    if (GetBackgroundBrush()|| GetBorderBrush())
    {
        GenerateContentBoundsImpl(pBounds);
    }
    else
    {
        EmptyRectF(pBounds);
    }
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Always generate bounds for the content of the element.
//
//  NOTE:
//      CPanel would normally return an emptyrect if no background exists, this
//      forces the bounding rectangle to be calculated
//
//------------------------------------------------------------------------

void
CContentPresenter::GenerateContentBoundsImpl(
    _Out_ XRECTF_RB* pBounds
    )
{
    pBounds->left = 0.0f;
    pBounds->top = 0.0f;
    pBounds->right = GetActualWidth();
    pBounds->bottom = GetActualHeight();
}

//-------------------------------------------------------------------------
//
//  Function:   CContentPresenter::PerformEmergencyInvalidation
//
//  Synopsis:
//      ContentPresenter may invalidate its children if not in the live tree.
//      Invalidation of the children will have the effect of removing the association which
//      allows us to reparent.
//      When the contentpresenter were to re-enter the visualtree, we would correctly fail at that point.
//      ContentPresenter does not disassociate its content while leaving the visual tree.
//      So disassociate content now, before content is re-parented. This is not done in ContentPresenter::Leave
//      because some apps depend on the old content being parented to the ContentPresenter, even
//      when not in the visual tree.
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CContentPresenter::PerformEmergencyInvalidation( _In_ CDependencyObject *pToBeReparented)
{
    if (pToBeReparented && !IsActive() && pToBeReparented->IsAssociated())
    {
        if (m_content.GetType() == valueObject && m_content.AsObject() == pToBeReparented)
        {
            // we can invalidate, and invalidating would actually help
            IFC_RETURN(Invalidate(true));
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: GetTransitionForChildElementNoAddRef
//
//  Synopsis: Child of contentpresenter should get the Child transitions of its parent
//            (this cp). Note that cp will more eagerly unload its elements.
//
//------------------------------------------------------------------------
_Check_return_ CTransitionCollection* CContentPresenter::GetTransitionsForChildElementNoAddRef(_In_ CUIElement*)
{
    return m_pContentTransitions;
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
CContentPresenter::PropagateInheritedProperty(_In_ CUIElement *pUIElement, _In_ const CDependencyProperty *pdp)
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

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pulls all non-locally set inherited properties from the parent.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CContentPresenter::PullInheritedTextFormatting()
{
    HRESULT         hr                    = S_OK;
    TextFormatting *pParentTextFormatting = NULL;

    IFCEXPECT_ASSERT(m_pTextFormatting != NULL);
    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC(GetParentTextFormatting(&pParentTextFormatting));

        // Process each TextElement text core property one by one.

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_FontFamily))
        {
            IFC(m_pTextFormatting->SetFontFamily(this, pParentTextFormatting->m_pFontFamily));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_Foreground)
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

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_FontSize))
        {
            m_pTextFormatting->m_eFontSize = pParentTextFormatting->m_eFontSize;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_FontWeight))
        {
            m_pTextFormatting->m_nFontWeight = pParentTextFormatting->m_nFontWeight;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_FontStyle))
        {
            m_pTextFormatting->m_nFontStyle = pParentTextFormatting->m_nFontStyle;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_FontStretch))
        {
            m_pTextFormatting->m_nFontStretch = pParentTextFormatting->m_nFontStretch;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_CharacterSpacing))
        {
            m_pTextFormatting->m_nCharacterSpacing = pParentTextFormatting->m_nCharacterSpacing;
        }

        m_pTextFormatting->m_nTextDecorations = pParentTextFormatting->m_nTextDecorations;

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection = pParentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::ContentPresenter_IsTextScaleFactorEnabled) &&
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

CUIElement* CContentPresenter::GetTemplateChildNoRef()
{
    return GetFirstChildNoAddRef();
}

_Check_return_
HRESULT
CContentPresenter::RemoveTemplateChild()
{
    CCollection* pChildren = GetChildren();
    if (pChildren)
    {
        IFC_RETURN(GetChildren()->Clear());
        IFC_RETURN(FxCallbacks::ContentPresenter_OnChildrenCleared(this));
    }
    return S_OK;
}

_Check_return_ HRESULT CContentPresenter::CreateDefaultContent(
        _In_opt_ const xstring_ptr *pDisplayMemberPath,
        _Outptr_ CDependencyObject** ppChild)
{
    HRESULT hr = S_OK;
    CGrid *pGrid = nullptr;
    CTextBlock* pTextBlock = nullptr;
    CValue value;
    CCoreServices *pCore = GetContext();
    CREATEPARAMETERS cp(pCore);

    *ppChild = nullptr;

    IFC(CTextBlock::Create(reinterpret_cast<CDependencyObject**>(&pTextBlock), &cp));
    // Act as if the TextBlock was the result of a template expansion
    IFC(pTextBlock->SetTemplatedParent(this));

    *ppChild = pTextBlock;
    pTextBlock->AddRef();

    value.Set(DirectUI::HorizontalAlignment::Left);
    IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));
    value.Set(DirectUI::VerticalAlignment::Top);
    IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

    // Note: Additional properties are only set when used in a list with DisplayMemberPath property
    if (pDisplayMemberPath)
    {
        value.Set(DirectUI::TextAlignment::Left);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextAlignment, value));
        value.Set(DirectUI::TextWrapping::NoWrap);
        IFC(pTextBlock->SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextWrapping, value));
    }

    // This TextBlock needs to participate in framework tree because it uses binding
    IFC(pTextBlock->SetParticipatesInManagedTreeDefault());

    // Create a temporary reference to Managed peers until *ppChild is added to the tree
    // This will be cleaned up when returned in ApplyTemplate
    // If we don't do that, the TextBlock peer is released when leaving ContentPresenter_BindDefaultTextBlock
    ASSERT(!m_pChildFromDefaultTemplate);
    IFC(AddPeerReferenceToItem(*ppChild));
    m_pChildFromDefaultTemplate = *ppChild;

    if (pCore)
    {
        IFC(FxCallbacks::ContentPresenter_BindDefaultTextBlock(pTextBlock, pDisplayMemberPath));
    }

    m_contentPresenterUsingDefaultTemplate = true;

Cleanup:
    ReleaseInterface(pTextBlock);
    ReleaseInterface(pGrid);
    RRETURN(hr);
}

xref_ptr<CDataTemplate> CContentPresenter::GetSelectedContentTemplate() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_SelectedContentTemplate, &result));
    return checked_sp_cast<CDataTemplate>(result.DetachObject());
}

/* static */ void
CContentPresenter::NWSetBorderBrushDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        CContentPresenter *pContentPresenter = static_cast<CContentPresenter*>(pTarget);

        //
        // Border brush - Dirties: (Render)
        //
        pContentPresenter->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render, pContentPresenter->m_fNWBorderBrushDirty);
        pContentPresenter->m_fNWBorderBrushDirty = TRUE;
    }
}

/* static */ void
CContentPresenter::NWSetBackgroundDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::ContentPresenter>());

    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        CContentPresenter *pContentPresenter = static_cast<CContentPresenter*>(pTarget);

        //
        // Background brush - Dirties: (Render | Bounds)
        //
        pContentPresenter->NWSetDirtyFlagsAndPropagate(flags | DirtyFlags::Render | DirtyFlags::Bounds, pContentPresenter->m_fNWBackgroundDirty);
        pContentPresenter->m_fNWBackgroundDirty = TRUE;
    }
}

bool CContentPresenter::IsMaskDirty(
    _In_ HWShapeRealization *pHwShapeRealization,
    const bool renderCollapsedMask,
    bool isFillBrushAnimated,
    bool isStrokeBrushAnimated,
    _Out_ bool* pIsFillForHitTestOnly,
    _Out_ bool* pIsStrokeForHitTestOnly
    )
{
    return CFrameworkElement::NWIsContentDirty()
        || pHwShapeRealization->MaskNeedsUpdate(
            renderCollapsedMask,
            GetBackgroundBrush().get(), m_fNWBackgroundDirty, isFillBrushAnimated, pIsFillForHitTestOnly,
            GetBorderBrush().get(), m_fNWBorderBrushDirty, isStrokeBrushAnimated, pIsStrokeForHitTestOnly);
}


xref_ptr<CBrush> CContentPresenter::GetBorderBrush() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_BorderBrush, &result));
    return static_sp_cast<CBrush>(result.DetachObject());
}

xref_ptr<CBrush> CContentPresenter::GetBackgroundBrush() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_Background, &result));
    return static_sp_cast<CBrush>(result.DetachObject());
}

XTHICKNESS CContentPresenter::GetBorderThickness() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_BorderThickness, &result));
    return *(result.AsThickness());
}

XTHICKNESS CContentPresenter::GetPadding() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_Padding, &result));
    return *(result.AsThickness());
}

XCORNERRADIUS CContentPresenter::GetCornerRadius() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_CornerRadius, &result));
    return *(result.AsCornerRadius());
}

DirectUI::BackgroundSizing CContentPresenter::GetBackgroundSizing() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_BackgroundSizing, &result));
    return static_cast<DirectUI::BackgroundSizing>(result.AsEnum());
}

DirectUI::HorizontalAlignment CContentPresenter::GetHorizontalContentAlignment() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment, &result));
    return static_cast<DirectUI::HorizontalAlignment>(result.AsEnum());

}
DirectUI::VerticalAlignment CContentPresenter::GetVerticalContentAlignment() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::ContentPresenter_VerticalContentAlignment, &result));
    return static_cast<DirectUI::VerticalAlignment>(result.AsEnum());
}

bool CContentPresenter::HasDataBoundTextBlockText()
{
    xref_ptr<CTextBlock> textBlock = GetTextBlockChildOfDefaultTemplate(true /* fAllowNullContent */);

    if (textBlock)
    {
        return FxCallbacks::TextBlock_HasDataboundText(textBlock);
    }

    return false;
}

_Check_return_ HRESULT
CContentPresenter::GetTextBlockText(
    _Out_ HSTRING *returnText)
{
    xref_ptr<CTextBlock> pTextBlock = GetTextBlockChildOfDefaultTemplate(true /* fAllowNullContent */);

    if (pTextBlock)
    {
        xruntime_string_ptr strPromoted;
        IFC_RETURN(pTextBlock->m_strText.Promote(&strPromoted));
        *returnText = strPromoted.DetachHSTRING();
    }

    return S_OK;
}

CTextBlock *CContentPresenter::GetTextBlockNoRef()
{
    return GetTextBlockChildOfDefaultTemplate(false /* fAllowNullContent */);
}

_Check_return_ HRESULT
CContentPresenter::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams& printParams
    )
{
    xref_ptr<IPALAcceleratedGeometry> pPALBackgroundGeometry;
    xref_ptr<IPALAcceleratedBrush> pPALBackgroundBrush;
    xref_ptr<IPALAcceleratedGeometry> pPALBorderGeometry;
    xref_ptr<IPALAcceleratedBrush> pPALBorderBrush;
    AcceleratedBrushParams PALBorderBrushParams;
    AcceleratedBrushParams PALBackgroundBrushParams;

    if (printParams.GetOverrideBrush() != nullptr)
    {
        pPALBackgroundBrush = printParams.GetOverrideBrush().Get();
        pPALBorderBrush = printParams.GetOverrideBrush().Get();
    }
    else
    {
        if (printParams.m_renderFill && GetBackgroundBrush())
        {
            IFC_RETURN(GetBackgroundBrush()->GetPrintBrush(printParams, pPALBackgroundBrush.ReleaseAndGetAddressOf()));
        }

        if (printParams.m_renderStroke && GetBorderBrush())
        {
            IFC_RETURN(GetBorderBrush()->GetPrintBrush(printParams, pPALBorderBrush.ReleaseAndGetAddressOf()));
        }
    }

    // Calculate the border and background geometries.
    IFC_RETURN(CreateBorderGeometriesAndBrushClipsCommon(
        sharedPrintParams.renderCollapsedMask,
        cp,
        sharedPrintParams.pCurrentTransform,
        &PALBackgroundBrushParams,
        &PALBorderBrushParams,
        printParams.m_renderFill ? pPALBackgroundGeometry.ReleaseAndGetAddressOf() : nullptr,
        printParams.m_renderStroke ? pPALBorderGeometry.ReleaseAndGetAddressOf() : nullptr));

    IFC_RETURN(AcceleratedBorderRenderCommon(
        sharedPrintParams,
        printParams,
        printParams.m_renderFill ? pPALBackgroundGeometry.get() : nullptr,
        printParams.m_renderFill ? pPALBackgroundBrush.get() : nullptr,
        &PALBackgroundBrushParams,
        printParams.m_renderStroke ? pPALBorderGeometry.get() : nullptr,
        printParams.m_renderStroke ? pPALBorderBrush.get() : nullptr,
        &PALBorderBrushParams));

    return S_OK;
}


_Check_return_ HRESULT
CContentPresenter::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    RRETURN(CBorder::HitTestLocalInternalImpl(this, target, pHit));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a polygon intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CContentPresenter::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    RRETURN(CBorder::HitTestLocalInternalImpl(this, target, pHit));
}
