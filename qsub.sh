#BSUB -q linuxlab-cse539
#BSUB -o sort.out.%J
#BSUB -e sort.out.%J
#BSUB -J sort

./sort 1000
