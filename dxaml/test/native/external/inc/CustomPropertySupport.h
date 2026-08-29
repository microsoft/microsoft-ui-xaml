// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <map>

namespace Microsoft { namespace UI { namespace Xaml {
            namespace Tests {
                namespace Common {

                    delegate Platform::Object^ PropertyGetValue(Platform::Object^ target);

                    delegate Platform::Object^ PropertyGetIndexedValue(Platform::Object^ target, Platform::Object^ index);

                    delegate void PropertySetValue(Platform::Object^ target, Platform::Object^ value);

                    delegate void PropertySetIndexedValue(Platform::Object^ target, Platform::Object^ value, Platform::Object^ index);

                    // Wrap the bare essentials around being able to access a property
                    ref class PropertyInfo sealed
                    {
                        PropertyGetValue^ propertyGetValue_;
                        PropertySetValue^ propertySetValue_;
                        PropertyGetIndexedValue^ propertyGetIndexedValue_;
                        PropertySetIndexedValue^ propertySetIndexedValue_;

                    public:
                        PropertyInfo(Platform::String^ name,
                            ::Windows::UI::Xaml::Interop::TypeName typeName,
                            PropertyGetIndexedValue^ propertyGetIndexedValue,
                            PropertySetIndexedValue^ propertySetIndexedValue)
                        {
                            Name = name;

                            Type = typeName;

                            propertyGetIndexedValue_ = propertyGetIndexedValue;
                            propertySetIndexedValue_ = propertySetIndexedValue;
                        }

                        PropertyInfo(Platform::String^ name,
                            ::Windows::UI::Xaml::Interop::TypeName typeName,
                            PropertyGetValue^ propertyGetValue,
                            PropertySetValue^ propertySetValue)
                        {
                            Name = name;

                            Type = typeName;

                            propertyGetValue_ = propertyGetValue;
                            propertySetValue_ = propertySetValue;
                            propertyGetIndexedValue_ = nullptr;
                            propertySetIndexedValue_ = nullptr;
                        }

                        property Platform::String^ Name;

                        property ::Windows::UI::Xaml::Interop::TypeName Type;

                        property bool CanRead
                        {
                            bool get()
                            {
                                return propertyGetValue_ != nullptr;
                            }
                        }

                        property bool CanWrite
                        {
                            bool get()
                            {
                                return propertySetValue_ != nullptr;
                            }
                        }

                        Platform::Object^ GetValue(Object^ target)
                        {
                            return propertyGetValue_(target);
                        }

                        void SetValue(Platform::Object^ target, Platform::Object^ value)
                        {
                            return propertySetValue_(target, value);
                        }

                        Platform::Object^ GetIndexedValue(Platform::Object^ target, Platform::Object^ index)
                        {
                            return propertyGetIndexedValue_(target, index);
                        }

                        void SetIndexedValue(Platform::Object^ target, Platform::Object^ value, Platform::Object^ index)
                        {
                            return propertySetIndexedValue_(target, value, index);
                        }
                    };

                    ref class CustomProperty sealed : public Microsoft::UI::Xaml::Data::ICustomProperty
                    {
                        PropertyInfo^ propertyInfo_;

                    public:
                        CustomProperty(PropertyInfo^ propertyInfo)
                        {
                            propertyInfo_ = propertyInfo;
                        }

                        virtual property ::Windows::UI::Xaml::Interop::TypeName Type
                        {
                            virtual ::Windows::UI::Xaml::Interop::TypeName get()
                            {
                                return propertyInfo_->Type;
                            }
                        }

                        virtual property Platform::String^ Name
                        {
                            virtual Platform::String^ get()
                            {
                                return propertyInfo_->Name;
                            }
                        }

                        virtual property bool CanRead
                        {
                            virtual bool get()
                            {
                                return propertyInfo_->CanRead;
                            }
                        }

                        virtual property bool CanWrite
                        {
                            virtual bool get()
                            {
                                return propertyInfo_->CanWrite;
                            }
                        }

                        virtual Platform::Object^ GetValue(Platform::Object^ target)
                        {
                            return propertyInfo_->GetValue(target);
                        }

                        virtual void SetValue(Platform::Object^ target, Platform::Object^ value)
                        {
                            return propertyInfo_->SetValue(target, value);
                        }

                        virtual void SetIndexedValue(Platform::Object^ target, Platform::Object^ value, Platform::Object^ index)
                        {
                            return propertyInfo_->SetIndexedValue(target, value, index);
                        }

                        virtual Platform::Object^ GetIndexedValue(Platform::Object^ target, Platform::Object^ index)
                        {
                            return propertyInfo_->GetIndexedValue(target, index);
                        }
                    };

                    ref class CustomPropertyProviderBase abstract
                        : public Microsoft::UI::Xaml::Data::ICustomPropertyProvider
                        , public Microsoft::UI::Xaml::Data::INotifyPropertyChanged
                    {
                        std::map<Platform::String^, Microsoft::UI::Xaml::Data::ICustomProperty^> customPropertyMap_;
                        bool customPropertiesAdded_;

                    public:
                        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name)
                        {
                            if (!customPropertiesAdded_)
                            {
                                AddCustomProperties();
                                customPropertiesAdded_ = true;
                            }

                            auto it = customPropertyMap_.find(name);
                            return it == customPropertyMap_.end() ? nullptr : it->second;
                        }

                        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName typeName)
                        {
                            if (!customPropertiesAdded_)
                            {
                                AddCustomProperties();
                                customPropertiesAdded_ = true;
                            }

                            auto it = customPropertyMap_.find(name);
                            return it == customPropertyMap_.end() ? nullptr : it->second;
                        }

                        virtual Platform::String^ GetStringRepresentation()
                        {
                            throw ref new Platform::NotImplementedException();
                        }

                        virtual property ::Windows::UI::Xaml::Interop::TypeName Type
                        {
                            ::Windows::UI::Xaml::Interop::TypeName get()
                            {
                                ::Windows::UI::Xaml::Interop::TypeName temp;
                                temp.Name = L"CustomPropertyProviderBase";
                                temp.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Primitive;
                                return temp;
                            }
                        }

                        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

                    protected:
                        void AddCustomProperty(
                            Platform::String^ name,
                            ::Windows::UI::Xaml::Interop::TypeName typeName,
                            PropertyGetValue^ propertyGetValue,
                            PropertySetValue^ propertySetValue)
                        {
                            customPropertyMap_[name] = ref new CustomProperty(
                                ref new PropertyInfo(name, typeName, propertyGetValue, propertySetValue));
                        }

                        void AddCustomIndexedProperty(
                            Platform::String^ name,
                            ::Windows::UI::Xaml::Interop::TypeName typeName,
                            PropertyGetIndexedValue^ propertyGetValue,
                            PropertySetIndexedValue^ propertySetValue)
                        {
                            customPropertyMap_[name] = ref new CustomProperty(
                                ref new PropertyInfo(name, typeName, propertyGetValue, propertySetValue));
                        }

                        virtual void AddCustomProperties() = 0;

                        void FirePropertyChanged(Platform::String^ name)
                        {
                            PropertyChanged(this, ref new Microsoft::UI::Xaml::Data::PropertyChangedEventArgs(name));
                        }
                    };
} } } } }

#define MAKEPROPGET(class, name) \
ref new Microsoft::UI::Xaml::Tests::Common::PropertyGetValue([](Platform::Object^ instance) \
{ \
    return safe_cast<class>(instance)->name; \
})

#define MAKEPROPSET(class, name, type) \
ref new Microsoft::UI::Xaml::Tests::Common::PropertySetValue([](Platform::Object^ instance, Platform::Object^ value) \
{ \
    safe_cast<class>(instance)->name = safe_cast<type>(value); \
})

#define MAKEPROPGETIDX(class, name, indexType) \
ref new Microsoft::UI::Xaml::Tests::Common::PropertyGetIndexedValue([](Platform::Object^ instance, Platform::Object^ index) \
{ \
    return safe_cast<class>(instance)->name->Lookup(safe_cast<indexType>(index)); \
})

#define MAKEPROPSETIDX(class, name, type, indexType) \
ref new Microsoft::UI::Xaml::Tests::Common::PropertySetIndexedValue([](Platform::Object^ instance, Platform::Object^ value, Platform::Object^ index) \
{ \
    safe_cast<class>(instance)->name->Insert(safe_cast<indexType>(index), safe_cast<type>(value)); \
})
