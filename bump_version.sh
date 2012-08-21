#!/bin/bash

function usage {
  [[ -z $1 ]] || echo "Error $1"
  echo "Usage: `basename $0` filename [--major | --minor] [--tag <tag>]"
  exit 1
}

if [[ $# -gt 5 || $# -lt 1 ]]; then
  usage "Incorrect number of arguments";
fi

filename=$1
[[ -e $filename ]] || usage "Version file '$filename' doesn't exist" 
shift

while [ $# -ne 0 ]; do
  case $1 in
  --minor)
    bump_minor=1
    ;;
  --major)
    bump_major=1
    ;;
  --tag)
    set_tag=1
    shift
    tag_name=$1
    ;;
  *)
    echo "Unrecognized option: '$1'"
    ;;
  esac
  shift
done


[[ (-z $bump_minor) && (-z $bump_major) && (-z $set_tag) ]] && usage "Need at least one option"
[[ (-n $bump_major) && (-n $bump_minor) ]] && usage "Can't bump major and minor versions at the same time"

function get_minor {
# "alpha-0.2"
  minor=$(echo $1 | cut -d '.' -f 2)
}

function get_major {
  major=$(echo ${1##*-} |cut -d '.' -f 1)
}

function get_tag {
  tag=$(echo ${1%%-*})  
}

function get_version_string {
  if [[ ${1##*.} = "py" ]]; then
    vtype="python"
    version=$(cat $1 | cut -d '"' -f 2)
  elif [[ ${1##*.} = "i" ]]; then
    vtype="c"
    version=$(cat $1 |cut -d ' ' -f 3 | cut -d '"' -f 2)
  else
    echo "Don't know how to parse version strings from files with '${1##*.}' extension"
    usage
  fi
}

get_version_string $filename
get_minor $version
get_major $version
get_tag $version

[[ -z $bump_minor ]] || minor=$(expr $minor + 1) 
if [[ ! -z $bump_major ]]; then
  major=$(expr $major + 1) 
  minor=0
fi
[[ -z $set_tag ]] || tag=$tag_name

case $vtype in
  python)
    echo "version = \"$tag-$major.$minor\"" > $filename
    ;;
  c)
    echo "#define VERSION \"$tag-$major.$minor\"" >$filename
    ;;
  *)
    echo "Error."
    usage
    ;;
esac
    




