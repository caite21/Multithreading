/* Learning OpenMP */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h> 
#include <omp.h> 


int p;


void Hello(void) {
    int my_rank = omp_get_thread_num();
    int thread_count = omp_get_num_threads();
    printf("Hello from thread %d of %d\n", my_rank, thread_count);
} 

void basic(void) {
	/*
	parallel directive: "master" is calling thread, creates and 
	works with num_threads-1 slave threads to complete block in
	parallel and they will synchronize at the end (all threads will
	meet up before master thread continues, like a barrier)
	*/
	Hello();
	printf("before omp parallel\n");

	// default num threads is 4 (or less sometimes!!)
	# pragma omp parallel num_threads(10)
	Hello();

	printf("afer omp parallel\n");
	Hello();

	// variables out here are shared by all threads
	# pragma omp parallel 
	{
		Hello();  // same as above
		// any variables in here are private to the thread
	}

}


void single_for(void) {
	int i, s;
	int k = 5;

	# pragma omp parallel for num_threads(2)
	for (i=0; i<k; i++) {  // must be defined number of loops
		Hello();
	}
	printf("\n");

	# pragma omp parallel for num_threads(2)
	for (i=0; i<k; i++) {  // must be defined number of loops
		Hello();
		// break;  // not allowed to use in for OpenMP for loop
		i++;  // changes number of loops only if num_threads < k
	}
	printf("\n");


	s = 0;
	// private initializes s to a random value for each thread
	# pragma omp parallel for private(s) num_threads(10)
	for (i=0; i<k; i++) {
		s=0;  // NEED
		s += 2;
		printf("%d\n", s);  // each =2
	}
	printf("s: %d\n", s);  // s still =0, not =2

	s = 0;
	// initializes s to original s value
	# pragma omp parallel for firstprivate(s) num_threads(10)
	for (i=0; i<k; i++) {
		s += 2;
		printf("%d\n", s);  // each =2
	}
	printf("s: %d\n", s);  // s still =0, not =2

	s = 0;
	# pragma omp parallel for shared(s) num_threads(10)
	for (i=0; i<k; i++) {  
		// default is i is private (copied) and set for each iter and k is automatically shared
		# pragma omp critical  // no race but is random order
		s += 2;
		printf("%d\n", s); // 2, 4, 6...
	}
	printf("s: %d\n", s); // =10

	s = 0;
	// each thread has copy of s then s is summed at the end
	// sys knows to set each copy of s to 0 for sum (1 for product, inf for min, -inf for max)
	# pragma omp parallel for num_threads(10) reduction(+: s)
	for (i=0; i<k; i++) {
		s += 2;
		printf("%d\n", s);  // each =2
	}
	printf("s: %d\n", s);  // =10

	// above is equal to this
	// note sum is added to s=10 from above
	# pragma omp parallel num_threads(k) reduction(+: s)
	{
		s += 2;
		printf("%d\n", s);  // each =2
	}
	printf("s: %d\n", s);  // s = 10 + 10 = 20

	// with a different operation:
	# pragma omp parallel num_threads(k) reduction(*: s)
	{
		// implied new_s = 1
		s -= 3;  
		printf("%d\n", s);  // each = 1 - 3 = -2
	}
	printf("s: %d\n", s);  // s = 20(-2)(-2)(-2)(-2)(-2) = -640
}


void nested_for(void) {
	
}


int main (int argc, char *argv[]) {

    p = 10;

	// basic();

	single_for();

	// nested_for();

    return 0;
}


