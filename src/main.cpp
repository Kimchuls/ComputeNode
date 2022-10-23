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
void work(char *sn)
{
    struct DSMEngine::config_t config = {
        NULL,  /* dev_name */
        sn,    /* server_name */
        19876, /* tcp_port */
        1,     /* ib_port */
        -1 /* gid_idx */};
    shared_ptr<DSMEngine::RDMA_Manager> rdma_mg = shared_ptr<DSMEngine::RDMA_Manager>();
    rdma_mg = make_shared<DSMEngine::RDMA_Manager>(config);

    int rc = 1;
    char temp_char;
    char temp_send[] = "R";
    if (rdma_mg->init())
    {
        fprintf(stderr, "failed to init resources\n");
        goto main_exit;
    }

    if (rdma_mg->RDMA_Receive())
    {
        fprintf(stderr, "failed to connect QPs\n");
        goto main_exit;
    }
    /* after polling the completion we have the message in the client buffer too */
    fprintf(stdout, "Message is: '%s', %ld\n", rdma_mg->res->buf, strlen(rdma_mg->res->buf));

    strcpy(rdma_mg->res->buf, "reverse send test");
    fprintf(stdout, "going to send the message: '%s'\n", rdma_mg->res->buf);
    /* let the server post the sr */
    if (rdma_mg->RDMA_Send())
    {
        fprintf(stderr, "failed to post sr\n");
        goto main_exit;
    }

    /*-------------------------------------------------------------------------------------------------------------------------------------------------------*/

    strcpy(rdma_mg->res->buf, "3522222222");
    if (rdma_mg->sock_sync_data(rdma_mg->res->sock, 1, temp_send, &temp_char)) /* just send a dummy char back and forth */
    {
        fprintf(stderr, "sync error before RDMA ops\n");
        rc = 1;
        goto main_exit;
    }

    /*-------------------------------------------------------------------------------------------------------------------------------------------------------*/
    strcpy(rdma_mg->res->buf, "3522222222");
    if (rdma_mg->RDMA_Read())
    {
        fprintf(stderr, "failed to RDMA_Read 2\n");
        rc = 1;
        goto main_exit;
    }
    fprintf(stdout, "Contents of server's buffer: '%s'\n", rdma_mg->res->buf);

    strcpy(rdma_mg->res->buf, "354444444444444");
    fprintf(stdout, "Now replacing it with: '%s'\n", rdma_mg->res->buf);
    if (rdma_mg->RDMA_Write())
    {
        fprintf(stderr, "failed to RDMA_Write\n");
        rc = 1;
        goto main_exit;
    }

    strcpy(rdma_mg->res->buf, "358888");
    fprintf(stdout, "Now replacing it with: '%s'\n", rdma_mg->res->buf);
    if (rdma_mg->RDMA_Write())
    {
        fprintf(stderr, "failed to RDMA_Write\n");
        rc = 1;
        goto main_exit;
    }

    strcpy(rdma_mg->res->buf, "36666666666");
    if (rdma_mg->RDMA_Read())
    {
        fprintf(stderr, "failed to RDMA_Read 2\n");
        rc = 1;
        goto main_exit;
    }
    fprintf(stdout, "Contents of server's buffer: '%s'\n", rdma_mg->res->buf);

    rc = 0;
main_exit:
    if (rdma_mg->resources_destroy())
    {
        fprintf(stderr, "failed to destroy resources\n");
        rc = 1;
    }
    if (config.dev_name)
        free((char *)rdma_mg->config.dev_name);
    fprintf(stdout, "\ntest result is %d\n", rc);
    // return rc;
}

#include <map>

void csv_data_load(char *filename)
{
    ifstream csv_data(filename);
    string line;
    if (!csv_data.is_open())
    {
        cout << "Error: opening file fail" << endl;
        exit(1);
    }
    istringstream sin;    //将整行字符串line读入到字符串istringstream中
    vector<string> words; //声明一个字符串向量
    string word;

    // 读取标题行
    getline(csv_data, line);
    // 读取数据
    while (getline(csv_data, line))
    {
        sin.clear();
        sin.str(line);
        words.clear();
        while (getline(sin, word, ',')) //将字符串流sin中的字符读到field字符串中，以逗号为分隔符
        {
            words.push_back(word); //将每一格中的数据逐个push
            cout << word;
            // cout << atol(word.c_str());
        }
        cout << endl;
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
    work(argv[argc - 1]);
    work(argv[argc-1]);
    // char *servername = argv[argc - 1];
    // csv_data_load("test.csv");
    // csv_write();
    return 0;
}
