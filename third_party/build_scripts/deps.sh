#!/usr/bin/env bash

# Parse args
JOBS=12
USAGE="./deps.sh [-j num_jobs] [-p prefix_dir] [-d|-r|-rdi] [-s|-a]"
INSTALL_TO_DIR="invalid"
build_type="invalid"
build_shared="invalid"
build_dir="_build"
build_tests="OFF"
build_samples="OFF"
cxx_std=17
checkpoint=0
checkpoint_end=1000000
while [ "$1" != "" ]; do
  case $1 in
    -j | --jobs  ) shift
                   JOBS=$1
                   ;;
    -p | --prefix) shift
                   INSTALL_TO_DIR=$1
                   ;;
    -d | --debug)
                   build_type="Debug"
                   build_dir="${build_dir}_debug"
                   ;;
    -r | --release)
                   build_type="Release"
                   build_dir="${build_dir}_release"
                   ;;
    -rdi | --rel-with-deb-info)
                   build_type="RelWithDebInfo"
                   build_dir="${build_dir}_relwithdebinfo"
                   ;;
    -s | --shared) build_shared="ON"
                   build_dir="${build_dir}_shared"
                   ;;
    -a | --static) build_shared="OFF"
                   build_dir="${build_dir}_static"
                   ;;
    -c | --checkpoint) shift
                   checkpoint=$1
                   ;;
    -e | --checkpoint-end) shift
                   checkpoint_end=$1
                   ;;
    * )            echo $USAGE
                   exit 1
esac
shift
done

if [ "${build_type,,}" = "invalid" ]; then
  echo "Must specify build type (debug/release)"
  exit 1
fi
if [ "${build_shared,,}" = "invalid" ]; then
  echo "Must specify build type (shared/static)"
  exit 1
fi
if [ "${INSTALL_TO_DIR,,}" = "invalid" ]; then
  echo "Must specify prefix dir"
  exit 1
fi

build_pic=$build_shared

cmake_extra_flags=()
case "$(uname -s)" in
    Linux*)     cmake_extra_flags=();;
    MINGW64*)   cmake_extra_flags=("-G" "MSYS Makefiles");;
    *)          echo "Unknown machine" && exit;;
esac




set -e
set -o xtrace
start_dir=$(pwd)
trap 'cd $start_dir' EXIT

# Must execute from the directory containing this script
cd "$(dirname "$0")"


echo "build_type = $build_type"
install_prefix=$(realpath $INSTALL_TO_DIR)/
prefix_path=$install_prefix
mkdir -p "$install_prefix"
echo "install_prefix     = $install_prefix"
echo "prefix_path        = $prefix_path"

deps_rev_file=deps_rev.txt

fmt_rev=$(grep fmt $deps_rev_file | tr -s ' ' | cut -f2 -d' ')
spdlog_rev=$(grep spdlog $deps_rev_file | tr -s ' ' | cut -f2 -d' ')
glog_rev=$(grep glog $deps_rev_file | tr -s ' ' | cut -f2 -d' ')
z3_rev=$(grep z3 $deps_rev_file | tr -s ' ' | cut -f2 -d' ')
zstd_rev=$(grep zstd $deps_rev_file | tr -s ' ' | cut -f2 -d' ')
rocksdb_rev=$(grep rocksdb $deps_rev_file | tr -s ' ' | cut -f2 -d' ')

boost_dl=$(grep boost_dl $deps_rev_file | tr -s ' ' | cut -f2 -d' ')

(
  set -e
  my_checkpoint=1
  if [ "$checkpoint" -gt "$my_checkpoint" ]; then exit 0; fi
  if [ "$checkpoint_end" -lt "$my_checkpoint" ]; then exit 0; fi
  # Install software
)

(
  set -e
  my_checkpoint=2
  if [ "$checkpoint" -gt "$my_checkpoint" ]; then exit 0; fi
  if [ "$checkpoint_end" -lt "$my_checkpoint" ]; then exit 0; fi

  if [ ! -e fmt ]; then
    git clone https://github.com/fmtlib/fmt.git
  fi
  cd fmt
  git fetch
  git checkout -f $fmt_rev

  mkdir -p $build_dir && cd $build_dir
  cmake .. "${cmake_extra_flags[@]}" \
    -DCMAKE_CXX_STANDARD=$cxx_std -DCMAKE_INSTALL_PREFIX="$install_prefix" -DCMAKE_PREFIX_PATH="$prefix_path" \
    -DCMAKE_BUILD_TYPE="$build_type" -DBUILD_SHARED_LIBS=$build_shared # -DFMT_DOC=OFF -DFMT_TEST=OFF

  make -j$JOBS
  make install
)

(
  set -e
  my_checkpoint=3
  if [ "$checkpoint" -gt "$my_checkpoint" ]; then exit 0; fi
  if [ "$checkpoint_end" -lt "$my_checkpoint" ]; then exit 0; fi

  if [ ! -e glog ]; then
    git clone https://github.com/google/glog.git
  fi
  cd glog
  git fetch
  git checkout -f $glog_rev
  git apply "$start_dir/_patch/glog.patch"

  mkdir -p $build_dir && cd $build_dir
  cmake .. "${cmake_extra_flags[@]}" \
    -DCMAKE_CXX_STANDARD=$cxx_std -DCMAKE_INSTALL_PREFIX="$install_prefix" -DCMAKE_PREFIX_PATH="$prefix_path;/mingw64" \
    -DCMAKE_BUILD_TYPE="$build_type" -DBUILD_SHARED_LIBS=$build_shared \
    -DWITH_GFLAGS=OFF

  make -j$JOBS
  make install
)

(
  set -e
  my_checkpoint=4
  if [ "$checkpoint" -gt "$my_checkpoint" ]; then exit 0; fi
  if [ "$checkpoint_end" -lt "$my_checkpoint" ]; then exit 0; fi

  if [ ! -e z3 ]; then
    git clone https://github.com/Z3Prover/z3.git
  fi
  cd z3
  git fetch
  git checkout -f $z3_rev

  mkdir -p $build_dir && cd $build_dir
  cmake .. "${cmake_extra_flags[@]}" \
    -DCMAKE_CXX_STANDARD=$cxx_std -DCMAKE_INSTALL_PREFIX="$install_prefix" -DCMAKE_PREFIX_PATH="$prefix_path" \
    -DCMAKE_BUILD_TYPE="$build_type" -DBUILD_SHARED_LIBS=$build_shared -DZ3_BUILD_LIBZ3_SHARED=$build_shared

  make -j$JOBS
  make install
)

(
  set -e
  my_checkpoint=5
  if [ "$checkpoint" -gt "$my_checkpoint" ]; then exit 0; fi
  if [ "$checkpoint_end" -lt "$my_checkpoint" ]; then exit 0; fi

  boost_ar_file="${boost_dl##*/}"
  if [ ! -e $boost_ar_file ]; then
    wget "$boost_dl"
  fi

  boost_dir=$(echo "$boost_ar_file" | sed s/.tar.bz2//)
  if [ ! -e $boost_dir ]; then
    tar xjf $boost_ar_file
  fi
  cd $boost_dir

  boost_build_type="invalid"
  boost_link="invalid"
  boost_debug_info="invalid"
  if [ "${build_type,,}" = "debug" ]; then
    boost_build_type="debug"
    boost_debug_info="on"
  elif [ "${build_type,,}" = "debug" ]; then
    boost_build_type="release"
    boost_debug_info="on"
  else
    boost_build_type="release"
    boost_debug_info="off"
  fi
  if [ "${build_shared,,}" = "on" ]; then
    boost_link="shared"
  else
    boost_link="static"
  fi
  boost_install_prefix="$install_prefix"
  case "$(uname -s)" in
    MINGW64*)   boost_install_prefix="`cygpath -m "$boost_install_prefix"`";;
  esac
  echo "BOOST VARIANT    = $boost_build_type"
  echo "BOOST LINK       = $boost_link"
  echo "BOOST DEBUG INFO = $boost_debug_info"
  ./bootstrap.sh --without-libraries=python --prefix="$boost_install_prefix" --with-toolset=gcc
  ./b2 install variant=$boost_build_type link=$boost_link toolset=gcc debug-symbols=$boost_debug_info address-model=64 \
        cxxflags="-std=c++$cxx_std" --build-dir="$build_dir" -j$JOBS
)

(
  set -e
  my_checkpoint=6
  if [ "$checkpoint" -gt "$my_checkpoint" ]; then exit 0; fi
  if [ "$checkpoint_end" -lt "$my_checkpoint" ]; then exit 0; fi

  if [ ! -e zstd ]; then
    git clone https://github.com/facebook/zstd.git
  fi
  cd zstd
  git fetch
  git checkout -f $zstd_rev

  make PREFIX="$install_prefix" -j$JOBS
  make PREFIX="$install_prefix" install
)

(
  set -e
  my_checkpoint=7
  if [ "$checkpoint" -gt "$my_checkpoint" ]; then exit 0; fi
  if [ "$checkpoint_end" -lt "$my_checkpoint" ]; then exit 0; fi

  if [ ! -e rocksdb ]; then
    git clone https://github.com/facebook/rocksdb
  fi
  cd rocksdb
  git fetch
  git checkout -f $rocksdb_rev

  mkdir -p $build_dir && cd $build_dir
  cmake .. "${cmake_extra_flags[@]}" \
    -DCMAKE_CXX_STANDARD=$cxx_std -DCMAKE_INSTALL_PREFIX="$install_prefix" -DCMAKE_PREFIX_PATH="$prefix_path;/mingw64" \
    -DCMAKE_BUILD_TYPE="$build_type" -DROCKSDB_BUILD_SHARED=ON \
    -DWITH_GFLAGS=OFF -DWITH_ZSTD=ON

  make -j$JOBS
  make install
)