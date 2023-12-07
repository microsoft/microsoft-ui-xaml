// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Util.h"
#include <FeatureFlags.h>
#include <sstream>
#include "Timer.h"

namespace DispatcherHelper
{
    HRESULT GetDispatcherQueueForCurrentThread(_COM_Outptr_ msy::IDispatcherQueue** value)
    {
        ::Microsoft::WRL::ComPtr<msy::IDispatcherQueueStatics> dispatcherQueueStatics;
        RETURN_IF_FAILED(wf::GetActivationFactory(
            ::wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(), &dispatcherQueueStatics));
        RETURN_IF_FAILED(dispatcherQueueStatics->GetForCurrentThread(value));

        return S_OK;
    }
}

HRESULT SetTimerInterval(_In_ CCoreServices* pCore, _In_ CDispatcherTimer* pTimer, float intervalInS)
{
    CREATEPARAMETERS cp(pCore);
    xref_ptr<CTimeSpan> pTimeSpan;
    IFC_RETURN(CreateDO(pTimeSpan.ReleaseAndGetAddressOf(), &cp));

    pTimeSpan->m_rTimeSpan = intervalInS;
    IFC_RETURN(pTimer->SetValueByKnownIndex(
        KnownPropertyIndex::DispatcherTimer_Interval,
        pTimeSpan.get()));

    return S_OK;
}

wf::Rect ToRect(const RECT& r)
{
    wf::Rect value;
    value.X = static_cast<float>(r.left);
    value.Y = static_cast<float>(r.top);
    value.Width = static_cast<float>(r.right - r.left);
    value.Height = static_cast<float>(r.bottom - r.top);
    return value;
}

RECT ToRECT(const wf::Rect& r)
{
    return {
        static_cast<long>(r.X + .5f),
        static_cast<long>(r.Y + .5f),
        static_cast<long>(r.X + r.Width + .5f),
        static_cast<long>(r.Y + r.Height + .5f)
    };
}

HRESULT GetAllCookies(_In_ ITextRange2* range, std::vector<unsigned int>& cookies)
{
    cookies.clear();

    long cpStart;
    IFC_RETURN(range->GetStart(&cpStart));

    long cpEnd;
    IFC_RETURN(range->GetEnd(&cpEnd));

    if (cpStart > cpEnd)
    {
        std::swap(cpStart, cpEnd);
    }

    wrl::ComPtr<ITextRange2> tempRange;
    IFC_RETURN(range->GetDuplicate2(&tempRange));

    for (long i = cpStart; i < cpEnd; i++)
    {
        IFC_RETURN(tempRange->SetStart(i));
        IFC_RETURN(tempRange->SetEnd(i + 1));

        unsigned int cookie = GetCookie(tempRange.Get());
        cookies.push_back(cookie);
    }

    return S_OK;
}

HRESULT SetCookie(_In_ ITextRange2* range, unsigned int cookie)
{
    wrl::ComPtr<ITextFont2> font2;
    IFC_RETURN(range->GetFont2(&font2));
    IFC_RETURN(font2->SetCookie(cookie));

    return S_OK;
}

HRESULT SetAllCookies(_In_ ITextRange2* range, const std::vector<unsigned int>& cookies)
{
    long cpStart;
    IFC_RETURN(range->GetStart(&cpStart));

    long cpEnd;
    IFC_RETURN(range->GetEnd(&cpEnd));

    if (cpStart > cpEnd)
    {
        std::swap(cpStart, cpEnd);
    }

    wrl::ComPtr<ITextRange2> tempRange;
    IFC_RETURN(range->GetDuplicate2(&tempRange));

    RETURN_HR_IF(E_INVALIDARG, cpEnd - cpStart != static_cast<long>(cookies.size()));
    for (long i = 0, cp = cpStart; cp < cpEnd; cp++, i++)
    {
        IFC_RETURN(tempRange->SetStart(cp));
        IFC_RETURN(tempRange->SetEnd(cp + 1));

        IFC_RETURN(SetCookie(tempRange.Get(), cookies[i]));
    }

    return S_OK;
}

//====================================================================================================================
// RichEdit helpers
//====================================================================================================================

unsigned int GetRangeLength(_In_ ITextRange* range)
{
    long cpStart;
    FAIL_FAST_IF_FAILED(range->GetStart(&cpStart));

    long cpEnd;
    FAIL_FAST_IF_FAILED(range->GetEnd(&cpEnd));

    return static_cast<unsigned int>(cpEnd >= cpStart ? cpEnd - cpStart : cpStart - cpEnd);
}

bool DoesRangeSpanMultipleUnits(_In_ ITextRange2* range, long tomUnit)
{
    long cpStart;
    FAIL_FAST_IF_FAILED(range->GetStart(&cpStart));

    long cpEnd;
    FAIL_FAST_IF_FAILED(range->GetEnd(&cpEnd));

    wrl::ComPtr<ITextRange2> tempRange;
    FAIL_FAST_IF_FAILED(range->GetDuplicate2(&tempRange));

    FAIL_FAST_IF_FAILED(tempRange->Collapse(tomStart));
    FAIL_FAST_IF_FAILED(tempRange->Move(tomUnit, 1, nullptr));

    long cpTempRangeEnd;
    FAIL_FAST_IF_FAILED(tempRange->GetEnd(&cpTempRangeEnd));

    return cpTempRangeEnd < cpEnd;
}

void GetEncompassingSentenceBoundaries(_In_ ITextRange2* range, _Out_ long* cpStart, _Out_ long* cpEnd)
{
    const std::unordered_set<long> endOfSentenceChars = {
        static_cast<long>(L'.'),
        static_cast<long>(L'?'),
        static_cast<long>(L'!')
    };

    wrl::ComPtr<ITextRange2> rangeBefore;
    FAIL_FAST_IF_FAILED(range->GetDuplicate2(&rangeBefore));
    ExcludeLeadingWhiteSpacesFromRange(rangeBefore.Get());
    ExcludeTrailingWhiteSpacesFromRange(rangeBefore.Get());

    wrl::ComPtr<ITextRange2> currentSentence;
    FAIL_FAST_IF_FAILED(rangeBefore->GetDuplicate2(&currentSentence));

    FAIL_FAST_IF_FAILED(rangeBefore->Collapse(tomStart));
    FAIL_FAST_IF_FAILED(rangeBefore->Move(tomCharacter, -1, nullptr));

    long previousCharacter;
    FAIL_FAST_IF_FAILED(rangeBefore->GetChar(&previousCharacter));

    long cpStartBefore, cpEndBefore;
    FAIL_FAST_IF_FAILED(currentSentence->GetStart(&cpStartBefore));
    FAIL_FAST_IF_FAILED(currentSentence->GetEnd(&cpEndBefore));

    FAIL_FAST_IF_FAILED(currentSentence->Expand(tomSentence, nullptr));
    ExcludeLeadingWhiteSpacesFromRange(currentSentence.Get());
    ExcludeTrailingWhiteSpacesFromRange(currentSentence.Get());

    long cpStartAfter, cpEndAfter;
    FAIL_FAST_IF_FAILED(currentSentence->GetStart(&cpStartAfter));
    FAIL_FAST_IF_FAILED(currentSentence->GetEnd(&cpEndAfter));

    if ((cpEndAfter <= cpStartBefore) && (endOfSentenceChars.find(previousCharacter) != endOfSentenceChars.end()))
    {
        // Range is the only word in the sentence so just return its boundary.
        *cpStart = cpStartBefore;
        *cpEnd = cpEndBefore;
    }
    else
    {
        *cpStart = cpStartAfter;
        *cpEnd = cpEndAfter;
    }
}

std::wstring GetTextFromRange(_In_ ITextRange* range)
{
    wil::unique_bstr bstr;
    if (S_OK == range->GetText(&bstr) && (bstr != nullptr))
    {
        return std::wstring(bstr.get());
    }
    return std::wstring();
}

void GetRangeForStory(_In_ ITextDocument2* doc, _COM_Outptr_ ITextRange2** range)
{
    wrl::ComPtr<ITextRange2> docRange;
    FAIL_FAST_IF_FAILED(doc->Range2(0, 0, &docRange));
    FAIL_FAST_IF_FAILED(docRange->Expand(tomStory, nullptr));
    *range = docRange.Detach();
}

void GetRangeForPoint(_In_ ITextDocument2* doc, long x, long y, _COM_Outptr_ ITextRange2** range)
{
    // RangeFromPoint2 returns unexpected results when y is above/below the document's content area.
    // Check for these condition and use the beginning/end of the story instead.

    RECT storyRect = ToRECT(GetRectForStory(doc));

    wrl::ComPtr<ITextRange2> hitTestRange;
    if (y < storyRect.top)
    {
        GetRangeForStory(doc, &hitTestRange);
        FAIL_FAST_IF_FAILED(hitTestRange->Collapse(tomStart));
    }
    else if (y > storyRect.bottom)
    {
        GetRangeForStory(doc, &hitTestRange);
        FAIL_FAST_IF_FAILED(hitTestRange->Collapse(tomEnd));
    }
    else
    {
        FAIL_FAST_IF_FAILED(doc->RangeFromPoint2(x, y, tomClientCoord, &hitTestRange));
    }

    *range = hitTestRange.Detach();
}

void GetRangeForRect(_In_ ITextDocument2* doc, const RECT& bounds, _COM_Outptr_ ITextRange2** range)
{
    wrl::ComPtr<ITextRange2> start;
    GetRangeForPoint(doc, bounds.left, bounds.top, &start);

    wrl::ComPtr<ITextRange2> end;
    GetRangeForPoint(doc, bounds.right, bounds.bottom, &end);

    long cpEnd;
    FAIL_FAST_IF_FAILED(end->GetStart(&cpEnd));
    FAIL_FAST_IF_FAILED(start->SetEnd(cpEnd));
    *range = start.Detach();
}

void GetRangeForRect(_In_ ITextDocument2* doc, const wf::Rect& bounds, _COM_Outptr_ ITextRange2** range)
{
    RECT r = ToRECT(bounds);
    GetRangeForRect(doc, r, range);
}

wf::Rect GetRectForStory(_In_ ITextDocument2* doc)
{
    wrl::ComPtr<ITextRange2> storyRange;
    GetRangeForStory(doc, &storyRange);

    RECT rect;
    FAIL_FAST_IF_FAILED(storyRange->GetPoint(
        tomClientCoord + tomStart + tomAllowOffClient + TA_LEFT + TA_TOP,
        &rect.left,
        &rect.top));
    FAIL_FAST_IF_FAILED(storyRange->GetPoint(
        tomClientCoord + tomEnd + tomAllowOffClient + TA_LEFT + TA_BOTTOM,
        &rect.right,
        &rect.bottom));

    return ToRect(rect);
}

wf::Rect GetRectForRange(_In_ ITextRange2* range, _Out_opt_ float* baseline)
{
    // TOM API: You'd think we need to pass TA_RIGHT for the right end, but this API works based on character positions (which are half a character behind)
    // This means that in order to get the x coordinate of the end of the range we pass TA_LEFT (but tomEnd).
    // Example:
    //
    //     |T|h|e| |q|u|i|c|k|...
    //     0 1 2 3 4 5 6 7 8 9
    // The end of the first word ('the') is the left edge of the end of the range (cp=3).

    RECT rect;
    FAIL_FAST_IF_FAILED(range->GetPoint(tomClientCoord + tomStart + tomAllowOffClient + TA_LEFT + TA_TOP, &rect.left, &rect.top));
    FAIL_FAST_IF_FAILED(range->GetPoint(tomClientCoord + tomEnd + tomAllowOffClient + TA_LEFT + TA_BOTTOM, &rect.right, &rect.bottom));

    if (baseline != nullptr)
    {
        long x, ybaseline;
        FAIL_FAST_IF_FAILED(range->GetPoint(tomClientCoord + tomEnd + TA_RIGHT + TA_BASELINE, &x, &ybaseline));
        *baseline = static_cast<float>(ybaseline);
    }

    return ToRect(rect);
}

void MoveDegenerateRangeAfterPunctuationsAndSpaces(_Inout_ ITextRange2* range)
{
    wil::unique_variant cset;
    cset.vt = VT_I4;
    cset.intVal = C1_PUNCT | C1_SPACE | C1_CNTRL;
    FAIL_FAST_IF_FAILED(range->MoveEndWhile(&cset, tomForward, nullptr));
    FAIL_FAST_IF_FAILED(range->Collapse(tomEnd));
}

void MoveDegenerateRangeBeforePunctuationsAndSpaces(_Inout_ ITextRange2* range)
{
    wil::unique_variant cset;
    cset.vt = VT_I4;
    cset.intVal = C1_PUNCT | C1_SPACE | C1_CNTRL;
    FAIL_FAST_IF_FAILED(range->MoveStartWhile(&cset, tomBackward, nullptr));
    FAIL_FAST_IF_FAILED(range->Collapse(tomStart));
}

void ExcludeLeadingWhiteSpacesFromRange(_Inout_ ITextRange2* range)
{
    wil::unique_variant cset;
    cset.vt = VT_I4;
    cset.intVal = C1_SPACE | C1_CNTRL;
    FAIL_FAST_IF_FAILED(range->MoveStartWhile(&cset, tomForward, nullptr));
}

void ExcludeTrailingWhiteSpacesFromRange(_Inout_ ITextRange2* range)
{
    wil::unique_variant cset;
    cset.vt = VT_I4;
    cset.intVal = C1_SPACE | C1_CNTRL;
    FAIL_FAST_IF_FAILED(range->MoveEndWhile(&cset, tomBackward, nullptr));
}

void SkipLeadingWhiteSpaces(_Inout_ ITextRange2* range)
{
    wil::unique_variant cset;
    cset.vt = VT_I4;
    cset.intVal = C1_SPACE | C1_CNTRL;
    FAIL_FAST_IF_FAILED(range->MoveStartWhile(&cset, tomBackward, nullptr));
}

void SkipTrailingWhiteSpaces(_Inout_ ITextRange2* range)
{
    wil::unique_variant cset;
    cset.vt = VT_I4;
    cset.intVal = C1_SPACE | C1_CNTRL;
    FAIL_FAST_IF_FAILED(range->MoveEndWhile(&cset, tomForward, nullptr));
}

bool IsRangeDegenerate(_In_ ITextRange* range)
{
    long cpStart;
    FAIL_FAST_IF_FAILED(range->GetStart(&cpStart));

    long cpEnd;
    FAIL_FAST_IF_FAILED(range->GetEnd(&cpEnd));

    return cpStart == cpEnd;
}

/// <remarks>
/// The range is updated only if it is inside text (aka no updates if the range is at beginning/end of text),
/// or if it doesn't overlap with text at all.
/// </remarks>
bool SnapDegenerateRangeToText(_Inout_ ITextRange2* range, const std::wstring& text, AnchorPoint anchorPoint)
{
    // Grab a range centered at range and twice as long as text, see if it contains text, and move range accordingly.
    const long textLength = static_cast<long>(text.length());

    // This function expects a degenerate range as input so collapse the range.
    FAIL_FAST_IF_FAILED(range->Collapse(tomStart));

    long leftPaddingLength = 0;
    FAIL_FAST_IF_FAILED(range->MoveStart(tomCharacter, -(textLength - 1), &leftPaddingLength));
    leftPaddingLength *= -1; // Make length positive since we moved in negative direction.

    long rightPaddingLength = 0;
    FAIL_FAST_IF_FAILED(range->MoveEnd(tomCharacter, textLength, &rightPaddingLength));

    std::wstring rangeText = GetTextFromRange(range);

    long desiredDelta = 0;
    bool rangeMoved = false;
    auto pos = rangeText.find(text);
    if ((pos == std::wstring::npos) || (static_cast<long>(pos) == leftPaddingLength))
    {
        // Insertion point was not inside text - move it back to where it was.
        desiredDelta = leftPaddingLength;
        rangeMoved = false;
    }
    else
    {
        // Insertion point was inside text - move it so it points to the beginning of text.
        desiredDelta = static_cast<long>(pos);

        if (anchorPoint == AnchorPoint::End)
        {
            desiredDelta += static_cast<long>(text.length());
        }

        rangeMoved = true;
    }

    if (desiredDelta != 0)
    {
        long delta = 0;
        FAIL_FAST_IF_FAILED(range->MoveStart(tomCharacter, desiredDelta, &delta));
        FAIL_FAST_IF(delta != desiredDelta); // Sanity check.
    }

    FAIL_FAST_IF_FAILED(range->Collapse(tomStart));

    return rangeMoved;
}

bool ExtendRangeWhileCookie(_Inout_ ITextRange2* range, unsigned int cookie)
{
    bool rangeUpdated = false;

    long cpStart;
    FAIL_FAST_IF_FAILED(range->GetStart(&cpStart));

    long cpEnd;
    FAIL_FAST_IF_FAILED(range->GetEnd(&cpEnd));

    wrl::ComPtr<ITextRange2> tempRange;

    // Move backwards while the cookie matches the specified one.
    FAIL_FAST_IF_FAILED(range->GetDuplicate2(&tempRange));
    for (;;)
    {
        FAIL_FAST_IF_FAILED(tempRange->Collapse(tomStart));

        long delta = 0;
        FAIL_FAST_IF_FAILED(tempRange->MoveStart(tomCharacter, -1, &delta));
        if (delta != -1)
        {
            break;
        }

        unsigned int currentCookie = GetCookie(tempRange.Get());
        if (currentCookie != cookie)
        {
            break;
        }

        --cpStart;
        rangeUpdated = true;
    }

    // Move forwards while the cookie matches the specified one.
    FAIL_FAST_IF_FAILED(range->GetDuplicate2(&tempRange));
    for (;;)
    {
        FAIL_FAST_IF_FAILED(tempRange->Collapse(tomEnd));

        long delta = 0;
        FAIL_FAST_IF_FAILED(tempRange->MoveEnd(tomCharacter, 1, &delta));
        if (delta != 1)
        {
            break;
        }

        unsigned int currentCookie = GetCookie(tempRange.Get());
        if (currentCookie != cookie)
        {
            break;
        }

        ++cpEnd;
        rangeUpdated = true;
    }

    if (rangeUpdated)
    {
        FAIL_FAST_IF_FAILED(range->SetStart(cpStart));
        FAIL_FAST_IF_FAILED(range->SetEnd(cpEnd));
    }

    return rangeUpdated;
}

//====================================================================================================================
// Debug
//====================================================================================================================

std::wstring MakePrintableChar(wchar_t c)
{
    std::wstringstream ss;

    if (iswprint(c))
    {
        ss << c;
    }
    else
    {
        ss << L"[0x" << std::hex << static_cast<unsigned int>(c) << std::dec << L"]";
    }

    return ss.str();
}

std::wstring MakePrintableString(const std::wstring& str)
{
    std::wstringstream ss;

    for (auto c : str)
    {
        ss << MakePrintableChar(c);
    }

    return ss.str();
}

std::wstring DocumentToString(_In_ ITextDocument2* doc)
{
    wrl::ComPtr<ITextRange2> storyRange;
    GetRangeForStory(doc, &storyRange);

    std::wstringstream ss;
    ss << L"{Story: " << RangeToString(storyRange.Get()) << L"}";

    return ss.str();
}

std::wstring RangeToString(_In_ ITextRange2* range)
{
    long cpStart;
    FAIL_FAST_IF_FAILED(range->GetStart(&cpStart));
    long cpEnd;
    FAIL_FAST_IF_FAILED(range->GetEnd(&cpEnd));

    std::wstring rangeText = GetTextFromRange(range);

    std::wstringstream ss;
    ss << L"{[" << cpStart << L", " << cpEnd << L"]: \'" << MakePrintableString(rangeText) << L"\'}";

    return ss.str();
}

std::wstring PointToString(const wf::Point& point)
{
    std::wstringstream ss;
    ss << L"(" << point.X << L", " << point.Y << L")";

    return ss.str();
}

std::wstring RectToString(const wf::Rect& rect)
{
    std::wstringstream ss;
    ss << L"{LT: " << PointToString({rect.X, rect.Y}) << L", RB: " << PointToString({rect.X + rect.Width, rect.Y + rect.Height})
       << L", Size: [" << rect.Width << "x" << rect.Height << L"]}";

    return ss.str();
}

