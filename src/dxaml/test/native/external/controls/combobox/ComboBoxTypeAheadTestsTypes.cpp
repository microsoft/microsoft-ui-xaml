// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "ComboBoxTypeAheadTestsTypes.h"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ComboBox {

    // -- Object interface implementations -- 
    Microsoft::UI::Xaml::Data::ICustomProperty^ PetObject::GetCustomProperty(Platform::String^ name)
    {
        return ref new TextProperty();
    }

    Platform::String^ PetObject::GetStringRepresentation()
    {
        return CommonName;
    }

    Platform::Object^ TextProperty::GetValue(Platform::Object^ target)
    {
        auto contact = dynamic_cast<PetOwnerObject^>(target);
        if (contact != nullptr)
        {
            return contact->FirstName;
        }
        auto animal = dynamic_cast<PetObject^>(target);
        if (animal != nullptr)
        {
            return animal->CommonName;
        }
        throw ref new Platform::NotImplementedException();
    }

    Microsoft::UI::Xaml::Data::ICustomProperty^ PetOwnerObject::GetCustomProperty(Platform::String^ name)
    {
        if (name->Equals(L"PetAnimal"))
        {
            return ref new PetAnimalProperty();
        }
        return ref new TextProperty();
    }

    Platform::String^ PetOwnerObject::GetStringRepresentation()
    {
        return FirstName;
    }

    Platform::Object^ PetAnimalProperty::GetValue(Platform::Object^ target)
    {
        auto petOwner = dynamic_cast<PetOwnerObject^>(target);
        return (petOwner == nullptr) ? nullptr : petOwner->PetAnimal;
    }

} } } } } }