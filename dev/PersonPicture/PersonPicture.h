// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PersonPicture.g.h"
#include "PersonPicture.properties.h"
#include "DispatcherHelper.h"

class PersonPicture :
    public ReferenceTracker<PersonPicture, winrt::implementation::PersonPictureT>,
    public PersonPictureProperties
{
public:
    PersonPicture();

    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();

    // Property changed handler.
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs &args);

private:
    /// <summary>
    /// Updates Control elements, if available, with the latest values.
    /// </summary>
    void UpdateIfReady();

    /// <summary>
    /// Updates the state of the Badging element.
    /// </summary>
    void UpdateBadge();

    /// <summary>
    /// Updates Badging Number text element.
    /// </summary>
    void UpdateBadgeNumber();

    /// <summary>
    /// Updates Badging Glyph element.
    /// </summary>
    void UpdateBadgeGlyph();

    /// <summary>
    /// Updates Badging Image element.
    /// </summary>
    void UpdateBadgeImageSource();

    /// <summary>
    /// Helper function to contain the calls to load and display a user's profile picture.
    /// </summary>
    /// <param name="isNewContact"> 
    /// Specifies if the target contact is to be handled as newContact.
    /// </param>
    void UpdateControlForContact(bool isNewContact);

    /// <summary>
    /// Sets the UI Automation name for the control based on contact name and badge state.
    /// </summary>
    void UpdateAutomationName();

    /// <summary>
    /// Helper to determine the initials that should be shown.
    /// </summary>
    winrt::hstring GetInitials();

    /// <summary>
    /// Helper to determine the image source that should be shown.
    /// </summary>
    winrt::ImageSource GetImageSource();

    // DependencyProperty changed event handlers
    void OnDisplayNameChanged(const winrt::DependencyPropertyChangedEventArgs &args);
    void OnContactChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    // Event handlers
    void OnSizeChanged(const winrt::IInspectable &sender, const winrt::SizeChangedEventArgs &e);
    void OnUnloaded(const winrt::IInspectable &sender, const winrt::RoutedEventArgs &e);

    // Helper functions
    void LoadImageAsync(
        std::shared_ptr<winrt::IRandomAccessStreamReference> thumbStreamReference,
        std::function<void(winrt::BitmapImage)> completedFunction);

    winrt::hstring PersonPicture::GetLocalizedPluralBadgeItemStringResource(unsigned int numericValue);

    /// <summary>
    /// XAML Element for the first TextBlock matching x:Name of InitialsTextBlock.
    /// </summary>
    tracker_ref<winrt::TextBlock> m_initialsTextBlock{ this };

    /// <summary>
    /// XAML Element for the first TextBlock matching x:Name of BadgeNumberTextBlock.
    /// </summary>
    tracker_ref<winrt::TextBlock> m_badgeNumberTextBlock{ this };

    /// <summary>
    /// XAML Element for the first TextBlock matching x:Name of BadgeGlyphIcon.
    /// </summary>
    tracker_ref<winrt::FontIcon> m_badgeGlyphIcon{ this };

    /// <summary>
    /// XAML Element for the first ImageBrush matching x:Name of BadgeImageBrush.
    /// </summary>
    tracker_ref<winrt::ImageBrush> m_badgeImageBrush{ this };

    /// <summary>
    /// XAML Element for the first Ellipse matching x:Name of BadgingBackgroundEllipse.
    /// </summary>
    tracker_ref<winrt::Ellipse> m_badgingEllipse{ this };
    
    /// <summary>
    /// XAML Element for the first Ellipse matching x:Name of BadgingEllipse.
    /// </summary>
    tracker_ref<winrt::Ellipse> m_badgingBackgroundEllipse{ this };

    /// <summary>
    /// The async operation object representing the loading and assignment of the Thumbnail.
    /// </summary>
    tracker_ref<winrt::IAsyncOperation<winrt::IRandomAccessStreamWithContentType>> m_profilePictureReadAsync{ this };

    /// <summary>
    /// The initials from the DisplayName property.
    /// </summary>
    tracker_ref<winrt::hstring> m_displayNameInitials{ this };

    /// <summary>
    /// The initials from the Contact property.
    /// </summary>
    tracker_ref<winrt::hstring> m_contactDisplayNameInitials{ this };

    /// <summary>
    /// The ImageSource from the Contact property.
    /// </summary>
    tracker_ref<winrt::ImageSource> m_contactImageSource{ this };

    DispatcherHelper m_dispatcherHelper{ *this };
};
