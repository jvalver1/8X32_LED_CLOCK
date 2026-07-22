#include "wokwi-api.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct { uint8_t regs[256], reg; bool expecting_reg; } chip_state_t;

static void set_u16(uint8_t *r, uint8_t a, uint16_t v) { r[a]=v; r[a+1]=v>>8; }
static void reset_registers(chip_state_t *c) {
  memset(c->regs, 0, sizeof(c->regs));
  c->regs[0xd0]=0x58;
  set_u16(c->regs,0x88,27504); set_u16(c->regs,0x8a,26435);
  set_u16(c->regs,0x8c,(uint16_t)-1000); set_u16(c->regs,0x8e,36477);
  set_u16(c->regs,0x90,(uint16_t)-10685); set_u16(c->regs,0x92,3024);
  set_u16(c->regs,0x94,2855); set_u16(c->regs,0x96,140);
  set_u16(c->regs,0x98,(uint16_t)-7); set_u16(c->regs,0x9a,15500);
  set_u16(c->regs,0x9c,(uint16_t)-14600); set_u16(c->regs,0x9e,6000);
  c->regs[0xf3]=0; c->regs[0xf7]=0x65; c->regs[0xf8]=0x5a; c->regs[0xf9]=0xc0;
  c->regs[0xfa]=0x7e; c->regs[0xfb]=0xed; c->regs[0xfc]=0;
}
static bool connect(void *u, uint32_t a, bool read) { if(!read)((chip_state_t*)u)->expecting_reg=true; return true; }
static uint8_t read_byte(void *u) { chip_state_t*c=u; return c->regs[c->reg++]; }
static bool write_byte(void *u,uint8_t d) { chip_state_t*c=u; if(c->expecting_reg){c->reg=d;c->expecting_reg=false;}else{if(c->reg==0xe0&&d==0xb6)reset_registers(c);else c->regs[c->reg]=d;c->reg++;}return true; }
void chip_init(void) {
  chip_state_t*c=calloc(1,sizeof(chip_state_t)); reset_registers(c); c->expecting_reg=true;
  i2c_config_t cfg={.address=0x76,.sda=pin_init("SDA",INPUT_PULLUP),.scl=pin_init("SCL",INPUT_PULLUP),.connect=connect,.read=read_byte,.write=write_byte,.user_data=c};
  i2c_init(&cfg);
}
