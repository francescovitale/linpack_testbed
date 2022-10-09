arr_size=$1
n_reps=$2
n_execution_reps=$3
linpack_filepath="/home/francesco/Desktop/linpack_testbed/task/bin/linpack"
ip="127.0.0.1"
port="2200"

cd ansibench/linpack
make clean
make
cd ../..

cp ansibench/linpack/bin/linpack task/bin

cd task/src
make clean
make
cd ../bin

./task $arr_size $n_reps $n_execution_reps $linpack_filepath $ip $port > execution_times.txt

chmod 666 execution_times.txt
