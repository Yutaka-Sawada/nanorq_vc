
#include <inttypes.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "nanorq/nanorq.h"


/*
PCG Random Number Generation
http://www.pcg-random.org

I use global RNGs only.
*/
static uint64_t pcg32_state, pcg32_inc;

uint32_t pcg32_random()
{
    uint64_t oldstate = pcg32_state;
    pcg32_state = oldstate * 6364136223846793005ULL + pcg32_inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((uint32_t)(-(int32_t)rot) & 31));
}

void pcg32_srandom(uint64_t seed, uint64_t seq)
{
	pcg32_state = 0U;
	pcg32_inc = (seq << 1u) | 1u;
	pcg32_random();
	pcg32_state += seed;
	pcg32_random();
}

uint32_t pcg32_boundedrand(uint32_t bound)
{
    uint32_t threshold = (uint32_t)(-(int32_t)bound) % bound;

    for (;;) {
        uint32_t r = pcg32_random();
        if (r >= threshold)
            return r % bound;
    }
}


/*
source_count = s#
parity_count = p#
lost_count = l#
each_size = d#
cohort_count = c#
*/
int main(int argc, char* argv[]){
	char *p;
	char *source_buf, *parity_buf, *work_buf;
	int i, j, k, max, rv;
	int source_count = 1000, parity_count = 200, lost_count = 0, cohort_count = 1;
	int input_count, sbn, esi;
	int full_source_cohort, full_source_count, full_source_border;
	int full_parity_cohort, full_parity_count, full_parity_border;
	int *order_buf;
	unsigned int *int_p;
	unsigned int each_size = 64000, tag;
	size_t source_size;
	size_t malloc_size, input_size, output_size;
	double value_kb, value_mb, input_speed, output_speed;
	clock_t start, finish;
	double duration;
	uint64_t oti_common;	// Common FEC Object Transmission Information
	uint32_t oti_scheme;	// Scheme-specific Object Transmission Information
	nanorq *rq;
	struct ioctx *myio;

	// read input
	for (i = 1; i < argc; i++){
		p = argv[i];
		//printf("argv[%d] = %s\n", i, p);

		if ( (p[0] == 's') || (p[0] == 'S') ){
			source_count = atoi(p + 1);
		} else if ( (p[0] == 'p') || (p[0] == 'P') ){
			parity_count = atoi(p + 1);
		} else if ( (p[0] == 'l') || (p[0] == 'L') ){
			lost_count = atoi(p + 1);
		} else if ( (p[0] == 'e') || (p[0] == 'E') ){
			each_size = atoi(p + 1);
		} else if ( (p[0] == 'c') || (p[0] == 'C') ){
			cohort_count = atoi(p + 1);

		} else {	// invalid option
			printf("options: s#, p#, l#, e#\ns# or S# = source count (1 ~ 56403)\np# or P# = parity count\nl# or L# = loss count\ne# or E# = each data size (multiple of 8 upto 65536)\nc# or C# = cohort count (1 ~ 256, 0 = auto)\n");
			return 0;
		}
	}

	// check input
	if (source_count < 1)
		source_count = 1;
	if (source_count > 56403)
		source_count = 56403;
	if (parity_count <= 0)
		parity_count = 1;
	if ( (lost_count <= 0) || (lost_count > parity_count) )
		lost_count = parity_count;
	if (lost_count > source_count)
		lost_count = source_count;
	printf("source count = %d, parity count = %d, loss count = %d\n", source_count, parity_count, lost_count);
	if (each_size & 7)	// each data size must be multiple of 8.
		each_size = (each_size + 7) & ~7;
	if (each_size > 65536)
		each_size = 65536;
	value_kb = (double)each_size / 1000;
	value_mb = (double)each_size / 1000000.f;
	printf("each data size = %u bytes = %g KB = %g MB\n", each_size, value_kb, value_mb);
	if (cohort_count > 256)
		cohort_count = 256;

	start = clock();
	printf("\n allocate memory and setup data ...\n");

	// allocate buffer
	source_size = (size_t)each_size * source_count;
	source_buf = malloc(source_size);
	if (source_buf == NULL){
		printf("Failed: malloc, %zd\n", source_size);
		return 1;
	}
	value_kb = (double)source_size / 1000;
	value_mb = (double)source_size / 1000000.f;
	printf("source data size = %zd bytes = %g KB = %g MB\n", source_size, value_kb, value_mb);

	// fill source data with random
	max = each_size / 4;
	for (i = 0; i < source_count; i++){
		int_p = (int *)(source_buf + each_size * i);
		pcg32_srandom(0, i);
		for (j = 0; j < max; j++)
			int_p[j] = pcg32_random();
	}

	// fill parity data with zero
	malloc_size = each_size * parity_count;
	parity_buf = calloc(malloc_size, 1);
	if (parity_buf == NULL){
		printf("Failed: malloc, %zd\n", malloc_size);
		free(source_buf);
		return 1;
	}
	value_kb = (double)malloc_size / 1000;
	value_mb = (double)malloc_size / 1000000.f;
	printf("parity data size = %zd bytes = %g KB = %g MB\n", malloc_size, value_kb, value_mb);

	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf(" ... %.3f seconds\n", duration);

	printf("\n encode ...\n");
	start = clock();

	// nanorq encoder reads source data from this.
	myio = ioctx_from_mem(source_buf, source_size);

	// initialize nanorq encoder
	if (cohort_count > 0){
		// SBN in RFC 6330 is same as number of cohorts.
		// You may set your favorite number for tests.
		// SBN must be 1 to disable interleaving at speed test.
		rq = nanorq_encoder_new_ex(source_size, each_size, 0, cohort_count, 8);
	} else {
		// Automatic SBN by RFC 6330
		// Caution, interleaving method in RFC 6330 isn't good for burst error.
		// When protecting some files, loss of a whole file may erase a SBN at once.
		rq = nanorq_encoder_new(source_size, each_size, 8);
	}
	if (rq == NULL){
		printf("Failed: nanorq_encoder_new_ex\n");
		free(source_buf);
		free(parity_buf);
		myio->destroy(myio);
		return 1;
	}

	oti_scheme = nanorq_oti_scheme_specific(rq);
	//i = (oti_scheme >> 24) + 1;
	//j = ((oti_scheme >> 8) & 0xffff) + 1;
	//rv = oti_scheme & 0xff;
	//printf("number of source blocks = %d\n", i);
	//printf("number of sub-blocks = %d\n", j);
	//printf("symbol alignment = %d\n", rv);
	//output_size = nanorq_symbol_size(rq);
	//printf("symbol size (T) = %zd bytes\n", output_size);
	cohort_count = (int)nanorq_blocks(rq);
	printf("number of SBN = %d\n", cohort_count);
	full_source_count = source_count / cohort_count;
	full_source_cohort = source_count % cohort_count;
	if (full_source_cohort == 0){
		full_source_cohort = cohort_count;	// number of full size cohort
	} else {
		full_source_count++;
	}
	full_source_border = full_source_count * full_source_cohort;
	input_size = nanorq_block_symbols(rq, 0);
	if (cohort_count == 1){
		printf("number of source per SBN = %zd\n", input_size);
	} else {
		output_size = nanorq_block_symbols(rq, cohort_count - 1);
		if (output_size == input_size){
			printf("number of source per SBN = %zd\n", input_size);
		} else {
			printf("number of source per SBN = %zd (%d) ~ %zd (%d)\n",
					output_size, cohort_count - full_source_cohort, input_size, full_source_cohort);
		}
	}
	full_parity_count = parity_count / cohort_count;
	full_parity_cohort = parity_count % cohort_count;
	if (full_parity_cohort == 0){
		full_parity_cohort = cohort_count;	// number of full size cohort
		printf("number of parity per SBN = %d\n", full_parity_count);
	} else {
		full_parity_count++;
		printf("number of parity per SBN = %d (%d) ~ %d (%d)\n",
				full_parity_count - 1, cohort_count - full_parity_cohort, full_parity_count, full_parity_cohort);
	}
	full_parity_border = full_parity_count * full_parity_cohort;

	// generate intermediate symbols from the source symbols
	for (k = 0; k < cohort_count; k++){
		rv = nanorq_generate_symbols(rq, k, myio);	// SBN index = 0 ~ cohort_count - 1
		if (rv == 0){
			printf("Failed: nanorq_generate_symbols, %d, SBN = %d\n", rv, k);
			free(source_buf);
			free(parity_buf);
			nanorq_free(rq);
			myio->destroy(myio);
			return 1;
		}
	}

	p = parity_buf;
	for (j = 0; j < parity_count; j++){
		if (j < full_parity_border){
			esi = j % full_parity_count;
			sbn = j / full_parity_count;
		} else {
			esi = (j - full_parity_border) % (full_parity_count - 1);
			sbn = full_parity_cohort + (j - full_parity_border) / (full_parity_count - 1);
		}
		if (sbn < full_source_cohort){
			esi += full_source_count;
		} else {
			esi += full_source_count - 1;
		}
		output_size = nanorq_encode(rq, p, esi, sbn, myio);
		if (output_size != each_size){
			printf("Failed: nanorq_encode, %zd, ESI = %d, SBN = %d\n", output_size, esi, sbn);
			free(source_buf);
			free(parity_buf);
			nanorq_free(rq);
			myio->destroy(myio);
			return 1;
		}
		p += each_size;
	}
	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf(" ... %.3f seconds\n", duration);

	oti_common = nanorq_oti_common(rq);
	input_size = oti_common >> 24;	// This transfer length must be same as source data size.
	i = (oti_common & 0xffff) + 1;	// This symbol size must be same as each data size.
	printf("transfer length = %zd bytes\n", input_size);
	printf("symbol size = %d bytes\n", i);
	nanorq_free(rq);	// No need encoder
	myio->destroy(myio);

	output_size = (size_t)each_size * parity_count;
	value_mb = (double)source_size / 1000000.f;
	input_speed = value_mb / duration;
	output_speed = (double)output_size / 1000000.f / duration;
	printf("nanorq Encoder(%g MB in %d pieces, %d parities): Input= %g MB/s, Output= %g MB/s\n",
			value_mb, source_count, parity_count, input_speed, output_speed);

	start = clock();
	printf("\n set %d random error and allocate memory ...\n", lost_count);

	// shuffle order of source and parity data
	malloc_size = (source_count + parity_count) * sizeof(int);	// available source and parity
	order_buf = malloc(malloc_size);
	if (order_buf == NULL){
		printf("Failed: malloc, %zd\n", malloc_size);
		free(source_buf);
		free(parity_buf);
		return 1;
	}
	max = source_count + parity_count;
	for (i = 0; i < max; i++)
		order_buf[i] = i;

	// swap index to select available data
	pcg32_srandom(0, 1);
	for (i = 0; i < source_count; i++){
		j = pcg32_boundedrand(source_count);
		if (j != i){
			rv = order_buf[j];
			order_buf[j] = order_buf[i];
			order_buf[i] = rv;
		}
	}
	max = source_count + lost_count;
	for (i = source_count; i < max; i++){
		j = source_count + pcg32_boundedrand(lost_count);
		if (j != i){
			rv = order_buf[j];
			order_buf[j] = order_buf[i];
			order_buf[i] = rv;
		}
	}
	if (parity_count > lost_count){
		max = source_count + parity_count;
		for (i = source_count + lost_count; i < max; i++){
			j = source_count + lost_count + pcg32_boundedrand(parity_count - lost_count);
			if (j != i){
				rv = order_buf[j];
				order_buf[j] = order_buf[i];
				order_buf[i] = rv;
			}
		}
	}

	// allocate working buffer
	work_buf = calloc(source_size, 1);
	if (parity_buf == NULL){
		printf("Failed: malloc, %zd\n", source_size);
		free(source_buf);
		free(parity_buf);
		free(order_buf);
		return 1;
	}
	value_kb = (double)source_size / 1000;
	value_mb = (double)source_size / 1000000.f;
	printf("work data size = %zd bytes = %g KB = %g MB\n", source_size, value_kb, value_mb);

	// initialize nanorq decoder
	rq = nanorq_decoder_new(oti_common, oti_scheme);
	if (rq == NULL){
		printf("Failed: nanorq_decoder_new\n");
		free(source_buf);
		free(parity_buf);
		free(order_buf);
		free(work_buf);
		return 1;
	}

	// nanorq decoder writes recovered source data on this.
	myio = ioctx_from_mem(work_buf, source_size);

	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf(" ... %.3f seconds\n", duration);

	printf("\n decode ...\n");
	start = clock();

	// input available source data at first
	max = source_count - lost_count;
	for (i = 0; i < max; i++){
		j = order_buf[i];	// index of available source data
		if (j < full_source_border){
			esi = j % full_source_count;
			sbn = j / full_source_count;
		} else {
			esi = (j - full_source_border) % (full_source_count - 1);
			sbn = full_source_cohort + (j - full_source_border) / (full_source_count - 1);
		}
		tag = esi | (sbn << 24);
		rv = nanorq_decoder_add_symbol(rq, source_buf + (size_t)each_size * j, tag, myio);
		if (rv == NANORQ_SYM_ERR){
			printf("Failed: nanorq_decoder_add_symbol, %d, ESI = %d, SBN = %d\n", rv, esi, sbn);
			free(source_buf);
			free(parity_buf);
			free(order_buf);
			free(work_buf);
			nanorq_free(rq);
			myio->destroy(myio);
			return 1;
		}
	}
	printf("input %d available source data\n", max);

	// nanorq doesn't have a function to determine possible repair.
	input_count = 0;
	// input available parity data until lost count
	for (i = 0; i < lost_count; i++){
	// input available parity data until lost count with 1% overhead
	// When it has some overhead parity, decode would become faster by 10% ~ 20%. 
	//for (i = 0; (i < parity_count) && (i < lost_count + lost_count / 100); i++){
		input_count++;
		j = order_buf[source_count + i] - source_count;	// index of available parity data
		if (j < full_parity_border){
			esi = j % full_parity_count;
			sbn = j / full_parity_count;
		} else {
			esi = (j - full_parity_border) % (full_parity_count - 1);
			sbn = full_parity_cohort + (j - full_parity_border) / (full_parity_count - 1);
		}
		if (sbn < full_source_cohort){
			esi += full_source_count;
		} else {
			esi += full_source_count - 1;
		}
		tag = esi | (sbn << 24);
		rv = nanorq_decoder_add_symbol(rq, parity_buf + (size_t)each_size * j, tag, myio);
		if (rv == NANORQ_SYM_ERR){
			printf("Failed: nanorq_decoder_add_symbol, %d, ESI = %d, SBN = %d\n", rv, esi, sbn);
			free(source_buf);
			free(parity_buf);
			free(order_buf);
			free(work_buf);
			nanorq_free(rq);
			myio->destroy(myio);
			return 1;
		}
	}
	printf("input %d available parity data\n", input_count);

/*
	if (cohort_count > 1){
		printf("\n");
		for (k = 0; k < cohort_count; k++){
			output_size = nanorq_num_missing(rq, k);
			input_size = nanorq_num_repair(rq, k);
			printf("number of missing symbols in decoder(%d) = %zd\n", k, output_size);
			printf("number of repair  symbols in decoder(%d) = %zd\n", k, input_size);
		}
		printf("\n");
	}
*/

	start = clock();
	for (k = 0; k < cohort_count; k++){
		rv = nanorq_repair_block(rq, myio, k);
		if (rv == 0){
			// input more parity data
			while (input_count < parity_count){
				j = order_buf[source_count + input_count] - source_count;	// index of available parity data
				input_count++;
				if (j < full_parity_border){
					esi = j % full_parity_count;
					sbn = j / full_parity_count;
				} else {
					esi = (j - full_parity_border) % (full_parity_count - 1);
					sbn = full_parity_cohort + (j - full_parity_border) / (full_parity_count - 1);
				}
				if (sbn < full_source_cohort){
					esi += full_source_count;
				} else {
					esi += full_source_count - 1;
				}
				if (sbn < k)
					continue;	// When parity is useless, try next one.
				tag = esi | (sbn << 24);
				rv = nanorq_decoder_add_symbol(rq, parity_buf + (size_t)each_size * j, tag, myio);
				if (rv == NANORQ_SYM_ERR){
					printf("Failed: nanorq_decoder_add_symbol, %d, ESI = %d, SBN = %d\n", rv, esi, sbn);
					free(source_buf);
					free(parity_buf);
					free(order_buf);
					free(work_buf);
					nanorq_free(rq);
					myio->destroy(myio);
					return 1;
				}
				// try to recover again
				rv = nanorq_repair_block(rq, myio, k);
				if (rv != 0){
					printf("input more available parity data to SBN(%d), total %d\n", k, input_count);
					break;
				}
			}
			// fail at s500 p100
			if (rv == 0){
				printf("Failed: nanorq_repair_block SBN(%d), total = %d\n", k, input_count);
					free(source_buf);
					free(parity_buf);
					free(order_buf);
					free(work_buf);
					nanorq_free(rq);
					myio->destroy(myio);
				return 1;
			}
		}
		//printf("nanorq_repair_block SBN(%d), total %d\n", k, input_count);
	}
	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf(" ... %.3f seconds\n", duration);

	free(source_buf);
	free(parity_buf);
	nanorq_free(rq);	// No need decoder
	myio->destroy(myio);

	output_size = (size_t)each_size * lost_count;	// bytes
	value_kb = (double)output_size / 1000;
	value_mb = (double)output_size / 1000000.f;
	printf("lost data size = %zd bytes = %g KB = %g MB\n", output_size, value_kb, value_mb);
	output_speed = value_mb / duration;

	input_count += source_count - lost_count;
	input_size = (size_t)each_size * input_count;
	value_kb = (double)input_size / 1000;
	value_mb = (double)input_size / 1000000.f;
	printf("input data size = %zd bytes = %g KB = %g MB\n", input_size, value_kb, value_mb);
	input_speed = value_mb / duration;
	printf("nanorq Decoder(%g MB in %d pieces, %d losses): Input= %g MB/s, Output= %g MB/s\n",
			value_mb, input_count, lost_count, input_speed, output_speed);

	start = clock();
	printf("\n verify recovered data ...\n");

	max = each_size / 4;
	for (i = 0; i < lost_count; i++){
		j = order_buf[i + source_count - lost_count];	// index of lost source data
		int_p = (int *)(work_buf + each_size * j);
		pcg32_srandom(0, j);
		for (j = 0; j < max; j++){
			if (int_p[j] != pcg32_random())
				break;
		}
		if (j < max){
			printf("Failed: recovered data, %d\n", i);
			break;
		}
	}
	if (i == lost_count)
		printf("All data repaired !\n");

	free(order_buf);	// End comparison
	free(work_buf);

	finish = clock();
	duration = (double)(finish - start) / CLOCKS_PER_SEC;
	printf(" ... %.3f seconds\n", duration);

	return 0;
}

