#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct { uint8_t reg,status; bool expecting_reg; uint16_t year; uint8_t month,day,hour,minute,second,weekday; } chip_state_t;
static uint8_t bcd(uint8_t v){return v+6*(v/10);} static uint8_t unbcd(uint8_t v){return v-6*(v>>4);}
static bool leap(uint16_t y){return(y%4==0&&y%100!=0)||y%400==0;}
static uint8_t mdays(uint16_t y,uint8_t m){static const uint8_t d[]={31,28,31,30,31,30,31,31,30,31,30,31};return m==2&&leap(y)?29:d[m-1];}
static void tick(void*u){chip_state_t*c=u;if(++c->second<60)return;c->second=0;if(++c->minute<60)return;c->minute=0;if(++c->hour<24)return;c->hour=0;c->weekday=c->weekday%7+1;if(++c->day<=mdays(c->year,c->month))return;c->day=1;if(++c->month<=12)return;c->month=1;c->year++;}
static uint8_t getreg(chip_state_t*c,uint8_t r){switch(r){case 0:return bcd(c->second);case 1:return bcd(c->minute);case 2:return bcd(c->hour);case 3:return c->weekday;case 4:return bcd(c->day);case 5:return bcd(c->month)|(c->year>=2100?0x80:0);case 6:return bcd(c->year%100);case 0x0e:return 0x1c;case 0x0f:return c->status;case 0x11:return 25;default:return 0;}}
static void setreg(chip_state_t*c,uint8_t r,uint8_t v){switch(r){case 0:c->second=unbcd(v&0x7f);break;case 1:c->minute=unbcd(v&0x7f);break;case 2:c->hour=unbcd(v&0x3f);break;case 3:c->weekday=v?v:1;break;case 4:c->day=unbcd(v&0x3f);break;case 5:c->month=unbcd(v&0x1f);break;case 6:c->year=2000+unbcd(v);break;case 0x0f:c->status=v;break;}}
static bool connect(void*u,uint32_t a,bool read){if(!read)((chip_state_t*)u)->expecting_reg=true;return true;}
static uint8_t read_byte(void*u){chip_state_t*c=u;return getreg(c,c->reg++);}
static bool write_byte(void*u,uint8_t d){chip_state_t*c=u;if(c->expecting_reg){c->reg=d;c->expecting_reg=false;}else setreg(c,c->reg++,d);return true;}
void chip_init(void){chip_state_t*c=calloc(1,sizeof(chip_state_t));c->year=2026;c->month=7;c->day=22;c->hour=12;c->weekday=4;c->status=0x80;
  i2c_config_t cfg={.address=0x68,.sda=pin_init("SDA",INPUT_PULLUP),.scl=pin_init("SCL",INPUT_PULLUP),.connect=connect,.read=read_byte,.write=write_byte,.user_data=c};i2c_init(&cfg);
  timer_config_t tc={.callback=tick,.user_data=c};timer_t t=timer_init(&tc);timer_start(t,1000000,true);
}
