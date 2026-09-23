#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RULE_LEN 32

typedef struct {
    int birth[9];
    int survival[9];
} Rule;

/*
 * Converte uma regra no formato B3/S23, B36/S23, etc.
 * para dois vetores booleanos:
 *
 * birth[n]    = 1 se uma célula morta nasce com n vizinhos
 * survival[n] = 1 se uma célula viva sobrevive com n vizinhos
 */
void parse_rule(const char *text, Rule *rule) {
    int i;

    for (i = 0; i <= 8; i++) {
        rule->birth[i] = 0;
        rule->survival[i] = 0;
    }

    int mode = 0; /* 1 = B, 2 = S */

    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] == 'B' || text[i] == 'b') {
            mode = 1;
        } else if (text[i] == 'S' || text[i] == 's') {
            mode = 2;
        } else if (text[i] >= '0' && text[i] <= '8') {
            int n = text[i] - '0';

            if (mode == 1) {
                rule->birth[n] = 1;
            } else if (mode == 2) {
                rule->survival[n] = 1;
            }
        }
    }
}


/*
 * Conta os vizinhos vivos da célula (row, col).
 *
 * A grade não é toroidal:
 * posições fora da matriz são simplesmente ignoradas.
 */
int count_neighbors(
    const char *grid,
    int rows,
    int cols,
    int row,
    int col
) {
    int count = 0;

    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {

            if (dr == 0 && dc == 0) {
                continue;
            }

            int nr = row + dr;
            int nc = col + dc;

            if (nr >= 0 && nr < rows &&
                nc >= 0 && nc < cols) {

                if (grid[nr * cols + nc] == 'x') {
                    count++;
                }
            }
        }
    }

    return count;
}


/*
 * Calcula uma geração completa.
 *
 * current nunca é alterada durante o cálculo.
 * Todos os novos estados são escritos em next.
 *
 * Isso garante a evolução síncrona.
 */
void next_generation(
    const char *current,
    char *next,
    const int *rule_matrix,
    const Rule *rules,
    int rows,
    int cols
) {
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {

            int pos = row * cols + col;

            int neighbors =
                count_neighbors(current, rows, cols, row, col);

            int rule_id = rule_matrix[pos];

            if (current[pos] == 'x') {
                /*
                 * Célula atualmente viva.
                 * Consulta a parte S da regra.
                 */
                if (rules[rule_id].survival[neighbors]) {
                    next[pos] = 'x';
                } else {
                    next[pos] = ' ';
                }

            } else {
                /*
                 * Célula atualmente morta.
                 * Consulta a parte B da regra.
                 */
                if (rules[rule_id].birth[neighbors]) {
                    next[pos] = 'x';
                } else {
                    next[pos] = ' ';
                }
            }
        }
    }
}


int main(void) {

    int L, C, G;

    if (scanf("%d %d %d", &L, &C, &G) != 3) {
        fprintf(stderr, "Erro ao ler L, C e G.\n");
        return 1;
    }

    int R;

    if (scanf("%d", &R) != 1) {
        fprintf(stderr, "Erro ao ler o numero de regras.\n");
        return 1;
    }

    Rule *rules = malloc(R * sizeof(Rule));

    if (rules == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");
        return 1;
    }

    /*
     * Leitura das regras.
     *
     * Exemplo:
     * B3/S23
     * B36/S23
     */
    for (int i = 0; i < R; i++) {
        char rule_text[MAX_RULE_LEN];

        if (scanf("%31s", rule_text) != 1) {
            fprintf(stderr, "Erro ao ler regra %d.\n", i);
            free(rules);
            return 1;
        }

        parse_rule(rule_text, &rules[i]);
    }

    int total_cells = L * C;

    int *rule_matrix = malloc(total_cells * sizeof(int));

    if (rule_matrix == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");
        free(rules);
        return 1;
    }

    /*
     * Matriz contendo o identificador da regra
     * associada a cada posição.
     */
    for (int row = 0; row < L; row++) {
        for (int col = 0; col < C; col++) {

            int pos = row * C + col;

            if (scanf("%d", &rule_matrix[pos]) != 1) {
                fprintf(stderr, "Erro ao ler matriz de regras.\n");
                free(rule_matrix);
                free(rules);
                return 1;
            }

            if (rule_matrix[pos] < 0 ||
                rule_matrix[pos] >= R) {

                fprintf(stderr,
                        "Identificador de regra invalido: %d\n",
                        rule_matrix[pos]);

                free(rule_matrix);
                free(rules);
                return 1;
            }
        }
    }

    /*
     * Remove o '\n' que ficou depois da leitura
     * da matriz de inteiros.
     */
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF) {
    }

    char *current = malloc(total_cells * sizeof(char));
    char *next = malloc(total_cells * sizeof(char));

    if (current == NULL || next == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");

        free(current);
        free(next);
        free(rule_matrix);
        free(rules);

        return 1;
    }

    /*
     * Como espaços são significativos, não podemos usar scanf("%s").
     *
     * É necessário usar fgets.
     */
    char *line = malloc((C + 3) * sizeof(char));

    if (line == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");

        free(current);
        free(next);
        free(rule_matrix);
        free(rules);

        return 1;
    }

    for (int row = 0; row < L; row++) {

        if (fgets(line, C + 3, stdin) == NULL) {
            fprintf(stderr, "Erro ao ler a grade inicial.\n");

            free(line);
            free(current);
            free(next);
            free(rule_matrix);
            free(rules);

            return 1;
        }

        /*
         * Copia exatamente C posições.
         *
         * Caso haja alguma linha menor por algum motivo,
         * as posições faltantes são consideradas mortas.
         */
        for (int col = 0; col < C; col++) {

            if (line[col] == 'x' || line[col] == 'X') {
                current[row * C + col] = 'x';
            } else {
                current[row * C + col] = ' ';
            }
        }
    }

    free(line);

    /*
     * Executa as G gerações.
     *
     * Ao final de cada geração simplesmente trocamos
     * os ponteiros current e next.
     *
     * Assim não é necessário copiar toda a matriz.
     */
    for (int generation = 0; generation < G; generation++) {

        next_generation(
            current,
            next,
            rule_matrix,
            rules,
            L,
            C
        );

        char *temp = current;
        current = next;
        next = temp;
    }

    /*
     * Imprime exatamente L linhas,
     * cada uma contendo C posições.
     */
    for (int row = 0; row < L; row++) {

        for (int col = 0; col < C; col++) {
            putchar(current[row * C + col]);
        }

        putchar('\n');
    }

    free(current);
    free(next);
    free(rule_matrix);
    free(rules);

    return 0;
}
