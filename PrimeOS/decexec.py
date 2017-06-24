#!/usr/bin/env python

import sys
import datetime
import zlib
import struct


def decryptPhase0(data, key, packedsize):
  for i in range(packedsize):
    data[i] ^= key[ i % 512 ]
  return data

def decryptPhase1(data, packedsize):
  seed = 0x391
  for i in range(packedsize):
    data[i] =  (exe[i] - seed) & 0xFF
    seed = ((0x132B5B3  * seed) & 0xFFFFFFFF)  >> 8
  return data

def decryptPhase2(data):
  for i in range(0, len(data), 4096):
    data[i+0x117] ^= 0x32
    data[i+0x613] = (data[i+0x613] + 0x33) & 0xFF
    data[i+0x91F] ^= 0xA3
  return data

def uncorruptPhase3(data):
  curpos = 0
  data = bytearray(data)

  for i in range(0, len(data), 0x1000):
    seed = (0x6B * data[i+97]) & 0xFF
    a = (seed >> 4) + 0x7B
    ub = ( seed >> 1 ) & 7
    sOff = i - ub

    chunkSize = 0x1000
    if i >= len(data) - 0x1000:
      chunkSize = len(data) - i

    while a < chunkSize:
      ub = data[i+a] & 0x80
      data[sOff+a-1] ^= ub
      a += 217 + (seed & 1)

  return data

def decompressPhase(data, page_size):
  print "Binary Page Total %d Bytes" %page_size
  data = data[:page_size]
  outdata = ""
  data = str(data)
  num_indexes = len(data) / 4096
  index_sz = num_indexes * 4
  index_tbl = data[:index_sz]
  curr_offset = index_sz + 4
  for i in range(0,len(index_tbl),4):
    curr_end = struct.unpack("<i", data[i:i+4])[0]
    outdata += zlib.decompress(data[curr_offset:curr_end])
    curr_offset = curr_end

  outdata += zlib.decompress(data[curr_offset:])
  return outdata

if len(sys.argv) != 3:
  print "Usage: python decexec.py exec key"

else:
  print "Executable %s - KeyFile: %s" %(sys.argv[1], sys.argv[2])
  # Load the exec
  exef = open(sys.argv[1])
  header = exef.read(512)
  exe = exef.read(0x100000)
  exef.close()

  # Pad to 0x100000
  print "Padding exe %s bytes" % (0x100000 - len(exe))
  exe += "\x00" * (0x100000 - len(exe))
  exe = bytearray(exe)

  timestamp = struct.unpack("<I", header[0x7C:0x80])[0] ^ 0x8AA8A9B8
  packedsize = struct.unpack("<I", header[0x178:0x17C])[0] ^ 0x6D42109D
  unpackedsize = struct.unpack("<I", header[0x11C:0x120])[0] ^ 0x106C6D42
  adler32 = struct.unpack("<I", header[0x134:0x138])[0]

  datetime = datetime.datetime.fromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%S')

  print "Exec produced in %s" %datetime
  print "Exec Packed Size: %s" %packedsize
  print "Exec Unpacked Size: %s" %unpackedsize
  print "Exec Adler32: %x" %adler32

  # Load the key
  keyf = open(sys.argv[2])
  key = bytearray(keyf.read())
  keyf.close()

  print "Doing Key Mess"
  for i in range(512):
    key[i] = (key[i] - 1) & 0xFF

  print "Decrypting Phase 0"
  exe = decryptPhase0(exe, key, packedsize)

  decadler = zlib.adler32(str(exe[:packedsize])) & 0xFFFFFFFF

  if decadler == adler32:
    print "Checksum OK"
    print "Decrypting Phase 1"
    exe = decryptPhase1(exe, packedsize)

    print "Decrypting Phase 2"
    exe = decryptPhase2(exe)

    print "Decompressing Phase 3"
    exe = decompressPhase(exe, unpackedsize)

    #print "Uncorrupting Phase 4"
    #exe = uncorruptPhase3(exe)

    print "Writting output to %s.dec" %sys.argv[1]
    f = open("%s.dec" %sys.argv[1],"wb")
    f.write(exe)
    f.close()
  else:
    print "Checksum Error! Expected %x - Matched %x" %(adler32, decadler)