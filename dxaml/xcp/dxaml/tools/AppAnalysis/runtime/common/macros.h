// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "AssertMacros.h"

#ifndef ASSERT_ON_ERROR
#ifdef ASSERT_ON_ERROR_ENABLED
#define ASSERT_ON_ERROR(x)  ASSERT(SUCCEEDED(x))
#else
#define ASSERT_ON_ERROR(x) (void) (x)
#endif
#endif

// REVIEW: For some reason, AppAnalysis defines its own IFC macros.  The difference between these and
//         the standard ones is that these ASSERT on failure where as the standard ones record the error
//         (via OnFailure) and displays to debug window.  The standard macros also check for GOTO/RETURN 
//         mismatches.  Can we just use the common ones.

#ifndef VERIFYHR
#if DBG
#define VERIFYHR(x) ASSERT(SUCCEEDED(x))
#else
#define VERIFYHR(x) (void) (x)
#endif
#endif

#ifndef IFC
#define IFC(x) { hr = (x); if (FAILED(hr)) { ASSERT_ON_ERROR(hr); goto Cleanup; } }
#endif

#ifndef IFCPTR
#define IFCPTR(x) if ((x) == NULL) { hr = E_POINTER; ASSERT_ON_ERROR(hr); goto Cleanup; }
#endif

#ifndef IFCSTATUS
#define IFCSTATUS(x) if ((x) != ERROR_SUCCESS) { hr = HRESULT_FROM_WIN32(x); if (SUCCEEDED(hr)) { hr = E_FAIL; }; ASSERT_ON_ERROR(hr); goto Cleanup; }
#endif

#ifndef IFCW32
#define IFCW32(x) if ((x) == FALSE) { hr = HRESULT_FROM_WIN32(GetLastError()); if (SUCCEEDED(hr)) { hr = E_FAIL; }; ASSERT_ON_ERROR(hr); goto Cleanup; }
#endif

#ifndef IFCOOM
#define IFCOOM(x) if ((x) == NULL)  { hr = E_OUTOFMEMORY; ASSERT_ON_ERROR(hr); goto Cleanup; }
#endif

#ifndef IFC_RETURN
#define IFC_RETURN(x) if (FAILED(x)) { ASSERT_ON_ERROR(x); return x; }
#endif

#ifndef IFCPTR_RETURN
#define IFCPTR_RETURN(x) if ((x) == NULL) { ASSERT_ON_ERROR(E_POINTER); return E_POINTER; }
#endif

#ifndef IFCOOM_RETURN
#define IFCOOM_RETURN(x) if ((x) == NULL)  {ASSERT_ON_ERROR(E_OUTOFMEMORY); return E_OUTOFMEMORY; }
#endif

#ifndef IFCW32_RETURN
#define IFCW32_RETURN(x) if ((x) == FALSE)  { IFC_RETURN((HRESULT_FROM_WIN32(x))); }
#endif

#ifndef IFCSTL_RETURN
#define IFCSTL_RETURN(exp) { try { exp; } catch (const std::exception&) { return E_UNEXPECTED; } }
#endif

#ifndef IFCSTATUS_RETURN
#define IFCSTATUS_RETURN(x) if ((x) != ERROR_SUCCESS) { IFC_RETURN(HRESULT_FROM_WIN32(x)); }
#endif

#ifndef IFCATL_RETURN
#define IFCATL_RETURN(exp) { try { exp; } catch (const CAtlException& ex) { IFC_RETURN(ex.m_hr); } }
#endif

#ifndef ARG_VALIDRETURNPOINTER
// Per new convention, force a null pointer deref on null return parameters.
#define ARG_VALIDRETURNPOINTER(x) { *x = *x; }
#endif

#ifndef HNS_TO_MS
#define HNS_TO_MS(hns) ((hns)/10000)
#endif

#ifndef MS_TO_S
#define MS_TO_S(ms) ((ms)/1000.00)
#endif

#ifndef BYTES_TO_KB
#define BYTES_TO_KB(bytes) ((bytes)/1000.00)
#endif

#ifndef StringRef
#define StringRef(string) wrl_wrappers::HStringReference(string).Get()
#endif


#define E_NOTFOUND HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
