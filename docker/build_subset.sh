#!/bin/bash
#
# This script makes binary subset file.

prefix=~/trajectorysubsetter
echo $prefix

docker build -t subset_c7 -f subset_c7.Dockerfile .
docker run --name subset_c7 -v ${prefix}/subsetter:/home/subsetter -it subset_c7 make
sleep 2m
docker cp subset_c7:/home/subsetter/subset ${prefix}/subset
# clean docker
docker rm subset_c7
docker image rm subset_c7
docker image prune -a -f
rm ${prefix}/subsetter/subset