// 包含头文件
#include "lzw.h"
#include "log.h"

/* 类型定义 */
/* LZW 串表项类型定义 */
/* prefixe code -1 means null */
typedef struct
{
    int  prefix;    /* 前缀       */
    int  firstbyte; /* 首字节     */
    int  lastbyte;  /* 尾字节     */
    int  head;      /* 前缀链头   */
    int  next;      /* 前缀链指针 */
} LZWSTRITEM, *PLZWSTRITEM;

/* LZW 编解码器类型定义 */
typedef struct
{
    #define LZW_TAB_SIZE       4096
    #define LZW_ROOT_SIZE      8
    #define LZW_CODE_SIZE_OUT  8
    #define LZW_CODE_SIZE_MIN  9
    #define LZW_CODE_SIZE_MAX  12
    #define LZW_ROOT_NUM       256
    #define LZW_CLR_CODE       256
    #define LZW_END_CODE       257

    #define LZW_MODE_DECODE    0
    #define LZW_MODE_ENCODE    1
    int   mode;
    FILE *fp  ;
    BYTE  bitbuf;
    int   bitflag;

    /* 内部使用的变量 */
    LZWSTRITEM lzw_str_tab[LZW_TAB_SIZE];
    int        str_tab_pos;
    int        lzw_str_buf[LZW_TAB_SIZE];
    int        str_buf_pos;
    int        curcodesize;
    int        prefixcode;
    int        oldcode;

    // for lzw_fseek & lzw_ftell
    long       fpos;
} LZW;

// 内部函数实现
static void resetlzwcodec(LZW *lzw)
{
    if (lzw->mode == LZW_MODE_ENCODE)
    {
        LZWSTRITEM *strtab_cur = &(lzw->lzw_str_tab[0]);
        LZWSTRITEM *strtab_end = strtab_cur + LZW_TAB_SIZE;
        /* init lzw prefix link list */
        do { strtab_cur->head = 0; } while (++strtab_cur < strtab_end);
    }

    /* reset */
    lzw->str_tab_pos = LZW_END_CODE + 1;
    lzw->curcodesize = LZW_CODE_SIZE_MIN;
    lzw->prefixcode  = -1;
    lzw->oldcode     = -1;
}

/* 向 LZW 编码表中加入一个字符串 */
static BOOL AddToLZWStringTable(LZW *lzw, int prefix, int byte, BOOL flag)
{
    LZWSTRITEM *strtab = (LZWSTRITEM*)lzw->lzw_str_tab;

    /* 前缀为空的字符串直接返回成功 */
    if (prefix == -1) return TRUE;

    /* 如果 LZW 编码表满则返回失败 */
    if (lzw->str_tab_pos >= LZW_TAB_SIZE) return FALSE;

    /* 在编码表中加入新字符串 */
    strtab[lzw->str_tab_pos].prefix    = prefix;
    strtab[lzw->str_tab_pos].firstbyte = strtab[prefix].firstbyte;
    strtab[lzw->str_tab_pos].lastbyte  = byte;

    if (flag)
    {
        /* 更新前缀链表 */
        strtab[lzw->str_tab_pos].next = strtab[prefix].head;
        strtab[prefix].head           = lzw->str_tab_pos;
    }

    /* 增加编码表当前位置指针 */
    lzw->str_tab_pos++;

    /* 返回成功 */
    return TRUE;
}

/*
  在编码表中查找一个字符串的编码
  找到则返回其编码，否则返回 -1 .
 */
static int FindInLZWStringTable(LZW *lzw, int prefix, int byte)
{
    LZWSTRITEM *strtab = (LZWSTRITEM*)lzw->lzw_str_tab;
    int         link;

    /* 如果前缀为 -1 则直接返回 */
    if (prefix == -1) return byte;

    /* 在 while 循环中遍历前缀链表 */
    link = strtab[prefix].head;
    while (link)
    {
        if (strtab[link].lastbyte == byte) return link;
        link = strtab[link].next;
    }

    /* 没有找到则返回 -1 */
    return -1;
}

/* 判断当前编码表中是否已经存在某个编码 */
static BOOL IsCodeInLZWStringTable(LZW *lzw, int code)
{
    if (code >= 0 && code < lzw->str_tab_pos) return TRUE;
    else return FALSE;
}

/* 取得一个编码对应字符串的首字符 */
static int GetFirstByteOfLZWCode(LZW *lzw, int code)
{
    return ((LZWSTRITEM*)lzw->lzw_str_tab)[code].firstbyte;
}

/* 解码 lzw 字符串并填充 lzw_str_buf */
static void DecodeLZWString(LZW *lzw, int code)
{
    LZWSTRITEM *strtab = (LZWSTRITEM*)lzw->lzw_str_tab;
    int        *pbuf   = lzw->lzw_str_buf;

    /* 构造 LZW 字符串 */
    while (code != -1 && lzw->str_buf_pos++ < LZW_TAB_SIZE)
    {
       *pbuf++ = strtab[code].lastbyte;
        code   = strtab[code].prefix;
    }
}

BOOL getbits(LZW *lzw, DWORD *data, int size)
{
    int pos, byte, need, i;

    need = (size - lzw->bitflag + 7) / 8;
    if (need > 0)
    {
       *data = lzw->bitbuf;
        pos  = lzw->bitflag;
        for (i=0; i<need; i++) {
            byte = fgetc(lzw->fp);
            if (byte == EOF) return FALSE;

           *data |= byte << pos;
            pos  += 8;
        }
        lzw->bitflag = need * 8 + lzw->bitflag - size;
        lzw->bitbuf  = byte >> (8 - lzw->bitflag);
    }
    else
    {
       *data  = lzw->bitbuf;
        lzw->bitflag -= size;
        lzw->bitbuf >>= size;
    }
    *data &= (1L << size) - 1;
    return TRUE;
}

BOOL putbits(LZW *lzw, DWORD data, int size)
{
    int nbit;

    data &= (1L << size) - 1;

    if (lzw->bitflag)
    {
        nbit = size < 8 - lzw->bitflag ? size : 8 - lzw->bitflag;
        lzw->bitbuf |= data << lzw->bitflag;
        lzw->bitflag+= nbit;
        if (lzw->bitflag < 8) return TRUE;

        if (EOF == fputc(lzw->bitbuf, lzw->fp)) return FALSE;
        data >>= nbit;
        size  -= nbit;
    }

    while (size >= 8)
    {
        if (EOF == fputc(data, lzw->fp)) return FALSE;
        data >>= 8;
        size  -= 8;
    }

    lzw->bitflag = size;
    lzw->bitbuf  = (BYTE)data;
    return TRUE;
}

BOOL flushbits(LZW *lzw, int flag)
{
    DWORD fill;

    if (!lzw->bitflag) return TRUE;

    if (flag) fill = 0xffffffff;
    else      fill = 0;

    if (!putbits(lzw, fill, 8 - lzw->bitflag)) return FALSE;
    else return TRUE;
}

// 函数实现
void* lzw_fopen(const char *filename, const char *mode)
{
    LZW        *lzw    = NULL;
    LZWSTRITEM *strtab = NULL;
    int         i;

    // allocate lzw context
    lzw = malloc(sizeof(LZW));
    if (!lzw) return NULL;

    // init lzw context
    memset(lzw, 0, sizeof(LZW));
    lzw->mode = *mode == 'w' ? LZW_MODE_ENCODE : LZW_MODE_DECODE;
    lzw->fp   = fopen(filename, mode);

    // init lzw root code
    strtab = lzw->lzw_str_tab;
    for (i=0; i<LZW_END_CODE; i++)
    {
        strtab[i].prefix    = -1;
        strtab[i].firstbyte =  i;
        strtab[i].lastbyte  =  i;
    }

    // reset lzw codec
    resetlzwcodec(lzw);

    return lzw;
}

int lzw_fclose(void *stream)
{
    LZW *lzw = (LZW*)stream;
    if (lzw)
    {
        if (lzw->fp)
        {
            if (!putbits(lzw, lzw->prefixcode, lzw->curcodesize)) return EOF;
            if (!putbits(lzw, LZW_END_CODE   , lzw->curcodesize)) return EOF;
            if (!flushbits(lzw, 0)) return EOF;
            fclose(lzw->fp);
        }
        free(lzw);
    }
    return 0;
}

int lzw_fgetc(void *stream)
{
    LZW  *lzw     = (LZW*)stream;
    DWORD curcode = 0;
    BOOL  find    = FALSE;

    if (lzw->mode == LZW_MODE_ENCODE) return EOF;

    /* 在一个 while 循环中进行解码
       每次读入 LZW 编码到 curcode 中
       然后进行 LZW 的解码处理
       直到读到 LZW_END_CODE 结束 */
    while (  lzw->str_buf_pos == 0
          && getbits(lzw, &curcode, lzw->curcodesize)
          && curcode != LZW_END_CODE )
    {
        if (curcode == LZW_CLR_CODE)
        {   /* 如果读到的是清除码 */
            resetlzwcodec(lzw); /* reset lzw codec */
        }
        else
        {   /* 如果读到的不是清除码 */
            /* 查找编码表中是否有该编码 */
            find = IsCodeInLZWStringTable(lzw, curcode);

            /* 根据查找结果向编码表中加入新的字符串 */
            if (find) AddToLZWStringTable(lzw, lzw->oldcode, GetFirstByteOfLZWCode(lzw, curcode), FALSE);
            else AddToLZWStringTable(lzw, lzw->oldcode, GetFirstByteOfLZWCode(lzw, lzw->oldcode), FALSE);

            /* 根据 _str_tab_pos 重新计算新的 curcodesize */
            if (   lzw->str_tab_pos == (1 << lzw->curcodesize)
                && lzw->curcodesize < LZW_CODE_SIZE_MAX) // note: 这个限制条件必须要
            {
                /* 增加 curcodesize 的值 */
                lzw->curcodesize++;
            }

            /* 解码当前字符串 */
            DecodeLZWString(lzw, curcode);

            /* oldcode 置为 curcode */
            lzw->oldcode = curcode;
        }
    }

    if (lzw->str_buf_pos <= 0) return EOF;
    else
    {
        lzw->fpos++; // file read/write pos
        return lzw->lzw_str_buf[--lzw->str_buf_pos];
    }
}

int lzw_fputc(int c, void *stream)
{
    LZW *lzw  = (LZW*)stream;
    int  find = -1;

    if (lzw->mode == LZW_MODE_DECODE) return EOF;

    /* 在 LZW 编码表中查找由当前前缀和当前字符组成的字符串 */
    find = FindInLZWStringTable(lzw, lzw->prefixcode, c);
    if (find == -1)
    {   /* 在 LZW 编码表中没有该字符串 */
        /* 以当前的 codesize 写出当前前缀 */
        if (!putbits(lzw, lzw->prefixcode, lzw->curcodesize)) {
            log_printf("failed to write prefixcode !\n");
            return EOF;
        }

        /* 将当前前缀和当前字符所组成的字符串加入编码表 */
        if (!AddToLZWStringTable(lzw, lzw->prefixcode, c, TRUE))
        {   /* 加入失败说明编码表已满 */
            /* 写出一个 LZW_CLEAR_CODE 到比特流 */
            if (!putbits(lzw, LZW_CLR_CODE, lzw->curcodesize)) {
                log_printf("failed to write LZW_CLEAR_CODE !\n");
                return EOF;
            }
            resetlzwcodec(lzw); /* reset lzw codec */
        }
        else 
        {   /* 加入到编码表成功 */
            /* 根据 _str_tab_pos 重新计算新的 curcodesize */
            if (   lzw->str_tab_pos - 1 == (1 << lzw->curcodesize)
                && lzw->curcodesize < LZW_CODE_SIZE_MAX) /* note: 这个限制条件必须要 */
            {
                /* 增加 curcodesize 的值 */
                lzw->curcodesize++;
            }
        }

        /* 置当前前缀码为 currentbyte */
        lzw->prefixcode = c;
    }
    else
    {   /* 在 LZW 编码表中找到该字符串 */
        /* 置当前前缀码为该字符串的编码 */
        lzw->prefixcode = find;
    }

    // file read/write pos
    lzw->fpos++;
    return 0;
}

size_t lzw_fread(void *buffer, size_t size, size_t count, void *stream)
{
    BYTE  *dst   = (BYTE*)buffer;
    size_t total = size * count;
    while (total--)
    {
        *dst = lzw_fgetc(stream);
        if (*dst++ == EOF)
        {
            total++;
            break;
        }
    }
    return (size * count - total);
}

size_t lzw_fwrite(void *buffer, size_t size, size_t count, void *stream)
{
    BYTE  *dst   = (BYTE*)buffer;
    size_t total = size * count;
    while (total--)
    {
        if (EOF == lzw_fputc(*dst++, stream))
        {
            total++;
            break;
        }
    }
    return (size * count - total);
}

int lzw_fseek(void *stream, long offset, int origin)
{
    LZW *lzw  = (LZW*)stream;
    int  skip = 0;

    // don't support seek for lzw encode mode
    if (lzw->mode == LZW_MODE_ENCODE) return EOF;

    // don't support seek if origin is SEEK_END
    if (origin == SEEK_END) return EOF;

    switch (origin)
    {
    case SEEK_SET: offset = offset; break;
    case SEEK_CUR: offset = lzw->fpos + offset; break;
    }

    // already at the offset, return directly
    if (offset == lzw->fpos) return offset;

    // seek lzw file
    skip = offset - lzw->fpos;
    if (skip < 0)
    {
        resetlzwcodec(lzw);
        lzw->str_buf_pos = 0;
        lzw->fpos        = 0;
        fseek(lzw->fp, 0, SEEK_SET);
        skip = offset;
    }
    while (skip--) lzw_fgetc(stream);

    return 0;
}

long lzw_ftell(void *stream) { return ((LZW*)stream)->fpos; }


