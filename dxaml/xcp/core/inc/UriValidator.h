// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef URI_VALIDATOR_H
#define URI_VALIDATOR_H

struct IPALUri;
class CCoreServices;

namespace UriValidator
{
    _Check_return_
    HRESULT Validate(
        _In_ CCoreServices *pCore,
        _In_ const CValue &value
        );

    _Check_return_
    HRESULT Validate(
        _In_ CCoreServices *pCore,
        _In_ const xstring_ptr& value
        );
 };

#endif
