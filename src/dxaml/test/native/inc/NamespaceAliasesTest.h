// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NamespaceAliases.h"

// // Declare the namespaces for the aliases below
XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { } } }
namespace Private { namespace Infrastructure { } }
namespace MockDComp {}
XAML_ABI_NAMESPACE_END

namespace appanalysis = XAML_ABI_PARAMETER(Microsoft::Diagnostics::AppAnalysis);
namespace test_infra = XAML_ABI_PARAMETER(Private::Infrastructure);
namespace mdc = XAML_ABI_PARAMETER(MockDComp);