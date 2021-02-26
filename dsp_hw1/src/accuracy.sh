#!/bin/bash

test_run() {
	local iter=${1}

	echo -e "\n${iter}\t\c"
	./train 1 model_01.txt data/train_seq_01.txt model_01.txt
	./train 1 model_02.txt data/train_seq_02.txt model_02.txt
	./train 1 model_03.txt data/train_seq_03.txt model_03.txt
	./train 1 model_04.txt data/train_seq_04.txt model_04.txt
	./train 1 model_05.txt data/train_seq_05.txt model_05.txt
	./test modellist.txt data/test_seq.txt result.txt
	./compare data/test_lbl.txt result.txt 

	return 0
}

main() {
	local iter=1
	local end=2000

	echo -e "1\t\c"
	./train 1 model_init.txt data/train_seq_01.txt model_01.txt
	./train 1 model_init.txt data/train_seq_02.txt model_02.txt
	./train 1 model_init.txt data/train_seq_03.txt model_03.txt
	./train 1 model_init.txt data/train_seq_04.txt model_04.txt
	./train 1 model_init.txt data/train_seq_05.txt model_05.txt
	./test modellist.txt data/test_seq.txt result.txt
	./compare data/test_lbl.txt result.txt 

	for ((iter=2; iter<=end; iter++)); do
		test_run ${iter}
	done

	return 0
}

# call main funcion
main "$@"
