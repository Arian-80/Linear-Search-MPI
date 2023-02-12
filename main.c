#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>

int getIntIndex(int* list, int listSize, int integerToFind) {
    int processorCount;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &processorCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (processorCount > listSize) {
        if (!rank) printf("More processors than elements in list. Aborting...\n");
        return -2;
    }

    int portion = listSize / processorCount;
    int remainder = listSize % processorCount;
    int start;
    int end;

    if (rank < remainder) {
        portion++;
        start = rank*portion;
    }
    else {
        start = (portion+1)*remainder + portion*(rank-remainder);
    }
    end = start + portion;

    int foundIndex = INT32_MAX;
    for (int i = start; i < end; i++) {
        if (list[i] == integerToFind) {
            foundIndex = i;
            break;
        }
    }

    if (!rank) {
        MPI_Reduce(MPI_IN_PLACE, &foundIndex, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
        return foundIndex == INT32_MAX ? -1 : foundIndex;
    }
    MPI_Reduce(&foundIndex, NULL, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    return -2;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int size = 1000000000;
    int* list = (int*) calloc(size, sizeof(int));
    list[597206541] = 45;
    int index;
    double start, end;
    start = MPI_Wtime();
    index = getIntIndex(list, size, 45);
    end = MPI_Wtime();
    if (index == -2) {
        MPI_Finalize();
        return 0;
    }
    else if (index == -1) {
        printf("Item not found.\n");
    }
    else printf("First instance of item found at index %d\n", index);
    printf("Time taken: %f\n", end - start);
    MPI_Finalize();
    return 0;
}
