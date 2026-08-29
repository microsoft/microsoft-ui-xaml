// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>
#include <memory>
#include <map>
#include <tuple>
#include <wrl\module.h>
#include <wil\resource.h>
#include "XamlOM.WinUI.h"
#include <TestCleanupWrapper.h>
#include "XamlDiagnosticsHelper.h"
#include "RuntimeParameters.h"

using namespace Microsoft::UI::Xaml::Controls;
namespace wrl = Microsoft::WRL;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace XamlDiagnostics {

        struct TemporaryInstance final
        {
            TemporaryInstance(InstanceHandle handle, wrl::ComPtr<IXamlDiagnosticsTap> tap)
                : Handle(handle)
                , m_tap(tap)
            {
            }

            TemporaryInstance(TemporaryInstance&& other)
            {
                Handle = other.Handle;
                other.Handle = 0u;
                m_tap = std::move(other.m_tap);
            }

            TemporaryInstance& operator=(TemporaryInstance&& other)
            {
                if (Handle > 0u)
                {
                    m_tap->UnregisterInstance(Handle);
                }
                Handle = other.Handle;
                other.Handle = 0u;

                m_tap = std::move(other.m_tap);
                return *this;
            }

            TemporaryInstance(const TemporaryInstance& other) = delete;
            TemporaryInstance& operator=(const TemporaryInstance& other) = delete;

            ~TemporaryInstance()
            {
                if (Handle > 0u)
                {
                    m_tap->UnregisterInstance(Handle);
                }
            }

            InstanceHandle Handle;

            private:
            wrl::ComPtr<IXamlDiagnosticsTap> m_tap;
        };

        template <class T>
        class BaseTestClass : public WEX::TestClass<T>
        {
        protected:

            struct CollectionProperty
            {
                InstanceHandle Handle;
                CoTaskMemPtr<CollectionElementValue> Elements;
                unsigned int Count;
                std::wstring PropertyName;
            };

            InstanceHandle GetProperty(InstanceHandle object, const std::wstring& fullPropertyName)
            {
                auto propertyIndex = GetPropertyIndex(object, fullPropertyName);

                InstanceHandle propertyHandle = 0;
                LogThrow_IfFailedWithMessage(m_tap->GetProperty(object, propertyIndex, &propertyHandle),
                    WEX::Common::String().Format(L"Failed getting %s", fullPropertyName.c_str()));

                return propertyHandle;
            }

            HRESULT TryGetPropertyByIndex(InstanceHandle object, unsigned int index, _Out_ InstanceHandle* propertyValue)
            {
                return m_tap->GetProperty(object, index, propertyValue);
            }

            wil::unique_propertychainvalue GetPropertyChainValue(InstanceHandle object, const std::wstring& propertyName, BaseValueSource valueSource = BaseValueSourceDefault)
            {
                return m_tap->GetPropertyChainValue(object, propertyName, valueSource);
            }

            CollectionProperty GetCollectionProperty(InstanceHandle object, const std::wstring& fullPropertyName)
            {
                CollectionProperty collection = {};
                collection.PropertyName = fullPropertyName;
                collection.Handle = GetProperty(object, collection.PropertyName);
                collection.Count = GetCollectionCount(collection.Handle);

                if (collection.Count > 0)
                {
                    LogThrow_IfFailed(m_tap->GetCollectionElements(collection.Handle, 0, &collection.Count, &collection.Elements));
                }

                return collection;
            }

            InstanceHandle GetCollectionItem(InstanceHandle object, const std::wstring& fullPropertyName, int index)
            {
                InstanceHandle item = 0;
                WEX::Common::Throw::IfFalse(TryGetCollectionItem(object, fullPropertyName, index, item), E_NOTFOUND);
                return item;
            }

            bool TryGetCollectionItem(InstanceHandle object, const std::wstring& fullPropertyName, int index, InstanceHandle& item)
            {
                CollectionProperty collection = {};
                collection.PropertyName = fullPropertyName;
                collection.Handle = GetProperty(object, collection.PropertyName);

                unsigned int count = 1;
                bool succeeded = SUCCEEDED(m_tap->GetCollectionElements(collection.Handle, index, &count, &collection.Elements));
                if (count > 0)
                {
                    item = ih_cast(collection.Elements[0].Value);
                }
                return succeeded && count > 0;
            }

            unsigned int GetCollectionCount(InstanceHandle collection)
            {
                unsigned int value = 0;
                LogThrow_IfFailed(m_tap->GetCollectionCount(collection, &value));
                return value;
            }

            CollectionProperty GetCollectionElements(InstanceHandle object)
            {
                CollectionProperty collection = {};
                collection.Handle = object;
                collection.Count = GetCollectionCount(collection.Handle);

                if (collection.Count > 0)
                {
                    LogThrow_IfFailed(m_tap->GetCollectionElements(collection.Handle, 0, &collection.Count, &collection.Elements));
                }

                return collection;
            }

            template <typename T>
            T^ GetProperty(InstanceHandle object, const std::wstring& fullPropertyName)
            {
                InstanceHandle propertyHandle = GetProperty(object, fullPropertyName);

                wrl::ComPtr<IInspectable> valueInsp;
                LogThrow_IfFailedWithMessage(m_tap->GetIInspectableFromHandle(propertyHandle, &valueInsp), L"Failed to get handle to property");

                T^ value = nullptr;
                LogThrow_IfFailedWithMessage(valueInsp.CopyTo(__uuidof(T^), reinterpret_cast<void**>(&value)), L"Property doesn't implement interface");

                return value;
            }

            unsigned int GetPropertyIndex(InstanceHandle object, const std::wstring& fullPropertyName)
            {
                unsigned int propertyIndex = 0;
                LogThrow_IfFailedWithMessage(m_tap->GetPropertyIndex(object, fullPropertyName.c_str(), &propertyIndex),
                    WEX::Common::String().Format(L"Failed getting index for %s", fullPropertyName.c_str()));

                return propertyIndex;
            }

            IInspectable* GetCurrentDispatcher()
            {
                auto dispatcher = TestServices::WindowHelper->CurrentDispatcher;
                return reinterpret_cast<IInspectable*>(safe_cast<Platform::Object^>(dispatcher));
            }

            void SetProperty(InstanceHandle object, InstanceHandle valueToSet, const std::wstring& fullPropertyName)
            {
                unsigned int propertyIndex = 0;

                LogThrow_IfFailedWithMessage(m_tap->GetPropertyIndex(object, fullPropertyName.c_str(), &propertyIndex),
                    WEX::Common::String().Format(L"Failed getting index for %s", fullPropertyName.c_str()));

                LogThrow_IfFailedWithMessage(TrySetPropertyByIndex(object, valueToSet, propertyIndex),
                    WEX::Common::String().Format(L"Failed setting %s", fullPropertyName.c_str()));
            }

            HRESULT TrySetPropertyByIndex(InstanceHandle object, InstanceHandle valueToSet, unsigned int index)
            {
                return m_tap->SetProperty(object, valueToSet, index);
            }

            void ClearProperty(InstanceHandle object, const std::wstring& fullPropertyName)
            {
                unsigned int propertyIndex = 0;

                LogThrow_IfFailedWithMessage(m_tap->GetPropertyIndex(object, fullPropertyName.c_str(), &propertyIndex),
                    WEX::Common::String().Format(L"Failed getting index for %s", fullPropertyName.c_str()));

                LogThrow_IfFailedWithMessage(m_tap->ClearProperty(object, propertyIndex),
                    WEX::Common::String().Format(L"Failed clearing %s", fullPropertyName.c_str()));
            }

            HRESULT TryClearPropertyByIndex(InstanceHandle object, unsigned int propertyIndex)
            {
                return m_tap->ClearProperty(object, propertyIndex);
            }

            InstanceHandle CloneAndModifySingleSetterStyle(
                InstanceHandle style,
                const std::wstring& propertyName,
                InstanceHandle newSetterValue)
            {
                InstanceHandle targetTypeHandle = GetProperty(style, L"Microsoft.UI.Xaml.Style.TargetType");

                // Create new style and set the target type on the new style
                InstanceHandle newStyleHandle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.Style", GetCurrentDispatcher());
                SetProperty(newStyleHandle, targetTypeHandle, L"Microsoft.UI.Xaml.Style.TargetType");

                // Create a new setter and set the property value on it and add it to the setter collection of the style
                InstanceHandle newSetterHandle = CreateSetterWithProperty(newSetterValue, propertyName);

                // finally, get the setter collection and append the new setter to it
                auto setters = GetCollectionProperty(newStyleHandle, L"Microsoft.UI.Xaml.Style.Setters");
                LogThrow_IfFailedWithMessage(m_tap->AddChild(setters.Handle, newSetterHandle, setters.Count), L"Failed adding setter to collection");

                return newStyleHandle;
            }

            InstanceHandle CreateStyle(const std::wstring& targetType)
            {
                InstanceHandle newStyleHandle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.Style", GetCurrentDispatcher());
                InstanceHandle boxHandle = CreateTypeName(targetType);
                SetProperty(newStyleHandle, boxHandle, L"Microsoft.UI.Xaml.Style.TargetType");

                return newStyleHandle;
            }

            InstanceHandle CreateTypeName(const std::wstring& typeNameStr)
            {
                // Our API's don't support this so go through WinRT and create a box
                // around the typename struct.
                ::Windows::UI::Xaml::Interop::TypeName typeName;
                typeName.Name = ref new Platform::String(typeNameStr.c_str());
                typeName.Kind = wxaml_interop::TypeKind::Metadata;

                auto typeNameBox = ref new Platform::Box<::Windows::UI::Xaml::Interop::TypeName>(typeName);
                return ih_cast(typeNameBox);
            }

            void AddSetterToStyle(InstanceHandle styleHandle, InstanceHandle setterHandle)
            {
                AppendToCollection(styleHandle, L"Microsoft.UI.Xaml.Style.Setters", setterHandle);
            }

            void AddTriggerToVisualState(InstanceHandle visualState, InstanceHandle trigger)
            {
                AppendToCollection(visualState, L"Microsoft.UI.Xaml.VisualState.StateTriggers", trigger);
            }

            void AppendToCollection(InstanceHandle parent, const std::wstring& collectionProperty, InstanceHandle itemToAdd)
            {
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                auto collection = GetCollectionProperty(parent, collectionProperty);
                LOG_OUTPUT(L"Adding item to %s", collectionProperty.c_str());
                InsertIntoCollection(collection.Handle, itemToAdd, collection.Count);
            }

            void InsertIntoCollection(InstanceHandle parent, const std::wstring& collectionProperty, InstanceHandle itemToAdd, unsigned int index)
            {
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                auto collection = GetCollectionProperty(parent, collectionProperty);
                WEX::Common::Throw::If(index > collection.Count, E_INVALIDARG, L"Index out of range");
                InsertIntoCollection(collection.Handle, itemToAdd, index);
            }

            void InsertIntoCollection(InstanceHandle collection, InstanceHandle itemToAdd, unsigned int index)
            {
                LogThrow_IfFailedWithMessage(m_tap->AddChild(collection, itemToAdd, index), L"Failed inserting into collection");
            }

            void RemoveFromCollection(InstanceHandle parent, const std::wstring& collectionProperty, unsigned int index)
            {
                auto collection = GetCollectionProperty(parent, collectionProperty);
                WEX::Common::Throw::If(index > collection.Count, E_INVALIDARG, L"Index out of range");
                RemoveFromCollection(collection.Handle, index);
            }

            void RemoveFromCollection(InstanceHandle collection, unsigned int index)
            {
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                LogThrow_IfFailedWithMessage(m_tap->RemoveChild(collection, index), L"Failed removing from collection");
            }

            void ClearCollection(InstanceHandle parent, const std::wstring& collectionProperty)
            {
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                auto collection = GetCollectionProperty(parent, collectionProperty);
                LogThrow_IfFailedWithMessage(m_tap->ClearChildren(collection.Handle), L"Failed clearing collection");
            }

            void ClearCollection(InstanceHandle collection)
            {
                LogThrow_IfFailedWithMessage(m_tap->ClearChildren(collection), L"Failed clearing collection");
            }

            InstanceHandle CreateBinding(const std::wstring& elementName, const std::wstring& path, const std::wstring& fallbackValue = L"")
            {
                auto newBindingHandle = CreateInstance(L"Microsoft.UI.Xaml.Data.Binding");

                if (!elementName.empty())
                {
                    auto nameHandle = CreateInstance(L"Windows.Foundation.String", elementName);
                    SetProperty(newBindingHandle, nameHandle, L"Microsoft.UI.Xaml.Data.Binding.ElementName");
                }

               if (!fallbackValue.empty())
               {
                    auto fallbackValueHandle = CreateInstance(L"Windows.Foundation.String", fallbackValue);
                    SetProperty(newBindingHandle, fallbackValueHandle, L"Microsoft.UI.Xaml.Data.Binding.FallbackValue");
               }

                auto pathHandle = CreateInstance(L"Windows.Foundation.String", path);
                SetProperty(newBindingHandle, pathHandle, L"Microsoft.UI.Xaml.Data.Binding.Path");

                return newBindingHandle;
            }

            InstanceHandle CreateSetterWithTarget(InstanceHandle newSetterValue, InstanceHandle targetHandle, const std::wstring& propertyPath)
            {
                InstanceHandle newSetterHandle = CreateSetterWithValueOnly(newSetterValue);

                SetTargetOnSetter(newSetterHandle, targetHandle, propertyPath);
                return newSetterHandle;
            }

            void SetTargetOnSetter(InstanceHandle setterHandle, InstanceHandle targetHandle, const std::wstring& propertyPath)
            {
                // Create the TargetPropertyPath
                InstanceHandle targetPropertyPath = m_tap->CreateInstance(L"Microsoft.UI.Xaml.TargetPropertyPath", GetCurrentDispatcher());

                // Set the target of the TargetPropertyPath
                SetProperty(targetPropertyPath, targetHandle, L"Microsoft.UI.Xaml.TargetPropertyPath.Target");

                // Create a PropertyPath and set it to the Path of the TargetPropertyPath
                LOG_OUTPUT(L"Creating property path: %s", propertyPath.c_str());
                InstanceHandle propertyPathHandle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.PropertyPath", propertyPath.c_str(), GetCurrentDispatcher());

                // Set the path on the TargetPropertyPath
                SetProperty(targetPropertyPath, propertyPathHandle, L"Microsoft.UI.Xaml.TargetPropertyPath.Path");

                // Finally set the Target on the Setter
                SetProperty(setterHandle, targetPropertyPath, L"Microsoft.UI.Xaml.Setter.Target");
            }

            InstanceHandle CreateSetterWithProperty(InstanceHandle newSetterValue, const std::wstring& propertyName)
            {
                InstanceHandle newSetterHandle = CreateSetterWithValueOnly(newSetterValue);

                unsigned int propertyIndex = 0;
                LogThrow_IfFailedWithMessage(m_tap->GetPropertyIndex(newSetterHandle, L"Microsoft.UI.Xaml.Setter.Property", &propertyIndex), L"Failed getting index for Microsoft.UI.Xaml.Setter.Property");

                LOG_OUTPUT(L"Creating DependencyProperty: %s", propertyName.c_str());
                InstanceHandle propertyHandle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.DependencyProperty", propertyName.c_str(), GetCurrentDispatcher());

                LogThrow_IfFailedWithMessage(TrySetPropertyByIndex(newSetterHandle, propertyHandle, propertyIndex), L"Failed to set Microsoft.UI.Xaml.Setter.Value");

                return newSetterHandle;
            }

            InstanceHandle CreateSetterWithValueOnly(InstanceHandle newSetterValue)
            {
                InstanceHandle newSetterHandle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.Setter", GetCurrentDispatcher());

                if (newSetterValue > 0)
                {
                    SetProperty(newSetterHandle, newSetterValue, L"Microsoft.UI.Xaml.Setter.Value");
                }

                return newSetterHandle;
            }

            InstanceHandle CreateAdaptiveTrigger(const std::wstring& propertyName, double value)
            {
                InstanceHandle newTriggerHandle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.AdaptiveTrigger", GetCurrentDispatcher());

                SetPropertyOnTrigger(newTriggerHandle, propertyName, value);

                return newTriggerHandle;
            }

            InstanceHandle CreateAnimation(const std::wstring& animationType, const std::wstring& targetName, const std::wstring& targetProperty, InstanceHandle toValue)
            {
                InstanceHandle animation = CreateAnimation(animationType, targetName, targetProperty);

                auto className = std::wstring(L"Microsoft.UI.Xaml.Media.Animation.").append(animationType);
                SetProperty(animation, toValue, className.append(L".To"));

                return animation;
            }

            InstanceHandle CreateAnimation(const std::wstring& animationType, const std::wstring& targetName, const std::wstring& targetProperty)
            {
                InstanceHandle animation = CreateAnimationWithTarget(animationType, targetName);

                InstanceHandle targetPropertyHandle = CreateInstance(L"Windows.Foundation.String", targetProperty);
                SetProperty(animation, targetPropertyHandle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.TargetProperty");

                return animation;
            }

            InstanceHandle CreateAnimationWithTarget(const std::wstring& animationType, const std::wstring& targetName)
            {
                auto className = std::wstring(L"Microsoft.UI.Xaml.Media.Animation.").append(animationType);
                InstanceHandle animation = CreateInstance(className);

                InstanceHandle targetNameHandle = CreateInstance(L"Windows.Foundation.String", targetName);
                SetProperty(animation, targetNameHandle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.TargetName");

                return animation;
            }

            InstanceHandle CreateControlWithBackground(const std::wstring& controlType, const std::wstring& backgroundColor, const std::wstring& name = std::wstring())
            {
                auto controlFullType = std::wstring(L"Microsoft.UI.Xaml.Controls.").append(controlType);
                InstanceHandle control = m_tap->CreateInstance(controlFullType, GetCurrentDispatcher());
                if (!name.empty())
                {
                    InstanceHandle nameHandle = m_tap->CreateInstance(L"Windows.Foundation.String", name, GetCurrentDispatcher());
                    SetProperty(control, nameHandle, L"Microsoft.UI.Xaml.FrameworkElement.Name");
                }

                InstanceHandle background = m_tap->CreateInstance(L"Microsoft.UI.Xaml.Media.Brush", backgroundColor, GetCurrentDispatcher());
                SetProperty(control, background, L"Microsoft.UI.Xaml.Controls.Control.Background");

                return control;
            }

            void SetPropertyOnTrigger(InstanceHandle triggerHandle, const std::wstring& propertyName, double value)
            {
                auto fullPropertyName = std::wstring(L"Microsoft.UI.Xaml.AdaptiveTrigger.").append(propertyName);

                InstanceHandle newValue = m_tap->CreateInstance(L"Windows.Foundation.Double", std::to_wstring(value).c_str(), GetCurrentDispatcher());

                SetProperty(triggerHandle, newValue, fullPropertyName);

                // Wait for animations and whatnot to update
                TestServices::WindowHelper->WaitForIdle();
            }

            CollectionProperty GetVisualStateProperty(InstanceHandle visualState, const std::wstring& propertyName)
            {
                CollectionProperty vsProperty = {};
                vsProperty.PropertyName = std::wstring(L"Microsoft.UI.Xaml.VisualState.").append(propertyName);
                vsProperty.Handle = GetProperty(visualState, vsProperty.PropertyName);

                if (propertyName != L"Storyboard")
                {
                    vsProperty.Count = GetCollectionCount(vsProperty.Handle);
                    LogThrow_IfFailed(m_tap->GetCollectionElements(vsProperty.Handle, 0, &vsProperty.Count, &vsProperty.Elements));
                }

                return vsProperty;
            }

            InstanceHandle GetItemInVisualStateProperty(InstanceHandle visualState, const std::wstring& propertyName, int index)
            {
                WEX::Common::Throw::If(propertyName == L"Storyboard", E_INVALIDARG);
                return GetCollectionItem(visualState, std::wstring(L"Microsoft.UI.Xaml.VisualState.").append(propertyName), index);
            }

            bool TryGetItemInVisualStateProperty(InstanceHandle visualState, const std::wstring& propertyName, int index, InstanceHandle& elementHandle)
            {
                WEX::Common::Throw::If(propertyName == L"Storyboard", E_INVALIDARG);
                return TryGetCollectionItem(visualState, std::wstring(L"Microsoft.UI.Xaml.VisualState.").append(propertyName), index, elementHandle);
            }

            CollectionProperty GetVisualStateGroupsForElement(InstanceHandle element)
            {
                auto visualStateGroupCollection = GetCollectionProperty(element, L"Microsoft.UI.Xaml.VisualStateManager.VisualStateGroups");

                if (visualStateGroupCollection.Count > 0)
                {
                    VERIFY_IS_TRUE(wcscmp(visualStateGroupCollection.Elements[0].ValueType, L"Microsoft.UI.Xaml.VisualStateGroup") == 0);
                }

                return visualStateGroupCollection;
            }

            InstanceHandle GetNamedVisualStateGroupForElement(InstanceHandle element, const std::wstring& desiredName)
            {
                auto groups = GetVisualStateGroupsForElement(element);

                for (unsigned int i = 0; i < groups.Count; i++)
                {
                    InstanceHandle groupHandle = std::stoll(groups.Elements[i].Value);
                    auto namePropertyValue = GetProperty<wf::IPropertyValue>(groupHandle, L"Microsoft.UI.Xaml.VisualStateGroup.Name");
                    Platform::String^ name = namePropertyValue->GetString();
                    if (desiredName.compare(name->Data()) == 0)
                    {
                        return groupHandle;
                    }
                }

                WEX::Common::Throw::Exception(E_NOTFOUND, WEX::Common::String().Format(L"Could not find VisualStateGroup with name: %s", desiredName.c_str()));
                return 0;
            }

            CollectionProperty GetStatesInVisualStateGroup(InstanceHandle visualStateGroup)
            {
                // Get the states for the visual state group
                auto visualStateCollection = GetCollectionProperty(visualStateGroup, L"Microsoft.UI.Xaml.VisualStateGroup.States");

                if (visualStateCollection.Count > 0)
                {
                    VERIFY_IS_TRUE(wcscmp(visualStateCollection.Elements[0].ValueType, L"Microsoft.UI.Xaml.VisualState") == 0);
                }

                return visualStateCollection;
            }

            InstanceHandle GetStateInVisualStateGroup(InstanceHandle visualStateGroup, int index)
            {
                // Get the states for the visual state group
                return GetCollectionItem(visualStateGroup, L"Microsoft.UI.Xaml.VisualStateGroup.States", index);
            }

            bool TryGetStateInVisualStateGroup(InstanceHandle visualStateGroup, int index, InstanceHandle& state)
            {
                // Get the states for the visual state group
                return TryGetCollectionItem(visualStateGroup, L"Microsoft.UI.Xaml.VisualStateGroup.States", index, state);
            }

            InstanceHandle GetNamedVisualStateInGroup(InstanceHandle visualStateGroup, const std::wstring& desiredName)
            {
                InstanceHandle visualState = 0;
                if (!TryGetNamedVisualStateInGroup(visualStateGroup, desiredName, visualState))
                {
                    WEX::Common::Throw::Exception(E_NOTFOUND, WEX::Common::String().Format(L"Could not find VisualState with name: %s", desiredName.c_str()));
                }
                return visualState;
            }

            bool TryGetNamedVisualStateInGroup(InstanceHandle visualStateGroup, const std::wstring& desiredName, InstanceHandle& visualState)
            {
                auto states = GetStatesInVisualStateGroup(visualStateGroup);

                for (unsigned int i = 0; i < states.Count; i++)
                {
                    InstanceHandle stateHandle = ih_cast(states.Elements[i].Value);
                    auto namePropertyValue = GetProperty<wf::IPropertyValue>(stateHandle, L"Microsoft.UI.Xaml.VisualState.Name");
                    Platform::String^ name = namePropertyValue->GetString();
                    if (desiredName.compare(name->Data()) == 0)
                    {
                        visualState = stateHandle;
                        return true;
                    }
                }

                return false;
            }

            InstanceHandle AddNewVisualStateToGroup(InstanceHandle visualStateGroup, const std::wstring& visualStateName = std::wstring())
            {
                // Get the states for the visual state group
                auto visualStateCollection = GetCollectionProperty(visualStateGroup, L"Microsoft.UI.Xaml.VisualStateGroup.States");
                if (visualStateCollection.Count > 0)
                {
                    VERIFY_IS_TRUE(wcscmp(visualStateCollection.Elements[0].ValueType, L"Microsoft.UI.Xaml.VisualState") == 0);
                }

                InstanceHandle visualStateHandle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.VisualState", GetCurrentDispatcher());

                if (!visualStateName.empty())
                {
                    InstanceHandle propertyHandle = m_tap->CreateInstance(L"Windows.Foundation.String", visualStateName.c_str(), GetCurrentDispatcher());
                    SetProperty(visualStateHandle, propertyHandle, L"Microsoft.UI.Xaml.VisualState.Name");
                }

                LogThrow_IfFailed(m_tap->AddChild(visualStateCollection.Handle, visualStateHandle, visualStateCollection.Count));

                return visualStateHandle;
            }

            void RemoveVisualStateFromGroup(InstanceHandle visualStateGroup, unsigned int index)
            {
                // Get the states for the visual state group
                InstanceHandle statesHandle = GetProperty(visualStateGroup, L"Microsoft.UI.Xaml.VisualStateGroup.States");
                LogThrow_IfFailedWithMessage(m_tap->RemoveChild(statesHandle, index), L"Failed to remove VisualState");
            }

            void AddSetterToVisualState(InstanceHandle setter, InstanceHandle visualState, unsigned int index)
            {
                // Get the Setters property
                InstanceHandle setters = GetProperty(visualState, L"Microsoft.UI.Xaml.VisualState.Setters");
                LogThrow_IfFailedWithMessage(m_tap->AddChild(setters, setter, index), L"Failed adding Setter to VisualState");
            }

            void AddAnimationToStoryboard(
                InstanceHandle visualStateRoot,
                const std::wstring& vsGroup,
                const std::wstring& vsName,
                InstanceHandle animationHandle)
            {

                // Get the visual group and create the animation and add it to the storyboard.
                auto visualGroupCollection = GetNamedVisualStateGroupForElement(visualStateRoot, vsGroup);
                InstanceHandle visualState = 0u;
                if (!TryGetNamedVisualStateInGroup(visualGroupCollection, vsName, visualState))
                {
                    visualState = AddNewVisualStateToGroup(visualGroupCollection, vsName);
                }

                AddAnimationToStoryboard(visualState, animationHandle);
            }

            InstanceHandle GetAnimationFromStoryboard(
                InstanceHandle visualStateRoot,
                const std::wstring& vsGroup,
                const std::wstring& vsName,
                int index = 0)
            {
                auto visualGroupCollection = GetNamedVisualStateGroupForElement(visualStateRoot, vsGroup);
                InstanceHandle visualState = GetNamedVisualStateInGroup(visualGroupCollection, vsName);
                auto storyboard = GetVisualStateProperty(visualState, L"Storyboard");
                return GetCollectionItem(storyboard.Handle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.Children", index);
            }

            void AddAnimationToStoryboard(
                InstanceHandle visualStateRoot,
                const int vsGroupIndex,
                const int vsIndex,
                InstanceHandle animationHandle)
            {
                // Get the visual group and create the animation and add it to the storyboard.
                auto visualGroupCollections = GetVisualStateGroupsForElement(visualStateRoot);
                auto visualStateGroup = ih_cast(visualGroupCollections.Elements[vsGroupIndex].Value);

                InstanceHandle visualState = 0u;
                if (!TryGetStateInVisualStateGroup(visualStateGroup, vsIndex, visualState))
                {
                    visualState = CreateInstance(L"Microsoft.UI.Xaml.VisualState");

                    InsertIntoCollection(visualStateGroup, L"Microsoft.UI.Xaml.VisualStateGroup.States", visualState, vsIndex);
                }

                AddAnimationToStoryboard(visualState, animationHandle);
            }

            void AddAnimationToStoryboard(
                InstanceHandle visualState,
                InstanceHandle animationHandle)
            {
                auto storyboard = GetVisualStateProperty(visualState, L"Storyboard");

                // If there is no storyboard, create one and insert it.
                if (storyboard.Handle == 0u)
                {
                    storyboard.Handle = m_tap->CreateInstance(L"Microsoft.UI.Xaml.Media.Animation.Storyboard", GetCurrentDispatcher());
                    SetProperty(visualState, storyboard.Handle, L"Microsoft.UI.Xaml.VisualState.Storyboard");
                }

                CollectionProperty storyBoardChildren = GetCollectionProperty(storyboard.Handle, L"Microsoft.UI.Xaml.Media.Animation.Storyboard.Children");

                LogThrow_IfFailedWithMessage(m_tap->AddChild(storyBoardChildren.Handle, animationHandle, storyBoardChildren.Count), L"Failed adding animation to storyboard");
            }

            template<typename T>
            T^ GetFromInstanceHandle(InstanceHandle handle)
            {
                wrl::ComPtr<IInspectable> insp;
                LogThrow_IfFailed(m_tap->GetIInspectableFromHandle(handle, &insp));
                auto obj = reinterpret_cast<Platform::Object^>(insp.Get());
                return safe_cast<T^>(obj);
            }

            InstanceHandle GetInstanceHandle(Platform::Object^ object)
            {
                InstanceHandle handle = 0;
                LogThrow_IfFailed(m_tap->GetHandleFromIInspectable(reinterpret_cast<IInspectable*>(object), &handle));
                return handle;
            }

            template<typename T>
            T^ ih_cast(InstanceHandle handle)
            {
                return GetFromInstanceHandle<T>(handle);
            }

            InstanceHandle ih_cast(Platform::Object^ object)
            {
                return GetInstanceHandle(object);
            }

            InstanceHandle ih_cast(PCWSTR value)
            {
                return std::stoll(value);
            }

            bool EnsureTapLoaded()
            {
                LOG_OUTPUT(L"*** EnsureTapLoaded ***");
                WEX::Common::String testName;

                bool isWPFRun = false;
                WEX::Common::String value;
                if (SUCCEEDED(WEX::TestExecution::RuntimeParameters::TryGetValue(L"HostingMode", value)))
                {
                    if (value == L"WPF")
                    {
                        isWPFRun = false;
                    }
                }
                if (m_connectionHelper == nullptr || isWPFRun)
                {
                    m_connectionHelper.reset(new XamlDiagnosticsHelper());
                    m_tap = m_connectionHelper->Connect();
                }

                RunOnUIThread([&]
                {
                    LogThrow_IfFailed(m_tap->UpdateMainDispatcherQueue());
                });

                return true;


                return true;
            }

            ::Windows::UI::Color GetColorProperty(InstanceHandle control, const std::wstring& prop, BaseValueSource source)
            {
                auto backgroundProp = GetPropertyChainValue(control, prop, source);
                if ((backgroundProp.MetadataBits & MetadataBit::IsValueBindingExpression) == MetadataBit::IsValueBindingExpression)
                {
                    backgroundProp = GetPropertyChainValue(std::stoll(backgroundProp.Value), L"EvaluatedValue", BaseValueSourceLocal);
                }
                ::Windows::UI::Color color;
                RunOnUIThread([&] {
                    auto brush = GetFromInstanceHandle<xaml_media::SolidColorBrush>(std::stoll(backgroundProp.Value));
                    color = brush->Color;
                });

                return color;
            }

            void ResolveResource(
                InstanceHandle dependencyObject,
                const std::wstring& resourceKey,
                unsigned propertyIndex,
                ResourceType resourceType)
            {
                LOG_OUTPUT(L"Resolving: %s", resourceKey.c_str());
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                m_tap->ResolveResource(dependencyObject, resourceKey, propertyIndex, resourceType);
            }

            void ResolveResource(
                InstanceHandle dependencyObject,
                const std::wstring& resourceKey,
                const std::wstring& propertyName,
                ResourceType resourceType)
            {
                auto propertyIndex = GetPropertyIndex(dependencyObject, propertyName);
                LOG_OUTPUT(L"Resolving: %s for %s", resourceKey.c_str(), propertyName.c_str());
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                m_tap->ResolveResource(dependencyObject, resourceKey, propertyIndex, resourceType);
            }

            void ReplaceResource(
                InstanceHandle dictionary,
                InstanceHandle resource,
                const std::wstring& resourceKey)
            {
                LOG_OUTPUT(L"Replacing resource %s", resourceKey.c_str());
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                auto keyText = CreateInstance(L"Windows.Foundation.String", resourceKey.c_str());
                LogThrow_IfFailedWithMessage(m_tap->ReplaceResource(dictionary, keyText, resource),
                    WEX::Common::String().Format(L"Failed replacing key %s", resourceKey.c_str()));
            }

            InstanceHandle GetDictionaryItem(
                InstanceHandle dictionary,
                const std::wstring& resourceKey)
            {
                return m_tap->GetDictionaryItem(dictionary, resourceKey);
            }

            InstanceHandle GetItemFromElementResources(
                const VisualElement& element,
                const std::wstring& resourceKey)
            {
                 auto resources = GetProperty(element.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
                 return GetDictionaryItem(resources, resourceKey);
            }

            InstanceHandle GetImplicitStyle(
                InstanceHandle dictionary,
                const std::wstring& resourceKey)
            {
                InstanceHandle style = m_tap->GetImplicitStyle(dictionary, resourceKey);
                return style;
            }

            InstanceHandle GetImplicitStyleFromElementResources(
                const VisualElement& element,
                const std::wstring& resourceKey)
            {
                 auto resources = GetProperty(element.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
                 return GetImplicitStyle(resources, resourceKey);
            }

            void AddDictionaryItem(
                InstanceHandle dictionary,
                const std::wstring& resourceKey,
                InstanceHandle item)
            {
                LOG_OUTPUT(L"Adding dictionary item %s", resourceKey.c_str());
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                m_tap->AddDictionaryItem(dictionary, item, resourceKey);
            }

            void AddImplicitStyle(
                InstanceHandle dictionary,
                InstanceHandle item)
            {
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                m_tap->AddDictionaryItem(dictionary, item);
            }

            void AddItemToElementResources(
                const VisualElement& element,
                const std::wstring& resourceKey,
                InstanceHandle item)
            {
                auto resources = GetProperty(element.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
                AddDictionaryItem(resources, resourceKey, item);
            }

            void AddImplicitStyleToElementResources(
                const VisualElement& element,
                InstanceHandle item)
            {
                auto resources = GetProperty(element.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
                AddImplicitStyle(resources, item);
            }

            void RemoveDictionaryItem(
                InstanceHandle dictionary,
                const std::wstring& resourceKey)
            {
                LOG_OUTPUT(L"Removing dictionary item %s", resourceKey.c_str());
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                m_tap->RemoveDictionaryItem(dictionary, resourceKey);
            }

            void RemoveImplicitStyle(
                InstanceHandle dictionary,
                const std::wstring& typeName)
            {
                LOG_OUTPUT(L"Removing implicit style for type %s", typeName.c_str());
                auto waitForIdle = wil::scope_exit([] { TestServices::WindowHelper->WaitForIdle(); });
                auto typeNameHandle = CreateTypeName(typeName);
                m_tap->RemoveDictionaryItem(dictionary, typeNameHandle);
            }

            void RemoveItemFromElementResources(
                const VisualElement& element,
                const std::wstring& resourceKey)
            {
                auto resources = GetProperty(element.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
                RemoveDictionaryItem(resources, resourceKey);
            }

            void RenameItemInElementResources(
                const VisualElement& element,
                const std::wstring& oldKey,
                const std::wstring& newKey)
            {
                // This is how VS renames items in dictionaries
                auto item = GetItemFromElementResources(element, oldKey);
                RemoveItemFromElementResources(element, oldKey);
                AddItemToElementResources(element, newKey, item);
            }

            void RemoveImplicitStyleFromElementResources(
                const VisualElement& element,
                const std::wstring& resourceKey)
            {
                auto resources = GetProperty(element.Handle, L"Microsoft.UI.Xaml.FrameworkElement.Resources");
                RemoveImplicitStyle(resources, resourceKey);
            }

            InstanceHandle CreateInstance(
                const std::wstring& type)
            {
                LOG_OUTPUT(L"Creating %s", type.c_str());
                return m_tap->CreateInstance(type, GetCurrentDispatcher());
            }

            InstanceHandle CreateInstance(
                const std::wstring& type,
                const std::wstring& value)
            {
                LOG_OUTPUT(L"Creating %s from '%s'", type.c_str(), value.c_str());
                return m_tap->CreateInstance(type, value, GetCurrentDispatcher());
            }

            TemporaryInstance CreateTemporaryInstance(
               const std::wstring& type,
                const std::wstring& value)
            {
                return TemporaryInstance(CreateInstance(type, value), m_tap);
            }

            TemporaryInstance CreateTemporaryInstance(
               const std::wstring& type)
            {
                return TemporaryInstance(CreateInstance(type), m_tap);
            }

            InstanceHandle GetApplicationResources()
            {
                InstanceHandle appResources = 0;
                RunOnUIThread([&]
                {
                    wrl::ComPtr<IInspectable> app;
                    VERIFY_SUCCEEDED(m_tap->GetApplication(&app));

                    InstanceHandle appHandle = 0;
                    VERIFY_SUCCEEDED(m_tap->GetHandleFromIInspectable(app.Get(), &appHandle));

                    xaml::Application^ application;
                    LogThrow_IfFailed(app.CopyTo(__uuidof(xaml::Application^), reinterpret_cast<void**>(&application)));

                    auto resources = application->Resources;
                    appResources = GetInstanceHandle(resources);
                });
                return appResources;
            }

            std::wstring GetColorName(::Windows::UI::Color color)
            {
                Platform::String^ colorString = nullptr;
                Common::RunOnUIThread([&] {
                    colorString = Microsoft::UI::Xaml::ColorDisplayNameHelper::ToDisplayName(color);
                });

                return std::wstring(colorString->Data());
            }

            std::unique_ptr<Microsoft::UI::Xaml::Tests::Common::XamlDiagnosticsHelper> m_connectionHelper;
            wrl::ComPtr<IXamlDiagnosticsTap> m_tap;
        };

    }
} } } } }
