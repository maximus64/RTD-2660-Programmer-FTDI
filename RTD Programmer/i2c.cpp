#include "stdafx.h"
#include <stdint.h>

#if 0
#include <Windows.h>
#include "CyAPI.h"
#include "cyioctl.h"

static CCyUSBDevice USBDevice;
static CCyControlEndPoint *ept = USBDevice.ControlEndPt;
#else

extern "C"
{
  #include "i2c_ftdi.h"
}

#define DONGLE_VID 0x0403 //0x0403
#define DONGLE_PID 0x6010 //0x6011
#define DONGLE_CH  0

static struct mpsse_ctx *mpsse_ctx;
static struct i2c_ctx *i2c_ctx;

static uint8_t I2C_Addr_R = 0x00;
static uint8_t I2C_Addr_W = 0x00;

#endif

#if 0
static BYTE ReadNakCnt() 
{
  ept->Target = TGT_DEVICE;
  ept->ReqType = REQ_VENDOR;
  ept->Direction = DIR_FROM_DEVICE;
  ept->ReqCode = 0xA3;
  ept->Value = 0;
  ept->Index = 0;

  uint8_t buf[1];
  LONG buflen = sizeof(buf);
  ept->XferData(buf, buflen);
  return buf[0];
}
#endif

void SetI2CAddr(uint8_t value) 
{
#if 0
  ept->Target = TGT_DEVICE;
  ept->ReqType = REQ_VENDOR;
  ept->Direction = DIR_TO_DEVICE;
  ept->ReqCode = 0xA5;
  ept->Value = 0;
  ept->Index = 0;

  uint8_t buf[1];
  LONG buflen =  1;
  buf[0] = value;
  ept->XferData(buf, buflen);
#else
  printf("Setting I2C address to: %02x\n", value);
  I2C_Addr_W = value << 1;
  I2C_Addr_R = (value << 1) | 0x01;
  printf("Read I2C address: %02x\n", I2C_Addr_R);
  printf("Write I2C address: %02x\n", I2C_Addr_W);
#endif
}

bool WriteBytesToAddr(uint8_t reg, uint8_t* values, uint8_t len)
{
#if 0
  ept->Target    = TGT_DEVICE;
  ept->ReqType   = REQ_VENDOR;
  ept->Direction = DIR_TO_DEVICE;
  ept->ReqCode   = 0xA2;
  ept->Value     = 0;
  ept->Index     = 0;

  uint8_t buf[64];
  if (len > 63)
  {
    len = 63;
  }
  LONG buflen =  len + 1;
  buf[0] = reg;
  for(int idx = 0; idx < len; idx++)
  {
    buf[1 + idx] = values[idx];
  }

  ept->XferData(buf, buflen);
  return ReadNakCnt() == 0;
#else
  //printf("Writing address: %02x len: %d byte0: %02x\n", reg, len, values[0]);
  uint8_t buf[64];
  if (len > 63)
  {
    len = 63;
  }

  buf[0] = reg;

  for(int idx = 0; idx < len; idx++)
  {
    buf[1 + idx] = values[idx];
  }

  i2c_send(i2c_ctx, I2C_Addr_W, len + 1, buf);
  return true;
#endif
}

#if 0
static void ReadBytes(uint8_t *dest, uint8_t len) 
{
  ept->Target = TGT_DEVICE;
  ept->ReqType = REQ_VENDOR;
  ept->Direction = DIR_FROM_DEVICE;
  ept->ReqCode = 0xA4;
  ept->Value = 0;
  ept->Index = 0;

  uint8_t buf[64];
  LONG buflen = sizeof(buf);
  ept->XferData(buf, buflen);
  if (len > sizeof(buf))
  {
    len = (uint8_t)sizeof(buf);
  }
  memcpy(dest, buf, len);
}
#endif

bool ReadBytesFromAddr(uint8_t reg, uint8_t* dest, uint8_t len)
{
#if 0
  ept->Target    = TGT_DEVICE;
  ept->ReqType   = REQ_VENDOR;
  ept->Direction = DIR_TO_DEVICE;
  ept->ReqCode   = 0xA1;
  ept->Value     = 0;
  ept->Index     = 0;

  uint8_t buf[2];
  LONG buflen = sizeof(buf);
  buf[0] = reg;
  buf[1] = len;
  ept->XferData(buf, buflen);
  ReadBytes(dest, len);
  return ReadNakCnt() == 0;
#else
  i2c_send_byte(i2c_ctx, I2C_Addr_W, reg);
  i2c_recv(i2c_ctx, I2C_Addr_R, len, dest);
  //printf("Reading address: %02x len: %d byte0: %02x\n", reg, len, dest[0]);

  return true;
#endif
}

uint8_t ReadReg(uint8_t reg)
{
  uint8_t result;
  ReadBytesFromAddr(reg, &result, 1);
  return result;
}

bool WriteReg(uint8_t reg, uint8_t value)
{
  return WriteBytesToAddr(reg, &value, 1);
}

bool InitI2C() {
#if 0
  return USBDevice.IsOpen();
#else
  printf("Initializing FTDI MPSSE port...\n");
  mpsse_ctx = mpsse_open(DONGLE_VID, DONGLE_PID, DONGLE_CH);
  if(!mpsse_ctx)
  {
    printf("Failed\n");
    return false;
  }


  printf("Initializing I2C interface...\n");
  i2c_ctx = i2c_initialize(mpsse_ctx);
  if(!i2c_ctx)
  {
    printf("Failed\n");
    mpsse_close(mpsse_ctx);
    return false;
  }

  return true;
#endif
}

void CloseI2C() {
#if 0
  USBDevice.Close();
#else
  printf("Closing interface\n");
  i2c_close(i2c_ctx);
  mpsse_close(mpsse_ctx);
  printf("Closed\n");
#endif
}
