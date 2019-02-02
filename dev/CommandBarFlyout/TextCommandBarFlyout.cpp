// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TextCommandBarFlyout.h"
#include "CommandBarFlyoutCommandBar.h"
#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"

TextCommandBarFlyout::TextCommandBarFlyout()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TextCommandBarFlyout);

    Opening({
        [this](auto const&, auto const&)
        {
            UpdateButtons();

            // If there aren't any primary commands and we aren't opening expanded,
            // or if there are just no commands at all, then we'll have literally no UI to show. 
            // We'll just close the flyout in that case - nothing should be opening us
            // in this state anyway, but this makes sure we don't have a light-dismiss layer
            // with nothing visible to light dismiss.
            if (PrimaryCommands().Size() == 0 &&
                (SecondaryCommands().Size() == 0 || !m_commandBar.get().IsOpen()))
            {
                Hide();
            }
        }
    });
}

void TextCommandBarFlyout::InitializeButtonWithUICommand(
    winrt::ButtonBase const& button,
    winrt::XamlUICommand const& uiCommand,
    std::function<void()> const& executeFunc)
{
    m_buttonCommandRevokers.push_back(uiCommand.ExecuteRequested(winrt::auto_revoke, [executeFunc](auto const&, auto const&) { executeFunc(); }));
    button.Command(uiCommand);
}

void TextCommandBarFlyout::InitializeButtonWithProperties(
    winrt::ButtonBase const& button,
    ResourceIdType labelResourceId,
    ResourceIdType acceleratorKeyResourceId,
    ResourceIdType descriptionResourceId,
    std::function<void()> const& executeFunc)
{
    winrt::AppBarButton elementAsButton = safe_try_cast<winrt::AppBarButton>(button);
    winrt::AppBarToggleButton elementAsToggleButton = safe_try_cast<winrt::AppBarToggleButton>(button);

    MUX_ASSERT(elementAsButton || elementAsToggleButton);

    if (!ResourceAccessor::IsResourceIdNull(labelResourceId))
    {
        winrt::hstring label{ ResourceAccessor::GetLocalizedStringResource(labelResourceId) };

        if (elementAsButton)
        {
            elementAsButton.Label(label);
        }
        else
        {
            elementAsToggleButton.Label(label);
        }
    }

    if (!ResourceAccessor::IsResourceIdNull(descriptionResourceId))
    {
        winrt::hstring description{ ResourceAccessor::GetLocalizedStringResource(descriptionResourceId) };

        winrt::AutomationProperties::SetHelpText(button, description);
        winrt::ToolTipService::SetToolTip(button, box_value(description));
    }

    if (elementAsToggleButton)
    {
        m_toggleButtonCheckedRevokers.push_back(elementAsToggleButton.Checked(winrt::auto_revoke, [executeFunc](auto const&, auto const&) { executeFunc(); }));
        m_toggleButtonUncheckedRevokers.push_back(elementAsToggleButton.Unchecked(winrt::auto_revoke, [executeFunc](auto const&, auto const&) { executeFunc(); }));
    }
    else
    {
        m_buttonClickRevokers.push_back(button.Click(winrt::auto_revoke, [executeFunc](auto const&, auto const&) { executeFunc(); }));
    }

    if (!ResourceAccessor::IsResourceIdNull(acceleratorKeyResourceId))
    {
        if (auto elementAsIUIElement7 = safe_try_cast<winrt::IUIElement7>(button))
        {
            winrt::hstring acceleratorKeyString{ ResourceAccessor::GetLocalizedStringResource(acceleratorKeyResourceId) };

            if (acceleratorKeyString.size() > 0)
            {
                WCHAR acceleratorKeyChar = acceleratorKeyString[0];
                winrt::VirtualKey acceleratorKey = SharedHelpers::GetVirtualKeyFromChar(acceleratorKeyChar);

                if (acceleratorKey != winrt::VirtualKey::None)
                {
                    winrt::KeyboardAccelerator keyboardAccelerator;
                    keyboardAccelerator.Key(acceleratorKey);
                    keyboardAccelerator.Modifiers(winrt::VirtualKeyModifiers::Control);

                    elementAsIUIElement7.KeyboardAccelerators().Append(keyboardAccelerator);
                }
            }
        }
    }
}

void TextCommandBarFlyout::InitializeButtonWithProperties(
    winrt::ButtonBase const& button,
    ResourceIdType labelResourceId,
    winrt::Symbol const& symbol,
    ResourceIdType acceleratorKeyResourceId,
    ResourceIdType descriptionResourceId,
    std::function<void()> const& executeFunc)
{
    InitializeButtonWithProperties(
        button,
        labelResourceId,
        acceleratorKeyResourceId,
        descriptionResourceId,
        executeFunc);

    winrt::AppBarButton elementAsButton = safe_try_cast<winrt::AppBarButton>(button);
    winrt::AppBarToggleButton elementAsToggleButton = safe_try_cast<winrt::AppBarToggleButton>(button);

    MUX_ASSERT(elementAsButton || elementAsToggleButton);

    winrt::SymbolIcon symbolIcon{ winrt::SymbolIcon(symbol) };

    if (elementAsButton)
    {
        elementAsButton.Icon(symbolIcon);
    }
    else
    {
        elementAsToggleButton.Icon(symbolIcon);
    }
}

void TextCommandBarFlyout::UpdateButtons()
{
    PrimaryCommands().Clear();
    SecondaryCommands().Clear();

    auto buttonsToAdd = GetButtonsToAdd();
    auto addButtonToCommandsIfPresent =
        [buttonsToAdd, this](auto buttonType, auto commandsList)
        {
            if ((buttonsToAdd & buttonType) != TextControlButtons::None)
            {
                commandsList.Append(GetButton(buttonType));
            }
        };
    auto addRichEditButtonToCommandsIfPresent =
        [buttonsToAdd, this](auto buttonType, auto commandsList, auto getIsChecked)
        {
            if ((buttonsToAdd & buttonType) != TextControlButtons::None)
            {
                auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(Target());
                auto toggleButton{ GetButton(buttonType).as<winrt::AppBarToggleButton>() };
                auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

                if (selection)
                {
                    auto initializingButtons = gsl::finally([this]()
                    {
                        m_isSettingToggleButtonState = false;
                    });
                    m_isSettingToggleButtonState = true;
                    toggleButton.IsChecked(getIsChecked(selection));
                }

                commandsList.Append(toggleButton);
            }
        };
        
    winrt::FlyoutBase proofingFlyout{ nullptr };
    
    if (auto textBoxTarget = safe_try_cast<winrt::ITextBox8>(Target()))
    {
        proofingFlyout = textBoxTarget.ProofingMenuFlyout();
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::IRichEditBox8>(Target()))
    {
        proofingFlyout = richEditBoxTarget.ProofingMenuFlyout();
    }
    
    winrt::MenuFlyout proofingMenuFlyout = safe_try_cast<winrt::MenuFlyout>(proofingFlyout);
    
    bool shouldIncludeProofingMenu =
        static_cast<bool>(proofingFlyout) &&
        (!proofingMenuFlyout || proofingMenuFlyout.Items().Size() > 0);
        
    if (shouldIncludeProofingMenu)
    {
        m_proofingButton = winrt::AppBarButton{};
        m_proofingButton.Label(ResourceAccessor::GetLocalizedStringResource(SR_ProofingMenuItemLabel));
        m_proofingButton.Flyout(proofingFlyout);

        m_proofingButtonLoadedRevoker = m_proofingButton.Loaded(winrt::auto_revoke,
            [this](auto const&, auto const&)
            {
                // If we have a proofing menu, we'll start with it open by invoking the button
                // as soon as the CommandBar opening animation completes.
                // We invoke the button instead of just showing the flyout to make the button
                // properly update its visual state as well.
                // If we have an open animation that we'll be executing, we'll postpone showing
                // the proofing menu until it's given a chance to get underway.
                // Otherwise, we'll just show the proofing menu immediately.
                if (auto commandBar = winrt::get_self<CommandBarFlyoutCommandBar>(m_commandBar.get()))
                {
                    auto strongThis = get_strong();
                    auto openProofingMenuAction = [strongThis]()
                    {
                        // There isn't likely to be any way that the proofing button was deleted
                        // between us scheduling this action and us actually executing it,
                        // but it doesn't hurt to be resilient when we're scheduling an action
                        // to occur in the future.
                        if (strongThis->m_proofingButton)
                        {
                            winrt::ButtonAutomationPeer peer = safe_cast<winrt::ButtonAutomationPeer>(strongThis->m_proofingButton.OnCreateAutomationPeer());
                            peer.Invoke();
                        }
                    };

                    if (commandBar->HasOpenAnimation() && commandBar->IsOpen())
                    {
                        // Allowing 100 ms to elapse before we open the proofing menu gives the proofing menu enough time
                        // to be mostly open when the proofing menu opens (so the menu doesn't look detached from the CommandBarFlyout UI),
                        // but doesn't wait long enough that there's a noticeable pause in between the user feeling like
                        // the CommandBarFlyout is fully open and the proofing menu finally opening.
                        // The exact number comes from design.
                        SharedHelpers::ScheduleActionAfterWait(openProofingMenuAction, 100);
                    }
                    else
                    {
                        openProofingMenuAction();
                    }
                }
            });

        // We want interactions with any proofing menu element to close the entire flyout,
        // same as interactions with secondary commands, so we'll attach click event handlers
        // to the proofing menu items (if they exist) to handle that.
        if (proofingMenuFlyout)
        {
            m_proofingMenuItemClickRevokers.clear();
            m_proofingMenuToggleItemClickRevokers.clear();

            auto closeFlyoutFunc = [this](auto const& sender, auto const& args) { Hide(); };

            // We might encounter MenuFlyoutSubItems, so we'll add them to this list
            // in order to ensure that we hook up handlers to their entries as well.
            std::list<winrt::IVector<winrt::MenuFlyoutItemBase>> itemsList;
            itemsList.push_back(proofingMenuFlyout.Items());

            while (!itemsList.empty())
            {
                auto currentItems = itemsList.front();
                itemsList.pop_front();

                for (uint32_t i = 0; i < currentItems.Size(); i++)
                {
                    if (auto menuFlyoutItem = safe_try_cast<winrt::MenuFlyoutItem>(currentItems.GetAt(i)))
                    {
                        m_proofingMenuItemClickRevokers.push_back(menuFlyoutItem.Click(winrt::auto_revoke, closeFlyoutFunc));
                    }
                    else if (auto toggleMenuFlyoutItem = safe_try_cast<winrt::ToggleMenuFlyoutItem>(currentItems.GetAt(i)))
                    {
                        m_proofingMenuToggleItemClickRevokers.push_back(toggleMenuFlyoutItem.Click(winrt::auto_revoke, closeFlyoutFunc));
                    }
                    else if (auto menuFlyoutSubItem = safe_try_cast<winrt::MenuFlyoutSubItem>(currentItems.GetAt(i)))
                    {
                        itemsList.push_back(menuFlyoutSubItem.Items());
                    }
                }
            }
        }

        SecondaryCommands().Append(m_proofingButton);
    }
    else
    {
        m_proofingButton = nullptr;
    }

    winrt::IFlyoutBase5 thisAsFlyoutBase5 = *this;

    auto commandListForCutCopyPaste =
        thisAsFlyoutBase5 && thisAsFlyoutBase5.InputDevicePrefersPrimaryCommands() ?
        PrimaryCommands() :
        SecondaryCommands();
    
    addButtonToCommandsIfPresent(TextControlButtons::Cut, commandListForCutCopyPaste);
    addButtonToCommandsIfPresent(TextControlButtons::Copy, commandListForCutCopyPaste);
    addButtonToCommandsIfPresent(TextControlButtons::Paste, commandListForCutCopyPaste);

    addRichEditButtonToCommandsIfPresent(TextControlButtons::Bold, PrimaryCommands(),
        [](winrt::ITextSelection textSelection) { return textSelection.CharacterFormat().Bold() == winrt::FormatEffect::On; });
    addRichEditButtonToCommandsIfPresent(TextControlButtons::Italic, PrimaryCommands(),
        [](winrt::ITextSelection textSelection) { return textSelection.CharacterFormat().Italic() == winrt::FormatEffect::On; });
    addRichEditButtonToCommandsIfPresent(TextControlButtons::Underline, PrimaryCommands(),
        [](winrt::ITextSelection textSelection)
    {
        auto underline = textSelection.CharacterFormat().Underline();
        return (underline != winrt::UnderlineType::None) && (underline != winrt::UnderlineType::Undefined);
    });

    addButtonToCommandsIfPresent(TextControlButtons::Undo, SecondaryCommands());
    addButtonToCommandsIfPresent(TextControlButtons::Redo, SecondaryCommands());
    addButtonToCommandsIfPresent(TextControlButtons::SelectAll, SecondaryCommands());
}

TextControlButtons TextCommandBarFlyout::GetButtonsToAdd()
{
    TextControlButtons buttonsToAdd = TextControlButtons::None;
    auto target = Target();

    if (auto textBoxTarget = safe_try_cast<winrt::TextBox>(target))
    {
        buttonsToAdd = GetTextBoxButtonsToAdd(textBoxTarget);
    }
    else if (auto textBlockTarget = safe_try_cast<winrt::TextBlock>(target))
    {
        buttonsToAdd = GetTextBlockButtonsToAdd(textBlockTarget);
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
    {
        buttonsToAdd = GetRichEditBoxButtonsToAdd(richEditBoxTarget);
    }
    else if (auto richTextBlockTarget = safe_try_cast<winrt::RichTextBlock>(target))
    {
        buttonsToAdd = GetRichTextBlockButtonsToAdd(richTextBlockTarget);
    }
    else if (auto richTextBlockOverflowTarget = safe_try_cast<winrt::RichTextBlockOverflow>(target))
    {
        if (auto richTextBlockSource = richTextBlockOverflowTarget.ContentSource())
        {
            buttonsToAdd = GetRichTextBlockButtonsToAdd(richTextBlockSource);
        }
    }
    else if (auto passwordBoxTarget = safe_try_cast<winrt::PasswordBox>(target))
    {
        buttonsToAdd = GetPasswordBoxButtonsToAdd(passwordBoxTarget);
    }

    return buttonsToAdd;
}

TextControlButtons TextCommandBarFlyout::GetTextBoxButtonsToAdd(winrt::TextBox const& textBox)
{
    TextControlButtons buttonsToAdd = TextControlButtons::None;

    if (!textBox.IsReadOnly())
    {
        if (textBox.SelectionLength() > 0)
        {
            buttonsToAdd |= TextControlButtons::Cut;
        }

        auto textBox8 = textBox.try_as<winrt::ITextBox8>();

        if (textBox8)
        {
            if (textBox8.CanPasteClipboardContent())
            {
                buttonsToAdd |= TextControlButtons::Paste;
            }
        }
        else
        {
            winrt::DataPackageView clipboardContent{ winrt::Clipboard::GetContent() };

            if (clipboardContent.Contains(winrt::StandardDataFormats::Text()))
            {
                buttonsToAdd |= TextControlButtons::Paste;
            }
        }

        // There's no way to polyfill undo and redo - those are black-box operations that we need
        // the TextBox to tell us about.
        if (textBox8 && textBox8.CanUndo())
        {
            buttonsToAdd |= TextControlButtons::Undo;
        }

        if (textBox8 && textBox8.CanRedo())
        {
            buttonsToAdd |= TextControlButtons::Redo;
        }
    }

    if (textBox.SelectionLength() > 0)
    {
        buttonsToAdd |= TextControlButtons::Copy;
    }

    if (textBox.Text().size() > 0)
    {
        buttonsToAdd |= TextControlButtons::SelectAll;
    }

    return buttonsToAdd;
}

TextControlButtons TextCommandBarFlyout::GetTextBlockButtonsToAdd(winrt::TextBlock const& textBlock)
{
    TextControlButtons buttonsToAdd = TextControlButtons::None;

    if (textBlock.SelectedText().size() > 0)
    {
        buttonsToAdd |= TextControlButtons::Copy;
    }

    if (textBlock.Text().size() > 0)
    {
        buttonsToAdd |= TextControlButtons::SelectAll;
    }

    return buttonsToAdd;
}

TextControlButtons TextCommandBarFlyout::GetRichEditBoxButtonsToAdd(winrt::RichEditBox const& richEditBox)
{
    TextControlButtons buttonsToAdd = TextControlButtons::None;
    auto document{ richEditBox.Document() };
    auto selection{ document ? document.Selection() : nullptr };

    if (!richEditBox.IsReadOnly())
    {
        if (auto richEditBox6 = richEditBox.try_as<winrt::IRichEditBox6>())
        {
            auto disabledFormattingAccelerators = richEditBox6.DisabledFormattingAccelerators();

            if ((disabledFormattingAccelerators & winrt::DisabledFormattingAccelerators::Bold) != winrt::DisabledFormattingAccelerators::Bold)
            {
                buttonsToAdd |= TextControlButtons::Bold;
            }

            if ((disabledFormattingAccelerators & winrt::DisabledFormattingAccelerators::Italic) != winrt::DisabledFormattingAccelerators::Italic)
            {
                buttonsToAdd |= TextControlButtons::Italic;
            }

            if ((disabledFormattingAccelerators & winrt::DisabledFormattingAccelerators::Underline) != winrt::DisabledFormattingAccelerators::Underline)
            {
                buttonsToAdd |= TextControlButtons::Underline;
            }
        }
        else
        {
            buttonsToAdd |= TextControlButtons::Bold | TextControlButtons::Italic | TextControlButtons::Underline;
        }

        if (document && document.CanCopy() && selection && selection.Length() != 0)
        {
            buttonsToAdd |= TextControlButtons::Cut;
        }

        if (selection && selection.CanPaste(0))
        {
            buttonsToAdd |= TextControlButtons::Paste;
        }

        if (document && document.CanUndo())
        {
            buttonsToAdd |= TextControlButtons::Undo;
        }

        if (document && document.CanRedo())
        {
            buttonsToAdd |= TextControlButtons::Redo;
        }
    }

    if (document && document.CanCopy() && selection && selection.Length() != 0)
    {
        buttonsToAdd |= TextControlButtons::Copy;
    }

    if (document)
    {
        winrt::hstring text;
        document.GetText(winrt::TextGetOptions::None, text);

        if (text.size() > 0)
        {
            buttonsToAdd |= TextControlButtons::SelectAll;
        }
    }

    return buttonsToAdd;
}

TextControlButtons TextCommandBarFlyout::GetRichTextBlockButtonsToAdd(winrt::RichTextBlock const& richTextBlock)
{
    TextControlButtons buttonsToAdd = TextControlButtons::None;

    if (richTextBlock.SelectedText().size() > 0)
    {
        buttonsToAdd |= TextControlButtons::Copy;
    }

    if (richTextBlock.Blocks().Size() > 0)
    {
        buttonsToAdd |= TextControlButtons::SelectAll;
    }

    return buttonsToAdd;
}

TextControlButtons TextCommandBarFlyout::GetPasswordBoxButtonsToAdd(winrt::PasswordBox const& passwordBox)
{
    TextControlButtons buttonsToAdd = TextControlButtons::None;

    // There isn't any way to get the selection in a PasswordBox, so there's no way to polyfill pasting.
    if (auto passwordBox5 = passwordBox.try_as<winrt::IPasswordBox5>())
    {
        if (passwordBox5.CanPasteClipboardContent())
        {
            buttonsToAdd |= TextControlButtons::Paste;
        }
    }

    if (passwordBox.Password().size() > 0)
    {
        buttonsToAdd |= TextControlButtons::SelectAll;
    }


    return buttonsToAdd;
}

bool TextCommandBarFlyout::IsButtonInPrimaryCommands(TextControlButtons button)
{
    uint32_t buttonIndex = 0;
    bool wasFound = PrimaryCommands().IndexOf(GetButton(button), buttonIndex);
    return wasFound;
}

void TextCommandBarFlyout::ExecuteCutCommand()
{
    auto target = Target();

    if (auto textBoxTarget = safe_try_cast<winrt::TextBox>(target))
    {
        if (auto textBox8 = textBoxTarget.try_as<winrt::ITextBox8>())
        {
            textBox8.CutSelectionToClipboard();
        }
        else
        {
            winrt::DataPackage cutPackage;

            cutPackage.RequestedOperation(winrt::DataPackageOperation::Move);
            cutPackage.SetText(textBoxTarget.SelectedText());

            winrt::Clipboard::SetContent(cutPackage);

            textBoxTarget.SelectedText(L"");
        }
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
    {
#ifdef BUILD_WINDOWS
        if (auto richEditBoxCutCopyPaste = richEditBoxTarget.try_as<winrt::IRichEditBoxFeature_RichEditBoxCutCopyPasteAPIs>())
        {
            richEditBoxCutCopyPaste.CutSelectionToClipboard();
        }
        else
#endif
        {
            auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

            if (selection)
            {
                selection.Cut();
            }
        }
    }

    if (IsButtonInPrimaryCommands(TextControlButtons::Cut))
    {
        UpdateButtons();
    }   
}

void TextCommandBarFlyout::ExecuteCopyCommand()
{
    auto target = Target();
    auto executeRichTextBlockCopyCommand =
        [this](winrt::RichTextBlock const& richTextBlockTarget)
        {
            if (auto richTextBlock6 = richTextBlockTarget.try_as<winrt::IRichTextBlock6>())
            {
                richTextBlock6.CopySelectionToClipboard();
            }
            else
            {
                winrt::DataPackage copyPackage;

                copyPackage.RequestedOperation(winrt::DataPackageOperation::Copy);
                copyPackage.SetText(richTextBlockTarget.SelectedText());

                winrt::Clipboard::SetContent(copyPackage);
            }
        };

    if (auto textBoxTarget = safe_try_cast<winrt::TextBox>(target))
    {
        if (auto textBox8 = textBoxTarget.try_as<winrt::ITextBox8>())
        {
            textBox8.CopySelectionToClipboard();
        }
        else
        {
            winrt::DataPackage copyPackage;

            copyPackage.RequestedOperation(winrt::DataPackageOperation::Copy);
            copyPackage.SetText(textBoxTarget.SelectedText());

            winrt::Clipboard::SetContent(copyPackage);
        }
    }
    else if (auto textBlockTarget = safe_try_cast<winrt::TextBlock>(target))
    {
        if (auto textBlock7 = textBlockTarget.try_as<winrt::ITextBlock7>())
        {
            textBlock7.CopySelectionToClipboard();
        }
        else
        {
            winrt::DataPackage copyPackage;

            copyPackage.RequestedOperation(winrt::DataPackageOperation::Copy);
            copyPackage.SetText(textBlockTarget.SelectedText());

            winrt::Clipboard::SetContent(copyPackage);
        }
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
    {
#ifdef BUILD_WINDOWS
        if (auto richEditBoxCutCopyPaste = richEditBoxTarget.try_as<winrt::IRichEditBoxFeature_RichEditBoxCutCopyPasteAPIs>())
        {
            richEditBoxCutCopyPaste.CopySelectionToClipboard();
        }
        else
#endif
        {
            auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

            if (selection)
            {
                selection.Copy();
            }
        }
    }
    else if (auto richTextBlockTarget = safe_try_cast<winrt::RichTextBlock>(target))
    {
        executeRichTextBlockCopyCommand(richTextBlockTarget);
    }
    else if (auto richTextBlockOverflowTarget = safe_try_cast<winrt::RichTextBlockOverflow>(target))
    {
        if (auto richTextBoxSource = richTextBlockOverflowTarget.ContentSource())
        {
            executeRichTextBlockCopyCommand(richTextBoxSource);
        }
    }

    if (IsButtonInPrimaryCommands(TextControlButtons::Copy))
    {
        UpdateButtons();
    }
}

void TextCommandBarFlyout::ExecutePasteCommand()
{
    auto target = Target();

    if (auto textBoxTarget = safe_try_cast<winrt::TextBox>(target))
    {
        if (auto textBox8 = textBoxTarget.try_as<winrt::ITextBox8>())
        {
            textBox8.PasteFromClipboard();
        }
        else
        {
            auto strongThis = get_strong();

            winrt::Clipboard::GetContent().GetTextAsync().Completed(
                winrt::AsyncOperationCompletedHandler<winrt::hstring>([strongThis, textBoxTarget](winrt::IAsyncOperation<winrt::hstring> asyncOperation, winrt::AsyncStatus asyncStatus)
                {
                    if (asyncStatus != winrt::AsyncStatus::Completed)
                    {
                        return;
                    }

                    auto textToPaste = asyncOperation.GetResults();

                    strongThis->m_dispatcherHelper.RunAsync(
                        [strongThis, textBoxTarget, textToPaste]()
                    {
                        textBoxTarget.SelectedText(textToPaste);
                        textBoxTarget.SelectionStart(textBoxTarget.SelectionStart() + textToPaste.size());
                        textBoxTarget.SelectionLength(0);

                        strongThis->UpdateButtons();
                    });
                }));
        }
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
    {
#ifdef BUILD_WINDOWS
        if (auto richEditBoxCutCopyPaste = richEditBoxTarget.try_as<winrt::IRichEditBoxFeature_RichEditBoxCutCopyPasteAPIs>())
        {
            richEditBoxCutCopyPaste.PasteFromClipboard();
        }
        else
#endif
        {
            auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

            if (selection)
            {
                selection.Paste(0);
            }
        }
    }
    else if (auto passwordBoxTarget = safe_try_cast<winrt::PasswordBox>(target))
    {
        passwordBoxTarget.PasteFromClipboard();
    }

    if (IsButtonInPrimaryCommands(TextControlButtons::Paste))
    {
        UpdateButtons();
    }
}

void TextCommandBarFlyout::ExecuteBoldCommand()
{
    if (!m_isSettingToggleButtonState)
    {
        auto target = Target();

        if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
        {
            auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

            if (selection)
            {
                auto characterFormat = selection.CharacterFormat();
                if (characterFormat.Bold() == winrt::FormatEffect::On)
                {
                    characterFormat.Bold(winrt::FormatEffect::Off);
                }
                else
                {
                    characterFormat.Bold(winrt::FormatEffect::On);
                }
            }
        }
    }
}

void TextCommandBarFlyout::ExecuteItalicCommand()
{
    if (!m_isSettingToggleButtonState)
    {
        auto target = Target();

        if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
        {
            auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

            if (selection)
            {
                auto characterFormat = selection.CharacterFormat();
                if (characterFormat.Italic() == winrt::FormatEffect::On)
                {
                    characterFormat.Italic(winrt::FormatEffect::Off);
                }
                else
                {
                    characterFormat.Italic(winrt::FormatEffect::On);
                }
            }
        }
    }
}

void TextCommandBarFlyout::ExecuteUnderlineCommand()
{
    if (!m_isSettingToggleButtonState)
    {
        auto target = Target();

        if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
        {
            auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

            if (selection)
            {
                auto characterFormat = selection.CharacterFormat();
                if (characterFormat.Underline() == winrt::UnderlineType::None || characterFormat.Underline() == winrt::UnderlineType::Undefined)
                {
                    characterFormat.Underline(winrt::UnderlineType::Single);
                }
                else
                {
                    characterFormat.Underline(winrt::UnderlineType::None);
                }
            }
        }
    }
}

void TextCommandBarFlyout::ExecuteUndoCommand()
{
    auto target = Target();

    if (auto textBoxTarget = safe_try_cast<winrt::TextBox>(target))
    {
        textBoxTarget.Undo();
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
    {
        richEditBoxTarget.Document().Undo();
    }

    if (IsButtonInPrimaryCommands(TextControlButtons::Undo))
    {
        UpdateButtons();
    }
}

void TextCommandBarFlyout::ExecuteRedoCommand()
{
    auto target = Target();

    if (auto textBoxTarget = safe_try_cast<winrt::TextBox>(target))
    {
        textBoxTarget.Redo();
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
    {
        richEditBoxTarget.Document().Redo();
    }

    if (IsButtonInPrimaryCommands(TextControlButtons::Redo))
    {
        UpdateButtons();
    }
}

void TextCommandBarFlyout::ExecuteSelectAllCommand()
{
    auto target = Target();

    if (auto textBoxTarget = safe_try_cast<winrt::TextBox>(target))
    {
        textBoxTarget.SelectAll();
    }
    else if (auto textBlockTarget = safe_try_cast<winrt::TextBlock>(target))
    {
        textBlockTarget.SelectAll();
    }
    else if (auto richEditBoxTarget = safe_try_cast<winrt::RichEditBox>(target))
    {
        auto selection{ SharedHelpers::GetRichTextSelection(richEditBoxTarget) };

        if (selection)
        {
            selection.Expand(winrt::TextRangeUnit::Story);
        }
    }
    else if (auto richTextBlockTarget = safe_try_cast<winrt::RichTextBlock>(target))
    {
        richTextBlockTarget.SelectAll();
    }
    else if (auto richTextBlockOverflowTarget = safe_try_cast<winrt::RichTextBlockOverflow>(target))
    {
        if (auto richTextBlockSource = richTextBlockOverflowTarget.ContentSource())
        {
            richTextBlockSource.SelectAll();
        }
    }
    else if (auto passwordBoxTarget = safe_try_cast<winrt::PasswordBox>(target))
    {
        passwordBoxTarget.SelectAll();
    }

    if (IsButtonInPrimaryCommands(TextControlButtons::SelectAll))
    {
        UpdateButtons();
    }
}

winrt::ICommandBarElement TextCommandBarFlyout::GetButton(TextControlButtons textControlButton)
{
    auto foundButton = m_buttons.find(textControlButton);

    if (foundButton != m_buttons.end())
    {
        return foundButton->second;
    }
    else
    {
        switch (textControlButton)
        {
        case TextControlButtons::Cut:
            {
                winrt::AppBarButton button;
                auto executeFunc = [this]() { ExecuteCutCommand(); };

                if (SharedHelpers::IsStandardUICommandAvailable())
                {
                    InitializeButtonWithUICommand(button, winrt::StandardUICommand(winrt::StandardUICommandKind::Cut), executeFunc);
                }
                else
                {
                    InitializeButtonWithProperties(
                        button,
                        SR_TextCommandLabelCut,
                        winrt::Symbol::Cut,
                        SR_TextCommandKeyboardAcceleratorKeyCut,
                        SR_TextCommandDescriptionCut,
                        executeFunc);
                }

                m_buttons[TextControlButtons::Cut] = button;
                return button;
            }
        case TextControlButtons::Copy:
            {
                winrt::AppBarButton button;
                auto executeFunc = [this]() { ExecuteCopyCommand(); };

                if (SharedHelpers::IsStandardUICommandAvailable())
                {
                    InitializeButtonWithUICommand(button, winrt::StandardUICommand(winrt::StandardUICommandKind::Copy), executeFunc);
                }
                else
                {
                    InitializeButtonWithProperties(
                        button,
                        SR_TextCommandLabelCopy,
                        winrt::Symbol::Copy,
                        SR_TextCommandKeyboardAcceleratorKeyCopy,
                        SR_TextCommandDescriptionCopy,
                        executeFunc);
                }

                m_buttons[TextControlButtons::Copy] = button;
                return button;
            }
        case TextControlButtons::Paste:
            {
                winrt::AppBarButton button;
                auto executeFunc = [this]() { ExecutePasteCommand(); };

                if (SharedHelpers::IsStandardUICommandAvailable())
                {
                    InitializeButtonWithUICommand(button, winrt::StandardUICommand(winrt::StandardUICommandKind::Paste), executeFunc);
                }
                else
                {
                    InitializeButtonWithProperties(
                        button,
                        SR_TextCommandLabelPaste,
                        winrt::Symbol::Paste,
                        SR_TextCommandKeyboardAcceleratorKeyPaste,
                        SR_TextCommandDescriptionPaste,
                        executeFunc);
                }

                m_buttons[TextControlButtons::Paste] = button;
                return button;
            }
        // Bold, Italic, and Underline don't have command library commands associated with them,
        // so we'll just unconditionally initialize them with properties.
        case TextControlButtons::Bold:
            {
                winrt::AppBarToggleButton button;
                InitializeButtonWithProperties(
                    button,
                    SR_TextCommandLabelBold,
                    winrt::Symbol::Bold,
                    SR_TextCommandKeyboardAcceleratorKeyBold,
                    SR_TextCommandDescriptionBold,
                    [this]() { ExecuteBoldCommand(); });

                m_buttons[TextControlButtons::Bold] = button;
                return button;
            }
        case TextControlButtons::Italic:
            {
                winrt::AppBarToggleButton button;
                InitializeButtonWithProperties(
                    button,
                    SR_TextCommandLabelItalic,
                    winrt::Symbol::Italic,
                    SR_TextCommandKeyboardAcceleratorKeyItalic,
                    SR_TextCommandDescriptionItalic,
                    [this]() { ExecuteItalicCommand(); });

                m_buttons[TextControlButtons::Italic] = button;
                return button;
            }
        case TextControlButtons::Underline:
            {
                winrt::AppBarToggleButton button;
                InitializeButtonWithProperties(
                    button,
                    SR_TextCommandLabelUnderline,
                    winrt::Symbol::Underline,
                    SR_TextCommandKeyboardAcceleratorKeyUnderline,
                    SR_TextCommandDescriptionUnderline,
                    [this]() { ExecuteUnderlineCommand(); });

                m_buttons[TextControlButtons::Underline] = button;
                return button;
            }
        case TextControlButtons::Undo:
            {
                winrt::AppBarButton button;
                auto executeFunc = [this]() { ExecuteUndoCommand(); };

                if (SharedHelpers::IsStandardUICommandAvailable())
                {
                    InitializeButtonWithUICommand(button, winrt::StandardUICommand(winrt::StandardUICommandKind::Undo), executeFunc);
                }
                else
                {
                    InitializeButtonWithProperties(
                        button,
                        SR_TextCommandLabelUndo,
                        winrt::Symbol::Undo,
                        SR_TextCommandKeyboardAcceleratorKeyUndo,
                        SR_TextCommandDescriptionUndo,
                        executeFunc);
                }

                m_buttons[TextControlButtons::Undo] = button;
                return button;
            }
        case TextControlButtons::Redo:
            {
                winrt::AppBarButton button;
                auto executeFunc = [this]() { ExecuteRedoCommand(); };

                if (SharedHelpers::IsStandardUICommandAvailable())
                {
                    InitializeButtonWithUICommand(button, winrt::StandardUICommand(winrt::StandardUICommandKind::Redo), executeFunc);
                }
                else
                {
                    InitializeButtonWithProperties(
                        button,
                        SR_TextCommandLabelRedo,
                        winrt::Symbol::Redo,
                        SR_TextCommandKeyboardAcceleratorKeyRedo,
                        SR_TextCommandDescriptionRedo,
                        executeFunc);
                }

                m_buttons[TextControlButtons::Redo] = button;
                return button;
            }
        case TextControlButtons::SelectAll:
            {
                winrt::AppBarButton button;
                auto executeFunc = [this]() { ExecuteSelectAllCommand(); };

                if (SharedHelpers::IsStandardUICommandAvailable())
                {
                    auto command = winrt::StandardUICommand(winrt::StandardUICommandKind::SelectAll);
                    command.IconSource(nullptr);

                    InitializeButtonWithUICommand(button, command, executeFunc);
                }
                else
                {
                    InitializeButtonWithProperties(
                        button,
                        SR_TextCommandLabelSelectAll,
                        SR_TextCommandKeyboardAcceleratorKeySelectAll,
                        SR_TextCommandDescriptionSelectAll,
                        executeFunc);
                }

                m_buttons[TextControlButtons::SelectAll] = button;
                return button;
            }
        default:
            MUX_ASSERT(false);
            return nullptr;
        }
    }
}
