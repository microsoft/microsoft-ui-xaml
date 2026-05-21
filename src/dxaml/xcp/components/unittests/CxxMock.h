// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <stdlib.h>
#include <limits.h>
#include <assert.h>

namespace CxxMock
{
namespace Internal
{

// ATL::CSimpleArray
template <class T>
class CSimpleArray
{
public:
    CSimpleArray() : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
    { }

    ~CSimpleArray()
    {
        RemoveAll();
    }

    CSimpleArray(const CSimpleArray< T >& src) : m_aT(NULL), m_nSize(0), m_nAllocSize(0)
    {
        if (src.GetSize())
        {
            m_aT = (T*)calloc(src.GetSize(), sizeof(T));
            if (m_aT != NULL)
            {
                m_nAllocSize = src.GetSize();
                for (int i=0; i<src.GetSize(); i++)
                    Add(src[i]);
            }
        }
    }

    CSimpleArray< T >& operator=(const CSimpleArray< T >& src)
    {
        if (GetSize() != src.GetSize())
        {
            RemoveAll();
            m_aT = (T*)calloc(src.GetSize(), sizeof(T));
            if (m_aT != NULL)
                m_nAllocSize = src.GetSize();
        }
        else
        {
            for (int i = GetSize(); i > 0; i--)
                RemoveAt(i - 1);
        }

        for (int i=0; i<src.GetSize(); i++)
            Add(src[i]);

        return *this;
    }

    int GetSize() const
    {
        return m_nSize;
    }

    void Add(const T& t)
    {
        if(m_nSize == m_nAllocSize)
        {
            T* aT;
            int nNewAllocSize = (m_nAllocSize == 0) ? 1 : (m_nSize * 2);

            if (nNewAllocSize<0 || nNewAllocSize > INT_MAX/sizeof(T))
            {
                ReportError("Out of Memory");
            }

            aT = (T*)_recalloc(m_aT, nNewAllocSize, sizeof(T));
            if(aT == NULL)
            {
                ReportError("Out of Memory");
            } else {
                memset(aT + m_nAllocSize, 0, sizeof(T) * (nNewAllocSize - m_nAllocSize));

                m_nAllocSize = nNewAllocSize;
                m_aT = aT;
            }
        }

        InternalSetAtIndex(m_nSize, t);
        m_nSize++;
    }

    void RemoveAt(int nIndex)
    {
        assert(nIndex >= 0 && nIndex < m_nSize);
        if (nIndex < 0 || nIndex >= m_nSize)
            ReportError("Index out of bound");

        m_aT[nIndex].~T();
        if(nIndex != (m_nSize - 1))
        {
            if(0 != memmove_s(
                    (void*)(m_aT + nIndex),
                    (m_nSize - nIndex) * sizeof(T),
                    (void*)(m_aT + nIndex + 1),
                    (m_nSize - (nIndex + 1)) * sizeof(T)))
                ReportError("memmove error");
        }
        m_nSize--;
    }

    void RemoveAll()
    {
        if(m_aT != NULL)
        {
            for(int i = 0; i < m_nSize; i++)
                m_aT[i].~T();
            free(m_aT);
            m_aT = NULL;
        }
        m_nSize = 0;
        m_nAllocSize = 0;
    }

    const T& operator[] (int nIndex) const
    {
        assert(nIndex >= 0 && nIndex < m_nSize);
        if(nIndex < 0 || nIndex >= m_nSize)
        {
            ReportError("Index out of bound");
        }
        return m_aT[nIndex];
    }

    T& GetItem(int nIndex)
    {
        assert(nIndex >= 0 && nIndex < m_nSize);
        if(nIndex < 0 || nIndex >= m_nSize)
        {
            ReportError("Index out of bound");
        }
        return m_aT[nIndex];
    }

    T& operator[] (int nIndex)
    {
        return GetItem(nIndex);
    }

    void SetAtIndex(int nIndex, const T& t)
    {
        if (nIndex < 0 || nIndex >= m_nSize)
            ReportError("Index out of bound");

        InternalSetAtIndex(nIndex, t);
    }

    void InternalSetAtIndex(int nIndex, const T& t)
    {
        m_aT[nIndex] = t;
    }

    typedef T _ArrayElementType;
    T* m_aT;
    int m_nSize;
    int m_nAllocSize;
};

// Always use CRefCountedObject as virtual base class.
class CRefCountedObject
{
public:
    CRefCountedObject() : m_refCount(0)
    {
    }

    void AddRef()
    {
        ++ m_refCount;
    }

    void Release()
    {
        -- m_refCount;
        if (m_refCount == 0)
            delete this;
    }

    virtual ~CRefCountedObject()
    {
    }

private:
    unsigned int m_refCount;
};

} // namespace Internal

template<typename T>
class CConstraint : public virtual Internal::CRefCountedObject
{
public:
    virtual bool Check(T) = 0;
    typedef T ConstrainedType;
};

template <typename T>
class CValueAssigner : virtual public Internal::CRefCountedObject
{
public:
    virtual void Assign(T) = 0;
};

// CObjectPtr is a smart pointer that manages any object with AddRef() and Release() defined.
template <typename T = Internal::CRefCountedObject>
class CObjectPtr
{
public:
    CObjectPtr() : m_pObject(NULL)
    {
    }

    CObjectPtr(T * pObject)
    {
        m_pObject = pObject;
        if (m_pObject != NULL)
            m_pObject->AddRef();
    }

    template<typename T2>
    CObjectPtr(T2 * pObject)
    {
        m_pObject = pObject;
        if (m_pObject != NULL)
            m_pObject->AddRef();
    }

    CObjectPtr(CObjectPtr<T> const & ptr)
    {
        m_pObject = ptr.m_pObject;
        if (m_pObject != NULL)
            m_pObject->AddRef();
    }

    template<typename T2>
    CObjectPtr(CObjectPtr<T2> const & ptr)
    {
        m_pObject = ptr.m_pObject;
        if (m_pObject != NULL)
            m_pObject->AddRef();
    }

    CObjectPtr<T> & operator = (T * pObject)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = pObject;

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    template<typename T2>
    CObjectPtr<T> & operator = (T2 * pObject)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = pObject;

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    CObjectPtr<T> & operator = (CObjectPtr<T> const & ptr)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = ptr.m_pObject;

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    template<typename T2>
    CObjectPtr<T> & operator = (CObjectPtr<T2> const & ptr)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = ptr.m_pObject;

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    template<typename T2>
    CObjectPtr<T> & DynamicCast(T2 * pObject)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = dynamic_cast<T*>(pObject);

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    template<typename T2>
    CObjectPtr<T> & DynamicCast(CObjectPtr<T2> const & ptr)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = dynamic_cast<T*>(ptr.m_pObject);

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    template<typename T2>
    CObjectPtr<T> & StaticCast(T2 * pObject)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = static_cast<T*>(pObject);

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    template<typename T2>
    CObjectPtr<T> & StaticCast(CObjectPtr<T2> const & ptr)
    {
        if (m_pObject != NULL)
            m_pObject->Release();

        m_pObject = static_cast<T*>(ptr.m_pObject);

        if (m_pObject != NULL)
            m_pObject->AddRef();

        return *this;
    }

    bool operator == (T const * pObject)
    {
        return m_pObject == pObject;
    }

    bool operator != (T const * pObject)
    {
        return m_pObject != pObject;
    }

    T * operator->()
    {
        return m_pObject;
    }

    __if_exists(T::ConstrainedType)
    {
        CObjectPtr< CConstraint<typename T::ConstrainedType> >
        operator && (CObjectPtr< CConstraint<typename T::ConstrainedType> > const & ptrConstraint)
        {
            return new Internal::CAnd<typename T::ConstrainedType>(*this, ptrConstraint);
        }

        CObjectPtr< CConstraint<typename T::ConstrainedType> >
        operator || (CObjectPtr< CConstraint<typename T::ConstrainedType> > const & ptrConstraint)
        {
            return new Internal::COr<typename T::ConstrainedType>(*this, ptrConstraint);
        }

        CObjectPtr< CConstraint<typename T::ConstrainedType> >
        operator !()
        {
            return new Internal::CNot<typename T::ConstrainedType>(*this);
        }
    }

    ~CObjectPtr()
    {
        if (m_pObject != NULL)
            m_pObject->Release();
    }

    typedef T ObjectType;

private:
    template <typename T2> friend class CObjectPtr;
    T * m_pObject;
};

#pragma warning (push)
#pragma warning (disable : 4180 4181 4510 4512 4610)
// warning C4180: qualifier applied to function type has no meaning; ignored
// warning C4181: qualifier applied to reference type; ignored
// warning C4510: 'CxxMock::ValueList<T>' : default constructor could not be generated
// warning C4512: 'CxxMock::ValueList<T>' : assignment operator could not be generated
// warning C4610: 'CxxMock::ValueList<T>' can never be instantiated - user defined constructor required

template <typename T>
struct DeepConstCast
{
    typedef const T Result;
};

template <typename T>
struct DeepConstCast<T *>
{
    typedef typename DeepConstCast<T>::Result * const Result;
};

template <typename T>
struct DeepConstCast<T * const>
{
    typedef typename DeepConstCast<T>::Result * const Result;
};

template <typename T>
struct DeepConstCast<T &>
{
    typedef typename DeepConstCast<T>::Result & Result;
};

class ICommand
{
public:
    virtual void Exec() = 0;
    virtual ~ICommand()
    {
    }
};

// ReferenceCast: cast a type to corresponding reference type, but leave reference types as is.
template <typename T>
struct ReferenceCast
{
    typedef T & Result;
};

template <typename T>
struct ReferenceCast<T const>
{
    typedef T const & Result;
};

template <typename T>
struct ReferenceCast<T &>
{
    typedef T & Result;
};

template <typename T>
struct ReferenceCast<T const &>
{
    typedef T const & Result;
};

// NonReferenceCast: cast away & on a type.
template <typename T>
struct NonReferenceCast
{
    typedef T Result;
};

template <typename T>
struct NonReferenceCast<T const>
{
    typedef T const Result;
};

template <typename T>
struct NonReferenceCast<T &>
{
    typedef T Result;
};

template <typename T>
struct NonReferenceCast<T const &>
{
    typedef T const Result;
};

class CMockObjectError
{
public:
    CMockObjectError(const char * fileName, int lineNumber, const char *const& p)
        : m_fileName(fileName)
        , m_lineNumber(lineNumber)
        , m_error(p)
    {
        if (fileName != NULL)
            sprintf_s(what_, sizeof(what_), "%s(%d): %s", fileName, lineNumber, p);
        else
            strncpy_s(what_, p, sizeof(what_));

        __if_exists(OutputDebugStringA)
        {
            OutputDebugStringA(what_);
            OutputDebugString(TEXT("\n"));
        }
    }

    int GetLineNumber()
    {
        return m_lineNumber;
    }

    char const * GetFileName()
    {
        return m_fileName;
    }

    char const * GetErrorText()
    {
        return m_error;
    }

    virtual const char* what() const
    {
        return m_error;
    }

private:
    char const * m_fileName;
    int m_lineNumber;
    const char * m_error;
    char what_[256];
    
    //enum {
    //    UnexpectedFunctionCall,
    //    IncorrectArgumentConstraintType,
    //    ReturnValueNotDefined,
    //    IncorrectCallSequence,
    //    IncorrectNumberOfCalls
    //};
};

namespace Internal
{

// Length enum is added to simplify the logic and syntax of computing typelist length.
struct NullType
{
    enum {Length = 0};
};

template <typename T1, typename T2>
struct TypeList
{
    typedef T1 Head;
    typedef T2 Tail;
    enum {Length = Tail::Length + 1};
};

#define CM_TYPELIST_0 ::CxxMock::Internal::NullType
#define CM_TYPELIST_1(T) ::CxxMock::Internal::TypeList< T, CM_TYPELIST_0 >
#define CM_TYPELIST_2(T1, T2) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_1(T2) >
#define CM_TYPELIST_3(T1, T2, T3) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_2(T2, T3) >
#define CM_TYPELIST_4(T1, T2, T3, T4) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_3(T2, T3, T4) >
#define CM_TYPELIST_5(T1, T2, T3, T4, T5) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_4(T2, T3, T4, T5) >
#define CM_TYPELIST_6(T1, T2, T3, T4, T5, T6) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_5(T2, T3, T4, T5, T6) >
#define CM_TYPELIST_7(T1, T2, T3, T4, T5, T6, T7) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_6(T2, T3, T4, T5, T6, T7) >
#define CM_TYPELIST_8(T1, T2, T3, T4, T5, T6, T7, T8) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_7(T2, T3, T4, T5, T6, T7, T8) >
#define CM_TYPELIST_9(T1, T2, T3, T4, T5, T6, T7, T8, T9) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_8(T2, T3, T4, T5, T6, T7, T8, T9) >
#define CM_TYPELIST_10(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) ::CxxMock::Internal::TypeList< T1, CM_TYPELIST_9(T2, T3, T4, T5, T6, T7, T8, T9, T10) >

template <typename T, unsigned int i>
struct TypeAt
{
    // Most likely caused by SetOutValue<> with out-of-bound index
    typedef char CheckTypeListIndex[(T::Length > i) ? 1 : -1];
    typedef typename TypeAt< typename T::Tail, i-1 >::Result Result;
};

template <typename T>
struct TypeAt<T, 0>
{
    typedef typename T::Head Result;
};

template <typename T>
struct ValueList
{
};

template <typename T1, typename T2>
struct ValueList< TypeList<T1, T2> >
{
    T1 value;
    ValueList<T2> next;
};

template <typename T> struct DerefType;
template <typename T>
struct DerefType<T*>
{
    typedef T Result;
};

template <typename T>
struct DerefType<T* const>
{
    typedef T Result;
};

template <typename T>
struct DerefType<T&>
{
    typedef T Result;
};

inline void ReportError(_In_z_ __format_string const char * szFormat, ...)
{
    va_list pArgList;
    va_start( pArgList, szFormat );
    char buffer[256];
    vsprintf_s(buffer, sizeof(buffer), szFormat, pArgList);
    va_end( pArgList );
    throw CMockObjectError(NULL, 0, buffer);
}

#pragma region simple expressions
template <typename T>
class CEqual : public CConstraint<T>
{
public:
    CEqual(T value) : m_value(value)
    {
    }

    bool Check(T arg)
    {
        return m_value == arg;
    }
private:
    T m_value;
};

//template <typename T>
//class CEqualArrayDeref : public CConstraint<T*>
//{
//public:
//    CEqualArrayDeref(T* value, unsigned n) : m_value(value), n_(n)
//    {
//        m_value = new T[n];
//        for (unsigned int i = 0; i < n_; i++)
//            m_value[i] = value[i]
//    }
//
//    bool Check(T* arg)
//    {
//        for (unsigned int i = 0; i < n_; i++)
//            if (m_value[i] != arg[i])
//                return false;
//
//        return true;
//    }
//    ~CEqualArrayDeref()
//    {
//        delete[] m_value;
//    }
//
//private:
//    T* m_value;
//    unsigned n_;
//};
//
//template <typename T>
//CEqualArrayDeref<T> * eqderef(T* value, unsigned int n = 1)
//{
//    return new CEqualArrayDeref<T>(value, n);
//}

template <typename T>
class CNotEqual : public CConstraint<T>
{
public:
    CNotEqual(T value) : m_value(value)
    {
    }

    bool Check(T arg)
    {
        return m_value != arg;
    }
private:
    T m_value;
};

template <typename T>
class CGreaterThan : public CConstraint<T>
{
public:
    CGreaterThan(T value) : m_value(value)
    {
    }

    bool Check(T arg)
    {
        return arg > m_value;
    }
private:
    T m_value;
};

template <typename T>
class CGreaterThanOrEqualTo : public CConstraint<T>
{
public:
    CGreaterThanOrEqualTo(T value) : m_value(value)
    {
    }

    bool Check(T arg)
    {
        return arg >= m_value;
    }
private:
    T m_value;
};

template <typename T>
class CLessThan : public CConstraint<T>
{
public:
    CLessThan(T value) : m_value(value)
    {
    }

    bool Check(T arg)
    {
        return arg < m_value;
    }
private:
    T m_value;
};

template <typename T>
class CLessThanOrEqualTo : public CConstraint<T>
{
public:
    CLessThanOrEqualTo(T value) : m_value(value)
    {
    }

    bool Check(T arg)
    {
        return arg <= m_value;
    }
private:
    T m_value;
};

#pragma endregion

#pragma region string expressions
template <typename T>
class CStringContains : public CConstraint<T>
{
public:
    CStringContains(T value) : m_value(value)
    {
    }
    bool Check(T arg);

private:
    T m_value;
};

inline bool CStringContains<const char * const>::Check(const char * const arg)
{
    return (strstr(arg, m_value) != NULL);
}

inline bool CStringContains<const wchar_t * const>::Check(const wchar_t * const arg)
{
    return (wcsstr(arg, m_value) != NULL);
}

template <typename T>
class CStringEquals : public CConstraint<T>
{
public:
    CStringEquals(T value, bool CaseSensitive)
        : m_value(value)
        , m_CaseSensitive(CaseSensitive)
    {
    }
    bool Check(T arg);

private:
    T m_value;
    bool m_CaseSensitive;
};

inline bool CStringEquals<const char * const>::Check(const char * const arg)
{
    return (m_CaseSensitive ? 0 == strcmp(arg, m_value) : 0 == _stricmp(arg, m_value));
}

inline bool CStringEquals<const wchar_t * const>::Check(const wchar_t * const arg)
{
    return (m_CaseSensitive ? 0 == wcscmp(arg, m_value) : 0 == _wcsicmp(arg, m_value));
}

#pragma endregion

template <typename T>
class CDeref : public CConstraint<const T * const>, public CConstraint<const T &>
{
public:
    CDeref(CObjectPtr< CConstraint<T> > const & cns) : m_cns(cns)
    {
    }

    bool Check(const T * const value)
    {
        return m_cns->Check(*value);
    }

    bool Check(const T & value)
    {
        return m_cns->Check(value);
    }

private:
    CObjectPtr< CConstraint<T> > m_cns;
};

template <typename SmartPointerType, typename ConstraintType>
class CSPDeref : public CConstraint<SmartPointerType>
{
public:
    CSPDeref(CObjectPtr< ConstraintType > const & cns) : m_cns(cns)
    {
    }

    bool Check(SmartPointerType value)
    {
        return m_cns->Check(*value);
    }

private:
    CObjectPtr< ConstraintType > m_cns;
};

template <typename StructType, typename FieldType>
class CFieldAccess : public CConstraint<StructType>
                   , public CConstraint<StructType * const>
                   , public CConstraint<StructType &>
{
public:
    CFieldAccess(FieldType StructType::* pField,
        CObjectPtr< CConstraint<typename DeepConstCast<FieldType>::Result> > const & pConstraint)
        : m_pField(pField)
        , m_pConstraint(pConstraint)
    {
    }

    // copying whole structure !
    bool Check(StructType s)
    {
        return m_pConstraint->Check(s.*m_pField);
    }

    bool Check(StructType * const p)
    {
        return m_pConstraint->Check(p->*m_pField);
    }

    bool Check(StructType & s)
    {
        return m_pConstraint->Check(s.*m_pField);
    }

private:
    FieldType StructType::* m_pField;
    CObjectPtr< CConstraint<typename DeepConstCast<FieldType>::Result> > m_pConstraint;
};

// Unlike CFieldAccess, CGetProperty cannot inherit from CConstraint<TClass>
// because interface and abstract classes cannot be passed by value
template <typename TClass, typename TReturn>
class CGetProperty : public CConstraint<TClass * const>
                     , public CConstraint<TClass &>
{
public:
    CGetProperty(TReturn (TClass::*pMethod)(void) const,
                   CObjectPtr< CConstraint<TReturn const> > const & exp)
        : m_pMethod(pMethod)
        , m_ptrConstraint(exp)
    {
    }

    bool Check(TClass * const p)
    {
        return m_ptrConstraint->Check((p->*m_pMethod)());
    }

    bool Check(TClass & s)
    {
        return m_ptrConstraint->Check((s.*m_pMethod)());
    }

    TReturn (TClass::*m_pMethod)(void) const;
    CObjectPtr< CConstraint<TReturn const> > m_ptrConstraint;
};

template <typename T>
class CAnd : public CConstraint<T>
{
public:
    CAnd(CObjectPtr< CConstraint<const T> > const & exp1,
         CObjectPtr< CConstraint<const T> > const & exp2)
        : m_exp1(exp1)
        , m_exp2(exp2)
    {
    }
    bool Check(T value)
    {
        return m_exp1->Check(value) && m_exp2->Check(value);
    }

private:
    CObjectPtr< CConstraint<const T> > m_exp1;
    CObjectPtr< CConstraint<const T> > m_exp2;
};

template <typename T>
class COr : public CConstraint<T>
{
public:
    COr(CObjectPtr< CConstraint<const T> > const & exp1,
        CObjectPtr< CConstraint<const T> > const & exp2)
        : m_exp1(exp1)
        , m_exp2(exp2)
    {
    }
    bool Check(T value)
    {
        return m_exp1->Check(value) || m_exp2->Check(value);
    }

private:
    CObjectPtr< CConstraint<const T> > m_exp1;
    CObjectPtr< CConstraint<const T> > m_exp2;
};

template <typename T>
class CNot : public CConstraint<T>
{
public:
    CNot(CObjectPtr< CConstraint<const T> > const & exp)
        : m_exp(exp)
    {
    }
    bool Check(T value)
    {
        return !m_exp->Check(value);
    }

private:
    CObjectPtr< CConstraint<const T> > m_exp;
};

//template <typename T1, typename T2>
//class CDynamicCast : public CConstraint<T1>
//{
//public:
//    CDynamicCast(CConstraint<T2>* exp)
//        : m_exp(exp)
//    {
//    }
//    bool Check(T1 value)
//    {
//        return m_exp->Check(dynamic_cast<T2>(value));
//    }
//    ~CDynamicCast()
//    {
//        delete m_exp;
//    }
//
//private:
//    CConstraint<T2>* m_exp;
//};
//
//template <typename T1, typename T2>
//class CStaticCast : public CConstraint<T1>
//{
//public:
//    CStaticCast(CConstraint<T2>* exp)
//        : m_exp(exp)
//    {
//    }
//    bool Check(T1 value)
//    {
//        return m_exp->Check(static_cast<T2>(value));
//    }
//    ~CStaticCast()
//    {
//        delete m_exp;
//    }
//
//private:
//    CConstraint<T2>* m_exp;
//};
//
//template <typename T1, typename T2>
//class CReinterpretCast : public CConstraint<T1>
//{
//public:
//    CReinterpretCast(CConstraint<T2>* exp)
//        : m_exp(exp)
//    {
//    }
//    bool Check(T1 value)
//    {
//        return m_exp->Check(reinterpret_cast<T2>(value));
//    }
//    ~CReinterpretCast()
//    {
//        delete m_exp;
//    }
//
//private:
//    CConstraint<T2>* m_exp;
//};
//
typedef unsigned int InstanceIdType;
typedef CObjectPtr< CConstraint<const int> > TCallCountConstraintPtr;

class IErrorReporter
{
public:
    virtual void ReportErrorWithLineNumber(_In_z_ __format_string const char * szFormat, ...) = 0;
    virtual void ReportArgumentError() = 0;
    virtual void ReportCallCountError() = 0;
    virtual void ReportCallOrderError() = 0;
    virtual void ReportUndefinedReturnValue() = 0;
};

template<typename ArgumentTypeList>
class CArgumentConstraints
{
public:
    CArgumentConstraints()
    {
        for (int i=0; i<ArgumentTypeList::Length; i++)
            m_constraints[i] = NULL;
    }

    virtual ~CArgumentConstraints()
    {
    }

    void SetConstraints(CObjectPtr<> constraints[])
    {
        // assert(ArgumentTypeList::Length > 0);
        // assert(ArgumentTypeList::Length < 10);
        typedef char cassert[ArgumentTypeList::Length > 0 && ArgumentTypeList::Length < 10 ? 1 : -1];

        for (int i=0; i<ArgumentTypeList::Length; i++)
        {
            m_constraints[i] = constraints[i];
        }

        VerifyConstraintTypes<ArgumentTypeList>(1, m_constraints);
    }

private:
    friend class CMethodExpectationListType;

    bool HasArgumentConstraint()
    {
        for (int i = 0; i < ArgumentTypeList::Length; i++)
        {
            if (m_constraints[i] != NULL)
                return true;
        }

        return false;
    }

    bool MatchArguments(ValueList< ArgumentTypeList >& args)
    {
        return MatchArgumentsRecursive(args, m_constraints);
    }

    template <typename T1, typename T2>
    static bool MatchArgumentsRecursive(ValueList<TypeList<T1, T2> >& args,
                                        CObjectPtr<> constraints[])
    {
        CObjectPtr< CConstraint<DeepConstCast<T1>::Result > > ptrConstraint;
        ptrConstraint.DynamicCast(constraints[0]);

        if (ptrConstraint != NULL && !ptrConstraint->Check(args.value))
            return false;

        return MatchArgumentsRecursive(args.next, constraints + 1);
    }

    template <typename T1>
    static bool MatchArgumentsRecursive(ValueList<TypeList<T1, NullType> >& args,
                                        CObjectPtr<> constraints[])
    {
        typedef DeepConstCast<T1>::Result ConstArgType;

        CObjectPtr< CConstraint< ReferenceCast<ConstArgType>::Result > > ptrRefConstraint;
        CObjectPtr< CConstraint< NonReferenceCast<ConstArgType>::Result > > ptrNonRefConstraint;

        ptrNonRefConstraint.DynamicCast(constraints[0]);

        if (ptrNonRefConstraint != NULL)
        {
            if (!ptrNonRefConstraint->Check(args.value))
                return false;
        }
        else
        {
            ptrRefConstraint.DynamicCast(constraints[0]);
            if (ptrRefConstraint != NULL && !ptrRefConstraint->Check(args.value))
                return false;
        }

        return true;
    }

    template <typename TL>
    void VerifyConstraintTypes(int position, CObjectPtr<> constraints[])
    {
        if (constraints[0] != NULL)
        {
            typedef DeepConstCast<TL::Head>::Result ConstArgType;

            CObjectPtr< CConstraint< ReferenceCast<ConstArgType>::Result > > ptrRefConstraint;
            CObjectPtr< CConstraint< NonReferenceCast<ConstArgType>::Result > > ptrNonRefConstraint;

            if (ptrRefConstraint.DynamicCast(constraints[0]) == NULL &&
                ptrNonRefConstraint.DynamicCast(constraints[0]) == NULL)
            {
                IErrorReporter *reporter = dynamic_cast<IErrorReporter*>(this);
                reporter->ReportErrorWithLineNumber("error: Constraint type for parameter %d is incorrect", position);
                //"error: cannot convert from %s to %s.",
                //typeid(**ppConstraint).name(),
                //typeid(CConstraint< DeepConstCast<TL::Head>::Result >).name());
            }
        }

        VerifyConstraintTypes<TL::Tail>(position + 1, constraints+1);
    }

    template <>
    void VerifyConstraintTypes<NullType>(int, CObjectPtr<> [])
    {
    }

    CObjectPtr<> m_constraints[ArgumentTypeList::Length];
};

template<>
class CArgumentConstraints<NullType>
{
public:
    CArgumentConstraints()
    {
    }

private:
    friend class CMethodExpectationListType;

    bool HasArgumentConstraint()
    {
        return false;
    }

    bool MatchArguments(ValueList<NullType>&)
    {
        return true;
    }
};

template<typename ArgumentTypeList, typename Derived>
class IArgumentConstraints
{
public:
    Derived & With(
        CObjectPtr<> pc1,
        CObjectPtr<> pc2 = NULL,
        CObjectPtr<> pc3 = NULL,
        CObjectPtr<> pc4 = NULL,
        CObjectPtr<> pc5 = NULL,
        CObjectPtr<> pc6 = NULL,
        CObjectPtr<> pc7 = NULL,
        CObjectPtr<> pc8 = NULL,
        CObjectPtr<> pc9 = NULL,
        CObjectPtr<> pc10 = NULL)
    {
        CObjectPtr<> constraints[] = {pc1, pc2, pc3, pc4, pc5, pc6, pc7, pc8, pc9, pc10};
        Derived *pDerived = static_cast<Derived*>(this);
        CArgumentConstraints<ArgumentTypeList> * p
            = dynamic_cast<CArgumentConstraints<ArgumentTypeList>*>(pDerived);

        p->SetConstraints(constraints);

        return *pDerived;
    };
};

template<typename Derived>
class IArgumentConstraints<NullType, Derived>
{
};

template<typename ReturnType>
class CReturnValues
{
public:
    CReturnValues()
        : m_indexNextReturnValue(0)
    {
    }

    void AddReturnValue(ReturnType arg)
    {
        m_returnValues.Add(arg);
    }

    ReturnType GetNextReturnValue()
    {
        if (m_returnValues.GetSize() == 0)
        {
            IErrorReporter *reporter = dynamic_cast<IErrorReporter*>(this);
            reporter->ReportUndefinedReturnValue();
        }

        if (m_indexNextReturnValue == m_returnValues.GetSize())
            m_indexNextReturnValue = 0;

        return m_returnValues[m_indexNextReturnValue++];
    }

    virtual ~CReturnValues() {}

private:
    CSimpleArray< ReturnType > m_returnValues;
    int m_indexNextReturnValue;
};

template<typename DerefReturnType>
class CReturnValues<DerefReturnType &>
{
public:
    CReturnValues()
        : m_indexNextReturnValue(0)
    {
    }

    void AddReturnValue(DerefReturnType & arg)
    {
        m_returnValues.Add(&arg);
    }

    DerefReturnType & GetNextReturnValue()
    {
        if (m_returnValues.GetSize() == 0)
        {
            IErrorReporter *reporter = dynamic_cast<IErrorReporter*>(this);
            reporter->ReportUndefinedReturnValue();
        }

        if (m_indexNextReturnValue == m_returnValues.GetSize())
            m_indexNextReturnValue = 0;

        return *m_returnValues[m_indexNextReturnValue++];
    }

    virtual ~CReturnValues() {}

private:
    CSimpleArray< DerefReturnType * > m_returnValues;
    int m_indexNextReturnValue;
};

template<>
class CReturnValues<void>
{
};

template<typename ReturnType, typename Derived>
class IReturnValues
{
public:
    Derived& ReturnValue(ReturnType arg)
    {
        Derived *pDerived = static_cast<Derived*>(this);
        CReturnValues<ReturnType> * p = dynamic_cast<CReturnValues<ReturnType>*>(pDerived);
        p->AddReturnValue(arg);
        return *pDerived;
    }

    Derived& ReturnValue(ReturnType arg1,
                         ReturnType arg2)
    {
        ReturnValue(arg1);
        return ReturnValue(arg2);
    }

    Derived& ReturnValue(ReturnType arg1,
                         ReturnType arg2,
                         ReturnType arg3)
    {
        ReturnValue(arg1);
        ReturnValue(arg2);
        return ReturnValue(arg3);
    }

    Derived& ReturnValue(ReturnType arg1,
                         ReturnType arg2,
                         ReturnType arg3,
                         ReturnType arg4)
    {
        ReturnValue(arg1);
        ReturnValue(arg2);
        ReturnValue(arg3);
        return ReturnValue(arg4);
    }

    Derived& ReturnValue(ReturnType arg1,
                         ReturnType arg2,
                         ReturnType arg3,
                         ReturnType arg4,
                         ReturnType arg5)
    {
        ReturnValue(arg1);
        ReturnValue(arg2);
        ReturnValue(arg3);
        ReturnValue(arg4);
        return ReturnValue(arg5);
    }
};

struct OutValueEntry
{
    OutValueEntry(int index, CObjectPtr<> const & ptrObject)
        : i(index)
        , m_ptrAssignValue(ptrObject)
    {
    }

    int i;
    CObjectPtr<> m_ptrAssignValue;
};

template<typename Derived>
class IReturnValues<void, Derived>
{
};

class COutValues
{
public:
    typedef CSimpleArray<OutValueEntry> OutValues;
    void AddOutValue(OutValueEntry value)
    {
        m_outValues.Add(value);
    }

protected:

    template < typename T >
    void AssignOutValue(int index, T* arg)
    {
        for (int k = 0; k < m_outValues.GetSize(); k++)
            if (m_outValues[k].i == index)
            {
                CObjectPtr< CValueAssigner<T *> > ptrAssignValue;
                ptrAssignValue.DynamicCast(m_outValues[k].m_ptrAssignValue);
                ptrAssignValue->Assign(arg);
            }
    }

    template < typename T >
    void AssignOutValue(int index, T& arg)
    {
        for (int k = 0; k < m_outValues.GetSize(); k++)
            if (m_outValues[k].i == index)
            {
                CObjectPtr< CValueAssigner<T &> > ptrAssignValue;
                ptrAssignValue.DynamicCast(m_outValues[k].m_ptrAssignValue);
                ptrAssignValue->Assign(arg);
            }
    }

    template <>
    void AssignOutValue(int, void*)
    {
    }

    template < typename T >
    void AssignOutValue(int, T const *)
    {
    }

    template < typename T >
    void AssignOutValue(int, T const &)
    {
    }

    template < typename T >
    void AssignOutValues(int i, ValueList<T> & arg)
    {
        AssignOutValue(i, arg.value);
        AssignOutValues(i+1, arg.next);
    }

    template <>
    void AssignOutValues(int, ValueList<NullType> &)
    {
    }

private:
    OutValues m_outValues;
};

template <typename ArgumentTypeList, typename Derived>
class IOutValues
{
public:
    template <int n>
    Derived & SetOutValue(typename DerefType<typename TypeAt<ArgumentTypeList, n>::Result>::Result * v,
                           unsigned int m)
    {
        typedef typename DerefType<typename TypeAt<ArgumentTypeList, n>::Result>::Result ArgType;
        Derived *pDerived = static_cast<Derived*>(this);
        COutValues * p = dynamic_cast<COutValues*>(pDerived);
        p->AddOutValue(OutValueEntry(n, new CValueArrayAssigner<ArgType>(v, m)));
        return *pDerived;
    }

    template <int n>
    Derived & SetOutValue(typename DerefType<typename TypeAt<ArgumentTypeList, n>::Result>::Result v)
    {
        typedef typename DerefType<typename TypeAt<ArgumentTypeList, n>::Result>::Result ArgType;
        Derived *pDerived = static_cast<Derived*>(this);
        COutValues * p = dynamic_cast<COutValues*>(pDerived);
        p->AddOutValue(OutValueEntry(n, new CSingleValueAssigner<ArgType>(v)));
        return *pDerived;
    }

    template <int n, typename Struct, typename FieldType>
    Derived & SetOutValue(FieldType Struct::* pField, FieldType value)
    {
        Derived *pDerived = static_cast<Derived*>(this);
        COutValues * p = dynamic_cast<COutValues*>(pDerived);
        p->AddOutValue(OutValueEntry(n, new CFieldValueAssigner<Struct, FieldType>(pField, value)));
        return *pDerived;
    }

    template <int n>
    Derived & SetOutValue(CObjectPtr<CValueAssigner<typename TypeAt<ArgumentTypeList, n>::Result> > v)
    {
        typedef typename DerefType<typename TypeAt<ArgumentTypeList, n>::Result>::Result ArgType;
        Derived *pDerived = static_cast<Derived*>(this);
        COutValues * p = dynamic_cast<COutValues*>(pDerived);
        p->AddOutValue(OutValueEntry(n, v));
        return *pDerived;
    }
};

template <typename Derived>
class IOutValues<NullType, Derived>
{
};

__interface IAfter
{
    void AfterMethod(char const *);
    void AfterInstanceMethod(InstanceIdType, char const *);
};

// Declare methods that are not dependent on stub method signagure.
template <typename ReturnType, typename ArgumentTypeList>
class IMethodExpectation
    : public IArgumentConstraints<ArgumentTypeList, IMethodExpectation<typename ReturnType, typename ArgumentTypeList> >
    , public IReturnValues<ReturnType, IMethodExpectation<typename ReturnType, typename ArgumentTypeList> >
    , public IOutValues<ArgumentTypeList, IMethodExpectation<typename ReturnType, typename ArgumentTypeList> >
{
public:
    virtual IMethodExpectation & CallCount(TCallCountConstraintPtr const & cons) = 0;

    IMethodExpectation & Once()
    {
        return CallCount(eq(1));
    }

    IMethodExpectation & Twice()
    {
        return CallCount(eq(2));
    }

    IMethodExpectation & AtLeastOnce()
    {
        return CallCount(ge(1));
    }

    IMethodExpectation & AtLeastTwice()
    {
        return CallCount(ge(2));
    }

    IMethodExpectation & AtMostOnce()
    {
        return CallCount(le(1));
    }

    IMethodExpectation & AtMostTwice()
    {
        return CallCount(le(2));
    }

    virtual IMethodExpectation & Id(char const *p) = 0;

    IMethodExpectation & After(char const *p)
    {
        IAfter * pAfter;
        pAfter = dynamic_cast<IAfter*>(this);

        pAfter->AfterMethod(p);

        return *this;
    };

    template<typename T>
    IMethodExpectation & After(T & instance, char const *p)
    {
        IAfter * pAfter;
        pAfter = dynamic_cast<IAfter*>(this);

        pAfter->AfterInstanceMethod(instance.zzbase_.GetInstanceId(), p);

        return *this;
    }

    virtual IMethodExpectation & Will(ICommand* command) = 0;
};

class CMethodExpectationBase;

class COrderedCallsManager
{
public:
    static void BeginOrderedCalls()
    {
        if (m_WithinOrderedCallsGroup)
            throw CMockObjectError(NULL, 0, "error: previous BeginOrderedCalls() has no matching EndOrderedCalls()");

        m_WithinOrderedCallsGroup = true;
        m_pOrderedCallsGroupTail = NULL;
    }

    static void EndOrderedCalls()
    {
        if (!m_WithinOrderedCallsGroup)
            throw CMockObjectError(NULL, 0, "error: EndOrderedCalls() is called without matching BeginOrderedCalls()");

        m_WithinOrderedCallsGroup = false;
        m_pOrderedCallsGroupTail = NULL;
    }

    static CMethodExpectationBase * GetGroupTail()
    {
        return m_pOrderedCallsGroupTail;
    }

    static void SetGroupTail(CMethodExpectationBase * p)
    {
        m_pOrderedCallsGroupTail = p;
    }

    static bool WithinOrderedCallsGroup()
    {
        return m_WithinOrderedCallsGroup;
    }

private:
    static bool m_WithinOrderedCallsGroup;
    static CMethodExpectationBase * m_pOrderedCallsGroupTail;
};

__declspec(selectany) CMethodExpectationBase * COrderedCallsManager::m_pOrderedCallsGroupTail = NULL;
__declspec(selectany) bool COrderedCallsManager::m_WithinOrderedCallsGroup = false;


class CMethodExpectationBase : public IErrorReporter
{
public:
    CMethodExpectationBase(InstanceIdType instanceId,
                           char const * fileName,
                           int lineNumber,
                           char const * objectName,
                           char const * methodName)
        : m_InstanceId(instanceId)
        , m_objectName(objectName)
        , m_methodName(methodName)
        , m_fileName(fileName)
        , m_lineNumber(lineNumber)
        , m_id(NULL)
        , m_afterInstance(0)
        , m_afterMethod(NULL)
        , m_callCount(0)
    {
        m_previousCallInGroup = COrderedCallsManager::GetGroupTail();
        if (COrderedCallsManager::WithinOrderedCallsGroup())
        {
            inOrderedCallsGroup = true;
            COrderedCallsManager::SetGroupTail(this);
        }
        else
        {
            inOrderedCallsGroup = false;
        }
    }

    virtual ~CMethodExpectationBase()
    {
    }

    bool HasCallCountConstraint()
    {
        return m_ptrCallCountConstraint != NULL;
    }

    bool CanMatchMoreCalls()
    {
        if (m_ptrCallCountConstraint == NULL)
            return true;

        if (!m_ptrCallCountConstraint->Check(m_callCount))
            // the requirement is not met yet.
            return true;

        // If the requirement has been met, can this entry accomadate one more call?
        return m_ptrCallCountConstraint->Check(m_callCount + 1);
    }

    bool MatchInstanceMethod(InstanceIdType instanceId, char const * pMethodName)
    {
        return (0 == strcmp(pMethodName, m_methodName)
                  && m_InstanceId == instanceId);
    }

    bool MatchMethodName(char const * pMethodName)
    {
        return (0 == strcmp(pMethodName, m_methodName));
    }

    bool MatchId(char const * p)
    {
        if (m_id != NULL && 0 == strcmp(p, m_id))
            return true;

        return false;
    }

    bool HasBeenCalled()
    {
        return m_callCount > 0;
    }

    bool HasMetCallTimesRequirements()
    {
        return m_ptrCallCountConstraint == NULL || m_ptrCallCountConstraint->Check(m_callCount);
    }

    void Invoke()
    {
        if (m_previousCallInGroup != NULL)
        {
            if (!m_previousCallInGroup->HasBeenCalled() ||
                !m_previousCallInGroup->HasMetCallTimesRequirements())
            {
                ReportErrorWithLineNumber("error: previous method has not been called enough times.");
            }
        }

        ++ m_callCount;
    }

    bool MatchInstanceId(InstanceIdType instanceId)
    {
        return instanceId == m_InstanceId;
    }

    InstanceIdType GetAfterInstanceId()
    {
        return m_afterInstance;
    }

    char const * GetAfterMethodName()
    {
        return m_afterMethod;
    }

    virtual void Verify()
    {
        if (inOrderedCallsGroup && m_callCount == 0)
        {
            // An expectation declared between BeginOrderedCalls()
            // and EndOrderedCalls() has to be invoked at least once.
            ReportErrorWithLineNumber("error: %s.%s is expected to be called at least once.", m_objectName, m_methodName);
        }

        if (m_ptrCallCountConstraint != NULL && !m_ptrCallCountConstraint->Check(m_callCount))
        {
            ReportErrorWithLineNumber("error: %s.%s was called %d time(s).", m_objectName, m_methodName, m_callCount);
        }
    }

    void ReportErrorWithLineNumber(_In_z_ __format_string const char * szFormat, ...)
    {
        va_list pArgList;
        va_start( pArgList, szFormat );
        char buffer[256];
        vsprintf_s(buffer, sizeof(buffer), szFormat, pArgList);
        va_end( pArgList );
        throw ::CxxMock::CMockObjectError(m_fileName, m_lineNumber, buffer);
    }

    void ReportArgumentError()
    {
        ReportErrorWithLineNumber("error: %s.%s was called with incorrect argument(s).", m_objectName, m_methodName);
    }

    void ReportCallCountError()
    {
        ReportErrorWithLineNumber("error: %s.%s cannot be called %d time(s).",
            m_objectName, m_methodName, m_callCount+1);
    }

    void ReportCallOrderError()
    {
        ReportErrorWithLineNumber("error: %s.%s was called without %s being called first.",
            m_objectName, m_methodName, m_afterMethod);
    }

    void ReportUndefinedReturnValue()
    {
        ReportErrorWithLineNumber("error: %s.%s was called but return value is not defined.", m_objectName, m_methodName);
    }

    char const * m_fileName;
    unsigned int m_lineNumber;
    char const * m_objectName;
    char const * m_methodName;
    char const * m_id;
    InstanceIdType m_afterInstance;
    char const * m_afterMethod;
    unsigned int m_callCount;
    TCallCountConstraintPtr m_ptrCallCountConstraint;
    InstanceIdType m_InstanceId;
    bool inOrderedCallsGroup;
    CMethodExpectationBase * m_previousCallInGroup;
};

class CMethodExpectationListType : public CSimpleArray<CMethodExpectationBase *>
{
public:
    void Verify(InstanceIdType instanceId)
    {
        for (int i=0; i<GetSize(); i++)
            if (GetItem(i)->MatchInstanceId(instanceId))
                GetItem(i)->Verify();
    }

    void Cleanup(InstanceIdType instanceId)
    {
        int i = 0;
        while(i != GetSize())
        {
            if (GetItem(i)->MatchInstanceId(instanceId))
            {
                delete (GetItem(i));
                RemoveAt(i);
            }
            else
                ++i;
        }
    }

    bool MethodHasBeenCalled(InstanceIdType callerInstanceId,
                             InstanceIdType afterInstanceId,
                             char const * methodName)
    {
        for (int i = 0; i != GetSize(); ++i)
        {
            if (GetItem(i)->HasBeenCalled())
            {
                // if afterInstanceId is 0, After() was called without instanceId
                if (afterInstanceId == 0)
                {
                    if (GetItem(i)->MatchId(methodName))
                        return true;

                    if (GetItem(i)->MatchInstanceId(callerInstanceId) &&
                        GetItem(i)->MatchMethodName(methodName))
                        return true;
                }
                else if (GetItem(i)->MatchInstanceId(afterInstanceId) &&
                    (GetItem(i)->MatchMethodName(methodName)) || (GetItem(i))->MatchId(methodName))
                    return true;
            }
        }

        return false;
    }

    template <typename ExpectationType, typename TList>
    ExpectationType* MatchExpectation(InstanceIdType instanceId,
                                      char const * pMethodName,
                                      ValueList<TList> &args)
    {
        for (int i = 0; i < GetSize(); ++i)
        {
            if (!(GetItem(i)->MatchInstanceMethod(instanceId, pMethodName)))
                continue;

            ExpectationType *pExpectation = static_cast<ExpectationType*>(GetItem(i));

            if (!pExpectation->CanMatchMoreCalls())
                continue;

            if (!pExpectation->MatchArguments(args))
                continue;

            if (pExpectation->GetAfterMethodName() != NULL &&
                !MethodHasBeenCalled(instanceId,
                                     pExpectation->GetAfterInstanceId(),
                                     pExpectation->GetAfterMethodName()))
                continue;

            return pExpectation;
        }

        return NULL;
    }

    template <typename ExpectationType, typename TList>
    void ReportUnexpectedCall(InstanceIdType instanceId,
                              char const * pMethodName,
                              ValueList<TList> &args)
    {
        ExpectationType *pBestMatch = NULL;
        MatchPoints bestMatch = {0};

        for (int i = 0; i < GetSize(); ++i)
        {
            if (!(GetItem(i)->MatchInstanceMethod(instanceId, pMethodName)))
                continue;

            ExpectationType *pExpectation = static_cast<ExpectationType*>(GetItem(i));

            MatchPoints points = {0};

            if (pExpectation->HasCallCountConstraint())
                points.callCount = pExpectation->CanMatchMoreCalls() ? 1 : -1;

            if (pExpectation->HasArgumentConstraint())
                points.arguments = pExpectation->MatchArguments(args) ? 1 : -1;

            if (pExpectation->GetAfterMethodName() != NULL)
            {
                points.callOrder =  MethodHasBeenCalled(instanceId,
                                        pExpectation->GetAfterInstanceId(),
                                        pExpectation->GetAfterMethodName())
                                    ? 1 : -1;
            }

            if (pBestMatch == NULL || points.GetTotal() > bestMatch.GetTotal())
            {
                pBestMatch = pExpectation;
                bestMatch = points;
            }
        }

        if (pBestMatch != NULL)
        {
            if (bestMatch.arguments == -1)
                pBestMatch->ReportArgumentError();

            if (bestMatch.callCount == -1)
                pBestMatch->ReportCallCountError();

            if (bestMatch.callOrder == -1)
                pBestMatch->ReportCallOrderError();
        }

        ReportError("Error: Unexpected call: %s.", pMethodName);
    }

private:
    struct MatchPoints
    {
        // -1: doesn't match; 0: doesn't care; 1: match
        int arguments;
        int callOrder;
        int callCount;
        int GetTotal()
        {
            int totalConstraints = 0;
            int totalMatches = 0;

            if (arguments != 0) totalConstraints += 2;
            if (arguments == 1) totalMatches += 2;
            if (callOrder != 0) totalConstraints ++;
            if (callOrder == 1) totalMatches ++;
            if (callCount != 0) totalConstraints ++;
            if (callCount == 1) totalMatches ++;

            return totalConstraints == 0 ? 100 : totalMatches * 100 / totalConstraints;
        }
    };
};

__declspec(selectany) CMethodExpectationListType MethodExpectationList;

template <typename T>
class CThrowException : public ICommand
{
public:
    CThrowException(T const & ex) : m_ex(ex)
    {
    }

    void Exec()
    {
        throw m_ex;
    };

private:
    T m_ex;
};

__if_exists(DebugBreak)
{
class CBreakIntoDebugger : public ICommand
{
public:
    void Exec()
    {
        DebugBreak();
    }
};
} // __if_exists(DebugBreak)

__if_exists(SetLastError)
{
class CSetLastError : public ICommand
{
public:
    CSetLastError(DWORD error) : m_error(error)
    {
    }

    void Exec()
    {
        SetLastError(m_error);
    };

private:
    DWORD m_error;
};
} // __if_exists(SetLastError)

template <typename T>
class CSingleValueAssigner : public CValueAssigner<T *>, public CValueAssigner<T &>
{
public:
    CSingleValueAssigner(T v)
        : m_v(v)
    {
    }

    void Assign(T * p)
    {
        *p = m_v;
    }

    void Assign(T & p)
    {
        p = m_v;
    }

private:
    T m_v;
};

template <typename T>
class CValueArrayAssigner : public CValueAssigner<T *>
{
public:
    CValueArrayAssigner(T * p, unsigned int n)
        : m_p(p), m_n(n)
    {
    }

// warning 22103: The buffer 'p' is being written inside a loop.
// The loop execution should be controlled using some related buffer size expression.
// The analysis cannot identify any related buffer size expression.
// Please consider adding a SAL buffer size annotation to 'p'.

// Unfortunately there is not enough information at this point to annotate the pointer.
    void Assign(T * p)
    {
        for (unsigned int i = 0; i < m_n; i++)
#pragma warning (suppress : 22103)
            p[i] = m_p[i];
    }

private:
    T * m_p;
    unsigned int m_n;
};

template <typename Struct, typename T>
class CFieldValueAssigner : public CValueAssigner<Struct *>, public CValueAssigner<Struct &>
{
public:
    CFieldValueAssigner(T Struct::* p, T v)
        : m_pField(p), m_value(v)
    {
    }

    void Assign(Struct * pStruct)
    {
        pStruct->*m_pField = m_value;
    }

    void Assign(Struct & Struct)
    {
        Struct.*m_pField = m_value;
    }

private:
    T Struct::* m_pField;
    T m_value;
};

template<typename ReturnType, typename ArgumentTypeList>
class CMethodExpectation
    : public IMethodExpectation<ReturnType, ArgumentTypeList>
    , public CMethodExpectationBase
    , public CArgumentConstraints<ArgumentTypeList>
    , public CReturnValues<ReturnType>
    , public COutValues
    , public IAfter
{
    typedef IMethodExpectation<ReturnType, ArgumentTypeList> ThisType;

public:
    CMethodExpectation(InstanceIdType instanceId,
                       char const * objectName,
                       char const * methodName,
                       char const * fileName,
                       unsigned lineNumber)
        : CMethodExpectationBase(instanceId, fileName, lineNumber, objectName, methodName)
    {
    }

    ThisType & CallCount(TCallCountConstraintPtr const & cons)
    {
        m_ptrCallCountConstraint = cons;
        return *this;
    }

    ThisType & Id(char const *p)
    {
        m_id = p;
        return *this;
    }

    void AfterMethod(char const *p)
    {
        m_afterInstance = m_InstanceId;
        m_afterMethod = p;
    }

    void AfterInstanceMethod(InstanceIdType instance, char const *p)
    {
        m_afterInstance = instance;
        m_afterMethod = p;
    }

    ThisType & Will(ICommand* command)
    {
        m_commands.Add(command);
        return *this;
    }

    void Invoke(ValueList<ArgumentTypeList> & arg)
    {
        __super::Invoke();

        for (int i = 0; i < m_commands.GetSize(); ++i)
            m_commands[i]->Exec();

        AssignOutValues(0, arg);
    }

    ~CMethodExpectation()
    {
        for (int i = 0; i < m_commands.GetSize(); ++i)
            delete m_commands[i];
    }

private:
    CSimpleArray<ICommand*> m_commands;
};

inline InstanceIdType GetNewInstanceId()
{
    static InstanceIdType nextId = 0;
    return ++nextId;
};

class CMockClassBase
{
public:
    CMockClassBase()
        : m_verified(false)
        , m_inMethodCall(false)
    {
        m_InstanceId = GetNewInstanceId();
    }

    ~CMockClassBase()
    {
        __try
        {
            // If a method call throws exception (e.g. undefined return value),
            // then the object will be destructed during stack unwind.
            // Do not attempt to verify, another exception will kill the process.
            if (!m_inMethodCall)
                VerifyExpectations();
        }
        __finally
        {
            MethodExpectationList.Cleanup(m_InstanceId);
        }
    }

    InstanceIdType GetInstanceId() const
    {
        return m_InstanceId;
    }

    void VerifyExpectations()
    {
        if (!m_verified)
        {
            m_verified = true;
            MethodExpectationList.Verify(m_InstanceId);
        }
    }

    void SetInMethodCall(bool b) const
    {
        m_inMethodCall = b;
    }

private:
    InstanceIdType m_InstanceId;
    bool m_verified;
    mutable bool m_inMethodCall;
};

} //namespace Internal

template <typename T>
CObjectPtr< CConstraint<typename DeepConstCast<T>::Result> >
eq(T value)
{
    return new Internal::CEqual<DeepConstCast<T>::Result>(value);
}

template<typename T>
CObjectPtr< CConstraint<typename DeepConstCast<T>::Result> >
ne(T value)
{
    return new Internal::CNotEqual<DeepConstCast<T>::Result>(value);
}

template <typename T>
CObjectPtr< CConstraint<typename DeepConstCast<T>::Result> >
gt(T value)
{
    return new Internal::CGreaterThan<DeepConstCast<T>::Result>(value);
}

template <typename T>
CObjectPtr< CConstraint<typename DeepConstCast<T>::Result> >
ge(T value)
{
    return new Internal::CGreaterThanOrEqualTo<DeepConstCast<T>::Result>(value);
}

template <typename T>
CObjectPtr< CConstraint<typename DeepConstCast<T>::Result> >
lt(T value)
{
    return new Internal::CLessThan<DeepConstCast<T>::Result>(value);
}

template <typename T>
CObjectPtr< CConstraint<typename DeepConstCast<T>::Result> >
le(T value)
{
    return new Internal::CLessThanOrEqualTo<DeepConstCast<T>::Result>(value);
}

inline CObjectPtr< CConstraint<const char * const> >
StringContains(const char * const arg)
{
    return new Internal::CStringContains<const char * const>(arg);
}

inline CObjectPtr< CConstraint<const wchar_t * const> >
StringContains(const wchar_t * const arg)
{
    return new Internal::CStringContains<const wchar_t * const>(arg);
}

inline CObjectPtr< CConstraint<const char * const> >
StringEquals(const char * const arg)
{
    return new Internal::CStringEquals<const char * const>(arg, true);
}

inline CObjectPtr< CConstraint<const wchar_t * const> >
StringEquals(const wchar_t * const arg)
{
    return new Internal::CStringEquals<const wchar_t * const>(arg, true);
}

inline CObjectPtr< CConstraint<const char * const> >
StringEqualsI(const char * const arg)
{
    return new Internal::CStringEquals<const char * const>(arg, false);
}

inline CObjectPtr< CConstraint<const wchar_t * const> >
StringEqualsI(const wchar_t * const arg)
{
    return new Internal::CStringEquals<const wchar_t * const>(arg, false);
}

// T could be CConstraint<> or derived from CConstraint<>
template <typename T>
CObjectPtr< CConstraint<typename T::ConstrainedType const * const> >
Deref(CObjectPtr<T> cns)
{
    return new Internal::CDeref<T::ConstrainedType>(cns);
}

template <typename SmartPointerType, typename ConstraintType>
CObjectPtr< CConstraint< SmartPointerType > >
SPDeref(CObjectPtr<ConstraintType> cns)
{
    return new Internal::CSPDeref<SmartPointerType, ConstraintType>(cns);
}

template <typename T>
CObjectPtr< CConstraint<typename T::ConstrainedType const * const> >
operator*(CObjectPtr<T> cns)
{
    return new Internal::CDeref<T::ConstrainedType>(cns);
}

template <typename StructType, typename FieldType>
CObjectPtr< CConstraint<const StructType> >
Field(FieldType StructType::* pField,
      CObjectPtr< CConstraint<typename DeepConstCast<FieldType>::Result> > pConstraint)
{
    return new Internal::CFieldAccess<const StructType, typename DeepConstCast<FieldType>::Result>(
            pField, pConstraint);
}

// Interface (or any abstract classes) cannot be passed by value.
// GetProperty returns a constraint on reference type (TClass const &)
// The code that tries to match argument with constraint always try both
// reference and non-reference types.
template <typename TClass, typename TReturn>
CObjectPtr< CConstraint<TClass const &> >
GetProperty(TReturn (TClass::*pMethod)(void) const,
              CObjectPtr< CConstraint<TReturn const> > const & exp)
{
    return new Internal::CGetProperty<TClass const, TReturn>(pMethod, exp);
}

//template <typename T1, typename T2>
//CObjectPtr< CConstraint <const T1> >
//DynamicCast(CObjectPtr< CConstraint <const T2> > exp)
//{
//    return new Internal::CDynamicCast<const T1, const T2>(exp);
//}
//
//template <typename T1, typename T2>
//CObjectPtr< CConstraint <const T1> >
//StaticCast(CObjectPtr< CConstraint <const T2> > exp)
//{
//    return new Internal::CStaticCast<const T1, const T2>(exp);
//}
//
//template <typename T1, typename T2>
//CObjectPtr< CConstraint <const T1> >
//ReinterpretCast(CObjectPtr< CConstraint <const T2> > exp)
//{
//    return new Internal::CReinterpretCast<const T1, const T2>(exp);
//}
//
#pragma warning (pop)

template <typename T>
inline Internal::CThrowException<T>* ThrowException(T const & ex)
{
    return new Internal::CThrowException<T>(ex);
}

__if_exists(Internal::CBreakIntoDebugger)
{
inline Internal::CBreakIntoDebugger * BreakIntoDebugger()
{
    return new Internal::CBreakIntoDebugger;
}
} // __if_exists(Internal::CBreakIntoDebugger)

__if_exists(SetLastError)
{
inline Internal::CSetLastError * SetLastErrorCode(DWORD dwError)
{
    return new Internal::CSetLastError(dwError);
}
}

inline void BeginOrderedCalls()
{
    Internal::COrderedCallsManager::BeginOrderedCalls();
}

inline void EndOrderedCalls()
{
    Internal::COrderedCallsManager::EndOrderedCalls();
}

template <typename T>
void Verify(T& v)
{
    v.zzbase_.VerifyExpectations();
}

#define INITIALIZERS_0       {}
#define INITIALIZERS_1(...)  {arg1}
#define INITIALIZERS_2(...)  {arg1, arg2}
#define INITIALIZERS_3(...)  {arg1, arg2, arg3}
#define INITIALIZERS_4(...)  {arg1, arg2, arg3, arg4}
#define INITIALIZERS_5(...)  {arg1, arg2, arg3, arg4, arg5}
#define INITIALIZERS_6(...)  {arg1, arg2, arg3, arg4, arg5, arg6}
#define INITIALIZERS_7(...)  {arg1, arg2, arg3, arg4, arg5, arg6, arg7}
#define INITIALIZERS_8(...)  {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8}
#define INITIALIZERS_9(...)  {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9}
#define INITIALIZERS_10(...) {arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10}

#define LIST_ARGS_0
#define LIST_ARGS_1(T) T arg1
#define LIST_ARGS_2(T1, T2) T1 arg1, T2 arg2
#define LIST_ARGS_3(T1, T2, T3) T1 arg1, T2 arg2, T3 arg3
#define LIST_ARGS_4(T1, T2, T3, T4) T1 arg1, T2 arg2, T3 arg3, T4 arg4
#define LIST_ARGS_5(T1, T2, T3, T4, T5) T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5
#define LIST_ARGS_6(T1, T2, T3, T4, T5, T6) T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6
#define LIST_ARGS_7(T1, T2, T3, T4, T5, T6, T7) T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7
#define LIST_ARGS_8(T1, T2, T3, T4, T5, T6, T7, T8) T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8
#define LIST_ARGS_9(T1, T2, T3, T4, T5, T6, T7, T8, T9) T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9
#define LIST_ARGS_10(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8, T9 arg9, T10 arg10

// warning C4229: anachronism used : modifiers on data are ignored

#define STUB_METHOD(ReturnType, MethodName, Arg, OverrideSpecifiers) \
typedef CM_TYPELIST_##Arg MethodName##ArgsType; \
__pragma(warning (push)); \
__pragma(warning (disable : 4229)); \
typedef ::CxxMock::Internal::CMethodExpectation< ReturnType, MethodName##ArgsType > MethodName##ExpectationType; \
typedef ::CxxMock::Internal::IMethodExpectation< ReturnType, MethodName##ArgsType > MethodName##ExpectationInterface; \
ReturnType MethodName (LIST_ARGS_##Arg) OverrideSpecifiers \
{ \
    ::CxxMock::Internal::ValueList< MethodName##ArgsType > args = INITIALIZERS_##Arg ; \
    zzbase_.SetInMethodCall(true); \
    MethodName##ExpectationType *pExpectation = \
        ::CxxMock::Internal::MethodExpectationList.MatchExpectation< MethodName##ExpectationType, MethodName##ArgsType >(zzbase_.GetInstanceId(), #MethodName, args); \
    if (pExpectation == NULL) \
        ::CxxMock::Internal::MethodExpectationList.ReportUnexpectedCall< MethodName##ExpectationType, MethodName##ArgsType >(zzbase_.GetInstanceId(), #MethodName, args); \
    pExpectation->Invoke(args); \
    __if_exists(MethodName##ExpectationType::GetNextReturnValue) \
    { \
        ReturnType returnValue = pExpectation->GetNextReturnValue(); \
        zzbase_.SetInMethodCall(false); \
        return  returnValue; \
    } \
    __if_not_exists(MethodName##ExpectationType::GetNextReturnValue) \
    { \
        zzbase_.SetInMethodCall(false); \
        return; \
    } \
} \
__pragma(warning (pop)); \
MethodName##ExpectationInterface& zzExpect##MethodName(char const * objectName, char const * sourceFile, unsigned int sourceLine) \
{ \
    MethodName##ExpectationType* pExpectation = new MethodName##ExpectationType(zzbase_.GetInstanceId(), objectName, #MethodName, sourceFile, sourceLine); \
    ::CxxMock::Internal::MethodExpectationList.Add(pExpectation); \
    return *pExpectation; \
}

#define STUB_STDMETHOD(ReturnType, MethodName, Arg, OverrideSpecifiers) \
    STUB_METHOD(ReturnType __stdcall, MethodName, Arg, OverrideSpecifiers)

// warning C4003: not enough actual parameters for macro 'STUB_METHOD'

#define MOCK_CLASS(Derived, Base) \
class Derived : public Base \
{ \
public: \
__pragma(warning (push)); \
__pragma(warning (disable : 4003));


#define END_MOCK \
::CxxMock::Internal::CMockClassBase zzbase_; }; \
__pragma(warning (pop));


#define CXXMOCK_EXPECT(object, method) (object).zzExpect##method(#object, __FILE__, __LINE__)

#ifndef Expect
#define Expect CXXMOCK_EXPECT
#endif

} //namspace CxxMock

