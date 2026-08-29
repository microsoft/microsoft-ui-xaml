// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Versioning.h>

#include <CustomPropertySupport.h>

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {

    ref class PetObject sealed : Microsoft::UI::Xaml::Data::INotifyPropertyChanged, Microsoft::UI::Xaml::Data::ICustomPropertyProvider
    {
    public:
        PetObject(Platform::String^ commonName)
        {
            this->CommonName = commonName;
        }
    public:
        property Platform::String^ CommonName;

        // Inherited via INotifyPropertyChanged
        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

        // Inherited via ICustomPropertyProvider
        virtual property ::Windows::UI::Xaml::Interop::TypeName Type;
        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name);
        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName type)  { VERIFY_FAIL(L"Unexpected call into unimplemented method"); return nullptr; }
        virtual Platform::String^ GetStringRepresentation();

        Platform::String^ ToString() { return CommonName; }
    };


    ref class PetOwnerObject sealed : Microsoft::UI::Xaml::Data::INotifyPropertyChanged, Microsoft::UI::Xaml::Data::ICustomPropertyProvider
    {
    public:
        PetOwnerObject(Platform::String^ firstName)
        {
            this->FirstName = firstName;
        }

        void SetPetAnimal(Platform::String^ animalName)
        {
            this->PetAnimal = ref new PetObject(animalName + L": " + FirstName + L"s Pet");
        }

    public:
        property Platform::String^ FirstName;
        property PetObject^ PetAnimal;

        // Inherited via INotifyPropertyChanged
        virtual event Microsoft::UI::Xaml::Data::PropertyChangedEventHandler^ PropertyChanged;

        // Inherited via ICustomPropertyProvider
        virtual property ::Windows::UI::Xaml::Interop::TypeName Type;
        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name);
        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName type)  { VERIFY_FAIL(L"Unexpected call into unimplemented method"); return nullptr; }
        virtual Platform::String^ GetStringRepresentation();

        Platform::String^ ToString() { return FirstName; }
    };

    ref class PetAnimalProperty sealed : public Microsoft::UI::Xaml::Data::ICustomProperty
    {
    public:
        // Inherited via ICustomProperty
        virtual property bool CanRead;
        virtual property bool CanWrite;
        virtual property Platform::String^ Name;
        virtual property ::Windows::UI::Xaml::Interop::TypeName Type;
        virtual Platform::Object^ GetValue(Platform::Object^ target);
        virtual void SetValue(Platform::Object^ target, Platform::Object^ value) { VERIFY_FAIL(L"Unexpected call into unimplemented method"); }
        virtual Platform::Object^ GetIndexedValue(Platform::Object^ target, Platform::Object^ index) { VERIFY_FAIL(L"Unexpected call into unimplemented method"); return nullptr; }
        virtual void SetIndexedValue(Platform::Object^ target, Platform::Object^ value, Platform::Object^ index) { VERIFY_FAIL(L"Unexpected call into unimplemented method"); }
    };

    ref class TextProperty sealed : public Microsoft::UI::Xaml::Data::ICustomProperty
    {
    public:
        // Inherited via ICustomProperty
        virtual property bool CanRead;
        virtual property bool CanWrite;
        virtual property Platform::String^ Name;
        virtual property ::Windows::UI::Xaml::Interop::TypeName Type;
        virtual Platform::Object^ GetValue(Platform::Object^ target);
        virtual void SetValue(Platform::Object^ target, Platform::Object^ value) { VERIFY_FAIL(L"Unexpected call into unimplemented method"); }
        virtual Platform::Object^ GetIndexedValue(Platform::Object^ target, Platform::Object^ index) { VERIFY_FAIL(L"Unexpected call into unimplemented method"); return nullptr; }
        virtual void SetIndexedValue(Platform::Object^ target, Platform::Object^ value, Platform::Object^ index) { VERIFY_FAIL(L"Unexpected call into unimplemented method"); }
    };

} } } } } }
