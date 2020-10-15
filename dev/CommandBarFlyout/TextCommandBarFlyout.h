// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextCommandBarFlyout.g.h"
#include "CommandBarFlyout.h"
#include "DispatcherHelper.h"

enum class TextControlButtons
{
    None = 0x0000,
    Cut = 0x0001,
    Copy = 0x0002,
    Paste = 0x0004,
    Bold = 0x0008,
    Italic = 0x0010,
    Underline = 0x0020,
    Undo = 0x0040,
    Redo = 0x0080,
    SelectAll = 0x0100,
};

DECLARE_FLAG_ENUM_OPERATOR_OVERLOADS(TextControlButtons);

class TextCommandBarFlyout :
    public winrt::implementation::TextCommandBarFlyoutT<TextCommandBarFlyout, CommandBarFlyout>
{
public:
    ForwardRefToBaseReferenceTracker(CommandBarFlyout)

    TextCommandBarFlyout();

private:
    void UpdateButtons();

    TextControlButtons GetButtonsToAdd();
    static TextControlButtons GetTextBoxButtonsToAdd(winrt::TextBox const& textBox);
    static TextControlButtons GetTextBlockButtonsToAdd(winrt::TextBlock const& textBlock);
    static TextControlButtons GetRichEditBoxButtonsToAdd(winrt::RichEditBox const& richEditBox);
    static TextControlButtons GetRichTextBlockButtonsToAdd(winrt::RichTextBlock const& richTextBlock);
    static TextControlButtons GetPasswordBoxButtonsToAdd(winrt::PasswordBox const& passwordBox);

    bool IsButtonInPrimaryCommands(TextControlButtons button);

    void InitializeButtonWithUICommand(
        winrt::ButtonBase const& button,
        winrt::XamlUICommand const& uiCommand,
        std::function<void()> const& executeFunc);

    void InitializeButtonWithProperties(
        winrt::ButtonBase const& button,
        ResourceIdType labelResourceId,
        ResourceIdType acceleratorKeyResourceId,
        ResourceIdType descriptionResourceId,
        std::function<void()> const& executeFunc);

    void InitializeButtonWithProperties(
        winrt::ButtonBase const& button,
        ResourceIdType labelResourceId,
        winrt::Symbol const& symbol,
        ResourceIdType acceleratorKeyResourceId,
        ResourceIdType descriptionResourceId,
        std::function<void()> const& executeFunc);

    void ExecuteCutCommand();
    void ExecuteCopyCommand();
    void ExecutePasteCommand();
    void ExecuteBoldCommand();
    void ExecuteItalicCommand();
    void ExecuteUnderlineCommand();
    void ExecuteUndoCommand();
    void ExecuteRedoCommand();
    void ExecuteSelectAllCommand();

    winrt::ICommandBarElement GetButton(TextControlButtons button);

    std::map<TextControlButtons, winrt::ICommandBarElement> m_buttons;
    winrt::AppBarButton m_proofingButton{ nullptr };

    std::vector<winrt::XamlUICommand::ExecuteRequested_revoker> m_buttonCommandRevokers;
    std::vector<winrt::ButtonBase::Click_revoker> m_buttonClickRevokers;
    std::vector<winrt::ToggleButton::Checked_revoker> m_toggleButtonCheckedRevokers;
    std::vector<winrt::ToggleButton::Unchecked_revoker> m_toggleButtonUncheckedRevokers;

    winrt::FrameworkElement::Loaded_revoker m_proofingButtonLoadedRevoker{};

    std::vector<winrt::MenuFlyoutItem::Click_revoker> m_proofingMenuItemClickRevokers;
    std::vector<winrt::ToggleMenuFlyoutItem::Click_revoker> m_proofingMenuToggleItemClickRevokers;
    DispatcherHelper m_dispatcherHelper{ *this };

    bool m_isSettingToggleButtonState = false;
};

