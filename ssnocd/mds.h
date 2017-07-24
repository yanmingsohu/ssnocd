#ifndef __MDS_LIB_H__
#define __MDS_LIB_H__
#include "lib.h"

#pragma pack(1)


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


//Raw sector size is always 2352 bytes. Data area size in a sector is 2048 bytes.
typedef enum CDTrackModeType_
{
  kUnknown     = 0x00,
  kAUDIO       = 0x01,
  kMODE1       = 0x02,	//Raw sector = [12 bytes sync] [4 bytes hdr] [2048 bytes data] [ecc + etc]

  kMODE2       = 0x103,	//Raw sector = [12 bytes sync] [4 bytes hdr] [8 bytes subhdr] [2048 bytes data] [ecc + etc]
  kMODE2_FORM1 = 0x104,
  kMODE2_FORM2 = 0x105,
  kMODE2_MIXED = 0x106,

  kMODE2_MASK	= 0x100,
} CDTrackModeType;


typedef struct MDSHeader_
{
  UCHAR Signature[16]; /* "MEDIA DESCRIPTOR" */
  UCHAR Version[2]; /* Version ? */
  USHORT MediumType; /* Medium type */
  USHORT SessionCount; /* Number of sessions */
  USHORT unused1[2];
  USHORT BCALength; /* Length of BCA data (DVD-ROM) */
  unsigned unused2[2];
  unsigned BCAOffset; /* Offset to BCA data (DVD-ROM) */
  unsigned unused3[6]; /* Probably more offsets */
  unsigned DiskStructuresOffset; /* Offset to disc structures */
  unsigned unused4[3]; /* Probably more offsets */
  unsigned SessionsBlocksOffset; /* Offset to session blocks */
  unsigned DpmBlocksOffset; /* offset to DPM data blocks */
  unsigned EncryptionKeyOffset;
} MDSHeader; /* length: 92 bytes */


typedef struct MDSSessionBlock_
{
  int SessionStart; /* Session's start address */
  int SessionEnd; /* Session's end address */
  USHORT SessionNumber; /* (Unknown) */
  UCHAR TotalBlockCount; /* Number of all data blocks. */
  UCHAR LeadInBlockCount; /* Number of lead-in data blocks */
  USHORT FirstTrack; /* Total number of sessions in image? */
  USHORT LastTrack; /* Number of regular track data blocks. */
  unsigned unused; /* (unknown) */
  unsigned TrackBlocksOffset; /* Offset of lead-in+regular track data blocks. */
} MDSSessionBlock; /* length: 24 bytes */


typedef struct MSFAddress_
{
  UCHAR M;
  UCHAR S;
  UCHAR F;
} MSFAddress;


typedef struct MDSTrackBlock_
{
  UCHAR Mode; /* Track mode */
  UCHAR SubchannelMode; /* Subchannel mode */
  UCHAR AdrCtk; /* Adr/Ctl */
  UCHAR unused1; /* Track flags? */
  UCHAR TrackNumber; /* Track number. (>0x99 is lead-in track) */

  unsigned unused2;
  MSFAddress MSF;
  unsigned ExtraOffset; /* Start offset of this track's extra block. */
  USHORT SectorSize; /* Sector size. */

  UCHAR unused3[18];
  unsigned StartSector; /* Track start sector (PLBA). */
  ULONGLONG StartOffset; /* Track start offset. */
  UCHAR Session; /* Session or index? */
  UCHAR unused4[3];
  unsigned FooterOffset; /* Start offset of footer. */
  UCHAR unused5[24];
} MDSTrackBlock; /* length: 80 bytes */


typedef struct MDSTrackExtraBlock_
{
  unsigned Pregap; /* Number of sectors in pregap. */
  unsigned Length; /* Number of sectors in track. */
} MDSTrackExtraBlock; /* length: 8 bytes */


typedef struct CDTrackMode_
{
  CDTrackModeType Type;
  unsigned MainSectorSize;
  unsigned SubSectorSize;
  unsigned DataOffsetInSector;
} CDTrackMode;


typedef struct ParsedTrackRecord_
{
  ULONGLONG OffsetInFile;

  unsigned SectorSize;
  unsigned SectorCount;

  CDTrackMode Mode;
  unsigned TrackFileIndex;
} ParsedTrackRecord;


typedef struct MDSFooter_
{
  unsigned FilenameOffset; /* Start offset of image filename. */
  unsigned WidecharFilename; /* Seems to be set to 1 if widechar filename is used */
  unsigned unused1;
  unsigned unused2;
} MDSFooter; /* length: 16 bytes */


//
// 首先打开该方法解析 mds
//
int openMds(char* filename);
//
// 关闭 mds 镜像
//
void mds_close();
//
// 获取轨道数量
//
int get_track_count();
//
// 读取 toc, 由该方法管理内存, 无需创建/释放
// 返回 toc 的数量
//
int get_toc(Toc **toc);


#endif