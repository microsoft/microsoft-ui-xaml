// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Convenience stuff for working with IXamlPredicate and associated logic

#pragma once

#include "XamlType.h"
#include "IXamlSchemaObject.h"

class xstring_ptr;

namespace Parser
{
    // Convenience struct to hold a XamlType representing an IXamlPredicate
    // and its arguments string
    struct XamlPredicateAndArgs : IXamlSchemaObject
    {
        std::shared_ptr<XamlType> PredicateType;
        xstring_ptr Arguments;
    
        XamlPredicateAndArgs() = default;
        XamlPredicateAndArgs(
            std::shared_ptr<XamlType> predicateType,
            xstring_ptr arguments)
            : PredicateType(predicateType)
            , Arguments(arguments)
        { }


        // Indicates whether or not this is empty (i.e. functionally equivalent to always 'true')
        bool IsEmpty() const { return !PredicateType || PredicateType->IsUnknown(); }
    };
}