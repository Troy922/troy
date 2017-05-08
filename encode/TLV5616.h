#ifndef _TLV5616_H_
#define _TLV5616_H_
#include <xdc/std.h>
#include<unistd.h>
/*analog signal output and input control*/
#define BL_CS           0x00
#define Tt_CS           0x01
#define MI_CS           0x02
#define TLV_SCK         28
#define TLV_DI          46
#define TLV_FS          26
#define TLV_CS0         29
#define TLV_CS1         27
#define TLV_CS2         22 
#define SW_OUT1         31
#define SW_OUT2         30
#define SW_IN           35
void DA_init(void);
void DA_write(unsigned int cs,float val);
void DA_write_test(float val);

#endif
