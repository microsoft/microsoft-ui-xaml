// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

winrt::hstring GetLocalizedStringResourceFromMui(int resourceId);
winrt::array_view<const byte> GetImageBytesFromDll(int assetId);