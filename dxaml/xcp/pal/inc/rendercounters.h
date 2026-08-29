// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Provides shared memory for counters.
//      This file is shared between rencount.exe and agwin.dll

#ifndef RENDER_COUNTERS_H
#define RENDER_COUNTERS_H

struct RenderCounters
{
    XUINT32 uGenerateEdges;
};

#endif