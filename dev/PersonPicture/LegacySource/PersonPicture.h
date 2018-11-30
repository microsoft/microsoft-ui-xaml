// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// PersonPicture.h
// Declaration of the PersonPicture class.

#pragma once

namespace Microsoft { namespace People { namespace Controls {

    /// <summary>
    /// PersonPicture Control. Displays the Profile Picture, or in its absence Initials,
    /// for a given TargetContact.
    /// </summary>
    [Windows::UI::Xaml::Data::Bindable]
    [Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public ref class PersonPicture sealed : public Windows::UI::Xaml::Controls::Control
    {

    public:
        PersonPicture();

        /// <summary>
        /// Public representation of DependencyProperty backing ProfilePicture property.
        /// </summary>
        /// <remarks>
        /// While not strictly necessary, usage is standards compliant for TemplateBinding properties.
        /// XAML automatically creates the association between <VariableName>Property and <VariableName>.
        /// </remarks>
        static property Windows::UI::Xaml::DependencyProperty^ ProfilePictureProperty
        {
            /// <summary>
            /// Gets the DepedencyProperty relating to the Control's ProfilePicture property.
            /// </summary>
            Windows::UI::Xaml::DependencyProperty^ get()
            {
                return _ProfilePicture;
            }
        };

        /// <summary>
        /// Contact object for which to render the correct display picture.
        /// </summary>
        property Windows::ApplicationModel::Contacts::Contact^ TargetContact
        {
            /// <summary>
            /// Gets the Contact object currently in use by the control.
            /// </summary>
            Windows::ApplicationModel::Contacts::Contact^ get()
            {
                return static_cast<Windows::ApplicationModel::Contacts::Contact^>(GetValue(_TargetContact));
            }

            /// <summary>
            /// Set the Contact object for use by the control.
            /// </summary>
            void set(Windows::ApplicationModel::Contacts::Contact^ value)
            {
                SetValue(_TargetContact, value);
            }
        }

        /// <summary>
        /// Bitmap resource for the target contact's display picture, if available.
        /// </summary>
        property Windows::UI::Xaml::Media::ImageSource^ ProfilePicture
        {
            /// <summary>
            /// Gets the Profile Picture currently in use by the control.
            /// If set by the Control, this resource will be of type BitmapImage.
            /// If set externally of the control, this resource must derive from ImageSource.
            /// </summary>
            Windows::UI::Xaml::Media::ImageSource^ get()
            {
                return static_cast<Windows::UI::Xaml::Media::ImageSource^>(GetValue(_ProfilePicture));
            }

            /// <summary>
            /// Sets the Profile Picture for use by the control.
            /// </summary>
            void set(Windows::UI::Xaml::Media::ImageSource^ value)
            {
                SetValue(_ProfilePicture, value);

                // This check only needs to be performed when ProfilePicture is updated.
                // Logic encapsulated in another method to assist in code clarity.
                PersonPicture::UpdateStateFromProfilePicture(value);
            }
        }

        /// <summary>
        /// String corresponding to the initials of the target contact, if available.
        /// </summary>
        property Platform::String^ PersonInitials
        {
            /// <summary>
            /// Gets the string of initials currently displayed by the control.
            /// </summary>
            Platform::String^ get()
            {
                return static_cast<Platform::String^>(GetValue(_PersonInitials));
            }

            /// <summary>
            /// Sets the initials for use by the control.
            /// </summary>
            void set(Platform::String^ value)
            {
                SetValue(_PersonInitials, value);
            }
        }

        /// <summary>
        /// String corresponding to the display name of the target contact, if available.
        /// </summary>
        property Platform::String^ PersonDisplayName
        {
            /// <summary>
            /// Gets the string of display name of the person currently displayed by the control.
            /// </summary>
            Platform::String^ get()
            {
                return static_cast<Platform::String^>(GetValue(_PersonDisplayName));
            }

            /// <summary>
            /// Sets the display name for use by the control.
            /// </summary>
            void set(Platform::String^ value)
            {
                SetValue(_PersonDisplayName, value);
            }
        }

        /// <summary>
        /// Boolean value which indicates if the control is showing a generic Group glyph.
        /// </summary>
        property bool ShowingGroupGlyph
        {
            /// <summary>
            /// Gets the state of the control indicating if a generic Group glyph is being displayed.
            /// </summary>
            bool get()
            {
                return static_cast<bool>(GetValue(_ShowingGroupGlyph));
            }

            /// <summary>
            /// Sets the state of the control to show or hide a generic Group glyph.
            /// </summary>
            void set(bool value)
            {
                SetValue(_ShowingGroupGlyph, value);
            }
        }

        /// <summary>
        /// The async operation object representing the loading and assignment of the Thumbnail.
        /// </summary>
        property Windows::Foundation::IAsyncAction^ ProfilePictureReadAsync
        {
            Windows::Foundation::IAsyncAction^ get()
            {
                return _ProfilePictureReadAsync;
            }
        }

        /// <summary>
        /// Property to prioritize using the small picture if available.
        /// </summary>
        property bool PreferSmallImage
        {
            bool get()
            {
                return m_preferSmallImage;
            }

            void set(bool value)
            {
                m_preferSmallImage = value;
            }
        }

        /// <summary>
        /// Property for badging number to be displayed.
        /// </summary>
        property int BadgingNumber
        {
            int get()
            {
                return static_cast<int>(GetValue(_BadgingNumber));
            }

            void set(int value)
            {
                SetValue(_BadgingNumber, value);
            }
        }

        /// <summary>
        /// Property for Badging Glyph Icon to be displayed.
        /// </summary>
        property Platform::String^ BadgingGlyph
        {
            Platform::String^ get()
            {
                return static_cast<Platform::String^>(GetValue(_BadgingGlyph));
            }

            void set(Platform::String^ value)
            {
                SetValue(_BadgingGlyph, value);
            }
        }

    protected:
        /// <summary>
        /// Gets the applicable XAML elements from the template.
        /// </summary>
        virtual void OnApplyTemplate() override;

        /// <summary>
        /// Provides the automated peer for the control.
        /// </summary>
        virtual Windows::UI::Xaml::Automation::Peers::AutomationPeer^ OnCreateAutomationPeer() override;

    private:
        /// <summary>
        /// Updates Control elements, if available, with the latest values.
        /// </summary>
        void UpdateIfReady();

        /// <summary>
        /// Updates Badging Number text element.
        /// </summary>
        void UpdateBadgingNumber();

        /// <summary>
        /// Updates Badging Glyph element.
        /// </summary>
        void UpdateBadgingGlyph();

        /// <summary>
        /// Helper function to contain the calls to load and display a user's profile picture.
        /// </summary>
        /// <param name="isNewContact"> 
        /// Specifies if the target contact is to be handled as newContact.
        /// </param>
        void UpdateControlForContact(bool isNewContact);

        /// <summary>
        /// Helper function to handle an external update of the ProfilePicture property.
        /// </summary>
        void UpdateStateFromProfilePicture(Windows::UI::Xaml::Media::ImageSource^ value);

        /// <summary>
        /// Sets the UI Automation name for the control based on contact name and badge state.
        /// </summary>
        void UpdateAutomationName();

        /// <summary>
        /// Called when either the PersonInitials or ProfilePicture are updated.
        /// </summary>
        static void _OnPersonDataChanged(
            Windows::UI::Xaml::DependencyObject^ obj,
            Windows::UI::Xaml::DependencyPropertyChangedEventArgs^ e);

        /// <summary>
        /// Called when TargetContact is updated.
        /// </summary>
        static void _OnTargetContactChanged(
            Windows::UI::Xaml::DependencyObject^ obj,
            Windows::UI::Xaml::DependencyPropertyChangedEventArgs^ e);
        
        /// <summary>
        /// Called when ShowingGroupGlyph is updated.
        /// </summary>
        static void _OnGlyphStateChanged(
            Windows::UI::Xaml::DependencyObject^ obj,
            Windows::UI::Xaml::DependencyPropertyChangedEventArgs^ e);

        /// <summary>
        /// Called when the control's size is changed. This allows us to update
        /// the font size, as well as enforce a circular size (width == height).
        /// </summary>
        void _OnSizeChanged(
            Platform::Object^ sender, 
            Windows::UI::Xaml::SizeChangedEventArgs^ args);
        
        /// <summary>
        /// Called when the control is unloaded, this allows us to cancel any outstanding tasks.
        /// </summary>
        void _OnUnloaded(
            Platform::Object ^sender,
            Windows::UI::Xaml::RoutedEventArgs ^e);

        /// <summary>
        /// Called when PersonDisplayName is updated.
        /// </summary>
        static void _OnDisplayNameChanged(
            Windows::UI::Xaml::DependencyObject^ obj,
            Windows::UI::Xaml::DependencyPropertyChangedEventArgs^ e);

        /// <summary>
        /// Called when PreferSmallImage property is updated.
        /// </summary>
        static void _OnPreferSmallImageChanged(
            Windows::UI::Xaml::DependencyObject^ obj,
            Windows::UI::Xaml::DependencyPropertyChangedEventArgs^ e);

        /// <summary>
        /// Called when BadgingNumber property is updated.
        /// </summary>
        static void _OnBadgingNumberChanged(
            Windows::UI::Xaml::DependencyObject^ obj,
            Windows::UI::Xaml::DependencyPropertyChangedEventArgs^ e);

        /// <summary>
        /// Called when BadgingGlyph property is updated.
        /// </summary>
        static void _OnBadgingGlyphChanged(
            Windows::UI::Xaml::DependencyObject^ obj,
            Windows::UI::Xaml::DependencyPropertyChangedEventArgs^ e);

        /// <summary>
        /// Dependency Property for the Control's TargetContact object.
        /// Changes to this Contact will require re-evaluation of the control's visual state.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _TargetContact;

        /// <summary>
        /// Dependency Property for the Control's ProfilePicture object.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _ProfilePicture;

        /// <summary>
        /// Dependency Property for the Control's PersonInitials object.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _PersonInitials;

        /// <summary>
        /// Dependency Property for the Control's PersonDisplayName object.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _PersonDisplayName;

        /// <summary>
        /// Dependency Property for the Control's ShowGroupGlyph state.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _ShowingGroupGlyph;

        /// <summary>
        /// Dependency Property for the Control to use the small contact image by default.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _PreferSmallImage;

        /// <summary>
        /// Dependency Property for the Control's Badging number to display.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _BadgingNumber;

        /// <summary>
        /// Dependency Property for the Control's Badging Glyph to display.
        /// </summary>
        static Windows::UI::Xaml::DependencyProperty^ _BadgingGlyph;

        /// XAML Element for the first TextBlock matching x:Name of PersonInitialsTextBlock.
        /// </summary>
        Windows::UI::Xaml::Controls::TextBlock^ _initialsTextBlock;

        /// <summary>
        /// XAML Element for the contact image matching x:Name of PersonPictureEllipse.
        /// </summary>
        Windows::UI::Xaml::Shapes::Ellipse^ _personPictureEllipse;

        /// <summary>
        /// The font family of person text block as default.
        /// </summary>
        Windows::UI::Xaml::Media::FontFamily^ _personTextFontFamily;

        /// <summary>
        /// XAML Element for the first TextBlock matching x:Name of BadgingNumberTextBlock.
        /// </summary>
        Windows::UI::Xaml::Controls::TextBlock^ _badgingNumberTextBlock;

        /// <summary>
        /// XAML Element for the first TextBlock matching x:Name of BadgingGlyphIcon.
        /// </summary>
        Windows::UI::Xaml::Controls::FontIcon^ _badgingGlyphIcon;

        /// <summary>
        /// XAML Element for the first Ellipse matching x:Name of BadgingEllipse.
        /// </summary>
        Windows::UI::Xaml::Shapes::Ellipse^ _badgingEllipse;

        /// <summary>
        /// The async operation object representing the loading and assignment of the Thumbnail.
        /// </summary>
        Windows::Foundation::IAsyncAction^ _ProfilePictureReadAsync;

        /// <summary>
        /// Parameter to prefer using the small contact image.
        /// </summary>
        bool m_preferSmallImage;
    };

}}} // namespace Microsoft { namespace People { namespace Controls {
