#include <mpi.h>
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2 || argc < 2) {
        if (rank == 0) std::cerr << "Запуск: mpirun --oversubscribe -np 2 ./v2_deadlock_find <msg_bytes>\n";
        MPI_Finalize();
        return 2;
    }

    const int m = std::stoi(argv[1]);
    const int partner = 1 - rank;
    std::vector<char> sbuf(m, 'S'), rbuf(m, 0);

    MPI_Send(sbuf.data(), m, MPI_CHAR, partner, 0, MPI_COMM_WORLD);
    MPI_Recv(rbuf.data(), m, MPI_CHAR, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == 0) std::cout << "OK " << m << " bytes: Send-Send-Recv-Recv завершився без тупика\n";
    MPI_Finalize();
    return 0;
}