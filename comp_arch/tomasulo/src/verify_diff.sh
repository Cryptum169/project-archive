declare -a arr=("bwaves603_2M" "gcc602_2M" "mcf605_2M" "perlbench600_2M")

declare -a confi=("default" "large_cache" "large_gshare" "two_r")


echo "Starting Verification"
for j in "${confi[@]}"
do
	for i in "${arr[@]}"
	do
		diff my_verification_logs/"$j"/"$i.log" ../../verification_logs/"$j"/"$i.log"
		echo "$j $i"
	done
done
