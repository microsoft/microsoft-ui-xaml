// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

class EnablePseudoLoc
{
public:
    EnablePseudoLoc()
    {
        test_infra::TestServices::Utilities->SetApplicationLanguageOverride(L"qps-ploc");
    }

    ~EnablePseudoLoc()
    {
        test_infra::TestServices::Utilities->ClearApplicationLanguageOverride();
    }

    // This type should never be copied or moved.
    EnablePseudoLoc(const EnablePseudoLoc&) = delete;
    EnablePseudoLoc(EnablePseudoLoc&&) = delete;
    EnablePseudoLoc& operator=(const EnablePseudoLoc&) = delete;
    EnablePseudoLoc& operator=(EnablePseudoLoc&&) = delete;
};

} } } } }
