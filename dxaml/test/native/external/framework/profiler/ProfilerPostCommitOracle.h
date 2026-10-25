// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once
#include <windows.h>
#include <cstdint>
#include <vector>
#include <stdexcept>

namespace LifecycleEtw
{
    struct PostCommitFrame
    {
        GUID Epoch{}, Root{};
        uint64_t Dispatcher = 0, Attempt = 0, Submitted = 0, Sequence = 0;
        int64_t Timestamp = 0;
        HRESULT Result = S_OK;
        uint8_t Outcome = 0;
        bool HasAttempt = false, HasSubmitted = false, HasResult = false;
        GUID AttemptEpoch{};
    };
    inline void ValidatePostCommit(const std::vector<PostCommitFrame>& frames,
        GUID epoch, GUID root, uint64_t dispatcher, int64_t begin, int64_t end, HRESULT failure)
    {
        auto require = [](bool value, const char* reason)
        {
            if (!value) throw std::runtime_error(reason);
        };
        require(epoch != GUID_NULL && root != GUID_NULL && dispatcher && begin < end && FAILED(failure),
            "qualified PostCommit oracle interval and identity");
        const PostCommitFrame* submitted = nullptr;
        const PostCommitFrame* terminated = nullptr;
        for (const auto& frame : frames)
        {
            if (frame.Timestamp < begin || frame.Timestamp > end) continue;
            if (frame.Outcome == 2)
            {
                require(!terminated, "exactly one failed outer attempt in the injected interval");
                require(frame.HasResult && FAILED(frame.Result),
                    "every outer failure termination requires a present failing HRESULT");
                terminated = &frame;
            }
            if (frame.Outcome != 1 || frame.Epoch != epoch || frame.Root != root ||
                frame.Dispatcher != dispatcher) continue;
            require(!submitted, "exactly one target primary submission before terminal failure");
            require(frame.HasAttempt && frame.Attempt && frame.AttemptEpoch == epoch &&
                frame.HasSubmitted && frame.Submitted && frame.HasResult && frame.Result == S_OK,
                "physical submission preserves qualified attempt, nonzero ID and actual Commit S_OK");
            submitted = &frame;
        }
        require(submitted && terminated, "both physical submission and independent failed outer termination are required");
        require(terminated->HasAttempt && terminated->AttemptEpoch == epoch &&
            terminated->Epoch == epoch && terminated->Root == root && terminated->Dispatcher == dispatcher &&
            terminated->Attempt == submitted->Attempt && terminated->HasResult && terminated->Result == failure &&
            !terminated->HasSubmitted && terminated->Timestamp >= submitted->Timestamp &&
            terminated->Sequence > submitted->Sequence, "failed outer attempt preserves submission identity and ordering");
        unsigned pending = 0;
        for (const auto& frame : frames)
        {
            if (frame.Timestamp < begin || frame.Timestamp > end || !frame.HasAttempt ||
                frame.Epoch != epoch || frame.Root != root || frame.Dispatcher != dispatcher ||
                frame.Attempt != submitted->Attempt) continue;
            if (frame.Outcome == 0)
            {
                require(frame.AttemptEpoch == epoch && !frame.HasSubmitted &&
                    frame.Sequence < submitted->Sequence && frame.Timestamp <= submitted->Timestamp,
                    "actual pending attempt precedes primary submission");
                ++pending;
            }
            if (frame.Outcome == 4) require(false, "failed outer attempt cannot also terminate successfully");
        }
        require(pending == 1, "one actual pending attempt precedes the submitted/failed pair");
    }
}
