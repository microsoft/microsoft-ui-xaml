// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XVectorUnitTests.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { 
    namespace Xaml {            

    void XVectorUnitTests::ValidateAdditionIteratorOperator()
    {
        auto& testVector = BuildTestVector();

        auto iter = testVector.begin();
        VERIFY_ARE_EQUAL(*iter, 0);

        auto addedIter = iter + 3;
        VERIFY_ARE_EQUAL(*iter, 0);
        VERIFY_ARE_EQUAL(*addedIter, 3);
    }

    void XVectorUnitTests::ValidateSubtractionIteratorOperatorWithIterator()
    {
        auto& testVector = BuildTestVector();

        auto iter = testVector.begin();
        VERIFY_ARE_EQUAL(*iter, 0);

        auto added3Iter = iter + 3;
        auto added1Iter = iter + 1;

        auto subtractedDifferenceValue = added3Iter - added1Iter;

        VERIFY_ARE_EQUAL(subtractedDifferenceValue, 2);
        VERIFY_ARE_EQUAL(*added3Iter, 3);
        VERIFY_ARE_EQUAL(*added1Iter, 1);
    }

    void XVectorUnitTests::ValidateSubtractionIteratorOperatorWithOffset()
    {
        auto& testVector = BuildTestVector();

        auto iter = testVector.begin();
        VERIFY_ARE_EQUAL(*iter, 0);

        auto added3Iter = iter + 3;
        auto added1Iter = iter + 1;

        auto subtractedIter = added3Iter - 2;
        VERIFY_ARE_EQUAL(*subtractedIter, 1);
        VERIFY_ARE_EQUAL(*added3Iter, 3);
        VERIFY_ARE_EQUAL(*added1Iter, 1);
    }

    void XVectorUnitTests::ValidateConstAdditionIteratorOperator()
    {
        const auto& testVector = BuildTestVector();

        auto iter = testVector.begin();
        VERIFY_ARE_EQUAL(*iter, 0);

        auto addedIter = iter + 3;
        VERIFY_ARE_EQUAL(*iter, 0);
        VERIFY_ARE_EQUAL(*addedIter, 3);
    }

    void XVectorUnitTests::ValidateConstSubtractionIteratorOperatorWithIterator()
    {
        const auto& testVector = BuildTestVector();

        auto iter = testVector.begin();
        VERIFY_ARE_EQUAL(*iter, 0);

        auto added3Iter = iter + 3;
        auto added1Iter = iter + 1;

        auto subtractedDifferenceValue = added3Iter - added1Iter;

        VERIFY_ARE_EQUAL(subtractedDifferenceValue, 2);
        VERIFY_ARE_EQUAL(*added3Iter, 3);
        VERIFY_ARE_EQUAL(*added1Iter, 1);
    }

    void XVectorUnitTests::ValidateConstSubtractionIteratorOperatorWithOffset()
    {
        const auto& testVector = BuildTestVector();

        auto iter = testVector.begin();
        VERIFY_ARE_EQUAL(*iter, 0);

        auto addedIter = iter + 3;
        VERIFY_ARE_EQUAL(*iter, 0);
        VERIFY_ARE_EQUAL(*addedIter, 3);

        auto subtractedIter = addedIter - 1;
        VERIFY_ARE_EQUAL(*subtractedIter, 2);
    }

    void XVectorUnitTests::ValidateStdDistance()
    {
        const auto& testVector = BuildTestVector();

        auto iter = testVector.begin();
        VERIFY_ARE_EQUAL(*iter, 0);

        auto addedIter = iter + 3;
        VERIFY_ARE_EQUAL(std::distance(testVector.begin(), addedIter), 3);
    }

    xvector<int>& XVectorUnitTests::BuildTestVector()
    {
        static xvector<int> testVector;
        testVector.clear();
        testVector.push_back(0);
        testVector.push_back(1);
        testVector.push_back(2);
        testVector.push_back(3);
        testVector.push_back(4);
        testVector.push_back(5);

        return testVector;
    }

    }
}}}}