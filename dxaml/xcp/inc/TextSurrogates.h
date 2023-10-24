// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef TextSurrogates_h
#define TextSurrogates_h

#define IS_SURROGATE(character)          (((character) & 0xF800) == 0xD800)
#define IS_LEADING_SURROGATE(character)  (((character) & 0xFC00) == 0xD800)
#define IS_TRAILING_SURROGATE(character) (((character) & 0xFC00) == 0xDC00)

#endif //TextSurrogates_h
