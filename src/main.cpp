#include <iostream>
// #include "memory_node_keeper.h"
#include "rdma.h"
#include <istream>
#include <streambuf>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdlib.h>
using namespace std;

string vector2string(vector<vector<string>> &file, int id1, int id2)
{
    string out = "{";
    long unsigned i, j;
    for (i = id1; i < file.size() && i <= id2; i++)
    {
        string outp = "{";
        for (j = 0; j < file[i].size(); j++)
        {
            outp += to_string(j) + ":" + file[i][j];
            if (j != file[i].size() - 1)
            {
                outp += ",";
            }
        }
        outp += "}";
        out += outp;
        if (i < id2)
        {
            out += ",";
        }
    }
    out += "}";
    cout << "vector2string " << out << endl;
    return out;
}

void work(char *sn, vector<vector<string>> &file)
{
    struct DSMEngine::config_t config = {
        NULL,  /* dev_name */
        sn,    /* server_name */
        19876, /* tcp_port */
        1,     /* ib_port */
        -1 /* gid_idx */};
    int rc = 1;
    char temp_char;
    char temp_send[] = "R";

    shared_ptr<DSMEngine::RDMA_Manager> rdma_mg_1 = shared_ptr<DSMEngine::RDMA_Manager>();
    rdma_mg_1 = make_shared<DSMEngine::RDMA_Manager>(config);
    shared_ptr<DSMEngine::RDMA_Manager> rdma_mg_2 = shared_ptr<DSMEngine::RDMA_Manager>();
    rdma_mg_2 = make_shared<DSMEngine::RDMA_Manager>(config);
    if (rdma_mg_1->init())
    {
        fprintf(stderr, "failed to init resources\n");
        rc = 1;
        goto main_exit;
    }

    if (rdma_mg_2->init())
    {
        fprintf(stderr, "failed to init resources\n");
        rc = 1;
        goto main_exit;
    }
    char order[30];
    int id1, id2;
    while (1)
    {
        fscanf(stdin, "%s", order);
        if (0 == strcmp(order, "read"))
        {
            fprintf(stdout, "read order solved\n");
            fscanf(stdin, "%d %d", &id1, &id2);
            string out_string = "read," + to_string(id1) + "," + to_string(id2) + ",";
            char *out_chars = const_cast<char *>(out_string.c_str());

            strcpy(rdma_mg_1->res->buf, out_chars);
            fprintf(stdout, "going to send the message: '%s'\n", rdma_mg_1->res->buf);
            /* let the server post the sr */
            if (rdma_mg_1->RDMA_Send())
            {
                fprintf(stderr, "failed to rdma_mg_1 RDMA_Send\n");
                rc = 1;
                goto main_exit;
            }

            if (rdma_mg_1->RDMA_Receive())
            {
                fprintf(stderr, "failed to rdma_mg_1 RDMA_Receive\n");
                rc = 1;
                goto main_exit;
            }
            if (0 == strcmp(rdma_mg_1->res->buf, "already"))
            {
                fprintf(stdout, "Message is: '%s', %ld\n", rdma_mg_1->res->buf, strlen(rdma_mg_1->res->buf));
            }
            else
            {
                fprintf(stderr, "failed receive already\n");
                rc = 1;
                goto main_exit;
            }
            if (rdma_mg_2->RDMA_Read())
            {
                fprintf(stderr, "failed to rdma_mg_2 RDMA_Read\n");
                rc = 1;
                goto main_exit;
            }
            fprintf(stdout, "Contents of server's buffer: '%s'\n", rdma_mg_2->res->buf);
            if (rdma_mg_1->sock_sync_data(rdma_mg_1->res->sock, 1, temp_send, &temp_char)) /* just send a dummy char back and forth */
            {
                fprintf(stderr, "sync error after RDMA ops\n");
                rc = 1;
                goto main_exit;
            }
            if (rdma_mg_2->sock_sync_data(rdma_mg_2->res->sock, 1, temp_send, &temp_char)) /* just send a dummy char back and forth */
            {
                fprintf(stderr, "sync error after RDMA ops\n");
                rc = 1;
                goto main_exit;
            }
        }
        else if (0 == strcmp(order, "write"))
        {
            fprintf(stdout, "write order solved\n");
            fscanf(stdin, "%d %d", &id1, &id2);

            strcpy(rdma_mg_1->res->buf, "write,");
            fprintf(stdout, "going to send the message: '%s'\n", rdma_mg_1->res->buf);
            /* let the server post the sr */
            if (rdma_mg_1->RDMA_Send())
            {
                fprintf(stderr, "failed to rdma_mg_1 RDMA_Send\n");
                rc = 1;
                goto main_exit;
            }

            string out_string = "write " + vector2string(file, id1, id2);
            char *out_chars = const_cast<char *>(out_string.c_str());
            strcpy(rdma_mg_2->res->buf, out_chars);
            fprintf(stdout, "Now replacing it with: '%s'\n", rdma_mg_2->res->buf);
            if (rdma_mg_2->RDMA_Write())
            {
                fprintf(stderr, "failed to rdma_mg_2 RDMA_Write\n");
                rc = 1;
                goto main_exit;
            }
            if (rdma_mg_1->sock_sync_data(rdma_mg_1->res->sock, 1, temp_send, &temp_char)) /* just send a dummy char back and forth */
            {
                fprintf(stderr, "sync error after RDMA ops\n");
                rc = 1;
                goto main_exit;
            }
            if (rdma_mg_2->sock_sync_data(rdma_mg_2->res->sock, 1, temp_send, &temp_char)) /* just send a dummy char back and forth */
            {
                fprintf(stderr, "sync error after RDMA ops\n");
                rc = 1;
                goto main_exit;
            }
        }
        else if (0 == strcmp(order, "exit"))
        {
            fprintf(stdout, "exit order solved\n");
            break;
        }
        else
        {
            fprintf(stdout, "wrong order\n");
        }
    }

    rc = 0;
main_exit:
    if (rdma_mg_1->resources_destroy())
    {
        fprintf(stderr, "failed to destroy resources\n");
        rc = 1;
    }
    if (rdma_mg_2->resources_destroy())
    {
        fprintf(stderr, "failed to destroy resources\n");
        rc = 1;
    }
    if (rdma_mg_1->config.dev_name)
        free((char *)rdma_mg_1->config.dev_name);
    if (rdma_mg_2->config.dev_name)
        free((char *)rdma_mg_2->config.dev_name);
    fprintf(stdout, "\ntest result is %d\n", rc);
    // return rc;
}

#include <map>

void csv_data_load(char *filename, vector<vector<string>> &file)
{
    ifstream csv_data(filename);
    string line;
    if (!csv_data.is_open())
    {
        cout << "Error: opening file fail" << endl;
        exit(1);
    }
    istringstream sin; //将整行字符串line读入到字符串istringstream中
    // vector<vector<string>> file;
    vector<string> words; //声明一个字符串向量
    string word;

    // 读取标题行
    // getline(csv_data, line);
    // 读取数据
    while (getline(csv_data, line))
    {
        sin.clear();
        sin.str(line);
        words.clear();
        while (getline(sin, word, ',')) //将字符串流sin中的字符读到field字符串中，以逗号为分隔符
        {
            words.push_back(word); //将每一格中的数据逐个push
            // cout << word;
            // cout << atol(word.c_str());
        }
        file.push_back(words);
        // cout << endl;
        // do something。。。
    }
    csv_data.close();
}

void csv_write()
{
    // char *fn = "madedata.csv";
    ofstream outFile;
    outFile.open("test.csv", ios::out | ios::trunc);
    for (int x = 1; x <= 10000; x++)
    {
        std::string str = "";
        for (int i = 1; i <= 50; i++)
        {
            int flag;
            flag = rand() % 2;                         //随机使flag为1或0，为1就是大写，为0就是小写
            if (flag == 1)                             //如果flag=1
                str += rand() % ('Z' - 'A' + 1) + 'A'; //追加大写字母的ascii码
            else
                str += rand() % ('z' - 'a' + 1) + 'a'; //如果flag=0，追加为小写字母的ascii码
        }
        outFile << to_string(x * 10000 + x) << "," << str << endl;
    }
    outFile.close();
}

int main(int argc, char *argv[])
{
    vector<vector<string>> file;
    csv_data_load("test.csv", file);
    work(argv[argc - 1], file);
    // work(argv[argc-1]);
    // char *servername = argv[argc - 1];
    // csv_data_load("test.csv");
    // csv_write();
    return 0;
}
