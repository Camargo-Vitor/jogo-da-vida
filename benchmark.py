import argparse
import os
from pathlib import Path
import statistics
import subprocess
import tempfile
import time


def compile_program(source, output, openmp=False):
    command = ["gcc", "-O3", str(source), "-o", str(output)]
    if openmp:
        command.insert(2, "-fopenmp")
    subprocess.run(command, check=True)


def run_program(program, input_data, environment):
    start = time.perf_counter()
    result = subprocess.run(
        [str(program)],
        input=input_data,
        stdout=subprocess.PIPE,
        check=True,
        env=environment,
    )
    return time.perf_counter() - start, result.stdout


def main():
    parser = argparse.ArgumentParser(
        description="Compara o tempo das versões sequencial e OpenMP."
    )
    parser.add_argument("entrada", type=Path, help="arquivo de entrada do jogo")
    parser.add_argument(
        "--repeticoes", type=int, default=5,
        help="número de execuções por versão (padrão: 5)",
    )
    parser.add_argument(
        "--threads", type=int, default=2,
        help="threads OpenMP (padrão: 2)",
    )
    args = parser.parse_args()

    if args.repeticoes < 1 or args.threads < 1:
        parser.error("repeticoes e threads devem ser maiores que zero")
    if not args.entrada.is_file():
        parser.error(f"arquivo não encontrado: {args.entrada}")

    folder = Path(__file__).resolve().parent
    input_data = args.entrada.read_bytes()

    with tempfile.TemporaryDirectory() as temporary_directory:
        temporary_directory = Path(temporary_directory)
        sequential = temporary_directory / "life"
        parallel = temporary_directory / "life_omp"

        compile_program(folder / "life.c", sequential)
        compile_program(folder / "life_omp.c", parallel, openmp=True)

        parallel_environment = os.environ.copy()
        parallel_environment["OMP_NUM_THREADS"] = str(args.threads)

        sequential_times = []
        parallel_times = []

        for _ in range(args.repeticoes):
            sequential_time, sequential_output = run_program(
                sequential, input_data, os.environ.copy()
            )
            parallel_time, parallel_output = run_program(
                parallel, input_data, parallel_environment
            )

            if sequential_output != parallel_output:
                raise SystemExit("Erro: as versões produziram saídas diferentes.")

            sequential_times.append(sequential_time)
            parallel_times.append(parallel_time)

    sequential_median = statistics.median(sequential_times)
    parallel_median = statistics.median(parallel_times)
    sequential_total = sum(sequential_times)
    parallel_total = sum(parallel_times)
    speedup = sequential_total / parallel_total

    print(f"Entrada: {args.entrada}")
    print(f"Repetições: {args.repeticoes}")
    print(f"Threads OpenMP: {args.threads}")
    print(f"Mediana sequencial: {sequential_median:.6f} s")
    print(f"Mediana OpenMP:     {parallel_median:.6f} s")
    print(f"Tempo total sequencial: {sequential_total:.6f} s")
    print(f"Tempo total OpenMP:     {parallel_total:.6f} s")
    print(f"Speedup total: {speedup:.2f}x")
    print("As saídas das duas versões são iguais.")


if __name__ == "__main__":
    main()