// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <NamespaceAliases.h>

// This file introduces some magic to overcome a problem with windows.foundation.collections.h and how
// we inherit iterable types and use them in JoltCollections.h.
//
// windows.foundation.collections.h has a definition for IIterator_impl which accepts an isStruct template parameter.
// This has a supports_cleanup template used to determine one of two template definitions, one of which is meant for structs.
// The problem is that __is_class(type) does not properly identify a struct type.  The windows foundation collections get
// around this by defining is_foundation_struct<> with specialized types.  As such, this file aims to do the same, it additionally
// provides an opportunity to declare Detail::_Cleanup which is another specialization required for xaml struct collections.
//
// This file is required to be included after Windows.Foundation.Collections.h but before Microsoft.UI.Xaml.h or Microsoft.UI.Xaml-controls.h
// This provides the opportunity to indicate that certain types are structs before IIterate_impl and similar classes are declared.
//
// A more permanent solution would be to refactor JoltCollections.h/cpp to possibly use WIL or customized implementation that
// doesn't have a dependency on windows.foundation.collections.h.  Alternatively, windows.foundation.collections.h
// can fix the use of __is_class or use an alternative that works better.

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft
{
    namespace UI
    {
        namespace Xaml
        {
            namespace Documents
            {
                //forward delcare the known structs
                struct TextRange;
            }
        }
    }
}

namespace Windows
{
    namespace Foundation
    {
        namespace Collections
        {
            template <>
            struct is_foundation_struct<xaml_docs::TextRange> { enum { value = true }; };

            namespace Detail
            {
                inline void _Cleanup(xaml_docs::TextRange * /* values[] */, unsigned /* actual */) {}
            }
        }
    }
}
XAML_ABI_NAMESPACE_END