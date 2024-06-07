#include "DSMgr.h"

bFrame buf[DEFBUFSIZE]; // buffer空间

int IOCount; // 记录 IO 次数

// 构造函数，初始化文件指针、页面数量、页面使用信息、IO次数
DSMgr::DSMgr()
{
    currFile = NULL;
    numPages = 0;
    IOCount = 0;
    for (int i = 0; i < MAXPAGES; ++i)
    {
        pages[i] = 0;
    }
}

// 打开文件
int DSMgr::OpenFile(std::string filename)
{
    // 检查文件是否已打开
    if (currFile)
    {
        std::cerr << "Error: File already open." << std::endl;
        return -1; 
    }

    // 尝试打开文件
    currFile = fopen(filename.c_str(), "rb+");
    if (!currFile)
    {
        std::cerr << "Error: Failed to open file." << std::endl;
        return -2; 
    }

    // 计算页面数量
    fseek(currFile, 0, SEEK_END);
    numPages = ftell(currFile) / sizeof(bFrame);

    return 0; 
}

// 关闭文件
int DSMgr::CloseFile()
{
    // 检查文件是否已打开
    if (!currFile)
    {
        std::cerr << "Error:  open." << std::endl;
        return -1;
    }

    // 尝试关闭文件
    if (fclose(currFile) != 0) 
    {
        return -1;
    }
    
    // 重置文件指针、页面数量以及页面使用信息
    currFile = NULL;
    numPages = 0;
    for (int i = 0; i < MAXPAGES; i++)
    {
        pages[i] = 0;
    }

    return 0;
}

// 读取页面，返回 frame
bFrame DSMgr::ReadPage(int page_id)
{
    bFrame frame;
    
    // 检查文件是否已打开
    if (!currFile)
    {
        std::cerr << "Error: File not open." << std::endl;
        return frame;
    }

    // 检查页面号是否合法
    if (page_id < 0 || page_id >= numPages)
    {
        std::cerr << "Error: Invalid page_id." << std::endl;
        return frame;
    }

    // 将文件指针移动到正确的位置
    if (fseek(currFile, page_id * sizeof(bFrame), SEEK_SET) != 0)
    {
        std::cerr << "Error: Failed to move file pointer for reading." << std::endl;
        return frame;
    }

    // 读取页面
    if (fread(&frame, sizeof(bFrame), 1, currFile) != 1)
    {
        std::cerr << "Error: Failed to read data from file." << std::endl;
        return frame;
    }

    // 读取成功，增加 IO 次数
    IOCount++;
    return frame; 
}

// 写入页面，返回写入的字节数
int DSMgr::WritePage(int page_id, bFrame frm)
{ 
    // 检查文件是否已打开
    if (!currFile)
    {
        std::cerr << "Error: File not open for writing." << std::endl;
        return -1; 
    }

    // 将文件指针移动到对应位置
    if (fseek(currFile, page_id * sizeof(bFrame), SEEK_SET) != 0)
    {
        std::cerr << "Error: Failed to move file pointer for writing." << std::endl;
        return -3; 
    }

    // 写入页面
    if (fwrite(&frm, sizeof(bFrame), 1, currFile) != 1)
    {
        std::cerr << "Error: Failed to write data to file." << std::endl;
        return -4; // 返回错误码
    }

    fflush(currFile); // 刷新缓冲区
    IOCount++; // 增加 IO 次数
    return sizeof(bFrame); 
}

// 移动文件指针
int DSMgr::Seek(int offset, int pos)
{
    // 检查文件是否已打开
    if (!currFile)
    {
        std::cerr << "Error: File not open." << std::endl;
        return -1;
    }

    // 将文件指针移动到对应位置
    if (fseek(currFile, offset + pos, SEEK_SET) != 0)
    {
        std::cerr << "Error: Failed to move file pointer." << std::endl;
        return -2;
    }

    return 0;
}

// 返回文件指针
FILE *DSMgr::GetFile()
{
    return currFile;
}

// 增加页面数量
void DSMgr::IncNumPages()
{
    numPages++;
}

// 返回页面数量
int DSMgr::GetNumPages()
{
    return numPages;
}

// 设置页面使用信息
void DSMgr::SetUse(int index, int use_bit)
{
    // 检查页面号是否合法
    if (index < 0 || index >= MAXPAGES)
    {
        std::cerr << "Error: Invalid index." << std::endl;
        return;
    }

    // 设置页面使用信息
    pages[index] = use_bit;
}

// 返回页面使用信息
int DSMgr::GetUse(int index)
{
    // 检查页面号是否合法
    if (index < 0 || index >= MAXPAGES)
    {
        std::cerr << "Error: Invalid index." << std::endl;
        return -1;
    }

    // 返回页面使用信息
    return pages[index];
}

// 返回 IO 次数
int DSMgr::GetIOCount()
{
    return IOCount;
}