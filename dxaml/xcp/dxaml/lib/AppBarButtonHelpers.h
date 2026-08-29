// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Contains helper methods that centralize functionality common to both
//      AppBarButton and AppBarToggleButton, which we need since
//      the two types are unrelated to each other in terms of class hierarchy.

#pragma once

#include "AppBarButton.g.h"
#include "AppBarButtonTemplateSettings.g.h"
#include "AppBarToggleButton.g.h"
#include "AppBarToggleButtonTemplateSettings.g.h"
#include "CommandBar.g.h"
#include "KeyboardAccelerator.g.h"
#include "TextBlock.g.h"
#include "localizedResource.h"
#include "CommandingHelpers.h"

namespace DirectUI
{
    template <typename AppBarButtonType>
    struct TemplateSettingsAssociation
    {
        using TemplateSettingsInterfaceType = void;
        using TemplateSettingsImplementationType = void;
    };

    template<> struct TemplateSettingsAssociation<AppBarButton>
    {
        using TemplateSettingsInterfaceType = xaml_primitives::IAppBarButtonTemplateSettings;
        using TemplateSettingsImplementationType = AppBarButtonTemplateSettings;
    };

    template<> struct TemplateSettingsAssociation<AppBarToggleButton>
    {
        using TemplateSettingsInterfaceType = xaml_primitives::IAppBarToggleButtonTemplateSettings;
        using TemplateSettingsImplementationType = AppBarToggleButtonTemplateSettings;
    };
    
    template <typename AppBarButtonType>
    using TemplateSettingsInterfaceType = typename TemplateSettingsAssociation<AppBarButtonType>::TemplateSettingsInterfaceType;
    
    template <typename AppBarButtonType>
    using TemplateSettingsImplementationType = typename TemplateSettingsAssociation<AppBarButtonType>::TemplateSettingsImplementationType;
        
    class AppBarButtonHelpers
    {
    public:
        template<class AppBarButtonType>
        static KnownPropertyIndex GetIsCompactDependencyProperty();
        
        template<class AppBarButtonType>
        static KnownPropertyIndex GetUseOverflowStyleDependencyProperty();
        
        template<class AppBarButtonType>
        static KnownPropertyIndex GetLabelPositionDependencyProperty();
        
        template<class AppBarButtonType>
        static KnownPropertyIndex GetLabelDependencyProperty();
        
        template<class AppBarButtonType>
        static KnownPropertyIndex GetIconDependencyProperty();
        
        template<class AppBarButtonType>
        static KnownPropertyIndex GetKeyboardAcceleratorTextDependencyProperty();

        template<class AppBarButtonType>
        static _Check_return_ HRESULT OnBeforeApplyTemplate(_In_ AppBarButtonType* const button)
        {
            if (button->m_isTemplateApplied)
            {
                IFC_RETURN(button->StopAnimationForWidthAdjustments());
                button->m_isTemplateApplied = false;
            }
            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT OnApplyTemplate(_In_ AppBarButtonType* const button)
        {
            ctl::ComPtr<xaml_controls::ITextBlock> keyboardAcceleratorTextLabel;
            IFC_RETURN(button->template GetTemplatePart<xaml_controls::ITextBlock>(STR_LEN_PAIR(L"KeyboardAcceleratorTextLabel"), keyboardAcceleratorTextLabel.ReleaseAndGetAddressOf()));
            button->SetPtrValue(button->m_tpKeyboardAcceleratorTextLabel, keyboardAcceleratorTextLabel.Get());

            button->m_isTemplateApplied = true;

            // Set the initial view state
            IFC_RETURN(button->UpdateInternalStyles());
            IFC_RETURN(button->UpdateVisualState());

            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT CloseSubMenusOnPointerEntered(_In_ AppBarButtonType* const button, _In_opt_ xaml_controls::ISubMenuOwner* pMenuToLeaveOpen)
        {
            BOOLEAN isInOverflow = FALSE;
            IFC_RETURN(button->get_IsInOverflow(&isInOverflow));

            if (isInOverflow)
            {
                // If there are other buttons that have open sub-menus, then we should
                // close those on a delay, since they no longer have mouse-over.
                ctl::ComPtr<ICommandBar> parentCommandBar;
                IFC_RETURN(CommandBar::FindParentCommandBarForElement(button, &parentCommandBar));

                if (parentCommandBar)
                {
                    IFC_RETURN(parentCommandBar.Cast<CommandBar>()->CloseSubMenus(pMenuToLeaveOpen, true /* closeOnDelay */));
                }
            }
            
            return S_OK;
        }
        
        template<class AppBarButtonType>
        static _Check_return_ HRESULT OnPropertyChanged(_In_ AppBarButtonType* const button, _In_ const PropertyChangedParams& args)
        {
            if (args.m_pDP->GetIndex() == GetIsCompactDependencyProperty<AppBarButtonType>() ||
                args.m_pDP->GetIndex() == GetUseOverflowStyleDependencyProperty<AppBarButtonType>() ||
                args.m_pDP->GetIndex() == GetLabelPositionDependencyProperty<AppBarButtonType>())
            {
                IFC_RETURN(button->UpdateInternalStyles());
                IFC_RETURN(button->UpdateVisualState());
            }

            if (args.m_pDP->GetIndex() == KnownPropertyIndex::ToolTipService_ToolTip)
            {
                ctl::ComPtr<IInspectable> boxedToolTipValue;
                
                IFC_RETURN(button->GetValue(
                    MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTipService_ToolTip),
                    &boxedToolTipValue)
                    );

                if (boxedToolTipValue)
                {
                    button->m_ownsToolTip = false;
                }
                else
                {
                    button->m_ownsToolTip = true;
                }
            }

            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT ChangeCommonVisualStates(_In_ AppBarButtonType* const button, _In_ bool useTransitions)
        {
            BOOLEAN ignored = FALSE;
            BOOLEAN isCompact = FALSE;
            BOOLEAN useOverflowStyle = FALSE;
            xaml_controls::CommandBarDefaultLabelPosition effectiveLabelPosition = xaml_controls::CommandBarDefaultLabelPosition_Bottom;
            bool isKeyboardPresent = 0;
        
            IFC_RETURN(button->get_UseOverflowStyle(&useOverflowStyle));
            IFC_RETURN(button->get_IsCompact(&isCompact));
            IFC_RETURN(button->GetEffectiveLabelPosition(&effectiveLabelPosition));
            
            // We only care about finding if we have a keyboard if we also have a menu item with keyboard accelerator text,
            // since if we don't have any menu items with keyboard accelerator text, we won't be showing any that text anyway.
            if (button->m_isWithKeyboardAcceleratorText)
            {
                isKeyboardPresent = DXamlCore::GetCurrent()->GetIsKeyboardPresent();
            }
        
            if (!useOverflowStyle)
            {
                if (effectiveLabelPosition == xaml_controls::CommandBarDefaultLabelPosition_Right)
                {
                    IFC_RETURN(button->GoToState(useTransitions, L"LabelOnRight", &ignored));
                }
                else if (effectiveLabelPosition == xaml_controls::CommandBarDefaultLabelPosition_Collapsed)
                {
                    IFC_RETURN(button->GoToState(useTransitions, L"LabelCollapsed", &ignored));
                }
                else if (isCompact)
                {
                    IFC_RETURN(button->GoToState(useTransitions, L"Compact", &ignored));
                }
                else
                {
                    IFC_RETURN(button->GoToState(useTransitions, L"FullSize", &ignored));
                }
            }
        
            IFC_RETURN(button->GoToState(useTransitions, L"InputModeDefault", &ignored));
            if (button->m_inputDeviceTypeUsedToOpenOverflow == DirectUI::InputDeviceType::Touch)
            {
                IFC_RETURN(button->GoToState(useTransitions, L"TouchInputMode", &ignored));
            }
            else if (button->m_inputDeviceTypeUsedToOpenOverflow == DirectUI::InputDeviceType::GamepadOrRemote)
            {
                IFC_RETURN(button->GoToState(useTransitions, L"GameControllerInputMode", &ignored));
            }

            // We'll make the keyboard accelerator text visible if any element in the overflow has keyboard accelerator text,
            // as this causes the margin to be applied which reserves space, ensuring that keyboard accelerator text
            // in one button won't be at the same horizontal position as label text in another button.
            if (button->m_isWithKeyboardAcceleratorText && isKeyboardPresent && useOverflowStyle)
            {
                IFC_RETURN(button->GoToState(useTransitions, L"KeyboardAcceleratorTextVisible", &ignored));
            }
            else
            {
                IFC_RETURN(button->GoToState(useTransitions, L"KeyboardAcceleratorTextCollapsed", &ignored));
            }
        
            return S_OK;
        }
        
        template<class AppBarButtonType>
        static _Check_return_ HRESULT OnCommandChanged(_In_ AppBarButtonType* const button, _In_  IInspectable* pOldValue, _In_ IInspectable* pNewValue)
        {
            if (pOldValue)
            {
                ctl::ComPtr<ICommand> oldCommand;
                IFC_RETURN(ctl::do_query_interface(oldCommand, pOldValue));
                
                ctl::ComPtr<IXamlUICommand> oldCommandAsUICommand = oldCommand.AsOrNull<IXamlUICommand>();

                if (oldCommandAsUICommand)
                {
                    IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), button, GetLabelDependencyProperty<AppBarButtonType>()));
                    IFC_RETURN(CommandingHelpers::ClearBindingIfSet(oldCommandAsUICommand.Get(), button, GetIconDependencyProperty<AppBarButtonType>()));
                }
            }
            
            if (pNewValue)
            {
                ctl::ComPtr<ICommand> newCommand;

                IFC_RETURN(ctl::do_query_interface(newCommand, pNewValue));
                ctl::ComPtr<IXamlUICommand> newCommandAsUICommand = newCommand.AsOrNull<IXamlUICommand>();

                if (newCommandAsUICommand)
                {
                    // The call to ButtonBase::OnCommandChanged() will have set the Content property, which we don't want -
                    // it's not used anywhere in AppBar*Button, and having it be set can cause problems if an AppBarButton
                    // has a ContentPresenter with a null Content property in its template, as that will be caused to pick up
                    // the parent ContentControl's Content property if one exists.
                    IFC_RETURN(CommandingHelpers::ClearBindingIfSet(newCommandAsUICommand.Get(), button, KnownPropertyIndex::ContentControl_Content));
                    
                    IFC_RETURN(CommandingHelpers::BindToLabelPropertyIfUnset(newCommandAsUICommand.Get(), button, GetLabelDependencyProperty<AppBarButtonType>()));
                    IFC_RETURN(CommandingHelpers::BindToIconPropertyIfUnset(newCommandAsUICommand.Get(), button, GetIconDependencyProperty<AppBarButtonType>()));
                }
            }
            
            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT UpdateTemplateSettings(_In_ AppBarButtonType* const button, _In_ double maxKeyboardAcceleratorTextWidth)
        {
            if (button->m_maxKeyboardAcceleratorTextWidth != maxKeyboardAcceleratorTextWidth)
            {
                button->m_maxKeyboardAcceleratorTextWidth = maxKeyboardAcceleratorTextWidth;
                
                ctl::ComPtr<TemplateSettingsInterfaceType<AppBarButtonType>> templateSettings;
                IFC_RETURN(button->get_TemplateSettings(&templateSettings));

                if (!templateSettings)
                {
                    ctl::ComPtr<TemplateSettingsImplementationType<AppBarButtonType>> templateSettingsImplementation;
                    IFC_RETURN(ctl::make(&templateSettingsImplementation));
                    IFC_RETURN(button->put_TemplateSettings(templateSettingsImplementation.Get()));
                    templateSettings = templateSettingsImplementation;
                }

                IFC_RETURN(templateSettings.template Cast<TemplateSettingsImplementationType<AppBarButtonType>>()->put_KeyboardAcceleratorTextMinWidth(button->m_maxKeyboardAcceleratorTextWidth));
            }
            
            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT UpdateToolTip(_In_ AppBarButtonType* const button)
        {
            if (button->m_ownsToolTip)
            {
                BOOLEAN useOverflowStyle = FALSE;
                wrl_wrappers::HString keyboardAcceleratorText;
                
                IFC_RETURN(button->get_UseOverflowStyle(&useOverflowStyle));
                IFC_RETURN(button->get_KeyboardAcceleratorTextOverride(keyboardAcceleratorText.ReleaseAndGetAddressOf()));

                if (!useOverflowStyle && !keyboardAcceleratorText.IsEmpty())
                {
                    // If we're in the primary section of the app bar or command bar and have accelerator text,
                    // then we should give ourselves a tool tip showing the label plus the accelerator text.
                    wrl_wrappers::HString labelText;

                    IFC_RETURN(button->get_Label(labelText.ReleaseAndGetAddressOf()));
                    
                    wrl_wrappers::HString toolTipFormatString;
                    IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(KEYBOARD_ACCELERATOR_TEXT_TOOLTIP, toolTipFormatString.ReleaseAndGetAddressOf()));
                    
                    WCHAR buffer[1024];
                    IFCEXPECT_RETURN(swprintf_s(
                        buffer, ARRAYSIZE(buffer),
                        toolTipFormatString.GetRawBuffer(nullptr),
                        labelText.GetRawBuffer(nullptr),
                        keyboardAcceleratorText.GetRawBuffer(nullptr)) >= 0);
                    
                    wrl_wrappers::HString toolTipString;
                    IFC_RETURN(toolTipString.Set(buffer));

                    ctl::ComPtr<IInspectable> boxedToolTipString;
                    IFC_RETURN(PropertyValue::CreateFromString(toolTipString.Get(), &boxedToolTipString));

                    IFC_RETURN(button->SetValue(
                        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTipService_ToolTip),
                        boxedToolTipString.Get())
                        );
                }
                else
                {
                    IFC_RETURN(button->ClearValue(
                        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ToolTipService_ToolTip))
                        );
                }

                // Setting the value of ToolTipService.ToolTip causes us to flag us as no longer owning the tool tip,
                // since that's the code path that an app setting the value will also take.
                // In order to ensure that we know that we still own the tool tip, we'll set this value to true here.
                button->m_ownsToolTip = true;
            }

            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT GetKeyboardAcceleratorTextDesiredSize(_In_ AppBarButtonType* const button, _Out_ wf::Size* desiredSize)
        {
            *desiredSize = { 0, 0 };

            if (button->m_tpKeyboardAcceleratorTextLabel)
            {
                xaml::Thickness margin;
                
                IFC_RETURN(button->m_tpKeyboardAcceleratorTextLabel.template Cast<TextBlock>()->Measure({ static_cast<FLOAT>(DoubleUtil::PositiveInfinity), static_cast<FLOAT>(DoubleUtil::PositiveInfinity) }));
                IFC_RETURN(button->m_tpKeyboardAcceleratorTextLabel.template Cast<TextBlock>()->get_DesiredSize(desiredSize));
                IFC_RETURN(button->m_tpKeyboardAcceleratorTextLabel.template Cast<TextBlock>()->get_Margin(&margin));

                desiredSize->Width -= static_cast<float>(margin.Left + margin.Right);
                desiredSize->Height -= static_cast<float>(margin.Top + margin.Bottom);
            }
            
            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT GetKeyboardAcceleratorText(_In_ AppBarButtonType* const button, _Outptr_result_maybenull_ HSTRING *keyboardAcceleratorText)
        {
            IFC_RETURN(button->GetValueByKnownIndex(GetKeyboardAcceleratorTextDependencyProperty<AppBarButtonType>(), keyboardAcceleratorText));

            // If we have no keyboard accelerator text already provided by the app,
            // then we'll see if we can construct it ourselves based on keyboard accelerators
            // set on this item.  For example, if a keyboard accelerator with key "S" and modifier "Control"
            // is set, then we'll convert that into the keyboard accelerator text "Ctrl+S".
            if (*keyboardAcceleratorText == nullptr)
            {
                IFC_RETURN(KeyboardAccelerator::GetStringRepresentationForUIElement(button, keyboardAcceleratorText));

                // If we were able to get a string representation from keyboard accelerators,
                // then we should now set that as the value of KeyboardAcceleratorText.
                if (*keyboardAcceleratorText != nullptr)
                {
                    IFC_RETURN(PutKeyboardAcceleratorText(button, *keyboardAcceleratorText));
                }
            }

            return S_OK;
        }

        template<class AppBarButtonType>
        static _Check_return_ HRESULT PutKeyboardAcceleratorText(_In_ AppBarButtonType* const button, _In_opt_ HSTRING keyboardAcceleratorText)
        {
            IFC_RETURN(button->SetValueByKnownIndex(GetKeyboardAcceleratorTextDependencyProperty<AppBarButtonType>(), keyboardAcceleratorText));
            return S_OK;
        }

        template<class AppBarButtonType>
        static bool IsKeyboardFocusable(_In_ AppBarButtonType* const button)
        {
            // If an AppBarButton is in a CommandBar, then it's keyboard focusable even if it's not a tab stop,
            // so we want to exclude that from the conditions we check in uielement.cpp.
            CFrameworkElement* coreButtonType = do_pointer_cast<CFrameworkElement>(button->GetHandle());

            return coreButtonType->IsActive() &&
                coreButtonType->IsVisible() &&
                (coreButtonType->IsEnabled() || coreButtonType->AllowFocusWhenDisabled()) &&
                coreButtonType->AreAllAncestorsVisible();
        }
    };
}
