// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Helper class to detour registry functions and return values to DManip which turn
// off certain DManip features, allowing tests to produce stable results when injecting input.
class PredictableDManipEnabler
{
public:
    PredictableDManipEnabler();
    ~PredictableDManipEnabler();

private:
    HRESULT DisableInputPredictionLatency();
    HRESULT RestoreInputPredictionLatency();

private:
    TOUCHPREDICTIONPARAMETERS m_savedTouchPredictionParameters;
    BOOL m_restoreTouchPredictionParameters;
};

