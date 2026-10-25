#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RULE_LEN 32
#define MIN_CELLS_FOR_PARALLEL 1000

/* Representação interna das células: 0 = morta, 1 = viva. */
typedef unsigned char cell_t;

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
 * Conta os vizinhos vivos da célula na posição 'pos'.
 *
 * Como as células valem 0 ou 1 e a grade tem borda morta,
 * o número de vizinhos vivos é simplesmente a soma dos 8 bytes
 * ao redor: sem comparações e sem testes de limite.
 *
 * stride = largura da linha na grade com padding (C + 2).
 */
static inline int count_neighbors(const cell_t *grid, int stride, int pos) {
    return grid[pos - stride - 1] + grid[pos - stride] + grid[pos - stride + 1]
         + grid[pos - 1]                               + grid[pos + 1]
         + grid[pos + stride - 1] + grid[pos + stride] + grid[pos + stride + 1];
}


/*
 * Calcula uma geração completa.
 *
 * current nunca é alterada durante o cálculo.
 * Todos os novos estados são escritos em next.
 *
 * rows e cols são as dimensões REAIS (L e C).
 * Os arrays current/next têm (rows+2) x (cols+2) posições.
 * rule_matrix continua com rows x cols (sem padding).
 *
 * Somente o interior de next é escrito; a moldura permanece morta (0).
 */
void next_generation(
    const cell_t *current,
    cell_t *next,
    const int *rule_matrix,
    const Rule *rules,
    int rows,
    int cols
) {
    const int stride = cols + 2;

    #pragma omp for
    for (int row = 0; row < rows; row++) {

        /* posição da célula (row, 0) dentro da grade com padding */
        const int base = (row + 1) * stride + 1;

        for (int col = 0; col < cols; col++) {

            const int pos = base + col;

            const int neighbors = count_neighbors(current, stride, pos);

            const Rule *rule = &rules[rule_matrix[row * cols + col]];

            /*
             * Viva  -> consulta a parte S da regra.
             * Morta -> consulta a parte B da regra.
             * Os vetores já guardam 0/1, que é o próximo estado.
             */
            next[pos] = current[pos] ? rule->survival[neighbors]
                                     : rule->birth[neighbors];
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
     * associada a cada posição (sem padding, L x C).
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

    /*
     * Grades com borda morta: (L+2) x (C+2).
     * A célula lógica (row, col) fica em (row+1)*stride + (col+1).
     */
    const int stride = C + 2;
    const size_t padded_cells = (size_t)(L + 2) * (size_t)stride;

    cell_t *current = malloc(padded_cells * sizeof(cell_t));
    cell_t *next = malloc(padded_cells * sizeof(cell_t));

    if (current == NULL || next == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");

        free(current);
        free(next);
        free(rule_matrix);
        free(rules);

        return 1;
    }

    /*
     * Inicializa AMBOS os buffers como mortos (0), garantindo a moldura
     * morta nos dois. next_generation só escreve no interior, então a
     * moldura permanece 0 mesmo após as trocas current <-> next.
     */
    memset(current, 0, padded_cells * sizeof(cell_t));
    memset(next, 0, padded_cells * sizeof(cell_t));

    /*
     * Como espaços são significativos no arquivo de entrada,
     * não podemos usar scanf("%s"). É necessário usar fgets.
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
         * Converte texto -> binário na entrada:
         * 'x' ou 'X' vira 1; qualquer outro caractere (ou posição
         * faltante em linha curta) permanece 0.
         */
        for (int col = 0; col < C; col++) {

            int pos = (row + 1) * stride + (col + 1);

            current[pos] = (line[col] == 'x' || line[col] == 'X') ? 1 : 0;
        }
    }

    free(line);

    /*
     * Executa as G gerações.
     *
     * A equipe OpenMP é criada uma vez e reutilizada em todas elas.
     * Ao final de cada geração, uma thread troca os ponteiros current e next.
     */
    #pragma omp parallel if((long long) L * C >= MIN_CELLS_FOR_PARALLEL)
    {
        for (int generation = 0; generation < G; generation++) {

            next_generation(
                current,
                next,
                rule_matrix,
                rules,
                L,
                C
            );

            #pragma omp single
            {
                cell_t *temp = current;
                current = next;
                next = temp;
            }
        }
    }

    /*
     * Converte binário -> texto na saída:
     * 1 vira 'x', 0 vira ' '.
     * Imprime exatamente L linhas de C caracteres (sem a moldura).
     */
    for (int row = 0; row < L; row++) {

        for (int col = 0; col < C; col++) {
            putchar(current[(row + 1) * stride + (col + 1)] ? 'x' : ' ');
        }

        putchar('\n');
    }

    free(current);
    free(next);
    free(rule_matrix);
    free(rules);

    return 0;
}