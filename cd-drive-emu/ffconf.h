#ifndef _FFCONF


#define _FFCONF 6502            //Revision ID
// Functions and Buffer Configurations
#define _FS_TINY        0       //==0:Normal ?==1:Tiny


//f_write, f_sync, f_unlink, f_mkdir, f_chmod, f_rename,f_truncate and useless f_getfree
#define _FS_READONLY    0       //0:Read/Write or 1:Read only

#define _FS_MINIMIZE    0       //0 to 3
// The _FS_MINIMIZE option defines minimization level to remove some functions.
//   0: Full function.
//   1: f_stat, f_getfree, f_unlink, f_mkdir, f_chmod, f_truncate and f_rename
//      are removed.
//   2: f_opendir and f_readdir are removed in addition to 1.
//   3: f_lseek is removed in addition to 2.


#define _USE_STRFUNC    0       //0:Disable or 1-2:Enable

#define _USE_MKFS       0       //0:Disable or 1:Enable
// To enable f_mkfs function, set _USE_MKFS to 1 and set _FS_READONLY to 0

#define _USE_FORWARD    0       //0:Disable or 1:Enable
// To enable f_forward function, set _USE_FORWARD to 1 and set _FS_TINY to 1.

#define _USE_FASTSEEK   0       //0:Disable or 1:Enable
// To enable fast seek feature, set _USE_FASTSEEK to 1.


#define _CODE_PAGE  1
//  Incorrect setting of the code page can cause a file open failure.
//   936  - Simplified Chinese GBK (DBCS, OEM, Windows)
//   950  - Traditional Chinese Big5 (DBCS, OEM, Windows)
//  1    - ASCII only (Valid for non LFN cfg.)

#define _USE_LFN    0           //==0\1\2\3
#define _MAX_LFN    64          //Maximum LFN length to handle (12 to 255)
// The _USE_LFN option switches the LFN support.
//   0: Disable LFN feature. _MAX_LFN and _LFN_UNICODE have no effect.
//   1: Enable LFN with static  working buffer on the BSS. Always NOT reentrant.
//   2: Enable LFN with dynamic working buffer on the STACK.        ?????
//   3: Enable LFN with dynamic working buffer on the HEAP.

#define _LFN_UNICODE    0       //0:ANSI/OEM or 1:Unicode
#define _FS_RPATH       0       //0 to 2
//****************************************************************************************
// Physical Drive Configurations

#define _VOLUMES    1           
#define _MAX_SS     512         


#define _MULTI_PARTITION    0   //0:Single partition, 1/2:Enable multiple partition
//==0 each volume is bound to the same physical drive number and it can mount only first primaly partition.
//==1 When it is set to 1, each volume is tied to the partitions listed in VolToPart[]

#define _USE_ERASE  0           //0:Disable or 1:Enable
// To enable sector erase feature, set _USE_ERASE to 1. CTRL_ERASE_SECTOR command
//  should be added to the disk_ioctl functio.

//****************************************************************************************
// System Configurations
#define _WORD_ACCESS    0       //0 or 1
#define _FS_REENTRANT   0       //0:Disable or 1:Enable
#define _FS_TIMEOUT     1000    
#define _SYNC_t         HANDLE  //O/S dependent type of sync object. e.g. HANDLE, OS_EVENT*, ID and etc..
#define _FS_SHARE   0           //0:Disable or >=1:Enable
// To enable file shareing feature, set _FS_SHARE to 1 or greater. The value
// defines how many files can be opened simultaneously.
//****************************************************************************************
#endif