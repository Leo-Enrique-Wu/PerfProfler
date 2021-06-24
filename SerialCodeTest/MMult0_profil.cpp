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

/* value for scale parameter that sets scale to 1 */
#define FULL_SCALE 65536

void handle_error (int retval)
{
     printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
     exit(1);
}

/* Given the profiling type (16, 32, or 64) this function returns the 
   bucket size in bytes. NOTE: the bucket size does not ALWAYS correspond
   to the expected value, esp on architectures like Cray with weird data types.
   This is necessary because the posix_profile routine in extras.c relies on
   the data types and sizes produced by the compiler.
*/
int
prof_buckets( int bucket )
{
	int bucket_size;
	switch ( bucket ) {
	case PAPI_PROFIL_BUCKET_16:
		bucket_size = sizeof ( short );
		break;
	case PAPI_PROFIL_BUCKET_32:
		bucket_size = sizeof ( int );
		break;
	case PAPI_PROFIL_BUCKET_64:
		bucket_size = sizeof ( unsigned long long );
		break;
	default:
		bucket_size = 0;
		break;
	}
	return ( bucket_size );
}

/* A standardized header printing routine. No assumed globals.
*/
void
prof_head( unsigned long blength, int bucket, int num_buckets, const char *header )
{
	int bucket_size = prof_buckets( bucket );
	printf
		( "\n------------------------------------------------------------\n" );
	printf( "PAPI_profil() hash table, Bucket size: %d bits.\n",
			bucket_size * 8 );
	printf( "Number of buckets: %d.\nLength of buffer: %ld bytes.\n",
			num_buckets, blength );
	printf( "------------------------------------------------------------\n" );
	printf( "%s\n", header );
}

/* This function prints a standardized profile output based on the bucket size.
   A row consisting of an address and 'n' data elements is displayed for each
   address with at least one non-zero bucket.
   Assumes global profbuf[] array pointers.
*/
void
prof_out( caddr_t start, int n, int bucket, int num_buckets,
		  unsigned int scale, unsigned short **profbuf)
{
	int i, j;
	unsigned short buf_16;
	unsigned int buf_32;
	unsigned long long buf_64;
	unsigned short **buf16 = ( unsigned short ** ) profbuf;
	unsigned int **buf32 = ( unsigned int ** ) profbuf;
	unsigned long long **buf64 = ( unsigned long long ** ) profbuf;
	
	printf("num_buckets=%d\n", num_buckets);

	// if ( !TESTS_QUIET ) {
		/* printf("%#lx\n",(unsigned long) start + (unsigned long) (2 * i)); */
		/* printf("start: %p; i: %#x; scale: %#x; i*scale: %#x; i*scale >>15: %#x\n", start, i, scale, i*scale, (i*scale)>>15); */
		switch ( bucket ) {
		case PAPI_PROFIL_BUCKET_16:
			for ( i = 0; i < num_buckets; i++ ) {
				for ( j = 0, buf_16 = 0; j < n; j++ ){
					buf_16 |= ( buf16[j] )[i];
				}
				
				if ( buf_16 ) {
/* On 32bit builds with gcc 4.3 gcc complained about casting caddr_t => long long
 * Thus the unsigned long to long long cast */
					printf( "%#-16llx",
						(long long) (unsigned long)start +
						( ( ( long long ) i * scale ) >> 15 ) );
					for ( j = 0, buf_16 = 0; j < n; j++ )
						printf( "\t%d", ( buf16[j] )[i] );
					printf( "\n" );
				}
			}
			break;
		case PAPI_PROFIL_BUCKET_32:
			for ( i = 0; i < num_buckets; i++ ) {
				for ( j = 0, buf_32 = 0; j < n; j++ )
					buf_32 |= ( buf32[j] )[i];
				if ( buf_32 ) {
					printf( "%#-16llx",
						(long long) (unsigned long)start +
						( ( ( long long ) i * scale ) >> 15 ) );
					for ( j = 0, buf_32 = 0; j < n; j++ )
						printf( "\t%d", ( buf32[j] )[i] );
					printf( "\n" );
				}
			}
			break;
		case PAPI_PROFIL_BUCKET_64:
			for ( i = 0; i < num_buckets; i++ ) {
				for ( j = 0, buf_64 = 0; j < n; j++ )
					buf_64 |= ( buf64[j] )[i];
				if ( buf_64 ) {
					printf( "%#-16llx",
						(long long) (unsigned long)start +
					        ( ( ( long long ) i * scale ) >> 15 ) );
					for ( j = 0, buf_64 = 0; j < n; j++ )
						printf( "\t%lld", ( buf64[j] )[i] );
					printf( "\n" );
				}
			}
			break;
		}
		printf
			( "------------------------------------------------------------\n\n" );
	// }
}

/* This function checks to make sure that some buffer value somewhere is nonzero.
   If all buffers are empty, zero is returned. This usually indicates a profiling
   failure. Assumes global profbuf[].
*/
int
prof_check( int n, int bucket, int num_buckets, unsigned short **profbuf )
{
	int i, j;
	int retval = 0;
	unsigned short **buf16 = ( unsigned short ** ) profbuf;
	unsigned int **buf32 = ( unsigned int ** ) profbuf;
	unsigned long long **buf64 = ( unsigned long long ** ) profbuf;

	switch ( bucket ) {
	case PAPI_PROFIL_BUCKET_16:
		for ( i = 0; i < num_buckets; i++ )
			for ( j = 0; j < n; j++ )
				retval = retval || buf16[j][i];
		break;
	case PAPI_PROFIL_BUCKET_32:
		for ( i = 0; i < num_buckets; i++ )
			for ( j = 0; j < n; j++ )
				retval = retval || buf32[j][i];
		break;
	case PAPI_PROFIL_BUCKET_64:
		for ( i = 0; i < num_buckets; i++ )
			for ( j = 0; j < n; j++ )
				retval = retval || buf64[j][i];
		break;
	}
	return ( retval );
}

/* Computes the length (in bytes) of the buffer required for profiling.
   'plength' is the profile length, or address range to be profiled.
   By convention, it is assumed that there are half as many buckets as addresses.
   The scale factor is a fixed point fraction in which 0xffff = ~1
                                                         0x8000 = 1/2
                                                         0x4000 = 1/4, etc.
   Thus, the number of profile buckets is (plength/2) * (scale/65536),
   and the length (in bytes) of the profile buffer is buckets * bucket size.
   */
unsigned long
prof_size( unsigned long plength, unsigned int scale, int bucket, int *num_buckets )
{
	unsigned long blength;
	long long llength = ( ( long long ) plength * scale );
	int bucket_size = prof_buckets( bucket );
	*num_buckets = ( int ) ( llength / 65536 / 2 );
	blength = ( unsigned long ) ( *num_buckets * bucket_size );
	return ( blength );
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
    unsigned short *profbuf[1];
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
    profbuf[0] = (unsigned short *)malloc(length*sizeof(unsigned short));
    
    if (profbuf[0] == NULL)
        handle_error(1);
        
    memset(profbuf[0],0x00,length*sizeof(unsigned short));
    
    if (PAPI_create_eventset(&EventSet) != PAPI_OK)
        handle_error(retval);
        
    /* Add Total FP Instructions Executed to our EventSet */
    if (PAPI_add_event(EventSet, PAPI_FP_INS) != PAPI_OK)
        handle_error(retval);
    
    if (PAPI_profil(profbuf[0], length, start, 65536, EventSet, 
    	PAPI_FP_INS, 1000000, PAPI_PROFIL_POSIX | PAPI_PROFIL_BUCKET_16) != PAPI_OK)
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
	  prof_out( start, 1, bucket, num_buckets, scale, profbuf );
	  retval = prof_check( 1, bucket, num_buckets, profbuf );
	  
	  if (retval < 0)
        handle_error(retval);

    free(a);
    free(b);
    free(c);
    
    free(profbuf[0]);
    
  // }

  return 0;
    
}

