// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CustomWriterRuntimeContext;
class CustomWriterRuntimeData;

// This interface is to be implemented by the runtime type that the CustomWriter maps to.
// It's a way for the parser to hand off the CustomRuntimeData that has been created by
// a CustomWriter to an instantiated version of that type. For example, VisualStateGroupCollectionCustomWriter
// creates a CustomWriterRuntimeData object containing information about VisualStates
// and VisualStateGroups. This object is handed back to the owning ObjectWriter, which will
// optionally serialize it. When it is time to create an actual object this data will
// be unserialized, a VisualStateGroupCollection instance will be created, and this
// data will be handed, through this interface, to that instance for storage.
struct ICustomWriterRuntimeDataReceiver
{
    virtual _Check_return_ HRESULT SetCustomWriterRuntimeData(std::shared_ptr<CustomWriterRuntimeData> data,
        std::unique_ptr<CustomWriterRuntimeContext> context) = 0;
};
