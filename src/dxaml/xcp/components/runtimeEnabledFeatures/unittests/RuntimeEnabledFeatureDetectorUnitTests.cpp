// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RuntimeEnabledFeatureDetectorUnitTests.h"

#include <RuntimeEnabledFeatures.h>
#include <RuntimeEnabledFeaturesEnum.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Framework {

        using namespace RuntimeFeatureBehavior;

        static const wchar_t* rootXamlKey = XAML_ROOT_KEY;

        void RuntimeEnabledFeatureDetectorUnitTests::ValidateDefaultValues()
        {
            RuntimeEnabledFeatureDetector detector;

            VERIFY_IS_FALSE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));
            VERIFY_IS_TRUE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestEnabledByDefaultFeature));
        }

        void RuntimeEnabledFeatureDetectorUnitTests::ValidateOverride()
        {
            RuntimeEnabledFeatureDetector detector;

            detector.SetFeatureOverride(RuntimeEnabledFeature::TestDisabledByDefaultFeature, true);
            VERIFY_IS_TRUE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));
            VERIFY_IS_TRUE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestEnabledByDefaultFeature));

            detector.ClearFeatureOverride(RuntimeEnabledFeature::TestDisabledByDefaultFeature);
            VERIFY_IS_FALSE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));
            VERIFY_IS_TRUE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestEnabledByDefaultFeature));

            detector.SetFeatureOverride(RuntimeEnabledFeature::TestEnabledByDefaultFeature, false);
            VERIFY_IS_FALSE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));
            VERIFY_IS_FALSE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));

            detector.ClearFeatureOverride(RuntimeEnabledFeature::TestEnabledByDefaultFeature);
            VERIFY_IS_FALSE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));
            VERIFY_IS_TRUE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestEnabledByDefaultFeature));

            detector.SetFeatureOverride(RuntimeEnabledFeature::TestEnabledByDefaultFeature, false);
            detector.SetFeatureOverride(RuntimeEnabledFeature::TestDisabledByDefaultFeature, true);
            VERIFY_IS_TRUE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));
            VERIFY_IS_FALSE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestEnabledByDefaultFeature));

            detector.ClearAllFeatureOverrides();
            VERIFY_IS_FALSE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestDisabledByDefaultFeature));
            VERIFY_IS_TRUE(detector.IsFeatureEnabled(RuntimeEnabledFeature::TestEnabledByDefaultFeature));
        }

    }
} } } }
