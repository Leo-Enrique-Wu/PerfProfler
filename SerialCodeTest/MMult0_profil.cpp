// + Experiment with different optimization levels from -O0 to -O3 and report
//   the flop-rate and the bandwidth observed on your machine.
// + Specify the the compiler version (using the command: "g++ -v")
// + Try to find out the frequency, the maximum flop-rate and the maximum main
//   memory bandwidth for your processor.
// $ g++ -O3 -std=c++11 MMult0.cpp && ./a.out

#include <stdio.h>
#include <papi.h>

#include "utils.h"
#include "prof_utils.h"

void handle_error (int retval)
{
     printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
     exit(1);
}

// Note: matrices are stored in column major order; i.e. the array elements in
// the (m x n) matrix C are stored in the sequence: {C_00, C_10, ..., C_m0,
// C_01, C_11, ..., C_m1, C_02, ..., C_0n, C_1n, ..., C_mn}
void MMult0( long m, long n, long k, double *a,
                                     double *b,
                                     double *c) {
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      for (int p = 0; p < k; p++) {
        
        // mop: 3(load a[], b[], c[])
        double A_ip = a[i+p*m];
        double B_pj = b[p+j*k];
        double C_ij = c[i+j*m];
        
        // flop: 2
        C_ij = C_ij + A_ip * B_pj;
        
        // mop: 1(save c[])
        c[i+j*m] = C_ij;
        
      }
    }
  }
}

int main(int argc, char** argv) {
    
  const long NREPEATS = 50;
  /*
  const long PFIRST = 20;
  const long PLAST = 600;
  const long PINC = 20;
  */

  printf(" Dimension       Time    Gflop/s       GB/s\n");
  // for (long p = PFIRST; p < PLAST; p += PINC) {
    
    long p = 100;
    long m = p, n = p, k = p;
    
    // alloc memory
    double* a = (double*) malloc(m * k * sizeof(double)); // m x k
    double* b = (double*) malloc(k * n * sizeof(double)); // k x n
    double* c = (double*) malloc(m * n * sizeof(double)); // m x n

    // Initialize matrices
    for (long i = 0; i < m*k; i++) a[i] = drand48();
    for (long i = 0; i < k*n; i++) b[i] = drand48();
    for (long i = 0; i < m*n; i++) c[i] = drand48();

    int retval;
    int EventSet = PAPI_NULL;
    long_long values[1];
    long length;
    caddr_t start, end;
    const PAPI_exe_info_t *prginfo;
    unsigned short *profbuf;
    unsigned long plength;
    unsigned scale;
    int bucket;
    unsigned long blength;
    int num_buckets;
    
    /* Initialize the PAPI library */
    retval = PAPI_library_init(PAPI_VER_CURRENT);
    if (retval != PAPI_VER_CURRENT & retval > 0) {
        fprintf(stderr,"PAPI library version mismatch!");
        exit(1);
    }
    
    if (retval < 0)
        handle_error(retval);

    if ((prginfo = PAPI_get_executable_info()) == NULL)
        handle_error(1);
    
    PAPI_address_map_t address_info = prginfo->address_info;
    start = address_info.text_start;
    end = address_info.text_end;
    length = end - start;
    
    scale = FULL_SCALE;
    bucket = PAPI_PROFIL_BUCKET_16;
    plength = (unsigned long)length;
    blength = prof_size( plength, scale, bucket, &num_buckets );
    profbuf = (unsigned short *)malloc(length*sizeof(unsigned short));
    
    if (profbuf == NULL)
        handle_error(1);
        
    memset(profbuf,0x00,length*sizeof(unsigned short));
    
    if (PAPI_create_eventset(&EventSet) != PAPI_OK)
        handle_error(retval);
        
    /* Add Total FP Instructions Executed to our EventSet */
    if (PAPI_add_event(EventSet, PAPI_FP_INS) != PAPI_OK)
        handle_error(retval);
    
    if (PAPI_profil(profbuf, length, start, 65536, EventSet, PAPI_FP_INS, 1000000, PAPI_PROFIL_POSIX | PAPI_PROFIL_BUCKET_16) != PAPI_OK)
        handle_error(1);
    
    /* Start counting */
    if (PAPI_start(EventSet) != PAPI_OK)
        handle_error(1);

    Timer t;
    t.tic();
    
    // do function MMult0 for @NREPEATS@ times
    for (long rep = 0; rep < NREPEATS; rep++) {
      MMult0(m, n, k, a, b, c);
    }
    
    double time = t.toc(); // unit: second
    
    /* Stop the counting of events in the Event Set */
    if (PAPI_stop(EventSet, values) != PAPI_OK)
        handle_error(1);
    
    double flops = (((2 * m * n * k) * NREPEATS) / 1e9) / time;
    double bandwidth = (((4 * m * n * k) * NREPEATS * sizeof(double)) / 1e9) / time;
    printf("%10ld %10f %10f %10f\n", p, time, flops, bandwidth);
    
    prof_head( blength, bucket, num_buckets,
				   "address\t\t\tflat\trandom\tweight\tcomprs\tall\n" );
	  prof_out( start, 1, bucket, num_buckets, scale );
	  retval = prof_check( 1, bucket, num_buckets );
	  
	  if (retval < 0)
        handle_error(retval);

    free(a);
    free(b);
    free(c);
    
    free(profbuf);
    
  // }

  return 0;
    
}

