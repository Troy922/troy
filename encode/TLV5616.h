#ifndef _TLV5616_H_
#define _TLV5616_H_
#include <xdc/std.h>
#include<unistd.h>
/*analog signal output and input control*/
#define BL_CS           0x00
#define Tt_CS           0x01
#define MI_CS           0x02
#define TLV_SCK         79
#define TLV_DI          81
#define TLV_FS          80
#define TLV_CS0         82
#define TLV_CS1         83
#define TLV_CS2         84
#define SW_OUT1         32
#define SW_OUT2         33
#define SW_IN           35
void DA_init(void);
void DA_write(unsigned int cs,float val);

#endif
