// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// In order to avoid polluting the global namespace with Started, Completed, Canceled, Error
// (from Microsoft-Windows-XAML-ETWEvents.h)
// _HIDE_GLOBAL_ASYNC_STATUS is defined in ProjectPreprocessorDefinitions in dxaml\common.props
// (For some reason defining it here did not work)

#pragma warning(push)
#pragma warning(disable:4996)
#pragma warning(disable:28159)
// This auto generated header uses deprecated api GetVersionEx 
#include "Microsoft-Windows-XAML-ETWEvents.h"
#pragma warning( pop )