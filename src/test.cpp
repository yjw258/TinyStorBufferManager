#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <ctime>
#include "DSMgr.h"
#include "BMgr.h"
using namespace std;

int main()
{
    BMgr *bmgr = new BMgr();
    // 创建一个5万个page的堆文件，页号从0到49999
    bmgr->dsmgr.OpenFile("./data/data.dbf");
    for (int i = 0; i < 50000; i++)
    {
        bmgr->FixNewPage();
        bmgr->UnfixPage(i);
    }

    bmgr->WriteDirtys(); // 将脏页写回磁盘
    bmgr->dsmgr.CloseFile();
    delete bmgr;

    bmgr = new BMgr();
    // 读取trace文件
    bmgr->dsmgr.OpenFile("./data/data.dbf");
    ifstream trace("./data/data-5w-50w-zipf.txt");
    if (!trace.is_open())
    {
        cerr << "Error: Failed to open trace file." << endl;
        return -1;
    }

    // 统计运行时间
    clock_t start, end;
    start = clock();

    // 读取trace文件中的每一行
    string line;
    while (getline(trace, line))
    {
        // 读取trace文件中的每一行 mode,page_id
        int mode, page_id;
        sscanf(line.c_str(), "%d,%d", &mode, &page_id);
        page_id--;                    // 页号从0开始
        bmgr->FixPage(page_id, mode); // 读取或写入页面
        bmgr->UnfixPage(page_id);     // 释放页面
    }
    bmgr->WriteDirtys(); // 将脏页写回磁盘
    bmgr->dsmgr.CloseFile();
    trace.close();
    end = clock();

    // 输出统计信息
    cout << " IOCount: " << bmgr->dsmgr.GetIOCount() << endl;
    cout << " BufferHitRate: " << bmgr->GetBufferHitRate() << endl;
    cout << " Runtime: " << (double)(end - start) / CLOCKS_PER_SEC << "s" << endl;

    return 0;
}