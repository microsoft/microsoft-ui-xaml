// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      This customization ensures that the location bounds is initialized
//      with the right value

#pragma once

#include "SemanticZoomLocation.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(SemanticZoomLocation)
    {
    protected:

        SemanticZoomLocation()
        {
        }
        
        _Check_return_ HRESULT Initialize() override
        {
            HRESULT hr = S_OK;
            wf::Rect defaultValue = {0, 0, -1, -1};
            wf::Point defaultPoint = {0, 0};
            wf::Rect defaultRemainder = {0, 0, 0, 0};
            
            IFC(SemanticZoomLocationGenerated::Initialize());
            
            IFC(put_ZoomPoint(defaultPoint));
            IFC(put_Bounds(defaultValue));
            IFC(put_Remainder(defaultRemainder));
            
            // Used on phone when displaying the jumplist
            m_bIsBottomAlignment = TRUE;
        Cleanup:   
            RRETURN(hr);
        }

    public:
        // Used on phone when displaying the jumplist
        void SetIsBottomAlignment(BOOLEAN bottomAlignment)
        {
            m_bIsBottomAlignment = bottomAlignment;
        }
        BOOLEAN GetIsBottomAlignment() const
        {
            return m_bIsBottomAlignment;
        }

    private:
        BOOLEAN m_bIsBottomAlignment;
    };
}
