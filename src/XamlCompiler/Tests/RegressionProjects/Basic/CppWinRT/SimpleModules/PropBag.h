// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include "PropBag.g.h"

namespace winrt::Simple::implementation
{
    struct PropBag : PropBagT<PropBag>
    {
        PropBag() {}

        ::winrt::hstring StringProp() const noexcept { return _StringProp; }
        void StringProp(::winrt::hstring const& value) noexcept { _StringProp = value; }
        int IntProp() const noexcept { return _IntProp; }
        void IntProp(int value) noexcept { _IntProp = value; }
        double DoubleProp() const noexcept { return _DoubleProp; }
        void DoubleProp(double value) noexcept { _DoubleProp = value; }
        bool BooleanProp() const noexcept { return _BooleanProp; }
        void BooleanProp(bool value) noexcept { _BooleanProp = value; }
        short Int16Prop() const noexcept { return _Int16Prop; }
        void Int16Prop(short value) noexcept { _Int16Prop = value; }

    private:
        bool _BooleanProp;
        double _DoubleProp;
        int _IntProp;
        short _Int16Prop;
        ::winrt::hstring _StringProp;
    };
}

namespace winrt::Simple::factory_implementation
{
    struct PropBag : PropBagT<PropBag, implementation::PropBag>
    {
    };
}
