import os
import linecache
import numpy as np

disks = ["disc 0.txt", "disc 1.txt", "disc 2.txt", "disc 3.txt", "disc 4.txt", "disc 5.txt"]

lines = 64


def fill_file(file_name: str) -> None:
    with open(file_name, 'w') as f:
        f.write('\n' * lines)


def find_broken():  # находит файлы в которых только -, то есть которые слетели
    s = []
    d = 0
    list_of_data = []
    for i in range(0, 6):
        #file = open(disks[i], 'r')
        if not os.path.isfile(disks[i]):
            s.append(disks[i])
        elif os.stat(disks[i]).st_size == 0:
            s.append(disks[i])
    return s


def valid_hex(new_value: str):  # проверяет, чтобы введенное значение было 16ричным
    a = True
    for c in new_value:
        flag = False
        if c.isdigit() or c in "abcdef":
            flag = True
        else:
            flag = False
        if not flag:
            a = False
            break
    return a


def check_input():
    while True:
        input_data = str(input("Enter a hexadecimal value of 8 characters: "))
        if len(input_data) != 8 or not valid_hex(input_data):
            print("Error, repeat entry ")
        else:
            break
    a = input_data[:len(input_data) // 4]
    b = input_data[len(input_data) // 4: len(input_data) // 2]
    c = input_data[len(input_data) // 2: (len(input_data) // 4 * 3)]
    d = input_data[len(input_data) // 4 * 3:]
    polyn1 = f"{(int(a, 16) + int(b, 16) + int(c, 16) + int(d, 16)):x}"  # a+b+c+d
    polyn2 = f"{(int(a, 16) + 2 * int(b, 16) - int(c, 16) - 2 * int(d, 16)):x}"  # a+b-c-d
    res = [a, b, c, d, polyn1, polyn2]
    return res


my_redundancy = [[0, 1, 2, 3, 4, 5], [0, 1, 2, 4, 5, 3], [0, 1, 4, 5, 2, 3], [0, 4, 5, 1, 2, 3],
                 [4, 5, 0, 1, 2, 3], [5, 0, 1, 2, 3, 4]]  # массив с порядком индексов, в соответствии с которым будут


# записываться элементы из массива со всеми значениями
def write():

    if len(find_broken()) != 0:
        print("Discs are broken, you can't record information")
        return
    #else:
    my_values = check_input()  # хранит массив со значениями в порядке a b c d polyn1 polyn2
    while True:
        ind = str(input("Enter the index of the row from 0 to 63 in which you want to write the values: "))
        if is_number(ind):
            if int(ind) > 63:
                print("Error, repeat entry ")
            else:
                break
    index_str = int(ind)
    ind1 = index_str % 6

    list_of_data = []  # будет двумерный массив(лист листов) со значениями из файликов
    for i in range(0, 6):
        file = open(disks[i], 'r')
        list_of_data += [file.readlines()]
        #file.close()

    for i in range(len(my_redundancy[ind1])):
        a = my_redundancy[ind1][i]
        list_of_data[i][index_str] = str(my_values[my_redundancy[ind1][i]]) + '\n'

    for i in range(0, 6):
        file = open(disks[i], 'w')
        for j in range(len(list_of_data[i])):
            file.write(list_of_data[i][j])
        #file.close()


def find_place(d: int, mass=None) -> int:
    for i in range(0, 6):
        if d == mass[i]:
            return i


def to_solve(string_address: int):
    global x1, c1, d1, y1

    list_of_data = []
    for i in range(0, 6):
        if os.path.isfile(disks[i]):
            file = open(disks[i], 'r')
            list_of_data += [file.readlines()]
            #file.close()
        else:
            list_of_data += [['0']]

    mas = find_broken()
    res = ''
    ind1 = string_address % 6
    mine = my_redundancy[ind1]

    ddf = is_empty(string_address)
    print(ddf)

    if is_empty(string_address):
        print("No information is available at this address")
        return

    if len(mas) > 2:
        print("More than two discs have failed, the information cannot be recovered")
        return

    if len(mas) == 0:  # ну если все диски целы
        for j in range(0, 6):
            if mine[j] != 4 and mine[j] != 5:
                str = list_of_data[j][string_address]
                res += str[:-1]
        print(res)
        return

    elif len(mas) == 1:
        res = ''
        for i in range(0, 6):
            if mas[0] == disks[i] and (mine[i] == 4 or mine[i] == 5):  # если падает диск с избыточностью   #mas[0] == disks[i] and в начале еще это было написано
                for j in range(0, 6):
                    if mine[j] != 4 and mine[j] != 5:
                        str = list_of_data[j][string_address]
                        res += str[:-1]
                #break
            elif mas[0] == disks[i] and (
                    mine[i] != 4 and mine[i] != 5):  # если падает какой-то 1 диск, который просто с данными
                a = []
                ff = mine[i]
                for j in range(0, 6):
                    if mine[j] == 4:
                        a.append(list_of_data[j][string_address][:-1])
                    elif mine[j] != mine[i] and mine[j] != 5:
                        a.insert(0, list_of_data[j][string_address][:-1])
                ss = f"{(int(a[3], 16) - int(a[0], 16) - int(a[1], 16) - int(a[2], 16)):x}"
                if len(ss) != 2:
                    ss = "0" + ss
                for s in range(65):
                    list_of_data[i].append('0')
                list_of_data[i][string_address] = (ss + '\n')
                for i1 in range(0, 4):
                    for j1 in range(0, 6):
                        if mine[j1] == i1:
                            res += list_of_data[j1][string_address][:-1]
                            break
    elif len(mas) == 2:
        res = ''
        for i in range(0, 6):
            for j in range(0, 6):
                if (((mas[0] == disks[i] and mas[1] == disks[j]) or (mas[1] == disks[i] and mas[0] == disks[j])) and
                        ((mine[i] == 4 and mine[j] == 5) or (
                                mine[i] == 4 and mine[j] == 5))):  # если слетело 2 диска с избыточностью
                    for ii in range(0, 6):
                        if mine[ii] != 4 and mine[ii] != 5:
                            str1 = list_of_data[ii][string_address]
                            res += str1[:-1]
                    break
                    print(res)
                    return
                elif (((mas[0] == disks[i] and mas[1] == disks[j]) or (mas[1] == disks[i] and mas[0] == disks[j])) and
                      mine[i] == 4 and mine[j] != 5):  # если слетел первый диск с избыточностью и диск с данными
                    a = []
                    for jj in range(0, 6):
                        if mine[jj] == 5:
                            a.append(list_of_data[jj][string_address][:-1])
                        elif mine[jj] != mine[j] and mine[jj] != 4:
                            a.insert(0, list_of_data[jj][string_address][:-1])
                    ss = ''
                    if mine[j] == 0:
                        ss = f"{(int(a[3], 16) + 2 * int(a[0], 16) + int(a[1], 16) - 2 * int(a[2], 16)):x}"
                        if len(ss) != 2:
                            ss = "0" + ss
                    elif mine[j] == 1:
                        ss = f"{((int(a[3], 16) + 2 * int(a[0], 16) + int(a[1], 16) - int(a[2], 16)) // 2):x}"
                        if len(ss) != 2:
                            ss = "0" + ss
                    elif mine[j] == 2:
                        ss = f"{(int(a[3], 16) + 2 * int(a[0], 16) - 2 * int(a[1], 16) - int(a[2], 16)):x}"
                        if len(ss) != 2:
                            ss = "0" + ss
                    elif mine[j] == 3:
                        ss = f"{((int(a[0], 16) - 2 * int(a[1], 16) - int(a[2], 16) + int(a[3], 16)) // (-2)):x}"
                        if len(ss) != 2:
                            ss = "0" + ss
                    for s in range(65):
                        list_of_data[j].append('0')
                    list_of_data[j][string_address] = ss + '\n'
                    for i1 in range(0, 4):
                        for j1 in range(0, 6):
                            if mine[j1] == i1:
                                res += list_of_data[j1][string_address][:-1]
                                break
                    print(res)
                    return
                elif (((mas[0] == disks[i] and mas[1] == disks[j]) or (mas[1] == disks[i] and mas[0] == disks[j]))
                      and mine[i] != 4 and mine[j] == 5):  # если слетел второй диск с избыточностью и диск с данными
                    a = []
                    for jj in range(0, 6):
                        if mine[jj] == 4:
                            a.append(list_of_data[jj][string_address][:-1])
                        elif mine[jj] != mine[i] and mine[jj] != 5:
                            a.insert(0, list_of_data[jj][string_address][:-1])
                    ss = f"{(int(a[3], 16) - int(a[0], 16) - int(a[1], 16) - int(a[2], 16)):x}"
                    if len(ss) != 2:
                        ss = "0" + ss
                    for s in range(65):
                        list_of_data[i].append('0')
                    list_of_data[i][string_address] = ss + '\n'
                    for i1 in range(0, 4):
                        for j1 in range(0, 6):
                            if mine[j1] == i1:
                                res += list_of_data[j1][string_address][:-1]
                                break
                    print(res)
                    return
        for i in range(0, 6):
            for j in range(0, 6):
                if (((mas[0] == disks[i] and mas[1] == disks[j]) or (mas[1] == disks[i] and mas[0] == disks[j]))
                      and mine[i] != 4 and mine[j] != 5):  # если слетели 2 диска с информацией
                    if (disks[find_place(4, mine)] not in find_broken()):
                        x1 = list_of_data[find_place(4, mine)][string_address][:-1]
                    if (disks[find_place(5, mine)] not in find_broken()):
                        y1 = list_of_data[find_place(5, mine)][string_address][:-1]
                    if (disks[find_place(0, mine)] not in find_broken()):
                        a1 = list_of_data[find_place(0, mine)][string_address][:-1]
                    if (disks[find_place(1, mine)] not in find_broken()):
                        b1 = list_of_data[find_place(1, mine)][string_address][:-1]
                    if (disks[find_place(2, mine)] not in find_broken()):
                        c1 = list_of_data[find_place(2, mine)][string_address][:-1]
                    if (disks[find_place(3, mine)] not in find_broken()):
                        d1 = list_of_data[find_place(3, mine)][string_address][:-1]

                    if mine[i] == 0 and mine[j] == 1:
                        M1 = np.array([[1., 1.], [1., 2.]])
                        V1 = np.array(
                            [int(x1, 16) - int(c1, 16) - int(d1, 16), int(y1, 16) + 2 * int(d1, 16) + int(c1, 16)])
                        answer = np.linalg.solve(M1, V1).astype(
                            int)  # но тут у нас значения в десятичном виде, для результата нужно в шестнадцатиричную
                        ss1 = f"{hex(answer[0])}"[2:]
                        if len(ss1) != 2:
                            ss1 = "0" + ss1
                        ss2 = f"{hex(answer[1])}"[2:]
                        if len(ss2) != 2:
                            ss2 = "0" + ss2
                        res = ss1 + ss2 + c1 + d1
                    if mine[i] == 0 and mine[j] == 2:
                        M1 = np.array([[1., 1.], [1., -1.]])
                        V1 = np.array(
                            [int(x1, 16) - int(b1, 16) - int(d1, 16), int(y1, 16) + 2 * int(d1, 16) - 2 * int(b1, 16)])
                        answer = np.linalg.solve(M1, V1).astype(
                            int)  # но тут у нас значения в десятичном виде, для результата нужно в шестнадцатиричную
                        ss1 = f"{hex(answer[0])}"[2:]
                        if len(ss1) != 2:
                            ss1 = "0" + ss1
                        ss2 = f"{hex(answer[1])}"[2:]
                        if len(ss2) != 2:
                            ss2 = "0" + ss2
                        res = ss1 + b1 + ss2 + d1
                    if mine[i] == 0 and mine[j] == 3:
                        M1 = np.array([[1., 1.], [1., -2.]])
                        V1 = np.array(
                            [int(x1, 16) - int(c1, 16) - int(b1, 16), int(y1, 16) - 2 * int(b1, 16) + int(c1, 16)])
                        answer = np.linalg.solve(M1, V1).astype(
                            int)  # но тут у нас значения в десятичном виде, для результата нужно в шестнадцатиричную
                        ss1 = f"{hex(answer[0])}"[2:]
                        if len(ss1) != 2:
                            ss1 = "0" + ss1
                        ss2 = f"{hex(answer[1])}"[2:]
                        if len(ss2) != 2:
                            ss2 = "0" + ss2
                        res = ss1 + b1 + c1 + ss2
                    if mine[i] == 1 and mine[j] == 2:
                        M1 = np.array([[1., 1.], [2., -1.]])
                        V1 = np.array(
                            [int(x1, 16) - int(a1, 16) - int(d1, 16), int(y1, 16) + 2 * int(d1, 16) - int(a1, 16)])
                        answer = np.linalg.solve(M1, V1).astype(
                            int)  # но тут у нас значения в десятичном виде, для результата нужно в шестнадцатиричную
                        res = a1 + f"{hex(answer[0])}"[2:] + f"{hex(answer[1])}"[2:] + d1
                    if mine[i] == 1 and mine[j] == 3:
                        M1 = np.array([[1., 1.], [2., -2.]])
                        V1 = np.array(
                            [int(x1, 16) - int(c1, 16) - int(a1, 16), int(y1, 16) - int(a1, 16) + int(c1, 16)])
                        answer = np.linalg.solve(M1, V1).astype(
                            int)  # но тут у нас значения в десятичном виде, для результата нужно в шестнадцатиричную
                        ss1 = f"{hex(answer[0])}"[2:]
                        if len(ss1) != 2:
                            ss1 = "0" + ss1
                        ss2 = f"{hex(answer[1])}"[2:]
                        if len(ss2) != 2:
                            ss2 = "0" + ss2
                        res = a1 + ss1 + c1 + ss2
                    if mine[i] == 2 and mine[j] == 3:
                        M1 = np.array([[1., 1.], [-1., -2.]])
                        V1 = np.array(
                            [int(x1, 16) - int(a1, 16) - int(b1, 16), int(y1, 16) - 2 * int(b1, 16) - int(a1, 16)])
                        answer = np.linalg.solve(M1, V1).astype(
                            int)  # но тут у нас значения в десятичном виде, для результата нужно в шестнадцатиричную
                        ss1 = f"{hex(answer[0])}"[2:]
                        if len(ss1) != 2:
                            ss1 = "0" + ss1
                        ss2 = f"{hex(answer[1])}"[2:]
                        if len(ss2) != 2:
                            ss2 = "0" + ss2
                        res = a1 + b1 + ss1 + ss2
    #else:
    print(res)


#cnt_iter = 0

def is_number(number : str):
    flag = True
    for s in number:
        if s not in "1234567890":
            flag = False
            break
    return flag


def is_empty(address: int):
    flag = False
    for i in range(0, 6):
        if disks[i] not in find_broken():
            file = open(disks[i], 'r')
            mas = file.readlines()
            print(mas[address])
            if mas[address] == '\n':
                flag = True
                #file.close()
                break
            #file.close()
    print(flag)
    return flag


def drop_file():
    while True:
        ind = str(input("Enter the number of the disc from 0 to 5 that you want to clear: "))
        if is_number(ind):
            if int(ind) > 5:
                print("Error, repeat entry ")
            else:
                break
    with open(disks[int(ind)], 'w'):
        pass


def read():
    if len(find_broken()) == 6:
        print("All discs are empty, you need to fill them up")
        return
    if len(find_broken()) > 2:
        print("More than two discs have failed, the information cannot be recovered")
        return
    else:
        while True:
            ind = str(input("Enter the index of the row from 0 to 63 from which you want to read the values: "))
            if is_number(ind):
                if int(ind) > 63:
                    print("Error, repeat entry ")
                else:
                    break
        to_solve(int(ind))


if __name__ == "__main__":
    # создаем диски и заполняем данные
    for i in disks:
        fill_file(i)

    while True:
        command = input("Choose an action: \n1. Write the value \n2. Read the value \n3. Clear the disc \n4. Exit the programme \n")
        if command == '1':
            write()
        elif command == '2':
            read()
        elif command == '3':
            drop_file()
        elif command == '4':
            break
        else:
            print("Error, repeat entry ")


