// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FocusProperties.h"

#include <corep.h>
#include <DXamlServices.h>

#include "DOPointerCast.h"

#include <CDependencyObject.h>
#include <DependencyObjectAbstractionHelpers.h>
#include <CControl.h>

#include <uielement.h>
#include <UIElementCollection.h>

#include <RichTextBlock.h>
#include <TextBlock.h>

#include <WeakReferenceSourceNoThreadId.h>

#include "VisualTree.h"
#include "focusmgr.h"
#include <FocusableHelper.h>
#include <CaretBrowsingGlobal.h>

namespace FocusProperties
{

//Gets the focus children of a CDO
template<>
CDOCollection* GetFocusChildren<CDOCollection, CDependencyObject>(_In_ CDependencyObject* const object)
{
    CDOCollection* focusChildren = nullptr;

    if (object->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        focusChildren = GetFocusChildren<CDOCollection>(static_cast<CRichTextBlock*>(object));
    }
    else if (object->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
    {
        // RichTextOverflow does not have any focusable children, since its master
        // (RichTextBlock) returns those.
        focusChildren = nullptr;
    }
    else if (object->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        focusChildren = GetFocusChildren<CDOCollection>(static_cast<CTextBlock*>(object));
    }
    else if (object->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement* uiElement = static_cast<CUIElement*>(object);
        focusChildren = uiElement->GetChildren();
    }

    return focusChildren;
}

//Determine if the object is visible.
template<>
bool IsVisible<CDependencyObject>(_In_ CDependencyObject* const object)
{
    bool isVisible = true;
    CUIElement* uiElement = do_pointer_cast<CUIElement>(object);
    if (uiElement)
    {
        isVisible = IsVisible(uiElement);
    }

    return isVisible;
}

//Determine if the object and its ancestors are visible.
template<>
bool AreAllAncestorsVisible<CDependencyObject>(_In_ CDependencyObject* const object)
{
    CUIElement* objectAsUIElement = do_pointer_cast<CUIElement>(object);

    if (objectAsUIElement)
    {
        return AreAllAncestorsVisible(objectAsUIElement);
    }

    CTextElement* objectAsTextElement = do_pointer_cast<CTextElement>(object);
    if (objectAsTextElement)
    {
        CUIElement* containingElement = objectAsTextElement->GetContainingFrameworkElement();
        return AreAllAncestorsVisible(containingElement);
    }

    return true;
}

template<>
bool IsEnabled<CDependencyObject>(_In_ CDependencyObject* const object)
{
    bool isEnabled = true;

    if (CControl* control = do_pointer_cast<CControl>(object))
    {
        isEnabled = IsEnabled(control);
    }

    return isEnabled;
}


//Determine if a particular DependencyObject cares to take focus.
template<>
bool IsFocusable<CDependencyObject>(_In_ CDependencyObject* const object)
{
    bool isFocusable = false;

    xref_ptr<CUIElement> objectAsUI;

    if (SUCCEEDED(DoPointerCast(objectAsUI, object)) && objectAsUI->SkipFocusSubtree())
    {
        return false;
    }

    if (object->OfTypeByIndex<KnownTypeIndex::Control>())
    {
        CControl* control = static_cast<CControl*>(object);

        ASSERT(control);
        isFocusable = IsFocusable(control);
    }
    else if (IFocusable* ifocusable = CFocusableHelper::GetIFocusableForDO(object))
    {
        isFocusable = ifocusable->IsFocusable();
    }
    else
    {
        if (objectAsUI)
        {
            isFocusable = IsFocusable(objectAsUI.get());
        }
    }

    // NULL objects, UIElements, panels, etc. are not focusable
    return isFocusable;
}

template<>
bool IsPotentialTabStop<CDependencyObject>(_In_ CDependencyObject* const object)
{
    if (object)
    {
        if (object->OfTypeByIndex<KnownTypeIndex::Control>() ||
            CFocusableHelper::IsFocusableDO(object))
        {
            return true;
        }
        else if (object->OfTypeByIndex<KnownTypeIndex::TextBlock>() || object->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            return GetCaretBrowsingModeEnable() || static_cast<CUIElement*>(object)->IsTabStop();
        }
        else
        {
            CUIElement* objectAsUI = nullptr;
            if (SUCCEEDED(DoPointerCast(objectAsUI, object)) && objectAsUI && (objectAsUI->IsTabStop()))
            {
                return true;
            }
        }
    }

    return false;
}

//Return true if there is a focusable child
template<>
bool CanHaveFocusableChildren<CDependencyObject>(_In_ CDependencyObject* const parent)
{
    CDOCollection* collection = nullptr;
    bool isFocusable = false;

    xref_ptr<CUIElement> parentAsUI;

    if (SUCCEEDED(DoPointerCast(parentAsUI, parent)) && parentAsUI->SkipFocusSubtree())
    {
        return false;
    }

    collection = GetFocusChildren<CDOCollection>(parent);

    if (collection && !collection->IsLeaving())
    {
        const auto kidCount = collection->GetCount();

        for (XUINT32 i = 0; i < kidCount && isFocusable == false; i++)
        {
            xref_ptr<CDependencyObject> child;
            child.attach(static_cast<CDependencyObject*>(collection->GetItemWithAddRef(i)));

            if (child && IsVisible(child.get()))
            {
                // Ignore TextBlock/RichTextBlock/RIchTextBlockOverflow here since we don?t want them
                // to get focus through FocusManager operations e.g. tab navigation.
                // We only allow focus on them through direct pointer input.  If we don?t ignore them here,
                // then tabbing into a control with Text elements may cause us to reset focus to the first
                // focusable item on the page, since we report CanHaveFocusableChildren=TRUE but since
                // IsTabStop=FALSE the focus operation fails.
                if (child->OfTypeByIndex<KnownTypeIndex::TextBlock>() ||
                    child->OfTypeByIndex<KnownTypeIndex::RichTextBlock>() ||
                    child->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
                {
                    if (IsFocusable(child.get()) && IsPotentialTabStop(child.get()))
                    {
                        isFocusable = true;
                    }
                    else
                    {
                        if (CanHaveFocusableChildren(child.get()))
                        {
                            isFocusable = true;
                        }
                    }
                }
                else
                {
                    if (IsFocusable(child.get()))
                    {
                        isFocusable = true;
                    }
                    else
                    {
                        if (CanHaveFocusableChildren(child.get()))
                        {
                            isFocusable = true;
                        }
                    }
                }
            }
        }
    }

    return isFocusable;
}

template<>
bool IsFocusEngagementEnabled<CDependencyObject>(_In_ CDependencyObject* const object)
{
    xref_ptr<CControl> control;
    control = do_pointer_cast<CControl>(object);

    bool isEngagedEnabled = false;

    if (control)
    {
        isEngagedEnabled = IsFocusEngagementEnabled(control.get());
    }

    return isEngagedEnabled;
}

template<>
bool IsFocusEngaged<CDependencyObject>(_In_ CDependencyObject* const object)
{
    xref_ptr<CControl> control;
    control = do_pointer_cast<CControl>(object);

    bool isEngaged = false;

    if (control)
    {
        isEngaged = IsFocusEngaged(control.get());
    }

    return isEngaged;
}

template<>
bool IsGamepadFocusCandidate<CDependencyObject>(_In_ CDependencyObject* const object)
{
    bool isGamepadFocusCandidate = true;
    if (object->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CValue val;
        object->GetValueByIndex(KnownPropertyIndex::UIElement_IsGamepadFocusCandidate, &val);
        isGamepadFocusCandidate = (val.AsBool() == TRUE);
    }

    return isGamepadFocusCandidate;
}

template<>
bool ShouldSkipFocusSubTree<CDependencyObject>(_In_ CDependencyObject* const parent)
{
    xref_ptr<CUIElement> parentAsUI;

    if (SUCCEEDED(DoPointerCast(parentAsUI, parent)) && parentAsUI->SkipFocusSubtree())
    {
        return true;
    }
    return false;
}

template<>
bool HasFocusedElement<CDependencyObject>(_In_ CDependencyObject* const element)
{
    if (element->IsActive())
    {
        const CFocusManager* const focusManager = VisualTree::GetFocusManagerForElement(element);

        if (element == focusManager->GetFocusedElementNoRef())
        {
            return true;
        }

        CDOCollection* const collection = GetFocusChildren<CDOCollection>(element);

        if (collection && collection->GetCount() > 0)
        {
            for (XUINT32 i = 0; i < collection->GetCount(); i++)
            {
                xref_ptr<CDependencyObject> child;
                child.attach(static_cast<CDependencyObject*>(collection->GetItemWithAddRef(i)));

                if (child.get())
                {
                    if (HasFocusedElement(child.get())) { return true; }
                }
            }
        }
    }

    return false;
}

FocusChildrenIteratorWrapper
GetFocusChildrenInTabOrderIterator(_In_ CDependencyObject* const parent)
{
    bool useCustomIterator = false;
    wrl::ComPtr<wfc::IIterator<xaml::DependencyObject*>> customIterator;

    DirectUI::DependencyObject* peerDO = nullptr;
    if (SUCCEEDED(DirectUI::DXamlServices::TryGetPeer(parent, &peerDO)) && peerDO)
    {
        wrl::ComPtr<xaml::IDependencyObject> peer;
        peer.Attach(DirectUI::DependencyObjectAbstractionHelpers::DOtoIDO(peerDO));
        if (DirectUI::DependencyObjectAbstractionHelpers::DOtoWRSNTI(peerDO)->IsComposed())
        {
            wrl::ComPtr<xaml::IUIElementOverrides> peerAsUIE;
            wrl::ComPtr<wfc::IIterable<xaml::DependencyObject*>> customIterable;
            useCustomIterator =
                SUCCEEDED(peer.As<xaml::IUIElementOverrides>(&peerAsUIE)) &&
                SUCCEEDED(peerAsUIE->GetChildrenInTabFocusOrder(&customIterable)) &&
                (!customIterable || SUCCEEDED(customIterable->First(&customIterator)));
        }
    }

    return
        useCustomIterator ?
        FocusChildrenIteratorWrapper(customIterator.Get()) :
        FocusChildrenIteratorWrapper(GetFocusChildren<CDOCollection>(parent));
}
}