#!/bin/bash

rm -rf ./data

pushd .

cd ../matrix-multiplier

rm -rf ./perf

make all-modes

echo Dry-running OpenCL
mpiexec -n 4 bin/matrix-multiplier-opencl -n 2000
mpiexec -n 4 bin/matrix-multiplier-opencl -n 2000

for m in serial openmp opencl ; do
	echo
	echo Measuring $m

	for n in 100 300 500 700 900 1000 1200 1500 1700 2000 ; do

		for p in 1 2 3 4 ; do

			if [ "$m" = "openmp" ] ; then

				for t in 2 4 8 0 ; do
					e=$(mpiexec -n $p bin/matrix-multiplier-$m -n $n -t $t)
					x=$(echo $e | rev | cut -d' ' -f 1,2 | rev | awk '{ gsub(/[^0-9.]/, ""); print }')

					echo $m/$n/$p/$t - $x
					mkdir -p perf/$m/$n/$p
					echo $x > perf/$m/$n/$p/$t
				done

			else
				e=$(mpiexec -n $p bin/matrix-multiplier-$m -n $n)
				x=$(echo $e | rev | cut -d' ' -f 1,2 | rev | awk '{ gsub(/[^0-9.]/, ""); print }')

				echo $m/$n/$p - $x
				mkdir -p perf/$m/$n
				echo $x > perf/$m/$n/$p
			fi
		done
	done
done

mv ./perf ../matrix-multiplier-perf/data

popd
