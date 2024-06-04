// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct IDirectManipulationCompositor;

// Represents the state shared between CDirectManipulationService instances within the same UI thread
class DirectManipulationServiceSharedState
{
public:
    DirectManipulationServiceSharedState();
    ~DirectManipulationServiceSharedState();

    HRESULT GetSharedDCompManipulationCompositor(IDirectManipulationCompositor **ppResult);
    void ReleaseSharedDCompManipulationCompositor(IDirectManipulationCompositor *&compositor);
    void ResetSharedDCompManipulationCompositor();

private:
    Microsoft::WRL::ComPtr<IDirectManipulationCompositor> m_compositor;
    unsigned int m_compositorUseCount;
};
