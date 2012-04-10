#ifndef MAY_BE_HEADER  
#define MAY_BE_HEADER  

#include <string.h>
#include <memory> 
#include <assert.h>

///��� ����, ��� �� ������� ���� ������� �� ������� ������ ��� ��������� ��������, ��������
/** \code
MayBe<Point> f() {
  return MayBeEmpty;
}
\endcode */
enum TMayBeEmpty { MayBeEmpty };
enum TMayBeDefault { MayBeDefault }; 

///����� ��� ���������� �������, ������� ��� ����� ���� ������, ��� � ���.
///������������ ����������� �� ���������, ����������� �����������, ������������, 
///�������������� ���������� � �����������, ���� ��� ������. ������ ������ ����� � ����.
///������������ - ����� ������� ��������� T *m_object; � ���� m_object == NULL - �� ������� ��� ������� ���.
///�� ��� �� ������ ������� ��������� ���������� ������� �� ���� ��� ������ operator new.
///����� ������ ������� ������ ����� "bIsInitialized", �� ����� �������� ��� ������ 
///�������������/�����������/���������� ������ ���� ���.
///����� ������� ������ - boost::optional ( http://boost.org/libs/optional/doc/optional.html )
///���������� �� boost::optional ����� ������� ������������� ��� ������� ����������� ����� � ����������� � ������������, 
///� �������� ����� ������� ��� "�������" �����, ����� int/string � ��, ��� ��� � boost::optional ���� ������� ��������������
///�� T � optional<T>.
/**
������ �������������:
\code

//�������������:
MayBe<Point> point;
if (���� ������� �����)
  MAYBE_INIT(point, Point(1, 2));

//������������� �� ���������:
if (���� ������� �����)
  point.ResetDefault();
  
//������������� ������������ ������������� �������:
Point existing;
MayBe<Point> point = CreateMayBe(existing);
  
//�������������:

if (Point *p = point.Get())
    std::cout << p->x;

if (point)
    std::cout << point->x;
    
\endcode
*/
template <class T>
class MayBe
{
public:
    ///��������� ������� ���������, ���� MayBe �� �������� �������.
    T *Get() { return m_initialized ? GetRaw() : NULL; }
    const T *Get() const { return m_initialized ? GetRaw() : NULL; }

    T *operator->() { assert(m_initialized); return GetRaw(); }
    const T *operator->() const { assert(m_initialized); return GetRaw(); }

    T &operator*() { assert(m_initialized); return *GetRaw(); }
    const T &operator*() const { assert(m_initialized); return *GetRaw(); }

    ///���� ����� ������ ��� ����, ��� �� ����� ���� ���������� ���������� � ��������� � �� ���� ambiguity ����� Get() � Get()const
    T *Get2() { return Get(); }

    MayBe() : m_initialized(0) { }
    
    MayBe(TMayBeDefault) //���������� �� explicit, ��� �� ����� ���� ������ ������� �� MayBeDefault 
    {
        new (GetRaw()) T();
        m_initialized = true;
    }

    
    MayBe(TMayBeEmpty): m_initialized(0) {} //���������� �� explicit �� MayBeEmpty

    explicit MayBe(const T &x)
    {
        new (GetRaw()) T(x);
        m_initialized = true;
    }

    MayBe(const MayBe &other)
    {
        m_initialized = other.m_initialized;
        if (m_initialized)
            new (this->GetRaw()) T (*other.GetRaw());
    }

    MayBe &operator=(const MayBe &other) 
    {
        //basic exception safety guarantee. 
        Reset();
        bool initialized = other.m_initialized;
        if (initialized)
            new (GetRaw())T(*other.GetRaw());
        m_initialized = initialized;
        return *this;
    }

    ///��������������� � ����������� ������������� ��-���������. 
    void ResetDefault()
    {
        Reset();
        new (GetRaw()) T();
        m_initialized = true;
    }

    void Reset(const T &x) 
    {
        Reset();
        new (GetRaw()) T(x);
        m_initialized = true;
    }


    ///���������������. 
    void Reset() 
    {
        if (T *p = Get())
        {
            p->~T();
            m_initialized = false;
            assert(memset(&m_storage, 0xDD, sizeof(m_storage)));
        }
    }

    ~MayBe()
    {
        if (T *p = Get())
            p->~T();
#ifndef NDEBUG
        Get2(); //����� ����� �� ��������������, � ���������� ��� ������� �� ���������.

        m_initialized = 0;
        memset(&m_storage, 0xDD, sizeof(m_storage));
#endif
    }

private:
    //������ ������� ���������� � bool - �������� ������, ��� ��� �������� ����������� �������������� � �������������
    //�������� � MayBe � ������� ������, ������� ���������� � bool. 
    //������� - �������� TSafeBoolGen, ������� ���������� ������������ �� �� ����������.
    struct TSafeBoolGen { int non_null; };
    typedef int TSafeBoolGen::*TSafeBool;
public:
    operator TSafeBool() const { return m_initialized ? &TSafeBoolGen::non_null : NULL; }
    

private:
    union TStorage
    {
        double used_for_proper_align_only;
        char buf[sizeof(T)];
    } m_storage;

    int m_initialized;

    T *GetRaw() { return reinterpret_cast<T*>(m_storage.buf); }
    const T *GetRaw() const { return reinterpret_cast<const T*>(m_storage.buf); }

public:
    void *MethodFor_MAYBE_INIT_Macro_GetInternalBufer() { return m_storage.buf; }
    int &MethodFor_MAYBE_INIT_Macro_GetInternalBool() { return m_initialized; }
    //���� ������������ ���� �� ��� ���, �� ��������� �� ��������� �����, � private, ��� ������� ������ ����������.
    void MethodFor_MAYBE_INIT_Macro_EnsureTheSameType(T *) {}
private:
    template <class U>
    int &MethodFor_MAYBE_INIT_Macro_EnsureTheSameType(const volatile U &);
};

template <class T> MayBe<T> CreateMayBe(const T &x) { return MayBe<T>(x); }

#define MAYBE_INIT(VAR, TYPE_PARAMS) \
((VAR).Reset(), \
 (VAR).MethodFor_MAYBE_INIT_Macro_EnsureTheSameType(  \
 new ((VAR).MethodFor_MAYBE_INIT_Macro_GetInternalBufer()) TYPE_PARAMS),     \
 (VAR).MethodFor_MAYBE_INIT_Macro_GetInternalBool() = true)            \

/* ��� ��� ���������� � ���� �����:
#define MAYBE_INIT(VAR, TYPE_PARAMS) 

VAR.Reset(), 
VAR.EnsureTheSameType(new (m_storage.buf) TYPE_PARAMS)),
VAR.m_initialized = true

*/ 

#endif //MAY_BE_HEADER
