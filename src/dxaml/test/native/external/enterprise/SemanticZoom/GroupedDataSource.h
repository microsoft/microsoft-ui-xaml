// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "collection.h"

using namespace Platform;
using namespace Platform::Collections;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Data;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace SemanticZoom {

   ref class GroupedHeader sealed
        : public ::Windows::Foundation::Collections::IVector<Platform::Object^>
        , public Microsoft::UI::Xaml::Data::ICustomPropertyProvider
    {
    private:
        Platform::String^ m_Header;
        Platform::Collections::Vector<Platform::Object^>^ m_Items;

    public:
        property Platform::String^ Key
        {
            Platform::String^ get()
            {
                return m_Header;
            }

            void set(Platform::String^ value)
            {
                m_Header = value;
            }
        }

        GroupedHeader(Platform::String^ header)
        {
            m_Header = header;
            m_Items = ref new Platform::Collections::Vector<Platform::Object^>();
        }

        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetCustomProperty(Platform::String^ name)
        {
            return nullptr;
        }

        virtual Microsoft::UI::Xaml::Data::ICustomProperty^ GetIndexedProperty(Platform::String^ name, ::Windows::UI::Xaml::Interop::TypeName typeName)
        {
            throw ref new Platform::NotImplementedException();
        }

        virtual Platform::String^ GetStringRepresentation()
        {
            return Key;
        }

        virtual property ::Windows::UI::Xaml::Interop::TypeName Type
        {
            virtual ::Windows::UI::Xaml::Interop::TypeName get()
            {
                ::Windows::UI::Xaml::Interop::TypeName tn;
                tn.Name = L"GroupedHeader";
                tn.Kind = ::Windows::UI::Xaml::Interop::TypeKind::Custom;
                return tn;
            }
        }

        virtual ::Windows::Foundation::Collections::IIterator<Platform::Object^>^ First()
        {
            return m_Items->First();
        }

        property unsigned int Size
        {
            virtual unsigned int get()
            {
                return m_Items->Size;
            }
        }

        virtual Platform::Object^ GetAt(unsigned int index)
        {
            return m_Items->GetAt(index);
        }

        virtual ::Windows::Foundation::Collections::IVectorView<Platform::Object^>^ GetView()
        {
            return m_Items->GetView();
        }

        virtual bool IndexOf(Platform::Object^ value, unsigned int* index)
        {
            return m_Items->IndexOf(value, index);
        }

        virtual void SetAt(unsigned int index, Platform::Object^ value)
        {
            m_Items->SetAt(index, value);
        }

        virtual void InsertAt(unsigned int index, Platform::Object^ value)
        {
            m_Items->InsertAt(index, value);
        }

        virtual void RemoveAt(unsigned int index)
        {
            m_Items->RemoveAt(index);
        }

        virtual void Append(Platform::Object^ value)
        {
            m_Items->Append(value);
        }

        virtual void RemoveAtEnd()
        {
            m_Items->RemoveAtEnd();
        }

        virtual void Clear()
        {
            m_Items->Clear();
        }

        virtual unsigned int GetMany(unsigned int startIndex, Platform::WriteOnlyArray<Platform::Object^>^ items)
        {
            return m_Items->GetMany(startIndex, items);
        }

        virtual void ReplaceAll(const Platform::Array<Platform::Object^>^ items)
        {
            m_Items->ReplaceAll(items);
        }

    };

} } } } } }