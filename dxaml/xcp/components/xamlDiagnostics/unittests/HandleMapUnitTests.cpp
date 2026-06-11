// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HandleMapUnitTests.h"
#include <HandleMap.h>

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace XamlDiagnostics {

    class DummyInspectable : public wrl::RuntimeClass<IInspectable>
    {
    };

    void HandleMapUnitTests::TestOrphanedHandleBasic()
    {
        //Our scenario is: handle0 is an element which has handle1 as a local property, and handle1 has handle2 as a local property.
        //When we overwrite handle1 with handle3 on handle0, we should fail to get both handle1 and handle2.

        //Setup our handle map - create the 4 dummy objects and their handles.
        HandleMap testMap;
        DummyInspectable dummies[4];
        InstanceHandle handles[4];

        for (int i = 0; i < 4; i++)
        {
            VERIFY_SUCCEEDED(HandleMap::CreateHandle(&dummies[i], &handles[i]));
        }

        //Setup the initial handle relationship in our map.
        int propertyIndex = 0xDEADBEEF;
        BaseValueSource bvs = BaseValueSource::BaseValueSourceLocal;
        testMap.AddElementToMap(handles[0]);
        testMap.AddElementToMap(handles[3]);
        testMap.AddPropertyToMap(handles[0], handles[1], propertyIndex, bvs, 0);
        testMap.AddPropertyToMap(handles[1], handles[2], propertyIndex, bvs, 0);

        //Sanity check we can initially get handle1 and handle2.
        VERIFY_IS_TRUE(testMap.HasProperty(handles[1]));
        VERIFY_IS_TRUE(testMap.HasProperty(handles[2]));

        //Overwrite handle1 with handle3.
        testMap.AddPropertyToMap(handles[0], handles[3], propertyIndex, bvs, 0);

        //Verify we can't get handle1 and handle2.
        VERIFY_IS_FALSE(testMap.HasProperty(handles[1]));
        VERIFY_IS_FALSE(testMap.HasProperty(handles[2]));

        //Sanity check that removing handle0 also cleans up handle3.
        //Sanity check we can get handle0 and handle3.
        VERIFY_IS_TRUE(testMap.HasElement(handles[0]));
        VERIFY_IS_TRUE(testMap.HasProperty(handles[3]));

        //Remove handle0, then verify handle0 and handle3 have been cleaned up as expected.
        testMap.ReleaseElement(handles[0]);
        VERIFY_IS_FALSE(testMap.HasElement(handles[0]));
        VERIFY_IS_FALSE(testMap.HasProperty(handles[0]));
        VERIFY_IS_FALSE(testMap.HasProperty(handles[3]));
    }

    void HandleMapUnitTests::TestOrphanedHandleMultipleOwners()
    {
        //Scenario: handle0 and handle1 are both elements which have handle2 as a local property.  handle2 has handle3 as a property.  When handle0
        //overwrites its handle2 property with handle4, handle2 and handle3 should still be accessible since handle1 still references handle2.

        //Setup our handle map - create the 5 dummy objects and their handles.
        HandleMap testMap;
        DummyInspectable dummies[5];
        InstanceHandle handles[5];

        for (int i = 0; i < 5; i++)
        {
            VERIFY_SUCCEEDED(HandleMap::CreateHandle(&dummies[i], &handles[i]));
        }

        //Setup the initial handle relationship in our map.
        int propertyIndex = 0xDEFACED;
        BaseValueSource bvs = BaseValueSource::BaseValueSourceLocal;
        testMap.AddElementToMap(handles[0]);
        testMap.AddElementToMap(handles[1]);
        testMap.AddElementToMap(handles[4]);
        testMap.AddPropertyToMap(handles[0], handles[2], propertyIndex, bvs, 0);
        testMap.AddPropertyToMap(handles[1], handles[2], propertyIndex, bvs, 0);
        testMap.AddPropertyToMap(handles[2], handles[3], propertyIndex, bvs, 0);

        //Sanity check we can initially get handle2 and handle3.
        VERIFY_IS_TRUE(testMap.HasProperty(handles[2]));
        VERIFY_IS_TRUE(testMap.HasProperty(handles[3]));

        //Overwrite handle0's property with handle4, and verify we can still access handle2 and handle3.
        testMap.AddPropertyToMap(handles[0], handles[4], propertyIndex, bvs, 0);
        VERIFY_IS_TRUE(testMap.HasProperty(handles[2]));
        VERIFY_IS_TRUE(testMap.HasProperty(handles[3]));

        //Delete handle1, verify handle2 and handle3 are both cleaned up since handle1 is now their sole owner
        testMap.ReleaseElement(handles[1]);
        VERIFY_IS_FALSE(testMap.HasElement(handles[1]));
        VERIFY_IS_FALSE(testMap.HasProperty(handles[1]));
        VERIFY_IS_FALSE(testMap.HasProperty(handles[2]));
        VERIFY_IS_FALSE(testMap.HasProperty(handles[3]));
    }
    
    void HandleMapUnitTests::PropertyGetsRemovedFromCreatedAfterSet()
    {
        HandleMap testMap;
        DummyInspectable dummyProp;
        
        auto propHandle = HandleMap::GetHandle(&dummyProp);
        testMap.AddToCreatedMap(propHandle);
        
        // Verify we can get the created item
        wrl::ComPtr<IInspectable> insp;
        VERIFY_SUCCEEDED(testMap.GetFromHandle(propHandle, &insp));
        
        DummyInspectable dummyElement;
        auto elemHandle = HandleMap::GetHandle(&dummyElement);
        int propertyIndex = 1;
        testMap.AddPropertyToMap(elemHandle, propHandle, propertyIndex, BaseValueSourceLocal);
        
        // Make sure the dummyProp is now found in the property map
        VERIFY_SUCCEEDED(testMap.GetPropertyFromHandle(propHandle, &insp));
        
        // And that it's just not in the created map at all
        auto iter = testMap.m_createdMap.find(propHandle);
        VERIFY_ARE_EQUAL(testMap.m_createdMap.end(), iter);
        
    }
}}}}}