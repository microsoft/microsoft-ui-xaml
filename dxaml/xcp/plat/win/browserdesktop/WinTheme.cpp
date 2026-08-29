// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "Windows.h"
#include "uxtheme.h"
#include "vsanimation.h"

#include "macros.h"
#include "palcore.h"

//------------------------------------------------------------------------
//
// Helper: retrieves the transform
//
//------------------------------------------------------------------------
_Check_return_ static HRESULT
XcpUxThemeRetrieveTransform(
    _In_ HTHEME hTheme,
    _In_ INT storyboardID,
    _In_ INT targetID,
    _In_ INT index,
    _Outptr_ TA_TRANSFORM **ppTransform)
{
    DWORD cbSizeTransform = 0;
    DWORD cbSizeOut = 0;
    BYTE* pBuffer = NULL;

    HRESULT hr = GetThemeAnimationTransform(hTheme, storyboardID, targetID, index, NULL, 0, &cbSizeTransform);
    IFCEXPECT(hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)); // This is the only valid response we are expecting
    hr = S_OK;  // All goes well
    pBuffer = new BYTE[cbSizeTransform];
    *ppTransform = reinterpret_cast<TA_TRANSFORM *>(pBuffer);
    // Call the function again with the size that is returned from the previous call
    IFC(GetThemeAnimationTransform(hTheme, storyboardID, targetID, index, *ppTransform, cbSizeTransform, &cbSizeOut));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// Helper: retrieves the timing function parameters
// Note: only Bezier transforms are supported in this version.
//
//------------------------------------------------------------------------
_Check_return_ static HRESULT
XcpUxThemeRetrieveTimingFunction(
    _In_ HTHEME hThemeTiming,
    _In_ DWORD dwTimingFunctionId,
    _Outptr_ TA_CUBIC_BEZIER **ppCubicBezier)
{
    DWORD cbSizeTiming = 0;
    DWORD cbSizeTimingOut = 0;
    HRESULT hr = GetThemeTimingFunction(hThemeTiming, dwTimingFunctionId, nullptr, 0, &cbSizeTiming);
    IFCEXPECT_RETURN(hr == HRESULT_FROM_WIN32(ERROR_MORE_DATA)); // This is the only valid response we are expecting
    std::unique_ptr<BYTE[]> pBufferTiming(new BYTE[cbSizeTiming]);
    TA_TIMINGFUNCTION *pTimingFunction = reinterpret_cast<TA_TIMINGFUNCTION *>(pBufferTiming.get());
    // Retrieve the timing function for the transform
    IFC_RETURN(GetThemeTimingFunction(hThemeTiming, dwTimingFunctionId, pTimingFunction, cbSizeTiming, &cbSizeTimingOut));
    switch (pTimingFunction->eTimingFunctionType)
    {
    case TTFT_CUBIC_BEZIER:
        {
            *ppCubicBezier = reinterpret_cast<TA_CUBIC_BEZIER*>(pTimingFunction);
            pBufferTiming.release();
        }
        break;

    default:
        {
            IFC_RETURN(E_NOTIMPL);
        }
        break;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
// Helper: retrieves the set of opacity transforms for given animation type
// Note: ignores all transforms except opacity
//
//------------------------------------------------------------------------
_Check_return_ static HRESULT
XcpUxThemeGetOpacityPvlAnimation(
    _In_ INT storyboardID,
    _In_ INT targetID,
    _Out_ XINT32 *pCount,
    _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms)
{
    HRESULT hr = S_OK;
    TA_TRANSFORM* pTransform = NULL;
    TA_CUBIC_BEZIER *pCubicBezier = NULL;
    HTHEME hThemeAnimation = NULL;
    HTHEME hThemeTiming = NULL;
    ThemingData::OpacitySplineTransform *pTransforms = NULL;
    DWORD dwTransforms = 0;
    DWORD cbSizeOut = 0;
    DWORD dwOutTransformIndex = 0;

    hThemeAnimation = OpenThemeData(NULL, L"Animations");
    IFCEXPECT(hThemeAnimation != NULL);
    hThemeTiming = OpenThemeData(NULL, L"timingfunction");
    IFCEXPECT(hThemeTiming != NULL);

    IFC(GetThemeAnimationProperty(hThemeAnimation, storyboardID, targetID, TAP_TRANSFORMCOUNT, &dwTransforms, sizeof(dwTransforms), &cbSizeOut));
    IFCEXPECT(dwTransforms > 0);

    pTransforms = new ThemingData::OpacitySplineTransform[dwTransforms];
    for (DWORD index = 0; index < dwTransforms; ++index)
    {
        IFC(XcpUxThemeRetrieveTransform(hThemeAnimation, storyboardID, targetID, index, &pTransform));
        IFC(XcpUxThemeRetrieveTimingFunction(hThemeTiming, pTransform->dwTimingFunctionId, &pCubicBezier));
        switch (pTransform->eTransformType)
        {
        case TATT_OPACITY:
            {
                TA_TRANSFORM_OPACITY* pOpacityTransform = reinterpret_cast<TA_TRANSFORM_OPACITY*>(pTransform);
                pTransforms[dwOutTransformIndex].startTime = XFLOAT(pTransform->dwStartTime) / 1000.0f;
                pTransforms[dwOutTransformIndex].durationTime = XFLOAT(pTransform->dwDurationTime) / 1000.0f;
                pTransforms[dwOutTransformIndex].hasInitialValue = ((pTransform->eFlags & TATF_HASINITIALVALUES) != 0);
                pTransforms[dwOutTransformIndex].startValue = pOpacityTransform->rInitialOpacity;
                pTransforms[dwOutTransformIndex].endValue = pOpacityTransform->rOpacity;
                pTransforms[dwOutTransformIndex].p1 = pCubicBezier->rX0;
                pTransforms[dwOutTransformIndex].p2 = pCubicBezier->rX1;
                ++dwOutTransformIndex;
            }
            break;

        case TATT_TRANSLATE_2D:
        case TATT_SCALE_2D:
        case TATT_CLIP:
        default:
            break;
        }

        delete[] reinterpret_cast<BYTE*>(pCubicBezier);
        pCubicBezier = NULL;
        delete[] reinterpret_cast<BYTE*>(pTransform);
        pTransform = NULL;
    }

    *ppTransforms = pTransforms;
    pTransforms = NULL;
    *pCount = dwOutTransformIndex;

Cleanup:
    CloseThemeData(hThemeAnimation);
    CloseThemeData(hThemeTiming);
    delete[] reinterpret_cast<BYTE*>(pCubicBezier);
    delete[] reinterpret_cast<BYTE*>(pTransform);
    delete[] pTransforms;
    return hr;
}

//------------------------------------------------------------------------
//
// Retrieves the set of transforms for TA_FADEIN
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsServices::GetFadeInThemeAnimationData(
    _Out_ XINT32 *pCount,
    _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms)
{
    return XcpUxThemeGetOpacityPvlAnimation(TAS_FADEIN, TA_FADEIN_SHOWN, pCount, ppTransforms);
}

//------------------------------------------------------------------------
//
// Retrieves the set of transforms for TA_FADEOUT
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowsServices::GetFadeOutThemeAnimationData(
    _Out_ XINT32 *pCount,
    _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms)
{
    return XcpUxThemeGetOpacityPvlAnimation(TAS_FADEOUT, TA_FADEOUT_HIDDEN, pCount, ppTransforms);
}
