// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Collection.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class TreeHelper
    {
    public:
        // Find the ancestor of the specified template control.
        template <class TAncestor>
        static TAncestor FindAncestor(xaml::DependencyObject^ element)
        {
            if (element == nullptr)
            {
                return nullptr;
            }

            auto result = dynamic_cast<TAncestor>(element);

            if (result != nullptr)
            {
                return result;
            }

            return FindAncestor<TAncestor>(xaml_media::VisualTreeHelper::GetParent(element));
        };

        static bool IsAncestorOf(xaml::DependencyObject^ ancestor, xaml::DependencyObject^ descendant)
        {
            auto current = xaml_media::VisualTreeHelper::GetParent(descendant);
            while (current != nullptr && ancestor != current)
            {
                current = xaml_media::VisualTreeHelper::GetParent(current);
            }

            return (ancestor == current);
        }

        // Used to find the first visual child of a given type.
        // Useful when there's a single unnamed visual child that a test needs access to.
        // Can't be used to reliably retrieve a specific child when multiple of that type exist in the visual tree.
        template <class TType>
        static TType^ GetVisualChildByType(FrameworkElement^ parent)
        {
            TType^ child = nullptr;

            int count = xaml_media::VisualTreeHelper::GetChildrenCount(parent);

            for (int i = 0; i < count && child == nullptr; i++)
            {
                auto current = dynamic_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(parent, i));
                auto currentAsType = dynamic_cast<TType^>(current);
                if (currentAsType)
                {
                    child = currentAsType;
                }
                else
                {
                    child = GetVisualChildByType<TType>(current);
                }
            }

            return child;
        }

        static xaml::FrameworkElement^ GetVisualChildByName(xaml::FrameworkElement^ parent, Platform::String^ name)
        {
            xaml::FrameworkElement^ child = nullptr;

            int count = xaml_media::VisualTreeHelper::GetChildrenCount(parent);

            for (int i = 0; i < count && child == nullptr; i++)
            {
                auto current = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(parent, i));
                if (current && current->Name != nullptr && current->Name == name)
                {
                    child = current;
                }
                else
                {
                    child = GetVisualChildByName(current, name);
                }
            }

            return child;
        }

        static wfc::IVectorView<xaml_primitives::Popup ^> ^ GetOpenPopups(xaml::DependencyObject^ element)
        {
            return xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(GetXamlRoot(element));
        }

        static xaml::FrameworkElement^ GetVisualChildByNameFromOpenPopups(Platform::String^ name, xaml::DependencyObject^ element)
        {
            auto popups = GetOpenPopups(element);

            for (auto popup : popups)
            {
                auto popupChild = safe_cast<xaml::FrameworkElement^>(popup->Child);
                if (popupChild->Name == name)
                {
                    return popupChild;
                }
                else
                {
                    auto child = GetVisualChildByName(popupChild, name);
                    if (child)
                    {
                        return child;
                    }
                }
            }

            return nullptr;
        }

        template <class TType>
        static TType^ GetVisualChildByTypeFromOpenPopups(xaml::DependencyObject^ element)
        {
            auto popups = GetOpenPopups(element);

            for (auto popup : popups)
            {
                auto popupChild = safe_cast<xaml::FrameworkElement^>(popup->Child);

                auto result = dynamic_cast<TType^>(popupChild);
                if (result != nullptr)
                {
                    return result;
                }
                result = GetVisualChildByType<TType>(popupChild);
                if (result != nullptr)
                {
                    return result;
                }
            }

            return nullptr;
        }

        // Searches the subtree exhaustively and extracts ALL visual children of given type. 
        template <class TType>
        static void GetVisualChildrenByType(FrameworkElement^ parent, wfc::IVector<TType^>^ children)
        {
            int count = xaml_media::VisualTreeHelper::GetChildrenCount(parent);

            for (int i = 0; i < count; i++)
            {
                auto current = dynamic_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(parent, i));
                auto currentAsType = dynamic_cast<TType^>(current);
                if (currentAsType)
                {
                    children->Append(currentAsType);
                }
                GetVisualChildrenByType<TType>(current, children);
            }
        }

        // Searches the subtree exhaustively and extracts ALL visual children of given type from open popups. 
        template <class TType>
        static void GetVisualChildrenByTypeFromOpenPopups(wfc::IVector<TType^>^ children)
        {
            auto popups = GetOpenPopups(TestServices::WindowHelper->WindowContent);

            for (auto popup : popups)
            {
                GetVisualChildrenByType<TType>(safe_cast<xaml::FrameworkElement^>(popup->Child), children);
            }
        }

        static void WalkTree(
            xaml::DependencyObject^ parent,
            std::function<void(xaml::DependencyObject^)> callback)
        {
            callback(parent);

            int count = xaml_media::VisualTreeHelper::GetChildrenCount(parent);
            for (int i = 0; i < count; ++i)
            {
                auto current = dynamic_cast<xaml::DependencyObject^>(xaml_media::VisualTreeHelper::GetChild(parent, i));
                WalkTree(current, callback);
            }
        }

        template <class TElement>
        static void AddElementIntoLivetree(TElement^ existingInstance, bool wrapInGrid = false)
        {
            auto spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(TElement, Loaded);

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(existingInstance, ref new xaml::RoutedEventHandler([spHasLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e) {
                    spHasLoadedEvent->Set();
                }));

                if (wrapInGrid)
                {
                    TestServices::WindowHelper->WindowContent = WrapInGrid(existingInstance);
                }
                else
                {
                    TestServices::WindowHelper->WindowContent = existingInstance;
                }
            });
            spHasLoadedEvent->WaitForDefault();
        }

        static XamlRoot^ GetXamlRoot(xaml::DependencyObject^ obj)
        {
            XamlRoot^ xamlRoot;
            if (xaml::UIElement^ e = safe_cast<xaml::UIElement^>(obj))
            {
                xamlRoot = e->XamlRoot;
            }
            else if (xaml_docs::TextElement^ te = safe_cast<xaml_docs::TextElement^>(obj))
            {
                xamlRoot = te->XamlRoot;
            }
            else if (xaml_primitives::FlyoutBase^ fb = safe_cast<xaml_primitives::FlyoutBase^>(obj))
            {
                xamlRoot = fb->XamlRoot;
            }
            else
            {
                throw "TreeHelper::GetXamlRoot: Can't find XamlRoot for element";
            }
            return xamlRoot;
        }

        // A number of tests set a custom Control as Window Content.  This *should* be fine, but we get an invalid cast
        // exception in C# code in WPFHost when we try to cast this to Microsoft.UI.Xaml.UIElement.  Seems like there's
        // an issue with our test code where we don't give cswinrt enough information to fully understand these custom
        // types in our tests.
        // For now we have this workaround to call this helper function.
        // Xaml test infra doesn't support setting a custom C++/Cx control as WindowHelper->WindowContent
        static xaml_controls::Grid^ WrapInGrid(xaml::UIElement^ element)
        {
            xaml_controls::Grid^ rootGrid = ref new xaml_controls::Grid();
            rootGrid->Children->Append(element);
            return rootGrid;
        }

    }; // class TreeHelper

} } } } }
