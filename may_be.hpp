#ifndef MAY_BE_HEADER  
#define MAY_BE_HEADER  

#include <string.h>
#include <memory> 
#include <assert.h>

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

    MayBe() : m_initialized(0) { }
    
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
        //basic exception safety guarantee. 
        Reset();
        bool initialized = other.m_initialized;
        if (initialized)
            new (GetRaw())T(*other.GetRaw());
        m_initialized = initialized;
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

        m_initialized = 0;
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
        double used_for_proper_align_only;
        char buf[sizeof(T)];
    } m_storage;

    int m_initialized;

    T *GetRaw() { return reinterpret_cast<T*>(m_storage.buf); }
    const T *GetRaw() const { return reinterpret_cast<const T*>(m_storage.buf); }

public:
    void *MethodFor_MAYBE_INIT_Macro_GetInternalBufer() { return m_storage.buf; }
    int &MethodFor_MAYBE_INIT_Macro_GetInternalBool() { return m_initialized; }
    //если пользователь даст не тот тип, то вызовется не публичный метод, а private, что вызовет ошибку компиляции.
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

/* вот что происходит в этом макро:
#define MAYBE_INIT(VAR, TYPE_PARAMS) 

VAR.Reset(), 
VAR.EnsureTheSameType(new (m_storage.buf) TYPE_PARAMS)),
VAR.m_initialized = true

*/ 

#endif //MAY_BE_HEADER
