#include <iostream>
#include <cmath>
#include <vector>

// Функция для проверки, все ли ячейки заполнены в игровом поле.
int solveCheck(std::vector<std::vector<int>>& tmpField)
{
    // Итерация по всем элементам поля.
    for (int i = 0; i < tmpField.size(); i++)
        for (int j = 0; j < tmpField.size(); j++)
            // Если находим пустую ячейку.
            if (!tmpField[i][j])
                return 0;
    // Все поля заполнены.
    return 1;
}

// Функция для определения корректных чисел, которые могут быть помещены в ячейку.
int correctNums(std::vector<std::vector<int>>& tmpField,
    std::vector<int>& corrNums, int row, int col)
{
    int sqn = static_cast<int>(sqrt(tmpField.size()));
    // Вектор для отслеживания уже использованных чисел.
    std::vector<int> appearedNums(tmpField.size() + 1, 0);

    // Отмечаем числа, которые уже есть в этой строке.
    for (int i = 0; i < tmpField.size(); i++)
        appearedNums[tmpField[row][i]] = 1;

    // Отмечаем числа, которые уже есть в этом столбце.
    for (int i = 0; i < tmpField.size(); i++)
        appearedNums[tmpField[i][col]] = 1;

    // Отмечаем числа, которые уже есть в этом квадрате.
    row = (row / sqn) * sqn, col = (col / sqn) * sqn;
    for (int i = row; i < (row + sqn); i++)
        for (int j = col; j < (col + sqn); j++)
            appearedNums[tmpField[i][j]] = 1;

    // Заполняем список корректных чисел.
    int index = 0;
    for (int i = 1; i <= tmpField.size(); i++)
        if (!appearedNums[i])
            corrNums[index++] = i;

    return index;
}

void solve(std::vector<std::vector<int>>& field, bool& solved)
{
    if (solved) {
        return;
    }
    // Проверяем, решено ли судоку.
    if (solveCheck(field)) {
        for (int i = 0; i < field.size(); i++) {
            for (int j = 0; j < field.size(); j++) {
                std::cout << field[i][j] << " ";
            }
            std::cout << "\n";
        }
        solved = true;
        return;
    }

    // Находим первую пустую ячейку.
    bool flag = false;
    int i, j;
    for (i = 0; i < field.size(); i++) {
        for (j = 0; j < field.size(); j++) {
            if (!field[i][j]) {
                flag = true;
                break;
            }
        }
        if (flag) {
            break;
        }
    }

    // Генерируем список корректных чисел для этой ячейки и пробуем их.
    std::vector<int> corrNums(field.size() + 1);
    int index = correctNums(field, corrNums, i, j);
    for (int it = 0; it < index; it++) {
        field[i][j] = corrNums[it];
        solve(field, solved);
    }
    // Возвращаем ячейку к пустому состоянию.
    field[i][j] = 0;
}

int main(int argc, char const* argv[])
{
    bool solved = false;
    int n = 0;
    std::cin >> n;
    std::vector<std::vector<int>> field(n, std::vector<int>(n));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            std::cin >> field[i][j];
        }
    }

    solve(field, solved);
    if (!solved) {
        std::cout << "Invalid field" << std::endl;
    }

    return 0;
}