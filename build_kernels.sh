#!/usr/bin/env bash
platform=$1

dirs=( bin kernel_bin kernel_bin/ioc64 kernel_bin/fpga kernel_bin/fpga/emulator kernel_bin/fpga/device kernel_bin/preprocessed logs/ioc64 logs/fpga logs/fpga/device reports )

mkdir -p ${dirs[*]}

function compilecl {
file="$1"
appendeds="$2"
buildopts="$3"
srcpath="device/$file.cl"
binpath=""

echo "Preprocess cl file"
binpath="kernel_bin/preprocessed/${file}_${appendeds}.cl"
cpp $buildopts -P $srcpath -o $binpath
srcpath=$binpath

echo "Compiling cl file $srcpath for platform $platform"
case $platform in
    ioc64)
        binpath="kernel_bin/ioc64/${file}_${appendeds}.ir"
        logpath="logs/ioc64/${file}_${appendeds}.txt"
        ioc64 -cmd=build -input="$srcpath" -ir="$binpath" -output="$logpath" -bo=\""$buildopts"\"
        ;;
    aoc_emulator)
        #binpath="kernel_bin/fpga/emulator/${file}_${appendeds}"
        #aoc -march=emulator "$srcpath" -o "$binpath" "$buildopts" |& tee -a "logs/fpga/emulator/${file}_${appendeds}.txt"
        mkdir -p "kernel_bin/fpga/emulator/${file}_${appendeds}/"
	    binpath="kernel_bin/fpga/emulator/${file}_${appendeds}/${file}_${appendeds}"
        aoc -march=emulator "$srcpath" -o "$binpath" "$buildopts" |& tee -a "logs/fpga/emulator/${file}_${appendeds}.txt"
        cp "$binpath.aocx" "kernel_bin/fpga/emulator/${file}_${appendeds}.aocx"
        ;;
    aoc_report)
        mkdir -p "kernel_bin/fpga/device/${file}_${appendeds}/"
	    binpath="kernel_bin/fpga/device/${file}_${appendeds}/${file}_${appendeds}"
        #binpath="kernel_bin/fpga/device/${file}_${appendeds}"
        aoc -rtl "$srcpath" -o "$binpath" "$buildopts" > "logs/fpga/device/${file}_${appendeds}.txt"
        if [ $? -eq 0 ]; then
            #zip -r "reports/${file}_${appendeds}.zip" "$binpath/reports"
            tar -cJf "reports/${file}_${appendeds}.tar.xz" "$binpath/reports"
            cat "$binpath/${file}_${appendeds}.log" | mail -s "${file}_${appendeds} report" -a "reports/${file}_${appendeds}.tar.xz" me@example.com
        fi
        ;;
    aoc_binary_qsub)
        mkdir -p "kernel_bin/fpga/device/${file}_${appendeds}/"
        binpath="kernel_bin/fpga/device/${file}_${appendeds}/${file}_${appendeds}"
        #binpath="kernel_bin/fpga/device/${file}_${appendeds}"
        #~/myqsub-aoc "$srcpath" -o "$binpath" "$buildopts"
        #cp "$binpath.aocx" "kernel_bin/fpga/device/${file}_${appendeds}.aocx"
        qsub <<EOF
#!/bin/bash
#PBS -q skl
#PBS -V
#PBS -j oe
#PBS -l nodes=1:ppn=4
#PBS -l mem=24gb
#PBS -N "build_qsub_aoc_${file}_${appendeds}"
#PBS -m abe
#PBS -M me@example.com

cd "\${PBS_O_WORKDIR}"
cat "" > "logs/fpga/device/${file}_${appendeds}.txt"
aoc --high-effort "$srcpath" -o "$binpath" "$buildopts" |& tee -a "logs/fpga/device/${file}_${appendeds}.txt"
cp "$binpath.aocx" "kernel_bin/fpga/device/${file}_${appendeds}.aocx"
EOF
        ;;
    aoc_binary)
        binpath="kernel_bin/fpga/device/${file}_${appendeds}"
        aoc "$srcpath" -o "$binpath" "$buildopts" > "logs/fpga/device/${file}_${appendeds}.txt"
        ;;
    preprocessor)
#        binpath="kernel_bin/preprocessed/${file}_${appendeds}.cl"
#        cpp $buildopts -P $srcpath -o $binpath
        ;;
    *)
        echo "Invalid first argument"
        return -1
        ;;
esac
}


if [[ $# == 4 ]]
then
    compilecl "$2" "$3" "$4"
else
    #compilecl histogram_swi_nestedloop "" ""
    #compilecl histogram_swi_nestedloop_1 "basic" ""

    #compilecl histogram_swi_nestedloop_1 "locmem" "-DLOCAL_HIST"
    #compilecl histogram_swi_nestedloop_1 "unrolled" "-DUNROLL"
    #compilecl histogram_swi_nestedloop_1 "loc_unrolled" "-DLOCAL_HIST -DUNROLL"

    #compilecl histogram_swi_nestedloop_2 "alt" "-Dstatic_keys"

    #compilecl histogram_swi_nestedloop_5 "alt" "-Dstatic_keys"

    compilecl histogram_swi_nestedloop_6 "unroll_1" "-DUNROLL=1"
    compilecl histogram_swi_nestedloop_6 "unroll_2" "-DUNROLL=2"
    compilecl histogram_swi_nestedloop_6 "unroll_4" "-DUNROLL=4"
    compilecl histogram_swi_nestedloop_6 "unroll_5" "-DUNROLL=5"
    compilecl histogram_swi_nestedloop_6 "unroll_6" "-DUNROLL=6"

    compilecl histogram_swi_nestedloop_6_extra "unroll_6" "-DUNROLL=6"
    compilecl histogram_swi_nestedloop_6_extra2 "unroll_6" "-DUNROLL=6"

fi
exit
