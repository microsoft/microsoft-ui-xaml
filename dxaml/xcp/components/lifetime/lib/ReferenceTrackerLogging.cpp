// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ReferenceTrackerLogging.h"
#include "ReferenceTrackerManager.h"

#if DBG

using namespace DirectUI;

void ReferenceTrackerLogWalk::Log(const DependencyObject *object, EReferenceTrackerWalkType type, bool isRoot, bool walked, bool isStart)
{
    if (!ReferenceTrackerManager::IsLoggingEnabled())
        return;

    ReferenceTrackerLogItem item = {};
    item.Type = ReferenceTrackerLogType::Walk;
    item.Walk._object = (xaml_hosting::IReferenceTrackerInternal*)object;
    item.Walk._walkType = type;
    item.Walk._isRoot = isRoot;
    item.Walk._walked = walked;
    item.Walk._isStart = isStart;

    ReferenceTrackerManager::Log(item);
}


void ReferenceTrackerLogTarget::Log(const IReferenceTrackerTarget *object, EReferenceTrackerWalkType type)
{
    if (!ReferenceTrackerManager::IsLoggingEnabled())
        return;

    ReferenceTrackerLogItem item = {};
    item.Type = ReferenceTrackerLogType::Target;
    item.Target._object = object;
    item.Target._walkType = type;

    ReferenceTrackerManager::Log(item);
}



void ReferenceTrackerLogCreate::Log(const DependencyObject *object, const CDependencyObject *coreObject)
{
    if (!ReferenceTrackerManager::IsLoggingEnabled())
        return;

    ReferenceTrackerLogItem item = {};
    item.Type = ReferenceTrackerLogType::Create;
    item.Create._object = (xaml_hosting::IReferenceTrackerInternal*)object;
    item.Create._coreObject = coreObject;

    ReferenceTrackerManager::Log(item);
}


void ReferenceTrackerLogRemove::Log(const DependencyObject *object)
{
    if (!ReferenceTrackerManager::IsLoggingEnabled())
        return;

    ReferenceTrackerLogItem item = {};
    item.Type = ReferenceTrackerLogType::Remove;
    item.Remove._object = (xaml_hosting::IReferenceTrackerInternal*)object;

    ReferenceTrackerManager::Log(item);
}


void ReferenceTrackerLogTracking::Log(ReferenceTrackerLogTrackingPhase phase)
{
    if (!ReferenceTrackerManager::IsLoggingEnabled())
        return;

    ReferenceTrackerLogItem item = {};
    item.Type = ReferenceTrackerLogType::Tracking;
    item.Tracking._phase = phase;

    ReferenceTrackerManager::Log(item);
}


void ReferenceTrackerLogUnreachable::Log(const IReferenceTracker *object)
{
    if (!ReferenceTrackerManager::IsLoggingEnabled())
        return;

    ReferenceTrackerLogItem item = {};
    item.Type = ReferenceTrackerLogType::Unreachable;
    item.Unreachable._object = (xaml_hosting::IReferenceTrackerInternal*)object;

    ReferenceTrackerManager::Log(item);
}


#endif // #if DBG
