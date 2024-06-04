// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace RuntimeProfiler
{
    class CDynamicProfilerBase
    {
    public:
        virtual void RegisterType(_In_ Parser::StableXbfTypeIndex uTypeIndex, _Inout_ volatile LONG *pCount) = 0;
        virtual LONG * GetCounterBuffer() = 0;
    };

    CDynamicProfilerBase *GetXamlIslandTypeProfiler(_In_ CContentRoot::IslandType IslandType);
}