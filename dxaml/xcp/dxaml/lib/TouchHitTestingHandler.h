// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class TouchHitTestingHandler
    {
    public:
        TouchHitTestingHandler();

        static bool IsTouchHitTestingEnabled(_In_ CCoreServices* core);

        _Check_return_ HRESULT Register(_In_ wuc::ICoreWindow* pCoreWindow);
        _Check_return_ HRESULT Unregister(_In_ wuc::ICoreWindow* pCoreWindow);

        _Check_return_ HRESULT Register(_In_ HWND hwnd);

        static _Check_return_ HRESULT TouchHitTesting(
            _In_opt_ CUIElement* subtreeRootElement,
            _In_ mui::ITouchHitTestingEventArgs* pArgs);

    private:
        using EvaluateProximityToRectFunction = std::function<HRESULT(wf::Rect, mui::ProximityEvaluation*)>;
        using EvaluateProximityToPolygonFunction = std::function<HRESULT(UINT32, wf::Point*, mui::ProximityEvaluation*)>;
        // IFACEMETHOD(EvaluateProximityToRect)(_In_ wf::Rect rect,
        //                     _Out_ wuc::CoreProximityEvaluation *pProximityEval);
        // IFACEMETHOD(EvaluateProximityToPolygon)(_In_ UINT32 numberVertices,
        //                     _In_reads_(numberVertices) wf::Point *vertices,
        //                     _Out_ wuc::CoreProximityEvaluation *pProximityEval);

        struct TouchHitTestingInputs
        {
            CUIElement* subtreeRootElement = nullptr;
            wf::Rect boundingBox = {};
            mui::ProximityEvaluation proximityEvaluation = {};
            wf::Point point = {};
            EvaluateProximityToRectFunction evaluateProximityToRect;
            EvaluateProximityToPolygonFunction evaluateProximityToPolygon;
        };

        struct TouchHitTestingOutputs
        {
            mui::ProximityEvaluation proximityEvaluation = {};
            bool handled = false;
        };

        static _Check_return_ HRESULT TouchHitTestingInternal(
            _In_ const TouchHitTestingInputs& inputs,
            _Out_ TouchHitTestingOutputs& outputs);

        _Check_return_ HRESULT OnCoreWindowTouchHitTesting(_In_ wuc::ICoreWindow* pSender, _In_ wuc::ITouchHitTestingEventArgs* pArgs);

        _Check_return_ static HRESULT UpdateBestEval(
            _In_ const mui::ProximityEvaluation& currentEval,
            _Inout_ mui::ProximityEvaluation& bestEval,
            _In_ const wf::Rect& currentBounds,
            _Inout_ wf::Rect& bestBounds,
            _Inout_ bool& elementEvaluated,
            _In_ CUIElement* pCurrentElement,
            _Inout_ CUIElement** ppBestEvalElement);

        static _Check_return_ HRESULT GetPointList(_In_ wfc::IIterable<wf::Point>* pIterable, _Inout_ std::vector<wf::Point>& list);

        static void GetPolygonBounds(XUINT32 numPoints, _In_reads_(numPoints) wf::Point* pPoints, _Out_ wf::Rect* pBounds);

        EventRegistrationToken m_tokTouchHitTesting;
    };
}
