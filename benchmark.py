import argparse
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import tempfile
import time


def compile_program(source, output, compiler, flags=()):
    command = [compiler, "-O3", *flags, str(source), "-o", str(output)]
    subprocess.run(command, check=True)


def run_program(command, input_data, environment):
    start = time.perf_counter()
    result = subprocess.run(
        command,
        input=input_data,
        stdout=subprocess.PIPE,
        check=True,
        env=environment,
    )
    return time.perf_counter() - start, result.stdout


def main():
    parser = argparse.ArgumentParser(
        description="Compara as versões sequencial, OpenMP e MPI."
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
    parser.add_argument(
        "--processos", type=int, default=2,
        help="processos MPI (padrão: 2)",
    )
    parser.add_argument(
        "--oversubscribe", action="store_true",
        help="permite ao Open MPI usar mais processos que slots disponíveis",
    )
    args = parser.parse_args()

    if args.repeticoes < 1 or args.threads < 1 or args.processos < 1:
        parser.error("repeticoes, threads e processos devem ser maiores que zero")
    if not args.entrada.is_file():
        parser.error(f"arquivo não encontrado: {args.entrada}")

    mpi_compiler = shutil.which("mpicc")
    mpi_launcher = shutil.which("mpiexec")
    if mpi_compiler is None or mpi_launcher is None:
        parser.error(
            "MPI não encontrado; instale um compilador MPI e o launcher "
            "(por exemplo, Open MPI: openmpi-bin e libopenmpi-dev)"
        )

    folder = Path(__file__).resolve().parent
    input_data = args.entrada.read_bytes()

    with tempfile.TemporaryDirectory() as temporary_directory:
        temporary_directory = Path(temporary_directory)
        sequential = temporary_directory / "life"
        openmp = temporary_directory / "life_omp"
        mpi = temporary_directory / "life_mpi"

        compile_program(folder / "life.c", sequential, "gcc")
        compile_program(folder / "life_omp.c", openmp, "gcc", ["-fopenmp"])
        compile_program(folder / "mpi.c", mpi, mpi_compiler)

        sequential_times = []
        openmp_times = []
        mpi_times = []

        mpi_command = [mpi_launcher]
        if args.oversubscribe:
            mpi_command.append("--oversubscribe")
        mpi_command.extend(["-n", str(args.processos), str(mpi)])
        mpi_environment = os.environ.copy()

        for _ in range(args.repeticoes):
            sequential_time, sequential_output = run_program(
                [str(sequential)], input_data, os.environ.copy()
            )
            openmp_time, openmp_output = run_program(
                [str(openmp), str(args.threads)], input_data, os.environ.copy()
            )
            mpi_time, mpi_output = run_program(
                mpi_command,
                input_data,
                mpi_environment,
            )

            if sequential_output != openmp_output:
                raise SystemExit(
                    "Erro: as versões sequencial e OpenMP produziram saídas diferentes."
                )
            if sequential_output != mpi_output:
                raise SystemExit(
                    "Erro: as versões sequencial e MPI produziram saídas diferentes."
                )

            sequential_times.append(sequential_time)
            openmp_times.append(openmp_time)
            mpi_times.append(mpi_time)

    sequential_median = statistics.median(sequential_times)
    openmp_median = statistics.median(openmp_times)
    mpi_median = statistics.median(mpi_times)
    sequential_total = sum(sequential_times)
    openmp_total = sum(openmp_times)
    mpi_total = sum(mpi_times)

    print(f"Entrada: {args.entrada}")
    print(f"Repetições: {args.repeticoes}")
    print(f"Threads OpenMP: {args.threads}")
    print(f"Processos MPI: {args.processos}")
    print(f"Mediana sequencial: {sequential_median:.6f} s")
    print(f"Mediana OpenMP:     {openmp_median:.6f} s")
    print(f"Mediana MPI:        {mpi_median:.6f} s")
    print(f"Tempo total sequencial: {sequential_total:.6f} s")
    print(f"Tempo total OpenMP:     {openmp_total:.6f} s")
    print(f"Tempo total MPI:        {mpi_total:.6f} s")
    print(f"Speedup OpenMP: {sequential_total / openmp_total:.3f}x")
    print(f"Speedup MPI:    {sequential_total / mpi_total:.3f}x")
    print("As saídas das três versões são iguais.")


if __name__ == "__main__":
    main()