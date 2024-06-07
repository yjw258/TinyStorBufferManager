#ifndef DSMGR_H
#define DSMGR_H

#include <iostream>
#include <string>
#include <cstdio>

#define MAXPAGES 50000 
#define FRAMESIZE 4096
#define DEFBUFSIZE 1024

struct bFrame
{
    char field[FRAMESIZE];
};

extern bFrame buf[DEFBUFSIZE];

extern int IOCount;

class DSMgr
{
public:
    DSMgr();
    int OpenFile(std::string filename);
    int CloseFile();
    bFrame ReadPage(int page_id);
    int WritePage(int page_id, bFrame frm); 
    int Seek(int offset, int pos);
    FILE *GetFile();
    void IncNumPages();
    int GetNumPages();
    void SetUse(int index, int use_bit);
    int GetUse(int index);
    int GetIOCount();
private:
    FILE *currFile;
    int numPages;        
    int pages[MAXPAGES]; 
};

#endif 
