#include "BMgr.h"

// 记录缓冲区命中次数和访问次数
int bufferHitCount;    
int bufferAccessCount; 

// 构造函数，初始化哈希表、buffer 空间、buffer 命中次数及访问次数
BMgr::BMgr()
{
    for (int i = 0; i < DEFBUFSIZE; ++i)
    {
        // 初始化 hash 表
        ftop[i] = -1;
        ptof[i] = NULL;

        // 初始化 buffer 空间
        buf[i] = bFrame();
    }

    // 初始化 buffer 命中次数及访问次数
    bufferHitCount = 0;
    bufferAccessCount = 0;
}

// 判断页面是否存在于缓冲区，并返回对应的frame_id；如果这个页面并不在缓冲区内，它会选择一个victim page
// mode = 0：读；mode = 1：写；mode = 2：新建
int BMgr::FixPage(int page_id, int mode)
{
    int frame_id = -1;
    BCB *bcb = ptof[Hash(page_id)];

    // 在 buffer 中找该 page
    for (; bcb != NULL; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            frame_id = bcb->frame_id;
            break;
        }
    }

    // 如果没有找到该 page
    if (frame_id == -1)
    {
        // 选择需要替换的页面
        frame_id = SelectVictim();

        // 更新 2 个哈希表
        ftop[frame_id] = page_id;
        if (ptof[Hash(page_id)] == NULL)
        {
            ptof[Hash(page_id)] = new BCB();
            ptof[Hash(page_id)]->page_id = page_id;
            ptof[Hash(page_id)]->frame_id = frame_id;
            ptof[Hash(page_id)]->count = 1;
        }
        else
        {
            BCB *current_bcb = ptof[Hash(page_id)];
            while (current_bcb->next != NULL)
                current_bcb = current_bcb->next;

            current_bcb->next = new BCB();
            current_bcb->next->page_id = page_id;
            current_bcb->next->frame_id = frame_id;
            current_bcb->next->count = 1;
        }

        // 读或写都需要将页面读入 buffer
        if (mode == 0 || mode == 1)
        {
            buf[frame_id] = dsmgr.ReadPage(page_id);
        }
    }
    // 在 buffer 中找到了该 page
    else
    {
        bcb->count++;     // 增加页面的 fix_count
        bufferHitCount++; // 增加 buffer 命中次数
    }

    // 如果是写操作，需要设置 dirty 位
    if (mode == 1)
    {
        SetDirty(frame_id);
    }

    // 更新 buffer 访问次数
    bufferAccessCount++;

    return frame_id;
}

// 在插入(索引分割、对象创建)时，找到一个空页面供上层使用
NewPage BMgr::FixNewPage()
{
    NewPage newPage;
    int i;

    // 找到第一个空闲的页面
    for (i = 0; i < dsmgr.GetNumPages(); i++)
    {
        if (dsmgr.GetUse(i) == 0)
            break;
    }

    // 如果没有空闲的页面，需要增加一个页面
    if (i == dsmgr.GetNumPages())
    {
        dsmgr.IncNumPages();
    }

    dsmgr.SetUse(i, 1); // 设置页面为已使用

    // 返回新页面的 page_id 和 frame_id
    newPage.page_id = i;
    newPage.frame_id = FixPage(i, 2); // 新建页面
    return newPage;
}

// 释放页面
int BMgr::UnfixPage(int page_id)
{
    // 检查 page_id 是否在 buffer 中
    int frame_id = -1;
    BCB *bcb = ptof[Hash(page_id)];
    for (; bcb != NULL; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            frame_id = bcb->frame_id;
            break;
        }
    }

    // 如果页面不在 buffer 中，返回错误码
    if (frame_id == -1)
    {
        std::cerr << "Page " << page_id << " is not in buffer." << std::endl;
        return -1;
    }

    // 如果页面的 fix_count 不为 0，减少 fix_count
    if (bcb->count != 0)
    {
        bcb->count--;
    }

    // 如果页面的 fix_count 为 0，更新 LRU 队列
    if (bcb->count == 0)
    {
        RemoveLRUEle(frame_id);
        lruList.push_back(frame_id);
    }

    return 0;
}

// 返回可用的 buffer 页面数
int BMgr::NumFreeFrames()
{
    int freeFrames = 0;
    for (int i = 0; i < DEFBUFSIZE; i++)
    {
        if (ftop[i] == -1)
        {
            freeFrames++;
        }
    }
    return freeFrames;
}

// 选择需要替换的页面，返回 frame_id
int BMgr::SelectVictim()
{
    // 如果有空闲的 frame，直接返回第一个空闲的 frame
    if (NumFreeFrames() != 0)
    {
        for (int i = 0; i < DEFBUFSIZE; ++i)
        {
            if (ftop[i] == -1)
                return i;
        }
    }

    // 如果没有空闲的 frame，选择 LRU 队列中的第一个 frame
    int victim_frame_id = lruList.front();
    lruList.pop_front();

    // 如果选择的页面是脏的，需要写回磁盘
    int page_id = ftop[victim_frame_id];
    for (BCB *bcb = ptof[Hash(page_id)]; bcb != NULL; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            if (bcb->dirty)
            {
                dsmgr.WritePage(page_id, buf[victim_frame_id]);
            }
            break;
        }
    }

    RemoveBCB(ptof[Hash(page_id)], page_id); // 移除 BCB

    return victim_frame_id;
}

// 返回 frame_id
int BMgr::Hash(int page_id)
{
    return page_id % DEFBUFSIZE;
}

// 从 BCB 链表中移除 page_id 对应的 BCB
void BMgr::RemoveBCB(BCB *ptr, int page_id)
{
    // 检查是否存在该 page_id 对应的 BCB 链表
    if (ptof[Hash(page_id)] == NULL)
        return;

    // 检查第一个元素是否是要删除的元素
    if (ptof[Hash(page_id)]->page_id == page_id)
    {
        // 删除第一个元素
        BCB *temp = ptof[Hash(page_id)];
        ptof[Hash(page_id)] = ptof[Hash(page_id)]->next;
        delete temp;
        return;
    }

    // 检查后续元素是否是要删除的元素
    for (BCB *bcb = ptof[Hash(page_id)]; bcb->next != NULL; bcb = bcb->next)
    {
        if (bcb->next->page_id == page_id)
        {
            // 删除 current_bcb->next
            BCB *temp = bcb->next;
            bcb->next = bcb->next->next;
            delete temp;
            return;
        }
    }
}

// 从 LRU 链表中移除 frame_id
void BMgr::RemoveLRUEle(int frid)
{
    lruList.remove(frid);
}

// 设置 dirty 位
void BMgr::SetDirty(int frame_id)
{
    // 检查 frame_id 是否在合法范围内
    if (frame_id < 0 || frame_id >= DEFBUFSIZE)
    {
        std::cerr << "Invalid frame_id: " << frame_id << std::endl;
        return;
    }

    // 找到对应的 BCB，设置 dirty 位
    int page_id = ftop[frame_id];
    for (BCB *bcb = ptof[Hash(page_id)]; bcb != NULL; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            bcb->dirty = 1;
            break;
        }
    }
}

// 重置 dirty 位
void BMgr::UnsetDirty(int frame_id)
{
    // 检查 frame_id 是否在合法范围内
    if (frame_id < 0 || frame_id >= DEFBUFSIZE)
    {
        std::cerr << "Invalid frame_id: " << frame_id << std::endl;
        return;
    }

    // 找到对应的 BCB，重置 dirty 位
    int page_id = ftop[frame_id];
    for (BCB *bcb = ptof[Hash(page_id)]; bcb != NULL; bcb = bcb->next)
    {
        if (bcb->page_id == page_id)
        {
            bcb->dirty = 0;
            break;
        }
    }
}

// 将所有脏页写回磁盘
void BMgr::WriteDirtys()
{
    for (int i = 0; i < DEFBUFSIZE; i++)
    {
        for (BCB *bcb = ptof[i]; bcb != NULL; bcb = bcb->next)
        {
            // 如果是脏页，则写回磁盘，并重置 dirty 位
            if (bcb->dirty)
            {
                dsmgr.WritePage(bcb->page_id, buf[bcb->frame_id]);
                bcb->dirty = 0;
            }
        }
    }
}

// 打印帧 frame_id 的 ID 和内容。
void BMgr::PrintFrame(int frame_id)
{
    if (frame_id < 0 || frame_id >= DEFBUFSIZE)
    {
        std::cerr << "Invalid frame_id: " << frame_id << std::endl;
        return;
    }

    std::cout << "Frame ID: " << frame_id << std::endl;
    std::cout << "Frame Content: " << buf[frame_id].field << std::endl;
}

// 计算并返回 buffer 命中率
double BMgr::GetBufferHitRate()
{
    return (double)bufferHitCount / bufferAccessCount;
}
