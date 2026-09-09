//
// pch.h
// Header for standard system include files.
//

#pragma once

// Avoid adding SDK headers here, to test that generated code
// includes all its dependencies.

// windows.h (pulled in indirectly) defines GetCurrentTime as a macro, which collides with
// the Microsoft.UI.Xaml.Media.Animation.Timeline.GetCurrentTime projection.
#include <unknwn.h>
#undef GetCurrentTime
