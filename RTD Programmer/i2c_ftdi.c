#include "i2c_ftdi.h"
#include "helper.h"
#include <stdbool.h>


int i2c_grow_buffer(struct i2c_ctx *ctx)
{
	//grow the buffer by double
	ssize_t newsize = ctx->buffer_size * 2;

	uint8_t *temp = realloc(ctx->cmd_buffer, newsize);
	if(!temp)
	{
		LOG_ERROR("Failed to grow commands buffer.");
		return 0;
	}

	ctx->cmd_buffer=temp;
	ctx->buffer_size=newsize;
	return 1;
}

void i2c_append_cmd(struct i2c_ctx *ctx, uint8_t *src, ssize_t len)
{
	if(ctx->buffer_size < (ctx->buffer_pos + len))
	{
		//can't grow buffer, bail out
		if(!i2c_grow_buffer(ctx))
			return;
	}

	memcpy(&(ctx->cmd_buffer[ctx->buffer_pos]), src, len);
	ctx->buffer_pos += len;
}


void i2c_set_io_cmd(struct i2c_ctx *ctx, uint8_t val, uint8_t dir)
{
	uint8_t buf[3];
	buf[0] = 0x80; buf[1] = val; buf[2] = dir;
	i2c_append_cmd(ctx, buf, sizeof(buf));
}

void i2c_start_cmd(struct i2c_ctx *ctx)
{
	//tell the controller start of i2c transaction
	i2c_set_io_cmd(ctx, SCK|SDA, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SCK|SDA, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SCK|SDA, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SCK, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SCK, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, 0, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, 0, I2C_DIR_OUT);
}

void i2c_write_cmd(struct i2c_ctx *ctx, uint8_t val)
{

	/*
	uint8_t bytecmd[] = {0x11, 0x00, 0x00, 0xff}; //clock in 1 byte 
	uint8_t ackcmd[] = {0x13, 0x00, 0xff}; //clock in 1'b1

	bytecmd[3] = val;
	i2c_append_cmd(ctx, bytecmd, sizeof(bytecmd));
	i2c_append_cmd(ctx, ackcmd, sizeof(ackcmd));

	i2c_set_io_cmd(ctx, SDA, I2C_DIR_OUT);
	*/

	//this method is more stable
	//but this one is slower than the other one.
	//Need more testing to see what was wrong

	int bit, i;

	for(i =7; i >= 0; i--)
	{
		bit = ((val >> i) & 1) ? SDA : 0;
		i2c_set_io_cmd(ctx, bit, I2C_DIR_OUT);
		i2c_set_io_cmd(ctx, bit|SCK, I2C_DIR_OUT);
		i2c_set_io_cmd(ctx, bit|SCK, I2C_DIR_OUT);
		i2c_set_io_cmd(ctx, bit, I2C_DIR_OUT);
		i2c_set_io_cmd(ctx, bit, I2C_DIR_OUT);
	}

	i2c_set_io_cmd(ctx, SDA, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SDA|SCK, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SDA|SCK, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SDA, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SDA, I2C_DIR_OUT);
}

void i2c_read_cmd(struct i2c_ctx *ctx)
{
	uint8_t bytecmd[] = {0x20, 0x00, 0x00}; //clock out 1 byte 
	uint8_t ackcmd[] = {0x13, 0x00, 0xff}; //clock in 1'b1

	i2c_append_cmd(ctx, bytecmd, sizeof(bytecmd));
	i2c_append_cmd(ctx, ackcmd, sizeof(ackcmd));

	i2c_set_io_cmd(ctx, SDA, I2C_DIR_OUT);
}

void i2c_end_cmd(struct i2c_ctx *ctx)
{
	//begin 
	i2c_set_io_cmd(ctx, 0, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SCK, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SCK, I2C_DIR_OUT);
	i2c_set_io_cmd(ctx, SCK|SDA, I2C_DIR_OUT);
}

void i2c_flush_buffer(struct i2c_ctx *ctx)
{

	if (ctx->buffer_pos > 0)
	{
		mpsse_write(ctx->mpsse_ctx, ctx->cmd_buffer, ctx->buffer_pos);

		ctx->buffer_pos = 0;
	}
}

void i2c_send_byte(struct i2c_ctx *ctx, uint8_t addr, uint8_t val)
{
	i2c_send(ctx, addr, 1, &val);
}


void i2c_send(struct i2c_ctx *ctx, uint8_t addr, int count, uint8_t *dat)
{
	int i;

	//start of i2c transaction
	i2c_start_cmd(ctx);

	//setup address for I2C write 
	i2c_write_cmd(ctx, addr);

	//write data in
	for(i=0; i<count; i++)
		i2c_write_cmd(ctx, dat[i]);

	i2c_end_cmd(ctx);

	//hexdump(ctx->cmd_buffer, ctx->buffer_pos);
	i2c_flush_buffer(ctx);
}

uint8_t i2c_recv_byte(struct i2c_ctx *ctx, uint8_t addr)
{
	uint8_t rev;
	i2c_recv(ctx, addr, 1, &rev);
	return rev;
}

#if 0
void i2c_recv(struct i2c_ctx *ctx, uint8_t addr, int len, uint8_t *buf)
{
	int i;
	uint8_t revcbuff[3]; //2 junk byte + 1 data byte 

	//start of i2c transaction
	i2c_start_cmd(ctx);

	//setup address for I2C read 
	i2c_write_cmd(ctx, addr);;

	//read data
	for(i=0; i<len; ++i)
	{
		i2c_read_cmd(ctx);

		//write send immediate command
		uint8_t cmd[]={0x87};
		i2c_append_cmd(ctx, cmd, sizeof(cmd));

		i2c_flush_buffer(ctx);

		mpsse_read(ctx->mpsse_ctx, revcbuff, sizeof(revcbuff));

		buf[i] = revcbuff[2];
	}

	i2c_end_cmd(ctx);

	i2c_flush_buffer(ctx);
}
#else
void i2c_recv(struct i2c_ctx *ctx, uint8_t addr, int len, uint8_t *buf)
{
	int i;
	uint8_t *revcbuff = malloc(len + 2); //plus 2 junk byte 

	//start of i2c transaction
	i2c_start_cmd(ctx);

	//setup address for I2C read 
	i2c_write_cmd(ctx, addr);;

	//read data
	for(i=0; i<len; ++i)
		i2c_read_cmd(ctx);

	i2c_end_cmd(ctx);

	//write send immediate command
	uint8_t cmd[]={0x87};
	i2c_append_cmd(ctx, cmd, sizeof(cmd));

	i2c_flush_buffer(ctx);

	mpsse_read(ctx->mpsse_ctx, revcbuff, len+2);

	memcpy(buf, revcbuff+2, len);
	free(revcbuff);
}
#endif

void i2c_set_clock(struct i2c_ctx *ctx)
{
	//set speed to 0x000f 353 KHz
	uint8_t mpsse_cmd[] = {0x86, 0x0f, 0x00};

	mpsse_write(ctx->mpsse_ctx, mpsse_cmd, sizeof(mpsse_cmd));
}

struct i2c_ctx *i2c_initialize(struct mpsse_ctx *mpsse)
{
	struct i2c_ctx *ctx = calloc(1, sizeof(*ctx));

	LOG_DEBUG("Initializing I2C interface...");
	ctx->mpsse_ctx = mpsse;

	i2c_set_clock(ctx);

	ctx->cmd_buffer = malloc(CMD_BUFFER_INITIAL_SIZE);
	if(!ctx->cmd_buffer)
	{
		LOG_ERROR("Failed to allocate commands buffer.");
		return 0;
	}

	ctx->buffer_pos=0;
	ctx->buffer_size=CMD_BUFFER_INITIAL_SIZE;

	return ctx;
}

void i2c_close(struct i2c_ctx *ctx)
{
	if (ctx->cmd_buffer)
		free(ctx->cmd_buffer);

	free(ctx);
}
