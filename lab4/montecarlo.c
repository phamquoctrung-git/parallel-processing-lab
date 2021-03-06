#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "mpi.h"
#include <unistd.h>

#define PI 3.14159265358979

MPI_Status status;

int     n, s, S, rank, num_processes, point_per_work;
struct  timeval tic, toc, seed_time;

// ham calPi se tinh va tra ve so diem nam trong vong tron, voi iter la so diem dau vao
int calPI(long iter, int rank) {
    int numPi;
    unsigned int seed = rand()%32767;
    uint64_t i = 0;
    for(i; i < iter; ++i) {
        // Tao diem
        double x = (double)rand_r(&seed)/(double)RAND_MAX;
        double y = (double)rand_r(&seed)/(double)RAND_MAX;
        // tinh ban kinh
        double r = sqrt(x*x + y*y);
        // Is it in a circle
        if(r <= 1) numPi=numPi+1;
    }
    return numPi;
}

void slave_io () {
    
    char slave_name[256];
    gethostname(slave_name, 256);
    
    printf("Slave\tProcess #%d\t[%s]\n", rank, slave_name);
    
    int computations = 0;
    
    while(1) { // While true...
        
        int seed;
        
        // Get the seed from the master (or kill msg)
        MPI_Recv(&seed, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        if(status.MPI_TAG == 0) { // Kill slave process if TAG == 0
            break;
        }
        else if(status.MPI_TAG == 200) { // Compute number of points in circle if TAG == 200
            int count = 0;
            // lay so diem trong vong tron ma process tinh duoc gan vao bien Count
            count = calPI(n/s, rank);
            // gui gia tri count den MASTER
            MPI_Send(&count, 1, MPI_INT, 0, 300, MPI_COMM_WORLD);
//            computations++;
            
        } // End if/else block...
        
    } // End while loop
    
//    // Send total computations back to the master
//    MPI_Send(&computations, 1, MPI_INT, 0, 400, MPI_COMM_WORLD);
    
} // End slave_io()


void master_io () {
    
    char master_name[256];
    gethostname(master_name, 256);
    
    printf("Master\tProcess #%d\t[%s]\n", rank, master_name);
    
    int count = 0, i = 0;
    int computations[num_processes];
    
    gettimeofday(&tic, NULL); // Get start time
    
    // neu so cong viec can phan chia nho hon process thi gan gia tri min cho s - number of work in workpool
    int min = num_processes - 1;
    if(s < num_processes) min = s;
    
    // phan phoi cong viec den cac process
    for(i = 1; i <= min; i++) {
        gettimeofday(&seed_time, NULL);
        srand(seed_time.tv_usec);
        int seed = rand();
        MPI_Send(&seed, 1, MPI_INT, i, 200, MPI_COMM_WORLD);
    }
    
    // thuc hien viec luu kq tra ve toi workpool da duoc thuc hien het tuong duong s = 0
    while(s > 0) {
        // moi lan nhan kq giam gia tri so cong viec xuong 1
        s--;
       
        // lay kq tra ve
        int value;
        MPI_Recv(&value, 1, MPI_INT, MPI_ANY_SOURCE, 300, MPI_COMM_WORLD, &status);
        count += value;
        
        gettimeofday(&seed_time, NULL);
        srand(seed_time.tv_usec);
        int seed = rand() % (RAND_MAX / 10) + 100000000;
        
        // neu process nao thuc hien xong ma cong viec van con thi gui den process do 1 tin hieu de thuc hien cong viec
        // TAG = 200 la thuc hien tiep cong viec
        if(s > 0) MPI_Send(&seed, 1, MPI_INT, status.MPI_SOURCE, 200, MPI_COMM_WORLD);
    } // ket thuc qua trinh nhan ket qua
    
    // lay gia tri thoi gian sau khi tinh toan xong
    gettimeofday(&toc, NULL); // Get end time
    
    // gui tin hieu de ket thuc tat ca process "TAG = 0"
    for(i = 1; i < num_processes; i++)
        MPI_Send(&i, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    
    // Lay so cong viec ma moi Process da tinh toan
//    for(i = 1; i < num_processes; i++) {
//        int value;
//        MPI_Recv(&value, 1, MPI_INT, i, 400, MPI_COMM_WORLD, &status);
//        computations[status.MPI_SOURCE] = value;
//    }
    
    
    // Compute final PI Value and display:
    double pi = count / ((double) (n)) * 4;
    
//    printf("N x S = %d\n", n * S);
    printf("PI Estimation: %f\n", pi);
    printf("Estimation Error: %f\n", fabs(PI - pi));
    
    // Print total elapsed time...
    double elapsed = (toc.tv_sec - tic.tv_sec) + ((toc.tv_usec - tic.tv_usec) / 1000000.0);
    printf("Time to Estimate PI: %fs\n", elapsed);
    
//    // Print the number of seeds each process handled...
//    printf("Seeds Computed: ");
//
//    for(i = 1; i < num_processes; i ++)
//    printf("[P%d] -> %d ", i, computations[i]);

    
}


int main (int argc, char *argv[]) {
    
    setbuf(stdout, NULL);
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
    
    // Must have atleast 2 processes for the workpool:
    if(num_processes < 2) {
        printf("You must utilize at least 2 processes for the workpool!\n");
        MPI_Finalize();
        return 0;
    }
    
    if(rank == 0) { // Master Process

        printf("Nhap tong so diem can tinh so Pi: ");
        scanf("%d", &n);
        
        printf("Nhap so cong viec can phan chia: ");
        scanf("%d", &S);
       
        s = S;
        
    }
    // gui gia tri tong so diem n va so cong viec phan chia den cho tat ca cac process con
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&s, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if(rank != 0)
        slave_io();
    else master_io();

    MPI_Finalize();
    return 0;
    
} // End main()
