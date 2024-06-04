// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

static const XCHAR* m_ppszKnownShortNames[] =
{
    //#comment  Generation tool will insert the short name for each listed type
    //#comment  L"<typeshortname>",
    //#insert TYPESHORTNAMES
};

static XBYTE m_uKnownShortNamesLength[] =
{
    //#comment  Generation tool will insert the character length of each listed short name
    //#comment  <typeshortnamelength>,
    //#insert TYPESHORTNAMELENGTHS
};

static const XCHAR* m_ppszKnownLongNames[] =
{
    //#comment  Generation tool will insert the short name for each listed type
    //#comment  L"<namespace.typeshortname>",
    //#insert TYPELONGNAMES
};

static XBYTE m_uKnownLongNamesLength[] =
{
    //#comment  Generation tool will insert the character length of each listed long name
    //#comment  <typelongnamelength>,
    //#insert TYPELONGNAMELENGTHS
};

static XBYTE m_pbTypeFlags[] =
{
    //#comment  Generation tool will insert the combined flags for each listed type
    //#comment  <typeflags>,
    //#insert TYPEFLAGS
};

static XUINT32 m_puCoreTypeIds[] =
{
    //#comment  Generation tool will insert the combined flags for each listed type
    //#comment  <typeindexname>,
    //#insert TYPEINDEXNAMES
};
