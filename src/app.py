import argparse as ap
from os import listdir
import os
from os.path import isfile, join, basename
import re

parser = ap.ArgumentParser(description="Benchmark Your CL")
parser.add_argument('-D', '--dir', dest="bin_dir")
parser.add_argument('-n', '--num', dest="reps")
parser.add_argument('-l', '--lengths', dest="arrlengths")
parser.add_argument('-b', '--binary', dest="bin", action="store_true")
parser.add_argument('-p', '--platform', dest="plat")
parser.add_argument('-d', '--device', dest="dev")
parser.add_argument('-v', '--verbose', dest="verbose", action="store_true")
parser.add_argument('-H', '--host-mem', dest='host_mem', action='store_true')
parser.add_argument('-F', '--filter', dest='filter')
parser.add_argument('-r', '--reps', dest='reps')

args = parser.parse_args()
reps = args.reps or 1

array_lengths = [int(a) for a in args.arrlengths.split(',')]
if args.verbose:
    print(array_lengths)

allfiles = [join(args.bin_dir, f) for f in listdir(args.bin_dir) if isfile(join(args.bin_dir, f))]
files = [join(args.bin_dir, f) for f in listdir(args.bin_dir) if isfile(join(args.bin_dir, f)) and not "special" in os.path.splitext(basename(f))[0]]
if args.filter:
    if args.verbose:
        print("Filtering by")
        print(args.filter)
    files = [f for f in files if args.filter in f]

if args.verbose:
    print(files)


def get_g_l_range(l, name):
    if "radix_local" in name:
        return (8,1,1), (8,1,1)
    if "radix_glob" in name:
        return (8,1,1), (1,1,1)
    if "2d" in name:
        return (l / 8, 8, 1), (16,8,1)
    if "nestedloop_2" in name:
        return (2, 1, 1), (1, 1, 1)
    if "global_and_local" in name:
        loc_size = re.findall(r'\d+', name)[0]
        glob_ndr = l / int(loc_size)
        if l % int(loc_size) != 0:
            raise RuntimeError()
        return (int(glob_ndr),1,1), (8,1,1)
    return (1,1,1), (1,1,1)



for f in files:
    for l in array_lengths:
        ((g1, g2, g3), (l1, l2, l3)) = get_g_l_range(l, f)
        runstr = "../bin/a {} {} {} {} {} {} {} {} {} {} {} {} {}".format(
            f,
            "b" if args.bin else "s",
            l,
            reps,
            g1, g2, g3,
            l1, l2, l3,
            "h" if args.host_mem else "d",
            args.plat,
            args.dev
        )
        if args.verbose:
            print(runstr)
        os.system(runstr)

