// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "XamlLightTargetIdMapUnitTests.h"
#include "XamlLightTargetIdMap.h"
#include <MinXcpTypes.h>
#include <unordered_set>
#include <UIElement.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

void XamlLightTargetIdMapUnitTests::ValidateMap()
{
    WEX::TestExecution::SetVerifyOutput verifySettings(WEX::TestExecution::VerifyOutputSettings::LogOnlyFailures);

    XamlLightTargetIdMap map;
    VERIFY_ARE_EQUAL(static_cast<size_t>(0), map.m_map.size());

    CUIElement* one = reinterpret_cast<CUIElement*>(1234);
    CUIElement* two = reinterpret_cast<CUIElement*>(5678);

    xstring_ptr firstTarget;
    VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(STR_LEN_PAIR(L"firstTarget"), &firstTarget));

    xstring_ptr secondTarget;
    VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(STR_LEN_PAIR(L"secondTarget"), &secondTarget));

    xstring_ptr firstTarget2;
    VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(STR_LEN_PAIR(L"firstTarget"), &firstTarget2));

    xstring_ptr thirdTarget;
    VERIFY_SUCCEEDED(xstring_ptr::CloneBuffer(STR_LEN_PAIR(L"thirdTarget"), &secondTarget));

    LOG_OUTPUT(L"> Add firstTarget to 1234. 1234 has firstTarget.");
    VERIFY_IS_TRUE(map.AddTargetElementAndId(one, firstTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(1), map.m_map.size());
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_FALSE(map.ContainsTarget(two));
    VERIFY_IS_TRUE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_FALSE(map.ContainsTarget(one, secondTarget));
    ValidateEntryCount(1, map, one);

    LOG_OUTPUT(L"> Add secondTarget to 1234. 1234 has firstTarget, secondTarget.");
    VERIFY_IS_TRUE(map.AddTargetElementAndId(one, secondTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(1), map.m_map.size());
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_FALSE(map.ContainsTarget(two));
    VERIFY_IS_TRUE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    ValidateEntryCount(2, map, one);

    LOG_OUTPUT(L"> Add firstTarget to 5678. 5678 has firstTarget.");
    VERIFY_IS_TRUE(map.AddTargetElementAndId(two, firstTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(2), map.m_map.size());
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_TRUE(map.ContainsTarget(two));
    VERIFY_IS_TRUE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(two, firstTarget2));
    ValidateEntryCount(1, map, two);

    LOG_OUTPUT(L"> Add another firstTarget to 5678 (string equality but not object equality), which shouldn't add to the set again. 5678 has firstTarget.");
    VERIFY_IS_FALSE(map.AddTargetElementAndId(two, firstTarget2));
    VERIFY_ARE_EQUAL(static_cast<size_t>(2), map.m_map.size());
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_TRUE(map.ContainsTarget(two));
    VERIFY_IS_TRUE(map.ContainsTarget(one, firstTarget2));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(two, firstTarget));
    ValidateEntryCount(1, map, two);

    LOG_OUTPUT(L"> Remove firstTarget from 1234. 1234 has secondTarget.");
    VERIFY_IS_TRUE(map.RemoveTargetElementAndId(one, firstTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(2), map.m_map.size());
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_TRUE(map.ContainsTarget(two));
    VERIFY_IS_FALSE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(two, firstTarget));
    ValidateEntryCount(1, map, one);

    LOG_OUTPUT(L"> Remove thirdTarget from 1234. 1234 has secondTarget.");
    VERIFY_IS_FALSE(map.RemoveTargetElementAndId(one, thirdTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(2), map.m_map.size());
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_TRUE(map.ContainsTarget(two));
    VERIFY_IS_FALSE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(two, firstTarget));
    ValidateEntryCount(1, map, one);

    LOG_OUTPUT(L"> Remove another firstTarget from 5678 (string equality but not object equality). 5678 is no longer in the map.");
    VERIFY_IS_TRUE(map.RemoveTargetElementAndId(two, firstTarget2));
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_FALSE(map.ContainsTarget(two));
    VERIFY_IS_FALSE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_FALSE(map.ContainsTarget(two, firstTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(1), map.m_map.size());

    LOG_OUTPUT(L"> Remove firstTarget again from 5678. 5678 isn't even the map.");
    VERIFY_IS_FALSE(map.RemoveTargetElementAndId(two, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_FALSE(map.ContainsTarget(two));
    VERIFY_IS_FALSE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_FALSE(map.ContainsTarget(two, firstTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(1), map.m_map.size());

    LOG_OUTPUT(L"> Remove 5678 from the map.");
    map.RemoveTarget(two);
    VERIFY_IS_TRUE(map.ContainsTarget(one));
    VERIFY_IS_FALSE(map.ContainsTarget(two));
    VERIFY_IS_FALSE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_TRUE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_FALSE(map.ContainsTarget(two, firstTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(1), map.m_map.size());

    LOG_OUTPUT(L"> Remove 1234 from the map.");
    map.RemoveTarget(one);
    VERIFY_IS_FALSE(map.ContainsTarget(one));
    VERIFY_IS_FALSE(map.ContainsTarget(two));
    VERIFY_IS_FALSE(map.ContainsTarget(one, firstTarget));
    VERIFY_IS_FALSE(map.ContainsTarget(one, secondTarget));
    VERIFY_IS_FALSE(map.ContainsTarget(two, firstTarget));
    VERIFY_ARE_EQUAL(static_cast<size_t>(0), map.m_map.size());
}

void XamlLightTargetIdMapUnitTests::ValidateEntryCount(int expected, _In_ XamlLightTargetIdMap& map, _In_ CDependencyObject* key)
{
    auto pair = map.m_map.find(key);
    VERIFY_IS_TRUE(pair != map.m_map.end());
    VERIFY_ARE_EQUAL(static_cast<size_t>(expected), pair->second.size());
}

} } } } }
