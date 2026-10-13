// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <type_traits>

// is_tracker_target<T>
//
// Single, codegen-owned authority describing which peer types must be stored
// through the reference tracker (TrackerPtr) so that cross-boundary references
// are visible to the GC walk. This mirrors OM.TypeDefinition.IsTrackerTarget:
// every XAML peer type gets a std::true_type specialization emitted into the
// generated TrackerTargetTraits.g.h; everything else defaults to false below.
//
// This trait is the source of truth consumed by the PeerComPtrFieldCheck
// clang-tidy lint, which errors on a raw ComPtr<T> field when
// is_tracker_target<T>::value is true (storing such a peer without tracking is
// invisible to the GC and is exactly how dangling / leaked peers are created).
template<class T>
struct is_tracker_target : std::false_type {};

template<class T>
inline constexpr bool is_tracker_target_v = is_tracker_target<T>::value;
