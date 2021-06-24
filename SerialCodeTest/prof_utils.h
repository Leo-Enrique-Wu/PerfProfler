
/* value for scale parameter that sets scale to 1 */
#define FULL_SCALE 65536

/* A standardized header printing routine. No assumed globals.
*/
void prof_head(unsigned long blength, int bucket_size, int num_buckets, const char *header);

/* This function prints a standardized profile output based on the bucket size.
   A row consisting of an address and 'n' data elements is displayed for each
   address with at least one non-zero bucket.
   Assumes global profbuf[] array pointers.
*/
void prof_out(caddr_t start, int n, int bucket, int num_buckets, unsigned int scale);

/* Given the profiling type (16, 32, or 64) this function returns the 
   bucket size in bytes. NOTE: the bucket size does not ALWAYS correspond
   to the expected value, esp on architectures like Cray with weird data types.
   This is necessary because the posix_profile routine in extras.c relies on
   the data types and sizes produced by the compiler.
*/
int prof_buckets(int bucket);

/* This function checks to make sure that some buffer value somewhere is nonzero.
   If all buffers are empty, zero is returned. This usually indicates a profiling
   failure. Assumes global profbuf[].
*/
int prof_check(int n, int bucket, int num_buckets);

/* Computes the length (in bytes) of the buffer required for profiling.
   'plength' is the profile length, or address range to be profiled.
   By convention, it is assumed that there are half as many buckets as addresses.
   The scale factor is a fixed point fraction in which 0xffff = ~1
                                                         0x8000 = 1/2
                                                         0x4000 = 1/4, etc.
   Thus, the number of profile buckets is (plength/2) * (scale/65536),
   and the length (in bytes) of the profile buffer is buckets * bucket size.
   */
unsigned long prof_size(unsigned long plength, unsigned int scale, int bucket, int *num_buckets);