// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wrl\module.h>

namespace Private { namespace Infrastructure {

class PrivateInfraModule: public Microsoft::WRL::Module<Microsoft::WRL::InProc, PrivateInfraModule>
{
public:
    bool IsFinalRelease() const
    {
        return s_bFinalRelease;
    }
    void SetFinalRelease()
    {
        s_bFinalRelease = true;
    }
    ~PrivateInfraModule() override;
private:
    static bool s_bFinalRelease;
};

} }
