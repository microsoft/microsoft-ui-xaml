// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BitVectorUnitTests.h"
#include <XamlLogging.h>

#include <bit_vector.h>
#include <array>
#include <vector>
#include <random>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace BitVector {

    void VerifyPreallocated(int count)
    {
        LOG_OUTPUT(L"VerifyPreallocated, count = %d", count);

        containers::bit_vector v(count);

        VERIFY_ARE_EQUAL(static_cast<size_t>(count), v.size());
        VERIFY_ARE_EQUAL(false, v.empty());
        VERIFY_ARE_EQUAL(true, v.all_false());

        for (int i = 0; i < count; ++i)
        {
            if (v[i] != false)
            {
                VERIFY_ARE_EQUAL(false, v[i]);
            }
        }

        VERIFY_ARE_EQUAL(static_cast<size_t>(count), v.size());
        VERIFY_ARE_EQUAL(false, v.empty());
        VERIFY_ARE_EQUAL(true, v.all_false());

        for (int i = 0; i < count; i += 3)
        {
            v.set(i, true);
        }

        VERIFY_ARE_EQUAL(static_cast<size_t>(count), v.size());
        VERIFY_ARE_EQUAL(false, v.empty());
        VERIFY_ARE_EQUAL(false, v.all_false());

        for (int i = 0; i < count; ++i)
        {
            bool expect = (i % 3) == 0;

            if (expect != v[i])
            {
                VERIFY_ARE_EQUAL(expect, v[i]);
            }
        }
    }

    void VerifyCopyCtor(int count)
    {
        LOG_OUTPUT(L"VerifyCopyCtor, count = %d", count);

        containers::bit_vector v(count);

        for (int i = 0; i < count; i += 2)
        {
            v.set(i, true);
        }

        containers::bit_vector v2(v);

        for (int i = 0; i < count; i += 2)
        {
            v.set(i, false);
        }

        for (int i = 0; i < count; ++i)
        {
            bool expect = (i % 2) == 0;

            if (expect != v2[i])
            {
                VERIFY_ARE_EQUAL(expect, v2[i]);
            }
        }
    }

    void VerifyMoveCtor(int count)
    {
        LOG_OUTPUT(L"VerifyMoveCtor, count = %d", count);

        containers::bit_vector v(count);

        for (int i = 0; i < count; i += 2)
        {
            v.set(i, true);
        }

        containers::bit_vector v2(std::move(v));

        VERIFY_ARE_EQUAL(true, v.empty());

        for (int i = 0; i < count; ++i)
        {
            bool expect = (i % 2) == 0;

            if (expect != v2[i])
            {
                VERIFY_ARE_EQUAL(expect, v2[i]);
            }
        }
    }

    void VerifyMoveAssignment(int count)
    {
        LOG_OUTPUT(L"VerifyMoveAssignment, count = %d", count);

        containers::bit_vector v(count);

        for (int i = 0; i < count; i += 2)
        {
            v.set(i, true);
        }

        containers::bit_vector v2;
        VERIFY_ARE_EQUAL(true, v2.empty());

        v2 = std::move(v);

        VERIFY_ARE_EQUAL(true, v.empty());
        VERIFY_ARE_EQUAL(false, v2.empty());

        for (int i = 0; i < count; ++i)
        {
            bool expect = (i % 2) == 0;

            if (expect != v2[i])
            {
                VERIFY_ARE_EQUAL(expect, v2[i]);
            }
        }
    }

    void CompareToReference(const std::vector<bool>& reference, const containers::bit_vector& v)
    {
        if (reference.size() != v.size())
        {
            VERIFY_ARE_EQUAL(reference.size(), v.size());
        }

        size_t count = reference.size();

        for (size_t i = 0; i < count; ++i)
        {
            if (reference[i] != v[i])
            {
                VERIFY_ARE_EQUAL(reference[i], v[i]);
            }
        }
    }

    void VerifyInserts(int count)
    {
        LOG_OUTPUT(L"VerifyInserts, count = %d", count);

        std::default_random_engine generator(count);    // so predictable...
        std::uniform_int_distribution<int> distribution(0,1);
        std::vector<bool> reference;

        for (int i = 0; i < count; ++i)
        {
            reference.push_back(distribution(generator) == 1);
        }

        containers::bit_vector v;

        for (auto i : reference)
        {
            v.push_back(i);
        }

        CompareToReference(reference, v);
    }

    void VerifyRandomOperationSequence(int count)
    {
        LOG_OUTPUT(L"VerifyRandomOperationSequence, count = %d", count);

        std::default_random_engine generator(count);    // so predictable...
        std::uniform_int_distribution<int> distribution(0, 4);
        std::uniform_int_distribution<int> bool_distribution(0, 1);
        std::vector<bool> reference;
        containers::bit_vector v;

        for (int i = 0; i < count; ++i)
        {
            switch (distribution(generator))
            {
                case 0:
                case 1:
                    {
                        bool value = bool_distribution(generator) == 1;
                        reference.push_back(value);
                        v.push_back(value);

                        value = bool_distribution(generator) == 1;
                        reference.push_back(value);
                        v.push_back(value);
                    }
                    break;

                case 2:
                    if (!reference.empty())
                    {
                        reference.pop_back();
                        v.pop_back();
                    }
                    break;

                case 3:
                    {
                        containers::bit_vector temp;
                        temp = v;
                        v = temp;
                    }
                    break;

                case 4:
                    if (!reference.empty())
                    {
                        int index = std::uniform_int_distribution<int>(0, static_cast<int>(reference.size()) - 1)(generator);
                        reference.erase(reference.begin() + index);
                        v.erase(index);
                    }
                    break;

                default:
                    VERIFY_IS_TRUE(false);
                    break;
            }

            CompareToReference(reference, v);
        }
    }

    void VerifyAllFalsePush(int maxCount)
    {
        LOG_OUTPUT(L"VerifyAllFalsePush, count = %d", maxCount);

        containers::bit_vector v;

        for (int i = 0; i < maxCount; ++i)
        {
            VERIFY_ARE_EQUAL(true, v.all_false());
            v.push_back(true);
            VERIFY_ARE_EQUAL(false, v.all_false());

            for (int j = 0; j < i; ++j)
            {
                if (v[j] != false)
                {
                    VERIFY_ARE_EQUAL(false, v[j]);
                }
            }

            VERIFY_ARE_EQUAL(true, v[i]);
            v.set(i, false);
        }
    }

    void VerifyAllFalsePop(int maxCount)
    {
        LOG_OUTPUT(L"VerifyAllFalsePop, count = %d", maxCount);

        containers::bit_vector v(maxCount);

        for (int i = maxCount - 1; i >= 0; --i)
        {
            VERIFY_ARE_EQUAL(true, v.all_false());
            v.set(i, true);
            VERIFY_ARE_EQUAL(false, v.all_false());
            v.pop_back();
            VERIFY_ARE_EQUAL(true, v.all_false());

            for (int j = 0; j < i; ++j)
            {
                if (v[j] != false)
                {
                    VERIFY_ARE_EQUAL(false, v[j]);
                }
            }
        }
    }

    void EraseAndVerify(containers::bit_vector& v, unsigned index)
    {
        v.erase(index);

        for (unsigned i = 0; i < v.size(); ++i)
        {
            bool good = false;

            if (i == 0 || i == v.size() - 2)
            {
                good = v[i] == true;
            }
            else
            {
                good = v[i] == false;
            }

            if (!good)
            {
                VERIFY_IS_TRUE(good);
                LOG_OUTPUT(L"Failed at index = %u", i);
            }
        }
    }

    void VerifyEraseBit(int maxCount)
    {
        LOG_OUTPUT(L"VerifyEraseBit, count = %d", maxCount);

        containers::bit_vector v(maxCount);

        v.set(0, true);
        v.set(maxCount - 2, true);

        for (int i = 0; i < maxCount - 2; ++i)
        {
            EraseAndVerify(v, 1);
        }

        EraseAndVerify(v, 1);
        VERIFY_IS_TRUE(v[0]);

        EraseAndVerify(v, 0);
        VERIFY_IS_TRUE(v.empty());
    }

    void BitVectorUnitTests::Basic()
    {
        // 15    - 32-bit - internal, 64-bit - internal
        // 26/27 - 32-bit - internal (cusp), 64-bit - internal
        // 57/58 - 32-bit - external, 64-bit - internal (cusp)
        // 64    - 32-bit - external, 64-bit - external
        // ... reallocated multiple times

        std::array<int, 7> important_sizes = { 15, 26, 27, 57, 58, 64, 1324 };

        {
            containers::bit_vector v;
            VERIFY_ARE_EQUAL(static_cast<size_t>(0), v.size());
            VERIFY_ARE_EQUAL(true, v.empty());
            VERIFY_ARE_EQUAL(true, v.all_false());
        }

        for (auto size : important_sizes)
        {
            VerifyPreallocated(size);
        }

        for (auto size : important_sizes)
        {
            VerifyCopyCtor(size);
        }

        for (auto size : important_sizes)
        {
            VerifyMoveCtor(size);
        }

        for (auto size : important_sizes)
        {
            VerifyMoveAssignment(size);
        }

        for (auto size : important_sizes)
        {
            VerifyInserts(size);
        }

        for (auto size : important_sizes)
        {
            VerifyEraseBit(size);
        }

        VerifyRandomOperationSequence(5000);
        VerifyRandomOperationSequence(5001);
        VerifyRandomOperationSequence(5002);

        VerifyAllFalsePush(512);
        VerifyAllFalsePop(512);
    }
} } } } }