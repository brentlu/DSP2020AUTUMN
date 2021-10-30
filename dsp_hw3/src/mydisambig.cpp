#include <stdio.h>
#include "Ngram.h"

#define MAX_LINE_SIZE   (1024*1024)

typedef char word[6]; /* to fit </s> */

/* generic line buffer */
static char line[MAX_LINE_SIZE*3]; /* 3MB line buffer for 1M big5 characters */

/* ZhuYin-Big5 mapping */
struct mapping {
    word key;
    word *values;
    int value_num;

    struct mapping *next;
};

static struct mapping *my_map = NULL;

/* viterbi algorithm */
struct v_state {
    word value;
    double delta;
    struct v_state *prev; /* for backtrace */
};

struct v_time {
    struct v_state *states;
    int state_num;
};

static struct v_time my_viterbi[MAX_LINE_SIZE]; /* easier for backtrace */
static int v_time_num;

void viterbi_cleanup(void)
{
    int i;

    for (i = 0; i < MAX_LINE_SIZE; i++) {
        if (my_viterbi[i].states == NULL)
            break;

        free(my_viterbi[i].states);

        my_viterbi[i].states = NULL;
        my_viterbi[i].state_num = 0;
    }
}

void viterbi_output(FILE *fp, struct v_state *state)
{
    if (state->prev != NULL) {
        /* print previous state first */
        viterbi_output(fp, state->prev);

        fprintf(fp, " %s", state->value);
    } else
        fprintf(fp, "%s", state->value);
}

double ngram_prob2(Vocab& voc, Ngram& lm, char *word_1, char *word_2)
{
    VocabIndex idx_1 = voc.getIndex(word_1);
    VocabIndex idx_2 = voc.getIndex(word_2);

    if (idx_1 == Vocab_None)
        idx_1 = voc.getIndex(Vocab_Unknown);
    if (idx_2 == Vocab_None)
        idx_2 = voc.getIndex(Vocab_Unknown);

    VocabIndex context[] = {idx_1, Vocab_None};

    return lm.wordProb(idx_2, context);
}

double ngram_prob3(Vocab& voc, Ngram& lm, char *word_1, char *word_2, char *word_3)
{
    VocabIndex idx_1 = voc.getIndex(word_1);
    VocabIndex idx_2 = voc.getIndex(word_2);
    VocabIndex idx_3 = voc.getIndex(word_3);

    if (idx_1 == Vocab_None)
        idx_1 = voc.getIndex(Vocab_Unknown);
    if (idx_2 == Vocab_None)
        idx_2 = voc.getIndex(Vocab_Unknown);
    if (idx_3 == Vocab_None)
        idx_3 = voc.getIndex(Vocab_Unknown);

    VocabIndex context[] = {idx_2, idx_1, Vocab_None};

    return lm.wordProb(idx_3, context);
}

struct mapping *get_mapping(char *key)
{
    struct mapping *map = my_map;

    while (map) {
        if (!strncmp(map->key, key, 4))
            return map;

        map = map->next;
    }

    return NULL;
}

int viterbi_process(char *line, Vocab &voc, Ngram &lm, int order)
{
    char *token;
    int ret = 0;

    struct v_time *curr, *prev;
    struct mapping *map;

    double delta, max_delta;
    struct v_state *max_prev;

    v_time_num = 0;
    curr = &my_viterbi[v_time_num];

    /* fake a start symbol */
    curr->states = (struct v_state *)malloc(1 * sizeof(struct v_state));
    if (curr->states == NULL) {
        printf("fail to allocate memory\n");
        ret = -1;
        goto _exit;
    }
    curr->state_num = 1;

    strncpy(curr->states[0].value, "<s>", sizeof(word));
    curr->states[0].delta = 0; /* 100% */
    curr->states[0].prev = NULL;

    v_time_num++;
    curr = &my_viterbi[v_time_num];
    prev = &my_viterbi[v_time_num-1];

    /* start working */
    token = strtok(line, " ");

    while (token != 0) {
        if (strlen(token) >= sizeof(word)) {
            printf("token too large\n");
            ret = -1;
            goto _exit;
        }

        map = get_mapping(token);
        if (!map) {
            printf("fail to get mapping\n");
            ret = -1;
            goto _exit;
        }

        curr->states = (struct v_state *)malloc(map->value_num * sizeof(struct v_state));
        if (curr->states == NULL) {
            printf("fail to allocate memory\n");
            ret = -1;
            goto _exit;
        }
        curr->state_num = map->value_num;

        for (int i = 0; i < map->value_num; i++) {
            strncpy(curr->states[i].value, map->values[i], sizeof(word));

            for (int j = 0; j < prev->state_num; j++) {
                if (order == 2 || v_time_num < 2)
                    delta = prev->states[j].delta +
                            ngram_prob2(voc, lm,
                                        prev->states[j].value, /* previous character */
                                        curr->states[i].value);
                else
                    delta = prev->states[j].delta +
                            ngram_prob3(voc, lm,
                                        prev->states[j].prev->value,
                                        prev->states[j].value, /* previous character */
                                        curr->states[i].value);

                if ((j == 0) || delta > max_delta) {
                    max_prev = &prev->states[j];
                    max_delta = delta;
                }
            }

            curr->states[i].delta = max_delta;
            curr->states[i].prev = max_prev;
        }

        v_time_num++;
        curr = &my_viterbi[v_time_num];
        prev = &my_viterbi[v_time_num-1];

        /* next big-5 character */
        token = strtok(0, " \n");
    }

    /* fake the last symbol */
    curr->states = (struct v_state *)malloc(1 * sizeof(struct v_state));
    if (curr->states == NULL) {
        printf("fail to allocate memory\n");
        ret = -1;
        goto _exit;
    }
    curr->state_num = 1;

    strncpy(curr->states[0].value, "</s>", sizeof(word));

    for (int j = 0; j < prev->state_num; j++) {
        if (order == 2 || v_time_num < 2)
            delta = prev->states[j].delta +
                    ngram_prob2(voc, lm,
                                prev->states[j].value,
                                curr->states[0].value);
        else
            delta = prev->states[j].delta +
                    ngram_prob3(voc, lm,
                                prev->states[j].prev->value,
                                prev->states[j].value, /* previous character */
                                curr->states[0].value);

        if ((j == 0) || delta > max_delta) {
            max_prev = &prev->states[j];
            max_delta = delta;
        }
    }

    curr->states[0].delta = max_delta;
    curr->states[0].prev = max_prev;

    v_time_num++;

_exit:
    return ret;
}

int mapping_init(char *file_path)
{
    FILE *fp;
    char *token;
    struct mapping *map;
    int ret = 0, value_num, count;

    fp = fopen(file_path, "r");
    if (!fp) {
        printf("fail to open mapping file %s\n", file_path);
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != 0) {
        /* calculate the number of values for memory allocation */
        value_num = 0;
        for (int i = 0; i < sizeof(line); i++) {
            if (line[i] == ' ' || line[i] == '\n')
                value_num++;
        }

        token = strtok(line, " ");

        if (strlen(token) >= sizeof(word)) {
            printf("token too large\n");
            ret = -1;
            goto _exit;
        }

        map = (struct mapping *)malloc(sizeof(struct mapping));
        if (map == NULL) {
            printf("fail to allocate memory\n");
            ret = -1;
            goto _exit;
        }

        map->values = (word *)malloc(value_num * sizeof(word));
        if (map->values == NULL) {
            printf("fail to allocate memory\n");
            ret = -1;
            goto _exit;
        }

        strncpy(map->key, token, sizeof(word));
        count = 0;

        token = strtok(0, " \n"); /* space or new line */
        while (token != 0) {
            if (strlen(token) >= sizeof(word)) {
                printf("token too large\n");
                ret = -1;
                goto _exit;
            }

            if (count >= value_num) {
                printf("count exceeds value_num\n");
                ret = -1;
                goto _exit;
            }

            strncpy(map->values[count], token, sizeof(word));
            count++;

            token = strtok(0, " \n");
        }

        map->value_num = count;

        /* insert to head */
        if (my_map != NULL)
            map->next = my_map;
        my_map = map;
    }

_exit:
    if (fp)
        fclose(fp);

    return ret;
}

void usage(char *name)
{
    printf("Usage: %s <input file> <mapping> <model> <output file>\n", name);
}

int main(int argc, char *argv[])
{
    FILE *input_fp, *output_fp;
    int i, ret = 0;

    if (argc != 5) {
        usage(argv[0]);
        return 0;
    }

    /* copy from web page example */
    int ngram_order = 3;
    Vocab voc;
    Ngram lm(voc, ngram_order);

    /* read language model */
    {
        File lmFile(argv[3], "r");
        lm.read(lmFile);
        lmFile.close();
    }

    /* read ZhuYin-Big5 mapping file */
    ret = mapping_init(argv[2]);
    if (ret != 0) {
        printf("fail to init mapping table\n");
        return -1;
    }

    input_fp = fopen(argv[1], "r");

    if (!input_fp) {
        printf("fail to open input file %s\n", argv[1]);
        ret = -1;
        goto _exit;
    }

    output_fp = fopen(argv[4], "w");

    if (!output_fp) {
        printf("fail to open output file %s\n", argv[4]);
        ret = -1;
        goto _exit;
    }

    /* init the state machine */
    for (i = 0; i < MAX_LINE_SIZE; i++) {
        my_viterbi[i].states = NULL;
        my_viterbi[i].state_num = 0;
    }

    while (fgets(line, sizeof(line), input_fp) != 0) {
        /* run viterbi for each line */
        viterbi_process(line, voc, lm, ngram_order);

        /* output the result */
        viterbi_output(output_fp, &(my_viterbi[v_time_num-1].states[0]));
        fprintf(output_fp, "\n");

        /* release memory */
        viterbi_cleanup();
    }

_exit:
    if (input_fp)
        fclose(input_fp);
    if (output_fp)
        fclose(output_fp);

    return ret;
}
