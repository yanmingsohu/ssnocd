const fs      = require('fs');
const assert  = require('assert');
const path    = require('path');
const cdimg   = require('./cd-image.js');

const Signature             = "MEDIA DESCRIPTOR";
const MDSSessionBlockSize   = 24;
const MDSHeaderSize         = 92;
const MDSTrackBlockSize     = 80;
const MDSFooterSize         = 16;

const kUnknown     = 0x00;
const kAUDIO       = 0x01;
const kMODE1       = 0x02;	//Raw sector = [12 bytes sync] [4 bytes hdr] [2048 bytes data] [ecc + etc]
const kMODE2       = 0x103;	//Raw sector = [12 bytes sync] [4 bytes hdr] [8 bytes subhdr] [2048 bytes data] [ecc + etc]
const kMODE2_FORM1 = 0x104;
const kMODE2_FORM2 = 0x105;
const kMODE2_MIXED = 0x106;
const kMODE2_MASK	 = 0x100;

const s_MDSTrackModeTypes = [
	kMODE2,			//Code 0
	kAUDIO,			//Code 1
	kMODE1,			//Code 2
	kMODE2,			//Code 3
	kMODE2_FORM1,	//Code 4
	kMODE2_FORM2,	//Code 5
	kUnknown,		//Code 6
	kMODE2,			//Code 7
];

const s_RawSync = Buffer.from([ 
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00 ]);


module.exports = {
  openMds : openMds,
};


function openMds(filename, cb) {
  try {
    var offset    = 0;
    var mds       = fs.readFileSync(filename);
    var br        = bin_reader(mds);
    var mdf_files = {};

    var inf = {
      header : {},
      sessions : [],
    };

    read_header(inf.header);
    read_session(inf.sessions);

    var ret = {
      inf : inf,
      OpenFirstMDFFile : OpenFirstMDFFile,
      open_cd_img : open_cd_img,
    };

  return cb(null, ret);


    function open_cd_img() {
      if (inf.sessions.length > 1) {
        log('WARN: too many sessions');
      } else if (inf.sessions.length < 1) {
        throw new Error('no session');
      }
      
      var cd = cdimg.create_cd(OpenFirstMDFFile());

      var session = inf.sessions[0];
      for (var i=0; i< session.tracks.length; ++i) {
        var p = session.tracks[i].extraInfo.ParsedTrackRecord;
        cd.AddDataTrack(p.Mode, p.OffsetInFile, 
          p.OffsetInFile + p.SectorSize * p.SectorCount);
      }

      return cd;
    }


    function OpenFirstMDFFile() {
      for (var f in mdf_files)
          return mdf_files[f];
      throw new Error('cannot fount MDF file');
    }


    function read_track(s) {
      for (var i=0; i<s.TotalBlockCount; ++i) {
        br.set_offset(s.TrackBlocksOffset + i*MDSTrackBlockSize);
        var begin = br.get_offset();
        var track = {};

        track.Mode            = br.readByte(1);
        track.SubchannelMode  = br.readByte(1);
        track.AdrCtk          = br.readByte(1);
        track.unused1         = br.readByte(1);
        track.TrackNumber     = br.readByte(1);
        track.unused2         = br.readInt(1);
        track.MSFAddress      = read_msf();
        track.ExtraOffset     = br.readInt(1);
        track.SectorSize      = br.readShort(1);
        track.unused3         = br.readByte(18);
        track.StartSector     = br.readInt(1);
        track.StartOffset     = br.readLong(1);
        track.Session         = br.readByte(1);
        track.unused4         = br.readByte(3);
        track.FooterOffset    = br.readInt(1);
        track.unused5         = br.readByte(24);

        track.StartOffset = track.StartOffset[0] + track.StartOffset[1] * 0xFFFFFFFF;

        assert.equal(MDSTrackBlockSize, br.get_offset()-begin, 'bad track size');
        
        track.extraInfo = {};
        log('\nTrack', s.SessionNumber, '-', i, track);
        if (read_extra_info(track, track.extraInfo)) {
          s.tracks.push(track);
          log('\n>>> Extra Info', track.extraInfo);
        }
      }
    }


    function read_extra_info(t, e) {
      if (!t.ExtraOffset)
        return;

      br.set_offset(t.ExtraOffset);
      e.Pregap = br.readInt(1);
      e.Length = br.readInt(1);
      if (!e.Length) 
        return;

      var p = e.ParsedTrackRecord = { Mode: {} };
      var MDFFiles = e.MDFFiles = [];

      if (t.FooterOffset) {
        var f = {};
        br.set_offset(t.FooterOffset);
        f.FilenameOffset   = br.readInt(1);
        f.WidecharFilename = br.readInt(1);
        f.unused1          = br.readInt(1);
        f.unused2          = br.readInt(1);

        if (!f.FilenameOffset)
          return;

        if (f.WidecharFilename)
          log('WARN: WidecharFilename');

        var fn = br.readCString(f.FilenameOffset);
        if ((fn.length >= 2) && (fn[0] == '*') && (fn[1] == '.')) {
          var ext = path.extname(fn);
          fn = path.join( path.dirname(filename),
               path.basename(filename, path.extname(filename)) ) + ext;
          fn = path.normalize(fn);
        }

        MDFFiles.push(fn);
        p.TrackFileIndex = MDFFiles.length - 1;
      }

      p.OffsetInFile = t.StartOffset;
      p.SectorSize   = t.SectorSize;
			p.SectorCount  = e.Length;

      p.Mode.Type = s_MDSTrackModeTypes[t.Mode & 0x07];
			p.Mode.MainSectorSize = t.SectorSize;
			p.Mode.SubSectorSize = 0;

      if (t.SubchannelMode == 8) {
				p.Mode.MainSectorSize -= 96;
				p.Mode.SubSectorSize = 96;
			}
  
      var fn = MDFFiles[p.TrackFileIndex];
      var pMdfFile;
      if (mdf_files[fn]) {
        pMdfFile = mdf_files[fn];
      } else {
        pMdfFile = fs.openSync(fn, 'r');
        mdf_files[fn] = pMdfFile;
      }
      if (pMdfFile) {
        t.MdfFile = pMdfFile;
	      p.Mode.DataOffsetInSector = 
            DetectDataOffsetInSector(pMdfFile, p.SectorSize, 0, p.OffsetInFile);
      }

      return true;
    }


    function DetectDataOffsetInSector(pFile, SectorSize, DefaultOffset, FirstSectorOffset) {
      if (!pFile)
		    return DefaultOffset;
	    if (SectorSize == 2048)
		    return DefaultOffset;

      var SectorHeader = Buffer.alloc(16);
      var rlen = fs.readSync(pFile, SectorHeader, 0, 16, FirstSectorOffset);
      if (rlen != SectorHeader.length) 
        return DefaultOffset;

      if (SectorHeader.compare(s_RawSync, 0, 12, 0, 12)) {
        for (var i = 0; i < SectorHeader.length; i++)
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


    function read_session(s) {
      br.set_offset(inf.header.SessionsBlocksOffset);
      for (var i=0; i<inf.header.SessionCount; ++i) {
        var begin = br.get_offset();
        var block = {};

        block.SessionStart      = br.readSint(1);
        block.SessionEnd        = br.readSint(1);
        block.SessionNumber     = br.readShort(1);
        block.TotalBlockCount   = br.readByte(1);
        block.LeadInBlockCount  = br.readByte(1);
        block.FirstTrack        = br.readShort(1);
        block.LastTrack         = br.readShort(1);
        block.unused            = br.readInt(1);
        block.TrackBlocksOffset = br.readInt(1);

        assert.equal(MDSSessionBlockSize, br.get_offset()-begin, 'bad session size');
        s.push(block);
        log('\nSession', block.SessionNumber, block);
        block.tracks = [];
        read_track(block);
      }
    }


    function read_header(inf) {
      inf.Signature             = br.readString(16);
      inf.Version               = br.readByte(2);
      inf.MediumType            = br.readShort(1);
      inf.SessionCount          = br.readShort(1);
      inf.unused1               = br.readShort(2);
      inf.BCALength             = br.readShort(1);
      inf.unused2               = br.readInt(2);
      inf.BCAOffset             = br.readInt(1);
      inf.unused3               = br.readInt(6);
      inf.DiskStructuresOffset  = br.readInt(1);
      inf.unused4               = br.readInt(3);
      inf.SessionsBlocksOffset  = br.readInt(1);
      inf.DpmBlocksOffset       = br.readInt(1);
      inf.EncryptionKeyOffset   = br.readInt(1);
      assert.equal(MDSHeaderSize, br.get_offset(), 'bad header size');
      assert.equal(Signature, inf.Signature, 'bad Signature');
      log('\nheader', inf);
    }

   
    function read_msf() {
      var msf = {};
      msf.M = br.readByte(1);
      msf.S = br.readByte(1);
      msf.F = br.readByte(1);
      msf.ToLBA = function() {
        return ((this.M * 60 + this.S) * 75 + this.F) - 150;
      };
      msf.LBA = msf.ToLBA();
      return msf;
    }
  } catch(e) {
    cb(e);
  }
}


function bin_reader(buf) {
  var offset = 0;

  return {
    readString : readString,
    readByte   : readByte,
    readShort  : readShort,
    readInt    : readInt,
    readSint   : readSint,
    readLong   : readLong,
    readCString: readCString,

    toString   : toString,
    get_offset : get_offset,
    set_offset : set_offset,
  };

  function toString() {
    return 'offset=' + offset;
  }

  function set_offset(off) {
    offset = off;
  }

  function get_offset() {
    return offset;
  }

  function readLong(len) {
    return _read_(len*2, 'readUInt32LE', 4);
  }

  function readString(end) {
    var str = buf.slice(offset, end).toString();
    offset = end;
    return str;
  }

  function readCString(beginat) {
    var end;
    for (end = beginat; end < buf.length; ++end) {
      if (buf[end] == 0) break;
    }
    offset = beginat;
    return readString(end);
  }

  function readSint(len) {
    return _read_(len, 'readInt32LE', 4);
  }

  function readInt(len) {
    return _read_(len, 'readUInt32LE', 4);
  }

  function readShort(len) {
    return _read_(len, 'readUInt16LE', 2);
  }

  function readByte(len) {
    return _read_(len, 'readUInt8', 1);
  }

  function _read_(len, fn, elem_len) {
    var ret;
    if (len == 1) {
      ret = buf[fn](offset);
    } else {
      ret = [];
      for (var i = 0; i < len; ++i) {
        ret[i] = buf[fn](offset + i*elem_len);
      }
    }
    offset += len * elem_len;
    return ret;
  }
}

function log(num) {
  console.log.apply(console, arguments);
}