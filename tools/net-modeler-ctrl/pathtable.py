import sys
import socket
import struct
import os

PATHTABLE_PROC="/proc/net-modeler/pathtable"

def read(args):
  if not os.path.exists(PATHTABLE_PROC):
    print "Cannot open proc interface: '{0}' does not appear to exist".format(PATHTABLE_PROC)
    sys.exit(1)

  try:
    with open(PATHTABLE_PROC) as procf:
      data = procf.readlines()
  except IOError,e:
    if e.errno == 13:
      print "Don't have permissions to read '{0}'".format(PATHTABLE_PROC)
      sys.exit(1)

  for line in data:
    print line.strip('\n')

def __check_zero(name,arg,fmt=sys.stderr):
  if arg < 0:
    fmt.write("Error: '{0}' must be at least zero. Was '{1}'".format(name,arg))
    return -1
     
  return 0

def __ip_to_id(ip):
  result = socket.htonl(struct.unpack("I",socket.inet_aton(ip))[0])
  return result
  

def set_path(args):

  # hopid, bw, latency
  path_static_fmt  = "=IIBB"
  src = __ip_to_id(args.src_ip)
  dst = __ip_to_id(args.dst_ip)
  nhops = len(args.hops)
  if nhops < 1:
    print "Number of hops supplied must be more than 1. Got {0}".format(nhops)
    sys.exit(-1)

  # Add the struct formatting characters for our list of hops
  path_static_fmt += "{0}I".format(nhops)

  hops = map(int,args.hops)

  bytestr = struct.pack(path_static_fmt,src,dst,0,nhops,*hops)

  if not os.path.exists(PATHTABLE_PROC):
    print "Cannot open proc interface: '{0}' does not appear to exist".format(PATHTABLE_PROC)
    sys.exit(1)

  sys.stderr.write("Writing {0} bytes to pathtable\n".format(len(bytestr)))

  try:
    with open(PATHTABLE_PROC,'w') as procf:
      procf.write(bytestr);
  except IOError,e:
    if e.errno == 13:
      print "Don't have permissions to write '{0}'".format(PATHTABLE_PROC)
      sys.exit(1)
    else:
      print "Failed. Check 'dmesg' for more details. Errno: {0}".format(e.errno)
  else:
    sys.stderr.write("Added {0} -> {1}: {2}\n".format(args.src_ip,args.dst_ip,hops))


