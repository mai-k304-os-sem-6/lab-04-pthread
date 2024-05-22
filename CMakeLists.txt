cmake_minimum_required(VERSION 3.10)

# Название проекта
project(lab-04-pthread)

# Стандарт C++
set(CMAKE_CXX_STANDARD 14)

# Указываем исполняемый файл
add_executable(lab-04-pthread main.c)

# Добавляем путь к заголовочным файлам
target_include_directories(lab-04-pthread PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Линкуем библиотеки pthread и m
target_link_libraries(lab-04-pthread PRIVATE pthread m)
