#!/bin/bash

if [ "$#" != "1" ]; then
    echo "usage: experiment.sh <filename>"
    exit 1
fi

filename=$1

echo binary-search iw-bel2
(cd binary-search; ./experiment.sh 80 10 iw-bel2 5 1 250 300 > results/$filename)
(cd binary-search; ./experiment.sh 80 10 iw-bel2 5 2 250 300 >> results/$filename)
(cd binary-search; ./experiment.sh 80 10 iw-bel2 5 3 250 300 >> results/$filename)
echo binary-search iw-bel3
(cd binary-search; ./experiment.sh 80 10 iw-bel3 5 1 250 300 >> results/$filename)
(cd binary-search; ./experiment.sh 80 10 iw-bel3 5 2 250 300 >> results/$filename)

echo rocksample iw-bel2
(cd rocksample; ./experiment.sh 8 6 1 10 iw-bel2 5 1 250 300 > results/$filename)
(cd rocksample; ./experiment.sh 8 6 1 10 iw-bel2 5 2 250 300 >> results/$filename)
(cd rocksample; ./experiment.sh 8 6 1 10 iw-bel2 5 3 250 300 >> results/$filename)
echo rocksample iw-bel3
(cd rocksample; ./experiment.sh 8 6 1 10 iw-bel3 3 1 250 300 >> results/$filename)
(cd rocksample; ./experiment.sh 8 6 1 10 iw-bel3 3 2 250 300 >> results/$filename)

echo rocksample_original iw-bel2
(cd rocksample_original; ./experiment.sh 8 6 10 iw-bel2 5 1 250 300 > results/$filename)
(cd rocksample_original; ./experiment.sh 8 6 10 iw-bel2 5 2 250 300 >> results/$filename)
(cd rocksample_original; ./experiment.sh 8 6 10 iw-bel2 5 3 250 300 >> results/$filename)
echo rocksample_original iw-bel3
(cd rocksample_original; ./experiment.sh 8 6 10 iw-bel3 3 1 250 300 >> results/$filename)
(cd rocksample_original; ./experiment.sh 8 6 10 iw-bel3 3 2 250 300 >> results/$filename)

