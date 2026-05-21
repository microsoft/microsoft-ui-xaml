// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "UtilitiesRoutineHelper.h"
#include "KeyboardRoutineHelper.h"
#include "InputRoutineHelper.h"
#include "TraceConsumerRoutineHelper.h"
#include "ETWWaiterServerHelper.h"
#include "WindowingHelper.h"

#include "AppAnalysisHelper.h"
#include <MockDComp-DPIHelpers.h>

using namespace WEX::Common;
using namespace Microsoft::UI::Xaml::Tests::Common;

#pragma region General utilities

HRESULT RpcIsOneCore(__RPC__out BOOLEAN* pIsOneCore)
{
    COM_START
    {
        *pIsOneCore = UtilitiesRoutineHelper::IsOneCore();
    }
    COM_END
}

HRESULT RpcIsXBox(__RPC__out BOOLEAN* pIsXBox)
{
    COM_START
    {
        *pIsXBox = UtilitiesRoutineHelper::IsXBox();
    }
    COM_END
}

HRESULT RpcIsDesktop(__RPC__out BOOLEAN* pIsDesktop)
{
    COM_START
    {
        *pIsDesktop = UtilitiesRoutineHelper::IsDesktop();
    }
    COM_END
}

HRESULT RpcSetRegKey(__RPC__in_string const wchar_t* path, __RPC__in_string const wchar_t* name, DWORD dwValue, __RPC__out BOOLEAN* nameExisted, BOOLEAN currentUser)
{
    COM_START_GROUP(L"RpcSetRegKey")
    {
        *nameExisted = UtilitiesRoutineHelper::SetRegKey(path, name, dwValue, currentUser);
    }
    COM_END
}

HRESULT RpcDeleteRegKey(__RPC__in_string const wchar_t* path, __RPC__in_string const wchar_t* name, BOOLEAN currentUser)
{
    COM_START_GROUP(L"RpcDeleteRegKey")
    {
        UtilitiesRoutineHelper::DeleteRegKey(path, name, currentUser);
    }
    COM_END
}

HRESULT RpcEnableChangingTimeZone(BOOLEAN enable)
{
    COM_START_GROUP(L"RpcEnableChangingTimeZone")
    {
        UtilitiesRoutineHelper::EnableChangingTimeZone(!!enable);
    }
    COM_END
}

HRESULT RpcSetTimeZone(__RPC__in_string const WCHAR* timezoneId)
{
    COM_START_GROUP(L"RpcSetTimeZone")
    {
        UtilitiesRoutineHelper::SetTimeZone(timezoneId);
    }
    COM_END
}

HRESULT RpcRunCommandLine(__RPC__in_string const WCHAR* commandLine, __RPC__out DWORD* pExitCode)
{
    COM_START_GROUP(L"RpcRunCommandLine")
    {
        UtilitiesRoutineHelper::RunCommandLine(commandLine, pExitCode);
    }
    COM_END
}

#pragma endregion

#pragma region Input Injection

HRESULT RpcInjectPress(InputDevice inputDevice, POINT pt, UINT durationMs, UINT pressCount, UINT pressDeltaMs)
{
    COM_START_GROUP(L"RpcInjectPress")
    {
        InputRoutineHelper::InjectPress(inputDevice, pt, durationMs, pressCount, pressDeltaMs);
    }
    COM_END
}

HRESULT RpcInjectPenBarrelTap(POINT pt)
{
    COM_START_GROUP(L"RpcInjectPenBarrelTap")
    {
        InputRoutineHelper::InjectPenBarrelTap(pt);
    }
    COM_END
}

HRESULT RpcInjectDragWithPenBarrelDown(POINT ptStart, POINT ptEnd, UINT durationInDragMs)
{
    COM_START_GROUP(L"RpcInjectDragWithPenBarrelDown")
    {
        InputRoutineHelper::InjectDragWithPenBarrelDown(ptStart, ptEnd, durationInDragMs);
    }
    COM_END
}

HRESULT RpcInjectPressAndDrag(InputDevice inputDevice, POINT ptStart, POINT ptEnd, UINT durationInDragMs, UINT durationInHoldMs)
{
    COM_START_GROUP(L"RpcInjectPressAndDrag")
    {
        InputRoutineHelper::InjectPressAndDrag(inputDevice, ptStart, ptEnd, durationInDragMs, durationInHoldMs);
    }
    COM_END
}

HRESULT RpcInjectZoom(POINT ptStartFinger1, POINT ptStartFinger2, FLOAT dbDirection, UINT durationMs, UINT distance)
{
    COM_START_GROUP(L"RpcInjectZoom")
    {
        InputRoutineHelper::InjectZoom(ptStartFinger1, ptStartFinger2, dbDirection, durationMs, distance);
    }
    COM_END
}

HRESULT RpcInjectRotate(POINT startFinger1, POINT startFinger2, FLOAT rotationAngle, UINT durationMs, BOOLEAN pivotRotate)
{
    COM_START_GROUP(L"RpcInjectRotate")
    {
        InputRoutineHelper::InjectRotate(startFinger1, startFinger2, rotationAngle, durationMs, pivotRotate);
    }
    COM_END
}

HRESULT RpcInjectDynamicPress(InputDevice inputDevice, POINT ptPress, DWORD dwWidth, DWORD dwHeight, UINT contactID)
{
    COM_START_GROUP(L"RpcInjectDynamicPress")
    {
        InputRoutineHelper::InjectDynamicPress(inputDevice, ptPress, dwWidth, dwHeight, contactID);
    }
    COM_END
}

HRESULT RpcInjectDynamicRelease(UINT contactID)
{
    COM_START_GROUP(L"RpcInjectDynamicRelease")
    {
        InputRoutineHelper::InjectDynamicRelease(contactID);
    }
    COM_END
}

HRESULT RpcSendMouseButtonInput(MouseButton button, POINT position, BOOLEAN down)
{
    COM_START_GROUP(L"RpcSendMouseButtonInput")
    {
        InputRoutineHelper::SendMouseButtonInput(button, position, down);
    }
    COM_END
}

HRESULT RpcSendMouseDragInput(MouseButton button, POINT positionSrc, POINT positionDest)
{
    COM_START_GROUP(L"RpcSendMouseDragInput")
    {
        InputRoutineHelper::SendMouseDragInput(button, positionSrc, positionDest);
    }
    COM_END
}

HRESULT RpcSendMouseMoveInput(POINT position)
{
    COM_START_GROUP(L"RpcSendMouseMoveInput")
    {
        InputRoutineHelper::SendMouseMoveInput(position);
    }
    COM_END
}

HRESULT RpcSendMouseWheelInput(POINT position, INT numberOfWheelClicks)
{
    COM_START_GROUP(L"RpcSendMouseWheelInput")
    {
        InputRoutineHelper::SendMouseWheelInput(position, numberOfWheelClicks);
    }
    COM_END
}

HRESULT RpcSendKeyInput(UINT16 keyCode, BOOLEAN down, BOOLEAN unicode)
{
    COM_START_GROUP(L"RpcSendKeyInput")
    {
        KeyboardRoutineHelper::SendKeyInput(keyCode, down, unicode);
    }
    COM_END
}
#pragma endregion

#pragma region TraceConsumer

HRESULT RpcTraceConsumerStartProvider(GUID xamlProvider)
{
    COM_START_GROUP(L"RpcTraceConsumerStartProvider")
    {
        TraceConsumerRoutineHelper::Start(xamlProvider);
    }
    COM_END
}

HRESULT RpcTraceConsumerStart()
{
    COM_START_GROUP(L"RpcTraceConsumerStart")
    {
        TraceConsumerRoutineHelper::Start();
    }
    COM_END
}

HRESULT RpcTraceConsumerStop()
{
    COM_START_GROUP(L"RpcTraceConsumerStop")
    {
        TraceConsumerRoutineHelper::Stop();
    }
    COM_END
}

HRESULT RpcVerifyEventTraced(__RPC__in_string const wchar_t* event, UINT count)
{
    COM_START_GROUP(L"RpcVerifyEventTraced")
    {
        TraceConsumerRoutineHelper::VerifyEventTraced(event, count);
    }
    COM_END
}

HRESULT RpcVerifyEventTracedById(int eventId, UINT count)
{
    COM_START_GROUP(L"RpcVerifyEventTracedById")
    {
        TraceConsumerRoutineHelper::VerifyEventTraced(eventId, count);
    }
    COM_END
}

HRESULT RpcVerifyEventTracedMoreThanOnce(int eventId)
{
    COM_START_GROUP(L"RpcVerifyEventTracedMoreThanOnce")
    {
        TraceConsumerRoutineHelper::VerifyEventTraced(eventId);
    }
    COM_END
}

HRESULT RpcEnableTracingByEventId(int eventId)
{
    COM_START_GROUP(L"RpcEnableTracingByEventId")
    {
        TraceConsumerRoutineHelper::EnableTracingByEventId(eventId);
    }
    COM_END
}

#pragma endregion

#pragma region ETWWaiter

HRESULT RpcETWWaiterStart(GUID providerGuid, unsigned long eventId)
{
    COM_START_GROUP(L"RpcETWWaiterStart")
    {
        ETWWaiterServerHelper::Start(providerGuid, eventId);
    }
    COM_END
}

HRESULT RpcETWWaiterStartWithTaskName(GUID providerGuid, __RPC__in_string const wchar_t* taskName)
{
    COM_START_GROUP(L"RpcETWWaiterStartWithTaskName")
    {
        ETWWaiterServerHelper::StartWithTaskName(providerGuid, taskName);
    }
    COM_END
}

HRESULT RpcETWWaiterStartWithPayLoad(GUID providerGuid, unsigned long eventId, __RPC__in_string const wchar_t* payloadCriteria)
{
    COM_START_GROUP(L"RpcETWWaiterStartWithPayLoad")
    {
        ETWWaiterServerHelper::StartWithPayloadMatch(providerGuid, eventId, payloadCriteria);
    }
    COM_END
}

HRESULT RpcETWWaiterWait(GUID providerGuid, unsigned long eventId, unsigned int timeoutMs)
{
    COM_START_GROUP(L"RpcETWWaiterWait")
    {
        ETWWaiterServerHelper::Wait(providerGuid, eventId, timeoutMs);
    }
    COM_END
}

HRESULT RpcETWWaiterWaitForTaskName(GUID providerGuid, __RPC__in_string const wchar_t* taskName, unsigned int timeout)
{
    COM_START_GROUP(L"RpcETWWaiterWaitForTaskName")
    {
        ETWWaiterServerHelper::WaitForTaskName(providerGuid, taskName, timeout);
    }
    COM_END
}

HRESULT RpcETWWaiterStop(GUID providerGuid, unsigned long eventId)
{
    COM_START_GROUP(L"RpcETWWaiterStop")
    {
        ETWWaiterServerHelper::Stop(providerGuid, eventId);
    }
    COM_END
}

HRESULT RpcETWWaiterStopTaskName(GUID providerGuid, __RPC__in_string const wchar_t* taskName)
{
    COM_START_GROUP(L"RpcETWWaiterStopTaskName")
    {
        ETWWaiterServerHelper::StopTaskName(providerGuid, taskName);
    }
    COM_END
}

HRESULT RpcETWWaiterCount(__RPC__in unsigned int *waiterCount)
{
    COM_START_GROUP(L"RpcETWWaiterCount")
    {
        ETWWaiterServerHelper::GetActiveWaiterCount(waiterCount);
    }
    COM_END
}

#pragma endregion

#pragma region WindowingHelper

HRESULT RpcSetForegroundWindow(LONG_PTR handle)
{
    COM_START_GROUP(L"RpcSetForegroundWindow")
    {
        WindowingHelper::GetRightsAndSetForegroundWindow(reinterpret_cast<HWND>(handle));
    }
    COM_END
}

HRESULT RpcMaximizeDesktopWindow(LONG_PTR handle)
{
    COM_START_GROUP(L"RpcMaximizeDesktopWindow")
    {
        WindowingHelper::MaximizeDesktopWindow(reinterpret_cast<HWND>(handle));
    }
    COM_END
}

HRESULT RpcSetDesktopWindowSize(LONG_PTR handle, unsigned int width, unsigned int height)
{
    COM_START_GROUP(L"RpcSetDesktopWindowSize")
    {
        WindowingHelper::SetDesktopWindowSize(reinterpret_cast<HWND>(handle), width, height);
    }
    COM_END
}

HRESULT RpcMoveDesktopWindow(LONG_PTR handle, unsigned int x, unsigned int y)
{
    COM_START_GROUP(L"RpcMoveDesktopWindow")
    {
        WindowingHelper::MoveDesktopWindow(reinterpret_cast<HWND>(handle), x, y);
    }
    COM_END
}

HRESULT RpcIsDesktopWindowMaximized(LONG_PTR hwnd, __RPC__out BOOL* isMaximized)
{
    COM_START_GROUP(L"RpcIsDesktopWindowMaximized")
    {
        *isMaximized = WindowingHelper::IsDesktopWindowMaximized(reinterpret_cast<HWND>(hwnd));
    }
    COM_END
}

HRESULT RpcCaptureScreenshot(__RPC__in_string const WCHAR* filename)
{
    // RpcCaptureScreenshot is called by the input injection utilities
    // which can also be called from the UI thread or the test thread.
    // This causes WEX groups to close in random order and fail BVTs.
    // We disable grouping for RpcCaptureScreenshot.
    COM_START
    {
        ScreenCapture::TakeScreenshot(filename);
    }
    COM_END
}

#pragma endregion

#pragma region AppAnalysis

HRESULT RpcEnableRule(
    unsigned int processId,
    __RPC__in_string const WCHAR* ruleId,
    __RPC__in_string const WCHAR* testIdentifier,
    BOOLEAN shouldHaveSourceInfo)
{
    COM_START
    {
        AppAnalysisHelper::EnableRule(processId, ruleId, testIdentifier, !!shouldHaveSourceInfo);
    }
    COM_END
}

HRESULT RpcVerifyRuleTriggered(unsigned int timesTriggered)
{
    COM_START
    {
        AppAnalysisHelper::VerifyRuleTriggered(timesTriggered);
    }
    COM_END
}

HRESULT RpcVerifyRuleNotTriggered()
{
    COM_START
    {
        AppAnalysisHelper::VerifyRuleNotTriggered();
    }
    COM_END
}

HRESULT RpcVerifyMeasurement(unsigned int index, unsigned int measurementUnit, double meausurementValue)
{
    COM_START
    {
        AppAnalysisHelper::VerifyMeasurement(index, measurementUnit, meausurementValue);
    }
    COM_END
}

HRESULT RpcVerifySourceInfo(unsigned int index, __RPC__in_string const WCHAR* fileName, unsigned int line, unsigned int column)
{
    COM_START
    {
        AppAnalysisHelper::VerifySourceInfo(index, fileName, line, column);
    }
    COM_END
}

HRESULT RpcVerifyCanLinkToLVT(unsigned int index, ObjectHandle lvtHandle)
{
    COM_START
    {
        AppAnalysisHelper::VerifyCanLinkToLVT(index, lvtHandle);
    }
    COM_END
}

HRESULT RpcVerifyDescription(
   unsigned int index,
   unsigned int resourceId,
   unsigned int count,
   __RPC__in_ecount_full(count) const WCHAR **args)
{
    COM_START
    {
        AppAnalysisHelper::VerifyDescription(index, resourceId, count, args);
    }
    COM_END
}

HRESULT RpcDisableCurrentRule()
{
    COM_START
    {
        AppAnalysisHelper::DisableCurrentRule();
    }
    COM_END
}

HRESULT RpcRunAsBVT()
{
    COM_START
    {
        UtilitiesRoutineHelper::RunAsBVT();
    }
    COM_END
}

HRESULT RpcTerminateProcess(__RPC__in_string const wchar_t* appExe)
{
    COM_START
    {
        auto hr = UtilitiesRoutineHelper::TerminateProcesses(appExe);
        if (S_OK != hr)
        {
            return hr;
        }
    }
    COM_END
}

HRESULT RpcResetInputInjection()
{
    COM_START
    {
        InputRoutineHelper::Reset();
    }
    COM_END
}

HRESULT RpcSetDpi(LUID displayAdapterId, unsigned int displayAdapterTargetId, int newDpi, _Out_ int* oldDpi)
{
    COM_START
    {
        return SetDisplayDpiOverride(displayAdapterId, displayAdapterTargetId, newDpi, oldDpi);
    }
    COM_END
}

#pragma endregion
