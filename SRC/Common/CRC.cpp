// Common/CRC.cpp

#include "StdAfx.h"

#include <stddef.h>

#include "CRC.h"

static const UInt32 kCRCPoly = 0xEDB88320;

UInt32 CCRC::Table[256];

void CCRC::InitTable()
{
  for (UInt32 i = 0; i < 256; i++)
  {
    UInt32 r = i;
    for (int j = 0; j < 8; j++)
      if (r & 1) 
        r = (r >> 1) ^ kCRCPoly;
      else     
        r >>= 1;
    CCRC::Table[i] = r;
  }
}

class CCRCTableInit
{
public:
  CCRCTableInit() { CCRC::InitTable(); }
} g_CRCTableInit;

// WARNING!!!!!!: this code is only for LITTLE_ENDIAN

void CCRC::Update(const void *data, UInt32 size)
{
  UInt32 v = _value;
  const Byte *p = (const Byte *)data;
  
  for(; (size_t(p) & 3) != 0 && size > 0; size--, p++)
    v = Table[(((Byte)(v)) ^ (*p))] ^ (v >> 8);
  
  const UInt32 kBlockSize = 4;
  while (size >= kBlockSize)
  {
    size -= kBlockSize;
    v ^= *(const UInt32 *)p;
    v = Table[(Byte)v] ^ (v >> 8);
    v = Table[(Byte)v] ^ (v >> 8);
    v = Table[(Byte)v] ^ (v >> 8);
    v = Table[(Byte)v] ^ (v >> 8);
    p += kBlockSize;
  }
  for(UInt32 i = 0; i < size; i++)
    v = Table[(((Byte)(v)) ^ (p)[i])] ^ 
        (v >> 8);
  _value = v;
}
