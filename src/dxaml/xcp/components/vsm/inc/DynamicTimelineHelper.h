// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CStoryboard;
enum class DynamicTimelineGenerationMode;

namespace Jupiter {
    namespace Animation {

        // Flows whether we should generate transition storyboards or steady-state
        // storyboards to the children dynamic timelines.
        void PropagateDynamicTimelineGenerationOptionToChildren(
            _In_ CStoryboard* target, DynamicTimelineGenerationMode option);

        // Attempts to generate dynamic storyboards for any children of the given storyboard that
        // are DynamicTimelines. This will fail and return false if any of the storyboards are currently
        // active.
        //
        // This method is only useful for cases where we want to create these storyboards upfront- they
        // are guaranteed to be created when the Storyboard is started. The major use case in the platform
        // at this time of this writing is inside VSM- where we have a secondary, sinister goal to
        // enumerate through the children of the dynamic timeline and build matching transitions when the
        // cooresponding transition is not present in the transition collection.
        _Check_return_ HRESULT TryGenerateDynamicChildrenStoryboardsForChildren(_In_ CStoryboard* target, _Out_opt_ bool* didSucceed);
    }
}
