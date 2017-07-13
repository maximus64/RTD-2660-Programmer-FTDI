#ifndef I2C_H
#define I2C_H

#include <string.h>
#include "mpsse.h"

#define SCK (1<<0)
#define SDA (1<<1)

#define CMD_BUFFER_INITIAL_SIZE 4096

#define I2C_DIR_OUT 0xfb //SDA - output, SCK - output

struct i2c_ctx
{
	struct mpsse_ctx *mpsse_ctx;
	uint8_t *cmd_buffer;
	uint32_t buffer_size;
	uint32_t buffer_pos;
};

void i2c_close(struct i2c_ctx *ctx);
struct i2c_ctx *i2c_initialize(struct mpsse_ctx *mpsse);
void i2c_send(struct i2c_ctx *ctx, uint8_t addr, int count, uint8_t *dat);
void i2c_send_byte(struct i2c_ctx *ctx, uint8_t addr, uint8_t val);
uint8_t i2c_recv_byte(struct i2c_ctx *ctx, uint8_t addr);
void i2c_recv(struct i2c_ctx *ctx, uint8_t addr, int len, uint8_t *buf);

#endif
