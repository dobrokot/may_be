
#include "may_be.hpp"
#include <iostream>

struct Point { //тип,  над которым мы будем эксперементировать.
    float x, y;
    Point(float x, float y): x(x), y(y) {}
    Point(): x(0), y(0) {}
};

MayBe<Point> f1() {
  return MayBeEmpty; //вернуть пустоту.
}

MayBe<Point> f2() {
  return MayBeDefault; //вернуть объект, инициализированный по умолчанию
}

MayBe<int> f3() {
  return CreateMayBe(7); //вернуть конкретное значение.
}


int main() {
    {
        MayBe<Point> point;
        //инициализация с параметрами.
        if (true)
          MAYBE_INIT(point, Point(1, 2));
    }

    {
        MayBe<Point> point;
        //Инициализация по умолчанию:
        if (true)
          point.ResetDefault();
    } 

    //Инициализация копированием существующего объекта:
    Point existing(3,4);
    MayBe<Point> point = CreateMayBe(existing);
      
    //использование:
    if (Point *p = point.Get()) //получить указатель, возможно NULL
        std::cout << p->x;

    if (point)
        std::cout << point->x;
    std::cout << std::endl;
}

