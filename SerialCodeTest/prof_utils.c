#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "papi.h"

#include "prof_utils.h"

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
		  unsigned int scale )
{
	int i, j;
	unsigned short buf_16;
	unsigned int buf_32;
	unsigned long long buf_64;
	unsigned short **buf16 = ( unsigned short ** ) profbuf;
	unsigned int **buf32 = ( unsigned int ** ) profbuf;
	unsigned long long **buf64 = ( unsigned long long ** ) profbuf;

	// if ( !TESTS_QUIET ) {
		/* printf("%#lx\n",(unsigned long) start + (unsigned long) (2 * i)); */
		/* printf("start: %p; i: %#x; scale: %#x; i*scale: %#x; i*scale >>15: %#x\n", start, i, scale, i*scale, (i*scale)>>15); */
		switch ( bucket ) {
		case PAPI_PROFIL_BUCKET_16:
			for ( i = 0; i < num_buckets; i++ ) {
				for ( j = 0, buf_16 = 0; j < n; j++ )
					buf_16 |= ( buf16[j] )[i];
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
prof_check( int n, int bucket, int num_buckets )
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