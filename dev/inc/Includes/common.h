// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

using namespace Windows::Foundation;
using namespace Windows::Foundation::Numerics;

using namespace std::chrono_literals;
using namespace std::string_view_literals;
using namespace std::chrono_literals;

using ResourceIdType = const winrt::hstring &;

#include "ErrorHandling.h"
#include "CppWinRTHelpers.h"
#include "RuntimeClassHelpers.h"
#include "SharedHelpers.h"
#include "BoxHelpers.h"
#include "CastHelpers.h"
#include "event.h"
#include "DownlevelHelper.h"
#include "AutoHandle.h"
#include "GlobalDependencyProperty.h"
#include "CollectionHelper.h"
#include "RoutedEventHelpers.h"
#include "MUXControlsFactory.h"
