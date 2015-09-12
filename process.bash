#!/bin/bash
ls > ligandlist ./Ligand
mkdir -p Output
mkdir -p ProcessedLigand
rm mpiVINA
mpicc mpiVINAfinal.c --output mpiVINA
mpiexec -np 15 -hostfile hostfiles mpiVINA
#result analysis
cd Output
grep "  1 " *.txt | cut -c1-12,35-42 >result
#grep "  1 " *.txt | cut -c1-7,30-35 >result
echo "Here is the merged result"
#cat result
echo "See the SortedResult file of Output folder"
sort -n +1 -2 result -o SortedResult
cat SortedResult
