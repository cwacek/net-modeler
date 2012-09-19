import sys
import argparse
import os
import modelinfo
import hoptable
import pathtable

def modelinfo_args(main_subparser):
  modelinfop = main_subparser.add_parser('modelinfo')
  modelinfo_sub = modelinfop.add_subparsers()

  modelinfo_read =  modelinfo_sub.add_parser('read')
  modelinfo_set =  modelinfo_sub.add_parser('set')

  modelinfo_read.set_defaults(func=modelinfo.read)

  modelinfo_set.set_defaults(func=modelinfo.set_info)
  modelinfo_set.add_argument('id', help="A numeric identifier ",
                                      type=int)
  modelinfo_set.add_argument('name', help="The name of the model [32 chars]",
                                      type=str)
  modelinfo_set.add_argument('hops', help="The number of total hops in the model",
                                      type=int)
  modelinfo_set.add_argument('endpoints',help="The total number of endpoints in the model",
                                      type=int)

def pathtable_args(main_subparser):
  pathtablep = main_subparser.add_parser('pathtable')
  pathtable_sub = pathtablep.add_subparsers()

  pathtable_read = pathtable_sub.add_parser('read')
  pathtable_read.set_defaults(func=pathtable.read)

  pathtable_set = pathtable_sub.add_parser('set')
  pathtable_set.set_defaults(func=pathtable.set_path)

  pathtable_set.add_argument("src_ip", help="The origin of the path",
                              type=str)
  pathtable_set.add_argument("dst_ip", help="The destination of the path",
                              type=str)
  pathtable_set.add_argument("hops", help="The numeric identifiers of the hops that comprise this path.",
                              nargs="+")


def hoptable_args(main_subparser):
  hoptablep = main_subparser.add_parser('hoptable')
  hoptable_sub = hoptablep.add_subparsers()

  hoptable_read = hoptable_sub.add_parser('read')
  hoptable_read.set_defaults(func=hoptable.read)

  hoptable_set = hoptable_sub.add_parser('set')
  hoptable_set.set_defaults(func=hoptable.set_hops)

  hoptable_set.add_argument('hopid', help="The numeric ID of the hop that you wish to set.",
                            type=int)
  hoptable_set.add_argument('bw', help="The speed of this hop in KBps.",
                            type=int)
  hoptable_set.add_argument('latency', help="The latency of this hop in milliseconds",
                            type=int)
  hoptable_set.add_argument('qlen', help="The buffer queue length of this hop.",
                            type=int,default=100)

def main():
  
  parser = argparse.ArgumentParser(description="Userspace control for net-modeler")

  subp = parser.add_subparsers()
  modelinfo_args(subp)
  hoptable_args(subp)
  pathtable_args(subp)

  args = parser.parse_args()
  args.func(args)




if __name__ == "__main__":
  main()
