#ifndef FUN_H
#define FUN_H
#include<stdio.h>
#include<time.h>
#include<stdbool.h>
#include<stdlib.h>
#include<string.h>
#include<conio.h>
#include"convertTime.c"


#define BLOCKSIZE 1024              //物理块容量
#define INODESIZE 32                //i节点大小（最大）
#define SIZE 4194304                //整个文件系统总大小（4MB）
#define NUMOFBLOCK 3950             //数据区物理块数量
#define NUMOFINODE 4096             //i节点数量（1/KB）
#define END 65535                   //结束符
#define FREE 0                      //？（没有用到）
#define ROOTNUM 2                   //？（没有用到）
#define MAXOPENFILE 10              //最大打开文件数
#define MAXINDEXNUM 512             //索引文件中最大索引数量
#define MAXFILENAME 64              //目录文件中最大目录项数量

//各区域开始盘块号
#define SUPER 0                     //超级块
#define FREETABLE 1                 //位示图
#define IFREETABLE 9                //i节点位示图
#define INODETABLE 17               //i节点表
#define ROOTDICTIONARY 145          //根目录目录项
#define DATASTART 146               //数据区


typedef struct INODE{               //i节点
    //char filename[8];
    //char exname[3];
    unsigned char attribute;        //是否为目录文件（1为目录文件，0为普通文件）
    //unsigned int ctime;
    unsigned int mtime;             //文件创建时间
    unsigned int atime;             //文件最后修改时间
    unsigned short first;           //文件单级索引块的物理块索引
    unsigned long length;           //文件总长度
    unsigned short block_num;       //文件所占物理块数量
    char free;                      //该inode是否被占用的标志位
}inode;

typedef struct FNAMENODE{           //目录项
    char filename[8];               //文件名
    unsigned short i_index;         //文件对应i节点的索引
}fname_node;

typedef struct USEROPEN{            //文件描述符数组fd[]
    char filename[8];               //文件名
    unsigned char attribute;        //以下皆同inode中成员
    //unsigned int ctime;
    unsigned int mtime;
    unsigned int atime;
    unsigned short first;
    unsigned long length;
    unsigned short block_num;
    
    char dir[80];                   //文件的绝对路径（包含文件本身）
    int count;                      //读写指针
    char istate;                    //文件自被打开之后是否更改过
    char topenfile;                 //该openfilelist项是否被占用的标志位
}useropen;

typedef struct SUPERBLOCK{          //超级块
    //文件系统物理块数量与i节点数量
    unsigned short block_size;      //单个物理块大小
    unsigned short inode_size;      //i节点大小

    //各分区块数
    unsigned short block_num;       //物理块数量
    unsigned short inode_num;       //i节点数量

    //空闲块与空闲i节点数量
    unsigned short free_block;      //空闲物理块数量
    unsigned short free_inode;      //空闲i节点数量

    //空间布局
    unsigned short free_table;      //位示图的开始物理块号
    unsigned short ifree_table;     //i节点位示图的开始物理块号
    unsigned short inode_table;     //i节点表的开始物理盘块号
    unsigned short data_start;      //数据区的开始物理盘块号
    unsigned short root_dictionary; //根目录的目录项所在的物理盘块号
}superblock;


char* path = "fileSystem.txt";          //文件系统保存文件路径
unsigned char* myvhard;                 //文件系统首地址
//useropen openfilelist[MAXOPENFILE];
useropen* openfilelist;                 //当前打开的文件表
int curdir;                             //当前路径对应的openfilelist索引
char currentdir[80];                    //当前路径
unsigned short curinode;                //当前路径对应的i节点
unsigned char* startp;                  //数据区首地址
unsigned char* freetablep;              //位示图首地址
unsigned char* ifreetablep;             //i节点位示图首地址
unsigned char* inodetablep;             //i节点表首地址
unsigned char* rootdictionaryp;         //根节点目录项首地址


//----------------------------------------------------------------------------------------------
//tool functions

unsigned char* Treadfile(char* path, int* length)       //读取整个文本文件到内存并返回其头指针
{
    FILE* temfile;
    unsigned char* data = NULL;
    temfile = fopen(path, "rb");
    if(temfile == NULL)
    {
        return NULL;
    }

    fseek(temfile, 0, SEEK_END);
    *length = ftell(temfile);
    data = (unsigned char*)malloc(SIZE * sizeof(char));
    rewind(temfile);
    *length = fread(data, 1, *length, temfile);
    data[*length] = '\0';
    fclose(temfile);
    return data;
}

unsigned short Tgetfreeinode()                      //获取空闲i节点的索引
{
    unsigned short* freep = (unsigned short*)ifreetablep;
    for(int i = 0; i < NUMOFINODE; i++)
    {
        if(freep[i] == 0)
        {
            freep[i] = 1;
            //return inodetablep + i * INODESIZE;
            superblock* super = (superblock*)myvhard;
            super->free_inode--;
            return (unsigned short)i;
        }
    }
    printf("There's no more free inode!\n");
    return -1;
}

unsigned char* Tgetinode(unsigned short index)      //通过索引获取对应i节点的地址
{
    return myvhard + INODETABLE * BLOCKSIZE + index * INODESIZE;
}

void Tfreeinodetable(unsigned short index)
{
    ((unsigned short*)(ifreetablep + index * sizeof(unsigned short)))[0] = 0;
}

unsigned short Tgetfreeblock()                      //申请空闲物理块并获取其索引
{
    unsigned short* freep = (unsigned short*)freetablep;
    for(int i = 0; i < NUMOFBLOCK; i++)
    {
        if(freep[i] == 0)
        {
            freep[i] = 1;
            superblock* super = (superblock*)myvhard;
            super->free_block--;
            return (unsigned short)i;
        }
    }
    printf("There's no more free block!\n");
    return -1;
}

unsigned char* Tgetblock(unsigned short index)      //通过索引获取对应物理块的地址
{
    return myvhard + DATASTART * BLOCKSIZE + index * BLOCKSIZE;
}

fname_node* Tdirtoinodep(char* dir, unsigned short curi)        //通过绝对路径返回对应文件的目录项地址
{
    char temdir[80];
    strcpy(temdir, dir);
    if(dir[0] == 'r' && dir[1] == 'o' && dir[2] == 'o' && dir[3] == 't' && dir[4] == '/')
    {
        //unsigned short curdiri = 0;
        return Tdirtoinodep(dir + 5, 0);
    }
    else if (dir[0] == 'r' && dir[1] == 'o' && dir[2] == 'o' && dir[3] == 't')
    {
        return (fname_node*)rootdictionaryp;
    }
    else
    {
        char* tar = strtok(temdir, "/");
        unsigned short curlen = strlen(tar);
        inode* curinodep = (inode*)Tgetinode(curi);
        unsigned short* blockindex = (unsigned short*)(startp + curinodep->first * BLOCKSIZE);
        for(int i = 0; i < curinodep->block_num; i++)
        {
            fname_node* search = (fname_node*)(startp + blockindex[i] * BLOCKSIZE);
            for(int j = 0; j < MAXFILENAME; j++)
            {
                if(search[j].i_index > 0)
                {
                    if(!strcmp(search[j].filename, tar))
                    {
                        if(!strcmp(search[j].filename, dir))
                        {
                            return &search[j];
                        }
                        return Tdirtoinodep(dir + curlen + 1, search[j].i_index);
                    }
                }
            }
        }
        return NULL;
    }
}

void Twriteback(useropen* fdp, inode* tarinodep)            //将已打开的文件被更改的数据写回到对应的i节点中
{
    if(fdp->istate == 1)
    {
        inode* teminode = (inode*)calloc(1, sizeof(inode));
        memcpy(teminode, tarinodep, sizeof(inode));
        //teminode->ctime = fdp->ctime;
        teminode->block_num = fdp->block_num;
        teminode->length = fdp->length;
        teminode->atime = fdp->atime;
        teminode->first = fdp->first;
        teminode->mtime = fdp->mtime;
        teminode->attribute = fdp->attribute;
        teminode->free = 1;
        
        memcpy(tarinodep, teminode, sizeof(inode));
    }
}

void Tfreetable(unsigned short index)               //释放物理块
{
    ((unsigned short*)(freetablep + index * sizeof(unsigned short)))[0] = 0;
}

/*
unsigned long Tcaldirlength(unsigned short index)   //计算文件的总大小
{
    inode* inodep = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)Tgetblock(inodep->first);
    unsigned long result = 0;
    for(int i = 0; i < inodep->block_num; i++)
    {
        fname_node* fnamep = (fname_node*)Tgetblock(blockindex[i]);
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(fnamep[j].i_index > 0)
            {
                inode* soninodep = (inode*)Tgetinode(fnamep[j].i_index);
                if(soninodep->attribute == 1)
                {
                    if(strcmp(fnamep[j].filename, "..") && strcmp(fnamep[j].filename, "."))
                    {
                        result += Tcaldirlength(fnamep[j].i_index);
                    }
                }
                else
                {
                    result += soninodep->length;
                }
            }
        }
    }
    //printf("%d, %d\n", index, result);
    return result;
}
*/

unsigned long Tcaldirlength(unsigned short index)   //计算文件的总大小
{
    inode* inodep = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)Tgetblock(inodep->first);
    unsigned long result = 0;
    if(inodep->attribute == 1)
    {
        for(int i = 0; i < inodep->block_num; i++)
        {
            fname_node* fnamep = (fname_node*)Tgetblock(blockindex[i]);
            for(int j = 0; j < MAXFILENAME; j++)
            {
                if(fnamep[j].i_index > 0)
                {
                    inode* soninodep = (inode*)Tgetinode(fnamep[j].i_index);
                    if(soninodep->attribute == 1)
                    {
                        if(strcmp(fnamep[j].filename, "..") && strcmp(fnamep[j].filename, "."))
                        {
                            result += Tcaldirlength(fnamep[j].i_index);
                        }
                    }
                    else
                    {
                        result += soninodep->length;
                    }
                }
            }
        }
        return result;
    }
    else
    {
        return inodep->length;
    }
}

void Tprintdictionary(unsigned short index)         //打印目录文件下所有的数据（my_ls的核心）
{
    inode* inodep = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)(startp + inodep->first * BLOCKSIZE);
    printf("name      size      create time\n");
    printf(".\n");
    printf("..\n");
    for(int i = 0; i < inodep->block_num; i++)
    {
        fname_node* fnamep = (fname_node*)(startp + blockindex[i] * BLOCKSIZE);
        unsigned short inblocknum;
        if(inodep->length / sizeof(fname_node) - i * MAXFILENAME > MAXFILENAME)
        {
            inblocknum = MAXFILENAME;
        }
        else
        {
            inblocknum = inodep->length / sizeof(fname_node) - i * MAXFILENAME;
        }
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(fnamep[j].i_index != 0 && strcmp(fnamep[j].filename, "..") && strcmp(fnamep[j].filename, "."))
            {
                inode* teminodep = (inode*)Tgetinode(fnamep[j].i_index);
                if(teminodep->attribute == 1)
                {
                    printf("\x1b[94m" "%-10s" "\x1b[0m", fnamep[j].filename);
                }
                else
                {
                    printf("%-10s", fnamep[j].filename);
                }
                
                printf("%-10d", Tcaldirlength(fnamep[j].i_index));
                char lasttime[20];
                Tlocaltime(teminodep->mtime, lasttime);
                printf("%-23s\n", lasttime);
            }
        }
    }
}

void Tprintfile(unsigned short index)           //打印文件内容
{
    inode* inodep = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)(startp + inodep->first * BLOCKSIZE);
    for(int i = 0; i < inodep->block_num; i++)
    {
        unsigned char* datap = startp + blockindex[i] * BLOCKSIZE;
        for(int j = 0; j < BLOCKSIZE; j++)
        {
            printf("%c", datap[j]);
        }
    }
}

void Tmkdirto(unsigned short index, char* name)      //在i节点所对应的目录下创建指定名字的新目录（my_mkdir核心）
{
    time_t t = time(NULL);
    unsigned int l = (int)time(&t);

    bool flag = 0;
    fname_node* freefname = NULL;

    //inode* fatherp = (inode*)(inodetablep + index * INODESIZE);
    inode* fatherp = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)Tgetblock(fatherp->first);
    for(int i = 0; i < fatherp->block_num; i++)
    {
        if(freefname != NULL)
        {
            break;
        }
        fname_node* existed = (fname_node*)Tgetblock(blockindex[i]);
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(existed[j].i_index > 0)
            {
                if(!strcmp(existed[j].filename, name))
                {
                    printf("The same file name!\n");
                    return;
                }
                
            }
            else if(existed[j].i_index == 0 && strcmp(existed[j].filename, "..") && strcmp(existed[j].filename, "."))
            {
                flag = 1;
                freefname = &existed[j];
                break;
            }
        }
    }

    if(flag)
    {
        fname_node* newfname = (fname_node*)calloc(1, sizeof(fname_node));
        strcpy(newfname->filename, name);
        newfname->i_index = Tgetfreeinode();
        inode* newinodep = (inode*)Tgetinode(newfname->i_index);

        memcpy(freefname, newfname, sizeof(fname_node));
        
        newinodep->mtime = l;
        newinodep->atime = l;
        newinodep->attribute = 1;
        newinodep->block_num = 1;
        newinodep->length = 2 * sizeof(fname_node);
        newinodep->free = 1;
        newinodep->first = Tgetfreeblock();
        unsigned short* indexblock = (unsigned short*)Tgetblock(newinodep->first);
        indexblock[0] = Tgetfreeblock();
        fname_node* dirdatap = (fname_node*)Tgetblock(indexblock[0]);
        strcpy(dirdatap[0].filename, ".");
        strcpy(dirdatap[1].filename, "..");
        dirdatap[0].i_index = 0;
        dirdatap[1].i_index = index;

        if(curinode == index)
        {
            openfilelist[curdir].length += sizeof(fname_node);
            openfilelist[curdir].istate = 1;
            openfilelist[curdir].atime = l;
            //openfilelist[curdir].block_num++;
        }
        else
        {
            fatherp->length += sizeof(fname_node);
            fatherp->atime = l;
            //fatherp->block_num++;
        }
    }
    else
    {
        fname_node* newfname = (fname_node*)calloc(1, sizeof(fname_node));
        strcpy(newfname->filename, name);
        newfname->i_index = Tgetfreeinode();
        inode* newinodep = (inode*)Tgetinode(newfname->i_index);

        unsigned short temblock = Tgetfreeblock();
        freefname = (fname_node*)Tgetblock(temblock);
        memcpy(freefname, newfname, sizeof(fname_node));
        /*
        if(curinode == index)
        {
            blockindex[openfilelist[curdir].block_num] = temblock;
        }
        else
        {
            blockindex[fatherp->block_num] = temblock;
        }
        */

        newinodep->mtime = l;
        newinodep->atime = l;
        newinodep->attribute = 1;
        newinodep->block_num = 1;
        newinodep->length = 2 * sizeof(fname_node);
        newinodep->free = 1;
        newinodep->first = Tgetfreeblock();
        unsigned short* indexblock = (unsigned short*)Tgetblock(newinodep->first);
        indexblock[0] = Tgetfreeblock();
        fname_node* dirdatap = (fname_node*)Tgetblock(indexblock[0]);
        strcpy(dirdatap[0].filename, ".");
        strcpy(dirdatap[1].filename, "..");
        dirdatap[0].i_index = 0;
        dirdatap[1].i_index = index;

        if(curinode == index)
        {
            blockindex[openfilelist[curdir].block_num] = temblock;
            openfilelist[curdir].block_num++;
            openfilelist[curdir].length += sizeof(fname_node);
            openfilelist[curdir].istate = 1;
            openfilelist[curdir].atime = l;
        }
        else
        {
            blockindex[fatherp->block_num] = temblock;
            fatherp->block_num++;
            fatherp->length += sizeof(fname_node);
            fatherp->atime = l;
        }
    }
}

void Trmdirfrom(unsigned short index, char* name)           //删除i节点对应目录下的特定目录（my_rmdir核心）
{
    bool flag = 0;
    fname_node* tarfname;

    //inode* fatherp = (inode*)(inodetablep + index * INODESIZE);
    inode* fatherp = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)Tgetblock(fatherp->first);
    for(int i = 0; i < fatherp->block_num; i++)
    {
        fname_node* existed = (fname_node*)Tgetblock(blockindex[i]);
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(existed[j].i_index > 0)
            {
                if(!strcmp(existed[j].filename, name))
                {
                    tarfname = &existed[j];
                }
            }
        }
    }

    if(Tcaldirlength(tarfname->i_index) == 0)
    {
        inode* tarinode = (inode*)Tgetinode(tarfname->i_index);
        if(tarinode->attribute == 0)
        {
            printf("It's not a dictionary!\n");
            return;
        }
        unsigned short* blockindex2 = (unsigned short*)Tgetblock(tarinode->first);
        unsigned char* datablock = Tgetblock(blockindex2[0]);

        unsigned char* emptyblock = (unsigned char*)calloc(1, BLOCKSIZE);
        memcpy(datablock, emptyblock, BLOCKSIZE);
        Tfreetable(blockindex2[0]);
        memcpy(blockindex2, emptyblock, BLOCKSIZE);
        Tfreetable(tarinode->first);

        inode* emptyinode = (inode*)calloc(1, sizeof(inode));
        memcpy(tarinode, emptyinode, sizeof(inode));

        fname_node* emptyfname = (fname_node*)calloc(1, sizeof(fname_node));
        memcpy(tarfname, emptyfname, sizeof(fname_node));

        if(curinode == index)
        {
            openfilelist[curdir].length -= sizeof(fname_node);
            openfilelist[curdir].istate = 1;
        }
        else
        {
            fatherp->length -= sizeof(fname_node);
        }
        
    }
    else
    {
        printf("There's still some files in this dictionary.\n");
        return;
    }

    for(int i = 0; i < fatherp->block_num; i++)
    {
        bool emptyflag = 0;
        fname_node* existed = (fname_node*)Tgetblock(blockindex[i]);
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(strcmp(existed[j].filename, ""))
            {
                emptyflag = 1;
            }
        }
        if(!emptyflag)
        {
            Tfreetable(blockindex[i]);
            blockindex[i] = 0;
            if(curinode == index)
            {
                openfilelist[curdir].block_num--;
                openfilelist[curdir].istate = 1;
            }
            else
            {
                fatherp->block_num--;
            }
        }
    }
}

void Tcreatto(unsigned short index, char* name)         //在i节点对应目录下创建文件（my_create核心）
{
    time_t t = time(NULL);
    unsigned int l = (int)time(&t);

    bool flag = 0;
    fname_node* freefname = NULL;

    //inode* fatherp = (inode*)(inodetablep + index * INODESIZE);
    inode* fatherp = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)Tgetblock(fatherp->first);
    for(int i = 0; i < fatherp->block_num; i++)
    {
        if(freefname != NULL)
        {
            break;
        }
        fname_node* existed = (fname_node*)Tgetblock(blockindex[i]);
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(existed[j].i_index > 0)
            {
                if(!strcmp(existed[j].filename, name))
                {
                    printf("The same file name!\n");
                    return;
                }
                
            }
            else if(existed[j].i_index == 0 && strcmp(existed[j].filename, "..") && strcmp(existed[j].filename, "."))
            {
                flag = 1;
                freefname = &existed[j];
                break;
            }
        }
    }

    if(flag)
    {
        fname_node* newfname = (fname_node*)calloc(1, sizeof(fname_node));
        strcpy(newfname->filename, name);
        newfname->i_index = Tgetfreeinode();
        inode* newinodep = (inode*)Tgetinode(newfname->i_index);

        memcpy(freefname, newfname, sizeof(fname_node));
        
        newinodep->mtime = l;
        newinodep->atime = l;
        newinodep->attribute = 0;
        newinodep->block_num = 1;
        newinodep->length = 0;
        newinodep->free = 1;
        newinodep->first = Tgetfreeblock();
        unsigned short* indexblock = (unsigned short*)Tgetblock(newinodep->first);
        indexblock[0] = Tgetfreeblock();
        //fname_node* dirdatap = (fname_node*)Tgetblock(indexblock[0]);
        //strcpy(dirdatap[0].filename, ".");
        //strcpy(dirdatap[1].filename, "..");
        //dirdatap[0].i_index = 0;
        //dirdatap[1].i_index = index;

        if(curinode == index)
        {
            openfilelist[curdir].length += sizeof(fname_node);
            openfilelist[curdir].istate = 1;
            openfilelist[curdir].atime = l;
            //openfilelist[curdir].block_num++;
        }
        else
        {
            fatherp->length += sizeof(fname_node);
            fatherp->atime = l;
            //fatherp->block_num++;
        }
    }
    else
    {
        fname_node* newfname = (fname_node*)calloc(1, sizeof(fname_node));
        strcpy(newfname->filename, name);
        newfname->i_index = Tgetfreeinode();
        inode* newinodep = (inode*)Tgetinode(newfname->i_index);

        unsigned short temblock = Tgetfreeblock();
        freefname = (fname_node*)Tgetblock(temblock);
        memcpy(freefname, newfname, sizeof(fname_node));

        newinodep->mtime = l;
        newinodep->atime = l;
        newinodep->attribute = 0;
        newinodep->block_num = 1;
        newinodep->length = 0;
        newinodep->free = 1;
        newinodep->first = Tgetfreeblock();
        unsigned short* indexblock = (unsigned short*)Tgetblock(newinodep->first);
        indexblock[0] = Tgetfreeblock();
        //fname_node* dirdatap = (fname_node*)Tgetblock(indexblock[0]);
        //strcpy(dirdatap[0].filename, ".");
        //strcpy(dirdatap[1].filename, "..");
        //dirdatap[0].i_index = 0;
        //dirdatap[1].i_index = index;

        if(curinode == index)
        {
            blockindex[openfilelist[curdir].block_num] = temblock;
            openfilelist[curdir].block_num++;
            openfilelist[curdir].length += sizeof(fname_node);
            openfilelist[curdir].istate = 1;
            openfilelist[curdir].atime = l;
        }
        else
        {
            blockindex[fatherp->block_num] = temblock;
            fatherp->block_num++;
            fatherp->length += sizeof(fname_node);
            fatherp->atime = l;
        }
    }
}

void Trmfrom(unsigned short index, char* name)          //删除i节点对应目录下的特定文件（my_rm核心）
{
    bool flag = 0;
    fname_node* tarfname;

    //inode* fatherp = (inode*)(inodetablep + index * INODESIZE);
    inode* fatherp = (inode*)Tgetinode(index);
    unsigned short* blockindex = (unsigned short*)Tgetblock(fatherp->first);
    for(int i = 0; i < fatherp->block_num; i++)
    {
        fname_node* existed = (fname_node*)Tgetblock(blockindex[i]);
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(existed[j].i_index > 0)
            {
                if(!strcmp(existed[j].filename, name))
                {
                    tarfname = &existed[j];
                }
            }
        }
    }

    inode* tarinode = (inode*)Tgetinode(tarfname->i_index);
    if(tarinode->attribute == 1)
    {
        printf("It's not a file!\n");
        return;
    }
    unsigned short* blockindex2 = (unsigned short*)Tgetblock(tarinode->first);
    unsigned char* emptyblock = (unsigned char*)calloc(1, BLOCKSIZE);
    for(int i = 0; i < tarinode->block_num; i++)
    {
        unsigned char* datablock = Tgetblock(blockindex2[i]);
        memcpy(datablock, emptyblock, BLOCKSIZE);
        Tfreetable(blockindex2[i]);
    }
    memcpy(blockindex2, emptyblock, BLOCKSIZE);
    Tfreetable(tarinode->first);

    inode* emptyinode = (inode*)calloc(1, sizeof(inode));
    memcpy(tarinode, emptyinode, sizeof(inode));

    fname_node* emptyfname = (fname_node*)calloc(1, sizeof(fname_node));
    memcpy(tarfname, emptyfname, sizeof(fname_node));

    if(curinode == index)
    {
        openfilelist[curdir].length -= sizeof(fname_node);
        openfilelist[curdir].istate = 1;
    }
    else
    {
        fatherp->length -= sizeof(fname_node);
    }
        

    for(int i = 0; i < fatherp->block_num; i++)
    {
        bool emptyflag = 0;
        fname_node* existed = (fname_node*)Tgetblock(blockindex[i]);
        for(int j = 0; j < MAXFILENAME; j++)
        {
            if(strcmp(existed[j].filename, ""))
            {
                emptyflag = 1;
            }
        }
        if(!emptyflag)
        {
            Tfreetable(blockindex[i]);
            blockindex[i] = 0;
            if(curinode == index)
            {
                openfilelist[curdir].block_num--;
                openfilelist[curdir].istate = 1;
            }
            else
            {
                fatherp->block_num--;
            }
        }
    }
}

void TsetdictionaryItem(char* name, unsigned char* fatheradd, unsigned short i_index)   //在指定地址中加入新的目录项
{
    fname_node* newdirfile = (fname_node*)calloc(1, sizeof(fname_node));
    strcpy(newdirfile->filename, name);
    newdirfile->i_index = i_index;
    memcpy(fatheradd, newdirfile, sizeof(fname_node));
}

int Tgetfreefd()                //获取空闲文件描述符
{
    for(int i = 0; i < 10; i++)
    {
        if(openfilelist[i].topenfile == 0)
        {
            return i;
        }
    }
}

int Topenfile(fname_node* file, char* dir)         //打开文件
{
    time_t t = time(NULL);
    unsigned int l = (int)time(&t);

    useropen* openfile = (useropen*)calloc(1, sizeof(useropen));
    inode* fileinode = (inode*)Tgetinode(file->i_index);
    strcpy(openfile->filename, file->filename);
    //openfile->first = ((unsigned short*)(startp + fileinode->first * BLOCKSIZE))[0];
    openfile->first = fileinode->first;
    openfile->atime = l;
    //openfile->ctime = fileinode->ctime;
    openfile->mtime = fileinode->mtime;
    openfile->attribute = fileinode->attribute;
    openfile->block_num = fileinode->block_num;
    openfile->length = fileinode->length;
    openfile->istate = 1;
    openfile->count = 0;

    //openfile->istate = 0;
    openfile->topenfile = 1;
    char temdir[80];
    strcpy(temdir, dir);
    if(strcmp("", temdir))
    {
        strcat(temdir, "/");
    }
    strcat(temdir, file->filename);
    strcpy(openfile->dir, temdir);
    openfile->count = 0;

    int fd = Tgetfreefd();
    memcpy(openfilelist + fd, openfile, sizeof(useropen));
    return fd;
}

void Tclosefile(fname_node* file)
{

}


//----------------------------------------------------------------------------------------------
//order function

void my_format()
{
    //初始化全局变量
    myvhard = (unsigned char*)calloc(SIZE, sizeof(char));
    startp = myvhard + DATASTART * BLOCKSIZE;
    freetablep = myvhard + FREETABLE * BLOCKSIZE;
    ifreetablep = myvhard + IFREETABLE * BLOCKSIZE;
    inodetablep = myvhard + INODETABLE * BLOCKSIZE;
    rootdictionaryp = myvhard + ROOTDICTIONARY * BLOCKSIZE;

    //初始化超级块
    superblock* block0 = (superblock*)calloc(1, sizeof(superblock));

    block0->block_size = BLOCKSIZE;
    block0->inode_size = INODESIZE;
    block0->block_num = SIZE/BLOCKSIZE;
    block0->inode_num = NUMOFINODE;

    block0->free_block = NUMOFBLOCK;
    block0->free_inode = block0->inode_num;

    block0->free_table = FREETABLE;
    block0->ifree_table = IFREETABLE;
    block0->inode_table = INODETABLE;
    block0->root_dictionary = ROOTDICTIONARY;
    block0->data_start = DATASTART;
    memcpy(myvhard, &block0, sizeof(superblock));

    //创建根目录
    unsigned short rootinode = Tgetfreeinode();
    TsetdictionaryItem("root", rootdictionaryp, rootinode);

    inode* newinode = (inode*)Tgetinode(rootinode);
    newinode->attribute = 1;
    newinode->first = Tgetfreeblock();
    unsigned short firstblock = Tgetfreeblock();
    unsigned short* indexblock = (unsigned short*)Tgetblock(newinode->first);
    memcpy(indexblock, &firstblock, sizeof(unsigned short));
    unsigned char* firstblockp = Tgetblock(firstblock);
    TsetdictionaryItem(".", firstblockp, 0);
    TsetdictionaryItem("..", firstblockp + sizeof(fname_node), 0);

    newinode->length = 2 * sizeof(fname_node);
    newinode->block_num = 1;

    curdir = Topenfile((fname_node*)rootdictionaryp, "");
    curinode = 0;
    printf("Formating virtual disk succeed.\n");
}

void startsys()
{
    printf("Starting file system, please wait...\n");
    int disksize;
    myvhard = Treadfile(path, &disksize);
    startp = myvhard + DATASTART * BLOCKSIZE;
    openfilelist = (useropen*)calloc(MAXOPENFILE, sizeof(useropen));
    useropen* rootdicfile = (useropen*)calloc(1, sizeof(useropen));
    strcpy(currentdir, "root");

    if(myvhard != NULL)
    {
        //printf("Virtual disk size: %d\n", disksize);
        printf("Initing virtual disk...\n");
        freetablep = myvhard + FREETABLE * BLOCKSIZE;
        ifreetablep = myvhard + IFREETABLE * BLOCKSIZE;
        inodetablep = myvhard + INODETABLE * BLOCKSIZE;
        rootdictionaryp = myvhard + ROOTDICTIONARY * BLOCKSIZE;

        curdir = Topenfile((fname_node*)rootdictionaryp, "");
        curinode = 0;
        printf("Initing virtual disk succeeded.\n");
    }
    else
    {
        my_format();
    }
}

int my_open(char* filename)
{
    fname_node* fnamenodep;
    char temdir[80];
    strcpy(temdir, filename);
    bool flag = 0;
    for(int i = 0; i < 10; i++)
    {
        if(!strcmp(openfilelist[i].dir, filename))
        {
            flag = 1;
        }
    }

    if(!flag)
    {
        int freefd;
        if(filename[0] == 'r' && filename[1] == 'o' && filename[2] == 'o' && filename[3] == 't')
        {
            fnamenodep = Tdirtoinodep(filename, 0);
            int len = strlen(filename);
            while(temdir[--len] != '/')
            {
                temdir[len] = '\0';
            }
            temdir[len] = '\0';
            freefd = Topenfile(fnamenodep, temdir);
        }
        else
        {
            fnamenodep = Tdirtoinodep(filename, curinode);
            freefd = Topenfile(fnamenodep, currentdir);
        }
        return freefd;
    }
    
}

void my_close(int fd)
{
    useropen* fdp = openfilelist + fd;
    if(fdp->topenfile == 1)
    {
        fname_node* tarfname = Tdirtoinodep(fdp->dir, 0);
        inode* tarinodep = (inode*)Tgetinode(tarfname->i_index);
        Twriteback(fdp, tarinodep);
        useropen* emptyuseropen = (useropen*)calloc(1, sizeof(useropen));
        memcpy(fdp, emptyuseropen, sizeof(useropen));
    }
    else
    {
        printf("There's no such fd.\n");
        return;
    }
}

void my_cd(char* dirname)
{
    if(dirname[0] == 'r' && dirname[1] == 'o' && dirname[2] == 'o' && dirname[3] == 't')
    {
        if(((inode*)Tgetinode(Tdirtoinodep(dirname, 0)->i_index))->attribute == 0)
        {
            printf("It's not a dictionary!\n");
            return;
        }
        strcpy(currentdir, dirname);
        my_close(curdir);
        curdir = my_open(dirname);//绝对路径
        curinode = Tdirtoinodep(currentdir, 0)->i_index;
    }
    else if(!strcmp("..", dirname))
    {
        int nowlen = strlen(openfilelist[curdir].filename);
        int totallen = strlen(currentdir);
        for(int i = 0; i <= nowlen; i++)
        {
            currentdir[--totallen] = '\0';
        }
        my_close(curdir);
        curdir = my_open(currentdir);
        curinode = Tdirtoinodep(currentdir, 0)->i_index;
    }
    else
    {
        char temdir[80];
        strcpy(temdir, currentdir);
        strcat(temdir, "/");
        strcat(temdir, dirname);
        if(Tdirtoinodep(temdir, 0) == NULL)
        {
            printf("Wrong dictionary.\n");
            return;
        }
        else if(((inode*)Tgetinode(Tdirtoinodep(temdir, 0)->i_index))->attribute == 0)
        {
            printf("It's not a dictionary!\n");
            return;
        }
        else
        {
            strcat(currentdir, "/");
            strcat(currentdir, dirname);
            my_close(curdir);
            curdir = my_open(currentdir);
            //strcat(currentdir, "/");
            //strcat(currentdir, dirname);
            curinode = Tdirtoinodep(currentdir, 0)->i_index;
        }
    }
}

void my_ls()
{
    Tprintdictionary(curinode);
}

void my_mkdir(char* dirname)
{
    if(dirname[0] == 'r' && dirname[1] == 'o' && dirname[2] == 'o' && dirname[3] == 't')
    {
        char temdir[80];
        strcpy(temdir, dirname);
        for(int i = strlen(temdir) - 1; temdir[i] != '/'; i--)
        {
            temdir[i] = '\0';
        }
        temdir[strlen(temdir) - 1] = '\0';
        fname_node* tarfname = Tdirtoinodep(temdir, 0);
        Tmkdirto(tarfname->i_index, dirname + strlen(temdir) + 1);
    }
    else
    {
        Tmkdirto(curinode, dirname);
    }
}

void my_rmdir(char* dirname)
{
    if(dirname[0] == 'r' && dirname[1] == 'o' && dirname[2] == 'o' && dirname[3] == 't')
    {
        char temdir[80];
        strcpy(temdir, dirname);
        for(int i = strlen(temdir) - 1; temdir[i] != '/'; i--)
        {
            temdir[i] = '\0';
        }
        temdir[strlen(temdir) - 1] = '\0';
        fname_node* tarfname = Tdirtoinodep(temdir, 0);
        Trmdirfrom(tarfname->i_index, dirname + strlen(temdir) + 1);
    }
    else
    {
        Trmdirfrom(curinode, dirname);
    }
}

void my_create(char* dirname)
{
    if(dirname[0] == 'r' && dirname[1] == 'o' && dirname[2] == 'o' && dirname[3] == 't')
    {
        char temdir[80];
        strcpy(temdir, dirname);
        for(int i = strlen(temdir) - 1; temdir[i] != '/'; i--)
        {
            temdir[i] = '\0';
        }
        temdir[strlen(temdir) - 1] = '\0';
        fname_node* tarfname = Tdirtoinodep(temdir, 0);
        Tcreatto(tarfname->i_index, dirname + strlen(temdir) + 1);
    }
    else
    {
        Tcreatto(curinode, dirname);
    }
}

void my_rm(char* dirname)
{
    if(dirname[0] == 'r' && dirname[1] == 'o' && dirname[2] == 'o' && dirname[3] == 't')
    {
        char temdir[80];
        strcpy(temdir, dirname);
        for(int i = strlen(temdir) - 1; temdir[i] != '/'; i--)
        {
            temdir[i] = '\0';
        }
        temdir[strlen(temdir) - 1] = '\0';
        fname_node* tarfname = Tdirtoinodep(temdir, 0);
        Trmfrom(tarfname->i_index, dirname + strlen(temdir) + 1);
    }
    else
    {
        Trmfrom(curinode, dirname);
    }
}

int do_read(int fd, int len, char* text)
{
    if(len + openfilelist[fd].count > openfilelist[fd].length)
    {
        printf("out of length.\n");
        return -1;
    }

    char* buffer = (char*)calloc(BLOCKSIZE + 1, sizeof(char));
    unsigned short* blockindex = (unsigned short*)Tgetblock(openfilelist[fd].first);
    int pointer = openfilelist[fd].count;
    int off = pointer % BLOCKSIZE;
    int startblock = pointer / BLOCKSIZE;
    unsigned short readblock = blockindex[startblock];
    unsigned char* datapointer = (unsigned char*)Tgetblock(readblock);
    memcpy(buffer, datapointer, BLOCKSIZE);
    if(len > BLOCKSIZE - off)
    {
        memcpy(text, buffer + off, BLOCKSIZE - off);
        openfilelist[fd].count += BLOCKSIZE - off;
        do_read(fd, len - (BLOCKSIZE - off), text);
    }
    else
    {
        memcpy(text, buffer + off, len);
        openfilelist[fd].count += len;
    }
    return 0;
}

int my_read(int fd, int len)
{
    if(openfilelist[fd].topenfile == 1)
    {
        char* data;
        data = (char*)calloc(10 * BLOCKSIZE, sizeof(char));
        do_read(fd, len, data);
        printf("reads: \n%s\n", data);
        return 0;
    }
    else
    {
        printf("file %d isn't opened.\n", fd);
        return -1;
    }
}

int do_write(int fd, char* text, int len, char wstyle)
{
    openfilelist[fd].istate = 1;
    int pointer = openfilelist[fd].count;
    unsigned short blocknum = openfilelist[fd].block_num;
    int newlength;
    unsigned short* blockindex = (unsigned short*)Tgetblock(openfilelist[fd].first);

    if(wstyle != 'i' && wstyle != 'c' && wstyle != 'a')
    {
        printf("Wrong writing style.\n");
        return -1;
    }

    //printf("pointer: %d\nlength: %d\n", pointer, openfilelist[fd].length);
    //printf("len: %d\n", len);

    if(wstyle == 'i')
    {
        //newlength = max(openfilelist[fd].length, pointer + len);
        if(openfilelist[fd].length > pointer + len)
        {
            newlength = openfilelist[fd].length;
        }
        else
        {
            newlength = pointer + len;
        }
        
    }
    else
    {
        newlength = openfilelist[fd].count + len;
    }

    //printf("newlength: %d\n", newlength);

    unsigned short newblocknum;
    unsigned short writenum;
    if(openfilelist[fd].block_num * BLOCKSIZE < newlength)
    {
        newblocknum = (newlength - openfilelist[fd].block_num * BLOCKSIZE) / BLOCKSIZE;
        if((newlength - openfilelist[fd].block_num * BLOCKSIZE) % BLOCKSIZE > 0)
        {
            newblocknum++;
        }

        for(int i = 0; i < newblocknum; i++)
        {
            unsigned short newblock = Tgetfreeblock();
            blockindex[openfilelist[fd].block_num] = newblock;
            openfilelist[fd].block_num++;
        }
        //printf("newblocknum: %d\n", openfilelist[fd].block_num);
    }
    else if(openfilelist[fd].block_num * BLOCKSIZE > newlength)
    {
        newblocknum = (openfilelist[fd].block_num * BLOCKSIZE - newlength) / BLOCKSIZE;

        for(int i = 0; i < newblocknum; i++)
        {
            unsigned char* emptyblock = (unsigned char*)calloc(1, BLOCKSIZE);
            unsigned char* tarblock = Tgetblock(blockindex[openfilelist[fd].block_num - 1 - i]);
            memcpy(tarblock, emptyblock, BLOCKSIZE);
            free(emptyblock);
            Tfreetable(blockindex[openfilelist[fd].block_num - 1 - i]);
            blockindex[openfilelist[fd].block_num - 1 - i] = 0;
            openfilelist[fd].block_num--;
        }
        //printf("newblocknum: %d\n", openfilelist[fd].block_num);
    }

    if(wstyle == 'c')
    {
        //printf("yesc\n");
        for(int i = 0; i < openfilelist[fd].block_num; i++)
        {
            unsigned char* emptyblock = (unsigned char*)calloc(1, BLOCKSIZE);
            unsigned char* tarblock = Tgetblock(blockindex[i]);
            memcpy(tarblock, emptyblock, BLOCKSIZE);
            free(emptyblock);
        }
    }

    int temlen = len;
    int wrote = 0;
    while(temlen > 0)
    {
        unsigned char* blocknow = (unsigned char*)Tgetblock(blockindex[pointer / BLOCKSIZE]);
        if(pointer % BLOCKSIZE == 0)
        {
            //printf("yes1\n");
            if(temlen <= BLOCKSIZE)
            {
                //printf("%s\n", text + wrote);
                memcpy(blocknow, text + wrote, temlen);
                wrote += temlen;
                pointer += temlen;
                temlen -= temlen;
            }
            else
            {
                memcpy(blocknow, text + wrote, BLOCKSIZE);
                temlen -= BLOCKSIZE;
                wrote += BLOCKSIZE;
                pointer += BLOCKSIZE;
            }
            
        }
        else
        {
            //printf("yes2\n");
            int remain = BLOCKSIZE - pointer % BLOCKSIZE;
            memcpy(blocknow + pointer % BLOCKSIZE, text + wrote, remain);
            temlen -= remain;
            wrote += remain;
            pointer += remain;
        }
        
    }
    
    openfilelist[fd].length = newlength;
    //printf("nowlength: %d\n", openfilelist[fd].length);
    return 0;
}

int my_write(int fd)
{
    if(openfilelist[fd].topenfile == 0)
    {
        printf("file %d isn't opened.\n", fd);
        return -1;
    }

    printf("Please input your writing style: \n");
    char wstyle;
    char c;
    char* text = (char*)calloc(4096 * BLOCKSIZE, sizeof(char));
    int counter = 0;
    scanf("%c", &wstyle);
    if(wstyle == 'c')
    {
        openfilelist[fd].count = 0;
    }
    else if(wstyle == 'a')
    {
        openfilelist[fd].count = openfilelist[fd].length;
    }
    printf("Please input your data: \n");
    while(1)
    {
        c = getche();
        if(c == 26)
        {
            getchar();
            printf("\n");
            break;
        }
        else
        {
            text[counter++] = c;
        }
    }
    //printf("%s\n%d\n", text, strlen(text));
    do_write(fd, text, strlen(text), wstyle);
}

void my_exitsys()
{
    FILE* temfile;
    unsigned char* data = NULL;
    temfile = fopen(path, "wb");
    data = (unsigned char*)malloc(SIZE * sizeof(char));
    memcpy(data, myvhard, SIZE * sizeof(char));
    fwrite(data, 1, SIZE, temfile);
    free(data);
    fclose(temfile);
}


#endif