#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>


int getIntIndex_speed(const int* intListToSearch, int listSize, int integerToFind) {
    /*
     * Prioritises speed over memory usage. If the list is smaller than..-
     * -.. or equal to INT_MAX, then the entire list is replicated..-
     * -.. in the memory of each processor.
     * This significantly improves speed but also increases total..-
     * -.. memory consumption across all processors.
     */
    int processorCount;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &processorCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (!rank) {
        if (listSize > INT_MAX) { // Overflow occurred
            printf("List is too large. Try dividing the list into multiple lists.\n");
            MPI_Abort(MPI_COMM_WORLD, -1);
            return -2;
        }
    }

    if (processorCount > listSize) {
        if (!rank) printf("More processors than items in list. Aborting...\n");
        return -2;
    }

    int portion = (int) (listSize / processorCount);
    int remainder = (int) (listSize % processorCount);
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

    int foundIndex = INT_MAX;
    for (int i = start; i < end; i++) {
        if (intListToSearch[i] == integerToFind) {
            foundIndex = i;
            break;
        }
    }

    if (!rank) {
        MPI_Reduce(MPI_IN_PLACE, &foundIndex, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
        return foundIndex == MPI_INT ? -1 : foundIndex;
    }
    MPI_Reduce(&foundIndex, NULL, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD);
    return -2;
}

long long getIntIndex_memory(
        int* intListToSearch, long long listSize, int integerToFind) {
    /*
     * Prioritises memory usage over speed. If the list is larger than..-
     * -.. INT_MAX, then the list is split across processors.
     * This significantly improves memory usage but also decreases speed.
     * This variation is substantially more scalable than the other variation.
     */
    if (listSize <= INT_MAX)
        return getIntIndex_speed(intListToSearch, (int) listSize, integerToFind);

    int processorCount;
    int rank;
    MPI_Comm_size(MPI_COMM_WORLD, &processorCount);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

        if (listSize > LONG_LONG_MAX) { // Overflow occurred
            if (!rank) printf("List is too large. Try dividing the list "
                              "into multiple lists.\n");
            return -2;
        } else if (listSize > (long long) INT_MAX * processorCount) {
            if (!rank) printf("List is too large for the provided number of "
                              "processors.\nTry adding more processors"
                              " or dividing the list.\n");
            return -2;
        }

    if (processorCount > listSize) {
        if (!rank) printf("More processors than items in list. Aborting...\n");
        return -2;
    }

    int portion = (int) (listSize / processorCount);
    int remainder = (int) (listSize % processorCount);

    /*
    // Compute counts and displacements for MPI_Scatterv.
    int sendcounts[processorCount];
    int scatterDispls[processorCount]; // Displacement for Scatterv
    if (!rank) { // Only rank 0 manages the global communications
        int currScatterDispls = 0;
        // Processes dealing with remainders have additional counts and displs
        for (int i = 0; i < remainder; i++) {
            scatterDispls[i] = currScatterDispls;
            sendcounts[i] = portion+1;
            currScatterDispls += portion+1;
        }
        for (int i = remainder; i < processorCount; i++) {
            scatterDispls[i] = currScatterDispls;
            sendcounts[i] = portion;
            currScatterDispls += portion;
        }
    }
    */

    if (rank < remainder) {
        portion++;
    }

    if (rank) {
        long long shiftBy = (long long) ((portion+1)*remainder) +
                (long long) portion*(rank-remainder); // Start index
        // memmove instead of memcpy as data overlaps and memcpy => undefined behaviour
        memmove(intListToSearch, intListToSearch+shiftBy,
                (size_t) portion * sizeof(int));

        intListToSearch = (int*) realloc(
                intListToSearch, (size_t) sizeof(int) * portion);

        if (intListToSearch == NULL) {
            MPI_Abort(MPI_COMM_WORLD, -1);
            return -2;
        }
//        MPI_Scatterv(intListToSearch, sendcounts, scatterDispls, MPI_INT,intListToSearch,
//                     1500000000, MPI_INT, 0, MPI_COMM_WORLD);
    }
    else {
//        MPI_Scatterv(intListToSearch, sendcounts, scatterDispls, MPI_INT,
//                     MPI_IN_PLACE, 1500000000, MPI_INT, 0, MPI_COMM_WORLD);
        intListToSearch = (int*) realloc(
                intListToSearch,(size_t) sizeof(int) * portion);
        if (intListToSearch == NULL) {
            MPI_Abort(MPI_COMM_WORLD, -1);
            return -2;
        }
    }

    long long foundIndex = LONG_LONG_MAX;
    for (int i = 0; i < portion; i++) {
        if (intListToSearch[i] == integerToFind) {
            foundIndex = (long long) portion * rank + (long long) i;
            break;
        }
    }

    if (!rank) {
        MPI_Reduce(MPI_IN_PLACE, &foundIndex, 1, MPI_LONG_LONG,
                   MPI_MIN, 0, MPI_COMM_WORLD);
        return foundIndex == LONG_LONG_MAX ? -1 : foundIndex;
    }
    MPI_Reduce(&foundIndex, NULL, 1, MPI_LONG_LONG, MPI_MIN, 0, MPI_COMM_WORLD);
    return -2;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    long long size = 1000000000;
    int* list = (int*) calloc((size_t) size, sizeof(int));
    if (list == NULL) {
        MPI_Finalize();
        return -1;
    }
    list[954554845] = 45;
    long long index;
    double start, end;
    start = MPI_Wtime();
    index = getIntIndex_memory(list, size, 45);
    end = MPI_Wtime();
    if (index == -2) {
        free(list);
        MPI_Finalize();
        return 0;
    }
    else if (index == -1) {
        printf("Item not found.\n");
    }
    else printf("First instance of item found at index %lli\n", index);
    printf("Time taken: %f\n", end - start);
    free(list);
    MPI_Finalize();
    return 0;
}
