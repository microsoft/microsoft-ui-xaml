// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "pch.h"

namespace wrl = Microsoft::WRL;

template<class... T> 
struct typelist {};

template<bool C, class T>
struct select_type_or_nil_impl
    : std::conditional<C, T, wrl::Details::Nil>
{
};

template<bool C, class T>
using select_type_or_nil = typename select_type_or_nil_impl<C, T>::type;

template<class A, class B> 
struct strip_nil_types_impl;

template<template<class...> class A, 
    template<class...> class B, 
    class... T>
struct strip_nil_types_impl<A<>, B<T...>>
{
    typedef B<T...> type;
};

template<template<class...> class A, 
    class... S, 
    template<class...> class B, 
    class... T>
struct strip_nil_types_impl<A<wrl::Details::Nil, S...>, B<T...>> 
    : strip_nil_types_impl<A<S...>, B<T...>>
{
};

template<template<class...> class A, 
    class S1, class... S, 
    template<class...> class B, 
    class... T>
struct strip_nil_types_impl<A<S1, S...>, B<T...>> 
    : strip_nil_types_impl<A<S...>, B<T..., S1>>
{
};

template<class A>
using strip_nil_types = typename strip_nil_types_impl<A, typelist<>>::type;

template<class A, 
    template<class...> class B> 
struct rename_template_class_impl;

template<template<class...> class A, 
    class... T, 
    template<class...> class B>
struct rename_template_class_impl<A<T...>, B>
{
    using type = B<T...>;
};

template<class A, template<class...> class B>
using rename_template_class = typename rename_template_class_impl<A, B>::type;