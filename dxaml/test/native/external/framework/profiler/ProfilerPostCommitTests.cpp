// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
// Bounded state/oracle tests only. Constructed records are NOT actual ETW qualification.
#include <windows.h>
#include <cstdio>
#include <functional>
#include "ProfilerPostCommitTest.h"
#include "ProfilerPostCommitOracle.h"

namespace
{
    void Require(bool value, const char* reason)
    {
        if (!value) throw std::runtime_error(reason);
    }
    void Reject(const std::function<void()>& action)
    {
        bool rejected = false;
        try { action(); } catch (const std::runtime_error&) { rejected = true; }
        Require(rejected, "oracle accepted a missing/corrupted failed outer termination");
    }
    const GUID Epoch{1, 2, 3, {4}}, Root{5, 6, 7, {8}};
    using Record = LifecycleEtw::PostCommitFrame;
    std::vector<Record> Valid()
    {
        std::vector<Record> frames{
            {Epoch, Root, 9, 10, 0, 20, 100, S_OK, 0, true, false, false},
            {Epoch, Root, 9, 10, 11, 21, 110, S_OK, 1, true, true, true},
            {Epoch, Root, 9, 10, 0, 22, 120, WinUIProfiler::FrameTest::PostCommitFailure, 2, true, false, true}};
        for (auto& frame : frames) frame.AttemptEpoch = Epoch;
        return frames;
    }
    void Validate(const std::vector<Record>& frames)
    {
        LifecycleEtw::ValidatePostCommit(frames, Epoch, Root, 9, 90, 130,
            WinUIProfiler::FrameTest::PostCommitFailure);
    }
}
int main()
{
    unsigned passed = 0, failed = 0;
    auto test = [&](const char* name, const std::function<void()>& body)
    {
        try { body(); ++passed; std::printf("PASS: %s\n", name); }
        catch (const std::exception& error) { ++failed; std::printf("FAIL: %s: %s\n", name, error.what()); }
    };
    test("current-core one-shot state and cleanup precedence", []
    {
        using namespace WinUIProfiler::FrameTest;
        PostCommitState armed, control;
        Require(armed.State() == 0 && control.State() == 0, "new cores must start unarmed");
        Require(armed.Arm() == S_OK && armed.State() == 1, "arm current core");
        Require(FAILED(armed.Arm()), "duplicate arm must be refused");
        Require(control.AfterCleanup(S_OK) == S_OK && control.State() == 0, "unarmed control is isolated");
        Require(armed.AfterCleanup(E_INVALIDARG) == E_INVALIDARG && armed.State() == 1,
            "real cleanup failure wins and must not consume the injection");
        Require(armed.AfterCleanup(S_OK) == PostCommitFailure && armed.State() == 2, "consume after real cleanup");
        Require(armed.AfterCleanup(S_OK) == S_OK && armed.State() == 2, "one-shot only");
        Require(FAILED(armed.Arm()), "consumed core cannot rearm");
    });
    test("valid submission then independently failed outer attempt", [] { Validate(Valid()); });
    test("submission alone is not success", [] { auto f = Valid(); f.pop_back(); Reject([&] { Validate(f); }); });
    test("outer failure alone is not submission", [] { auto f = Valid(); f.erase(f.begin() + 1); Reject([&] { Validate(f); }); });
    test("successful outer termination is not injected failure", []
    {
        auto f = Valid(); f.back().Outcome = 4; f.back().Result = S_OK; Reject([&] { Validate(f); });
    });
    test("strict identity, order, result and physical ID", []
    {
        for (unsigned mutation = 0; mutation != 14; ++mutation)
        {
            auto f = Valid();
            switch (mutation)
            {
            case 0: f.back().Epoch = GUID_NULL; break;
            case 1: f.back().Root = GUID_NULL; break;
            case 2: ++f.back().Dispatcher; break;
            case 3: ++f.back().Attempt; break;
            case 4: f.back().Timestamp = 105; break;
            case 5: f.back().Sequence = 20; break;
            case 6: f.back().Result = E_FAIL; break;
            case 7: f.back().HasResult = false; break;
            case 8: f[1].Submitted = 0; break;
            case 9: f[1].Result = E_FAIL; break;
            case 10: f[1].HasAttempt = false; break;
            case 11: f.back().HasSubmitted = true; break;
            case 12: f.erase(f.begin()); break;
            case 13: f.back().AttemptEpoch = GUID_NULL; break;
            }
            Reject([&] { Validate(f); });
        }
    });
    test("extra Outcome2 with missing result is rejected", []
    {
        auto f = Valid();
        auto extra = f.back();
        extra.Timestamp = 125;
        extra.Sequence = 23;
        extra.HasResult = false;
        f.push_back(extra);
        Reject([&] { Validate(f); });
    });
    test("extra Outcome2 with S_OK is rejected", []
    {
        auto f = Valid();
        auto extra = f.back();
        extra.Timestamp = 125;
        extra.Sequence = 23;
        extra.Result = S_OK;
        f.push_back(extra);
        Reject([&] { Validate(f); });
    });
    test("extra Outcome2 with malformed identity and missing result is rejected", []
    {
        auto f = Valid();
        auto extra = f.back();
        extra.Timestamp = 125;
        extra.Sequence = 23;
        extra.Epoch = GUID_NULL;
        extra.Root = GUID_NULL;
        extra.AttemptEpoch = GUID_NULL;
        extra.Dispatcher = 0;
        extra.Attempt = 0;
        extra.HasAttempt = false;
        extra.HasResult = false;
        f.push_back(extra);
        Reject([&] { Validate(f); });
    });
    test("second or duplicate terminal failure is rejected", []
    {
        auto f = Valid(); f.push_back(f.back()); Reject([&] { Validate(f); });
    });
    std::printf("BOUNDED_STATE_ORACLE: passed=%u failed=%u (not actual ETW)\n", passed, failed);
    return failed ? 1 : 0;
}
