// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <RuntimeEnabledFeaturesEnum.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class RuntimeEnabledFeatureOverride
        {
        public:
            RuntimeEnabledFeatureOverride() : m_isOverrideActive(false)
            {
            }

            RuntimeEnabledFeatureOverride(RuntimeFeatureBehavior::RuntimeEnabledFeature feature, bool enabled)
                : m_isOverrideActive(false)
            {
                Initialize(feature, enabled);
            }

            void Initialize(RuntimeFeatureBehavior::RuntimeEnabledFeature feature, bool enabled)
            {
                if (m_isOverrideActive)
                {
                    bool ignored;
                    test_infra::TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(m_feature), m_wasPreviouslyEnabled, &ignored);
                }

                test_infra::TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(feature), !!enabled, &m_wasPreviouslyEnabled);
                m_feature = feature;
                m_isOverrideActive = true;
            }

            ~RuntimeEnabledFeatureOverride()
            {
                if (m_isOverrideActive)
                {
                    bool ignored;
                    test_infra::TestServices::Utilities->SetRuntimeEnabledFeatureOverride(static_cast<int>(m_feature), m_wasPreviouslyEnabled, &ignored);
                }
            }

        private:
            RuntimeFeatureBehavior::RuntimeEnabledFeature m_feature;
            bool m_wasPreviouslyEnabled;
            bool m_isOverrideActive;
        };

    }
} } } }
