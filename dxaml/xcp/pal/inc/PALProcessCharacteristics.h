// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//-----------------------------------------------------------------------------
//  Abstract:
//      Defines IPALProcessCharacteristics, a PAL interface for obtaining
//      information about a process.
//
// Exposes information about a process through the PAL.
//
// Currently IPlatformServices derives from IPALProcessCharacteristics, so
// the PAL describes the current process. If a scenario arises where we
// need to examine other processes, the PAL could hand out an instance of
// IPALProcessCharacteristics to describe a different process.
//-----------------------------------------------------------------------------

struct IPALProcessCharacteristics
{
    virtual bool IsProcessPackaged() = 0;
};


