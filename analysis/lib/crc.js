var b1 = [0x06, 0x01, 0x00, 0x08, 0x02, 0x15, 0x61, 0x07, 0x34, 0x01, 0x14, 0x28, 0x00];
var b2 = [0x46, 0x01, 0x01, 0x00, 0x00, 0x00, 0x05, 0x07, 0x00, 0x01, 0x69, 0x41, 0x00];
var b3 = [0x80, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x02, 0x00, 0x73, 0x00];
var b4 = [0x12, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x07, 0x00, 0x02, 0x00, 0xe1, 0x00];
var b5 = [0x06, 0x01, 0x00, 0xa1, 0x02, 0x15, 0x35, 0x07, 0x11, 0x00, 0x00, 0xf3, 0x00];

/**
t(b1);
t(b2);
t(b3);
t(b4);
t(b5);

/**
ff 1111 1111

c0 1100 0000
80 1000 0000  0100 0000
40 0100 0000  0001 0000
9f 1001 1111
*/

function t(b) {
    var t = b.slice(0, 11);
    var r = crc(t);
    console.log('\t', r == b[11] ? 'Y' : 'N', r.toString(16), b[11].toString(16));
    console.log();
}


function crc(buf) {
    var ret = 0;
    for (var i = 0; i < 11; ++i) {
        ret += buf[i];
        // console.log(i, ret.toString(16), buf[i].toString(16));
    }
    return 0xFF & (~ret);
}

module.exports = crc;
