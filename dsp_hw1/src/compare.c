#include <stdio.h>
#include "hmm.h"


#define LINE_MAX	64

void usage(char *name)
{
	printf("Usage: %s <file1_path> <file2_path>\n", name);
}

int main(int argc, char **argv)
{
	char *path1, *path2;

	FILE *fp_path1;
	FILE *fp_path2;

	char line1[LINE_MAX];
	char line2[LINE_MAX];

	int count1 = 0;
	int count2 = 0;

	int eof = 0;
	int diff = 0;

	if (argc != 3) {
		usage(argv[0]);
		return 0;
	}

	path1 = argv[1];
	path2 = argv[2];

	fp_path1 = open_or_die(path1, "r");
	fp_path2 = open_or_die(path2, "r");

	while (1) {
		if (fgets(line1, LINE_MAX, fp_path1) != NULL)
			count1++;
		else
			eof = 1;

		if (fgets(line2, LINE_MAX, fp_path2) != NULL)
			count2++;
		else
			eof = 1;

		if (count1 != count2) {
			printf("ERROR: line count differents in two files\n");
			break;
		}

		if (eof != 0)
			break;

		/* compare the 'model_05.txt' string */
		if (strncmp(line1, line2, 12))
			diff++;
	}

	fclose(fp_path1);
	fclose(fp_path2);

	printf("%.5lf", (double)(count1-diff)/(double)count1);
	return 0;
}
