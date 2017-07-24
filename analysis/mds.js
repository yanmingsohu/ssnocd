const mds = require("./lib/parse-mds.js");
const util = require('util');

const infilen = __dirname + '/../大航海时代2[简][V1.01]/大航海时代2.mds';

console.log('Open MDS', infilen);

mds.openMds(infilen, function(err, mds) {
  if (err) return console.log(err);

  console.log('\nCreate CD from Image.');
  var cdimg = mds.open_cd_img();
  test_read(cdimg);
});


function test_read(cdimg) {
  var FirstSector = parseInt(process.argv[2]) || 0;
  var SectorCount = 1;
  var buf = Buffer.alloc(SectorCount * cdimg.CD_SECTOR_SIZE);
  var ret = cdimg.ReadSectorsBoundsChecked(FirstSector, buf, SectorCount);
  console.log('\nREAD Sector', FirstSector, '=', ret);
  hex(buf);
  
return;
  var toc = cdimg.getToc();
  console.log('\nToc', toc);
  console.log('TrackData:', toc.DataBuffer.TrackData);
}


function hex(buf) {
  var out = [], oi=0;
  for (var i=0; i<buf.length; ++i, ++oi) {
    if (oi == 16) {
      var cc = [];
      for (var c=i-16; c<i; ++c) {
        if (buf[c]>0x1F && buf[c] < 0x80) {
          cc.push( String.fromCharCode(buf[c]) );
        } else {
          cc.push('.');
        }
      }
      console.log((i-16).toString(16), '\t', out.join(' '), '  ', cc.join(''));
      out.length = oi = 0;
    }

    out[oi] = (buf[i] < 0x10 ? '0' : '') + buf[i].toString(16);

    if (oi == 8) {
      out[oi] = '  ' + out[oi];
    }
  }
}