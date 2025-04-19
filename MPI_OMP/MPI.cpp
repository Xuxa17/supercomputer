#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <mpi.h>
#include <limits>
#include <cmath>
#include <sstream>
using namespace std;



vector<string> alph{ "f", "k", "p", "a", "o", "i", "F", "K", "P", "A", "O", "I"," ", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "%", ".", "," };

const unsigned short file_size = 10000;

string File("File.txt");
string encode_File("encode_File.txt");
string decode_File("decode_File.txt");
string rle_coder("rle_coder.txt");
string _RLE_decoder("RLE_decoder.txt");

int length_of_code(int size_of_dictionary) {
    int result = 0;
    int i = 1;
    while (i < size_of_dictionary) {
        result++;
        i *= 2;
    }
    return result;
}



string to_binary_string(int n)//просто перевод в двоичную систему
{
    string buffer;
    buffer.reserve(numeric_limits<unsigned int>::digits);
    do
    {
        buffer += char('0' + n % 2);
        n = n / 2;
    } while (n > 0);
    return string(buffer.crbegin(), buffer.crend());
}



string binary_res(int n, int length_of_dict) {//здесь уже двоичное число с тем количеством битов, которое нам нужно для представления
    string str = to_binary_string(n);
    int result = length_of_code(length_of_dict);

    int length = result - str.length();
    string str1(length, '0');
    str1 = str1 + str;
    return str1;
}



int from_binary(const string& str) {
    int sum = 0;
    for (int i = 0; i < str.size(); i++) {
        if (str[str.size() - i - 1] == '1') {
            sum += pow(2, i);
        }
    }
    return sum;
}


void parts_size(int tid, int size, vector<int>& parts) {
    size_t part_size = file_size / tid;
    parts.reserve(tid);

    for (size_t i = 0; i < tid; ++i) {
        size_t start = i * part_size;
        size_t end = (i == tid - 1) ? file_size : (i + 1) * part_size;
        parts.push_back(end - start);
    }
}


void gen_File(vector<string> init_dict) {
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int alphabet_size = init_dict.size();
    int local_size = file_size / size;  // размер задачи на процесс
    string local_buffer;

    if (rank == 0) {
        local_size += file_size % size;
    }

    // Генерация данных в каждом процессе
    srand(time(NULL) + rank);
    for (int i = 0; i < local_size; ++i) {
        local_buffer += init_dict[rand() % alphabet_size];
    }
    //      cout << local_buffer.size() << " " << rank << endl;
        // Сбор закодированных частей в процесс 0
    if (rank == 0) {
        vector<string> generated_parts(size);
        generated_parts[0] = std::move(local_buffer);
        for (int i = 1; i < size; i++) {
            MPI_Status status;
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            int count;
            MPI_Get_count(&status, MPI_CHAR, &count);
            generated_parts[i].resize(count);
            MPI_Recv(&generated_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Процесс 0 записывает собранные данные в файл
        ofstream file(File);
        for (const auto& gp : generated_parts) {
            file << gp;
        }
        file.close();
    }

    else {
        // Отправка сгенерированной части процессу 0
        MPI_Send(local_buffer.data(), local_buffer.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }


    printf("Номер потока: %d\n", rank);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}



vector<int> codes(14);
float dict_vol;

void plus_size(int place) {
    int tmp = codes[place];
    tmp++;
    codes[place] = tmp;
}

void LZW_coder(vector<string> init_dict, string w) {
    ifstream fin;
    ofstream fout;

    fin.open(w);
    fout.open(encode_File);


    string line1;
    if (fin.is_open())
    {
        getline(fin, line1);
    }

    unordered_map<string, int> dict;
    for (int i = 0; i < init_dict.size(); i++) {
        dict[init_dict[i]] = i;
    }

    string str = "";

    for (unsigned char symbol : line1) {
        str += symbol;

        if (dict.find(str) == dict.end()) {

            unsigned char eChar = str[str.size() - 1];
            str.pop_back();

            string cur_str = binary_res(dict[str], dict.size());
            fout << cur_str;
            plus_size(cur_str.size());


            str.push_back(eChar);
            dict.insert(make_pair(str, dict.size()));

            str = eChar;
        }
    }


    string cur_str = binary_res(dict[str], dict.size());
    fout << cur_str;
    plus_size(cur_str.size());

    dict_vol = dict.size();

    fin.close();
    fout.close();
}

string LZW_code(vector<string> init_dict, string& text) {
    int rank;

    //  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
      // cout << rank << endl;
    unordered_map<string, int> dict;
    string res = "";

    for (int i = 0; i < init_dict.size(); i++) {
        dict[init_dict[i]] = i;
    }

    string str = "";

    for (unsigned char symbol : text) {
        str += symbol;

        if (dict.find(str) == dict.end()) {

            unsigned char eChar = str[str.size() - 1];
            str.pop_back();

            string cur_str = binary_res(dict[str], dict.size());
            res += cur_str;
            plus_size(cur_str.size());


            str.push_back(eChar);
            dict.insert(make_pair(str, dict.size()));

            str = eChar;
        }
    }


    string cur_str = binary_res(dict[str], dict.size());
    res += cur_str;
    plus_size(cur_str.size());

    dict_vol = dict.size();
    return res;
}

vector<string> encode_parts(string& file, int numb) {//делим на части еще незакодированный файлик
    ifstream file1(file);

    string content((istreambuf_iterator<char>(file1)), istreambuf_iterator<char>());
    file1.close();


    size_t part_size = content.size() / numb;
    vector<string> parts;
    parts.reserve(numb);

    for (size_t i = 0; i < numb; ++i) {
        size_t start = i * part_size;
        size_t end = (i == numb - 1) ? content.size() : (i + 1) * part_size;
        parts.push_back(content.substr(start, end - start));
    }

    return parts;
}




void LZW_coder_parallel(vector<string> init_dict, string& fin_name, string& fout_name, vector<int>& my_cnts) {
    my_cnts.clear();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);//показывает какой именно поток
    MPI_Comm_size(MPI_COMM_WORLD, &size);//показывает сколько всего у нас потоков

    vector<string> parts;//храним части строки
    string part;//
    string coded_part;//закодированная строка (часть)

    if (rank == 0) {
        // Разделить файл только в процессе с рангом 0
        parts = encode_parts(fin_name, size);

        // Рассылка каждой части файла соответствующему процессу
        for (int i = 1; i < size; i++) {
            MPI_Send(parts[i].data(), parts[i].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
        part = parts[0];  // Часть для процесса 0
  //      cout << "thread 0" << endl;
    }

    else {
        MPI_Status status;
        MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
        int count;
        MPI_Get_count(&status, MPI_CHAR, &count);
        part.resize(count);
        MPI_Recv(&part[0], count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //    cout << rank << endl;
    }

    // Непосредственно кодировка
    if (rank == 0)  cout << "lzw " << rank << endl;
    coded_part = LZW_code(init_dict, part);

    // Сбор закодированных частей обратно в процесс 0
    if (rank == 0) {
        vector<string> coded_parts(size);
        coded_parts[0] = std::move(coded_part);
        for (int i = 1; i < size; i++) {
            MPI_Status status;
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            int count;
            MPI_Get_count(&status, MPI_CHAR, &count);
            coded_parts[i].resize(count);
            MPI_Recv(&coded_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Сохранение результатов
        ofstream fout(fout_name);
        for (const auto& cp : coded_parts) {
            //fout << cp.size() << ' ';
            my_cnts.push_back(cp.size());
        }
        for (const auto& cp : coded_parts) {
            fout << cp;
        }
        fout.close();
        //      cout << my_cnts[0] << " " << my_cnts[1] << " " << my_cnts[2] << endl;
    }

    else {
        // Отправка закодированной части обратно процессу 0
        MPI_Send(coded_part.data(), coded_part.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }


    printf("Номер потока: %d\n", rank);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


void LZW_decoder(vector<string> init_dict, string w) {
    ifstream fin;
    ofstream fout;

    fin.open(w);
    fout.open(decode_File);

    unordered_map<unsigned int, string> dict_1_a;//словарик, где ключ-цифра
    unordered_map<string, unsigned int> dict_a_1;//словарик, где ключ-буквы

    for (int i = 0; i < init_dict.size(); i++) {
        dict_1_a[i] = init_dict[i];
        dict_a_1[init_dict[i]] = i;
    }

    bool first_sym = 1;
    bool flag = 1;
    string str = "";

    string line1;
    if (fin.is_open())
    {
        getline(fin, line1);
    }
    int i = 0;
    while (i < line1.size()) {
        string subinput;

        flag = 1;

        if (dict_1_a.size() == 2 and first_sym) {
            subinput = line1.substr(i, 2);
            first_sym = 0;
            i += 2;
        }
        else {
            subinput = line1.substr(i, length_of_code(dict_1_a.size() + 1));
            i += length_of_code(dict_1_a.size() + 1);
        }

        //string binary_string(buff);
        string binary_string = subinput;

        if (binary_string == "") break;
        string tmp;

        if (from_binary(binary_string) == dict_1_a.size())
            tmp = str + str[0];
        else
            tmp = dict_1_a[from_binary(binary_string)];


        for (int i = 0; i < tmp.size(); i++) {
            str += tmp[i];

            if (dict_a_1.find(str) == dict_a_1.end() and flag) {

                unsigned char eChar = str[str.size() - 1];
                str.pop_back();

                fout << str;

                str.push_back(eChar);
                //dictionary.push_back(str);
                dict_1_a.insert(make_pair(dict_1_a.size(), str));
                dict_a_1.insert(make_pair(str, dict_a_1.size()));

                str = tmp.substr(i, tmp.size() - i);
                flag = false;
            }
            if (!flag) break;
        }
    }


    fout << str;

    fin.close();
    fout.close();
}

string LZW_decode(vector<string> init_dict, string text) {
    unordered_map<unsigned int, string> dict_1_a;//словарик, где ключ-цифра
    unordered_map<string, unsigned int> dict_a_1;//словарик, где ключ-буквы

    string res = "";

    for (int i = 0; i < init_dict.size(); i++) {
        dict_1_a[i] = init_dict[i];
        dict_a_1[init_dict[i]] = i;
    }

    bool first_sym = 1;
    bool flag = 1;

    string str = "";
    int i = 0;
    while (i < text.size()) {
        string subinput;

        flag = 1;

        if (dict_1_a.size() == 2 and first_sym) {
            subinput = text.substr(i, 2);
            first_sym = 0;
            i += 2;
        }
        else {
            subinput = text.substr(i, length_of_code(dict_1_a.size() + 1));
            i += length_of_code(dict_1_a.size() + 1);
        }

        //string binary_string(buff);
        string binary_string = subinput;

        if (binary_string == "") break;
        string tmp;

        if (from_binary(binary_string) == dict_1_a.size())
            tmp = str + str[0];
        else
            tmp = dict_1_a[from_binary(binary_string)];


        for (int i = 0; i < tmp.size(); i++) {
            str += tmp[i];

            if (dict_a_1.find(str) == dict_a_1.end() and flag) {

                unsigned char eChar = str[str.size() - 1];
                str.pop_back();

                res += str;

                str.push_back(eChar);
                //dictionary.push_back(str);
                dict_1_a.insert(make_pair(dict_1_a.size(), str));
                dict_a_1.insert(make_pair(str, dict_a_1.size()));

                str = tmp.substr(i, tmp.size() - i);
                flag = false;
            }
            if (!flag) break;
        }
    }

    res += str;
    return res;
}

vector<string> decode_parts(string& file, vector<int>& numbers) {//делим на части закодированный файлик
    ifstream fin(file);
    string line;

    getline(fin, line);
    fin.close();

    vector<string> res(numbers.size());
    int cur_pos = 0;
    for (int i = 0; i < res.size(); i++) {
        res[i] = line.substr(cur_pos, numbers[i]);
        cur_pos += numbers[i];
    }

    return res;
}


void LZW_decoder_parallel(vector<string> init_dict, string& fin_name, string& fout_name, vector<int>& my_cnts) {
    //vector<string> parts = decode_parts(fin1, my_cnts);
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    vector<string> parts;
    string part;
    string decoded_part;

    if (rank == 0) {
        // Разделить файл только в процессе с рангом 0
        //std::vector<std::string> parts = split_file_decoder(fin_name);
        parts = decode_parts(fin_name, my_cnts);

        // Рассылка каждой части файла соответствующему процессу
        for (int i = 1; i < size; i++) {
            MPI_Send(parts[i].data(), parts[i].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
        part = parts[0];  // Часть для процесса 0
    }

    else {
        MPI_Status status;
        MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
        int count;
        MPI_Get_count(&status, MPI_CHAR, &count);
        part.resize(count);
        MPI_Recv(&part[0], count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    decoded_part = LZW_decode(init_dict, part);

    // Сбор декодированных частей обратно в процесс 0
    if (rank == 0) {
        vector<string> decoded_parts(size);
        decoded_parts[0] = std::move(decoded_part);
        for (int i = 1; i < size; i++) {
            MPI_Status status;
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            int count;
            MPI_Get_count(&status, MPI_CHAR, &count);
            decoded_parts[i].resize(count);
            MPI_Recv(&decoded_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Сохранение результатов
        std::ofstream fout(fout_name, std::ios::binary);
        for (const auto& dp : decoded_parts) {
            fout << dp;
        }
        fout.close();
    }

    else {
        // Отправка закодированной части обратно процессу 0
        MPI_Send(decoded_part.data(), decoded_part.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }


    printf("Номер потока: %d\n", rank);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}



void RLE_coder(string w) {
    ifstream fin;
    ofstream fout;

    fin.open(w);
    fout.open(rle_coder);

    string my_str;
    if (fin.is_open())
    {
        getline(fin, my_str);
    }

    int tmp = 1;//повторения
    string res;
    int length = my_str.size();



    for (int i = 0; i < my_str.size() - 1; i++) {
        for (int j = i + 1; j <= my_str.size(); j++) {
            if (my_str[i] == my_str[j]) {
                tmp++;
                length--;
            }
            else {
                res += my_str[i];
                res += to_string(tmp);
                res.push_back(',');//добавляю разделитель, чтобы потом было удобно считывать строки
                tmp = 1;
                i = j;
            }
        }
    }

    fout << res;
    fout.close();
    fin.close();
}


string RLE_code(string& text) {
    int tmp = 1;//повторения
    string res;
    int length = text.size();

    for (int i = 0; i < text.size() - 1; i++) {
        for (int j = i + 1; j <= text.size(); j++) {
            if (text[i] == text[j]) {
                tmp++;
                length--;
            }
            else {
                res += text[i];
                res += to_string(tmp);
                res.push_back(',');//добавляю разделитель, чтобы потом было удобно считывать строки
                tmp = 1;
                i = j;
            }
        }
    }

    return res;
}




void RLE_coder_parallel(string& fin_name, string& fout_name, vector<int>& my_cnts) {
    my_cnts.clear();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);//показывает какой именно поток
    MPI_Comm_size(MPI_COMM_WORLD, &size);//показывает сколько всего у нас потоков

    vector<string> parts;//храним части строки
    string part;//
    string coded_part;//закодированная строка (часть)

    if (rank == 0) {
        // Разделить файл только в процессе с рангом 0
        parts = encode_parts(fin_name, size);

        // Рассылка каждой части файла соответствующему процессу
        // MPI_Scatter ожидает, что все элементы будут одинакового размера, поэтому используем Send/Recv
        for (int i = 1; i < size; i++) {
            MPI_Send(parts[i].data(), parts[i].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }//то есть в этом цикле мы отправляем вот эти штуки на все потоки начиная с 1 (изначально потоки начинаются с 0)
        part = parts[0];  // Часть для процесса 0
    }

    else {
        MPI_Status status;//состояние полученного сообщения
        MPI_Probe(0, 0, MPI_COMM_WORLD, &status);//проверка блокировки сообщения /источник/значение тега/коммуникатор/объект status
        int count;
        MPI_Get_count(&status, MPI_CHAR, &count);//количство элементов первого уровня   /состояние получения/тип данных/число полученных элементов
        part.resize(count);
        MPI_Recv(&part[0], count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //указатель на буфер, содержащий отправляеме данные/кол-во эл-ов в буфере/тип данных/ранг проуесса отправки/тег для различения разных типов/коммуникатор/инф-ия о статусе
    }

    // Непосредственно кодировка
    coded_part = RLE_code(part);

    // Сбор закодированных частей обратно в процесс 0
    if (rank == 0) {
        vector<string> coded_parts(size);
        coded_parts[0] = std::move(coded_part);
        for (int i = 1; i < size; i++) {
            MPI_Status status;
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            int count;
            MPI_Get_count(&status, MPI_CHAR, &count);
            coded_parts[i].resize(count);
            MPI_Recv(&coded_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Сохранение результатов
        ofstream fout(fout_name);
        for (const auto& cp : coded_parts) {
            //fout << cp.size() << ' ';
            my_cnts.push_back(cp.size());//заполняю вектор с длинами
        }

        for (const auto& cp : coded_parts) {
            fout << cp;
        }
        fout.close();
    }

    else {
        // Отправка закодированной части обратно процессу 0
        MPI_Send(coded_part.data(), coded_part.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    printf("Номер потока: %d\n", rank);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}




void RLE_decoder(string w) {
    ifstream fin;
    ofstream fout;

    fin.open(w);
    fout.open(_RLE_decoder);

    string my_str;
    if (fin.is_open())
    {
        getline(fin, my_str);
    }


    string res;

    for (int i = 0; i < my_str.size() - 1; i++) {
        int counter = 1;
        string number;
        for (int j = i + 1; j <= my_str.size(); j++) {
            if (my_str[j] == ',') {
                break;
            }
            else {
                number += my_str[j];
                counter++;
            }
        }
        int new_cnt = stoi(number);
        while (new_cnt > 0) {
            res += my_str[i];
            new_cnt--;
        }
        i += counter;
    }

    fout << res;

    fout.close();
    fin.close();

}


string RLE_decode(string& text) {
    string res;

    for (int i = 0; i < text.size() - 1; i++) {
        int counter = 1;
        string number;
        for (int j = i + 1; j <= text.size(); j++) {
            if (text[j] == ',') {
                break;
            }
            else {
                number += text[j];
                counter++;
            }
        }
        int new_cnt = stoi(number);
        while (new_cnt > 0) {
            res += text[i];
            new_cnt--;
        }
        i += counter;
    }

    return res;
}

void RLE_decoder_parallel(string& fin_name, string& fout_name, vector<int>& my_cnts) {
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    vector<string> parts;
    string part;
    string decoded_part;

    if (rank == 0) {
        // Разделить файл только в процессе с рангом 0
        //std::vector<std::string> parts = decode_parts(fin, my_cnts);
        parts = decode_parts(fin_name, my_cnts);

        // Рассылка каждой части файла соответствующему процессу
        for (int i = 1; i < size; i++) {
            MPI_Send(parts[i].data(), parts[i].size(), MPI_CHAR, i, 0, MPI_COMM_WORLD);
        }
        part = parts[0];  // Часть для процесса 0
    }

    else {
        MPI_Status status;
        MPI_Probe(0, 0, MPI_COMM_WORLD, &status);
        int count;
        MPI_Get_count(&status, MPI_CHAR, &count);
        part.resize(count);
        MPI_Recv(&part[0], count, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    decoded_part = RLE_decode(part);

    // Сбор декодированных частей обратно в процесс 0
    if (rank == 0) {
        vector<string> decoded_parts(size);
        decoded_parts[0] = std::move(decoded_part);
        for (int i = 1; i < size; i++) {
            MPI_Status status;
            MPI_Probe(i, 0, MPI_COMM_WORLD, &status);
            int count;
            MPI_Get_count(&status, MPI_CHAR, &count);
            decoded_parts[i].resize(count);
            MPI_Recv(&decoded_parts[i][0], count, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        // Сохранение результатов
        ofstream fout(fout_name, std::ios::binary);
        for (const auto& dp : decoded_parts) {
            fout << dp;
        }
        fout.close();
    }

    else {
        // Отправка закодированной части обратно процессу 0
        MPI_Send(decoded_part.data(), decoded_part.size(), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
    }

    printf("Номер потока: %d\n", rank);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}


void check(const string& str1, const string& str2) {
    bool flag = true;

    if (str1.size() == str2.size()) {
        for (int i = 0; i < str1.size(); i++) {
            if (str1[i] != str2[i]) {
                flag = false;
                break;
            }
        }
    }
    else {
        flag = false;
    }

    if (flag == true) {
        cout << "Изначальный и декодированный файлы совпали" << endl;
    }
    else {
        cout << "Файлы разные, возникла ошибка" << endl;
    }

}

/*
bool check_part(const string& str1, const string& str2) {
    bool flag = true;

    if (str1.size() == str2.size()) {
        for (int i = 0; i < str1.size(); i++) {
            if (str1[i] != str2[i]) {
                flag = false;
                break;
            }
        }
    }
    else {
        flag = false;
    }

    return flag;
}


void check_parallel(string& file1, string& file2, int tid) {
    vector<string> str1 = encode_parts(file1, tid);
    vector<string> str2 = encode_parts(file2, tid);

    bool flag = true;
    #pragma omp parallel for
    for (int i = 0; i < tid; i++) {
        if (check_part(str1[i], str2[i]) == false) {
            flag = false;
            cout << "error" << endl;
          //  break;
        }
        cout <<"Номер потока: " << omp_get_thread_num() << endl;
    }

    if (flag == true) {
        cout << "изначальный и декодированный файлы совпали" << endl;
    }
}

*/

string getline_file(string str) {
    string gett;
    ifstream fin1;
    fin1.open(str);
    if (fin1.is_open())
    {
        getline(fin1, gett);
    }
    return gett;
}


void RLE_LZW_coder() {
    RLE_coder(File);
    LZW_coder(alph, rle_coder);
}
void RLE_LZW_decoder() {
    LZW_decoder(alph, encode_File);
    RLE_decoder(decode_File);
}



void LZW_RLE_coder() {
    LZW_coder(alph, File);
    RLE_coder(encode_File);
}
void LZW_RLE_decoder() {
    RLE_decoder(rle_coder);
    LZW_decoder(alph, _RLE_decoder);
}



int size_f(string w) {
    fstream file(w);
    int size = 0;
    file.seekg(0, ios::end);
    size = file.tellg();
    //cout << "Вашь фаил весит : " << size << " баит" << endl;
    file.close();
    return size;
}

float code_l() {//цена кодирования
    float tmp = 0;
    float vol = 0;
    float res = 0;
    for (int i = 0; i < codes.size(); i++) {
        vol += codes[i];//тип объем словарика, а именно сколько кодов мы добавили в файлик
    }
    for (float i = 0; i < codes.size(); i++) {
        res += (codes[i] / dict_vol) * (i);//вероятность, что символ закодируется кодом такой длины, умножаем на длину кода
        //тут размер словарика тип вообще весь, даже те слова которые закодированы в словарике но не использовались
    }
    return res;
}



int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    setlocale(LC_ALL, "Russian");
    srand(time(0));
    //int tid = omp_get_max_threads();
   // cout << tid << endl;

    if (rank == 0) cout << "Параллельная генерация строки в 10000 символов" << endl;
    gen_File(alph);
    string my_str = getline_file(File);
    //cout << my_str.size() << endl;

 //  cout << "Файл с 10000 символами сгенерирован" << endl;
    if (rank == 0) cout << endl;
    // cout << my_str << endl;

/*

    LZW_coder(alph, File);
    if(rank ==0) cout << "Файл закодирован с помощью алгоритма LZW" << endl;
    LZW_decoder(alph, encode_File);
    if(rank ==0) cout << "Файл декодирован с помощью алгоритма LZW" << endl;
    string my_str1 = getline_file(decode_File);
    if(rank ==0) check(my_str, my_str1);
    if(rank ==0) cout << endl;
*/

    if (rank == 0) cout << "Распареллеленное кодирование и декодирование файла при помощи алгоритма LZW" << endl;
    vector<int> my_cnt;
    LZW_coder_parallel(alph, File, encode_File, my_cnt);
    if (rank == 0) cout << endl;
    LZW_decoder_parallel(alph, encode_File, decode_File, my_cnt);
    if (rank == 0) cout << endl;
    if (rank == 0) check(my_str, getline_file(decode_File));
    //cout << "Параллельная проверка" << endl;
    //check_parallel(File, decode_File, tid);
    if (rank == 0) cout << endl;

    /*
        RLE_coder(File);
        if(rank ==0) cout << "Файл закодирован с помощью алгоритма RLE" << endl;
        RLE_decoder(rle_coder);
        if(rank ==0) cout << "Файл декодирован с помощью алгоритма RLE" << endl;
        string my_str2 = getline_file(_RLE_decoder);
        if(rank ==0) check(my_str, my_str2);
        if(rank ==0) cout << endl;
    */
    //my_cnt.clear();
    if (rank == 0) cout << "Распараллеленное кодирование и декодирование файла при помощи алгоритма RLE" << endl;
    RLE_coder_parallel(File, rle_coder, my_cnt);
    if (rank == 0) cout << endl;
    RLE_decoder_parallel(rle_coder, _RLE_decoder, my_cnt);
    //cout << "Праллельная проверка" << endl;
    if (rank == 0) check(my_str, getline_file(_RLE_decoder));
    if (rank == 0) cout << endl;


    /*
        RLE_LZW_coder();
        if(rank ==0) cout << "Файл закодирован с помощью двуступенчатого кодирования RLE->LZW" << endl;
        RLE_LZW_decoder();
        if(rank ==0) cout << "Файл декодирован с помощью двуступенчатого декодирования LZW->RLE" << endl;
       // string my_str3 = getline_file(_RLE_decoder);
        if(rank ==0) check(my_str, getline_file(_RLE_decoder));
        if(rank ==0) cout << endl;
    */
    if (rank == 0) cout << "Параллельное двуступенчатое кодирование и декодирование RLE->LZW" << endl;

    RLE_coder_parallel(File, rle_coder, my_cnt);
    vector<int> my_cnt1;
    if (rank == 0) cout << endl;
    LZW_coder_parallel(alph, rle_coder, encode_File, my_cnt1);
    if (rank == 0) cout << endl;

    LZW_decoder_parallel(alph, encode_File, decode_File, my_cnt1);
    if (rank == 0) cout << endl;
    RLE_decoder_parallel(decode_File, _RLE_decoder, my_cnt);
    //cout << "Параллельная проверка" << endl;
    if (rank == 0) check(my_str, getline_file(_RLE_decoder));
    if (rank == 0) cout << endl;


    /*
        LZW_RLE_coder();
        if(rank ==0) cout << "Файл закодирован с помощью двуступенчатого кодирования LZW->RLE" << endl;
        LZW_RLE_decoder();
        if(rank ==0) cout << "Файл декодирован с помощью двуступенчатого декодирования RLE->LZW" << endl;
       // string my_str4 = getline_file(decode_File);
        if(rank ==0) check(my_str, getline_file(decode_File));
    */
    if (rank == 0) cout << "Параллельное двуступенчатое кодирование и декодирование LZW->RLE" << endl;

    LZW_coder_parallel(alph, File, encode_File, my_cnt);
    if (rank == 0) cout << endl;
    RLE_coder_parallel(encode_File, rle_coder, my_cnt1);
    if (rank == 0) cout << endl;
    RLE_decoder_parallel(rle_coder, _RLE_decoder, my_cnt1);
    if (rank == 0) cout << endl;
    LZW_decoder_parallel(alph, _RLE_decoder, decode_File, my_cnt);
    //cout << "Параллельная проверка" << endl;
    if (rank == 0) check(my_str, getline_file(decode_File));

    MPI_Finalize();
