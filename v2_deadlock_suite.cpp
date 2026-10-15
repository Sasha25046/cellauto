#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) {
            std::cout << "Запустіть на 2 процесах: mpirun --oversubscribe -np 2 ./v2_deadlock_suite\n";
        }
        MPI_Finalize();
        return 0;
    }

    int partner = (rank == 0) ? 1 : 0;

    std::vector<int> test_sizes = {1024, 4096, 16384, 32768, 65536, 131072, 262144, 524288, 1048576};

    if (rank == 0) {
        std::cout << "=== V2: Тестування передавання даних та оцінка розмірів ===" << std::endl;
    }

    for (int sz : test_sizes) {
        std::vector<char> sbuf(sz, 'X');
        std::vector<char> rbuf(sz, 0);

        MPI_Barrier(MPI_COMM_WORLD);
        double t_start = MPI_Wtime();

        MPI_Request reqs[2];
        MPI_Irecv(rbuf.data(), sz, MPI_CHAR, partner, 100, MPI_COMM_WORLD, &reqs[0]);
        MPI_Isend(sbuf.data(), sz, MPI_CHAR, partner, 100, MPI_COMM_WORLD, &reqs[1]);
        MPI_Waitall(2, reqs, MPI_STATUSES_IGNORE);

        double elapsed = MPI_Wtime() - t_start;
        if (rank == 0) {
            std::cout << "Розмір: " << std::setw(8) << sz << " байтів (" 
                      << std::setw(6) << (sz / 1024.0) << " КБ) -> Успішно, час: " 
                      << std::fixed << std::setprecision(6) << elapsed << " с" << std::endl;
        }
    }

    int bench_size = 1048576; // 1 МБ
    int iters = 100;
    std::vector<char> s_up(bench_size, 'A');
    std::vector<char> r_up(bench_size, 0);

    MPI_Barrier(MPI_COMM_WORLD);
    double t_a_start = MPI_Wtime();
    for (int i = 0; i < iters; ++i) {
        if (rank % 2 == 0) {
            MPI_Send(s_up.data(), bench_size, MPI_CHAR, partner, 1, MPI_COMM_WORLD);
            MPI_Recv(r_up.data(), bench_size, MPI_CHAR, partner, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Recv(r_up.data(), bench_size, MPI_CHAR, partner, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(s_up.data(), bench_size, MPI_CHAR, partner, 2, MPI_COMM_WORLD);
        }
    }
    double t_method_a = MPI_Wtime() - t_a_start;

    MPI_Barrier(MPI_COMM_WORLD);
    double t_b_start = MPI_Wtime();
    for (int i = 0; i < iters; ++i) {
        MPI_Request r[2];
        MPI_Irecv(r_up.data(), bench_size, MPI_CHAR, partner, 10, MPI_COMM_WORLD, &r[0]);
        MPI_Isend(s_up.data(), bench_size, MPI_CHAR, partner, 10, MPI_COMM_WORLD, &r[1]);
        MPI_Waitall(2, r, MPI_STATUSES_IGNORE);
    }
    double t_method_b = MPI_Wtime() - t_b_start;

    if (rank == 0) {
        std::cout << "\n=== Порівняння способів усунення тупика (1 МБ, 100 ітерацій) ===" << std::endl;
        std::cout << "Спосіб А (Упорядкування Even/Odd Send-Recv): " << std::fixed << std::setprecision(5) << t_method_a << " с" << std::endl;
        std::cout << "Спосіб В (Неблокувальний Isend/Irecv):        " << std::fixed << std::setprecision(5) << t_method_b << " с" << std::endl;
        std::cout << "Прискорення способу В над А:                 " << std::fixed << std::setprecision(2) << (t_method_a / t_method_b) << "x" << std::endl;
    }

    MPI_Finalize();
    return 0;
}