// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <utility>              // for std::forward
#include <vector>
#include <array>
#include <xref_ptr.h>
#include "ValueObjectBase.h"

namespace Flyweight
{
    template <typename T>
    class Factory
    {
        using value_type        = T;
        using wrapper_type      = typename Wrapper<T>::Type;
        using wrapper_ref_type  = xref_ptr<wrapper_type>;

        // How many non-default inserts between purging a bucket from unused items (e.g. ref-count = 1).
        static constexpr std::size_t c_autoPurgeInterval    = 250;

        // Used to avoid purging too frequently when bucket is on cusp of resizing.
        static constexpr std::size_t c_minPurgeInterval     = 50;

        // Hard limit on how many elements are allowed in a bucket.  Used to limit lookup times in misbehaving cases.
        // Once this limit is reached, if element is not found it will be allocated but not managed / shared.
        // This number is std::vector implementation dependent.
        static constexpr std::size_t c_bucketSizeLimit      = 271;

        static bool WillAdditionResize(const std::vector<wrapper_ref_type>& instances, std::size_t capacity)
        {
            return instances.size() + 1 >= capacity;
        }

        struct Bucket
        {
            Bucket()
            {
                m_instances.reserve(16);
            }

            void Reset()
            {
                m_instances.clear();
                m_instances.shrink_to_fit();        // Keep leak-detection happy by clearing the vector.
                m_counterSinceLastPurge = 0;
            }

            std::vector<wrapper_ref_type>   m_instances;
            unsigned int                    m_counterSinceLastPurge = 0;
        };

    public:
        // state externalized to store per-core
        class State
        {
            friend class Factory<value_type>;

            wrapper_type                        m_default;
            Bucket                              m_bucket;
            unsigned int                        m_countPassThru     = 0;

        public:
            State(const value_type& defaultValue = Operators::Default<value_type>())
                : m_default(wrapper_type(defaultValue))
            {}

            void Reset()
            {
                m_bucket.Reset();
            }
        };

        template <typename ...Types>
        static wrapper_ref_type Create(
            _In_opt_ State* state,
            _In_ Types&&... args)
        {
            if (state != nullptr)
            {
                value_type requested(std::forward<Types>(args)...);

                if (Operators::equal(requested, state->m_default.Value()))
                {
                    // short-circuit to default value
                    return wrapper_ref_type(&state->m_default);
                }
                else
                {
                    Bucket& bucket = state->m_bucket;

                    // Purge unused entries if it's time or we are possibly going to resize.
                    // Since we want to avoid searching for item (or where to insert) twice,
                    // purge needs to happen before the search.  A problem arises when we are
                    // on cusp of resizing, but item exists in the bucket.  The algorithm will
                    // try to purge every time - hence c_minPurgeInterval to prevent that.

                    if ((bucket.m_counterSinceLastPurge >= c_minPurgeInterval &&
                         WillAdditionResize(bucket.m_instances, bucket.m_instances.capacity())) ||
                        bucket.m_counterSinceLastPurge >= c_autoPurgeInterval)
                    {
                        bucket.m_instances.erase(
                            std::remove_if(
                                bucket.m_instances.begin(),
                                bucket.m_instances.end(),
                                [](const wrapper_ref_type& element) -> bool
                                {
                                    return element->GetRefCount() == 1;
                                }),
                            bucket.m_instances.end());

                        bucket.m_counterSinceLastPurge = 0;
                    }

                    // Find where this element should exist in a bucket.
                    auto iter = std::lower_bound(
                        bucket.m_instances.begin(),
                        bucket.m_instances.end(),
                        requested,
                        [](const wrapper_ref_type& lhs, const value_type& rhs) -> bool
                        {
                            return Operators::less(lhs->Value(), rhs);
                        }
                    );

                    ++bucket.m_counterSinceLastPurge;

                    if (iter == bucket.m_instances.end() ||
                        !Operators::equal(requested, iter->get()->Value()))
                    {
                        // It does not exist yet...

                        if (!WillAdditionResize(bucket.m_instances, c_bucketSizeLimit))
                        {
                            // Not reached capacity yet, so just add it.
                            iter = bucket.m_instances.emplace(
                                iter,
                                make_xref<wrapper_type>(requested));
                        }
                        else
                        {
                            ++state->m_countPassThru;
                            
                            // We reached the limit of number of elements in a bucket, so just create one and don't manage it.
                            return make_xref<wrapper_type>(requested);
                        }
                    }

                    return wrapper_ref_type(iter->get());
                }
            }
            else
            {
                // Cannot get a hold of state, so just pass-thru.
                // This code-path should only be applicable for unit-testing.
                return make_xref<wrapper_type>(std::forward<Types>(args)...);
            }
        }
    };
}