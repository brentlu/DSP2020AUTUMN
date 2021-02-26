#include <stdio.h>
#include "hmm.h"


#define MAX_MODEL 100

typedef struct {
	double delta[MAX_MODEL][MAX_STATE][MAX_SEQ];

	double p_star[MAX_MODEL];

	int seq_num;
	int sequence[MAX_SEQ];
} TEST_SEQ;

HMM hmms[MAX_MODEL]; /* around 1.6MB */

void print_result(FILE *fp, TEST_SEQ *test, HMM *hmms, int model_num)
{
	int max_m;
	double max_p_star;

	max_m = 0;
	max_p_star = test->p_star[0];

	for (int m = 1; m < model_num; m++) {
		if (test->p_star[m] > max_p_star) {
			max_p_star = test->p_star[m];
			max_m = m;
		}
	}

	fprintf(fp, "%s %e\n", hmms[max_m].model_name, test->p_star[max_m]);
}

void update_p_star(TEST_SEQ *test, HMM *hmms, int model_num)
{
	double tmp, max;

	for (int m = 0; m < model_num; m++) {
		max = 0.0;
		for (int i = 0; i < hmms[m].state_num; i++) {
			tmp = test->delta[m][i][test->seq_num-1];
			if (tmp > max)
				max = tmp;
		}

		test->p_star[m] = max;
	}
}

void update_delta_matrix(TEST_SEQ *test, HMM *hmms, int model_num)
{
	double tmp, max;

	for (int m = 0; m < model_num; m++) {
		for (int i = 0; i < hmms[m].state_num; i++) {
			for (int j = 0; j < test->seq_num; j++) {
				test->delta[m][i][j] = 0.0;
			}
		}

		/* initialization */
		for (int i = 0; i < hmms[m].state_num; i++) {
			test->delta[m][i][0] = hmms[m].initial[i]*hmms[m].observation[test->sequence[0]][i];
		}

		/* induction */
		for (int t = 1; t < test->seq_num; t++) {
			for (int j = 0; j < hmms[m].state_num; j++) {
				max = 0.0;
				for (int i = 0; i < hmms[m].state_num; i++) {
					tmp = test->delta[m][i][t-1]*hmms[m].transition[i][j];
					if (tmp > max)
						max = tmp;
				}

				test->delta[m][j][t] = max*hmms[m].observation[test->sequence[t]][j];
			}
		}
	}
}

void usage(char *name)
{
	printf("Usage: %s <models_list_path> <seq_path> <output_result_path>\n", name);
}

int main(int argc, char **argv)
{
	char *models_list_path;
	char *seq_path;
	char *output_result_path;

	TEST_SEQ test;

	FILE *fp_seq;
	FILE *fp_output_result;

	int model_num;
	char sequence[MAX_SEQ+1];

	if (argc != 4) {
		usage(argv[0]);
		return 0;
	}

	models_list_path = argv[1];
	seq_path = argv[2];
	output_result_path = argv[3];

	model_num = load_models(models_list_path, hmms, MAX_MODEL);

	if (model_num == 0) {
		printf("fail to open models\n");
		return 0;
	}

	fp_seq = open_or_die(seq_path, "r");
	fp_output_result = open_or_die(output_result_path, "w");

	while (fgets(sequence, MAX_SEQ+1, fp_seq) != NULL) {
		test.seq_num = strlen(sequence);

		if (sequence[test.seq_num-1] == '\n') {
			test.seq_num--;
		}

		for (int i = 0; i < test.seq_num; i++)
			test.sequence[i] = sequence[i] - 'A';

		update_delta_matrix(&test, hmms, model_num);

		update_p_star(&test, hmms, model_num);

		print_result(fp_output_result, &test, hmms, model_num);
	}

	fclose(fp_seq);
	fclose(fp_output_result);

	return 0;
}
