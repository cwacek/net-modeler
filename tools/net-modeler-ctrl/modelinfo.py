import struct
import sys
import os

MODELINFO_PROC="/proc/net-modeler/modelinfo"

def read(args):
  if not os.path.exists(MODELINFO_PROC):
    print "Cannot open proc interface: '{0}' does not appear to exist".format(MODELINFO_PROC)
    sys.exit(1)

  try:
    with open(MODELINFO_PROC) as procf:
      data = procf.readlines()
  except IOError,e:
    if e.errno == 13:
      print "Don't have permissions to read '{0}'".format(MODELINFO_PROC)
      sys.exit(1)

  for line in data:
    print line.strip('\n')


def set_info(args):

  nm_model_details_fmt = "B32sII"
              
  name = args.name[:31]
  name += '\0'

  bytestr = struct.pack(nm_model_details_fmt,args.id,name,args.hops,args.endpoints)

  if not os.path.exists(MODELINFO_PROC):
    print "Cannot open proc interface: '{0}' does not appear to exist".format(MODELINFO_PROC)
    sys.exit(1)

  try:
    with open(MODELINFO_PROC,'w') as procf:
      procf.write(bytestr);
  except IOError,e:
    if e.errno == 13:
      print "Don't have permissions to write '{0}'".format(MODELINFO_PROC)
      sys.exit(1)


  pass

  


