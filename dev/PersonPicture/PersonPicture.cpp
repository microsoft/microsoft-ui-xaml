// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InitialsGenerator.h"
#include "PersonPicture.h"
#include "PersonPictureAutomationPeer.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "RuntimeProfiler.h"
#include "PersonPictureTemplateSettings.h"

PersonPicture::PersonPicture()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_PersonPicture);

    SetDefaultStyleKey(this);

    TemplateSettings(winrt::make<PersonPictureTemplateSettings>());

    Unloaded({ this, &PersonPicture::OnUnloaded });
    SizeChanged({ this, &PersonPicture::OnSizeChanged });
}

void PersonPicture::LoadImageAsync(
    std::shared_ptr<winrt::IRandomAccessStreamReference> thumbStreamReference,
    std::function<void(winrt::BitmapImage)> completedFunction)
{
    com_ptr<PersonPicture> strongThis = get_strong();
    auto operation = thumbStreamReference->OpenReadAsync();

    operation.Completed(
        winrt::AsyncOperationCompletedHandler<winrt::IRandomAccessStreamWithContentType>(
            [strongThis, completedFunction](
                winrt::IAsyncOperation<winrt::IRandomAccessStreamWithContentType> operation,
                winrt::AsyncStatus asyncStatus)
    {
        strongThis->m_dispatcherHelper.RunAsync(
            [strongThis, asyncStatus, completedFunction, operation]()
        {
            winrt::BitmapImage bitmap;

            // Handle the failure case here to ensure we are on the UI thread.
            if (asyncStatus != winrt::AsyncStatus::Completed)
            {
                strongThis->m_profilePictureReadAsync.set(nullptr);
                return;
            }

            try
            {
                bitmap.SetSourceAsync(operation.GetResults()).Completed(
                    winrt::AsyncActionCompletedHandler(
                        [strongThis, completedFunction, bitmap](winrt::IAsyncAction, winrt::AsyncStatus asyncStatus)
                {
                    if (asyncStatus != winrt::AsyncStatus::Completed)
                    {
                        strongThis->m_profilePictureReadAsync.set(nullptr);
                        return;
                    }

                    completedFunction(bitmap);
                    strongThis->m_profilePictureReadAsync.set(nullptr);
                }));
            }
            catch (const winrt::hresult_error &e)
            {
                strongThis->m_profilePictureReadAsync.set(nullptr);

                // Ignore the exception if the image is invalid
                if (e.to_abi() == E_INVALIDARG)
                {
                    return;
                }
                else
                {
                    throw;
                }
            }
        });
    }));

    m_profilePictureReadAsync.set(operation);
}

winrt::AutomationPeer PersonPicture::OnCreateAutomationPeer()
{
    return winrt::make<PersonPictureAutomationPeer>(*this);
}

void PersonPicture::OnApplyTemplate()
{
    // Retrieve pointers to stable controls
    winrt::IControlProtected thisAsControlProtected = *this;

    m_initialsTextBlock.set(GetTemplateChildT<winrt::TextBlock>(L"InitialsTextBlock", thisAsControlProtected));

    m_badgeNumberTextBlock.set(GetTemplateChildT<winrt::TextBlock>(L"BadgeNumberTextBlock", thisAsControlProtected));
    m_badgeGlyphIcon.set(GetTemplateChildT<winrt::FontIcon>(L"BadgeGlyphIcon", thisAsControlProtected));
    m_badgingEllipse.set(GetTemplateChildT<winrt::Ellipse>(L"BadgingEllipse", thisAsControlProtected));
    m_badgingBackgroundEllipse.set(GetTemplateChildT<winrt::Ellipse>(L"BadgingBackgroundEllipse", thisAsControlProtected));

    UpdateBadge();
    UpdateIfReady();
}

winrt::hstring PersonPicture::GetInitials()
{
    if (!Initials().empty())
    {
        return Initials();
    }
    else if (!m_displayNameInitials.empty())
    {
        return m_displayNameInitials;
    }
    else
    {
        return m_contactDisplayNameInitials;
    }
}

winrt::ImageSource PersonPicture::GetImageSource()
{
    if (ProfilePicture())
    {
        return ProfilePicture();
    }
    else
    {
        return m_contactImageSource.get();
    }
}

void PersonPicture::UpdateIfReady()
{
    winrt::hstring initials = GetInitials();
    winrt::ImageSource imageSrc = GetImageSource();

    auto templateSettings = winrt::get_self<PersonPictureTemplateSettings>(TemplateSettings());
    templateSettings->ActualInitials(initials);
    if (imageSrc)
    {
        auto imageBrush = templateSettings->ActualImageBrush();
        if (!imageBrush)
        {
            imageBrush = winrt::ImageBrush{};
            imageBrush.Stretch(winrt::Stretch::UniformToFill);
            templateSettings->ActualImageBrush(imageBrush);
        }

        imageBrush.ImageSource(imageSrc);
    }
    else
    {
        templateSettings->ActualImageBrush(nullptr);
    }

    // If the control is converted to 'Group-mode', we'll clear individual-specific information.
    // When IsGroup evaluates to false, we will restore state.
    if (IsGroup())
    {
        winrt::VisualStateManager::GoToState(*this, L"Group", false);
    }
    else
    {
        if (imageSrc)
        {
            winrt::VisualStateManager::GoToState(*this, L"Photo", false);
        }
        else if (!initials.empty())
        {
            winrt::VisualStateManager::GoToState(*this, L"Initials", false);
        }
        else
        {
            winrt::VisualStateManager::GoToState(*this, L"NoPhotoOrInitials", false);
        }
    }

    UpdateAutomationName();
}

void PersonPicture::UpdateBadge()
{
    if (BadgeImageSource())
    {
        UpdateBadgeImageSource();
    }
    else if (BadgeNumber() != 0)
    {
        UpdateBadgeNumber();
    }
    else if (!BadgeGlyph().empty())
    {
        UpdateBadgeGlyph();
    }
    // No badge properties set, so clear the badge XAML
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NoBadge", false);

        if (auto badgeNumberTextBlock = m_badgeNumberTextBlock.get())
        {
            badgeNumberTextBlock.Text(L"");
        }

        if (auto badgeGlyphIcon = m_badgeGlyphIcon.get())
        {
            badgeGlyphIcon.Glyph(L"");
        }
    }

    UpdateAutomationName();
}

void PersonPicture::UpdateBadgeNumber()
{
    if (!m_badgingEllipse || !m_badgeNumberTextBlock)
    {
        return;
    }

    const int badgeNumber = BadgeNumber();

    if (badgeNumber <= 0)
    {
        winrt::VisualStateManager::GoToState(*this, L"NoBadge", false);
        m_badgeNumberTextBlock.get().Text(L"");
        return;
    }

    // should have badging number to show if we are here
    winrt::VisualStateManager::GoToState(*this, L"BadgeWithoutImageSource", false);

    if (badgeNumber <= 99)
    {
        m_badgeNumberTextBlock.get().Text(std::to_wstring(badgeNumber));
    }
    else
    {
        m_badgeNumberTextBlock.get().Text(L"99+");
    }
}

void PersonPicture::UpdateBadgeGlyph()
{
    if (!m_badgingEllipse || !m_badgeGlyphIcon)
    {
        return;
    }

    winrt::hstring badgeGlyph = BadgeGlyph();

    if (badgeGlyph.empty())
    {
        winrt::VisualStateManager::GoToState(*this, L"NoBadge", false);
        m_badgeGlyphIcon.get().Glyph(L"");
        return;
    }

    // should have badging Glyph to show if we are here
    winrt::VisualStateManager::GoToState(*this, L"BadgeWithoutImageSource", false);

    m_badgeGlyphIcon.get().Glyph(badgeGlyph);
}

void PersonPicture::UpdateBadgeImageSource()
{
    if (m_badgeImageBrush == nullptr)
    {
        m_badgeImageBrush.set(GetTemplateChildT<winrt::ImageBrush>(L"BadgeImageBrush", *this));
    }

    if (!m_badgingEllipse || !m_badgeImageBrush)
    {
        return;
    }

    m_badgeImageBrush.get().ImageSource(BadgeImageSource());

    if (BadgeImageSource())
    {
        winrt::VisualStateManager::GoToState(*this, L"BadgeWithImageSource", false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"NoBadge", false);
    }
}

void PersonPicture::UpdateAutomationName()
{
    winrt::Contact contact = Contact();
    std::wstring automationName;
    std::wstring contactName;

    // The AutomationName for the control is in the format: PersonName, BadgeInformation.
    // PersonName is set based on the name / initial properties in the order below.
    // if none exist, it defaults to "Person"
    if (IsGroup())
    {
        contactName = ResourceAccessor::GetLocalizedStringResource(SR_GroupName);
    }
    else if (contact && !contact.DisplayName().empty())
    {
        contactName = contact.DisplayName();
    }
    else if (!DisplayName().empty())
    {
        contactName = DisplayName();
    }
    else if (!Initials().empty())
    {
        contactName = Initials();
    }
    else
    {
        contactName = ResourceAccessor::GetLocalizedStringResource(SR_PersonName);
    }

    // BadgeInformation portion of the AutomationName is set to 'n items' if there is a BadgeNumber,
    // or 'icon' for BadgeGlyph or BadgeImageSource. If BadgeText is specified, it will override
    // the string 'items' or 'icon'
    if (BadgeNumber() > 0)
    {
        if (!BadgeText().empty())
        {
            automationName = StringUtil::FormatString(
                ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemTextOverride),
                contactName.data(),
                BadgeNumber(),
                BadgeText().data());
        }
        else
        {
            automationName = StringUtil::FormatString(
                GetLocalizedPluralBadgeItemStringResource(BadgeNumber()),
                contactName.data(),
                BadgeNumber());
        }
    }
    else if (!BadgeGlyph().empty() || BadgeImageSource())
    {
        if (!BadgeText().empty())
        {
            automationName = StringUtil::FormatString(
                ResourceAccessor::GetLocalizedStringResource(SR_BadgeIconTextOverride),
                contactName.data(),
                BadgeText().data());
        }
        else
        {
            automationName = StringUtil::FormatString(
                ResourceAccessor::GetLocalizedStringResource(SR_BadgeIcon),
                contactName.data());
        }
    }
    else
    {
        automationName = contactName;
    }

    winrt::AutomationProperties::SetName(*this, automationName);
}

winrt::hstring PersonPicture::GetLocalizedPluralBadgeItemStringResource(unsigned int numericValue)
{
    const UINT32 valueMod10 = numericValue % 10;
    winrt::hstring value;

    if (numericValue == 1)  // Singular
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemSingular);
    }
    else if (numericValue == 2) // 2
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemPlural7);
    }
    else if (numericValue == 3 || numericValue == 4) // 3,4
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemPlural2);
    }
    else if (numericValue >= 5 && numericValue <= 10) // 5-10
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemPlural5);
    }
    else if (numericValue >= 11 && numericValue <= 19) // 11-19
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemPlural6);
    }
    else if (valueMod10 == 1) // 21, 31, 41, etc.
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemPlural1);
    }
    else if (valueMod10 >= 2 && valueMod10 <= 4) // 22-24, 32-34, 42-44, etc.
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemPlural3);
    }
    else // Everything else... 0, 20, 25-30, 35-40, etc.
    {
        value = ResourceAccessor::GetLocalizedStringResource(SR_BadgeItemPlural4);
    }

    return value;
}

void PersonPicture::UpdateControlForContact(bool isNewContact)
{
    winrt::Contact contact = Contact();

    if (!contact)
    {
        // Explicitly setting to empty/nullptr ensures the bound XAML is
        // correctly updated.
        m_contactDisplayNameInitials.set(L"");
        m_contactImageSource.set(nullptr);
        UpdateIfReady();
        return;
    }

    // It's possible for a second update to occur before the first finished loading
    // a profile picture (regardless of second having a picture or not).
    // Cancellation of any previously-activated tasks will mitigate race conditions.
    if (auto profilePictureReadAsync = m_profilePictureReadAsync.get())
    {
        profilePictureReadAsync.Cancel();
    }

    m_contactDisplayNameInitials.set(InitialsGenerator::InitialsFromContactObject(contact));

    // Order of preference (but all work): Large, Small, Source, Thumbnail
    std::shared_ptr<winrt::IRandomAccessStreamReference> thumbStreamReference = std::make_shared<winrt::IRandomAccessStreamReference>();

    if (PreferSmallImage() && contact.SmallDisplayPicture())
    {
        *thumbStreamReference = contact.SmallDisplayPicture();
    }
    else
    {
        if (contact.LargeDisplayPicture())
        {
            *thumbStreamReference = contact.LargeDisplayPicture();
        }
        else if (contact.SmallDisplayPicture())
        {
            *thumbStreamReference = contact.SmallDisplayPicture();
        }
        else if (contact.SourceDisplayPicture())
        {
            *thumbStreamReference = contact.SourceDisplayPicture();
        }
        else if (contact.Thumbnail())
        {
            *thumbStreamReference = contact.Thumbnail();
        }
    }

    // If we have profile picture data available per the above, async load the picture from the platform.
    if (*thumbStreamReference != nullptr)
    {
        if (isNewContact)
        {
            // Prevent the case where context of a contact changes, but we show an old person while the new one is loaded.
            m_contactImageSource.set(nullptr);
        }

        // The dispatcher is not available in design mode, so when in design mode bypass the call to LoadImageAsync.
        if (!SharedHelpers::IsInDesignMode())
        {
            com_ptr<PersonPicture> strongThis = get_strong();

            LoadImageAsync(
                thumbStreamReference,
                [strongThis](winrt::BitmapImage profileBitmap)
            {
                profileBitmap.DecodePixelType(winrt::DecodePixelType::Logical);

                // We want to constrain the shorter side to the same dimension as the control, allowing the decoder to
                // choose the other dimension without distorting the image.
                if (profileBitmap.PixelHeight() < profileBitmap.PixelWidth())
                {
                    profileBitmap.DecodePixelHeight(static_cast<int>(strongThis->Height()));
                }
                else
                {
                    profileBitmap.DecodePixelWidth(static_cast<int>(strongThis->Width()));
                }

                strongThis->m_contactImageSource.set(winrt::ImageSource(profileBitmap));
                strongThis->UpdateIfReady();
            });
        }
    }
    else
    {
        // Else clause indicates that (Contact->Thumbnail == nullptr).
        m_contactImageSource.set(nullptr);
    }

    UpdateIfReady();
}

void PersonPicture::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs &args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_BadgeNumberProperty ||
        property == s_BadgeGlyphProperty ||
        property == s_BadgeImageSourceProperty)
    {
        UpdateBadge();
    }
    else if (property == s_BadgeTextProperty)
    {
        UpdateAutomationName();
    }
    else if (property == s_ContactProperty)
    {
        OnContactChanged(args);
    }
    else if (property == s_DisplayNameProperty)
    {
        OnDisplayNameChanged(args);
    }
    else if (property == s_ProfilePictureProperty ||
        property == s_InitialsProperty ||
        property == s_IsGroupProperty)
    {
        UpdateIfReady();
    }
    // No additional action required for s_PreferSmallImageProperty
}

void PersonPicture::OnDisplayNameChanged(winrt::DependencyPropertyChangedEventArgs const& /* args */)
{
    m_displayNameInitials.set(InitialsGenerator::InitialsFromDisplayName(DisplayName()));

    UpdateIfReady();
}

void PersonPicture::OnContactChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    bool isNewContact = true;

    if (args && args.OldValue() && args.NewValue())
    {
        winrt::Contact oldContact = args.OldValue().as<winrt::Contact>();
        winrt::Contact newContact = args.NewValue().as<winrt::Contact>();

        // Verify that the IDs are not null before comparing the old and new contact object.
        // If both contact IDs are null, it will be treated as a newcontact.
        if (!oldContact.Id().empty() || !newContact.Id().empty())
        {
            isNewContact = oldContact.Id() != newContact.Id();
        }
    }

    UpdateControlForContact(isNewContact);
}

void PersonPicture::OnSizeChanged(winrt::IInspectable const& /*sender*/, const winrt::SizeChangedEventArgs &args)
{
    {
        const bool widthChanged = (args.NewSize().Width != args.PreviousSize().Width);
        const bool heightChanged = (args.NewSize().Height != args.PreviousSize().Height);
        double newSize;

        if (widthChanged && heightChanged)
        {
            // Maintain circle by enforcing the new size on both Width and Height.
            // To do so, we will use the minimum value.
            newSize = (args.NewSize().Width < args.NewSize().Height) ? args.NewSize().Width : args.NewSize().Height;
        }
        else if (widthChanged)
        {
            newSize = args.NewSize().Width;
        }
        else if (heightChanged)
        {
            newSize = args.NewSize().Height;
        }
        else
        {
            return;
        }

        Height(newSize);
        Width(newSize);
    }

    // Calculate the FontSize of the control's text. Design guidelines have specified the
    // font size to be 42% of the container. Since it's circular, 42% of either Width or Height.
    // Note that we cap it to a minimum of 1, since a font size of less than 1 is an invalid value
    // that will result in a failure.
    const double fontSize = std::max(1.0, Width() * .42);

    if (auto initialsTextBlock = m_initialsTextBlock.get())
    {
        initialsTextBlock.FontSize(fontSize);
    }

    if (m_badgingEllipse && m_badgingBackgroundEllipse && m_badgeNumberTextBlock && m_badgeGlyphIcon)
    {
        // Maintain badging circle and font size by enforcing the new size on both Width and Height.
        // Design guidelines have specified the font size to be 60% of the badging plate, and we want to keep 
        // badging plate to be about 50% of the control so that don't block the initial/profile picture.
        const double newSize = (args.NewSize().Width < args.NewSize().Height) ? args.NewSize().Width : args.NewSize().Height;
        m_badgingEllipse.get().Height(newSize * 0.5);
        m_badgingEllipse.get().Width(newSize * 0.5);
        m_badgingBackgroundEllipse.get().Height(newSize * 0.5);
        m_badgingBackgroundEllipse.get().Width(newSize * 0.5);
        m_badgeNumberTextBlock.get().FontSize(std::max(1.0, m_badgingEllipse.get().Height() * 0.6));
        m_badgeGlyphIcon.get().FontSize(std::max(1.0, m_badgingEllipse.get().Height() * 0.6));
    }
}

void PersonPicture::OnUnloaded(winrt::IInspectable const& /*sender*/, winrt::RoutedEventArgs const& /*e*/)
{
    if (auto profilePictureReadAsync = m_profilePictureReadAsync.get())
    {
        profilePictureReadAsync.Cancel();
    }
}
