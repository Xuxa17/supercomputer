#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <map>
#include <vector>
#include <unordered_map>
#include <omp.h>
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


void gen_File(vector<string> init_dict, int tid) {
    //int tid = omp_get_max_threads();
    //size_t part_size = 10000 / tid;
    vector<int> parts;
    parts_size(tid, file_size, parts);
    /*
    parts.reserve(tid);

    for (size_t i = 0; i < tid; ++i) {
        size_t start = i * part_size;
        size_t end = (i == tid - 1) ? 10000 : (i + 1) * part_size;
        parts.push_back(end - start);
    }
    */
    ofstream file;
    file.open(File);
    if (file.is_open())
    {
#pragma omp parallel for
        for (int z = 0; z < tid; z++) {
            srand(time(NULL));
            string str;
            for (int i = 0; i < parts[z]; i++) {
                str += alph[rand() % 25];
            }
            file << str;
            cout << "Номер потока: " << omp_get_thread_num() << endl;
        }
    }
    file.close();
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


//vector<int> my_cnts;

void LZW_coder_parallel(vector<string> init_dict, int numb, string& fin1, string& fout1, vector<int>& my_cnts) {
    //int threads_num = omp_get_max_threads();
    vector<string> parts = encode_parts(fin1, numb);
    vector<string> coded(numb, "");
    my_cnts.clear();

    /*
    for (int i = 0; i < numb; i++) {
        coded[i] = LZW_code(init_dict, parts[i]);
    }*/

#pragma omp parallel
    {
        int tid = omp_get_thread_num();
        coded[tid] = LZW_code(init_dict, parts[tid]);
        cout << "Номер потока: " << tid << endl;
    }

    ofstream fout;
    fout.open(fout1);
    for (const auto& code : coded) {
        my_cnts.push_back(code.size());
        //fout << code.size() << ' ';
    }

    //fout << '\n';

    for (const auto& code : coded) {
        fout << code;
    }
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


void LZW_decoder_parallel(vector<string> init_dict, string& fin1, string& fout1, vector<int>& my_cnts) {
    vector<string> parts = decode_parts(fin1, my_cnts);
    vector<string> decode(parts.size(), "");

#pragma omp parallel
    {
#pragma omp for
        for (int i = 0; i < parts.size(); i++) {
            decode[i] = LZW_decode(init_dict, parts[i]);
            cout << "Номер потока: " << omp_get_thread_num() << endl;
        }
    }

    ofstream fout;
    fout.open(fout1);
    for (const auto& part : decode) {
        fout << part;
    }
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




void RLE_coder_parallel(string& w, int numb, string& outt, vector<int>& my_cnts) {
    //int threads_num = omp_get_max_threads();
    vector<string> parts = encode_parts(w, numb);
    vector<string> res(numb, "");
    my_cnts.clear();

    /*
    for (int i = 0; i < numb; i++) {
        res[i] = RLE_code(parts[i]);
    }*/




#pragma omp parallel
    {
#pragma omp for
        for (int i = 0; i < numb; i++) {
            res[i] = RLE_code(parts[i]);
            cout << "Номер потока: " << omp_get_thread_num() << endl;
        }

        //int tid = omp_get_thread_num();
        //res[tid] = RLE_code(parts[tid]);
    }


    ofstream fout(outt);
    for (const auto& ress : res) {
        //fout << ress.size() << ' ';
        my_cnts.push_back(ress.size());
    }
    //fout << '\n';
    for (const auto& ress : res) {
        fout << ress;
    }
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

void RLE_decoder_parallel(string& fin, string& fout, vector<int>& my_cnts) {
    vector<string> parts = decode_parts(fin, my_cnts);
    vector<string> decoded(parts.size(), "");
#pragma omp parallel
    {
#pragma omp for
        for (int i = 0; i < parts.size(); i++) {
            decoded[i] = RLE_decode(parts[i]);
            cout << "Номер потока: " << omp_get_thread_num() << endl;
        }
    }

    ofstream fout1;
    fout1.open(fout);
    for (const auto& decode : decoded) {
        fout1 << decode;
    }
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
        cout << "Номер потока: " << omp_get_thread_num() << endl;
    }

    if (flag == true) {
        cout << "изначальный и декодированный файлы совпали" << endl;
    }
}


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



int main(int argc, const char* argv[]) {
    setlocale(LC_ALL, "Russian");
    int tid = omp_get_max_threads();
    // cout << tid << endl;
    cout << "Параллельная генерация строки в 10000 символов" << endl;
    gen_File(alph, tid);
    string my_str = getline_file(File);
    //cout << my_str.size() << endl;
    cout << "Файл с 10000 символами сгенерирован" << endl;
    cout << endl;




    LZW_coder(alph, File);
    cout << "Файл закодирован с помощью алгоритма LZW" << endl;
    LZW_decoder(alph, encode_File);
    cout << "Файл декодирован с помощью алгоритма LZW" << endl;
    string my_str1 = getline_file(decode_File);
    check(my_str, my_str1);
    cout << endl;

    cout << "Распареллеленное кодирование и декодирование файла при помощи алгоритма LZW" << endl;
    vector<int> my_cnt;
    LZW_coder_parallel(alph, tid, File, encode_File, my_cnt);
    cout << endl;
    LZW_decoder_parallel(alph, encode_File, decode_File, my_cnt);
    cout << endl;
    // check(my_str, getline_file(decode_File));
    cout << "Параллельная проверка" << endl;
    check_parallel(File, decode_File, tid);
    cout << endl;


    RLE_coder(File);
    cout << "Файл закодирован с помощью алгоритма RLE" << endl;
    RLE_decoder(rle_coder);
    cout << "Файл декодирован с помощью алгоритма RLE" << endl;
    string my_str2 = getline_file(_RLE_decoder);
    check(my_str, my_str2);
    cout << endl;

    //my_cnt.clear();
    cout << "Распараллеленное кодирование и декодирование файла при помощи алгоритма RLE" << endl;
    RLE_coder_parallel(File, tid, rle_coder, my_cnt);
    cout << endl;
    RLE_decoder_parallel(rle_coder, _RLE_decoder, my_cnt);
    cout << "Праллельная проверка" << endl;
    check_parallel(File, _RLE_decoder, tid);
    cout << endl;



    RLE_LZW_coder();
    cout << "Файл закодирован с помощью двуступенчатого кодирования RLE->LZW" << endl;
    RLE_LZW_decoder();
    cout << "Файл декодирован с помощью двуступенчатого декодирования LZW->RLE" << endl;
    string my_str3 = getline_file(_RLE_decoder);
    check(my_str, my_str3);
    cout << endl;

    cout << "Параллельное двуступенчатое кодирование и декодирование RLE->LZW" << endl;

    RLE_coder_parallel(File, tid, rle_coder, my_cnt);
    vector<int> my_cnt1;
    cout << endl;
    LZW_coder_parallel(alph, tid, rle_coder, encode_File, my_cnt1);
    cout << endl;
    LZW_decoder_parallel(alph, encode_File, decode_File, my_cnt1);
    cout << endl;
    RLE_decoder_parallel(decode_File, _RLE_decoder, my_cnt);
    cout << "Параллельная проверка" << endl;
    check_parallel(File, _RLE_decoder, tid);
    cout << endl;



    LZW_RLE_coder();
    cout << "Файл закодирован с помощью двуступенчатого кодирования LZW->RLE" << endl;
    LZW_RLE_decoder();
    cout << "Файл декодирован с помощью двуступенчатого декодирования RLE->LZW" << endl;
    string my_str4 = getline_file(decode_File);
    check(my_str, my_str4);

    cout << "Параллельное двуступенчатое кодирование и декодирование LZW->RLE" << endl;

    LZW_coder_parallel(alph, tid, File, encode_File, my_cnt);
    cout << endl;
    RLE_coder_parallel(encode_File, tid, rle_coder, my_cnt1);
    cout << endl;
    RLE_decoder_parallel(rle_coder, _RLE_decoder, my_cnt1);
    cout << endl;
    LZW_decoder_parallel(alph, _RLE_decoder, decode_File, my_cnt);
    cout << "Параллельная проверка" << endl;
    check_parallel(File, decode_File, tid);



}
