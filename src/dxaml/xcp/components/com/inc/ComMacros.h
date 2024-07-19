// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define PARTIAL_CLASS(T) class __declspec(novtable) __declspec(uuid(__##T##_GUID)) T : public T##Generated

#define BEGIN_INTERFACE_MAP(CURRENT, BASE) \
   private:\
      IID const * const * GetLocalImplementedIIDs(_Out_ INT *count)\
      {\
         static IID const * const localIIDs[] = \
         {
#define INTERFACE_ENTRY(CURRENT, i)             &__uuidof(i),
            
#define END_INTERFACE_MAP(CURRENT, BASE) \
         };\
         \
         *count = _countof(localIIDs);\
         \
         return localIIDs;\
      }\
      \
      INT GetLocalImplementedIIDsCount()\
      {\
         INT count = 0;\
         \
         GetLocalImplementedIIDs(&count);\
         \
         return count;\
      }\
      \
   protected:\
   \
      INT GetImplementedIIDsCount()\
      {\
         return GetLocalImplementedIIDsCount() + BASE::GetImplementedIIDsCount();\
      }\
      \
      virtual void CopyIIDsToArray(INT first, IID *pResult)\
      {\
         INT count = 0;\
         IID const * const * pLocalIIDs = GetLocalImplementedIIDs(&count);\
         INT current = 0;\
         \
         for (current = 0; current < count; current++)\
         {\
            pResult[first + current] = *(pLocalIIDs[current]);\
         }\
         \
         BASE::CopyIIDsToArray(first + current, pResult);\
      }

#define BEGIN_INTERFACE_MAP_NO_BASE(CURRENT) \
   private:\
      IID const * const GetLocalImplementedIIDs(_Out_ INT *count)\
      {\
         static IID const * const localIIDs[] = \
         {
#define INTERFACE_ENTRY(CURRENT, i)             &__uuidof(i),
            
#define END_INTERFACE_MAP_NO_BASE(CURRENT) \
         };\
         \
         *count = _countof(localIIDs);\
         \
         return localIIDs;\
      }\
      \
      INT GetLocalImplementedIIDsCount()\
      {\
         INT count = 0;\
         \
         GetLocalImplementedIIDs(&count);\
         \
         return count;\
      }\
      \
   protected:\
   \
      INT GetImplementedIIDsCount()\
      {\
         return GetLocalImplementedIIDsCount();\
      }\
      \
      virtual void CopyIIDsToArray(INT first, IID *pResult)\
      {\
         INT count = 0;\
         IID const * const * pLocalIIDs = GetLocalImplementedIIDs(&count);\
         INT current = 0;\
         \
         for (current = 0; current < count; current++)\
         {\
            pResult[first + current] = *(pLocalIIDs[current]);\
         }\
      }      

#define INSPECTABLE_CLASS(NAME) \
      protected:\
      HRESULT GetRuntimeClassNameImpl(_Out_ HSTRING *className)\
      {\
         return wrl_wrappers::HStringReference(NAME, SZ_COUNT(NAME)).CopyTo(className);\
      }
