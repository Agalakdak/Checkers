#pragma once
#include <tuple>

#include "../Models/Move.h"
#include "../Models/Response.h"
#include "Board.h"

// methods for hands

//Я так понял, это метод, который отвечает за 
class Hand
{
  public:
      //коснтруктор по умолчанию.
    Hand(Board *board) : board(board)
    {
    }
    tuple<Response, POS_T, POS_T> get_cell() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        int x = -1, y = -1;
        int xc = -1, yc = -1;
        while (true)
        {
            //проверяем, есть ли событие в очереди
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type) //смотрим, каког отипасобытие обрабатывается сейчас
                {
                case SDL_QUIT: // нажали на "выход"
                    resp = Response::QUIT;
                    break;
                case SDL_MOUSEBUTTONDOWN: // нажали мышкой на клетку (я так думаю)
                    x = windowEvent.motion.x;       //думаю, тут получаем координаты
                    y = windowEvent.motion.y;       //думаю, тут получаем координаты
                    xc = int(y / (board->H / 10) - 1); //думаю, тут высчитываем координаты
                    yc = int(x / (board->W / 10) - 1);//думаю, тут высчитываем координаты
                    if (xc == -1 && yc == -1 && board->history_mtx.size() > 1) //если выбрана кнопка "Назад"
                    {
                        resp = Response::BACK;
                    }
                    else if (xc == -1 && yc == 8) // если выбрана кнопка "переиграть"
                    {
                        resp = Response::REPLAY;
                    }
                    else if (xc >= 0 && xc < 8 && yc >= 0 && yc < 8) //проверяем координаты клетки
                    {
                        resp = Response::CELL;
                    }
                    else
                    {
                        xc = -1;
                        yc = -1;
                    }
                    break;
                case SDL_WINDOWEVENT: // если изменилось положение окна или рамер окна
                    if (windowEvent.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                    {
                        board->reset_window_size(); // записываем новые размеры окна
                        break;
                    }
                }
                if (resp != Response::OK)
                    break;
            }
        }
        //возвращаем ответ и координаты клетки
        return {resp, xc, yc};
    }

    Response wait() const
    {
        SDL_Event windowEvent;
        Response resp = Response::OK;
        while (true)
        {
            if (SDL_PollEvent(&windowEvent))
            {
                switch (windowEvent.type)
                {
                case SDL_QUIT:
                    resp = Response::QUIT;
                    break;
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    board->reset_window_size();
                    break;
                case SDL_MOUSEBUTTONDOWN: { // аналогично как и чать кода выше
                    int x = windowEvent.motion.x;
                    int y = windowEvent.motion.y;
                    int xc = int(y / (board->H / 10) - 1);
                    int yc = int(x / (board->W / 10) - 1);
                    if (xc == -1 && yc == 8)
                        resp = Response::REPLAY;
                }
                break;
                }
                if (resp != Response::OK)
                    break;
            }
        }
        return resp;
    }

  private:
    Board *board;
};
