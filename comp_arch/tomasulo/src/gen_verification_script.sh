#!/bin/bash
# Version 1.1


mkdir my_verification_logs

echo "****************************"
echo "***Default Configuration ***"
echo "****************************"
mkdir my_verification_logs/default
for i in `ls ../../traces/`; do
    echo ""
    echo "--- $i ---"
    out_name="${i%%.*}.log"
	./procsim -i ../../traces/$i > my_verification_logs/default/$out_name
done;

echo ""
echo "2 X Reservation Stations"
echo "****************************"
echo "********** R = 2  **********"
echo "****************************"
mkdir my_verification_logs/two_r
for i in `ls ../../traces/`; do
    echo ""
    echo "--- $i ---"
    out_name="${i%%.*}.log"
	./procsim -r 2 -i ../../traces/$i > my_verification_logs/two_r/$out_name
done;

echo ""
echo "Bigger Cache"
echo "****************************"
echo "********** C = 15  *********"
echo "****************************"
mkdir my_verification_logs/large_cache
for i in `ls ../../traces/`; do
    echo ""
    echo "--- $i ---"
    out_name="${i%%.*}.log"
	./procsim -c 15 -i ../../traces/$i > my_verification_logs/large_cache/$out_name
done;

echo ""
echo "Bigger Gshare"
echo "****************************"
echo "********** G = 10  *********"
echo "****************************"
mkdir my_verification_logs/large_gshare
for i in `ls ../../traces/`; do
    echo ""
    echo "--- $i ---"
    out_name="${i%%.*}.log"
	./procsim -g 10 -i ../../traces/$i > my_verification_logs/large_gshare/$out_name
done;
