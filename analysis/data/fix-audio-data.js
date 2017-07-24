const fs = require('fs');
const readline = require('readline');


const infile = __dirname + '/audio-cd.txt';
const outfile = __dirname + '/out.csv';
console.log('Read', infile, '\n Write', outfile);

const output = fs.openSync(outfile, 'w');
const rl = readline.createInterface({
  input: fs.createReadStream(infile),
});


var i = 0;
var begin, end;
var p3;
var p4;

//
// 将 格式化 输出的解析文件重新还原为 csv.
// 用于修复 bit 错误.
//
rl.on('line', function(line) {
  if (i%3 == 0) {
    // 0, 1.128999800000000 - 1.131179800000000	r: 0
    var a = line.substring(line.indexOf(',')+1, line.indexOf('r'));
    var b = a.split('-');
    begin = Number(b[0]);
    end = Number(b[1]);
    //console.log(begin, end);
  } 
  else if (i%3==1) {
    //    p3: 48 00 00   00 00 00 00 - 00 00 00 00   b7 00  CD 已经就绪(上一个指令完成)
    var a = line.substr(5, 46);
    var b = a.split(' ');
    p3 = arr_hex(b);
    //console.log(a, p3);
  } 
  else if (i%3==2) {
    // 	p4: c0 80 00   00 00 00 00 - 00 00 00 40   9f 00  请求检测光盘
    var a = line.substr(5, 46);
    var b = a.split(' ');
    p4 = arr_hex(b);
    fs.appendFileSync(output, out_csv());
  }
  ++i;
});

rl.on('close', function() {
  fs.closeSync(output);
  console.log('over');
});


function out_csv() {
  // 1.337224200000000,SPI,MOSI: 0x48;  MISO: 0x00
  var out = [];
  for (var i=0; i<12; ++i) {
    write(begin, i);
  }
  write(end, 12);
  return out.join('');

  function write(time, i) {
    out.push(time, ',SPI,MOSI: 0x', p3[i].toString(16), 
      '; MISO : 0x', p4[i].toString(16), '\n'); 
  }
}


function arr_hex(a) {
  var ret = [];
  for (var i=0; i<a.length; ++i) {
    var n = parseInt(a[i], 16);
    if (!isNaN(n)) ret.push(n);
  }
  return ret;
}