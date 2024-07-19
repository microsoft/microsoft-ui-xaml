// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <SelectionBehavior.h>

namespace TouchTextSelection
{
    struct StartingPoint
    {
    public:
        StartingPoint();
        unsigned __int64 timestamp;      
        POINT pt;                 
        int dyContactCenterAnchor;  // The distance from anchor the contact started at.
        POINT ptAnchor;
        POINT ptGripperCenterOffsetFromAnchor; 
        int cGripperDiameter;
        int dyExpandedInitialZone;
    };

    struct LastPoint
    {
    public:
        LastPoint();
        unsigned __int64 timestamp;      // timestamp the gripper was here
        POINT pt;                        // the screen location
        unsigned int cCurrentLineHeight;
    };

    class CHandleProcessor : public IHandleInputProcessor 
    {
    public:
        HRESULT SetHimetricsPerPixel(float rScale) override;
        void SetGripperDisplayLocation(POINT ptAnchor, POINT ptGripperCenterOffsetFromAnchor, unsigned int cDiameter) override;
        HRESULT OnStartGripperDrag(POINT unFilteredPoint, unsigned int cCurrentLineHeight) override;
        HRESULT OnGripperDrag(POINT unFilteredPoint, unsigned int cCurrentLineHeight, _Out_ POINT *ptResult) override;
        HRESULT OnEndGripperDrag(void) override;
        CHandleProcessor();
        ~CHandleProcessor() override;
        void OverrideSettings(unsigned int cVerticalMovementLimit, unsigned __int64 cDragStopAnchorResetTicks);
        
    private:
        POINT _CalcLogicalPos();
        bool _IsInputWithinHorizontalMovementRect(const POINT &input);
        void _LoadSafetyZoneSettings();
        void _UpdateStartingPoint(POINT unFilteredPoint, unsigned __int64 timestamp, unsigned int cCurrentLineHeight);
        bool _HasMovedFromInitialPosition(const POINT& currentPosition);
    
        StartingPoint _startingLocation;
        LastPoint _lastLocation; 
        bool _fUseRegSettings;
        bool _fActive;
        bool _fHasMovedFromInitialPosition;
        float _rCurrentScale;
    
        int _cVerticalDriftUpLimit;
        int _cVerticalDriftDownLimit;
        unsigned __int64 _cDragStopAnchorResetTicks;

        static unsigned int s_cVerticalMovementUpLimit;
        static unsigned int s_cVerticalMovementDownLimit;
        static unsigned __int64 s_cDragStopAnchorResetTicks;
    };

};
