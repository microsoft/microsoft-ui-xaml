// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <Windows.h>
#include <WexTestClass.h>
#include "WindowPlacementPublic.h"

using namespace DirectUI::WindowPlacementPersistence;

class WindowPlacementPublicTests
{
public:
    TEST_CLASS(WindowPlacementPublicTests);

    TEST_METHOD(ValidatesDefaultAndAllDefinedPolicies)
    {
        InitialRequest request;
        for (int reason = 0; reason != 3; ++reason)
        {
            for (int cascade = 0; cascade != 3; ++cascade)
            {
                request.Reason = reason;
                request.CascadeBehavior = cascade;
                VERIFY_IS_TRUE(IsValidInitialRequest(request, false));
                VERIFY_IS_TRUE(IsValidInitialRequest(request, true));
            }
        }
    }

    TEST_METHOD(ValidatesSkippedPoliciesRatherThanIgnoringInvalidEnums)
    {
        InitialRequest request;
        request.SkipInitialPlacement = true;
        VERIFY_IS_TRUE(IsValidInitialRequest(request, false));
        VERIFY_IS_FALSE(IsValidInitialRequest(request, true));
        request.Reason = 3;
        VERIFY_IS_FALSE(IsValidInitialRequest(request, false));
        request.Reason = 2;
        request.CascadeBehavior = -1;
        VERIFY_IS_FALSE(IsValidInitialRequest(request, false));
    }

    TEST_METHOD(ValidatesCompletePlacementAndSkipCombination)
    {
        InitialRequest request;
        request.Placement.emplace();
        request.Placement->NormalRect = {0, 0, 100, 100};
        request.Placement->WorkArea = {0, 0, 1920, 1080};
        request.Placement->Dpi = 96;
        VERIFY_IS_TRUE(IsValidInitialRequest(request, true));
        request.SkipInitialPlacement = true;
        VERIFY_IS_FALSE(IsValidInitialRequest(request, false));
        request.SkipInitialPlacement = false;
        request.Placement->PlacementState = State::Snapped;
        VERIFY_IS_FALSE(IsValidInitialRequest(request, false));
        request.Placement->SnapRect = Rect{0, 0, 500, 500};
        VERIFY_IS_TRUE(IsValidInitialRequest(request, false));
        request.Placement->Dpi = 0;
        VERIFY_IS_FALSE(IsValidInitialRequest(request, false));
    }

    TEST_METHOD(SnapshotCopiesOwnTheirEditableData)
    {
        Snapshot source;
        source.NormalRect = {0, 0, 100, 100};
        source.WorkArea = {0, 0, 1920, 1080};
        source.Dpi = 96;
        source.DisplayDeviceName = u"original";
        source.SnapRect = Rect{0, 0, 500, 500};
        Snapshot first;
        Snapshot second;
        VERIFY_IS_TRUE(TryCopySnapshot(source, first));
        VERIFY_IS_TRUE(TryCopySnapshot(source, second));
        source.DisplayDeviceName[0] = u'X';
        source.SnapRect->Width = 1;
        first.DisplayDeviceName[0] = u'Y';
        first.NormalRect.Width = 200;
        VERIFY_IS_TRUE(second.DisplayDeviceName == u"original");
        VERIFY_ARE_EQUAL(500, second.SnapRect->Width);
        VERIFY_ARE_EQUAL(100, second.NormalRect.Width);
    }

    TEST_METHOD(DeviceNameCopyPreservesLengthForDeferredValidation)
    {
        const char16_t name[] = {u'a', u'\0', u'b'};
        std::u16string copied;
        VERIFY_IS_TRUE(TryCopyDeviceName(name, 3, copied));
        VERIFY_ARE_EQUAL(static_cast<size_t>(3), copied.size());
        VERIFY_ARE_EQUAL(u'\0', copied[1]);
        VERIFY_ARE_EQUAL(u'b', copied[2]);
    }

    TEST_METHOD(DeviceNameCopyClearsFromEmptyInput)
    {
        std::u16string copied = u"existing";
        VERIFY_IS_TRUE(TryCopyDeviceName(nullptr, 0, copied));
        VERIFY_IS_TRUE(copied.empty());
        VERIFY_IS_FALSE(TryCopyDeviceName(nullptr, 1, copied));
        VERIFY_IS_TRUE(copied.empty());
    }
};
