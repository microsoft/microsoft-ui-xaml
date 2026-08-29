// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <TestEvent.h>

#include "Timeouts.h"

namespace Microsoft {  namespace UI { namespace Xaml {
  namespace Tests { namespace Common {

    bool Event::IsBVT()
    {
        return COM_GROUP_ISBVT;
    }

#ifndef __DISABLE_EVENT_WAIT

    std::chrono::milliseconds Event::GetDefaultTimeout()
    {
        if (COM_GROUP_ISBVT)
        {
            return BVT_DefaultTimeout;
        }
        else
        {
            return DefaultTimeout;
        }
    }

    std::chrono::milliseconds Event::GetDefaultTextInputTimeout()
    {
        if (COM_GROUP_ISBVT)
        {
            return BVT_DefaultTextInputTimeout;
        }
        else
        {
            return DefaultTextInputTimeout;
        }
    }

#endif

    } } } }
}