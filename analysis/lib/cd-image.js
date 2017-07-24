const fs = require('fs');

const ssGood = 0,
			ssCheckCondition = 2,
			ssConditionMet = 4,
			ssBusy = 8,
			ssIntermediate = 0x10,
			ssIntermediateCondMet = 0x14,
			ssReservationConflict = 0x18,
			ssCommandTerminated = 0x22,
			ssQueueFull = 0x28;
      
const scsiInvalidField = SCSIResult([ssCheckCondition, [5, 0x24, 0]]);
const TOC_DATA_TRACK = 4;

const CD_SECTOR_SIZE = 2048;
const TocHeaderSize = 4;
const TRACK_DATA_SIZE = 8;
const MAXIMUM_NUMBER_TRACKS = 100;
const CDROM_TOC_SIZE = 4 + TRACK_DATA_SIZE * MAXIMUM_NUMBER_TRACKS;


module.exports = {
  create_cd  : create_cd,
  SCSIResult : SCSIResult,

  SCSI_STATUS : {
  }
};

//
// img_file_handle 文件未关闭
// 
function create_cd(img_file_handle) {
  var m_TotalSectorCount = 0;
  var m_TrackRecords = [];
  var m_FirstSectorToTrackMap = {};
  var m_RawSectorBuffer;

  var cd = {
    CD_SECTOR_SIZE            : CD_SECTOR_SIZE,
    AddDataTrack              : AddDataTrack,
    GetSectorCount            : GetSectorCount,
    GetTrackCount             : GetTrackCount,
    GetTrackEndSector         : GetTrackEndSector,
    ReadSectorsBoundsChecked  : ReadSectorsBoundsChecked,
    getSaturnToc              : getSaturnToc,
  };

 return cd;


  function GetSectorCount() {
    return m_TotalSectorCount;
  }


  function GetTrackCount() {
    return m_TrackRecords.length;
  }


  function GetTrackEndSector(trackNum) {
    var track = m_TrackRecords[trackNum];
    return track ? track.EndSector : 0;
  }


  function AddDataTrack(Mode, StartFileOffset, EndFileOffset) {
    imageSectorSize = Mode.MainSectorSize + Mode.SubSectorSize;
    if (!imageSectorSize)
      throw new Error('bad imageSectorSize');

    if ((EndFileOffset - StartFileOffset) % imageSectorSize)
      throw new Error('sector not alignment');

    StartFileOffset += Mode.DataOffsetInSector;
		EndFileOffset += Mode.DataOffsetInSector;

    var stat = fs.fstatSync(img_file_handle);
    if (EndFileOffset > stat.size)
      EndFileOffset = stat.size;

    var info = {
      Mode            : Mode,
      OffsetInFile    : StartFileOffset,
      ImageSectorSize : imageSectorSize,
      SectorCount     : ((EndFileOffset - StartFileOffset) / imageSectorSize),
      StartSector     : m_TotalSectorCount,
    };
    info.EndSector = info.StartSector + info.SectorCount;

    m_TrackRecords.push(info);
		m_FirstSectorToTrackMap[m_TotalSectorCount] = info;
		m_TotalSectorCount += info.SectorCount;

    if (m_RawSectorBuffer == null || m_RawSectorBuffer.length < info.ImageSectorSize) {
      m_RawSectorBuffer = Buffer.alloc(info.ImageSectorSize);
    }
    console.log('\nAdd Data Track', info);
  }


  function find_track_upper_bound(sector) {
    for (var i=0; i<m_TrackRecords.length; ++i) {
      if (m_TrackRecords[i].StartSector > sector) {
        if (i == 0) {
          return m_TrackRecords[i];
        } else {
          return m_TrackRecords[i-1];
        }
      }
    }
    throw new Error('cannot found sector ' + sector);
  }


  function ReadSectorsBoundsChecked(FirstSector, pBuffer, SectorCount) {
    if (m_TrackRecords.length < 1)
      return 0;

    for (done=0, todo=0; done < SectorCount; done += todo) {
      var sector = FirstSector + done;
      var pTrack = find_track_upper_bound(sector);

      var inTrackSectorIndex = sector - pTrack.StartSector;
		  var maxSectorsFromThisTrack = pTrack.EndSector - inTrackSectorIndex;

      todo = Math.min(maxSectorsFromThisTrack, (SectorCount - done));
      if (!todo)
        return done * CD_SECTOR_SIZE;

      var fileOffset = pTrack.OffsetInFile + (pTrack.ImageSectorSize * inTrackSectorIndex);
			if (m_RawSectorBuffer.length < pTrack.ImageSectorSize) 
        throw new Error('bad m_RawSectorBuffer length');

      if (pTrack.ImageSectorSize == CD_SECTOR_SIZE) {
        var pos = (done * CD_SECTOR_SIZE);
        var rlen = todo * CD_SECTOR_SIZE;

        if (fs.readSync(img_file_handle, pBuffer, pos, rlen, fileOffset) != rlen) {
          return (done + todo) * CD_SECTOR_SIZE;
        }
      } else {
        var rlen = pTrack.ImageSectorSize;
        for (var i = 0; i < todo; i++) {
          if (fs.readSync(img_file_handle, m_RawSectorBuffer, 0, rlen, fileOffset) != rlen) {
            return done * CD_SECTOR_SIZE;
          }
          m_RawSectorBuffer.copy(pBuffer, 
            (done+i) * CD_SECTOR_SIZE, 0, CD_SECTOR_SIZE);
        }
      }
    }
    return SectorCount * CD_SECTOR_SIZE;
  }


  function getSaturnToc() {
    var trackCount = GetTrackCount();
    var request = {
      RequestType : null,
      DataBuffer : {
        Length : [],
        FirstTrack : 1,
        LastTrack : trackCount,
        TrackData : [],
      },
      DataTransferSize : CDROM_TOC_SIZE * trackCount,
      pCDB : {
        OperationCode : null,
        Reserved0 : null,
        Msf : true,
        Reserved1 : null,
        LogicalUnitNumber : null,
        Format2 : null,
        Reserved2 : null,
        Reserved3 : null,
        StartingTrack : 1,
        AllocationLength : null,
        Control : null,
        Format : null,
      }
    }
    OnReadTOC(request);
    return request;
  }


  //
  // request : { RequestType, DataBuffer, DataTransferSize, pCDB }
  // pCDB : { OperationCode, Reserved0 1, Msf 1, Reserved1 3, LogicalUnitNumber 3
  //    Format2 4, Reserved2 4, Reserved3[3], StartingTrack, 
  //    AllocationLength[2], Control 6, Format 2 } All UCHAR
  // DataBuffer : [{ UCHAR Length[2], FirstTrack, LastTrack, 
  //    TrackData[MAXIMUM_NUMBER_TRACKS] }]
  //
  function OnReadTOC(request) {
    var trackCount = GetTrackCount();
    var tocLen = 2 + TRACK_DATA_SIZE * (trackCount + 1);

    var bias = request.pCDB.StartingTrack - 1;
	  if (bias == -1)
		  bias = 0;
	  else if (bias >= trackCount)
		  return scsiInvalidField;

    if (!request.DataBuffer || (request.DataTransferSize < TocHeaderSize))
		  return scsiInvalidField;
	  if ((tocLen + 2) > CDROM_TOC_SIZE)
		  return scsiInvalidField;

    var pData = request.DataBuffer;
	  pData.Length[0] = (tocLen >> 8) & 0xFF;
	  pData.Length[1] = (tocLen >> 0) & 0xFF;

	  pData.FirstTrack = 1;
	  pData.LastTrack = trackCount + 1;

    var reportedTrackCount = trackCount - bias;

    for (var i = 0; i < reportedTrackCount; i++) {
      var pTrack = pData.TrackData[i] = {};
		  InitializeTrackRecord(pTrack, i + 1 + bias, 
          (i + bias) ? GetTrackEndSector(i + bias - 1) : 0, 
          request.pCDB.Msf);
	  }

    var pTrack = pData.TrackData[reportedTrackCount] = {};
    InitializeTrackRecord(pTrack, 0xAA, 
        GetTrackEndSector(trackCount - 1), request.pCDB.Msf);
  }
}


function InitializeTrackRecord(pTrack, TrackNumber, SectorNumber, bMSF) {
  pTrack.Control = TOC_DATA_TRACK;
	pTrack.Adr = 1;
	pTrack.TrackNumber = TrackNumber;
  pTrack.Address = [];
	if (bMSF)
		BuildMSFTrackAddress(SectorNumber, pTrack.Address);
	else
    SwapByteOrder32(pTrack.Address, SectorNumber);
}


function BuildMSFTrackAddress(SectorNumber, pAddr) {
	if (!pAddr)
		return;
	SectorNumber += 150;
	pAddr[3] = (SectorNumber % 75);
	SectorNumber /= 75;
	pAddr[2] = parseInt(SectorNumber % 60);
	pAddr[1] = parseInt(SectorNumber / 60);
	pAddr[0] = 0;
}


function SwapByteOrder32(pAddr, val) {
  pAddr[3] = (val & 0xFF000000) >> 24;
  pAddr[2] = (val & 0x00FF0000) >> 16;
  pAddr[1] = (val & 0x0000FF00) >> 8;
  pAddr[0] = (val & 0x000000FF);
}


function SCSIResult(parm) {
  return {
    Status : parm[0],
    SenseData : {
      Key  : parm[1][0],
      ASC  : parm[1][1],
      ASCQ : parm[1][2],
    }
  }
}