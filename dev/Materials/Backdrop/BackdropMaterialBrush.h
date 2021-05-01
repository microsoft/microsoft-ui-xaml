// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BackdropMaterialBrush.g.h"
//#include "BackdropMaterialBrush.properties.h"

#include "MicaController.h"

class BackdropMaterialBrush :
    public ReferenceTracker<BackdropMaterialBrush, winrt::implementation::BackdropMaterialBrushT>
    //public BackdropMaterialBrushProperties
{
public:
    void OnConnected();
    void OnDisconnected();

    ~BackdropMaterialBrush();

private:
    void UpdateFallbackBrush();

    winrt::CompositionColorBrush m_fallbackBrush{ nullptr };
    PropertyChanged_revoker m_fallbackColorChangedRevoker;
    bool m_connected{};

    void CreateOrDestroyMicaController();

    static thread_local int m_connectedBrushCount;
    static thread_local winrt::com_ptr<MicaController> m_micaController;
};
