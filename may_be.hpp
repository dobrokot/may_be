#ifndef MAY_BE_HEADER  
#define MAY_BE_HEADER  

#include <string.h>
#include <memory> 
#include <assert.h>

//------------------------ utility class for internal purposes ----------------------
template<size_t> struct TAlignSwitch;
template<> struct TAlignSwitch<1> { typedef char   type; };
template<> struct TAlignSwitch<2> { typedef short  type; };
template<> struct TAlignSwitch<4> { typedef float  type; };
template<> struct TAlignSwitch<8> { typedef double type; };

template <size_t i> struct TAlign {
    enum { align = i%2 ? 1 : (i%4 ? 2 : (i%8 ? 4 : 8)) };
    typedef typename TAlignSwitch<align>::type type;  
};

//------------------------ documentation and examples follow ----------------------

///для того, что бы удобней было вернуть из функции пустое или дефолтное значение, например
/** \code
MayBe<Point> f() {
  return MayBeEmpty;
}
\endcode */
enum TMayBeEmpty { MayBeEmpty };
enum TMayBeDefault { MayBeDefault }; 


///класс для размещения объекта, который как может быть создан, так и нет.
///обеспечивает конструктор по умолчанию, конструктор копирования, присваивание, 
///автоматическое разрушение в деструкторе, если был создан. Хранит объект прямо в себе.
///альтернатива - можно завести указатель T *m_object; и если m_object == NULL - то считать что объекта нет.
///но это во многих случаях потребует размещения объекта на хипе при помощи operator new.
///можно самому завести флажок вроде "bIsInitialized", но можно написать эту логику 
///инициализации/копирования/разрушения только один раз.
///очень близкий аналог - boost::optional ( http://boost.org/libs/optional/doc/optional.html )
///отличается от boost::optional более удобной инициализаций для сложных некопирумых типов с параметрами в конструкторе, 
///и наоборот менее удобной для "простых" типов, вроде int/string и тп, так как у boost::optional есть неявное преобразования
///из T в optional<T>.
/**
Пример использования:
\code

//Инициализация:
MayBe<Point> point;
if (надо создать точку)
  MAYBE_INIT(point, Point(1, 2));

//Инициализация по умолчанию:
if (надо создать точку)
  point.ResetDefault();
  
//Инициализация копированием существующего объекта:
Point existing;
MayBe<Point> point = CreateMayBe(existing);
  
//использование:

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
    ///возращает нулевой указатель, если MayBe не содержит объекта.
    T *Get() { return m_initialized ? GetRaw() : NULL; }
    const T *Get() const { return m_initialized ? GetRaw() : NULL; }

    T *operator->() { assert(m_initialized); return GetRaw(); }
    const T *operator->() const { assert(m_initialized); return GetRaw(); }

    T &operator*() { assert(m_initialized); return *GetRaw(); }
    const T &operator*() const { assert(m_initialized); return *GetRaw(); }

    ///этот метод просто для того, что бы можно было посмотреть содержимое в отладчике и не было ambiguity между Get() и Get()const
    T *Get2() { return Get(); }

    MayBe() : m_initialized(false) { }
    
    MayBe(TMayBeDefault) //специально не explicit, что бы можно было неявно создать из MayBeDefault 
    {
        new (GetRaw()) T();
        m_initialized = true;
    }

    
    MayBe(TMayBeEmpty): m_initialized(0) {} //специально не explicit из MayBeEmpty

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
        if (m_initialized && other.m_initialized)
            *(*this) = *other;
        else if (m_initialized && !other.m_initialized)
            Reset();
        else if (!m_initialized && other.m_initialized) {
            new (GetRaw()) T(*other);
            m_initialized = true;
        }
        return *this;
    }

    ///Деинициализация и последующая инициализация по-умолчанию. 
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


    ///Деинициализация. 
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
        Get2(); //иначе метод не инстанцируется, и невозможно его вызвать из отладчика.

        m_initialized = false;
        memset(&m_storage, 0xDD, sizeof(m_storage));
#endif
    }

private:
    //делать неявное приведение к bool - довольно опасно, так как позволит неожиданные арифметические и сравнительные
    //операции с MayBe и другими типами, которые приводятся к bool. 
    //Поэтому - пушистый TSafeBoolGen, который невозможно использовать по не назначению.
    struct TSafeBoolGen { int non_null; };
    typedef int TSafeBoolGen::*TSafeBool;
public:
    operator TSafeBool() const { return m_initialized ? &TSafeBoolGen::non_null : NULL; }
    

private:
    union TStorage
    {
        typename TAlign<sizeof(T)>::type used_for_proper_align_only;
        char buf[sizeof(T)];
    } m_storage;

    bool m_initialized;

    T *GetRaw() { return reinterpret_cast<T*>(m_storage.buf); }
    const T *GetRaw() const { return reinterpret_cast<const T*>(m_storage.buf); }

public:
    //internal methods, used by macro MAYBE_INIT
    void *MAYBE_INIT_storage_buf() { return m_storage.buf; }
    bool &MAYBE_INIT_initialized() { return m_initialized; }
    //если пользователь даст не тот тип, то вызовется не публичный метод, а private, что вызовет ошибку компиляции.
    void MAYBE_INIT_EnsureTheSameType(T *) {}
private:
    template <class U>
    bool &MAYBE_INIT_EnsureTheSameType(const volatile U &);
};

template <class T> MayBe<T> CreateMayBe(const T &x) { return MayBe<T>(x); }

#define MAYBE_INIT(VAR, TYPE_PARAMS) \
((VAR).Reset(), \
 (VAR).MAYBE_INIT_EnsureTheSameType(  \
 new ((VAR).MAYBE_INIT_storage_buf()) TYPE_PARAMS),     \
 (VAR).MAYBE_INIT_initialized() = true)            \


#endif //MAY_BE_HEADER
