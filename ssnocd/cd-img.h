#ifndef ___CD_IMG_H__
#define ___CD_IMG_H__
#include "lib.h"

#define MAX_TOC_NUM     30
#define MAX_TRACK_NUM   MAX_TOC_NUM
#define MAX_SECTOR_SIZE 2352


enum {CD_SECTOR_SIZE = 2048};


//Raw sector size is always 2352 bytes. Data area size in a sector is 2048 bytes.
typedef enum CDTrackModeType_ {
  kUnknown     = 0x00,
  kAUDIO       = 0x01,
  kMODE1       = 0x02,	//Raw sector = [12 bytes sync] [4 bytes hdr] [2048 bytes data] [ecc + etc]

  kMODE2       = 0x103,	//Raw sector = [12 bytes sync] [4 bytes hdr] [8 bytes subhdr] [2048 bytes data] [ecc + etc]
  kMODE2_FORM1 = 0x104,
  kMODE2_FORM2 = 0x105,
  kMODE2_MIXED = 0x106,

  kMODE2_MASK	= 0x100,
} CDTrackModeType;


typedef struct TOC_ {
  UCHAR ctrladr;
  UCHAR tno;
  UCHAR point;
  UCHAR min;
  UCHAR sec;
  UCHAR frame;
  UCHAR zero;
  UCHAR pmin;
  UCHAR psec;
  UCHAR pframe;
} Toc;


typedef struct CDTrackMode_ {
  CDTrackModeType Type;
  unsigned MainSectorSize;
  unsigned SubSectorSize;
  unsigned DataOffsetInSector;
} CDTrackMode;


typedef struct TrackInfo_ {
  CDTrackMode Mode;	//Not currently used

  ULONGLONG OffsetInFile;
  unsigned ImageSectorSize;	//First 2048 bytes of each sector will be read
  unsigned SectorCount;

  unsigned StartSector;
  unsigned EndSector;
} TrackInfo;


/* 读取 toc, 由该方法管理内存, 无需创建/释放
   返回 toc 的数量 */
int cd_get_toc(Toc **toc, int index);

/* 获取 toc 数量 */
int cd_get_toc_count();

/* 在添加完 toc 后必须调用该方法对结果排序 */
void cd_sort_toc();

/* 重置所有状态 */
void cd_reset();

/* 添加一个 toc 信息, 这会让 toc + 1 */
int cd_add_toc(Toc * src);

/* 添加一个数据轨道 */
int cd_add_data_track(CDTrackMode *Mode,  
  ULONGLONG StartFileOffset, ULONGLONG EndFileOffset);

/* 返回扇区数量 */
UINT cd_get_sector_count();

/* 返回数据轨道的数量 */
UINT cd_get_track_count();
UINT cd_get_track_end_sector(unsigned TrackNumber);

/* 读取一个扇区 2352/2048 字节 */
UINT cd_read_sector(unsigned sector, Byte *pBuffer, int buflen);


#endif // ___CD_IMG_H__