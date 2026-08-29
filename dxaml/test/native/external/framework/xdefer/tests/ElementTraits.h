// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        struct NoCommand                    { void operator()(Platform::Object^); };
        struct ShowButtonFlyoutCommand      { void operator()(Platform::Object^); };
        struct ShowAttachedFlyoutCommand    { void operator()(Platform::Object^); };

        #define ACCESSOR_DEFINITION_ATT_PROP_WITH_NAME(accessorName, property_name, property_class, property_type, command_type) \
        template<typename T> struct accessorName##Accessor { \
            typedef T type; \
            unsigned          get_size(T^ e)                          { return (get_value(e, 0) != nullptr) ? 1 : 0; } \
            Platform::Object^ get_value(T^ e, unsigned)               { return property_class::Get##property_name(e); } \
            void              set_value(T^ e, Platform::Object^ c)    { property_class::Set##property_name(e, safe_cast<property_type^>(c)); } \
            void              clear_value(T^ e)                       { e->ClearValue(property_class::property_name##Property); } \
            const wchar_t*    get_property_name()                     { return L#property_name; } \
            void              execute_command(Platform::Object^ e)    { command_type()(e); } \
        };

        #define ACCESSOR_DEFINITION_ATT_PROP(property_name, property_class, property_type) \
        ACCESSOR_DEFINITION_ATT_PROP_WITH_NAME(property_name, property_name, property_class, property_type, NoCommand);

        #define ACCESSOR_DEFINITION_DEP_PROP_WITH_NAME(accessorName, property_name, property_type, command_type) \
        template<typename T> struct accessorName##Accessor { \
            typedef T type; \
            unsigned          get_size(T^ e)                          { return (get_value(e, 0) != nullptr) ? 1 : 0; } \
            Platform::Object^ get_value(T^ e, unsigned)               { return e->##property_name; } \
            void              set_value(T^ e, Platform::Object^ c)    { e->##property_name = safe_cast<property_type^>(c); } \
            void              clear_value(T^ e)                       { e->ClearValue(T::property_name##Property); } \
            const wchar_t*    get_property_name()                     { return L#property_name; } \
            void              execute_command(Platform::Object^ e)    { command_type()(e); } \
        };

        #define ACCESSOR_DEFINITION_DEP_PROP(property_name, property_type) \
        ACCESSOR_DEFINITION_DEP_PROP_WITH_NAME(property_name, property_name, property_type, NoCommand);

        #define ACCESSOR_DEFINITION_PROP_WITH_NAME(accessorName, property_name, property_type, command_type) \
        template<typename T> struct accessorName##Accessor { \
            typedef T type; \
            unsigned          get_size(T^ e)                          { return (get_value(e, 0) != nullptr) ? 1 : 0; } \
            Platform::Object^ get_value(T^ e, unsigned)               { return e->##property_name; } \
            void              set_value(T^ e, Platform::Object^ c)    { e->##property_name = safe_cast<property_type^>(c); } \
            void              clear_value(T^ e)                       { e->##property_name = nullptr; } \
            const wchar_t*    get_property_name()                     { return L#property_name; } \
            void              execute_command(Platform::Object^ e)    { command_type()(e); } \
        };

        #define ACCESSOR_DEFINITION_PROP(property_name, property_type) \
        ACCESSOR_DEFINITION_PROP_WITH_NAME(property_name, property_name, property_type, NoCommand);

        #define ACCESSOR_DEFINITION_COLLECTION_WITH_NAME(accessorName, property_name, property_type, command_type) \
        template<typename T> struct accessorName##Accessor { \
            typedef T type; \
            unsigned          get_size(T^ e)                          { return e->##property_name->Size; } \
            Platform::Object^ get_value(T^ e, unsigned index)         { return e->##property_name->GetAt(index); } \
            void              set_value(T^ e, Platform::Object^ c)    { e->##property_name->Append(safe_cast<property_type^>(c)); } \
            void              clear_value(T^ e)                       { e->##property_name->Clear(); } \
            const wchar_t*    get_property_name()                     { return L#property_name; } \
            void              execute_command(Platform::Object^ e)    { command_type()(e); } \
        };

        #define ACCESSOR_DEFINITION_COLLECTION(property_name, property_type) \
        ACCESSOR_DEFINITION_COLLECTION_WITH_NAME(property_name, property_name, property_type, NoCommand);

        template<typename T> struct EmptyAccessor {
            typedef T type;
            unsigned          get_size(T^)                            { VERIFY_FAIL(); return 0; }
            Platform::Object^ get_value(T^, unsigned)                 { VERIFY_FAIL(); return nullptr; }
            void              set_value(T^, Platform::Object^)        { VERIFY_FAIL(); }
            void              clear_value(T^)                         { VERIFY_FAIL(); }
            const wchar_t*    get_property_name()                     { VERIFY_FAIL(); return nullptr; }
            void              execute_command(Platform::Object^)      { VERIFY_FAIL(); }
        };

        template<typename T>
        struct ElementTraits
        {
            typedef T type;
            static const wchar_t* get_class_name();
        };

        template<typename T, typename P>
        struct PropertyTraits
        {
            typedef T type;
            static const wchar_t*    get_class_name()                     { static_assert(false); }
            static const wchar_t*    get_property_name()                  { static_assert(false); }
            static unsigned          get_size(T^)                         { static_assert(false); }
            static Platform::Object^ get_value(T^, unsigned)              { static_assert(false); }
            static void              set_value(T^, Platform::Object^)     { static_assert(false); }
            static void              clear_value(T^)                      { static_assert(false); }
            static void              execute_command(Platform::Object^)   { static_assert(false); }
        };

        #define ATT_PROPERTY_TRAITS_SPECIALIZATION(_namespace, _type, _decl_namespace, _decl_type, _property) \
        template<> \
        struct PropertyTraits<_namespace::_type, _property##Accessor<_namespace::_type>> \
        { \
            typedef _namespace::_type type; \
            typedef _decl_namespace::_decl_type declaringtype; \
            typedef _property##Accessor<type> accesortype; \
            static accesortype accessor; \
            static const wchar_t*    get_class_name()                        { return ElementTraits<declaringtype>::get_class_name(); } \
            static const wchar_t*    get_property_name()                     { return accessor.get_property_name(); } \
            static unsigned          get_size(type^ e)                       { return accessor.get_size(e); } \
            static Platform::Object^ get_value(type^ e, unsigned index = 0)  { return accessor.get_value(e, index); } \
            static void              set_value(type^ e, Platform::Object^ c) { accessor.set_value(e, c); } \
            static void              clear_value(type^ e)                    { accessor.clear_value(e); } \
            static void              execute_command(Platform::Object^ c)    { accessor.execute_command(c); } \
        }; \
        PropertyTraits<_namespace::_type, _property##Accessor<_namespace::_type>>::accesortype PropertyTraits<_namespace::_type, _property##Accessor<_namespace::_type>>::accessor;

        #define PROPERTY_TRAITS_SPECIALIZATION(_namespace, _type, _property) \
        ATT_PROPERTY_TRAITS_SPECIALIZATION(_namespace, _type, _namespace, _type, _property)

        #define ELEMENT_TRAITS_SPECIALIZATION(_namespace, _type) \
        const wchar_t* ElementTraits<_namespace::_type>::get_class_name() { return L#_type; } \
        PROPERTY_TRAITS_SPECIALIZATION(_namespace, _type, Empty);

        // Property accessors

        ACCESSOR_DEFINITION_ATT_PROP_WITH_NAME(AttachedFlyout, AttachedFlyout, FlyoutBase, FlyoutBase, ShowAttachedFlyoutCommand);
        ACCESSOR_DEFINITION_DEP_PROP(BottomAppBar, AppBar);
        ACCESSOR_DEFINITION_PROP(Child, UIElement);
        ACCESSOR_DEFINITION_COLLECTION(Children, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(CommandParameter, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(Content, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP_WITH_NAME(Flyout, Flyout, FlyoutBase, ShowButtonFlyoutCommand);
        ACCESSOR_DEFINITION_DEP_PROP(Footer, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(Header, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(Icon, IconElement);
        ACCESSOR_DEFINITION_COLLECTION(Items, Platform::Object);
        ACCESSOR_DEFINITION_DEP_PROP(LeftHeader, UIElement);
        ACCESSOR_DEFINITION_COLLECTION_WITH_NAME(MenuItems, Items, MenuFlyoutItemBase, NoCommand);
        ACCESSOR_DEFINITION_DEP_PROP(OffContent, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(OnContent, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(PrimaryButtonCommandParameter, UIElement);
        ACCESSOR_DEFINITION_COLLECTION(PrimaryCommands, ICommandBarElement);
        ACCESSOR_DEFINITION_DEP_PROP(SecondaryButtonCommandParameter, UIElement);
        ACCESSOR_DEFINITION_COLLECTION(SecondaryCommands, ICommandBarElement);
        ACCESSOR_DEFINITION_DEP_PROP(Title, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(TopAppBar, AppBar);
        ACCESSOR_DEFINITION_DEP_PROP(TopHeader, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(TopLeftHeader, UIElement);
        ACCESSOR_DEFINITION_DEP_PROP(ZoomedInView, ISemanticZoomInformation);
        ACCESSOR_DEFINITION_DEP_PROP(ZoomedOutView, ISemanticZoomInformation);

        // Classes and properties

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml, FrameworkElement);
        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, FlyoutBase);
        ATT_PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml, FrameworkElement, Microsoft::UI::Xaml::Controls::Primitives, FlyoutBase, AttachedFlyout);

        // Microsoft::UI::Xaml::Controls

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBar);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBar, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBarButton);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBarButton, Content);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBarButton, Icon);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBarSeparator);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBarToggleButton);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBarToggleButton, Content);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, AppBarToggleButton, Icon);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, BitmapIcon);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Border);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Border, Child);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Button);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Button, Content);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Button, CommandParameter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Button, Flyout);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Canvas);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Canvas, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, CheckBox);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, CheckBox, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ComboBox);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ComboBox, Header);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ComboBox, Items);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ComboBoxItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ComboBoxItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, CommandBar);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, CommandBar, PrimaryCommands);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, CommandBar, SecondaryCommands);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentControl);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentControl, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentDialog);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentDialog, Content);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentDialog, Title);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentDialog, PrimaryButtonCommandParameter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentDialog, SecondaryButtonCommandParameter);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentPresenter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ContentPresenter, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, DatePicker);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, DatePicker, Header);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, FlipView);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, FlipView, Items);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, FlipViewItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, FlipViewItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Flyout);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Flyout, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, FlyoutPresenter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, FlyoutPresenter, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, FontIcon);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Frame);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Frame, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Grid);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Grid, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridView);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridView, Items);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridView, Header);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridView, Footer);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridViewHeaderItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridViewHeaderItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridViewItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GridViewItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GroupItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, GroupItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, HyperlinkButton);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, HyperlinkButton, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Image);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsControl);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsControl, Items);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsPresenter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsPresenter, Footer);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsPresenter, Header);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsStackPanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsStackPanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsWrapGrid);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ItemsWrapGrid, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListBox);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListBox, Items);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListBoxItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListBoxItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListView);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListView, Items);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListView, Header);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListView, Footer);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListViewHeaderItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListViewHeaderItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListViewItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ListViewItem, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MediaTransportControls);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyout);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyout, MenuItems);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyoutItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyoutItem, CommandParameter);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyoutPresenter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyoutPresenter, Items);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyoutSeparator);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyoutSubItem);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, MenuFlyoutSubItem, MenuItems);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Page);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Page, BottomAppBar);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Page, Content);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Page, TopAppBar);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, PasswordBox);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, PasswordBox, Header);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, PathIcon);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RadioButton);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RadioButton, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RelativePanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RelativePanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RichEditBox);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RichEditBox, Header);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RichTextBlock);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, RichTextBlockOverflow);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ScrollContentPresenter);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ScrollViewer);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ScrollViewer, Content);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ScrollViewer, LeftHeader);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ScrollViewer, TopHeader);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ScrollViewer, TopLeftHeader);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SemanticZoom);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SemanticZoom, ZoomedInView);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SemanticZoom, ZoomedOutView);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Slider);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Slider, Header);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, StackPanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, StackPanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SwapChainBackgroundPanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SwapChainBackgroundPanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SwapChainPanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SwapChainPanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, SymbolIcon);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, TextBlock);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, TextBox);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, TextBox, Header);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, TimePicker);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, TimePicker, Header);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ToggleMenuFlyoutItem);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ToggleSwitch);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ToggleSwitch, Header);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ToggleSwitch, OffContent);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ToggleSwitch, OnContent);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ToolTip);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, ToolTip, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, UserControl);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, UserControl, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, VariableSizedWrapGrid);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, VariableSizedWrapGrid, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Viewbox);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, Viewbox, Child);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, VirtualizingStackPanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, VirtualizingStackPanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, WrapGrid);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls, WrapGrid, Children);

        // Microsoft::UI::Xaml::Controls::Primitives

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, CalendarPanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, CalendarPanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, CarouselPanel);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, CarouselPanel, Children);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, GridViewItemPresenter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, GridViewItemPresenter, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, ListViewItemPresenter);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, ListViewItemPresenter, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, Popup);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, Popup, Child);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, RepeatButton);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, RepeatButton, Content);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, ScrollBar);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, Thumb);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, TickBar);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, ToggleButton);
        PROPERTY_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Controls::Primitives, ToggleButton, Content);

        // Microsoft::UI::Xaml::Shapes

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Shapes, Ellipse);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Shapes, Line);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Shapes, Path);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Shapes, Polygon);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Shapes, Polyline);

        ELEMENT_TRAITS_SPECIALIZATION(Microsoft::UI::Xaml::Shapes, Rectangle);
    }
} } } }
