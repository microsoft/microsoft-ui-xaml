// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <precomp.h>
#include <windows.h>
#include <macros.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>
#include "InputProcessor.h"
#include <new.h>

IHandleInputProcessor * IHandleInputProcessor::s_CreateInstance()
{
    return new TouchTextSelection::CHandleProcessor();
}

namespace TouchTextSelection
{
    StartingPoint::StartingPoint() : timestamp(0), cGripperDiameter(0), dyContactCenterAnchor(0), dyExpandedInitialZone(0)
    {
        POINT ptEmpty = {};
        pt = ptEmpty;
        ptGripperCenterOffsetFromAnchor = ptEmpty;
    }

    LastPoint::LastPoint() : timestamp(0), cCurrentLineHeight(0)
    {
        POINT ptEmpty = {};
        pt = ptEmpty;
    }

    unsigned int CHandleProcessor::s_cVerticalMovementUpLimit = 550;
    unsigned int CHandleProcessor::s_cVerticalMovementDownLimit = 550;
    unsigned __int64 CHandleProcessor::s_cDragStopAnchorResetTicks = 250;  // 0.200 s

    bool IsEffectivelyZero(float fValue)
    {
        return (fabsf(fValue) <= FLT_EPSILON);
    }

    bool IsEffectivelyZero(double fValue)
    {
        return (fabs(fValue) <= DBL_EPSILON);
    }

    bool IsNonZero(float fValue)
    {
        return !IsEffectivelyZero(fValue);
    }

    bool IsNonZero(double fValue)
    {
        return !IsEffectivelyZero(fValue);
    }

    CHandleProcessor::CHandleProcessor() : _rCurrentScale(0), _fActive(false), _fUseRegSettings(true)
    {
        _LoadSafetyZoneSettings();
    }

    CHandleProcessor::~CHandleProcessor() 
    {

    }

    bool CHandleProcessor::_IsInputWithinHorizontalMovementRect(const POINT &input)
    {
        int deltaUp = _startingLocation.pt.y - input.y;
        int deltaDown = -deltaUp;
        return ((deltaUp <= _cVerticalDriftUpLimit) && (deltaDown <= _cVerticalDriftDownLimit));
    }

    POINT CHandleProcessor::_CalcLogicalPos()
    {
        POINT ptAdjusted = _lastLocation.pt;
        if (!_HasMovedFromInitialPosition(_lastLocation.pt))
        {
            // If we're still in the initial touch position, we return the gripper position to the anchor to ensure we only change the selection after we really started dragging
            // Only the x coordinate needs to be adjusted, the y coordinate is adjusted below for all cases
            // Given that _lastLocation.pt is still the first touch point, the safety zones will prevent ptAdjusted from hitting a different line
            ptAdjusted.x = _startingLocation.ptAnchor.x;
        }

        if (_IsInputWithinHorizontalMovementRect(_lastLocation.pt))
        {
            ptAdjusted.y = _startingLocation.pt.y;
        }
        else if (_startingLocation.dyExpandedInitialZone != 0)
        {
            // Normally we would be leaving the safety zone, but the initial touch point was high.
            // We'll shift the started point down.
            _startingLocation.pt.y += _startingLocation.dyExpandedInitialZone;
            
            // We need to recalculate dyContactCenterAnchor because we shifted the safety zone without moving the anchor.
            _startingLocation.dyContactCenterAnchor = _startingLocation.ptAnchor.y - _startingLocation.pt.y;
            
            _startingLocation.dyExpandedInitialZone = 0;
            if (_IsInputWithinHorizontalMovementRect(_lastLocation.pt))
            {
                ptAdjusted.y = _startingLocation.pt.y;
            }
        }
        else
        {  
            // Shift the band toward the finger's location.
            if (_startingLocation.pt.y < _lastLocation.pt.y)
            {
                _startingLocation.ptAnchor.y += _lastLocation.cCurrentLineHeight;
                _startingLocation.pt.y += _lastLocation.cCurrentLineHeight;
            }
            else
            {   
                _startingLocation.ptAnchor.y -= _lastLocation.cCurrentLineHeight;
                _startingLocation.pt.y -= _lastLocation.cCurrentLineHeight;
            }
            
            _startingLocation.timestamp = _lastLocation.timestamp;
            if (_IsInputWithinHorizontalMovementRect(_lastLocation.pt))
            {
                ptAdjusted.y = _startingLocation.pt.y;
            }
        }

        ptAdjusted.y += _startingLocation.dyContactCenterAnchor;
        return ptAdjusted;
    }

    bool CHandleProcessor::_HasMovedFromInitialPosition(const POINT& currentPosition)
    {
        if ((currentPosition.x != _startingLocation.pt.x) || (currentPosition.y != _startingLocation.pt.y))
        {
            _fHasMovedFromInitialPosition = true;
        }

        return _fHasMovedFromInitialPosition;
    }

    HRESULT CHandleProcessor::OnStartGripperDrag(POINT unFilteredPoint, unsigned int cCurrentLineHeight)
    {
        HRESULT hr = E_UNEXPECTED;
        if (!_fActive)
        {
             unsigned __int64 timestamp = GetTickCount64();
            _fActive = true;
            _fHasMovedFromInitialPosition = false;

            if (_startingLocation.ptAnchor.y + _startingLocation.cGripperDiameter > unFilteredPoint.y)
            {
                // The initial contact is above the center of the gripper hit target.  
                // Expand the initial 
                _startingLocation.dyExpandedInitialZone = _startingLocation.ptAnchor.y + _startingLocation.cGripperDiameter - unFilteredPoint.y;
            }
            _startingLocation.dyContactCenterAnchor = _startingLocation.ptAnchor.y - unFilteredPoint.y;
            _UpdateStartingPoint(unFilteredPoint, timestamp, cCurrentLineHeight);
            hr = S_OK;
        }
        return hr;
    }

    void CHandleProcessor::SetGripperDisplayLocation(POINT ptAnchor, POINT ptGripperTopOffsetFromAnchor, unsigned int cDiameter)
    {
        _startingLocation.ptAnchor = ptAnchor;
        _startingLocation.ptGripperCenterOffsetFromAnchor = ptGripperTopOffsetFromAnchor;
        _startingLocation.cGripperDiameter = static_cast<int>(cDiameter);      
        _startingLocation.dyContactCenterAnchor = _startingLocation.ptAnchor.y - _startingLocation.pt.y;
    }

    void CHandleProcessor::_UpdateStartingPoint(POINT unFilteredPoint, unsigned __int64 timestamp, unsigned int cCurrentLineHeight)
    {
        _startingLocation.timestamp = timestamp;
        _startingLocation.pt = unFilteredPoint;
        _lastLocation.timestamp = timestamp;
        _lastLocation.cCurrentLineHeight = cCurrentLineHeight;
    }

    HRESULT CHandleProcessor::OnGripperDrag(POINT unFilteredPoint, unsigned int cCurrentLineHeight, _Out_ POINT *ptResult)
    {
        HRESULT hr = E_UNEXPECTED;
        ptResult->x = 0;
        ptResult->y = 0;
        if (_fActive)
        {
            // The timestamps that the frameworks pass us aren't reliable.
            unsigned __int64 timestamp = GetTickCount64();
            if ((_lastLocation.pt.y != unFilteredPoint.y) || (_lastLocation.pt.x != unFilteredPoint.x))
            {
                _lastLocation.timestamp = timestamp;
                _lastLocation.pt = unFilteredPoint;
                _lastLocation.cCurrentLineHeight = cCurrentLineHeight;
            }
            *ptResult = _CalcLogicalPos();
            hr = S_OK;
        }
        return hr;
    }

    HRESULT CHandleProcessor::SetHimetricsPerPixel(float rScale)
    {
        if (IsNonZero(rScale))
        {
            _rCurrentScale = rScale;
            _LoadSafetyZoneSettings();
        }
        return S_OK;
    }

    HRESULT CHandleProcessor::OnEndGripperDrag()
    {
        HRESULT hr = E_UNEXPECTED;
        if (_fActive)
        {
            hr = S_OK;
        }
        _fActive = false;
        return hr;
    }
    
    void CHandleProcessor::OverrideSettings(unsigned int cVerticalMovementLimit, unsigned __int64 cDragStopAnchorResetTicks)
    {
        _fUseRegSettings = false;
        CHandleProcessor::s_cVerticalMovementUpLimit = cVerticalMovementLimit;
        CHandleProcessor::s_cVerticalMovementDownLimit = cVerticalMovementLimit;
        CHandleProcessor::s_cDragStopAnchorResetTicks = cDragStopAnchorResetTicks;
        _LoadSafetyZoneSettings();
    }

    void CHandleProcessor::_LoadSafetyZoneSettings()
    {
        if (IsNonZero(_rCurrentScale))
        {
            _cDragStopAnchorResetTicks = static_cast<int>(s_cDragStopAnchorResetTicks);
            _cVerticalDriftUpLimit = static_cast<int>(CHandleProcessor::s_cVerticalMovementUpLimit / _rCurrentScale);
            _cVerticalDriftDownLimit = static_cast<int>(CHandleProcessor::s_cVerticalMovementDownLimit / _rCurrentScale);

            if (_fUseRegSettings)
            {
                DWORD dwDriftLimit = 0;
                DWORD cbDriftLimit = 0;
                
                cbDriftLimit = sizeof(dwDriftLimit);
                if (RegGetValue(HKEY_LOCAL_MACHINE,  
                                L"Software\\Microsoft\\Input\\Settings",  
                                L"VerticalMovementLimit",  
                                RRF_RT_REG_DWORD,  
                                nullptr, &dwDriftLimit, &cbDriftLimit)
                    == ERROR_SUCCESS)  
                {
                    static const DWORD cMaxVerticalLimit = 5000; // 5.00 cm in HiMetrics
                    if (dwDriftLimit < cMaxVerticalLimit)
                    {
                        _cVerticalDriftDownLimit = static_cast<int>(dwDriftLimit / _rCurrentScale);
                        _cVerticalDriftUpLimit = _cVerticalDriftDownLimit;
                    }
                }
                
                cbDriftLimit = sizeof(dwDriftLimit);  
                if (RegGetValue(HKEY_LOCAL_MACHINE,  
                                L"Software\\Microsoft\\Input\\Settings",  
                                L"VerticalMovementUpLimit",  
                                RRF_RT_REG_DWORD,  
                                nullptr, &dwDriftLimit, &cbDriftLimit)
                    == ERROR_SUCCESS)  
                {
                    static const DWORD cMaxVerticalLimit = 5000; // 5.00 cm in HiMetrics
                    if (dwDriftLimit < cMaxVerticalLimit)
                    {
                        _cVerticalDriftUpLimit = static_cast<int>(dwDriftLimit / _rCurrentScale);
                    }
                }
            }
        }
    }
};

