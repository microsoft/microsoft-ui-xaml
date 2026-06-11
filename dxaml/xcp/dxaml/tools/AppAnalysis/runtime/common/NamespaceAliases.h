// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#if defined(MIDL_NS_PREFIX)
    #define XAML_ABI_PARAMETER(x) ABI::x
    #define XAML_ABI_NAMESPACE_BEGIN namespace ABI {
    #define XAML_ABI_NAMESPACE_END }
#else
    #define XAML_ABI_PARAMETER(x) x
    #define XAML_ABI_NAMESPACE_BEGIN
    #define XAML_ABI_NAMESPACE_END
#endif // defined(MIDL_NS_PREFIX)

// Declare the namespaces for the aliases below+

namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { } } }
namespace Microsoft { namespace WRL { namespace Wrappers { } } }
namespace Windows { namespace Foundation { namespace Collections { namespace Internal {} } } }

XAML_ABI_NAMESPACE_BEGIN
namespace Windows { namespace Foundation {} }
namespace Windows { namespace Foundation { namespace Collections {} } }
namespace Windows { namespace Storage { namespace Streams { } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Data { } } } }
namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { } } }
XAML_ABI_NAMESPACE_END

namespace wrl = ::Microsoft::WRL;
namespace wrl_wrappers = ::Microsoft::WRL::Wrappers;
namespace wf = XAML_ABI_PARAMETER(Windows::Foundation);
namespace wfc = XAML_ABI_PARAMETER(Windows::Foundation::Collections);
namespace wfci_ = ::Windows::Foundation::Collections::Internal;
namespace wsts = XAML_ABI_PARAMETER(Windows::Storage::Streams);
namespace xaml = XAML_ABI_PARAMETER(Microsoft::UI::Xaml);
namespace wux_data = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Data);
namespace appanalysis = XAML_ABI_PARAMETER(Microsoft::Diagnostics::AppAnalysis);

namespace appanalysis_impl = ::Microsoft::Diagnostics::AppAnalysis;