// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Performance marker definitions

#pragma once

//------------- bits 32 - 1  --------------------------------------------------
//
//      NOTE: this is specific to the type of data
//


//
//      event marker: begin rasterization (first time)
//
const XUINT64 XCP_PMK_RASTER_BEGIN_FIRST = 0x0000000000000001LL | XCP_PMK_EVENT;

//
//      event marker: end rasterization (first time)
//
const XUINT64 XCP_PMK_RASTER_END_FIRST = 0x0000000000000002LL | XCP_PMK_EVENT;

//
//      event marker: begin rasterization (subsequent times)
//
const XUINT64 XCP_PMK_RASTER_BEGIN = 0x0000000000000004LL | XCP_PMK_EVENT;

//
//      event marker: end rasterization (subsequent times)
//
const XUINT64 XCP_PMK_RASTER_END = 0x0000000000000008LL | XCP_PMK_EVENT;

//
//      event marker: begin parsing
//
const XUINT64 XCP_PMK_PARSE_BEGIN = 0x0000000000000010LL | XCP_PMK_EVENT;

//
//      event marker: end parsing
//
const XUINT64 XCP_PMK_PARSE_END = 0x0000000000000020LL | XCP_PMK_EVENT;

//
//      event marker: begin render
//
const XUINT64 XCP_PMK_RENDER_BEGIN = 0x0000000000000040LL | XCP_PMK_EVENT;

//
//      event marker: end render
//
const XUINT64 XCP_PMK_RENDER_END = 0x0000000000000080LL | XCP_PMK_EVENT;

//
//      event marker: begin frame processing
//
const XUINT64 XCP_PMK_FRAME_BEGIN = 0x0000000000000100LL | XCP_PMK_EVENT;

//
//      event marker: end frame processing
//
const XUINT64 XCP_PMK_FRAME_END = 0x0000000000000200LL | XCP_PMK_EVENT;

//
//      event marker: script callback begin
//
const XUINT64 XCP_PMK_SCRIPTCALLBACK_BEGIN = 0x0000000000000400LL | XCP_PMK_EVENT;

//
//      event marker: script callback end
//
const XUINT64 XCP_PMK_SCRIPTCALLBACK_END = 0x0000000000000800LL | XCP_PMK_EVENT;

//
//      event marker: storyboard begin
//
const XUINT64 XCP_PMK_STORYBOARD_BEGIN = 0x0000000000001000LL | XCP_PMK_EVENT;

//
//      event marker: storyboard end
//
const XUINT64 XCP_PMK_STORYBOARD_END = 0x0000000000002000LL | XCP_PMK_EVENT;

//
//      event marker: layout begin
// 
const XUINT64 XCP_PMK_LAYOUT_BEGIN = 0x0000000000004000LL | XCP_PMK_EVENT;

//
//      event marker: layout end
//
const XUINT64 XCP_PMK_LAYOUT_END = 0x0000000000008000LL | XCP_PMK_EVENT;

//
//      event marker: CLR callback begin
//
const XUINT64 XCP_PMK_CLRCALLBACK_BEGIN = 0x0000000000010000LL | XCP_PMK_EVENT;

//
//      event marker: CLR callback end
//
const XUINT64 XCP_PMK_CLRCALLBACK_END = 0x0000000000020000LL | XCP_PMK_EVENT;

//
//      event marker: Content source load
//
const XUINT64 XCP_PMK_CONTENTSRC_LOAD = 0x0000000000040000LL | XCP_PMK_EVENT;

//
//      event marker: Content source unload
//
const XUINT64 XCP_PMK_CONTENTSRC_UNLOAD = 0x0000000000080000LL | XCP_PMK_EVENT;

//
//      event marker: Frame ignore (fired when there is no actual frame update)
//
const XUINT64 XCP_PMK_FRAME_IGNORE = 0x0000000000100000LL | XCP_PMK_EVENT;

//
//      event marker: Present begin
//
const XUINT64 XCP_PMK_PRESENT_BEGIN = 0x0000000000200000LL | XCP_PMK_EVENT;

//
//      event marker: Present end
//
const XUINT64 XCP_PMK_PRESENT_END = 0x0000000000400000LL | XCP_PMK_EVENT;
