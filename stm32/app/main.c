#include "lib.h"
#include "mds.h"
#include "fs.h"
#include "cdstate.h"
#include "cd-img.h"

int main(void)
{
	assert(1 == sizeof(UCHAR));
	assert(2 == sizeof(USHORT));
	assert(4 == sizeof(unsigned));
	assert(8 == sizeof(ULONGLONG));
	assert(sizeof(MDSTrackBlock) == 80);
	assert(sizeof(MDSSessionBlock) == 24);
	assert(sizeof(MDSHeader) == 92);

	SystemInit();
	mds_open("/file");
	cdd_reset();

    while(1)
    {
    }
}
//! @}
//! @}
