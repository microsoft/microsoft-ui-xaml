// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextControlFlyoutHelper.h"
#include "FlyoutBase.g.h"
#include "FlyoutShowOptions.g.h"
#include "LosingFocusEventArgs.g.h"
#include "CoreEventArgsGroup.h"
#include "TextBoxBase.h"
#include "TextSelectionManager.h"
#include "UIElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace std::placeholders;

bool TextControlFlyoutHelper::IsGettingFocus(_In_opt_ CFlyoutBase * flyout, _In_ CFrameworkElement * owner)
{
	// This method is a bit under named.  In addition to determining whether the specified flyout is in the
	// process of getting focus, it also determines whether that flyout is associated with a specific 
	// element (text block)

	auto activeFlyout = DXamlCore::GetCurrent()->GetTextControlFlyout(flyout);

    if (!activeFlyout || !activeFlyout->IsGettingFocus())
    {
	    // If we don't have an active flyout or it is not getting focus then we don't need to check the owner
        return false;
    }

    auto activeOwner = activeFlyout->GetActiveOwnerNoRef();
    if (activeOwner != owner)
    {
        if (auto richTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(activeOwner))
        {
            // RichTextBlockOverflow elements are special in that although the flyout is displaying over the
            // RichTextBlockOverflow, the actual owner of the flyout is the primary RichText Block.
            activeOwner = richTextBlockOverflow->m_pMaster;
        }
    }

    return activeOwner == owner;
}


bool TextControlFlyoutHelper::IsOpen(_In_ CFlyoutBase* flyout)
{
    auto activeFlyout = DXamlCore::GetCurrent()->GetTextControlFlyout(flyout);
    return activeFlyout && activeFlyout->IsOpen();
}

bool TextControlFlyoutHelper::IsElementChildOfOpenedFlyout(_In_opt_ CUIElement *element)
{
    auto flyout = CPopup::GetClosestFlyoutAncestor(element);

    auto activeTextControlFlyout = DXamlCore::GetCurrent()->GetTextControlFlyout(flyout);
    return activeTextControlFlyout && activeTextControlFlyout->IsOpened();
}

bool TextControlFlyoutHelper::IsElementChildOfTransientOpenedFlyout(_In_opt_ CUIElement* element)
{
    auto flyout = CPopup::GetClosestFlyoutAncestor(element);

    auto activeTextControlFlyout = DXamlCore::GetCurrent()->GetTextControlFlyout(flyout);
    return activeTextControlFlyout && activeTextControlFlyout->IsOpened() && activeTextControlFlyout->IsTransient();
}

bool TextControlFlyoutHelper::IsElementChildOfProofingFlyout(_In_opt_ CUIElement *element)
{
    auto flyout = CPopup::GetClosestFlyoutAncestor(element);

    auto activeTextControlFlyout = DXamlCore::GetCurrent()->GetTextControlFlyout(flyout);
    return activeTextControlFlyout && activeTextControlFlyout->IsProofingFlyout();
}

_Check_return_ HRESULT TextControlFlyoutHelper::DismissAllFlyoutsForOwner(_In_opt_ CUIElement *element)
{
    auto flyout = CPopup::GetClosestFlyoutAncestor(element);
    auto activeTextControlFlyout = DXamlCore::GetCurrent()->GetTextControlFlyout(flyout);
    if (activeTextControlFlyout)
    {
        CFrameworkElement* owner = activeTextControlFlyout->GetActiveOwnerNoRef();
        if (auto textBoxBase = do_pointer_cast<CTextBoxBase>(owner))
        {
            IFC_RETURN(textBoxBase->DismissAllFlyouts());
        }
        else
        {
            TextSelectionManager* selectionManager = nullptr;

            if (auto textBlock = do_pointer_cast<CTextBlock>(owner))
            {
                selectionManager = textBlock->GetSelectionManager();
            }
            else if (auto richTextBlock = do_pointer_cast<CRichTextBlock>(owner))
            {
                selectionManager = richTextBlock->GetSelectionManager();
            }
            else if (auto richTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(owner))
            {
                selectionManager = richTextBlockOverflow->GetMaster()->GetSelectionManager();
            }

            if (selectionManager)
            {
                IFC_RETURN(selectionManager->DismissAllFlyouts());
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT TextControlFlyoutHelper::CloseIfOpen(_In_opt_ CFlyoutBase* flyout)
{
    auto activeFlyout = DXamlCore::GetCurrent()->GetTextControlFlyout(flyout);
    if (activeFlyout)
    {
        IFC_RETURN(activeFlyout->CloseIfOpen());
    }

    return S_OK;
}

_Check_return_ HRESULT TextControlFlyoutHelper::AddProofingFlyout(_In_ CFlyoutBase* flyout, _In_ CFrameworkElement* owner)
{
    auto core = DXamlCore::GetCurrent();
    TextControlFlyout* activeFlyout = core->GetTextControlFlyout(flyout);

    // If the flyout isn't in the map, add it
    if (!activeFlyout)
    {
        ctl::ComPtr<FlyoutBase> flyoutBase;
        IFC_RETURN(core->GetPeer<FlyoutBase>(flyout, &flyoutBase));

        activeFlyout = new TextControlFlyout(flyout, true /* isProofingFlyout */);
        core->SetTextControlFlyout(flyoutBase.Get(), activeFlyout);
    }

    // Update the flyout's active owner
    if (activeFlyout->GetActiveOwnerNoRef() != owner)
    {
        activeFlyout->SetActiveOwner(owner);
    }

    return S_OK;
}


_Check_return_ HRESULT TextControlFlyoutHelper::ShowAt(_In_ CFlyoutBase* flyout, _In_ CFrameworkElement* owner, wf::Point point, wf::Rect exclusionRect, xaml_primitives::FlyoutShowMode showMode)
{
    // For backward compatibility reason, we need to fire ContextMenuOpening event for every floatie invocation.
    // We also need to check if app handled the event before showing the floatie.
    if (auto textBoxBase = do_pointer_cast<CTextBoxBase>(owner))
    {
        bool handled = false;
        IFC_RETURN(textBoxBase->FireContextMenuOpeningEventSynchronously(handled, point));
        if (handled)
        {
            return S_OK;
        }
    }
    else if (auto textBoxBlock = do_pointer_cast<CTextBlock>(owner))
    {
        bool handled = false;
        IFC_RETURN(textBoxBlock->FireContextMenuOpeningEventSynchronously(handled, point));
        if (handled)
        {
            return S_OK;
        }
    }
    else if (auto richTextBoxBlock = do_pointer_cast<CRichTextBlock>(owner))
    {
        bool handled = false;
        IFC_RETURN(richTextBoxBlock->FireContextMenuOpeningEventSynchronously(handled, point));
        if (handled)
        {
            return S_OK;
        }
    }

    auto core = DXamlCore::GetCurrent();
    TextControlFlyout* activeFlyout = core->GetTextControlFlyout(flyout);

    // If the flyout isn't in the map, add it
    if (!activeFlyout)
    {
        ctl::ComPtr<FlyoutBase> flyoutBase;
        IFC_RETURN(core->GetPeer<FlyoutBase>(flyout, &flyoutBase));

        activeFlyout = new TextControlFlyout(flyout, false /* isProofingFlyout */);
        core->SetTextControlFlyout(flyoutBase.Get(), activeFlyout);
    }

    // Update the flyout's active owner
    if (activeFlyout->GetActiveOwnerNoRef() != owner)
    {
        activeFlyout->SetActiveOwner(owner);
    }

    IFC_RETURN(activeFlyout->ShowAt(point, exclusionRect, showMode));

    return S_OK;
}

//
// TextControlFlyout
//

TextControlFlyout::TextControlFlyout(_In_ CFlyoutBase* flyout, bool isProofingFlyout):
    m_isProofingFlyout(isProofingFlyout)
{
    m_flyoutWeakRef = xref::get_weakref(flyout);

    ctl::ComPtr<FlyoutBase> flyoutBase;
    VERIFYHR(DXamlCore::GetCurrent()->GetPeer<FlyoutBase>(flyout, &flyoutBase));

    xaml_primitives::FlyoutShowMode showMode;
    VERIFYHR(flyoutBase->get_ShowMode(&showMode));

    m_isTransient =
        showMode == xaml_primitives::FlyoutShowMode_Transient ||
        showMode == xaml_primitives::FlyoutShowMode_TransientWithDismissOnPointerMoveAway;

    VERIFYHR(m_openedHandler.AttachEventHandler(
        flyoutBase.Get(),
        std::bind(&TextControlFlyout::OnOpened, this, _1, _2)));

    VERIFYHR(m_closedHandler.AttachEventHandler(
        flyoutBase.Get(),
        std::bind(&TextControlFlyout::OnClosed, this, _1, _2)));

    VERIFYHR(m_closingHandler.AttachEventHandler(
        flyoutBase.Get(),
        std::bind(&TextControlFlyout::OnClosing, this, _1, _2)));
}

TextControlFlyout::~TextControlFlyout()
{
    m_activeOwnerWeakRef.reset();
    m_flyoutWeakRef.reset();
}

_Check_return_ HRESULT TextControlFlyout::OnClosing(_In_ xaml_primitives::IFlyoutBase* sender, _In_ IFlyoutBaseClosingEventArgs* eventArgs)
{
    m_isGettingFocus = false;
    return S_OK;
}

_Check_return_ HRESULT TextControlFlyout::OnClosed(_In_ IInspectable* sender, _In_ IInspectable* eventArgs)
{
    // For some reason, the Closed event is sometimes being fired when the flyout is still open...
    // Verify the flyout is closed and close it if it isn't.
    IFC_RETURN(CloseIfOpen());

    m_isOpened = false;
    IFC_RETURN(ClearActiveOwnerEventHandlers());

    auto activeOwner = GetActiveOwnerNoRef();

    if (activeOwner && !activeOwner->IsFocused())
    {
        if (!IsProofingFlyout())
        {
            if (auto textBoxBase = do_pointer_cast<CTextBoxBase>(activeOwner))
            {
                IFC_RETURN(textBoxBase->ForceFocusLoss());
            }
            else
            {
                TextSelectionManager* selectionManager = nullptr;

                if (auto textBlock = do_pointer_cast<CTextBlock>(activeOwner))
                {
                    selectionManager = textBlock->GetSelectionManager();
                }
                else if (auto richTextBlock = do_pointer_cast<CRichTextBlock>(activeOwner))
                {
                    selectionManager = richTextBlock->GetSelectionManager();
                }
                else if (auto richTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(activeOwner))
                {
                    selectionManager = richTextBlockOverflow->GetMaster()->GetSelectionManager();
                }

                if (selectionManager)
                {
                    IFC_RETURN(selectionManager->ForceFocusLoss());
                }
            }
        }
    }

    m_activeOwnerWeakRef.reset();

    return S_OK;
}

_Check_return_ HRESULT TextControlFlyout::OnOpened(_In_ IInspectable* sender, _In_ IInspectable* eventArgs)
{
    m_isOpened = true;
    return S_OK;
}

_Check_return_ HRESULT TextControlFlyout::OnActiveOwnerLosingFocus(_In_ IUIElement* sender, _In_ ILosingFocusEventArgs* eventArgs)
{
    xref_ptr<CEventArgs> coreEventArgs;
    coreEventArgs.attach(static_cast<LosingFocusEventArgs*>(eventArgs)->GetCorePeer());

    auto coreLosingFocusEventArgs = static_cast<CLosingFocusEventArgs*>(coreEventArgs.get());

    xref_ptr<CDependencyObject> newFocusedElement;
    coreLosingFocusEventArgs->get_NewFocusedElement(newFocusedElement.ReleaseAndGetAddressOf());

    auto contactPopup = CPopup::GetClosestPopupAncestor(do_pointer_cast<CUIElement>(newFocusedElement.get()));
    auto flyoutGettingFocus = contactPopup ? contactPopup->GetAssociatedFlyoutNoRef() : nullptr;

    if (flyoutGettingFocus && flyoutGettingFocus == GetFlyoutNoRef())
    {
        m_isGettingFocus = true;
    }

    return S_OK;
}

void TextControlFlyout::SetActiveOwner(_In_ CFrameworkElement* owner)
{
    VERIFYHR(ClearActiveOwnerEventHandlers());

    m_activeOwnerWeakRef = xref::get_weakref(owner);

    ctl::ComPtr<UIElement> ownerAsUI;
    VERIFYHR(DXamlCore::GetCurrent()->GetPeer<UIElement>(owner, &ownerAsUI));

    VERIFYHR(m_activeOwnerLosingFocusEventHandler.AttachEventHandler(
        ownerAsUI.Get(),
        std::bind(&TextControlFlyout::OnActiveOwnerLosingFocus, this, _1, _2)));

    if( auto flyout = GetFlyoutNoRef() )
    {
        auto visualTree = VisualTree::GetForElementNoRef(owner);
        ASSERT(nullptr != visualTree);

        // this is an unusual case where element is moved explicitly from one xaml root to another
        // calling setvisualtree instead of calling visualTree->AttachElement(flyout), allows one to overwrite visual tree without any checks
        flyout->SetVisualTree(visualTree);
    }
}

bool TextControlFlyout::IsOpen()
{
    bool isOpen = false;
    VERIFYHR(FlyoutBase::IsOpen(GetFlyoutNoRef(), isOpen));

    return isOpen;
}

_Check_return_ HRESULT TextControlFlyout::CloseIfOpen()
{
    if (IsOpen())
    {
        IFC_RETURN(FlyoutBase::HideFlyout(GetFlyoutNoRef()));
    }

    return S_OK;
}

_Check_return_ HRESULT TextControlFlyout::ShowAt(wf::Point point, wf::Rect exclusionRect, xaml_primitives::FlyoutShowMode showMode)
{
    // If the flyout is open, close it so it refreshes the buttons
    IFC_RETURN(CloseIfOpen());

    // ShowAtStatic might return a failed HR if the pointer was released outside the window.
    IGNOREHR(FlyoutBase::ShowAtStatic(GetFlyoutNoRef(), GetActiveOwnerNoRef(), point, exclusionRect, showMode));

    return S_OK;
}

_Check_return_ HRESULT TextControlFlyout::ClearActiveOwnerEventHandlers()
{
    auto activeOwner = GetActiveOwnerNoRef();
    if (activeOwner)
    {
        ctl::ComPtr<IInspectable> ownerAsI;
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer<IInspectable>(activeOwner, &ownerAsI));

        IFC_RETURN(m_activeOwnerLosingFocusEventHandler.DetachEventHandler(ownerAsI.Get()));
    }

    return S_OK;
}
