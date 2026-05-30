// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Style.h"
#include "MetadataAPI.h"
#include <OptimizedStyle.h>
#include <StyleCustomRuntimeData.h>
#include <ICustomWriterRuntimeDataReceiver.h>
#include <CustomWriterRuntimeContext.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <DXamlServices.h>
#include "MUX-ETWEvents.h"
#include "collection\inc\SetterBaseCollection.h"
#include "metadata\inc\TypeTableStructs.h"
#include "DOPointerCast.h"
#include "Setter.h"
#include <XcpErrorResource.h>
#include <Xcp_error.h>
#include "diagnosticsInterop\inc\ResourceGraph.h"

using namespace RuntimeFeatureBehavior;

// Override for virtual to release reference on the object.
// The code in this override breaks cycles due to BasedOn circular references to avoid memory leaks.
void CStyle::ReleaseOverride()
{
    // BasedOn circular references will lead to memory leak as the native style objects
    // will keep holding onto each other forever (GC takes care of cycles on the managed side.)
    // This is what we do to break it - when the ref count goes from 2 to 1, we walk the circular
    // reference cycle. If all the style objects on the reference circle have a ref count of 1, then
    // the only reference keeping them alive is the BasedOn reference. Release the BasedOn reference
    // from this style and that will break the tie.
    // Also, do this when the current style is part of the cycle, determined by m_cBasedOnCircularRefCount.
    // It is possible that there is a cycle in the basedon hierarchy but the current style
    // is not part of that cycle, in which case, we don't care about breaking that cycle yet.

    if(GetRefCount() == 2 && m_pBasedOn && m_cBasedOnCircularRefCount > 1)
    {
        CStyle* pNext = m_pBasedOn;
        while(pNext && pNext != this && pNext->GetRefCount() == 1)
        {
            pNext = pNext->m_pBasedOn;
        }
        if(pNext == this)
        {
            // All styles in based on circle have ref count 1. break the circle to release objects.
            // Releasing BasedOn reference will cause re-entrancy in this method.
            // Setting m_pBasedOn to NULL to avoid that.
            CStyle* pStyle = m_pBasedOn;
            m_pBasedOn = nullptr;
            ReleaseInterface(pStyle);
        }
    }
}

void CStyle::RegisterSetterCollection()
{
    if (m_pSetters)
    {
        const auto resourceGraph = Diagnostics::GetResourceGraph();
        resourceGraph->RegisterStyleAndSetterCollection(this, m_pSetters);
    }
}