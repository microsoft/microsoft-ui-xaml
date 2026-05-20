// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <map>
#include <collection.h>
#include <cassert>
#include <cstdint>
#include <type_traits>

namespace Tests { namespace Native { namespace External { namespace Framework {

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class PropertyGotEventArgs sealed
    {
    private:
        Platform::String^ m_propertyName;
        Platform::Type^ m_type;
        Platform::Object^ m_returnedValue;
    public:
        PropertyGotEventArgs(Platform::String^ propertyName, Platform::Type^ type, Platform::Object^ returnedValue)
        {
            m_propertyName = propertyName;
            m_type = type;
            m_returnedValue = returnedValue;
        }

        property Platform::String^ PropertyName
        {
            Platform::String^ get() { return m_propertyName; }
        }
        property Platform::Type^ Type
        {
            Platform::Type^ get() { return m_type; }
        }
        property Platform::Object^ ReturnedValue
        {
            Platform::Object^ get() { return m_returnedValue; }
        }
    };
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class PropertySetEventArgs sealed
    {
    private:
        Platform::String^ m_propertyName;
        Platform::Type^ m_type;
        Platform::Object^ m_oldValue;
        Platform::Object^ m_newValue;
    public:
        PropertySetEventArgs(Platform::String^ propertyName,
            Platform::Type^ type,
            Platform::Object^ oldValue,
            Platform::Object^ newValue)
        {
            m_propertyName = propertyName;
            m_type = type;
            m_oldValue = oldValue;
            m_newValue = newValue;
        }

        property Platform::String^ PropertyName
        {
            Platform::String^ get() { return m_propertyName; }
        }
        property Platform::Type^ Type
        {
            Platform::Type^ get() { return m_type; }
        }
        property Platform::Object^ OldValue
        {
            Platform::Object^ get() { return m_oldValue; }
        }
        property Platform::Object^ NewValue
        {
            Platform::Object^ get() { return m_newValue; }
        }
    };

    ref class DataSource;
    ref class InpcDataSource;

    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public delegate void PropertyGotEventHandler(DataSource^ sender, PropertyGotEventArgs^ args);
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    public delegate void PropertySetEventHandler(DataSource^ sender, PropertySetEventArgs^ args);
    
    [::Windows::Foundation::Metadata::WebHostHiddenAttribute]
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class DataSource : public Microsoft::UI::Xaml::DependencyObject
    {
    public:
        Platform::Object^ GetValue(Platform::Type^ type)
        {
            auto iterator = m_properties.find(type);
            assert(iterator != m_properties.end());
            PropertyGot(this, ref new PropertyGotEventArgs(iterator->second.m_propertyName, type, iterator->second.m_value));
            return iterator->second.m_value;
        }
        event PropertyGotEventHandler^ PropertyGot;
        event PropertySetEventHandler^ PropertySet;

        property int16 Int16Property
        {
            int16 get() { return GetValue<int16>(); }
            void set(int16 value) { SetValue<int16>(value); }
        }
        property uint16 UInt16Property
        {
            uint16 get() { return GetValue<uint16>(); }
            void set(uint16 value) { SetValue<uint16>(value); }
        }
        property int32 Int32Property
        {
            int32 get() { return GetValue<int32>(); }
            void set(int32 value) { SetValue<int32>(value); }
        }
        property uint32 UInt32Property
        {
            uint32 get() { return GetValue<uint32>(); }
            void set(uint32 value) { SetValue<uint32>(value); }
        }
        property float SingleProperty
        {
            float get() { return GetValue<float>(); }
            void set(float value) { SetValue<float>(value); }
        }
        property double DoubleProperty
        {
            double get() { return GetValue<double>(); }
            void set(double value) { SetValue<double>(value); }
        }
        property Platform::Boolean BooleanProperty
        {
            bool get() { return GetValue<Platform::Boolean>(); }
            void set(Platform::Boolean value) { SetValue<Platform::Boolean>(value); }
        }

        property Platform::String^ StringProperty
        {
            Platform::String^ get() { return GetRefValue<Platform::String>(); }
            void set(Platform::String^ value) { SetRefValue<Platform::String>(value); }
        }
        property Platform::Object^ ObjectProperty
        {
            Platform::Object^ get() { return GetRefValue<Platform::Object>(); }
            void set(Platform::Object^ value) { SetRefValue<Platform::Object>(value); }
        }

        property InpcDataSource^ InpcDataSourceProperty
        {
            InpcDataSource^ get();
            void set(InpcDataSource^ value);
        }

    protected private:
        DataSource();

        template <class T>
        T GetValue()
        {
            return safe_cast<T>(GetValue(T::typeid));
        }
        template <class T>
        T^ GetRefValue()
        {
            return safe_cast<T^>(GetValue(T::typeid));
        }

        template <class T>
        void SetValue(T value)
        {
            SetValue(T::typeid, value);
        }
        template <class T>
        void SetRefValue(T^ value)
        {
            SetValue(T::typeid, value);
        }

        virtual void SetValue(Platform::Type^ type, Platform::Object^ value)
        {
            auto iterator = m_properties.find(type);
            assert(iterator != m_properties.end());
            
            auto oldValue = iterator->second.m_value;
            iterator->second.m_value = value;
            PropertySet(this, ref new PropertySetEventArgs(iterator->second.m_propertyName, type, oldValue, value));
        }
        
        Platform::String^ GetPropertyName(Platform::Type^ type)
        {
            auto iterator = m_properties.find(type);
            assert(iterator != m_properties.end());
            return iterator->second.m_propertyName;
        }
    private:
        struct PropertyInfo
        {
            Platform::String^ m_propertyName;
            Platform::Object^ m_value;
            PropertyInfo() : m_propertyName(), m_value() {}
            PropertyInfo(Platform::String^ propertyName, Platform::Object^ value)
                : m_propertyName(propertyName), m_value(value) {}
        };
        struct TypeComparator
        {
            bool operator()(Platform::Type^ lhs, Platform::Type^ rhs) const
            {
                return lhs->FullName < rhs->FullName;
            }
        };
        std::map<Platform::Type^, PropertyInfo, TypeComparator> m_properties;
    };

} } } }