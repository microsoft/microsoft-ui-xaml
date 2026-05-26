// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <AutomationClient\AutomationEventHandler.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Automation { namespace Patterns {

    class SelectionItemPatternHandler : public AutomationClient::AutomationEventHandler
    {
    public:
        SelectionItemPatternHandler(std::shared_ptr<AutomationClient::AutomationClientManager> spAClientManager, std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event> spEvent, TreeScope tScope, EVENTID id)
            : AutomationEventHandler(spAClientManager, spEvent, tScope, id)
        {
        }
    };

} } } } } }
