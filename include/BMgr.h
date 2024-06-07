#ifndef BMGR_H
#define BMGR_H
#include <list>
#include "DSMgr.h"

extern int bufferHitCount;
extern int bufferAccessCount;

struct NewPage
{
    int page_id;
    int frame_id;
};

struct BCB
{
    int page_id;
    int frame_id;
    int latch;
    int count;
    int dirty;
    BCB *next;

    BCB()
    {
        page_id = -1;
        frame_id = -1;
        latch = 0;
        count = 0;
        dirty = 0;
        next = NULL;
    }
};

class BMgr
{
public:
    BMgr();
    // Interface functions
    int FixPage(int page_id, int prot);
    NewPage FixNewPage();
    int UnfixPage(int page_id);
    int NumFreeFrames();
    
    // Internal Functions
    int SelectVictim();
    int Hash(int page_id);
    void RemoveBCB(BCB *ptr, int page_id);
    void RemoveLRUEle(int frid);
    void SetDirty(int frame_id);
    void UnsetDirty(int frame_id);
    void WriteDirtys();
    void PrintFrame(int frame_id);

    double GetBufferHitRate();
    DSMgr dsmgr;

private:
    // Hash Table
    int ftop[DEFBUFSIZE];  // frame -> page
    BCB *ptof[DEFBUFSIZE]; // Page -> frame

    // LRU List
    std::list<int> lruList;
};

#endif