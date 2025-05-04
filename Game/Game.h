#pragma once
#include <chrono>
#include <thread>

#include "../Models/Project_path.h"
#include "Board.h"
#include "Config.h"
#include "Hand.h"
#include "Logic.h"

class Game
{
  public:
    Game() : board(config("WindowSize", "Width"), config("WindowSize", "Hight")), hand(&board), logic(&board, &config)
    {
        ofstream fout(project_path + "log.txt", ios_base::trunc);
        fout.close();
    }

    // to start checkers
    int play()
    {
        //начинаем отсчёт времени
        auto start = chrono::steady_clock::now();
        

        if (is_replay)
        {
            logic = Logic(&board, &config);
            config.reload();
            board.redraw();
        }
        else
        {
            board.start_draw();
        }
        is_replay = false;

        int turn_num = -1;
        bool is_quit = false;
        const int Max_turns = config("Game", "MaxNumTurns");


        //делаем ходы поочередно 
        while (++turn_num < Max_turns)
        {
            beat_series = 0;
            logic.find_turns(turn_num % 2);
            if (logic.turns.empty())
                break;
            //конструкция ниже не сразу, но была понята мной.
            //Достаточно хорошее применеие кратного if. По сути, мы здесь просто получаем значение из поля config. 
            logic.Max_depth = config("Bot", string((turn_num % 2) ? "Black" : "White") + string("BotLevel"));
            // определяем то, кто сейчас ходит 
            if (!config("Bot", string("Is") + string((turn_num % 2) ? "Black" : "White") + string("Bot")))
            {
                //если ходит человек, мы вызываемфункцию ниже 
                auto resp = player_turn(turn_num % 2);

                //если нажали на "выйти"
                if (resp == Response::QUIT)
                {
                    is_quit = true;
                    break;
                }
                // если решили начать игру с начала
                else if (resp == Response::REPLAY)
                {
                    is_replay = true;
                    break;
                }
                //если решили вернуть ход назад
                else if (resp == Response::BACK)
                {
                    if (config("Bot", string("Is") + string((1 - turn_num % 2) ? "Black" : "White") + string("Bot")) &&
                        !beat_series && board.history_mtx.size() > 2)
                    {
                        board.rollback();
                        --turn_num;
                    }
                    if (!beat_series)
                        --turn_num;

                    board.rollback();
                    --turn_num;
                    beat_series = 0;
                }
            }
            else
                bot_turn(turn_num % 2); // если ходит бот, то выполняется эта часть кода
        }
        
        // заканчиваем подсчёт времени
        auto end = chrono::steady_clock::now();

        //я так понимаю, что тут записываем лог
        ofstream fout(project_path + "log.txt", ios_base::app);
        // Записываем время хода
        fout << "Game time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        //закрываем файл для записи
        fout.close();

        if (is_replay)
            return play();
        if (is_quit)
            return 0;
        int res = 2;
        //смотрим, превысили мы кол-во ходов или нет, если превысили, то игра заканчивается
        if (turn_num == Max_turns)
        {
            res = 0;
        }
        else if (turn_num % 2)
        {
            res = 1;
        }

        board.show_final(res);
        auto resp = hand.wait();
        if (resp == Response::REPLAY)
        {
            is_replay = true;
            return play();
        }
        return res;
    }

  private:
    void bot_turn(const bool color)
    {
        auto start = chrono::steady_clock::now(); // начинаем считать время

        auto delay_ms = config("Bot", "BotDelayMS");  //записываем значение из json файла
        // new thread for equal delay for each turn
        thread th(SDL_Delay, delay_ms); // открываем дополнительный поток, что бы подсчёт был одинаков (хз, взял из лекции)
        auto turns = logic.find_best_turns(color); //ищем "лучший" ход
        th.join(); // соединяемся с потоком
        bool is_first = true;
        // making moves делаем ход 
        for (auto turn : turns)
        {
            if (!is_first)
            {
                SDL_Delay(delay_ms);
            }
            is_first = false;
            beat_series += (turn.xb != -1);
            board.move_piece(turn, beat_series);
        }

        auto end = chrono::steady_clock::now();
        ofstream fout(project_path + "log.txt", ios_base::app);
        fout << "Bot turn time: " << (int)chrono::duration<double, milli>(end - start).count() << " millisec\n";
        fout.close();
    }

    Response player_turn(const bool color)
    {
        // return 1 if quit
        vector<pair<POS_T, POS_T>> cells;
        for (auto turn : logic.turns)
        {
            cells.emplace_back(turn.x, turn.y);
        }
        //подсвечиваем клетки доступные для хода
        board.highlight_cells(cells);
        move_pos pos = {-1, -1, -1, -1};
        POS_T x = -1, y = -1;
        // Обрабатываем клики в цикле ниже до тех пор, пока не будет сделан ход 
        while (true)
        {
            //получаем клетку при помощи клика 
            auto resp = hand.get_cell();
            //Проверка, что мы выбрали клетку
            if (get<0>(resp) != Response::CELL)
                return get<0>(resp);
            pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};

            bool is_correct = false;

            //проверяем, что мы можем походить так, как указал игрок
            for (auto turn : logic.turns)
            {
                if (turn.x == cell.first && turn.y == cell.second)
                {
                    is_correct = true;
                    break;
                }
                if (turn == move_pos{x, y, cell.first, cell.second})
                {
                    pos = turn;
                    break;
                }
            }
            if (pos.x != -1)
                break;
            if (!is_correct)
            {
                if (x != -1)
                {
                    board.clear_active();
                    board.clear_highlight();
                    board.highlight_cells(cells);
                }
                x = -1;
                y = -1;
                continue;
            }
            x = cell.first;
            y = cell.second;
            board.clear_highlight();
            board.set_active(x, y);
            vector<pair<POS_T, POS_T>> cells2;
            for (auto turn : logic.turns)
            {
                if (turn.x == x && turn.y == y)
                {
                    cells2.emplace_back(turn.x2, turn.y2);
                }
            }
            board.highlight_cells(cells2);
        }

        board.clear_highlight();
        board.clear_active();
        board.move_piece(pos, pos.xb != -1);
        if (pos.xb == -1)
            return Response::OK;
        // continue beating while can
        beat_series = 1;
        while (true)
        {
            logic.find_turns(pos.x2, pos.y2);
            if (!logic.have_beats)
                break;

            vector<pair<POS_T, POS_T>> cells;
            for (auto turn : logic.turns)
            {
                cells.emplace_back(turn.x2, turn.y2);
            }
            board.highlight_cells(cells);
            board.set_active(pos.x2, pos.y2);
            // trying to make move
            while (true)
            {
                auto resp = hand.get_cell();
                if (get<0>(resp) != Response::CELL)
                    return get<0>(resp);
                pair<POS_T, POS_T> cell{get<1>(resp), get<2>(resp)};
                
                //проверка корректности хода 
                bool is_correct = false;
                for (auto turn : logic.turns)
                {
                    if (turn.x2 == cell.first && turn.y2 == cell.second)
                    {
                        is_correct = true;
                        pos = turn;
                        break;
                    }
                }
                if (!is_correct)
                    continue;

                board.clear_highlight();
                board.clear_active();
                beat_series += 1;
                board.move_piece(pos, beat_series);
                break;
            }
        }

        return Response::OK;
    }

  private:
    Config config;
    Board board;
    Hand hand;
    Logic logic;
    int beat_series;
    bool is_replay = false;
};
