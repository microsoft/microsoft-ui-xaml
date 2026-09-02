#pragma once
#include "Person.g.h"

namespace winrt::TableViewAppCppUnpackaged::implementation
{
    struct Person : PersonT<Person>
    {
        Person() = default;

        hstring Name() const { return m_name; }
        void Name(hstring const& value) { m_name = value; }

        int32_t Age() const { return m_age; }
        void Age(int32_t value) { m_age = value; }

        // String projection so the cell templates can x:Bind without a converter.
        hstring AgeText() const { return to_hstring(m_age); }

    private:
        hstring m_name;
        int32_t m_age{ 0 };
    };
}

namespace winrt::TableViewAppCppUnpackaged::factory_implementation
{
    struct Person : PersonT<Person, implementation::Person>
    {
    };
}