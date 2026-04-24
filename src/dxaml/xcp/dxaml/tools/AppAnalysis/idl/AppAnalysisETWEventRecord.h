// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
//CONST UUID IID_IAppAnalysisEtwEventRecord = { 0x5713EE3A, 0x36B5, 0x400E, { 0xBF, 0x6A, 0xDB, 0xB8, 0xDC, 0xD8, 0x47, 0xF8 } } };
namespace Microsoft { namespace Diagnostics { namespace AppAnalysis { 

    struct __declspec(uuid("C94C2C8A-8314-458A-AD62-A38F2EF7132E")) IEventProcessorPrivate : 
        public IInspectable
    {
        STDMETHOD(ProcessEvent)(
            PEVENT_RECORD value
            ) = 0;
    };
    
} } }
