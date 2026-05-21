// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "DataSource.h"
#include <utility>
#include <cstdint>
#include "InpcDataSource.h"

using namespace std;
using namespace default;

namespace Tests { namespace Native { namespace External { namespace Framework {

    DataSource::DataSource()
    {
        m_properties.insert(make_pair(int16::typeid, PropertyInfo(L"Int16Property", nullptr)));
        m_properties.insert(make_pair(uint16::typeid, PropertyInfo(L"UInt16Property", nullptr)));
        m_properties.insert(make_pair(int32::typeid, PropertyInfo(L"Int32Property", nullptr)));
        m_properties.insert(make_pair(uint32::typeid, PropertyInfo(L"UInt32Property", nullptr)));
        m_properties.insert(make_pair(float::typeid, PropertyInfo(L"SingleProperty", nullptr)));
        m_properties.insert(make_pair(double::typeid, PropertyInfo(L"DoubleProperty", nullptr)));
        m_properties.insert(make_pair(Platform::Boolean::typeid, PropertyInfo(L"BooleanProperty", nullptr)));

        m_properties.insert(make_pair(Platform::Object::typeid, PropertyInfo(L"ObjectProperty", nullptr)));
        m_properties.insert(make_pair(Platform::String::typeid, PropertyInfo(L"StringProperty", nullptr)));

        m_properties.insert(make_pair(InpcDataSource::typeid, PropertyInfo(L"InpcDataSourceProperty", nullptr)));
    }

    InpcDataSource^ DataSource::InpcDataSourceProperty::get()
    {
        return safe_cast<InpcDataSource^>(GetValue(InpcDataSource::typeid));
    }
    void DataSource::InpcDataSourceProperty::set(InpcDataSource^ value)
    {
        SetValue(InpcDataSource::typeid, safe_cast<InpcDataSource^>(value));
    }

} } } }