// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <abi/xaml_abi.h>

namespace Jupiter { namespace Components { } }
namespace jc                    = Jupiter::Components;

namespace DebugTool {}
namespace dt                    = DebugTool;

namespace Microsoft { namespace WRL { namespace Wrappers { } } }
namespace wrl                   = ::Microsoft::WRL;
namespace wrl_wrappers          = ::Microsoft::WRL::Wrappers;

namespace Windows { namespace Internal { } }
namespace wi                    = Windows::Internal;
namespace win_internal          = Windows::Internal;

// As a matter of convention, namespaces suffixed with _ (underscore) are not subject to ABI prefixing.

namespace Windows { namespace Foundation { } }
namespace wf_                   = Windows::Foundation;

namespace Windows { namespace Foundation { namespace Collections { namespace Internal { } } } }
namespace wfci_                 = ::Windows::Foundation::Collections::Internal;

namespace Windows { namespace Foundation { namespace Numerics { } } }
namespace wfn_                  = ::Windows::Foundation::Numerics;

XAML_ABI_NAMESPACE_BEGIN
namespace Windows { namespace ApplicationModel { } }
namespace Windows { namespace ApplicationModel { namespace Activation { } } }
namespace Windows { namespace ApplicationModel { namespace Background { } } }
namespace Windows { namespace ApplicationModel { namespace Core { } } }
namespace Windows { namespace ApplicationModel { namespace DataTransfer { } } }
namespace Microsoft { namespace Windows { namespace ApplicationModel { namespace Resources { } } } }
namespace Windows { namespace ApplicationModel { namespace Search { } } }
namespace Windows { namespace Data { } }
namespace Windows { namespace Devices { } }
namespace Windows { namespace Devices { namespace Enumeration { } } }
namespace Windows { namespace Devices { namespace Display { } } }
namespace Windows { namespace Devices { namespace Geolocation { } } }
namespace Windows { namespace Devices { namespace Input { } } }
namespace Windows { namespace Foundation { } }
namespace Windows { namespace Foundation { namespace Collections { } } }
namespace Windows { namespace Foundation { namespace Numerics { } } }
namespace Windows { namespace Globalization { } }
namespace Microsoft { namespace Graphics { } }
namespace Microsoft { namespace Graphics { namespace DirectX { } } }
namespace Windows { namespace Graphics { } }
namespace Windows { namespace Graphics { namespace DirectX { } } }
namespace Windows { namespace Graphics { namespace Display { } } }
namespace Windows { namespace Graphics { namespace Imaging { } } }
namespace Windows { namespace Media { } }
namespace Windows { namespace Media { namespace Core { } } }
namespace Windows { namespace Media { namespace Playback { } } }
namespace Windows { namespace Phone { } }
namespace Windows { namespace Security { } }
namespace Windows { namespace Services { } }
namespace Windows { namespace Storage { } }
namespace Windows { namespace Storage { namespace Streams { } } }
namespace Windows { namespace System {} }
namespace Microsoft { namespace UI { namespace Dispatching {} } }
namespace Microsoft { namespace UI { namespace Windowing {} } }
namespace Windows { namespace System { namespace Power {} } }
namespace Windows { namespace System { namespace Threading {} } }
namespace Microsoft { namespace UI { namespace Composition { namespace Experimental {} } } }
namespace Microsoft { namespace UI { namespace Composition { namespace Internal {} } } }
namespace Microsoft { namespace UI { namespace Content {} } }
namespace Microsoft { namespace UI { namespace Content { namespace Partner {} } } }
namespace Windows { namespace UI { } }
namespace Windows { namespace UI { namespace Composition { } } }
namespace Windows { namespace UI { namespace Core { } } }
namespace Windows { namespace UI { namespace Input { } } }
namespace Microsoft { namespace UI { namespace Input { } } }
namespace Microsoft { namespace UI { namespace Input { namespace Experimental { } } } }
namespace Microsoft { namespace UI { namespace Input { namespace Partner { } } } }
namespace Microsoft { namespace UI { namespace Hosting { namespace Experimental { } } } }
namespace Windows { namespace UI { namespace Internal { } } }
namespace Windows { namespace UI { namespace Popups { } } }
namespace Windows { namespace UI { namespace ViewManagement { } } }
namespace Windows { namespace UI { namespace Text { } } }
namespace Microsoft { namespace UI { namespace Text { } } }
namespace Windows { namespace UI { namespace WindowManagement { } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers { } } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Inking {} } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives { } } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Maps { } } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Data { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Documents {} } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Hosting {} } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Input { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Interop { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Markup { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Media { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Media { namespace Animation { } } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Media { namespace Imaging { } } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Navigation { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Phone_XamlTypeInfo { } } } }
namespace Microsoft { namespace UI { namespace Xaml { namespace Shapes { } } } }
namespace Windows { namespace UI { namespace Xaml { namespace Interop { namespace Marshal {} } } } };
namespace Microsoft { namespace Internal { namespace FrameworkUdk {} } };
XAML_ABI_NAMESPACE_END

namespace wa                    = XAML_ABI_PARAMETER(Windows::ApplicationModel);
namespace waa                   = XAML_ABI_PARAMETER(Windows::ApplicationModel::Activation);
namespace wab                   = XAML_ABI_PARAMETER(Windows::ApplicationModel::Background);
namespace wac                   = XAML_ABI_PARAMETER(Windows::ApplicationModel::Core);
namespace wadt                  = XAML_ABI_PARAMETER(Windows::ApplicationModel::DataTransfer);
namespace mwar                  = XAML_ABI_PARAMETER(Microsoft::Windows::ApplicationModel::Resources);
namespace was                   = XAML_ABI_PARAMETER(Windows::ApplicationModel::Search);
namespace wda                   = XAML_ABI_PARAMETER(Windows::Data);
namespace wde                   = XAML_ABI_PARAMETER(Windows::Devices);
namespace wded                  = XAML_ABI_PARAMETER(Windows::Devices::Display);
namespace wdee                  = XAML_ABI_PARAMETER(Windows::Devices::Enumeration);
namespace wdeg                  = XAML_ABI_PARAMETER(Windows::Devices::Geolocation);
namespace wdei                  = XAML_ABI_PARAMETER(Windows::Devices::Input);
namespace wf                    = XAML_ABI_PARAMETER(Windows::Foundation);
namespace wfc                   = XAML_ABI_PARAMETER(Windows::Foundation::Collections);
namespace wfn                   = XAML_ABI_PARAMETER(Windows::Foundation::Numerics);
namespace wg                    = XAML_ABI_PARAMETER(Windows::Globalization);
namespace wgr                   = XAML_ABI_PARAMETER(Windows::Graphics);
namespace wgrdx                 = XAML_ABI_PARAMETER(Windows::Graphics::DirectX);
namespace wgrd                  = XAML_ABI_PARAMETER(Windows::Graphics::Display);
namespace wgri                  = XAML_ABI_PARAMETER(Windows::Graphics::Imaging);
namespace wm                    = XAML_ABI_PARAMETER(Windows::Media);
namespace wmc                   = XAML_ABI_PARAMETER(Windows::Media::Core);
namespace wmp                   = XAML_ABI_PARAMETER(Windows::Media::Playback);
namespace wp                    = XAML_ABI_PARAMETER(Windows::Phone);
namespace wsec                  = XAML_ABI_PARAMETER(Windows::Security);
namespace wsrv                  = XAML_ABI_PARAMETER(Windows::Services);
namespace wst                   = XAML_ABI_PARAMETER(Windows::Storage);
namespace wsts                  = XAML_ABI_PARAMETER(Windows::Storage::Streams);
namespace wsy                   = XAML_ABI_PARAMETER(Windows::System);
namespace msy                   = XAML_ABI_PARAMETER(Microsoft::UI::Dispatching);
namespace wsyp                  = XAML_ABI_PARAMETER(Windows::System::Power);
namespace wsyt                  = XAML_ABI_PARAMETER(Windows::System::Threading);
namespace WUComp                = XAML_ABI_PARAMETER(Microsoft::UI::Composition);   // todo - delete and replace users with "ixp"
namespace RealWUComp            = XAML_ABI_PARAMETER(Windows::UI::Composition);
namespace wu                    = XAML_ABI_PARAMETER(Windows::UI);
namespace mu                    = XAML_ABI_PARAMETER(Microsoft::UI);
namespace wuc                   = XAML_ABI_PARAMETER(Windows::UI::Core);
namespace wui                   = XAML_ABI_PARAMETER(Windows::UI::Input);
namespace mui                   = XAML_ABI_PARAMETER(Microsoft::UI::Input);
namespace wuint                 = XAML_ABI_PARAMETER(Windows::UI::Internal);
namespace wup                   = XAML_ABI_PARAMETER(Windows::UI::Popups);
namespace wuv                   = XAML_ABI_PARAMETER(Windows::UI::ViewManagement);
namespace wut                   = XAML_ABI_PARAMETER(Windows::UI::Text);
namespace mut                   = XAML_ABI_PARAMETER(Microsoft::UI::Text);
namespace wuwm                  = XAML_ABI_PARAMETER(Windows::UI::WindowManagement);
namespace xaml                  = XAML_ABI_PARAMETER(Microsoft::UI::Xaml);
namespace xaml_automation       = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Automation);
namespace xaml_automation_peers = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Automation::Peers);
namespace xaml_controls         = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Controls);
namespace xaml_inking           = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Controls::Inking);
namespace xaml_primitives       = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Controls::Primitives);
namespace xaml_maps             = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Controls::Maps);
namespace xaml_data             = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Data);
namespace xaml_docs             = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Documents);
namespace xaml_hosting          = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Hosting);
namespace xaml_input            = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Input);
namespace xaml_interop          = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Interop);
namespace xaml_markup           = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Markup);
namespace xaml_media            = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Media);
namespace xaml_navigation       = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Navigation);
namespace xaml_animation        = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Media::Animation);
namespace xaml_imaging          = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Media::Imaging);
namespace xaml_phone_xti        = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Phone_XamlTypeInfo);
namespace xaml_shapes           = XAML_ABI_PARAMETER(Microsoft::UI::Xaml::Shapes);
namespace wxaml_interop         = XAML_ABI_PARAMETER(Windows::UI::Xaml::Interop);
namespace xaml_interop_marshal  = XAML_ABI_PARAMETER(Windows::UI::Xaml::Interop::Marshal);
namespace udk_                  = XAML_ABI_PARAMETER(Microsoft::Internal::FrameworkUdk);

namespace ixp
{
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Composition);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Composition::Internal);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Composition::Experimental);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Content);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Content::Partner);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Input);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Input::Experimental);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Input::Partner);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Hosting::Experimental);
    using namespace XAML_ABI_PARAMETER(Microsoft::Graphics::DirectX);
    using namespace XAML_ABI_PARAMETER(Microsoft::UI::Windowing);
}