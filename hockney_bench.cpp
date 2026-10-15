#include <mpi.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <fstream>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2) {
        if (rank == 0) {
            std::cerr << "Запустіть на 2 процесах: mpirun --oversubscribe -np 2 ./hockney_bench\n";
        }
        MPI_Finalize();
        return 1;
    }

    std::vector<int> sizes;
    for (int s = 8; s <= 4194304; s *= 2) {
        sizes.push_back(s);
    }

    int partner = (rank == 0) ? 1 : 0;
    const int N_ROUNDS = 500; 

    std::ofstream csv_file;
    if (rank == 0) {
        csv_file.open("hockney_data.csv");
        csv_file << "bytes,time_sec,bandwidth_MBps\n";
        std::cout << "=== Калібрування каналу (Модель Хокні) ===" << std::endl;
        std::cout << std::setw(12) << "Розмір (Б)" 
                  << std::setw(18) << "Час 1 обміну (с)" 
                  << std::setw(22) << "Пропускна здатність (МБ/с)" << std::endl;
    }

    for (int m : sizes) {
        std::vector<char> sbuf(m, 'a');
        std::vector<char> rbuf(m, 'b');

        for (int i = 0; i < 10; ++i) {
            if (rank == 0) {
                MPI_Send(sbuf.data(), m, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(rbuf.data(), m, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(rbuf.data(), m, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(sbuf.data(), m, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        double t_start = MPI_Wtime();

        for (int i = 0; i < N_ROUNDS; ++i) {
            if (rank == 0) {
                MPI_Send(sbuf.data(), m, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(rbuf.data(), m, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else {
                MPI_Recv(rbuf.data(), m, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(sbuf.data(), m, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }

        MPI_Barrier(MPI_COMM_WORLD);
        double total_time = MPI_Wtime() - t_start;
        double t_single = total_time / (2.0 * N_ROUNDS);
        double bandwidth_MBps = ((double)m / (1024.0 * 1024.0)) / t_single;

        if (rank == 0) {
            std::cout << std::setw(12) << m 
                      << std::setw(18) << std::fixed << std::setprecision(8) << t_single 
                      << std::setw(22) << std::fixed << std::setprecision(2) << bandwidth_MBps 
                      << std::endl;
            csv_file << m << "," << std::setprecision(9) << t_single << "," << std::setprecision(2) << bandwidth_MBps << "\n";
        }
    }

    if (rank == 0) {
        csv_file.close();
        std::cout << "\nРезультати збережено в hockney_data.csv" << std::endl;
    }

    MPI_Finalize();
    return 0;
}