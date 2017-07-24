#include "mds.h"
#include "fs.h"
#include "lib.h"

#define MDF_NAME_LEN    128
#define MAX_TOC_NUM     30

static pByte pMdfFile[MDF_NAME_LEN] = {0};
static Toc pToc[MAX_TOC_NUM] = {0};

static FileState state = Closed;
static char toc_index = 0;
static char track_count = 0;


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


int DetectDataOffsetInSector(
      unsigned SectorSize,unsigned DefaultOffset,ULONGLONG FirstSectorOffset) {
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
           int *pSuccessful,
           ParsedTrackRecord *parsedTrack) {
  *pSuccessful = 0;
  for (unsigned i = 0; i < pSessionHeader->TotalBlockCount; i++) {
    MDSTrackBlock track;
    MDSTrackExtraBlock extraInfo;
    fs_seek(pSessionHeader->TrackBlocksOffset + i * sizeof(MDSTrackBlock));
    if (fs_read(&track, sizeof(track)) != sizeof(track))
      return;

    DEBUG("\n  TrackNumber %d (0x%x) Begin\n", track.TrackNumber, track.TrackNumber);
    DEBUG("    Mode %d\n", track.Mode);
    DEBUG("    SubchannelMode %d\n", track.SubchannelMode);
    DEBUG("    AdrCtk %d\n", track.AdrCtk);
    DEBUG("    MSF %d:%d:%d\n", track.MSF.M, track.MSF.S, track.MSF.F);
    DEBUG("    SectorSize %d\n", track.SectorSize);
    DEBUG("    StartOffset %d\n", track.StartOffset);
    DEBUG("    Session %d\n", track.Session);
    DEBUG("    FooterOffset %d\n", track.FooterOffset);
    DEBUG("    SubchannelMode %d\n", track.SubchannelMode);
    DEBUG("    ExtraOffset %d\n", track.ExtraOffset);

    if (!track.ExtraOffset)
      continue;

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

    DEBUG("      Ext OffsetInFile %d\n", parsedTrack->OffsetInFile);
    DEBUG("      Ext SectorSize %d\n", parsedTrack->SectorSize);
    DEBUG("      Ext SectorCount %d\n", parsedTrack->SectorCount);
    DEBUG("      Ext Type %d\n", parsedTrack->Mode.Type);
    DEBUG("      Ext MainSectorSize %d\n", parsedTrack->Mode.MainSectorSize);
    DEBUG("      Ext SubSectorSize %d\n", parsedTrack->Mode.SubSectorSize);
    DEBUG("      Ext DataOffsetInSector %d\n", parsedTrack->Mode.DataOffsetInSector);
  }

  *pSuccessful = 1;				
}


int openMds(char* filename) {
  assert(state != Opened);

  static MDSHeader hdr;
  fs_open(filename);
  fs_seek(0);
  fs_read(&hdr, sizeof(hdr));
  DEBUG("Has %d Sessions\n", hdr.SessionCount);

  // only one session
  //for (unsigned i = 0; i < hdr.SessionCount; i++)
  const int i = 0;
  {
    MDSSessionBlock session;
    fs_seek(hdr.SessionsBlocksOffset + i * sizeof(MDSSessionBlock));
    if (fs_read(&session, sizeof(MDSSessionBlock)) != sizeof(MDSSessionBlock))
      return FAILED;

    DEBUG(" And %d Track\n", session.TotalBlockCount);

    int bSessionOk = 0;
    ParsedTrackRecord track = {0,};
    MDSSession(&hdr, &session, filename, &bSessionOk, &track);

    if (bSessionOk) {
      track.Mode.DataOffsetInSector = 
          DetectDataOffsetInSector(track.SectorSize, 0, track.OffsetInFile);

      if (fs_state() == Closed)
        fs_open(filename);
    }
  }
  track_count = hdr.SessionCount;
  state = Opened;
  return SUCCESS;
}


int get_track_count() {
  assert(state == Opened);
  return track_count;
}


void mds_close() {
  assert(state == Opened);
  fs_close();
  state = Closed;
  track_count = 0;
  toc_index = 0;
  return;
}