#pragma once
#include "Person.g.h"

namespace winrt::TableViewAppCppPackaged::implementation
{
    struct Person : PersonT<Person>
    {
        Person() = default;

        hstring Name() const { return m_name; }
        void Name(hstring const& value) { m_name = value; }

        int32_t Age() const { return m_age; }
        void Age(int32_t value) { m_age = value; }

    private:
        hstring m_name;
        int32_t m_age{ 0 };
    };
}

namespace winrt::TableViewAppCppPackaged::factory_implementation
{
    struct Person : PersonT<Person, implementation::Person>
    {
    };
}