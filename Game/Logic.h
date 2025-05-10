﻿#pragma once
#include <random>
#include <vector>

#include "../Models/Move.h"
#include "Board.h"
#include "Config.h"

const int INF = 1e9;

class Logic
{
  public:
    Logic(Board *board, Config *config):board(board), config(config)
    {
        rand_eng = std::default_random_engine (
            !((*config)("Bot", "NoRandom")) ? unsigned(time(0)) : 0);
        scoring_mode = (*config)("Bot", "BotScoringType");
        optimization = (*config)("Bot", "Optimization");
    }

    vector<move_pos> find_best_turns(const bool color)       
        {
        next_best_state.clear();
        next_move.clear();

        find_first_best_turn(board->get_board(), color, -1, -1, 0);

        int cur_state = 0;
        vector<move_pos> res;
        do
        {
            res.push_back(next_move[cur_state]);
            cur_state = next_best_state[cur_state];
        } while (cur_state != -1 && next_move[cur_state].x != -1);
        return res;
    }

private:
    vector<vector<POS_T>> make_turn(vector<vector<POS_T>> mtx, move_pos turn) const
    {
        if (turn.xb != -1)
            mtx[turn.xb][turn.yb] = 0;
        if ((mtx[turn.x][turn.y] == 1 && turn.x2 == 0) || (mtx[turn.x][turn.y] == 2 && turn.x2 == 7))
            mtx[turn.x][turn.y] += 2;
        mtx[turn.x2][turn.y2] = mtx[turn.x][turn.y];
        mtx[turn.x][turn.y] = 0;
        return mtx;
    }


    double calc_score(const vector<vector<POS_T>>& mtx, const bool first_bot_color) const
    {
        // Цель: вычислить оценку текущей позиции на доске.
        // Чем выше значение — тем лучше позиция для "чёрных" относительно "белых".

        double w = 0, wq = 0, b = 0, bq = 0;

        // Проходим по всей доске и считаем:
        // 1, 3 - обычные белые и чёрные шашки
        // 2, 4 - дамки (королевы)
        for (POS_T i = 0; i < 8; ++i)
        {
            for (POS_T j = 0; j < 8; ++j)
            {
                w += (mtx[i][j] == 1);  // Белые простые фишки
                wq += (mtx[i][j] == 3); // Белые дамки
                b += (mtx[i][j] == 2);  // Чёрные простые фишки
                bq += (mtx[i][j] == 4); // Чёрные дамки

                // Если режим оценки учитывает потенциал (ближе к превращению),
                // добавляем небольшой бонус за позицию
                if (scoring_mode == "NumberAndPotential")
                {
                    w += 0.05 * (mtx[i][j] == 1) * (7 - i); // белые получают бонус за продвижение вверх
                    b += 0.05 * (mtx[i][j] == 2) * (i);     // чёрные получают бонус за продвижение вниз
                }
            }
        }

        // Если бот играет за белые, меняем местами счётчики,
        // чтобы всегда максимизировать игрока, за которого играет бот
        if (!first_bot_color)
        {
            swap(b, w);
            swap(bq, wq);
        }

        // Если у белых не осталось фигур возвращаем очень большое число
        if (w + wq == 0)
            return INF;

        // Если у чёрных не осталось фигур возвращаем 0
        if (b + bq == 0)
            return 0;

        // Коэффициент ценности дамки относительно простой фишки
        int q_coef = 4;
        if (scoring_mode == "NumberAndPotential")
        {
            q_coef = 5; // В режиме с учётом позиции дамкам даётся больший вес
        }

        // отношение силы чёрных к силе белых (чем больше — тем лучше для чёрных)
        return (b + bq * q_coef) / (w + wq * q_coef);
    }

    // только начинаем искать лучшие ходы.
    double find_first_best_turn(vector<vector<POS_T>> mtx, const bool color, const POS_T x, const POS_T y, size_t state,
        double alpha = -1)
    {
        next_best_state.push_back(-1);              
        next_move.emplace_back(-1, -1, -1, -1);
        double best_score = -1;
        if (state != 0)                             // получаем ходы
            find_turns(x, y, mtx); 
        auto turns_now = turns;
        bool have_beats_now = have_beats;

        if (!have_beats_now && state != 0)
        {
            return find_best_turns_rec(mtx, 1 - color, 0, alpha);
        }

        vector<move_pos> best_moves;
        vector<int> best_states;

        for (auto turn : turns_now)
        {
            size_t next_state = next_move.size();
            double score;
            if (have_beats_now) // если есть "побития, продолжаем бить"
            {
                score = find_first_best_turn(make_turn(mtx, turn), color, turn.x2, turn.y2, next_state, best_score);
            }
            else // если нет побитий, то просто делаем ход
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, 0, best_score);
            }
            if (score > best_score) // проверка результата 
            {                       //Здесь мы нашли новый оптимум 
                best_score = score;
                next_best_state[state] = (have_beats_now ? int(next_state) : -1);
                next_move[state] = turn;
            }
        }
        return best_score; 
    }

    // В самой функции мы ищем лучший ход с альфа бета отсечением 
    //Функция получает
    // vector<vector<POS_T>> mtx    текущее состояние доски
    // const bool color             кто ходит
    // const size_t depth           текущая глубина подсчета 
    // double alpha =               -1  - текущее значение для альфа отсечения -1 
    // double beta =                INF + 1 - бетта отсечение = дофига + 1
    // const POS_T x =              -1 - координата x текущей фигуры = -1 
    // const POS_T y =              -1 - координата y текущей фигуры = -1
    double find_best_turns_rec(vector<vector<POS_T>> mtx, const bool color, const size_t depth, double alpha = -1,
        double beta = INF + 1, const POS_T x = -1, const POS_T y = -1){
        //если максимальная глубина достигнута, то возвращаем "вес" текущей позиции (ну типа, смотрим сколько белых и чёрных фигур осталось.)
        if (depth == Max_depth)
        {
            return calc_score(mtx, (depth % 2 == color)); // выход из рекурсии и возврат максимального результата.
        }
        if (x != -1)
        {
            find_turns(x, y, mtx); // это поск ходов для конкретной фигуры с определёнными координатами
        }
        else
            find_turns(color, mtx);

        auto turns_now = turns; // обязательно надо сделать копии т.к. далее будет рекурсия со своими turns и have_beats 
        bool have_beats_now = have_beats;

        if (!have_beats_now && x != -1)
        {
            return find_best_turns_rec(mtx, 1 - color, depth + 1, alpha, beta); // стоит заметить, что здесь так же меняется цвет
        }

        if (turns.empty()) // если ходов совсем нет.
            return (depth % 2 ? 0 : INF);

        double min_score = INF + 1;
        double max_score = -1;
        for (auto turn : turns_now)
        {
            double score = 0.0;
            if (!have_beats_now && x == -1)
            {
                score = find_best_turns_rec(make_turn(mtx, turn), 1 - color, depth + 1, alpha, beta);
            }
            else
            {
                score = find_best_turns_rec(make_turn(mtx, turn), color, depth, alpha, beta, turn.x2, turn.y2); // если есть побития
            } 
            min_score = min(min_score, score);
            max_score = max(max_score, score);
            // alpha-beta отсечение 
            if (depth % 2)
                alpha = max(alpha, max_score);
            else
                beta = min(beta, min_score);
            if (optimization != "O0" && alpha >= beta) //настройка оптимизации
                return (depth % 2 ? max_score + 1 : min_score - 1);
        }
        return (depth % 2 ? max_score : min_score);
    }

public:
    //просто ищем ход для бота определённого цвета
    void find_turns(const bool color)
    {
        find_turns(color, board->get_board());
    }
    //ищем ходы для фишки под определенными координатами
    void find_turns(const POS_T x, const POS_T y)
    {
        find_turns(x, y, board->get_board());
    }

private:
    // Определение возможных ходов для указанного цвета на доске
    //получаем цвет текущего игрока
    //получаем матрицу (доску)
    void find_turns(const bool color, const vector<vector<POS_T>> &mtx)
    {
        vector<move_pos> res_turns; // вектор структуры move_pos
        bool have_beats_before = false; // побили фигуру ходом ранее ? НЕТ
        for (POS_T i = 0; i < 8; ++i)
        {// Обход всех ячеек с целью найти все ходы используя find_turns или поиск взятия
            for (POS_T j = 0; j < 8; ++j)
            {
                if (mtx[i][j] && mtx[i][j] % 2 != color) // Смотрим, кому принадлежит текущая фигура
                {
                    find_turns(i, j, mtx); 
                    if (have_beats && !have_beats_before)
                    {
                        have_beats_before = true;
                        res_turns.clear();
                    }
                    if ((have_beats_before && have_beats) || !have_beats_before)
                    {
                        res_turns.insert(res_turns.end(), turns.begin(), turns.end());
                    }
                }
            }
        }
        turns = res_turns;
        shuffle(turns.begin(), turns.end(), rand_eng); // перемешиваем ходы 
        have_beats = have_beats_before;
    }
    // Определение возможных ходов с определенной позиции на доске
    void find_turns(const POS_T x, const POS_T y, const vector<vector<POS_T>> &mtx)
    {
        turns.clear(); // очищаем ходы
        have_beats = false; // побития = не было
        POS_T type = mtx[x][y]; 
        // check beats
        // Проверка взятия для пешки или дамки
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            for (POS_T i = x - 2; i <= x + 2; i += 4)
            {
                for (POS_T j = y - 2; j <= y + 2; j += 4)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7)
                        continue;
                    POS_T xb = (x + i) / 2, yb = (y + j) / 2;
                    if (mtx[i][j] || !mtx[xb][yb] || mtx[xb][yb] % 2 == type % 2)
                        continue;
                    turns.emplace_back(x, y, i, j, xb, yb);
                }
            }
            break;
        default:
            // проверка для дамок
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    POS_T xb = -1, yb = -1;
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                        {
                            if (mtx[i2][j2] % 2 == type % 2 || (mtx[i2][j2] % 2 != type % 2 && xb != -1))
                            {
                                break;
                            }
                            xb = i2;
                            yb = j2;
                        }
                        if (xb != -1 && xb != i2)
                        {
                            turns.emplace_back(x, y, i2, j2, xb, yb);
                        }
                    }
                }
            }
            break;
        }
        // check other turns
        if (!turns.empty())
        {
            have_beats = true;
            return;
        }
        switch (type)
        {
        case 1:
        case 2:
            // check pieces
            {
                POS_T i = ((type % 2) ? x - 1 : x + 1);
                for (POS_T j = y - 1; j <= y + 1; j += 2)
                {
                    if (i < 0 || i > 7 || j < 0 || j > 7 || mtx[i][j])
                        continue;
                    turns.emplace_back(x, y, i, j);
                }
                break;
            }
        default:
            // check queens
            for (POS_T i = -1; i <= 1; i += 2)
            {
                for (POS_T j = -1; j <= 1; j += 2)
                {
                    for (POS_T i2 = x + i, j2 = y + j; i2 != 8 && j2 != 8 && i2 != -1 && j2 != -1; i2 += i, j2 += j)
                    {
                        if (mtx[i2][j2])
                            break;
                        turns.emplace_back(x, y, i2, j2);
                    }
                }
            }
            break;
        }
    }

  public:
     vector<move_pos> turns; // Возможные ходы
     bool have_beats;        // Есть ли среди возможных ходов взятие
     int Max_depth;          // Максимальная глубина поиска

  private:
    default_random_engine rand_eng; // генератор случайных чисел
    string scoring_mode;            // Настройка тип подсчёта очков
    string optimization;            // Настройка оптимизации поиска
    vector<move_pos> next_move;     // Следующие лучшие ходы
    vector<int> next_best_state;    // Следующие лучшие состояния
    Board* board;                   // Текущее состояние доски
    Config* config;                 // Настройки игры
};
