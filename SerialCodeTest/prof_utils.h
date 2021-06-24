

void prof_head(unsigned long blength, int bucket_size, int num_buckets, const char *header);
void prof_out(caddr_t start, int n, int bucket, int num_buckets, unsigned int scale);
int prof_buckets(int bucket);
int prof_check(int n, int bucket, int num_buckets);