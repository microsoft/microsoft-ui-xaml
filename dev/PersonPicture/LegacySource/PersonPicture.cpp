// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// PersonPicture.cpp
// Implementation of the PersonPicture class.

#include "pch.h"
#include "PersonPicture.h"
#include "PersonPictureAutomationPeer.h"
#include "Utilities\InitialsGenerator.h"
#include "Utilities\ResourceAccessor.h"
#include "Utilities\Utils.h"

using namespace Platform;
using namespace Concurrency;
using namespace Windows::ApplicationModel::Contacts;
using namespace Windows::ApplicationModel::Resources;
using namespace Windows::Foundation;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Automation;
using namespace Windows::UI::Xaml::Automation::Peers;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Shapes;
using namespace Microsoft::People::Controls;

DependencyProperty^ PersonPicture::_TargetContact = DependencyProperty::Register(
    "TargetContact",
    Windows::ApplicationModel::Contacts::Contact::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        nullptr,
        ref new PropertyChangedCallback(_OnTargetContactChanged)));

DependencyProperty^ PersonPicture::_ProfilePicture = DependencyProperty::Register(
    "ProfilePicture",
    Windows::UI::Xaml::Media::ImageSource::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        nullptr,
        ref new PropertyChangedCallback(_OnPersonDataChanged)));

DependencyProperty^ PersonPicture::_PersonInitials = DependencyProperty::Register(
    "PersonInitials",
    Platform::String::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        nullptr,
        ref new PropertyChangedCallback(_OnPersonDataChanged)));

DependencyProperty^ PersonPicture::_PersonDisplayName = DependencyProperty::Register(
    "PersonDisplayName",
    Platform::String::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        nullptr,
        ref new PropertyChangedCallback(_OnDisplayNameChanged)));

DependencyProperty^ PersonPicture::_ShowingGroupGlyph = DependencyProperty::Register(
    "ShowingGroupGlyph",
    bool::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        false,
        ref new PropertyChangedCallback(_OnGlyphStateChanged)));

DependencyProperty^ PersonPicture::_PreferSmallImage = DependencyProperty::Register(
    "PreferSmallImage",
    bool::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        false,
        ref new PropertyChangedCallback(_OnPreferSmallImageChanged)));

DependencyProperty^ PersonPicture::_BadgingNumber = DependencyProperty::Register(
    "BadgingNumber",
    int::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        nullptr,
        ref new PropertyChangedCallback(_OnBadgingNumberChanged)));

DependencyProperty^ PersonPicture::_BadgingGlyph = DependencyProperty::Register(
    "BadgingGlyph",
    Platform::String::typeid,
    PersonPicture::typeid,
    ref new PropertyMetadata(
        nullptr,
        ref new PropertyChangedCallback(_OnBadgingGlyphChanged)));

task<BitmapImage^> LoadImageAsync(
    IRandomAccessStreamReference^ thumbStreamReference,
    int width,
    int height,
    cancellation_token cancelToken)
{
    BitmapImage^ profileBitmap = ref new BitmapImage();

    // Attach the image open event to get the size of the image before the final decode.
    EventRegistrationToken imageOpenCookie = profileBitmap->ImageOpened += ref new RoutedEventHandler(
        [height, width](Object^ sender, RoutedEventArgs^ e)
        {
            BitmapImage^ profileBitmap = static_cast<BitmapImage^>(sender);

            // The height and width of the control are in logical pixels, let the decoder know that.
            profileBitmap->DecodePixelType = DecodePixelType::Logical;

            // We want to constrain the shorter side to the same dimension as the control, allowing the decoder to
            // choose the other dimension without distorting the image.
            if (profileBitmap->PixelHeight < profileBitmap->PixelWidth)
            {
                profileBitmap->DecodePixelHeight = height;
            }
            else
            {
                profileBitmap->DecodePixelWidth = width;
            }
        });

    return create_task(thumbStreamReference->OpenReadAsync(), cancelToken)
        .then([profileBitmap, cancelToken](IRandomAccessStreamWithContentType^ stream)
        {
            return create_task(profileBitmap->SetSourceAsync(stream), cancelToken);
        })
        .then([profileBitmap, imageOpenCookie]()
        {
            // Detach the event as it's no longer needed. It's not strictly necessary
            // given that this is a weak reference, but it's good practice before
            // we return the object.
            profileBitmap->ImageOpened -= imageOpenCookie;

            return profileBitmap;
        });
}

PersonPicture::PersonPicture() :
    m_preferSmallImage(false)
{
    DefaultStyleKey = "Microsoft.People.Controls.PersonPicture";

    SizeChanged += ref new SizeChangedEventHandler(this, &PersonPicture::_OnSizeChanged);
    Unloaded += ref new RoutedEventHandler(this, &PersonPicture::_OnUnloaded);
}

AutomationPeer^ PersonPicture::OnCreateAutomationPeer()
{
    return ref new PersonPictureAutomationPeer(this);
}

void PersonPicture::OnApplyTemplate()
{
    Control::OnApplyTemplate();

    _initialsTextBlock = safe_cast<TextBlock^>(GetTemplateChild(L"PersonInitialsTextBlock"));

    if (_initialsTextBlock != nullptr)
    {
        // Save the original font family setting of textblock
        _personTextFontFamily = _initialsTextBlock->FontFamily;
    }

    _badgingNumberTextBlock = safe_cast<TextBlock^>(GetTemplateChild(L"BadgingNumberTextBlock"));

    _badgingGlyphIcon = safe_cast<FontIcon^>(GetTemplateChild(L"BadgingGlyphIcon"));

    _badgingEllipse = safe_cast<Windows::UI::Xaml::Shapes::Ellipse^>(GetTemplateChild(L"BadgingEllipse"));

    UpdateIfReady();

    if (BadgingNumber > 0)
    {
        UpdateBadgingNumber();
    }
    else if (BadgingGlyph != nullptr)
    {
        UpdateBadgingGlyph();
    }
}

void PersonPicture::UpdateIfReady()
{
    if (_initialsTextBlock != nullptr)
    {
        if ((PersonInitials == nullptr && ProfilePicture == nullptr) || ShowingGroupGlyph)
        {
            // Change the FontFamily of text block to "Segoe MDL2 Assets"
            _initialsTextBlock->FontFamily = ref new Windows::UI::Xaml::Media::FontFamily(L"Segoe MDL2 Assets");

            // If true, change the symbol block to show the Group asset. Otherwise show the Person asset.
            _initialsTextBlock->Text = (ShowingGroupGlyph) ? L"\uE716" : L"\uE77B";
        }
        else
        {
            // Change the FontFamily of text block to original setting
            _initialsTextBlock->FontFamily = _personTextFontFamily;

            // Assign the initials of contact to text block 
            _initialsTextBlock->Text = (ProfilePicture == nullptr) ? PersonInitials : nullptr;
        }
    }

    // If the control is converted to 'Group-mode', we'll clear individual-specific information.
    // When ShowingGroupGlyph evaluates to false, we will restore state.
    if (ShowingGroupGlyph)
    {
        if (_personPictureEllipse != nullptr)
        {
            // change the ellipse brush to original setting
            _personPictureEllipse->Fill = nullptr;
        }
    }
    else
    {
        if (_personPictureEllipse == nullptr && ProfilePicture != nullptr)
        {
            // Realize the control to display the contact picture.
            _personPictureEllipse = safe_cast<Windows::UI::Xaml::Shapes::Ellipse^>(GetTemplateChild(L"PersonPictureEllipse"));
        }

        if (_personPictureEllipse != nullptr)
        {
            if (ProfilePicture != nullptr)
            {
                // Clear the initials for showing the photo
                _initialsTextBlock->Text = nullptr;

                ImageBrush^ imageBrush = ref new ImageBrush();
                imageBrush->Stretch = Windows::UI::Xaml::Media::Stretch::UniformToFill;
                imageBrush->ImageSource = ProfilePicture;

                _personPictureEllipse->Fill = imageBrush;
            }
            else
            {
                _personPictureEllipse->Fill = nullptr;
            }
        }
    }

    UpdateAutomationName();
}

void PersonPicture::UpdateBadgingNumber()
{
    if (_badgingEllipse == nullptr || _badgingNumberTextBlock == nullptr)
    {
        return;
    }

    if (BadgingNumber <= 0)
    {
        _badgingEllipse->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _badgingNumberTextBlock->Text = nullptr;
        return;
    }

    // should have badging number to show if we are here	
    _badgingEllipse->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _badgingGlyphIcon->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _badgingNumberTextBlock->Visibility = Windows::UI::Xaml::Visibility::Visible;

    if (BadgingNumber <= 99)
    {
        _badgingNumberTextBlock->Text = BadgingNumber.ToString();
    }
    else
    {
        int number = 99;
        _badgingNumberTextBlock->Text = number.ToString() + "+";
    }
}

void PersonPicture::UpdateBadgingGlyph()
{
    if (_badgingEllipse == nullptr || _badgingGlyphIcon == nullptr)
    {
        return;
    }

    if (BadgingGlyph == nullptr)
    {
        _badgingEllipse->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _badgingGlyphIcon->Glyph = nullptr;
        return;
    }

    // should have badging Glyph to show if we are here	
    _badgingEllipse->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _badgingNumberTextBlock->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _badgingGlyphIcon->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _badgingGlyphIcon->Glyph = BadgingGlyph;
}

void PersonPicture::UpdateAutomationName()
{
    String^ automationName;
    String^ contactName;

    // Set the automation name to the contact display name if one has been
    // set. If only initials or a picture were specified this will be empty.
    if (this->TargetContact && !this->TargetContact->DisplayName->IsEmpty())
    {
        contactName = this->TargetContact->DisplayName;
    }
    else if (!this->PersonDisplayName->IsEmpty())
    {
        contactName = this->PersonDisplayName;
    }
    else
    {
        contactName = L"";
    }

    // Append the number of badge items or badge glyph being present if needed.
    // Otherwise keep the automation name as the contact name.
    if (BadgingNumber > 0)
    {
        automationName = StringUtil::FormatString(
            ResourceAccessor::GetLocalizedPluralStringResource("BadgeItem", BadgingNumber),
            contactName->Data(),
            BadgingNumber);
    }
    else if (BadgingGlyph != nullptr)
    {
        automationName = StringUtil::FormatString(
            ResourceAccessor::GetLocalizedStringResource("BadgeIcon"),
            contactName->Data());
    }
    else
    {
        automationName = contactName;
    }

    AutomationProperties::SetName(this, automationName);
}

void PersonPicture::UpdateStateFromProfilePicture(ImageSource^ value)
{
    if (value != nullptr)
    {
        VisualStateManager::GoToState(this, "Photo", false);
    }
    else
    {
        VisualStateManager::GoToState(this, "Initials", false);
    }
}

void PersonPicture::UpdateControlForContact(bool isNewContact)
{
    if (TargetContact == nullptr)
    {
        // Explicitly setting to nullptr ensures the bound XAML is
        // correctly updated.
        this->PersonInitials = nullptr;
        this->ProfilePicture = nullptr;
        return;
    }

    // It's possible for a second update to occur before the first finished loading
    // a profile picture (regardless of second having a picture or not).
    // Cancellation of any previously-activated tasks will mitigate race conditions.
    if (_ProfilePictureReadAsync != nullptr)
    {
        _ProfilePictureReadAsync->Cancel();
    }

    this->PersonInitials = InitialsGenerator::InitialsFromContactObject(TargetContact);

    // Order of preference (but all work): Large, Small, Source, Thumbnail
    IRandomAccessStreamReference^ thumbStreamReference;

    if (this->m_preferSmallImage && TargetContact->SmallDisplayPicture != nullptr)
    {
        thumbStreamReference = this->TargetContact->SmallDisplayPicture;
    }
    else
    {
        if (TargetContact->LargeDisplayPicture != nullptr)
        {
            thumbStreamReference = this->TargetContact->LargeDisplayPicture;
        }
        else if (TargetContact->SmallDisplayPicture != nullptr)
        {
            thumbStreamReference = this->TargetContact->SmallDisplayPicture;
        }
        else if (TargetContact->SourceDisplayPicture != nullptr)
        {
            thumbStreamReference = this->TargetContact->SourceDisplayPicture;
        }
        else if (TargetContact->Thumbnail != nullptr)
        {
            thumbStreamReference = this->TargetContact->Thumbnail;
        }
    }

    // If we have profile picture data available per the above, async load the picture from the platform.
    if (thumbStreamReference != nullptr)
    {
        if (isNewContact)
        {
            // Prevent the case where context of a contact changes, but we show an old person while the new one is loaded.
            this->ProfilePicture = nullptr;
        }

        task_completion_event<void> tce;
        cancellation_token_source cancelSource;
        cancellation_token cancelToken = cancelSource.get_token();

        LoadImageAsync(
            thumbStreamReference,
            static_cast<int>(this->Width),
            static_cast<int>(this->Height),
            cancelToken)
            .then([this, cancelToken, tce](task<BitmapImage^> bitmapImageTask)
            {
                try
                {
                    BitmapImage^ bitmapImage = bitmapImageTask.get();

                    if (!cancelToken.is_canceled())
                    {
                        // This continuation will not be captured by the create_async, preventing
                        // a circular reference and memory leak.
                        ProfilePicture = bitmapImage;
                    }

                    tce.set();
                }
                catch (...)
                {
                    tce.set_exception(std::current_exception());
                }
            });

        // We need to make sure that the IAsyncAction doesn't capture "this".
        //
        // In a ref class "this" is a smart pointer which adds a reference when captured.
        // In this case where we are keeping a reference to the action (and by association
        // the lambda), capturing "this" would lead to a circular reference and a memory leak.
        _ProfilePictureReadAsync = create_async(
            [tce, cancelSource](cancellation_token cancelToken)
            {
                // Register for the IAsyncAction's cancel token in order to notify the
                // already-in-progress action that a cancellation has been requested.
                cancellation_token_registration reg = cancelToken.register_callback(
                    [cancelSource]()
                {
                    // Relay the cancellation through to the cancellation token that
                    // is being used by the load operation.
                    cancelSource.cancel();
                });

                return create_task(tce)
                    .then([cancelToken, reg](task<void> resultTask)
                {
                    // Deregister the callback now that the task has completed.
                    cancelToken.deregister_callback(reg);

                    // Throw any exceptions that are being held onto.
                    resultTask.get();
                });
            });
    }
    else
    {
        // Else clause indicates that (TargetContact->Thumbnail == nullptr).
        this->ProfilePicture = nullptr;
    }

    UpdateAutomationName();
}

void PersonPicture::_OnDisplayNameChanged(DependencyObject^ obj, DependencyPropertyChangedEventArgs^ e)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(obj);
    personPicture->PersonInitials = InitialsGenerator::InitialsFromDisplayName(personPicture->PersonDisplayName);
    personPicture->UpdateIfReady();
}

void PersonPicture::_OnPersonDataChanged(DependencyObject^ obj, DependencyPropertyChangedEventArgs^ e)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(obj);
    personPicture->UpdateIfReady();
}

void PersonPicture::_OnTargetContactChanged(DependencyObject^ obj, DependencyPropertyChangedEventArgs^ e)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(obj);

    bool isNewContact = true;
    if ((e != nullptr) && (e->OldValue != nullptr) && (e->NewValue != nullptr))
    {
        Contact^ oldContact = static_cast<Contact^>(e->OldValue);
        Contact^ newContact = static_cast<Contact^>(e->NewValue);

        // Verify that the IDs are not null before comparing the old and new contact object.
        // If both contact IDs are null, it will be treated as a newcontact.
        if ((oldContact->Id != nullptr) || (newContact->Id != nullptr))
        {
            isNewContact = oldContact->Id != newContact->Id;
        }
    }

    personPicture->UpdateControlForContact(isNewContact);
}

void PersonPicture::_OnGlyphStateChanged(DependencyObject^ obj, DependencyPropertyChangedEventArgs^ e)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(obj);
    personPicture->UpdateIfReady();
}

void PersonPicture::_OnPreferSmallImageChanged(DependencyObject^ obj, DependencyPropertyChangedEventArgs^ e)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(obj);
    personPicture->PreferSmallImage = static_cast<bool>(e->NewValue);
}

void PersonPicture::_OnBadgingNumberChanged(DependencyObject^ obj, DependencyPropertyChangedEventArgs^ e)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(obj);
    personPicture->BadgingNumber = static_cast<int>(e->NewValue);
    personPicture->UpdateBadgingNumber();
    personPicture->UpdateAutomationName();
}

void PersonPicture::_OnBadgingGlyphChanged(DependencyObject^ obj, DependencyPropertyChangedEventArgs^ e)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(obj);
    personPicture->BadgingGlyph = static_cast<Platform::String^>(e->NewValue);
    personPicture->UpdateBadgingGlyph();
    personPicture->UpdateAutomationName();
}

void PersonPicture::_OnSizeChanged(Object^ sender, SizeChangedEventArgs^ args)
{
    PersonPicture^ personPicture = static_cast<PersonPicture^>(sender);

    // Maintain circle by enforcing the new size on both Width and Height.
    // To do so, we will use the minimum value.
    if (args->NewSize.Width != args->NewSize.Height)
    {
        double newSize = (args->NewSize.Width < args->NewSize.Height) ? args->NewSize.Width : args->NewSize.Height;
        personPicture->Height = newSize;
        personPicture->Width = newSize;
    }

    // Calculate the FontSize of the control's text. Design guidelines have specified the
    // font size to be 42% of the container. Since it's circular, 42% of either Width or Height.
    double fontSize = personPicture->Width * .42;

    if (_initialsTextBlock != nullptr)
    {
        _initialsTextBlock->FontSize = fontSize;
    }

    if (_badgingEllipse != nullptr && _badgingNumberTextBlock != nullptr && _badgingGlyphIcon != nullptr)
    {
        // Maintain badging circle and font size by enforcing the new size on both Width and Height.
        // Design guidelines have specified the font size to be 50% of the badging plate, and we want to keep 
        // badging plate to be about 40% of the control so that don't block the initial/profile picture.
        double newSize = (args->NewSize.Width < args->NewSize.Height) ? args->NewSize.Width : args->NewSize.Height;
        _badgingEllipse->Height = newSize * 0.4;
        _badgingEllipse->Width = newSize * 0.4;
        _badgingNumberTextBlock->FontSize = _badgingEllipse->Height * 0.5;
        _badgingGlyphIcon->FontSize = _badgingEllipse->Height * 0.5;
    }
}

void PersonPicture::_OnUnloaded(Object ^sender, RoutedEventArgs ^e)
{
    if (_ProfilePictureReadAsync != nullptr)
    {
        _ProfilePictureReadAsync->Cancel();
        _ProfilePictureReadAsync = nullptr;
    }
}