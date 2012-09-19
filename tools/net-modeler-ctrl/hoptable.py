import sys
import struct
import os

HOPTABLE_PROC="/proc/net-modeler/hoptable"

def read(args):
  if not os.path.exists(HOPTABLE_PROC):
    print "Cannot open proc interface: '{0}' does not appear to exist".format(HOPTABLE_PROC)
    sys.exit(1)

  try:
    with open(HOPTABLE_PROC) as procf:
      data = procf.readlines()
  except IOError,e:
    if e.errno == 13:
      print "Don't have permissions to read '{0}'".format(HOPTABLE_PROC)
      sys.exit(1)

  for line in data:
    print line.strip('\n')

def __check_zero(name,arg,fmt=sys.stderr):
  if arg < 0:
    fmt.write("Error: '{0}' must be at least zero. Was '{1}'".format(name,arg))
    return -1
     
  return 0

def set_hops(args):

  # hopid, bw, latency
  hop_proc_fmt = "IIII"
  if __check_zero('bw',args.bw):
    sys.exit(-1)
  if __check_zero('latency',args.latency):
    sys.exit(-1)
  if __check_zero('id',args.hopid):
    sys.exit(-1)
  if __check_zero('qlen',args.qlen):
    sys.exit(-1)

  # We input bw in KBps, and need it in B/ms
  bw = args.bw *1024 / 1000

  bytestr = struct.pack(hop_proc_fmt,args.hopid,bw,args.latency,args.qlen)

  if not os.path.exists(HOPTABLE_PROC):
    print "Cannot open proc interface: '{0}' does not appear to exist".format(HOPTABLE_PROC)
    sys.exit(1)

  try:
    with open(HOPTABLE_PROC,'w') as procf:
      procf.write(bytestr);
  except IOError,e:
    if e.errno == 13:
      print "Don't have permissions to write '{0}'".format(HOPTABLE_PROC)
      sys.exit(1)
    else:
      print "Failed. Check 'dmesg' for more details."


