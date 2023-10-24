// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AutoHandle.h"
#include "StaticAssertFalse.h"

class SharedHelpers
{
public:
    static bool IsAnimationsEnabled();
    static winrt::IInspectable FindResource(const std::wstring_view& resource, const winrt::ResourceDictionary& resources, const winrt::IInspectable& defaultValue = nullptr);
    static winrt::IInspectable FindInApplicationResources(const std::wstring_view& resource, const winrt::IInspectable& defaultValue = nullptr);
    static bool IsInDesignMode();
    static bool IsInDesignModeV1();
    static bool IsInDesignModeV2();
    
    static bool IsCoreWindowActivationModeAvailable();

    static bool IsApplicationViewGetDisplayRegionsAvailable();

    static bool IsInFrameworkPackage();

    static bool IsInFrameworkPackage(winrt::hstring& frameworkPackageInstallLocation);

    static bool IsBringIntoViewOptionsVerticalAlignmentRatioAvailable();

    // Platform scale helper
    static winrt::Rect ConvertDipsToPhysical(winrt::UIElement const& xamlRootReference, const winrt::Rect& dipsRect);

    static bool IsOnXbox();
    static bool IsMouseModeEnabled();

    // Rect helpers
    static bool DoRectsIntersect(const winrt::Rect& rect1, const winrt::Rect& rect2);

    // DependencyObject helpers
    static bool IsAncestor(const winrt::DependencyObject& child, const winrt::DependencyObject& parent, bool checkVisibility = false);

    static void SyncWait(const winrt::IAsyncAction& asyncAction)
    {
        MUXControls::Common::Handle synchronizationHandle(::CreateEvent(nullptr, FALSE, FALSE, nullptr));

        asyncAction.Completed(winrt::AsyncActionCompletedHandler(
            [&synchronizationHandle](winrt::IAsyncAction asyncAction, winrt::AsyncStatus asyncStatus)
        {
            if (asyncStatus == winrt::AsyncStatus::Completed)
            {
                SetEvent(synchronizationHandle);
            }
            else if (asyncStatus == winrt::AsyncStatus::Error)
            {
                throw winrt::hresult_error(E_FAIL, L"Async operation failed!");
            }
        }));

        WaitForSingleObject(synchronizationHandle, INFINITE);
    }

    template <typename T>
    static T SyncWait(winrt::IAsyncOperation<T> asyncOperation)
    {
        T returnValue{};
        MUXControls::Common::Handle synchronizationHandle(::CreateEvent(nullptr, FALSE, FALSE, nullptr));

        asyncOperation.Completed(winrt::AsyncOperationCompletedHandler<T>(
            [&synchronizationHandle, &returnValue](winrt::IAsyncOperation<T> asyncOperation, winrt::AsyncStatus asyncStatus)
        {
            if (asyncStatus == winrt::AsyncStatus::Completed)
            {
                SetEvent(synchronizationHandle);
                returnValue = asyncOperation.GetResults();
            }
            else if (asyncStatus == winrt::AsyncStatus::Error)
            {
                throw winrt::hresult_error(E_FAIL, L"Async operation failed!");
            }
        }));

        WaitForSingleObject(synchronizationHandle, INFINITE);
        return returnValue;
    }

    static void ScheduleActionAfterWait(
        std::function<void()> const& action,
        unsigned int millisecondWait);

    static winrt::InMemoryRandomAccessStream CreateStreamFromBytes(const winrt::array_view<const byte>& bytes);

    static void QueueCallbackForCompositionRendering(std::function<void()> callback);

    static winrt::FrameworkElement FindInVisualTreeByName(winrt::FrameworkElement const& parent, std::wstring_view const& name)
    {
        return FindInVisualTree(parent,
            [name](const winrt::FrameworkElement& element)
            {
                return (element.Name() == name);
            });
    }

    template <typename ElementType>
    static ElementType FindInVisualTreeByType(winrt::FrameworkElement const& parent)
    {
        auto element = FindInVisualTree(parent,
            [](const winrt::FrameworkElement& element)
            {
                return (element.try_as<ElementType>() != nullptr);
            });
        return element.as<ElementType>();
    }

    static winrt::FrameworkElement FindInVisualTree(winrt::FrameworkElement const& parent, std::function<bool(winrt::FrameworkElement element)> const& isMatch)
    {
        int numChildren = winrt::VisualTreeHelper::GetChildrenCount(parent);

        winrt::FrameworkElement foundElement = parent;
        if (isMatch(foundElement))
        {
            return foundElement;
        }

        for (int i = 0; i < numChildren; i++)
        {
            if (auto dp = winrt::VisualTreeHelper::GetChild(parent, i))
            {
                if (auto fe = dp.as<winrt::FrameworkElement>())
                {
                    foundElement = FindInVisualTree(fe, isMatch);
                    if (foundElement)
                    {
                        return foundElement;
                    }
                }
            }
        }

        return nullptr;
    }

    static bool IsTrue(winrt::IReference<bool> const& nullableBool)
    {
        if (nullableBool)
        {
            return nullableBool.GetBoolean();
        }
        return false;
    }

    template<typename AncestorType>
    static AncestorType GetAncestorOfType(winrt::DependencyObject const& firstGuess)
    {
        auto obj = firstGuess;
        AncestorType matchedAncestor{ nullptr };
        while (obj && !matchedAncestor)
        {
            matchedAncestor = obj.try_as<AncestorType>();
            obj = winrt::VisualTreeHelper::GetParent(obj);
        }

        if (matchedAncestor)
        {
            return matchedAncestor.as<AncestorType>();
        }
        else
        {
            return nullptr;
        }
    }

    static winrt::hstring TryGetStringRepresentationFromObject(winrt::IInspectable obj);

#if defined(TITLEBAR_INCLUDED) || defined(SWIPECONTROL_INCLUDED) || defined(TEACHINGTIP_INCLUDED) || defined(TABVIEW_INCLUDED)
    static winrt::IconElement MakeIconElementFrom(winrt::IconSource const& iconSource);
#endif

    static void SetBinding(
        std::wstring_view const& pathString,
        winrt::DependencyObject const& target,
        winrt::DependencyProperty const& targetProperty);

    static void SetBinding(
        winrt::IInspectable const& source,
        std::wstring_view const& pathString,
        winrt::DependencyObject const& target,
        winrt::DependencyProperty const& targetProperty,
        winrt::IValueConverter const& converter = nullptr,
        winrt::BindingMode mode = winrt::BindingMode::OneWay);

    template <class ElementType>
    static void CopyVector(
        winrt::IObservableVector<ElementType> const& source,
        winrt::IObservableVector<ElementType> const& destination)
    {
        destination.Clear();

        for (auto const& element : source)
        {
            destination.Append(element);
        }
    }

    template <class ElementType>
    static void ForwardVectorChange(
        winrt::IObservableVector<ElementType> const& source,
        winrt::IObservableVector<ElementType> const& destination,
        winrt::IVectorChangedEventArgs const& args)
    {
        const uint32_t index = args.Index();

        switch (args.CollectionChange())
        {
        case winrt::CollectionChange::ItemChanged:
            destination.SetAt(index, source.GetAt(index));
            break;
        case winrt::CollectionChange::ItemInserted:
            destination.InsertAt(index, source.GetAt(index));
            break;
        case winrt::CollectionChange::ItemRemoved:
            destination.RemoveAt(index);
            break;
        case winrt::CollectionChange::Reset:
            CopyVector(source, destination);
            break;
        default:
            MUX_ASSERT(false);
        }
    }

    static winrt::ITextSelection GetRichTextSelection(winrt::RichEditBox const& richEditBox);

    static winrt::VirtualKey GetVirtualKeyFromChar(WCHAR c);

    template <typename IndexType, typename ElementType>
    static void EraseIfExists(std::map<IndexType, ElementType>& map, IndexType const& index)
    {
        const auto it = map.find(index);
        if (it != map.end())
        {
            map.erase(it);
        }
    }

    template <typename ParameterType>
    static void RaiseAutomationPropertyChangedEvent(winrt::UIElement const& element, ParameterType oldValue, ParameterType newValue)
    {
        if (winrt::AutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(element))
        {
            peer.RaisePropertyChangedEvent(
                winrt::ExpandCollapsePatternIdentifiers::ExpandCollapseStateProperty(),
                winrt::box_value(oldValue),
                winrt::box_value(newValue));
        }
    }

    static winrt::float4 RgbaColor(winrt::Color const& color);

    static winrt::CoreApplicationView TryGetCurrentCoreApplicationView();

private:
    SharedHelpers() = default;

    static bool s_isOnXboxInitialized;
    static bool s_isOnXbox;
    static bool s_isMouseModeEnabledInitialized;
    static bool s_isMouseModeEnabled;
};

// Any time you declare a flag enum, you should use this macro to provide
// operator overloads in order to make working with that flag enum a lot easier.
//
// Example usage:
//
//     enum class MyFlagEnum
//     {
//        None = 0x0000,
//        Foo = 0x0001,
//        Bar = 0x0002,
//        Baz = 0x0004,
//     };
//
//     DECLARE_FLAG_ENUM_OPERATOR_OVERLOADS(MyFlagEnum);
//
// This makes it so that, for example, instead of needing to do all this:
//
//     MyFlagEnum enum = MyFlagEnum::None;
//     enum = static_cast<int>(enum) | static_cast<int>(MyFlagEnum::Foo);
//     enum = static_cast<int>(enum) | static_cast<int>(MyFlagEnum::Bar);
//     enum = static_cast<int>(enum) | static_cast<int>(MyFlagEnum::Baz);
//
// ...you can instead just do this:
//
//     MyFlagEnum enum = MyFlagEnum::None;
//     enum |= MyFlagEnum::Foo;
//     enum |= MyFlagEnum::Bar;
//     enum |= MyFlagEnum::Baz;
//
// This is needed because the bitwise & and | operators only operate out of the box
// on built-in numeric types, so they need to be told how to interact with a flag enum.
//
#define DECLARE_FLAG_ENUM_OPERATOR_OVERLOADS(EnumType) \
inline EnumType operator | (EnumType lhs, EnumType rhs) \
{ \
    return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) | static_cast<std::underlying_type_t<EnumType>>(rhs)); \
} \
 \
inline EnumType& operator |= (EnumType& lhs, EnumType rhs) \
{ \
    lhs = static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) | static_cast<std::underlying_type_t<EnumType>>(rhs)); \
    return lhs; \
} \
 \
inline EnumType operator & (EnumType lhs, EnumType rhs) \
{ \
    return static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) & static_cast<std::underlying_type_t<EnumType>>(rhs)); \
} \
\
inline EnumType& operator &= (EnumType& lhs, EnumType rhs) \
{ \
    lhs = static_cast<EnumType>(static_cast<std::underlying_type_t<EnumType>>(lhs) & static_cast<std::underlying_type_t<EnumType>>(rhs)); \
    return lhs; \
}
