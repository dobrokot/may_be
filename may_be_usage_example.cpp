
#include "may_be.hpp"
#include <iostream>

struct Point { //���,  ��� ������� �� ����� ������������������.
    float x, y;
    Point(float x, float y): x(x), y(y) {}
    Point(): x(0), y(0) {}
};

MayBe<Point> f1() {
  return MayBeEmpty; //������� �������.
}

MayBe<Point> f2() {
  return MayBeDefault; //������� ������, ������������������ �� ���������
}

MayBe<int> f3() {
  return CreateMayBe(7); //������� ���������� ��������.
}


int main() {
    {
        MayBe<Point> point;
        //������������� � �����������.
        if (true)
          MAYBE_INIT(point, Point(1, 2));
    }

    {
        MayBe<Point> point;
        //������������� �� ���������:
        if (true)
          point.ResetDefault();
    } 

    //������������� ������������ ������������� �������:
    Point existing(3,4);
    MayBe<Point> point = CreateMayBe(existing);
      
    //�������������:
    if (Point *p = point.Get()) //�������� ���������, �������� NULL
        std::cout << p->x;

    if (point)
        std::cout << point->x;
    std::cout << std::endl;
}

