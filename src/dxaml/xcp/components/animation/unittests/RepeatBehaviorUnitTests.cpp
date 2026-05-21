// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RepeatBehaviorUnitTests.h"
#include "RepeatBehavior.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Animation {

void RepeatBehaviorUnitTests::ValidateGetTotalDuration()
{
    double totalDuration_out;
    bool loopsForever_out;

    // Iterations
    {
        // TimeSpan is ignored if provided
        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Count, 100, 3.1f);

        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 0, &loopsForever_out);
        VERIFY_ARE_EQUAL(0, totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 2.3, &loopsForever_out);
        VERIFY_ARE_EQUAL(2.3 * static_cast<double>(3.1f), totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        // Contrived, but the method doesn't care.
        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), -10, &loopsForever_out);
        VERIFY_ARE_EQUAL(-10 * static_cast<double>(3.1f), totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        // Repeating an animation less than once will make it shorter.
        CRepeatBehavior smaller(DirectUI::RepeatBehaviorType::Count, 100, 0.3f);
        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(smaller.ValueWrapper()->Value(), 2.3, &loopsForever_out);
        VERIFY_ARE_EQUAL(2.3 * static_cast<double>(0.3f), totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);
    }

    // TimeSpan
    {
        // Iterations are ignored if provided
        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Duration, 13.24, 100);

        // It's physically impossible to loop a zero duration animation for a non-zero amount of time, but the method doesn't care.
        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 0, &loopsForever_out);
        VERIFY_ARE_EQUAL(13.24, totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 5, &loopsForever_out);
        VERIFY_ARE_EQUAL(13.24, totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 13.24, &loopsForever_out);
        VERIFY_ARE_EQUAL(13.24, totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 100, &loopsForever_out);
        VERIFY_ARE_EQUAL(13.24, totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        // An animation can be repeated for less than its natural duration.
        CRepeatBehavior smaller(DirectUI::RepeatBehaviorType::Duration, 1.4, 100);
        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(smaller.ValueWrapper()->Value(), 2.3, &loopsForever_out);
        VERIFY_ARE_EQUAL(1.4, totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);

        // Contrived, but the method doesn't care.
        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), -10, &loopsForever_out);
        VERIFY_ARE_EQUAL(13.24, totalDuration_out);
        VERIFY_ARE_EQUAL(false, loopsForever_out);
    }

    // Forever
    {
        // Iterations and TimeSpan are both ignored
        CRepeatBehavior repeat(DirectUI::RepeatBehaviorType::Forever, 100, 200);

        // It's physically impossible to loop a zero duration animation for a non-zero amount of time, but the method doesn't care.
        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 0, &loopsForever_out);
        VERIFY_ARE_EQUAL(0, totalDuration_out);
        VERIFY_ARE_EQUAL(true, loopsForever_out);

        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), 5, &loopsForever_out);
        VERIFY_ARE_EQUAL(0, totalDuration_out);
        VERIFY_ARE_EQUAL(true, loopsForever_out);

        // Contrived, but the method doesn't care.
        totalDuration_out = RepeatBehaviorVOHelper::GetTotalDuration(repeat.ValueWrapper()->Value(), -10, &loopsForever_out);
        VERIFY_ARE_EQUAL(0, totalDuration_out);
        VERIFY_ARE_EQUAL(true, loopsForever_out);
    }
}

} } } } } }
