# jogo-da-vida
Implementação da variação do "Jogo da vida" para disciplina INE5645

## Benchmark

O benchmark compara as versões sequencial, OpenMP e MPI. Instale um compilador
e um launcher MPI (por exemplo, `libopenmpi-dev` e `openmpi-bin` no Debian/Ubuntu)
e execute:

```bash
python3 benchmark.py exemplos_entradas/life-1.in --repeticoes 3 --threads 3 --processos 3 --oversubscribe
```

A opção `--oversubscribe` é específica do Open MPI e permite solicitar mais
processos do que os slots disponíveis. Use-a apenas quando necessário, pois
mais processos do que recursos disponíveis podem distorcer os tempos do benchmark.
Use `--threads` para configurar o número de threads OpenMP e `--processos` para
configurar o número de processos MPI.

Também é possível executar o binário OpenMP diretamente, passando o número de
threads como argumento e a entrada pelo stdin:

```bash
gcc -O3 -fopenmp life_omp.c -o life_omp
./life_omp 4 < exemplos_entradas/life-1.in
```
