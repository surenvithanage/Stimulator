#!/bin/bash

rm -rf ./data

pushd .

N=(10000 40000 80000 120000 150000 250000 500000 1000000)
P=(2 3 4)
k=5

# Generate dataset
cd ../cluster-py

rm -rf ../cluster/perf

source env/bin/activate

for n in "${N[@]}" ; do
	echo Generating n=$n
	python cluster-gen.py -n $n -k $k -o ../cluster/perf/$n -g
done

deactivate

cd ../cluster

# Compile
make all-modes

# Measure

for n in "${N[@]}"; do
	e=$(bin/cluster-serial -k $k -i perf/$n/values)
	x=$(echo $e | rev | cut -d' ' -f 1,2 | rev | awk '{ gsub(/[^0-9.]/, ""); print }')

	mkdir -p perf/serial
	echo $x > perf/serial/$n
	echo Wrote: perf/serial/$n
done

for p in "${P[@]}"; do
	for n in "${N[@]}"; do

		for m in mpi_serial mpi_opencl ; do
			e=$(mpiexec -n $p bin/cluster-$m -k $k -i perf/$n/values)
			x=$(echo $e | rev | cut -d' ' -f 1,2 | rev | awk '{ gsub(/[^0-9.]/, ""); print }')

			mkdir -p perf/$m/$p
			echo $x > perf/$m/$p/$n
			echo Wrote: perf/$m/$p/$n
		done
	done
done

mv ./perf ../cluster-perf/data

popd
