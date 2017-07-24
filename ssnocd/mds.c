#include "mds.h"
#include "fs.h"
#include "lib.h"
#include "cd-img.h"

#define MDF_NAME_LEN    128

static pByte   pMdfFile[MDF_NAME_LEN] = {0};
static FileState  state               = Closed;


static const CDTrackModeType s_MDSTrackModeTypes[] = {
  kMODE2,			//Code 0
  kAUDIO,			//Code 1
  kMODE1,			//Code 2
  kMODE2,			//Code 3
  kMODE2_FORM1,	//Code 4
  kMODE2_FORM2,	//Code 5
  kUnknown,		//Code 6
  kMODE2,			//Code 7
};


int add_toc(MDSTrackBlock *track, MDSTrackExtraBlock *extraInfo) {
  int num = cd_get_toc_count();
  Toc toc;
  toc.ctrladr = ((track->AdrCtk & 0xF0) >> 4) + ((track->AdrCtk & 0x0F) << 4);
  toc.tno     = 0;
  toc.point   = track->TrackNumber;
  toc.min     = num + 150;
  toc.sec     = num + 150;
  toc.frame   = num + 150;
  toc.zero    = 0;
  toc.pmin    = track->MSF.M;
  toc.psec    = track->MSF.S;
  toc.pframe  = track->MSF.F;
  cd_add_toc(&toc);
}


int DetectDataOffsetInSector(
    unsigned SectorSize,
    unsigned DefaultOffset,
    ULONGLONG FirstSectorOffset) 
{
  if (SectorSize == 2048)
    return DefaultOffset;
  if (pMdfFile[0] == 0)
    return DefaultOffset;

  char SectorHeader[16];

  fs_close();
  fs_open(pMdfFile);
  fs_seek(FirstSectorOffset);

  int len = fs_read(SectorHeader, sizeof(SectorHeader));
  fs_close();

  if (len != sizeof(SectorHeader))
    return DefaultOffset;

  static const unsigned char s_RawSync[12] = { 
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 };

  if (memcmp(SectorHeader, s_RawSync, sizeof(s_RawSync))) {
    for (int i = 0; i < sizeof(SectorHeader); i++)
      if (SectorHeader[i])
        return DefaultOffset;
    return 0;	//Does not seem to be raw sector format
  }

  switch (SectorHeader[15]) {
  case 1:
    return 16;	//Raw MODE1 sector
  case 2:
    return 24;	//Raw MODE2 sector
  default:
    return DefaultOffset;	//Unknown sector
  }
}


void get_mdf_filename(char* in_mds_name, char* inout_mdf_name) {
  if ((inout_mdf_name[0] == '*') && (inout_mdf_name[1] == '.')) {
    char ext[10] = {0};
    strcpy(ext, inout_mdf_name+1, sizeof(ext));
    strcpy(inout_mdf_name, in_mds_name, MDF_NAME_LEN);
    int last = 0;
    while (inout_mdf_name[last++] != 0);
    while (inout_mdf_name[--last] != '.' && last > 0);
    strcpy(inout_mdf_name + last, &ext, sizeof(ext));
  }
  printf("MDS: %s \nMDF: %s\n", in_mds_name, inout_mdf_name);
}


void MDSSession(MDSHeader *pHeader, 
     MDSSessionBlock *pSessionHeader, 
     char *FullFilePath,
     ParsedTrackRecord *tracks,
     int *track_len)
{
  ParsedTrackRecord *parsedTrack = 0;

  for (unsigned i = 0; i < pSessionHeader->TotalBlockCount; i++) {
    MDSTrackBlock track;
    MDSTrackExtraBlock extraInfo;

    fs_seek(pSessionHeader->TrackBlocksOffset + i * sizeof(MDSTrackBlock));
    if (fs_read(&track, sizeof(track)) != sizeof(track))
      return;

    DEBUG("\n"SP1"TrackNumber %d (0x%x) Begin\n", track.TrackNumber, track.TrackNumber);
    DEBUG(SP2"Mode %x\n", track.Mode);
    DEBUG(SP2"SubchannelMode %d\n", track.SubchannelMode);
    DEBUG(SP2"AdrCtk %x\n", track.AdrCtk);
    DEBUG(SP2"MSF %d:%d:%d\n", track.MSF.M, track.MSF.S, track.MSF.F);
    DEBUG(SP2"SectorSize %d\n", track.SectorSize);
    DEBUG(SP2"StartOffset %d\n", track.StartOffset);
    DEBUG(SP2"Session %d\n", track.Session);
    DEBUG(SP2"FooterOffset %d\n", track.FooterOffset);
    DEBUG(SP2"SubchannelMode %d\n", track.SubchannelMode);
    DEBUG(SP2"ExtraOffset %d\n", track.ExtraOffset);

    if (!track.ExtraOffset) {
      add_toc(&track, 0);
      continue;
    }

    if ((pHeader->MediumType & 0x10))	//If this is a DVD, use different handling
    {
      extraInfo.Length = track.ExtraOffset;
      extraInfo.Pregap = 0;
    }
    else
    {
      fs_seek(track.ExtraOffset);
      if (fs_read(&extraInfo, sizeof(extraInfo)) != sizeof(extraInfo))
        return;

      if (!extraInfo.Length)
        continue;
    }

    parsedTrack = tracks + (*track_len);

    if (track.FooterOffset)
    {
      MDSFooter footer;
      fs_seek(track.FooterOffset);
      if (fs_read(&footer, sizeof(footer)) != sizeof(footer))
        return;
      if (!footer.FilenameOffset)
        continue;

      assert(footer.WidecharFilename == 0);

      if (pMdfFile[0] == 0) {
        fs_seek(footer.FilenameOffset);
        int len = fs_read_string(pMdfFile, MDF_NAME_LEN);
        if (len <= 0)
          return;
        get_mdf_filename(FullFilePath, pMdfFile);
      }

      parsedTrack->TrackFileIndex = 0;
    }

    parsedTrack->OffsetInFile = track.StartOffset;
    parsedTrack->SectorSize = track.SectorSize;
    parsedTrack->SectorCount = extraInfo.Length;

    if ((track.Mode & 0x07) >= __countof(s_MDSTrackModeTypes))
      return;

    parsedTrack->Mode.Type = s_MDSTrackModeTypes[track.Mode & 0x07];
    parsedTrack->Mode.MainSectorSize = track.SectorSize;
    parsedTrack->Mode.SubSectorSize = 0;

    if (track.SubchannelMode == 8)
    {
      parsedTrack->Mode.MainSectorSize -= 96;
      parsedTrack->Mode.SubSectorSize = 96;
    }

    parsedTrack->Mode.DataOffsetInSector = 
        DetectDataOffsetInSector(
          parsedTrack->SectorSize, 0, parsedTrack->OffsetInFile);

    if (fs_state() == Closed)
        fs_open(FullFilePath);

    DEBUG(SP3"Ext OffsetInFile %d\n", parsedTrack->OffsetInFile);
    DEBUG(SP3"Ext SectorSize %d\n", parsedTrack->SectorSize);
    DEBUG(SP3"Ext SectorCount %d\n", parsedTrack->SectorCount);
    DEBUG(SP3"Ext Type %d\n", parsedTrack->Mode.Type);
    DEBUG(SP3"Ext MainSectorSize %d\n", parsedTrack->Mode.MainSectorSize);
    DEBUG(SP3"Ext SubSectorSize %d\n", parsedTrack->Mode.SubSectorSize);
    DEBUG(SP3"Ext DataOffsetInSector %d\n", parsedTrack->Mode.DataOffsetInSector);
    *track_len += 1;
    add_toc(&track, &extraInfo);
  }
}


int mds_open(char* filename) {
  assert(state != Opened);

  ParsedTrackRecord tracks[MAX_TOC_NUM] = {0,};
  int track_len  = 0;
  MDSHeader hdr;

  fs_open(filename);
  fs_seek(0);
  fs_read(&hdr, sizeof(hdr));

  DEBUG("Has %d Sessions\n", hdr.SessionCount);

  if (hdr.SessionCount >= MAX_TOC_NUM) {
    DEBUG(" A Lot of Session count %d\n", hdr.SessionCount);
  }

  // only one session
  int i = 0;
  // for (unsigned i = 0; i < hdr.SessionCount; i++) {
    MDSSessionBlock session;
    fs_seek(hdr.SessionsBlocksOffset + i * sizeof(MDSSessionBlock));
    if (fs_read(&session, sizeof(MDSSessionBlock)) != sizeof(MDSSessionBlock))
      return FAILED;

    DEBUG(" And %d Track\n", session.TotalBlockCount);

    MDSSession(&hdr, &session, filename, tracks, &track_len);
    DEBUG(" %d tracks parsed in session %d\n", track_len, i);
  // }

  // 文件打开失败
  assert(pMdfFile[0] != 0);

  cd_sort_toc();
  // close MDS then open MDF
  fs_close();
  fs_open(pMdfFile);

  for (i=0; i<track_len; ++i) {
    ULONGLONG end = tracks[i].OffsetInFile 
                  + (ULONGLONG)tracks[i].SectorSize 
                  * tracks[i].SectorCount;
    cd_add_data_track(&tracks[i].Mode, tracks[i].OffsetInFile, end);
  }

  state = Opened;
  return SUCCESS;
}


void mds_close() {
  assert(state == Opened);
  fs_close();
  cd_reset();
  state = Closed;
  pMdfFile[0] = 0;
  return;
}