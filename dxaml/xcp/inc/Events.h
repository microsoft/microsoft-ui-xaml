// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CValue;
class CDependencyObject;
interface IScriptObject;
class CEventArgs;

#include <Indexes.g.h>

struct EventHandle
{
    KnownEventIndex index;

    EventHandle(KnownEventIndex eventIndex = KnownEventIndex::UnknownType_UnknownEvent)
    {
        index = eventIndex;
    }

    bool operator==(const EventHandle &other)
    {
        return index == other.index;
    }

    bool operator!=(const EventHandle &other)
    {
        return !operator==(other);
    }
};

//TODO - Move to Host layer?
// Prototype for event handler methods which are implementally internally instead of script or outside code.
typedef HRESULT(__stdcall *INTERNAL_EVENT_HANDLER)(_In_ CDependencyObject* pSender, _In_ CEventArgs* pEventArgs);

//TODO - Move to Host layer?
// event call back mechanism, the parameter "pe" is the eventargs being sent
typedef HRESULT(__stdcall *EVENTPFN)(_In_ void* pControl,
    _In_ CDependencyObject *pListener,
    _In_ EventHandle hEvent,
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs *pArgs,
    _In_ XINT32 flags,
    _In_ IScriptObject* pScriptObject,
    _In_ INTERNAL_EVENT_HANDLER pInternalHandler);

// This method is exported out of agcore for use by other DLLs that use
// CValue, such as Silverlight's npctrl.
void __stdcall
CValue_FreeValuePointer(
    _In_ CValue* pValue
    );
