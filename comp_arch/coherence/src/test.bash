rm myresult/*

for protocol in MSI MESI MOSI MOESIF
do
    for trace in 4 8 16
    do
        trace_name="proc_validation/"
        export_name="$protocol-$trace.log"
        echo $export_name
        ./sim_trace -t traces/$trace$trace_name -p $protocol 2> "myresult/$export_name"
    done
done

echo "diffing"
diff myresult/MSI-4.log traces/4proc_validation/MSI_validation.txt
diff myresult/MOSI-4.log traces/4proc_validation/MOSI_validation.txt
diff myresult/MESI-4.log traces/4proc_validation/MESI_validation.txt
diff myresult/MOESIF-4.log traces/4proc_validation/MOESIF_validation.txt
diff myresult/MSI-8.log traces/8proc_validation/MSI_validation.txt
diff myresult/MOSI-8.log traces/8proc_validation/MOSI_validation.txt
diff myresult/MESI-8.log traces/8proc_validation/MESI_validation.txt
diff myresult/MOESIF-8.log traces/8proc_validation/MOESIF_validation.txt
diff myresult/MSI-16.log traces/16proc_validation/MSI_validation.txt
diff myresult/MOSI-16.log traces/16proc_validation/MOSI_validation.txt
diff myresult/MESI-16.log traces/16proc_validation/MESI_validation.txt
diff myresult/MOESIF-16.log traces/16proc_validation/MOESIF_validation.txt
echo "finished diffing"