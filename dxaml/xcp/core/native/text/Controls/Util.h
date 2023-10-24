// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <string>
#include <tom.h>
#include <Windows.Foundation.h>
#include <Windows.System.h>

#define RETURN_FALSE_IF_FAILED(hr) if (FAILED((hr))) { return false; }

namespace DispatcherHelper
{
    HRESULT GetDispatcherQueueForCurrentThread(_COM_Outptr_ msy::IDispatcherQueue** value);

    template<typename Func_t>
    static HRESULT ScheduleCallbackOnCurrentThread(const Func_t& callback)
    {
        Microsoft::WRL::ComPtr<msy::IDispatcherQueue> pThisThreadQueue;
        IFC_RETURN(GetDispatcherQueueForCurrentThread(&pThisThreadQueue));

        boolean enqueueSucceeded;
        IFC_RETURN(pThisThreadQueue->TryEnqueue(
            wrl::Callback<wrl::Implements<
                wrl::RuntimeClassFlags<wrl::ClassicCom>,
                msy::IDispatcherQueueHandler,
                wrl::FtmBase>>(callback).Get(), &enqueueSucceeded));

        return enqueueSucceeded ? S_OK : E_FAIL;
    }
}

HRESULT SetTimerInterval(_In_ CCoreServices* pCore, _In_ CDispatcherTimer* pTimer, float intervalInS);

template <class T, HRESULT (T::*Method)()>
HRESULT CreateTimer(
    _In_         CCoreServices*     pCore,
    _In_         T*                 pTargetObject,
                 float              intervalInS,
    _COM_Outptr_ CDispatcherTimer** ppTimer)
{
    *ppTimer = nullptr;

    CREATEPARAMETERS cp(pCore);
    xref_ptr<CDispatcherTimer> dispatcherTimer;
    IFC_RETURN(CreateDO(dispatcherTimer.ReleaseAndGetAddressOf(), &cp));

    CValue value;
    value.SetInternalHandler([](_In_ CDependencyObject* pSender, _In_ CEventArgs* /*pEventArgs*/) -> HRESULT
    {
        CDispatcherTimer *pTimer = nullptr;
        IFC_RETURN(DoPointerCast(pTimer, pSender));

        // Put the timer in a state where it can fire again if not stopped or restarted. Do this before
        // callng the actual timer handler routine (Method) so that it can call Stop/Start as needed.
        IFC_RETURN(pTimer->WorkComplete());

        // Check for nullptr since timer can fire after the target object (weak ref) is released.
        xref_ptr<T> pThis = static_sp_cast<T>(pTimer->GetTargetObject());
        if (pThis != nullptr)
        {
            IFC_RETURN((pThis->*Method)());
        }

        return S_OK;
    });

    IFC_RETURN(dispatcherTimer->AddEventListener(
        EventHandle(KnownEventIndex::DispatcherTimer_Tick),
        &value,
        EVENTLISTENER_INTERNAL,
        nullptr,
        FALSE));
    IFC_RETURN(SetTimerInterval(pCore, dispatcherTimer.get(), intervalInS));
    IFC_RETURN(dispatcherTimer->SetTargetObject(pTargetObject));
    IFC_RETURN(dispatcherTimer->Stop());

    *ppTimer = dispatcherTimer.detach();

    return S_OK;
}

template <typename T>
class ScopedIncrement final
{
public:
    ScopedIncrement(_In_ T* target) : m_target(target)
    {
        ++(*m_target);
    }

    ScopedIncrement(const ScopedIncrement&) = delete;

    ScopedIncrement(ScopedIncrement&& move) :
        m_target(move.m_target)
    {
        move.m_target = nullptr;
    }

    ~ScopedIncrement()
    {
        Reset();
    }

    ScopedIncrement& operator=(const ScopedIncrement&) = delete;

    ScopedIncrement& operator=(ScopedIncrement&& move)
    {
        if (this != &move)
        {
            Reset();
            m_target = move.m_target;
            move.m_target = nullptr;
        }

        return *this;
    }

private:
    void Reset()
    {
        if (m_target != nullptr)
        {
            --(*m_target);
            m_target = nullptr;
        }
    }

    T* m_target;
};

wf::Rect ToRect(const RECT& r);
RECT ToRECT(const wf::Rect& r);

unsigned int GetCookie(_In_ ITextRange2* range);
HRESULT GetCookieVerifyUnique(_In_ ITextRange2* range, unsigned int& cookie);
HRESULT GetAllCookies(_In_ ITextRange2* range, std::vector<unsigned int>& cookies);
HRESULT SetCookie(_In_ ITextRange2* range, unsigned int cookie);
HRESULT SetAllCookies(_In_ ITextRange2* range, const std::vector<unsigned int>& cookies);

//====================================================================================================================
// RichEdit helpers
//====================================================================================================================

unsigned int GetRangeLength(_In_ ITextRange* range);
bool DoesRangeSpanMultipleUnits(_In_ ITextRange2* range, long tomUnit);
void GetEncompassingSentenceBoundaries(_In_ ITextRange2* range, _Out_ long* cpStart, _Out_ long* cpEnd);
std::wstring GetTextFromRange(_In_ ITextRange* range);

void GetRangeForStory(_In_ ITextDocument2* doc, _COM_Outptr_ ITextRange2** range);
void GetRangeForPoint(_In_ ITextDocument2* doc, long x, long y, _COM_Outptr_ ITextRange2** range);
void GetRangeForRect(_In_ ITextDocument2* doc, const RECT& bounds, _COM_Outptr_ ITextRange2** range);
void GetRangeForRect(_In_ ITextDocument2* doc, const wf::Rect& bounds, _COM_Outptr_ ITextRange2** range);
wf::Rect GetRectForStory(_In_ ITextDocument2* doc);
wf::Rect GetRectForRange(_In_ ITextRange2* range,_Out_opt_ float* baseline = nullptr);

void MoveDegenerateRangeBeforePunctuationsAndSpaces(_Inout_ ITextRange2* range);
void MoveDegenerateRangeAfterPunctuationsAndSpaces(_Inout_ ITextRange2* range);
void ExcludeLeadingWhiteSpacesFromRange(_Inout_ ITextRange2* range);
void ExcludeTrailingWhiteSpacesFromRange(_Inout_ ITextRange2* range);
void SkipLeadingWhiteSpaces(_Inout_ ITextRange2* range);
void SkipTrailingWhiteSpaces(_Inout_ ITextRange2* range);

bool IsRangeDegenerate(_In_ ITextRange* range);

enum class AnchorPoint
{
    Begin = 0,
    End,
};

/// <remarks>
/// The range is updated only if it is inside text (aka no updates if the range is at beginning/end of text),
/// or if it doesn't overlap with text at all.
/// </remarks>
bool SnapDegenerateRangeToText(_Inout_ ITextRange2* range, const std::wstring& text, AnchorPoint anchorPoint);

bool ExtendRangeWhileCookie(_Inout_ ITextRange2* range, unsigned int cookie);

//====================================================================================================================
// Debug
//====================================================================================================================

std::wstring MakePrintableChar(wchar_t c);
std::wstring MakePrintableString(const std::wstring& str);
std::wstring DocumentToString(_In_ ITextDocument2* doc);
std::wstring RangeToString(_In_ ITextRange2* range);
std::wstring PointToString(const wf::Point& point);
std::wstring RectToString(const wf::Rect& rect);

#if defined(DEBUG)
struct TimedExecutionStats
{
    unsigned long long m_lastExec = 0;
    float m_averageExec = 0.f;
    unsigned long m_countExec = 0;
};

/// <summary>
/// Class that can be used to track function or block-level performance.
/// To use, declare a variable of this type at the beginning of your block and a static TimedExecutionStats.
/// </summary>
struct TimedExecution
{
    TimedExecution(TimedExecutionStats& t) : stats(t) { stats.m_lastExec = GetTickCount64(); }
    ~TimedExecution()
    {
        stats.m_lastExec = GetTickCount64() - stats.m_lastExec;
        auto p = stats.m_countExec * stats.m_averageExec;
        stats.m_countExec++;
        if (stats.m_countExec != 0)
        {
            stats.m_averageExec = (p + stats.m_lastExec) / stats.m_countExec;
        }
    }
    TimedExecutionStats& stats;
};

#define TIME_EXECUTION_OF_THIS_BLOCK(stats) \
    static TimedExecutionStats stats;\
    TimedExecution _timedExecutionVariable_##__COUNTER__(stats);
#else // defined(DEBUG)
#define TIME_EXECUTION_OF_THIS_BLOCK(stats)
#endif // defined(DEBUG)
