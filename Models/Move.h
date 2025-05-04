#pragma once
#include <stdlib.h>

typedef int8_t POS_T;

struct move_pos
{
    // Координаты: откуда ходим (x, y), куда ходим (x2, y2), и координаты побитой фигуры (xb, yb)
    POS_T x, y;             // from
    POS_T x2, y2;           // to
    POS_T xb = -1, yb = -1; // beaten
    //присваивание одних переменных - другим, просто ход, без съедания других фишек
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2) : x(x), y(y), x2(x2), y2(y2)
    {
    }
    //присваивание одних переменных - другим, просто ход, с съеданием других фишек
    move_pos(const POS_T x, const POS_T y, const POS_T x2, const POS_T y2, const POS_T xb, const POS_T yb)
        : x(x), y(y), x2(x2), y2(y2), xb(xb), yb(yb)
    {
    }

    //переопределяем оператор сравнения ==
    bool operator==(const move_pos &other) const
    {
        return (x == other.x && y == other.y && x2 == other.x2 && y2 == other.y2);
    }
    //переопределяем оператор сравнения !=
    bool operator!=(const move_pos &other) const
    {
        return !(*this == other);
    }
};
