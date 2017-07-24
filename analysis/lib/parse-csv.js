const fs = require('fs');
const readline = require('readline');
const csv = require('csv');
const crc = require('./crc.js');

const PKG_LEN = 13; // 包字节长度
const I_TIME = 0;   // csv 项目列索引
const I_TYPE = 1;
const I_DATA = 2;
const DESC_DEF = ';';
const sp = ' ';

const TOC_TNO = 2,
      TOC_CTRLADR = 1,
      TOC_POINT = 3,
      TOC_MIN = 4,
      TOC_SEC = 5,
      TOC_FRAME = 6,
      TOC_ZERO = 7,
      TOC_PMIN = 8,
      TOC_PSEC = 9,
      TOC_PFRAME = 10;

const cdrom_type = {}; // saturn 总是 00
cdrom_type[ 0x00 ] = "CD-ROM";
cdrom_type[ 0x10 ] = "CD-I";
cdrom_type[ 0x20 ] = "CD-ROM-XA";

//
// http://wiki.osdev.org/ISO_9660
// https://zh.wikipedia.org/wiki/CD-ROM
// https://en.wikipedia.org/wiki/Compact_Disc_Digital_Audio
//
const protocol_define3 = [
  // cdrom -> cpu
  [[0x00], 'nop'],
  [[0x02], '正在读取, 等待.'],
  [[0x03], 'xx信息?'],
  [[0x06], 'TOC 信息'],
  [[0x12], 'CD 已经就绪(上一个指令完成)'],
  [[0x22], '正在重定位到轨道'],
  [[0x32], ';'],
  [[0x34], '音频帧 ReadingAudioData'],
  [[0x36], '数据帧 ReadingDataSectors'],
  [[0x42], '搜寻轨道/正在读取?'],
  [[0x46], 'Idle'], // 不停的发送轨道信息
  [[0x80], 'CD 仓盖打开'],
  [[0x83], '没有光盘 NoDisc'],
  [[0xB2], 'SeekSecurityRing1'],
  [[0xB6], 'SeekSecurityRing2'],
];
  
const protocol_define4 = [
  // cpu -> cdrom
  [[0x00], 'nop'],
  [[0x02], 'Seeking Ring'],
  [[0x03], '读取内容表 Read TOC'],
  [[0x04], '停止操作 Stop Disc'],
  [[0x06], '读取数据 Read'],
  [[0x08], '操作终止 Pause'],
  [[0x09], '寻道 Seek'],
  [[0x26], ';'],
];


function hex3(buf) {
  var out = [];
  buffer_hex(buf, out);
  var desc = protocol_map3[ gen_key(buf) ] || DESC_DEF;
  out.push(sp, desc);
  
  if (desc[0] == ';') {
    unknow3[ gen_key(buf) ] = buf;
  }

  // var sector = buf.readUInt16BE(1).toString(16);
  switch (buf[0]) {
    case 0x03: // read toc
      // 模拟器应答 0x04, 但是实机是 0x06
      break;

    case 0x80:
      au_ch = 1;
      ch_desc = 1;
      break;

    case 0x06: // TOC
      out.push(sp, ch_desc++);
      var toc = parse_toc_item(buf);
      out.push(toc.desc);
      break;

    case 0x46:
      // 数据部分可能是 crc
      out.push(sp, au_ch++);
    case 0x22:
    case 0x36:
    case 0x34:
      var track_fad = msf_bcd2fad(buf[4], buf[5], buf[6]);
      var abs_fad = msf_bcd2fad(buf[8], buf[9], buf[10]);
      out.push(', Track fad:', track_fad, h16(track_fad));
      out.push(', Abs fad:', abs_fad, h16(abs_fad));
      break;
  }

  //check_mask(mask3, buf, out);

  return out.join('');
}


function hex4(buf) {
  var out = [''];
  buffer_hex(buf, out);
  var desc = protocol_map4[ gen_key(buf) ] || DESC_DEF;
  out.push(sp, desc);
  
  if (desc == ';') {
    unknow4[ gen_key(buf) ] = buf;
  }

  // var sector = buf.readUInt16BE(1).toString(16);
  switch (buf[0]) {
    case 0x06:
    case 0x09:
      var fad = get_fad_from_command(buf);
      out.push(', FAD ', fad, h16(fad));
      break;
  }

  //check_mask(mask4, buf, out);

  return out.join('');
}


function h16(x) {
  return '(0x' + x.toString(16) + ')';
}


var filename  = open_in_filename();
var data      = fs.readFileSync(filename);
var offset    = 0;
var p3buf     = Buffer.alloc(PKG_LEN);
var p4buf     = Buffer.alloc(PKG_LEN);
var all       = [];
var current   = {};
var au_ch     = 1;
var ch_desc   = 1;
var unknow4   = {};
var unknow3   = {};
var mask3     = [];
var mask4     = [];
var protocol_map3 = {};
var protocol_map4 = {};
  
for (var i=0; i<13; ++i) {
  mask3[i] = 0;
  mask4[i] = 0;
}
  
  
protocol_define_to_map(protocol_define3, protocol_map3);
protocol_define_to_map(protocol_define4, protocol_map4);


csv.parse(data, function(err, data) {
  data.forEach(function(data) {
    if (!data)
      return;

    var time = data[I_TIME];
    var type = data[I_TYPE];
    var p34  = data[I_DATA];
    if (type != 'SPI')
      return;

    var tmp = p34.split(';');
    var p3 = parse_ms(tmp[0]);
    var p4 = parse_ms(tmp[1]);
    if (p3>=0 && p4>=0) {
      // console.log(p3, p4);
      push_byte({
        time : time,
        p3 : p3,
        p4 : p4,
      });
    }
  });
  report();
});


function report() {
  all.forEach(function(r, i) {
    var o = i + ', ' + r.begin + ' - ' + r.end + '\tr: ' + r.repeat
          + '\n\tp3: ' + hex3(r.p3) + '\n\tp4: ' + hex4(r.p4);
    console.log(o, '\n');
  });
  console.log("\n未知的p3指令", unknow3);
  console.log("\n未知的p4指令", unknow4);

  console.log("\nP3 掩码, 1:在该bit上出现过1, 0:从未出现过1.");
  print_mask(mask3);

  console.log("\nP4 掩码.");
  print_mask(mask4);

  function print_mask(m) {
    for (var i=0; i<m.length; ++i) {
      console.log('\t', i.toString(16), '\t', m[i].toString(16), m[i].toString(2));
    }
  }
}


function push_byte(d) {
  p3buf[offset] = d.p3;
  p4buf[offset] = d.p4;
  if (offset == 0) {
    current.begin = d.time;
  }
  if (++offset == PKG_LEN) {
    current.end = d.time;
    offset = 0;

    if (current.p3==null ||
        p3buf.equals(current.p3)==false ||
        p4buf.equals(current.p4)==false) {
      current.p3 = Buffer.from(p3buf);
      current.p4 = Buffer.from(p4buf);
      all.push({
        begin  : current.begin,
        end    : current.end,
        p3     : current.p3,
        p4     : current.p4,
        repeat : 0,
      });
    } else {
      all[all.length-1].repeat++;
    }
  }
}


function gen_key(p) {
  // p[0] 应该是命令字节, 后面两个字节可能是寄存器数据?
  return '' + p[0]; // + p[1] + p[2];
}


function buffer_hex(buf, out) {
  for (var i=0; i<buf.length; ++i) {
    if (buf[i] < 0x0F) {
      out.push('0', buf[i].toString(16), sp);
    } else {
      out.push(buf[i].toString(16), sp);
    }
    if (i == 0 || i == 3 || i == 6 || i==7 || i==10) out.push(sp, sp);
    //if (i == 6) out.push('- ');
  }

  var c = crc(buf);
	if (c != buf[11]) {
		out.push('! Bad CRC ', c, '!=', buf[12]);
	}
}


function protocol_define_to_map(d, m) {
  d.forEach(function(p) {
    var key = gen_key(p[0]);
    // console.log(p[0][0].toString(16), '=>', revese_byte(key).toString(16));
    var name = p[1];
    m[key] = name;
  });
}


function check_mask(mask, buf, out) {
  for (var i=0; i < mask.length; ++i) {
    var a = mask[i];
    mask[i] |= buf[i];
    if (a != mask[i]) out.push(', mask change ', i, ': ', 
      a.toString(16), ' != ', mask[i].toString(16));
  }
}


function parse_ms(x) {
  x = x && parseInt(x.split(':')[1], 16);
  return revese_byte(x);
}


function revese_byte(a) {
    if (!a) return a;

    var b = 0;
    var bit;
    for (var i = 0; i < 8; ++i) {
        bit = (a >> i) & 0x01;
        if (bit) b |= (1 << (7 - i));
    }
    // console.log(a.toString(16), b.toString(16))
    return b;
}


function get_fad_from_command(buf) {
    var fad = buf[1];
    fad <<= 8;
    fad |= buf[2];
    fad <<= 8;
    fad |= buf[3];
    return fad;
}


function msf_bcd2fad(min, sec, frame) {
    var fad = 0;
    fad += bcd2num(min);
    fad *= 60;
    fad += bcd2num(sec);
    fad *= 75;
    fad += bcd2num(frame);
    return fad;
}


function fad2msf(fad, msf) {
   msf[0] = fad / (75 * 60);
   fad -= msf[0] * (75 * 60);
   msf[1] = fad / 75;
   fad -= msf[1] * 75;
   msf[2] = fad;
}


function bcd2num(bcd) {
   return (bcd >> 4) * 10 + (bcd & 0xf);
}


function num2bcd(num) {
   return ((num / 10) << 4) | (num % 10);
}


function fad2msf_bcd() {
  fad2msf(fad, msf);
  msf[0] = num2bcd(msf[0]);
  msf[1] = num2bcd(msf[1]);
  msf[2] = num2bcd(msf[2]);
}


function make_ring_status(buf, disc_fad) {
  var fad = disc_fad + 4;
  var msf_buf = [0,0,0];
  fad2msf_bcd(disc_fad, msf_buf);

  buf[0] = SeekSecurityRing2;
  buf[1] = 0x44;
  buf[2] = 0xf1;
  buf[3] = fad >> 16;
  buf[4] = fad >> 8;
  buf[5] = fad;
  buf[6] = 0x09;
  buf[7] = 0x09;
  buf[8] = 0x09;
  buf[9] = 0x09;
  buf[10] = msf_buf[2];
  buf[11] = crc(buf);
}


function parse_toc_item(buf) {
  var toc = {
    tno   : buf[TOC_TNO],
    adr   : buf[TOC_CTRLADR] & 0x0F,
    ctrl  : buf[TOC_CTRLADR] >> 4,
    point : buf[TOC_POINT],
    msf   : msf_bcd2fad(buf[TOC_MIN], buf[TOC_SEC], buf[TOC_FRAME]),
    pmsf  : msf_bcd2fad(buf[TOC_PMIN], buf[TOC_PSEC], buf[TOC_PFRAME]),
  };
  var out = [','];

  // mmc3r10g.pdf 207p
  switch(toc.adr) {
  case 0x00:
    out.push('Q子通道模式信息未提供');
    break;
  case 0x01:
    out.push('Q子通道编码为当前位置数据');
    break;
  case 0x02:
    out.push('Q子通道编码为媒体目录号');
    break;
  case 0x03:
    out.push('Q子信道编码 ISRC');
    break;
  default:
    out.push('未知的Q通道编码');
  }

  // mmc3r10g.pdf 208p
  if ((toc.ctrl & 0xD) == 0x0) { // 00x0b 
    out.push(', 2 声道音频无加重');
  } else if ((toc.ctrl & 0xD) == 0x1) { // 00x1b
    out.push(', 2 声道音频, 加重50/15 µs');
  } else if ((toc.ctrl & 0xD) == 0x8) { // 10x0b
    out.push(', 音频通道, 无加重 (保留在 cd-r/rw)')
  } else if ((toc.ctrl & 0xD) == 0x9) { // 10x1b
    out.push(', 音频通道, 加重50/15 µs (保留在 cd-r/rw)');
  } else if ((toc.ctrl & 0xD) == 0x4) { // 01x0b
    out.push(', 数据通道, 记录不间断');
  } else if ((toc.ctrl & 0xD) == 0x5) { // 01x1b
    out.push(', 数据通道, 记录增量');
  } else if ((toc.ctrl & 0xC) == 0xC) { // 11xxb
    out.push(', 保留');
  } else {
    out.push(', ctrl 位不明' + toc.ctrl);
  }
  if ((toc.ctrl & 0x2) == 0x0) { // xx0xb
    out.push(', 数字拷贝禁止');
  } else if ((toc.ctrl & 0x2) == 0x2) { // xx1xb
    out.push(', 数字拷贝允许');
  }

  // mmc3r10g.pdf 27p
  if (toc.adr == 0x01) {
    switch(toc.point) {
    case 0xA0:
      out.push(', 第一个程序轨道:', bcd2num(buf[TOC_PMIN]) );
      out.push(', 光盘类型:', cdrom_type[buf[TOC_PSEC]]);
      break;
    case 0xA1:
      out.push(', 最后的程序轨道:' + bcd2num(buf[TOC_PMIN]) );
      break;
    case 0xA2:
      out.push(', Lead-out 在:', toc.pmsf);
      break;
    default:
      out.push(', 轨道', bcd2num(buf[TOC_POINT]), '开始于', toc.pmsf);
    }
  } else {
    out.push(', adr 位不明' + toc.adr);
  }


  return {
    toc  : toc,
    desc : out.join(' '),
  };
}


function open_in_filename() {
  var arg = process.argv[2];
  var def = __dirname + '/../data/untitled.csv'
  if (!arg) 
    return def;
  
  var p = __dirname + '/../data/' + arg;
  if (fs.existsSync(p)) {
    return p;
  }
  p = process.cwd + '/' + arg;
  if (fs.existsSync(p)) {
    return p;
  }
  throw new Error('cannot open file ' + arg);
}