// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TestEvent.h>

#include "Timeouts.h"

namespace Microsoft {  namespace UI { namespace Xaml {
  namespace Tests { namespace Common {

#ifndef __DISABLE_EVENT_WAIT
      bool Event::IsBVT() { return false; }
#endif

#ifndef __DISABLE_EVENT_WAIT

      std::chrono::milliseconds Event::GetDefaultTimeout()
      {
          return DefaultTimeout;
      }

      std::chrono::milliseconds Event::GetDefaultTextInputTimeout()
      {
          return DefaultTextInputTimeout;
      }

#endif

    } } } }
}