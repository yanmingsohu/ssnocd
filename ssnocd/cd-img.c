#include "cd-img.h"
#include "fs.h"


static char       toc_count           = 0;
static Toc        pToc[MAX_TOC_NUM]   = {0};
static Byte sorted_toc[MAX_TOC_NUM]   = {0};
static TrackInfo tracks[MAX_TOC_NUM]  = {0};
static UINT m_TotalSectorCount        = 0;
static UINT track_record_count        = 0;


int cd_add_toc(Toc * src) {
  Toc * toc = pToc + toc_count;
  memcpy(toc, src, sizeof(Toc));
  return ++toc_count;
}


void cd_sort_toc() {
  int i, j, sw;
  UCHAR a, b;

  for (i=0; i<toc_count; ++i) {
    sorted_toc[i] = i;
  }

  for (i=0; i<toc_count; ++i) {
    for (j=i+1; j<toc_count; ++j) {
      a = (pToc + sorted_toc[i])->point;
      b = (pToc + sorted_toc[j])->point;

      if (a > b) {
        sw = sorted_toc[i];
        sorted_toc[i] = sorted_toc[j];
        sorted_toc[j] = sw;
      }
    }
  }
}


int cd_get_toc_count() {
  return toc_count;
}


int cd_get_toc(Toc ** toc, int index) {
  assert(toc_count > 0);
  if (index >= toc_count || index < 0)
    return FAILED;
  *toc = pToc + sorted_toc[index];
  return SUCCESS;
}


void cd_reset() {
  toc_count = 0;
  m_TotalSectorCount = 0;
}


int cd_add_data_track(CDTrackMode *Mode,  
    ULONGLONG StartFileOffset, ULONGLONG EndFileOffset) 
{
  unsigned imageSectorSize = Mode->MainSectorSize + Mode->SubSectorSize;
  if (!imageSectorSize)
    return FAILED;
  if ((unsigned)((EndFileOffset - StartFileOffset) % imageSectorSize))
    return FAILED;

  StartFileOffset += Mode->DataOffsetInSector;
  EndFileOffset += Mode->DataOffsetInSector;

  ULONGLONG fileSize = fs_size();

  if (EndFileOffset > fileSize)
    EndFileOffset = fileSize;

  TrackInfo info = {
    *Mode, 
    StartFileOffset, 
    imageSectorSize, 
    (unsigned)((EndFileOffset - StartFileOffset) / imageSectorSize)
  };

  info.ImageSectorSize = imageSectorSize;
  info.StartSector = m_TotalSectorCount;
  info.EndSector = info.StartSector + info.SectorCount;

  // tracks.push_back(info);
  tracks[track_record_count] = info;
  // m_FirstSectorToTrackMap[m_TotalSectorCount] = info;
  m_TotalSectorCount += info.SectorCount;
  ++track_record_count;

  return SUCCESS;
}


UINT cd_get_sector_count() {
  return m_TotalSectorCount;
}


UINT cd_get_track_count() {
  return track_record_count;
}


UINT cd_get_track_end_sector(unsigned TrackNumber) {
  if (TrackNumber >= track_record_count)
    return 0;
  return tracks[TrackNumber].EndSector;
}


UINT cd_read_sector(unsigned sector, Byte *pBuffer, int buflen) {
  if (!track_record_count)
    return 0;

  TrackInfo *pTrack = 0;
  for (int i=0; i<track_record_count; ++i) {
    if (tracks[i].StartSector > sector) {
      if (i > 0) --i;
      pTrack = tracks + i;
      break;
    }
  }

  if (pTrack == 0)
    return 0;

  unsigned inTrackSectorIndex = sector - pTrack->StartSector;
  unsigned maxSectorsFromThisTrack = pTrack->EndSector - inTrackSectorIndex;

  if (!maxSectorsFromThisTrack)
    return 0;

  ULONGLONG fileOffset = pTrack->OffsetInFile 
      + (pTrack->ImageSectorSize * (ULONGLONG)inTrackSectorIndex);

  if (fs_seek(fileOffset) == FS_FAILED)
    return 0;

  if (buflen < pTrack->ImageSectorSize) 
    return 0;


  int total = pTrack->ImageSectorSize;
  int rlen = 0;
  do {
    rlen = fs_read(pBuffer + rlen, total);
    total -= rlen;
  } while(rlen > 0 && total > 0);
 
  return pTrack->ImageSectorSize;
}