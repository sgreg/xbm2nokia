#!/bin/bash
#
# xbm2nokia.sh - create raw data for Nokia 3310/5110 LCD from XBM files
#
# Copyright (C) 2017 Sven Gregori <sven@craplab.fi>
# Released under MIT License
#
# see usage function for general usage information.
#
# All files, variables and labels related to generating the LCD data
# from XBM files is prefixes with "xbm2nokia"
# All files, variables and lables related to the generated, raw LCD
# used on the target device is prefixed with "nokia_gfx"
#
#
# TODO improvements could be made in few ways:
#   1. Nokia LCD memory buffer is 84x48 / 8 == 504 bytes, the transition
#      diff could therefore fit in two 8bit variables. Currently one 16bit
#      variable is used instead, which wastes quite some memory. Splitting
#      a frame in upper and lower part would be one example to save space.
#
#   2. The main idea of animation transition diff is to save space by only
#      storing the information that changes relative to the previous frame.
#      One full frame requires 504 bytes, if only one byte changes between
#      frames, it can be achieved with 3 bytes (16bit address, 8bit data)
#      instead of unnecessary 504 bytes. (2 bytes if splitting frame in
#      upper and lower parts). However, if more than 1/3, i.e. 168 bytes
#      change from one frame to the other, the whole concept is futile and
#      more storage is required compared to storing full frames.
#      Long story short, add concept that loads the whole frame if more
#      than certain amount of bytes change between frames.
#
#   3. This script could have some additional options like output file name
#      and maybe even modes to mix animation and graphics data.
#
#   4. Add some helper C file for easier usage of the generated raw data
#      on the target device.
#

# enable/disable debug mode
# set to 1 to keep build dir and individually generated files in it
# set to 0 to delete build dir after operations are done
debug=0

function usage {
    cat << EOL
Usage: $0 [-a|-g] <file_1.xbm> ... <file_n.xbm>"

Create raw data for Nokia 3310/5110 LCD from given XBM files.
Data can be either created as set of individual graphics, in which
case the entire graphic data is written, or as animation, in which
case only the frame diff data is created.

Options:
    -a  create animation from given files
    -g  create set of individual graphics from given files

All given XBM files are processed in the given order, therfore, if
creating an animation, the animation itself is built from that order
EOL
}

# check that there are at least two paramters
if [ $# -lt 2 ] ; then
    usage
    exit 1
fi

# check first parameter which should determine operation mode
mode_animation=0
mode_graphics=0

if [ "$1" == "-a" ] ; then
    echo "Creating animation"
    mode_animation=1
elif [ "$1" == "-g" ] ; then
    echo "Creating set of graphics"
    mode_graphics=1
else
    usage
    exit 1
fi

# shift parameters, they all should be files now
shift

# check if all given files actually exist
files_okay=1
for arg ; do
    if [ ! -f $arg ] ; then
        echo "ERROR: no such file $arg"
        files_okay=0
    fi
done

# nope, at least one of the given files doesn't exist, abort
if [ $files_okay -ne 1 ] ; then
    exit 1
fi

# split paramters into array
files=("$@")
file_count=$#

# create build dir
builddir="$(mktemp -d)"
if [ $debug -eq 1 ] ; then
    echo "Using build dir $builddir"
fi

# path to xbm2nokia directory
srcdir="$(dirname $0)"

xbm2nokia_header="xbm2nokia.h"
xbm2nokia_src="xbm2nokia.c"
cp $srcdir/$xbm2nokia_src $builddir

# define output files
headerfile="$builddir/nokia_gfx.h"
sourcefile="$builddir/nokia_gfx.c"

# write output header file header
cat > $headerfile << EOL
/* Automatically created by $(basename $0) */
#ifndef _NOKIA_GFX_H_
#define _NOKIA_GFX_H_

EOL

# write animation data types for output header file, if creating animation
[ $mode_animation -eq 1 ] && cat >> $headerfile << EOL
#define NOKIA_GFX_ANIMATION

struct nokia_gfx_diff {
    uint16_t addr;
    uint8_t data;
};

struct nokia_gfx_frame {
    uint16_t delay;
    uint16_t diffcnt;
    struct nokia_gfx_diff diffs[];
};

#define NOKIA_GFX_FRAME_COUNT $file_count

EOL

[ $mode_graphics -eq 1 ] && cat >> $headerfile << EOL
#define NOKIA_GFX_COUNT $file_count

EOL

# write ouput source file header
cat > $sourcefile << EOL
/* Automatically created by $(basename $0) */
#include <avr/pgmspace.h>
#include <stdint.h>
#include "$(basename $headerfile)"

EOL


function create_single_frame {
    file="$1"
    template=$2

    prefix="$(basename $file .xbm)"
    outfile="keyframe_${prefix}"

    cp $file $builddir

    sed s/@frame_one@/$prefix/g $template > $builddir/$xbm2nokia_header
    if [ $debug -eq 1 ] ; then
        cp $builddir/$xbm2nokia_header $builddir/${xbm2nokia_header}-${outfile}
    fi

    gcc -o $builddir/$outfile $builddir/$xbm2nokia_src
    $builddir/$outfile 2>>$headerfile >>$sourcefile;
}

function create_graphic {
    echo "creating graphic from $1"
    if [ -e $builddir/$1 ] ; then
        echo "WARNING: file is duplicate, skipping"
    else
        create_single_frame $1 "$srcdir/xbm2nokia_h_graphic.template"
    fi
}

function create_key_frame {
    echo "creating keyframe from $1"
    create_single_frame $1 "$srcdir/xbm2nokia_h_keyframe.template"
}

function create_diff_frame {
    one="$1"
    two="$2"
    template="$srcdir/xbm2nokia_h_transition.template"

    prefix_one="$(basename $one .xbm)"
    prefix_two="$(basename $two .xbm)"
    outfile="${prefix_one}-${prefix_two}"

    echo "creating frame $one -> $two"
    if [ ! -e $builddir/$one ] ; then
        cp $one $builddir
    fi

    if [ ! -e $builddir/$two ] ; then
        cp $two $builddir
    fi

    sed s/@frame_one@/$prefix_one/g $template |
        sed s/@frame_two@/$prefix_two/g > $builddir/$xbm2nokia_header
    if [ $debug -eq 1 ] ; then
        cp $builddir/$xbm2nokia_header $builddir/${xbm2nokia_header}-${outfile}
    fi

    gcc -o $builddir/$outfile $builddir/$xbm2nokia_src
    $builddir/$outfile 2>>$headerfile >>$sourcefile;
}


function create_animation {
    # create keyframe data
    create_key_frame ${files[0]}

    # create frame transition diff data
    let cnt=0
    while [ $cnt -lt $((file_count - 1)) ] ; do
        create_diff_frame ${files[cnt]} ${files[$((cnt + 1))]}
        let cnt=cnt+1
    done

    create_diff_frame ${files[cnt]} ${files[0]}
}

function create_graphics {
    for file in ${files[@]} ; do
        create_graphic $file
    done
}

# create actual data
[ $mode_animation -eq 1 ] && create_animation || create_graphics


# write header file footer
cat >> $headerfile << EOL

#endif
EOL

# copy generated header and source file in current directory
cp $headerfile .
cp $sourcefile .

# clean up if needed
if [ $debug -ne 1 ] ; then
    rm -rf $builddir
fi

