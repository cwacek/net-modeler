import sys
import argparse
import os
import modelinfo

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


def main():
  
  parser = argparse.ArgumentParser(description="Userspace control for net-modeler")

  subp = parser.add_subparsers()
  modelinfo_args(subp)

  args = parser.parse_args()
  args.func(args)




if __name__ == "__main__":
  main()
