// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#if DBG

#include "ReferenceTrackerInterfaces.h"

class CDependencyObject;

namespace DirectUI
{
    class DependencyObject;

    // The different kinds of log records
    enum ReferenceTrackerLogType
    {
        Create = 1, // Create a DXaml peer
        Remove,     // Remove a DXaml peer
        Walk,       // Reference Tracker Walk step
        Target,     // Walk to an IReferenceTrackerTarget
        Tracking,   // Reference tracking start/stop
        Unreachable // An object that is now unreachable
    };

    // Log that an object is being walked
    class ReferenceTrackerLogWalk
    {
    public:
        static void Log(
            const DependencyObject* object, 
            EReferenceTrackerWalkType type, 
            bool isRoot, // Root of the walk or part of transitive closure?
            bool walked, // When !isStart, did the walk happen?
            bool isStart); // True when starting the walk on this object, false at the end

    private:
        const void* _object;
        EReferenceTrackerWalkType _walkType;
        bool _isRoot:1;
        bool _walked:1;
        bool _isStart:1;
    };



    // Log an IReferenceTrackerTarget that is being walked
    class ReferenceTrackerLogTarget
    {
    public:
        static void Log(const IReferenceTrackerTarget* object, EReferenceTrackerWalkType type);

    private:
        const void* _object;
        EReferenceTrackerWalkType _walkType;
    };


    // Log that an object has been created
    class ReferenceTrackerLogCreate
    {
    public:
        static void Log(const DependencyObject *object, const CDependencyObject *coreObject);

    private:
        const void* _object;
        const void* _coreObject;
    };

    // Log that an object has been removed
    class ReferenceTrackerLogRemove
    {
    public:
        static void Log(const DependencyObject *object);

    private:
        const void* _object;
    };

    // Log that an object has an implicit peg
    class ReferenceTrackerLogImplicitPeg
    {
    public:
        static void Log(const IReferenceTracker *object);

    private:
        const void* _object;
    };



    // Log that an tracking is started/completed
    enum ReferenceTrackerLogTrackingPhase
    {
        Start = 1,
        Completed = 2
    };
    class ReferenceTrackerLogTracking
    {
    public:
        static void Log(ReferenceTrackerLogTrackingPhase phase);

    private:
        ReferenceTrackerLogTrackingPhase _phase;
    };


    // Log that an object is unreachable
    class ReferenceTrackerLogUnreachable
    {
    public:
        static void Log(const IReferenceTracker *object);

    private:
        void* _object;
    };



    // A single log record (discrimate union)
    struct ReferenceTrackerLogItem
    {
    public:
        ReferenceTrackerLogType Type;

        union
        {
            ReferenceTrackerLogWalk Walk;
            ReferenceTrackerLogTarget Target;
            ReferenceTrackerLogCreate Create;
            ReferenceTrackerLogRemove Remove;
            ReferenceTrackerLogTracking Tracking;
            ReferenceTrackerLogUnreachable Unreachable;
        };
    };


}

#endif // #if DBG