# csv2json

Вариант 14: конвертер CSV в JSON.

Консольная программа на C (C11). Читает CSV-файл с заголовком и выводит JSON: массив объектов, где ключи — названия колонок из первой строки, а значения — ячейки остальных строк.

## Сборка

    sudo apt install $(cat build-deps.txt)
    cmake -S . -B build
    cmake --build build

## Использование

    csv2json convert [-d РАЗДЕЛИТЕЛЬ] [-o ФАЙЛ] ФАЙЛ.csv
    csv2json --help
    csv2json --version

- `-d`, `--delimiter` — разделитель полей: один символ (по умолчанию `,`) или `\t` для табуляции.
- `-o`, `--output` — записать JSON в файл, а не в стандартный вывод.

Примеры:

    ./build/csv2json convert examples/people.csv
    ./build/csv2json convert -d ';' examples/people_semicolon.csv
    ./build/csv2json convert -o result.json examples/people.csv

Код возврата: 0 — успех, 1 — ошибка чтения или разбора, 2 — неверные аргументы. Сообщения об ошибках выводятся в stderr.

Все значения в JSON — строки: типы данных программа не определяет.

## Что сделано

- ПР 1: репозиторий, программа, сборка CMake.
