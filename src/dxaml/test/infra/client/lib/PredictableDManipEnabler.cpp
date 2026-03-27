// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PredictableDManipEnabler.h"
#include "Utilities.h"

using namespace WEX::Common;

PredictableDManipEnabler::PredictableDManipEnabler()
    : m_restoreTouchPredictionParameters(FALSE)
{
    ZeroMemory(&m_savedTouchPredictionParameters, sizeof(TOUCHPREDICTIONPARAMETERS));

    // Unfortunately, disabling prediction latency relies on making a call to SystemParametersInfo
    // which is only implemented on desktop.  Hence we are not able to do this on all platforms.
    BOOLEAN isDesktop = FALSE;
    LogThrow_IfFailed(Private::Infrastructure::Utilities::IsDesktopStatic(&isDesktop));
    if (isDesktop)
    {
        LogThrow_IfFailed(DisableInputPredictionLatency());
    }
}

PredictableDManipEnabler::~PredictableDManipEnabler()
{
    HRESULT hr = RestoreInputPredictionLatency();
    if (FAILED(hr))
    {
        LOG_WARNING(L"Restoring DManip input prediction latency failed!");
    }
}

HRESULT PredictableDManipEnabler::DisableInputPredictionLatency()
{
    COM_START
    {
        TOUCHPREDICTIONPARAMETERS predParams = { 0 };
        ZeroMemory(&predParams, sizeof(predParams));
        predParams.cbSize = sizeof(predParams);

        Throw::IfFalse(!!::SystemParametersInfo(SPI_GETTOUCHPREDICTIONPARAMETERS, sizeof(predParams), &predParams, 0),
            E_FAIL, L"SystemParametersInfo call failed.");

        m_savedTouchPredictionParameters = predParams;

        predParams.dwLatency = 0;
        Throw::IfFalse(!!::SystemParametersInfo(SPI_SETTOUCHPREDICTIONPARAMETERS, sizeof(predParams), &predParams, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE),
            E_FAIL, L"SystemParametersInfo call failed.");

        m_restoreTouchPredictionParameters = TRUE;
        LOG_OUTPUT(L"Disabled DManip Input Prediction Latency");
    }
    COM_END
}

HRESULT PredictableDManipEnabler::RestoreInputPredictionLatency()
{
    COM_START
    {
        if (m_restoreTouchPredictionParameters)
        {
            Throw::IfFalse(!!::SystemParametersInfo(SPI_SETTOUCHPREDICTIONPARAMETERS, sizeof(m_savedTouchPredictionParameters), &m_savedTouchPredictionParameters, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE),
                E_FAIL, L"SystemParametersInfo call failed.");
            LOG_OUTPUT(L"Restored DManip Input Prediction Latency");
            m_restoreTouchPredictionParameters = FALSE;
        }
    }
    COM_END
}
