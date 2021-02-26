#include <stdio.h>
#include <time.h>
#include "hmm.h"


typedef struct {
	double alpha[MAX_STATE][MAX_SEQ];
	double beta[MAX_STATE][MAX_SEQ];
	double gamma[MAX_STATE][MAX_SEQ];
	double epsilon[MAX_SEQ-1][MAX_STATE][MAX_STATE];

	int seq_num;
	int sequence[MAX_SEQ];
} TRAIN_SEQ;

typedef struct{
	double initial[MAX_STATE];
	double transition_a[MAX_STATE][MAX_STATE];
	double transition_b[MAX_STATE][MAX_STATE];
	double observation_a[MAX_OBSERV][MAX_STATE];
	double observation_b[MAX_OBSERV][MAX_STATE];
} UPDATE_HMM;

void update_hmm_matrices(HMM *hmm, UPDATE_HMM *update, int train_num)
{
	for (int i = 0; i < hmm->state_num; i++) {
		hmm->initial[i] = update->initial[i] / train_num;
	}

	for (int i = 0; i < hmm->state_num; i++) {
		for (int j = 0; j < hmm->state_num; j++) {
			hmm->transition[i][j] = update->transition_a[i][j] / update->transition_b[i][j];
		}
	}

	for (int k = 0; k < MAX_OBSERV; k++) {
		for (int i = 0; i < hmm->state_num; i++) {
			hmm->observation[k][i] = update->observation_a[k][i] / update->observation_b[k][i];
		}
	}
}

void update_update_matrices(UPDATE_HMM *update, TRAIN_SEQ *train, HMM *hmm)
{
	for (int i = 0; i < hmm->state_num; i++) {
		update->initial[i] += train->gamma[i][0];
	}

	for (int t = 0; t < train->seq_num-1; t++) {
		for (int i = 0; i < hmm->state_num; i++) {
			for (int j = 0; j < hmm->state_num; j++) {
				update->transition_a[i][j] += train->epsilon[t][i][j];
				update->transition_b[i][j] += train->gamma[i][t];
			}
		}
	}

	for (int t = 0; t < train->seq_num; t++) {
		for (int k = 0; k < MAX_OBSERV; k++) {
			for (int i = 0; i < hmm->state_num; i++) {
				if (train->sequence[t] == k)
					update->observation_a[k][i] += train->gamma[i][t];
				update->observation_b[k][i] += train->gamma[i][t];
			}
		}
	}
}

void update_alpha_matrix(TRAIN_SEQ *train, HMM *hmm)
{
	for (int i = 0; i < hmm->state_num; i++) {
		for (int j = 0; j < train->seq_num; j++) {
			train->alpha[i][j] = 0.0;
		}
	}

	/* initialization */
	for (int i = 0; i < hmm->state_num; i++)
		train->alpha[i][0] = hmm->initial[i]*hmm->observation[train->sequence[0]][i];

	/* induction */
	for (int t = 1; t < train->seq_num; t++) {
		for (int j = 0; j < hmm->state_num; j++) {

			for (int i = 0; i < hmm->state_num; i++)
				train->alpha[j][t] += train->alpha[i][t-1]*hmm->transition[i][j];

			train->alpha[j][t] *= hmm->observation[train->sequence[t]][j];
		}
	}
}

void update_beta_matrix(TRAIN_SEQ *train, HMM *hmm)
{
	for (int i = 0; i < hmm->state_num; i++) {
		for (int j = 0; j < train->seq_num; j++) {
			train->beta[i][j] = 0.0;
		}
	}

	/* initialization */
	for (int i = 0; i < hmm->state_num; i++)
		train->beta[i][train->seq_num-1] = 1.0;

	/* induction */
	for (int t = train->seq_num-2; t >= 0; t--) {
		for (int i = 0; i < hmm->state_num; i++){
			for (int j = 0; j < hmm->state_num; j++) {
				train->beta[i][t] += hmm->transition[i][j]*hmm->observation[train->sequence[t+1]][j]*train->beta[j][t+1];
			}
		}
	}
}

void update_gamma_matrix(TRAIN_SEQ *train, HMM *hmm)
{
	double tmp = 0.0;

	for (int i = 0; i < hmm->state_num; i++) {
		for (int j = 0; j < train->seq_num; j++) {
			train->gamma[i][j] = 0.0;
		}
	}

	for (int t = 0; t < train->seq_num; t++) {
		for (int i = 0; i < hmm->state_num; i++){
			train->gamma[i][t] = train->alpha[i][t]*train->beta[i][t];
			tmp += train->gamma[i][t];
		}

		for (int i = 0; i < hmm->state_num; i++){
			train->gamma[i][t] /= tmp;
		}
		tmp = 0.0;
	}
}

void update_epsilon_matrix(TRAIN_SEQ *train, HMM *hmm)
{
	double tmp = 0.0;

	for (int i = 0; i < train->seq_num; i++) {
		for (int j = 0; j < hmm->state_num; j++) {
			for (int k = 0; k < hmm->state_num; k++) {
				train->epsilon[i][j][k] = 0.0;
			}
		}
	}

	for (int t = 0; t < train->seq_num-1; t++) {
		for (int i = 0; i < hmm->state_num; i++) {
			for (int j = 0; j < hmm->state_num; j++) {
				train->epsilon[t][i][j] = train->alpha[i][t]*hmm->transition[i][j]*hmm->observation[train->sequence[t+1]][j]*train->beta[j][t+1];
				tmp += train->epsilon[t][i][j];
			}
		}

		for (int i = 0; i < hmm->state_num; i++) {
			for (int j = 0; j < hmm->state_num; j++) {
				train->epsilon[t][i][j] /= tmp;
			}
		}
		tmp = 0.0;
	}
}

void update_train_matrices(TRAIN_SEQ *train, HMM *hmm)
{
	update_alpha_matrix(train, hmm);
	update_beta_matrix(train, hmm);
	update_gamma_matrix(train, hmm);
	update_epsilon_matrix(train, hmm);
}

void reset_update_matrices(UPDATE_HMM *update)
{
	for (int i = 0; i < MAX_STATE; i++)
		update->initial[i] = 0.0;

	for (int i = 0; i < MAX_STATE; i++) {
		for (int j = 0; j < MAX_STATE; j++) {
			update->transition_a[i][j] = 0.0;
			update->transition_b[i][j] = 0.0;
		}
	}

	for (int i = 0; i < MAX_OBSERV; i++) {
		for (int j = 0; j < MAX_STATE; j++) {
			update->observation_a[i][j] = 0.0;
			update->observation_b[i][j] = 0.0;
		}
	}
}

void usage(char *name)
{
	printf("Usage: %s <iter> <model_init_path> <seq_path> <output_model_path>\n", name);
}

int main(int argc, char **argv)
{
	clock_t start, end;
	double time_used;

	int iter;
	char *model_init_path;
	char *seq_path;
	char *output_model_path;

	HMM hmm_initial;
	TRAIN_SEQ train;
	UPDATE_HMM update;

	FILE *fp_output_model;

	char sequence[MAX_SEQ+1];
	int count = 0;

	start = clock();

	if (argc != 5) {
		usage(argv[0]);
		return 0;
	}

	iter = atoi(argv[1]);
	model_init_path = argv[2];
	seq_path = argv[3];
	output_model_path = argv[4];

	loadHMM(&hmm_initial, model_init_path);

	while (iter > 0) {
		FILE *fp_seq = open_or_die(seq_path, "r");

		reset_update_matrices(&update);

		count = 0;
		while (fgets(sequence, MAX_SEQ+1, fp_seq) != NULL) {

			train.seq_num = strlen(sequence);

			if (sequence[train.seq_num-1] == '\n') {
				train.seq_num--;
			}

			for (int i = 0; i < train.seq_num; i++)
				train.sequence[i] = sequence[i] - 'A';

			update_train_matrices(&train, &hmm_initial);

			update_update_matrices(&update, &train, &hmm_initial);

			count++;
			//printf("%d: %s\n", len, train);
		}

		update_hmm_matrices(&hmm_initial, &update, count);

		fclose(fp_seq);
		iter--;
	}

	/* save the model */
	fp_output_model = open_or_die(output_model_path, "w");
	dumpHMM(fp_output_model, &hmm_initial);
	fclose(fp_output_model);

	end = clock();
	time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

	printf("time used: %f\n", time_used);

	return 0;
}
