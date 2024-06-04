// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;
class CEventArgs;

// Prototype for event handler methods which are implementally internally instead of script or outside code.
typedef HRESULT (__stdcall *INTERNAL_EVENT_HANDLER)(_In_ CDependencyObject* pSender, _In_ CEventArgs* pEventArgs);
