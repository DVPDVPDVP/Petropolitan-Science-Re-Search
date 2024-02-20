#include <iostream>
#include <algorithm>
#include <mpi.h>
#include <mutex>
#include <vector>
#include <cmath>
#include <string>
#include <thread>
#include <numeric>
#include <memory>

#define NUMBER_OF_THREADS 12

class Stack {
public:
    int y;
    int x;
    int num;
    int lvl;
    std::shared_ptr<Stack> prev;
    std::shared_ptr<Stack> next;

    Stack(int y, int x, int num, int lvl, std::shared_ptr<Stack> prev = nullptr)
        : y(y), x(x), num(num), lvl(lvl), prev(prev), next(nullptr) {}

    static std::shared_ptr<Stack> push(std::shared_ptr<Stack> s, int y, int x, int num, int lvl) {
        auto temp = std::make_shared<Stack>(y, x, num, lvl, s);
        if (s != nullptr) {
            s->next = temp;
        }
        return temp;
    }

    static std::shared_ptr<Stack> pop(std::shared_ptr<Stack> s) {
        if (s == nullptr) {
            return s;
        }
        auto temp = s->prev;
        if (temp != nullptr) {
            temp->next = nullptr;
        }
        return temp;
    }
};

struct Ceil  
{
public:
    int y;  
    int x;  
    int num;    
    int lvl;   
    friend bool operator==(const Ceil& lhs, const Ceil& rhs) {
        return lhs.y == rhs.y && lhs.x == rhs.x && lhs.num == rhs.num && lhs.lvl == rhs.lvl;
    }
    friend bool operator!=(const Ceil& lhs, const Ceil& rhs) {
        return !(lhs == rhs);
    }
    Ceil(const Ceil& rhs): 
        y(rhs.y), x(rhs.x), num(rhs.num), lvl(rhs.lvl) {}
    Ceil(int y, int x, int num, int lvl): 
        y(y), x(x), num(num), lvl(lvl) {}
    Ceil() {}
    ~Ceil() {}
};

std::vector<int> workingThreads(NUMBER_OF_THREADS, 0);
std::mutex mutex;
int perm = 0;
int solved = 0;
int completeFlag = 0;
std::vector<std::vector<int>> field(100, std::vector<int>(100, -1));
std::shared_ptr<Stack> st;

class ThreadData
{
public:
    int threadId;  
    int processId; 
    int sz; 
    ThreadData(int thread_id, int process_id, int sz):
        threadId(threadId), processId(processId), sz(sz) {};
    ThreadData() {};
    ~ThreadData() {};
};

int solveCheck(std::vector<std::vector<int>> &tmpField)
{
    int i, j;
    for (i = 0; i < tmpField.size(); i++)
        for (j = 0; j < tmpField.size(); j++)
            if (!tmpField[i][j])
                return 0;
    return 1;
}

int correctNums(std::vector<std::vector<int>> &tmpField, std::vector<int> &corrNums, int row, int col)
{
    int index = 0, sqn = static_cast<int>(sqrt(field.size()));
    std::vector<int> appearedNums(field.size() + 1, 0);

    for (int i = 0; i < tmpField.size(); i++)
        appearedNums[tmpField[row][i]] = 1;

    for (int i = 0; i < tmpField.size(); i++)
        appearedNums[tmpField[i][col]] = 1;

    row = (row / sqn) * sqn, col = (col / sqn) * sqn;

    for (int i = row; i < (row + sqn); i++)
        for (int j = col; j < (col + sqn); j++)
            appearedNums[tmpField[i][j]] = 1;

    for (int i = 1; i <= tmpField.size(); i++)
        if (!appearedNums[i])
            corrNums[index++] = i;

    return index;
}

void solve(ThreadData data)
{
    // Инициализация переменных для идентификации источника и назначения сообщений MPI, управления запросами и состояниями MPI.
    int src, dest;
    MPI_Request sendReq;
    MPI_Status work, wait, other;

    // Определение размеров блоков и типов данных для пользовательского типа данных MPI, представляющего ячейку.
    int blokSizes[4] = { 1, 1, 1, 1 };
    MPI_Datatype types[4] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT };
    MPI_Datatype ceilType;

    // Определение смещений для каждого поля структуры 'Ceil' для создания пользовательского типа данных MPI.
    MPI_Aint offsets[4];
    offsets[0] = offsetof(Ceil, y);
    offsets[1] = offsetof(Ceil, x);
    offsets[2] = offsetof(Ceil, num);
    offsets[3] = offsetof(Ceil, lvl);

    // Создание и подтверждение пользовательского типа данных MPI на основе предыдущих определений.
    MPI_Type_create_struct(4, blokSizes, offsets, types, &ceilType);
    MPI_Type_commit(&ceilType);

    // Инициализация различных переменных для управления состоянием работы.
    int reqWait1, resWait, request_for_work_busy, resWork, reqWait2;
    int curWaitData[2], curWorkData[2];
    Ceil data_received_idle[field.size()];
    int send = 0;

    // Инициализация переменной для отслеживания, было ли отправлено сообщение MPI.
    int isSent = 0;
    srand(time(nullptr) + data.processId);

    // Проверка, является ли текущий поток главным потоком (threadId == 0).
    if (data.threadId == 0) {
        // Пока не найдено решение ('solved' не установлено в 1).
        while (!solved) {
            // Проверка, все ли рабочие потоки простаивают (их сумма равна 0).
            if (std::accumulate(workingThreads.begin(), workingThreads.end(), 0) == 0) {

                // Ожидание сообщения от любого источника с проверкой наличия сообщения.
                int check;
                MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &check, &other);

                // Если есть сообщение, обработать его.
                if (check) {
                    // Получение сообщения с запросом на ожидание.
                    MPI_Recv(&reqWait2, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &other);

                    // Если запрос на ожидание говорит о завершении (reqWait2 == 0), завершить выполнение.
                    if (reqWait2 == 0) {
                        solved = 1;
                        break;
                    }

                    // Подготовка данных для ответа на запрос ожидания.
                    curWaitData[0] = 0;
                    curWaitData[1] = 0;
                    src = other.MPI_SOURCE;

                    // Отправка данных в ответ на запрос ожидания.
                    MPI_Send(curWaitData, 2, MPI_INT, src, 2, MPI_COMM_WORLD);
                }

                // Установка флага reqWait1 в 1 для последующего запроса на ожидание.
                reqWait1 = 1;

                // Проверка, было ли уже отправлено сообщение.
                if (isSent == 1) {
                    // Проверка состояния отправленного запроса.
                    MPI_Test(&sendReq, &send, MPI_STATUS_IGNORE);
                    if (send == 1) {
                        // Если сообщение было отправлено, сбросить флаг отправки и получить данные.
                        isSent = 0;
                        MPI_Recv(curWaitData, 2, MPI_INT, dest, 2, MPI_COMM_WORLD, &wait);

                        // Если данные указывают на наличие работы (curWaitData[0] == 1),
                        // получить данные и обновить состояние.
                        if (curWaitData[0] == 1) {
                            // Получение данных о работе.
                            MPI_Recv(data_received_idle, curWaitData[1], ceilType, dest, 3, MPI_COMM_WORLD, &wait);
                            int g;
                            for (g = 0; g < curWaitData[1]; ++g) {
                                // Добавление полученных данных в стек.
                                Stack::push(st, data_received_idle[g].y, data_received_idle[g].x, data_received_idle[g].num, data_received_idle[g].lvl);
                            }

                            // Отправка подтверждения приема работы.
                            resWait = 1;
                            MPI_Send(&resWait, 1, MPI_INT, dest, 1, MPI_COMM_WORLD);

                            // Получение обновленного состояния поля.
                            MPI_Recv(&field[0][0], field.size() * field.size(), MPI_INT, dest, field.size() * field.size(), MPI_COMM_WORLD, &wait);

                            // Установка флага perm в 1 для разрешения работы другим потокам.
                            mutex.lock();
                            perm = 1;
                            mutex.unlock();

                            // Ожидание, пока все потоки не начнут работу.
                            while (std::accumulate(workingThreads.begin(), workingThreads.end(), 0) == 0) { }
                        }
                    }
                } else {
                    // Если сообщение не было отправлено, выбрать случайный процесс-получатель и отправить ему запрос на ожидание.
                    dest = rand() % data.sz;
                    while (dest == data.processId) {
                        dest = rand() % data.sz;
                    }
                    MPI_Issend(&reqWait1, 1, MPI_INT, dest, 1, MPI_COMM_WORLD, &sendReq);
                    isSent = 1;
                }
            }

            else {
                // Если не все потоки простаивают, выполнять следующую логику.
                while ((!solved) && (std::accumulate(workingThreads.begin(), workingThreads.end(), 0) != 0)) {
                    // Проверка на наличие входящих сообщений.
                    int check;
                    MPI_Iprobe(MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &check, &other);
                    if (check == 1) {
                        // Если есть сообщение, заблокировать мьютекс и обработать сообщение.
                        mutex.lock();
                        MPI_Recv(&request_for_work_busy, 1, MPI_INT, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &other);
                        if (request_for_work_busy == 0) {
                            // Если запрос указывает на завершение работы, завершить выполнение.
                            solved = 1;
                            mutex.unlock();
                            break;
                        }
                        src = other.MPI_SOURCE;

                        // Проверка, есть ли работа для отправки.
                        if (field[0][0] == -1) {
                            // Если работы нет, отправить пустой ответ.
                            curWorkData[0] = 0;
                            curWorkData[1] = 0;
                            MPI_Send(curWorkData, 2, MPI_INT, src, 2, MPI_COMM_WORLD);
                            mutex.unlock();
                        }

                        else {
                            // Если есть работа, подготовить данные и отправить их запрашивающему процессу.
                            Ceil tmp[field.size()];
                            std::shared_ptr<Stack> tmpStack = st;
                            int k = 0;

                            // Перенос данных из стека в временный массив.
                            while (tmpStack != nullptr) {
                                tmp[k].y = tmpStack->y;
                                tmp[k].x = tmpStack->x;
                                tmp[k].num = tmpStack->num;
                                tmp[k].lvl = tmpStack->lvl;
                                k++;
                                tmpStack = tmpStack->prev;
                            }

                            // Отправка данных о работе.
                            curWorkData[0] = 1;
                            curWorkData[1] = k;
                            MPI_Send(curWorkData, 2, MPI_INT, src, 2, MPI_COMM_WORLD);
                            MPI_Send(tmp, k, ceilType, src, 3, MPI_COMM_WORLD);

                            // Ожидание подтверждения и отправка обновленного состояния поля.
                            MPI_Recv(&resWork, 1, MPI_INT, src, 1, MPI_COMM_WORLD, &work);
                            MPI_Send(&field[0][0], field.size() * field.size(), MPI_INT, src, field.size() * field.size(), MPI_COMM_WORLD);

                            // Очистка стека и обновление состояния.
                            while (st != nullptr) {
                                st = Stack::pop(st);
                            }
                            field[0][0] = -1;
                            mutex.unlock();
                            break;
                        }
                    } else {
                        // Если нет входящих сообщений, разрешить работу другим потокам.
                        mutex.lock();
                        perm = 1;
                        mutex.unlock();
                    }
                }
            }
        }
    } else {
        // Логика для неглавных потоков (threadId != 0).
        std::vector<std::vector<int>> tmpField;
        std::shared_ptr<Stack> tmpStack = nullptr, rebuildStack = nullptr, stackPointer = nullptr;
        int cntTop = 0, prevLvl = 0, curLvl = 0, flag, index, i, j;
        std::vector<int> corrNums;

        // Логика инициализации для одного из потоков.
        if (data.threadId == 1) {
            if (data.processId == 0) {
                // Чтение данных из файла и инициализация поля.
                FILE* file;
                file = fopen("test1.txt", "r");
                int n = 0;
                fscanf(file, "%d", &n);
                int sqn = static_cast<int>(sqrt(n));
                if (sqn * sqn != n) {
                    std::cout << "Invalid field" << std::endl;
                    int complete = 0;
                    solved = 1;
                    for (int k = 0; k < data.sz; k++) {
                        if (k != data.processId) {
                            MPI_Ssend(&complete, 1, MPI_INT, k, 1, MPI_COMM_WORLD);
                        }
                    }
                    return;
                }
                tmpField.resize(n);
                for (auto &iter : tmpField) {
                    iter.resize(n);
                }
                field.resize(n);
                for (auto &iter : field) {
                    iter.resize(n);
                    iter.front() = -1;
                }
                corrNums.resize((field.size() + 1));
                for (i = 0; i < n; i++)
                    for (j = 0; j < n; j++)
                        fscanf(file, "%d", &tmpField[i][j]);
                fclose(file);
                workingThreads[data.threadId] = 1;
            }
        }
        // Основной цикл неглавных потоков.
        while (!solved) {

            // Пока поток активен и решение не найдено.
            while (workingThreads[data.threadId] && !solved) {

                // Проверка, решена ли задача.
                if (solveCheck(tmpField)) {
                    // Если решение найдено, обновить состояние и завершить работу.
                    mutex.lock();
                    for (i = 0; i < field.size(); i++) {
                        for (j = 0; j < field.size(); j++) {
                            std::cout << tmpField[i][j] << " ";
                            field[i][j] = tmpField[i][j];
                        }
                        std::cout << std::endl;
                    }
                    solved = 1;
                    completeFlag = 1;
                    mutex.unlock();
                    break;
                }

                // Поиск первой пустой ячейки на поле.
                flag = 0;
                for (i = 0; i < field.size(); i++) {
                    for (j = 0; j < field.size(); j++) {
                        if (!tmpField[i][j]) {
                            flag = 1;
                            break;
                        }
                    }
                    if (flag)
                        break;
                }

                // Определение корректных значений для текущей ячейки.
                index = correctNums(tmpField, corrNums, i, j);
                for (int l = 0; l < index; l++) {
                    // Добавление возможных значений в стек.
                    tmpStack = Stack::push(tmpStack, i, j, corrNums[l], curLvl);

                    // Установка указателя на начало стека.
                    if (tmpStack->prev == nullptr) {
                        stackPointer = tmpStack;
                    }
                }

                mutex.lock();
                if (tmpStack == nullptr && tmpStack != stackPointer && field[0][0] == -1) {
                    // Если стек пуст, но были выполнены некоторые действия, обновить поле.
                    for (i = 0; i < field.size(); i++) {
                        for (j = 0; j < field.size(); j++) {
                            field[i][j] = tmpField[i][j];
                        }
                    }

                    // Реконструкция стека с обновленными данными.
                    std::shared_ptr<Stack> temp = stackPointer;
                    cntTop = 1, prevLvl = stackPointer->lvl;
                    temp = temp->next;

                    while (temp != nullptr) {
                        if (temp->lvl != prevLvl) {
                            break;
                        }
                        cntTop++;
                        temp = temp->next;
                    }

                    cntTop = (cntTop + 1) / 2;

                    while (cntTop--) {
                        st = Stack::push(st, stackPointer->y, stackPointer->x, stackPointer->num, stackPointer->lvl);
                        stackPointer = stackPointer->next;
                    }

                    // Очистка и восстановление поля.
                    temp = rebuildStack;
                    i = tmpStack->lvl - prevLvl;

                    while (temp != nullptr && i--) {
                        field[temp->y][temp->x] = 0;
                        temp = temp->next;
                    }
                }

                mutex.unlock();

                // Обработка случая, когда не осталось доступных значений для ячейки.
                if (index == 0 && tmpStack != nullptr) {
                    while (rebuildStack != nullptr && rebuildStack->lvl >= tmpStack->lvl) {
                        tmpField[rebuildStack->y][rebuildStack->x] = 0;
                        rebuildStack = Stack::pop(rebuildStack);
                    }
                }

                else {
                    // Переход на следующий уровень в стеке.
                    curLvl++;
                }

                // Если стек не пуст, обновить поле и перейти к следующему элементу.
                if (tmpStack != nullptr) {
                    tmpField[tmpStack->y][tmpStack->x] = tmpStack->num;
                    rebuildStack = Stack::push(rebuildStack, tmpStack->y, tmpStack->x, tmpStack->num, tmpStack->lvl);
                    curLvl = tmpStack->lvl + 1;
                    tmpStack = Stack::pop(tmpStack);
                }

                else {
                    // Если стек пуст, очистить rebuildStack и пометить поток как простаивающий.
                    while (rebuildStack != nullptr) {
                        rebuildStack = Stack::pop(rebuildStack);
                    }

                    mutex.lock();
                    workingThreads[data.threadId] = 0;
                    mutex.unlock();
                }
            }

            // Если поток простаивает и решение не найдено, ожидать работы.
            if (!workingThreads[data.threadId] && !solved) { }
            while (!workingThreads[data.threadId] && !solved) {
                mutex.lock();
                if (perm == 1 && !solved) {
                    // Если есть разрешение на выполнение работы, проверить, есть ли доступная работа.
                    if (field[0][0] != -1) {
                        // Если есть доступная работа, восстановить состояние стека и поля.
                        while (st != nullptr) {
                            tmpStack = Stack::push(tmpStack, st->y, st->x, st->num, st->lvl);
                            if (tmpStack->prev == nullptr) {
                                stackPointer = tmpStack;
                            }
                            st = Stack::pop(st);
                        }

                        for (i = 0; i < field.size(); i++) {
                            for (j = 0; j < field.size(); j++) {
                                tmpField[i][j] = field[i][j];
                            }
                        }

                        tmpField[tmpStack->y][tmpStack->x] = tmpStack->num;
                        rebuildStack = Stack::push(rebuildStack, tmpStack->y, tmpStack->x, tmpStack->num, tmpStack->lvl);
                        curLvl = tmpStack->lvl + 1;
                        tmpStack = Stack::pop(tmpStack);

                        field[0][0] = -1;
                        workingThreads[data.threadId] = 1;

                        perm = 0;
                    }
                }
                mutex.unlock();
            }
        }
    }
    // Пометка решения как найденного.
    solved = 1;
    if (data.threadId == 0) {
        // Если поток является главным и решение найдено, отправить уведомление о завершении другим процессам.
        if (completeFlag == 1) {
            int complete = 0;
            for (int k = 0; k < data.sz; k++) {
                if (k != data.processId) {
                    MPI_Ssend(&complete, 1, MPI_INT, k, 1, MPI_COMM_WORLD);
                }
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    MPI_Init(nullptr, nullptr);
    std::vector<std::thread> threads(NUMBER_OF_THREADS);
    std::vector<ThreadData> threadData(NUMBER_OF_THREADS);
    int rank = 0, size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        workingThreads[1] = 1;
    }
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        threadData[i].threadId = i;
        threadData[i].processId = rank;
        threadData[i].sz = size;
        threads[i] = std::thread(&solve, threadData[i]);

    }
    for (int i = 0; i < NUMBER_OF_THREADS; i++) {
        threads[i].join();
    }
    MPI_Finalize();
    return 0;
}
