// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "contentrootcoordinator.h"

ContentRootCoordinator::ContentRootCoordinator(CCoreServices& coreServices)
    : m_coreServices(coreServices)
{
}

ContentRootCoordinator::~ContentRootCoordinator()
{
}

XUINT32 CContentRoot::Release()
{
    return 0;
}