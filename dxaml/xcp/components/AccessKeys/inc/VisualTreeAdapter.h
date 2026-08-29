// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <RootVisual.h>
#include <VisualTree.h>
#include "AKCommon.h"
#include <CValueBoxer.h>
#include <MetadataAPI.h>
#include <FxCallbacks.h>

class CUIElementCollection;

namespace AccessKeys
{
    class AKVisualTreeFinder
    {
    public:
        AKVisualTreeFinder(_In_ CCoreServices* core) : m_visualTree(nullptr)
        {
            CRootVisual* rootVisual = core->GetMainRootVisual();

            if (rootVisual != nullptr)
            {
                m_visualTree = rootVisual->GetAssociatedVisualTree();
            }
        }

        void SetVisualTree(_In_ VisualTree* tree)
        {
            m_visualTree = tree;
        }

        CDOCollection* GetChildren(_In_ CDependencyObject* const element)
        {
            if (element->OfTypeByIndex<KnownTypeIndex::MenuFlyoutSubItem>())
            {
                CValue result;
                VERIFYHR(element->GetValueByIndex(KnownPropertyIndex::MenuFlyoutSubItem_Items, &result));
                return static_cast<CDOCollection*>(result.AsObject());
            }
            else if (element->OfTypeByIndex<KnownTypeIndex::TextBlock>())
            {
                CValue result;
                VERIFYHR(element->GetValueByIndex(KnownPropertyIndex::TextBlock_Inlines, &result));
                return static_cast<CDOCollection*>(result.AsObject());
            }
            else if (element->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
            {
                CValue result;
                VERIFYHR(element->GetValueByIndex(KnownPropertyIndex::RichTextBlock_Blocks, &result));
                return static_cast<CDOCollection*>(result.AsObject());
            }
            else if (element->OfTypeByIndex<KnownTypeIndex::UIElement>())
            {
                CUIElement* uiElement = static_cast<CUIElement*>(element);
                return uiElement->GetChildren();
            }
            else if (element->OfTypeByIndex<KnownTypeIndex::Paragraph>())
            {
                CValue result;
                VERIFYHR(element->GetValueByIndex(KnownPropertyIndex::Paragraph_Inlines, &result));
                return static_cast<CDOCollection*>(result.AsObject());
            }
            else if (element->OfTypeByIndex<KnownTypeIndex::Span>())
            {
                CValue result;
                VERIFYHR(element->GetValueByIndex(KnownPropertyIndex::Span_Inlines, &result));
                return static_cast<CDOCollection*>(result.AsObject());
            }

            return nullptr;
        }

        CDependencyObject* GetParent(_In_ CDependencyObject* const element)
        {
            return element->GetParent();
        }

        CDependencyObject* GetMentor(_In_ CDependencyObject* const element)
        {
            return element->GetMentor();
        }

        // TODO:  This can be refactored to use the AKCommon.h version.  Deferring this work for now due to the
        // impact on unit tests.
        static bool IsScope(_In_ CDependencyObject* element)
        {
            return AccessKeys::IsAccessKeyScope(element);
        }

        CDependencyObject* GetScopeOwner(_In_ CDependencyObject* element)
        {
            if (element->OfTypeByIndex<KnownTypeIndex::MenuFlyoutPresenter>())
            {
                // MenuFlyout and MenuFlyoutPresenter are special, because logically the MenuFlyout is the parent of MenuFlyoutPresenter
                // but the MenuFlyoutPresenter is a popup, so is not actually a descendant of MenuFlyout.  To handle this, we consider
                // all MenuFlyoutPresenters to have their MenuFlyout as their scope owner.  This makes the descendants of MenuFlyoutPresenter
                // part of the MenuFlyout's scope (and no other scope), just like we expect.
                return GetParentFromMenuFlyoutPresenter(static_cast<CFrameworkElement*>(element));
            }
            else if (element->OfTypeByIndex<KnownTypeIndex::UIElement>())
            {
                return GetOwnerHelper(element, KnownPropertyIndex::UIElement_AccessKeyScopeOwner);
            }
            else if (element->OfTypeByIndex<KnownTypeIndex::TextElement>())
            {
                return GetOwnerHelper(element, KnownPropertyIndex::TextElement_AccessKeyScopeOwner);
            }

            return nullptr;
        }

        void GetAllVisibleRootsNoRef(_Out_writes_(3) CDependencyObject** roots)
        {
            return m_visualTree->GetAllVisibleRootsNoRef(roots);
        }
    private:
        static CDependencyObject* GetOwnerHelper(_In_ CDependencyObject* element, KnownPropertyIndex index)
        {
            CValue value;

            if (FAILED(element->GetValueByIndex(index, &value)))
            {
                return nullptr;
            }

            CDependencyObject* owner = nullptr;

            if (FAILED(DirectUI::CValueBoxer::UnwrapWeakRef(&value, DirectUI::MetadataAPI::GetDependencyPropertyByIndex(index), &owner)))
            {
                return nullptr;
            }

            return owner;
        }

        CDependencyObject* GetParentFromMenuFlyoutPresenter(_In_ CFrameworkElement* const pFrameworkElement)
        {
            ASSERT(pFrameworkElement->OfTypeByIndex<KnownTypeIndex::MenuFlyoutPresenter>());

            auto pLogicalParent = pFrameworkElement->GetLogicalParentNoRef();
            if (pLogicalParent->OfTypeByIndex<KnownTypeIndex::Popup>())
            {
                auto pPopup = static_cast<CPopup* const>(pLogicalParent);
                if (pPopup->IsFlyout())
                {
                    //
                    // The flyout associated with the Popup is not living in the VisualTree
                    // So we need to use the link between the Popup and the flyout to correctly
                    // resolve scope owner references made in the markup.
                    //
                    return static_cast<CDependencyObject*>(pPopup->GetAssociatedFlyoutNoRef());
                }
                else
                {
                    CDependencyObject* pParent = nullptr;
                    VERIFYHR(FxCallbacks::FlyoutPresenter_GetParentMenuFlyoutSubItem(pFrameworkElement, &pParent));
                    return pParent;
                }
            }
            return nullptr;
        }

        VisualTree* m_visualTree;

    };

    typedef class AKVisualTreeLibraryImpl VisualTreeLibraryImpl;
}
