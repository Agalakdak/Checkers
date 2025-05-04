#pragma once
#include <fstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "../Models/Project_path.h"

class Config
{
  public:
    Config()
    {
        reload();
    }

    // данная функция вызывается для загрузки файла конфига в программу. 
    void reload()
    {
        //сначала идёт считывание файла settings.json из корневого каталога проекта.
        std::ifstream fin(project_path + "settings.json");
        fin >> config;    //здесь происходит запись из файла settings.json в экземпляр класса (ну вернее только в его переменную)
        fin.close(); //закрываем сам файл.
    }


    //Прекрасное и элегантное решение (на мой взгляд). Здесь мы перегружаем оператор круглых скобок, что бы получить значение нужного нам поля. 
    // Перегрузка самого оператора, делает объект класса вызываемым как функцию.
    auto operator()(const string &setting_dir, const string &setting_name) const
    {
        return config[setting_dir][setting_name];
    }

  private:
    json config;
};
