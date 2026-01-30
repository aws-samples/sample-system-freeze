// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2026 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 */

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>

#define NS_PER_SEC	1000000000ULL
#define NS_PER_MSEC	1000000ULL

static volatile int should_exit = 0;
static struct config {
	long threshold_ns;
	int duration_sec;
	unsigned long cpu_mask;
} cfg;

static uint64_t timespec_to_ns(struct timespec *ts) {
	return ts->tv_sec * NS_PER_SEC + ts->tv_nsec;
}

static void usage(const char *prog) {
	fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "  -t, --threshold <ms>    Threshold in milliseconds (default: 100)\n");
	fprintf(stderr, "  -c, --cpus <mask|all>   CPU mask in hex (CPUs 0-63) or 'all' (default: all)\n");
	fprintf(stderr, "  -d, --duration <sec>    Test duration in seconds (default: run forever)\n");
	fprintf(stderr, "  -h, --help              Show this help\n");
}

static int parse_options(int argc, char *argv[]) {
	static struct option long_options[] = {
		{"threshold", required_argument, 0, 't'},
		{"cpus", required_argument, 0, 'c'},
		{"duration", required_argument, 0, 'd'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int opt;

	cfg.threshold_ns = 100 * NS_PER_MSEC;
	cfg.duration_sec = 0;
	cfg.cpu_mask = 0;

	while ((opt = getopt_long(argc, argv, "t:c:d:h", long_options, NULL)) != -1) {
		switch (opt) {
		case 't':
			cfg.threshold_ns = strtol(optarg, NULL, 10) * NS_PER_MSEC;
			break;
		case 'c':
			if (!strncmp(optarg, "all", 3))
				break;
			cfg.cpu_mask = strtoul(optarg, NULL, 16);
			break;
		case 'd':
			cfg.duration_sec = atoi(optarg);
			break;
		case 'h':
			usage(argv[0]);
			return 1;
		default:
			usage(argv[0]);
			return -1;
		}
	}

	if (cfg.threshold_ns <= 0) {
		fprintf(stderr, "Error: threshold must be positive\n\n");
		usage(argv[0]);
		return -1;
	}

	return 0;
}

void *monitor_thread(void *arg) {
	struct timespec ts, prev_ts;
	int cpu = (int)(long)arg;
	cpu_set_t cpuset;
	uint64_t diff_ns;

	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

	clock_gettime(CLOCK_MONOTONIC_RAW, &prev_ts);
	while (!should_exit) {

		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		diff_ns = timespec_to_ns(&ts) - timespec_to_ns(&prev_ts);
		if (diff_ns >= cfg.threshold_ns) {
			time_t now = time(NULL);
			struct tm *tm_info = localtime(&now);
			char time_buf[64];
			strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

			printf("[CPU %u] [%s] Jump: %lu us\n", cpu, time_buf, diff_ns / 1000UL);
		}

		prev_ts = ts;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
	int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	int thread_count = 0;
	int ret;

	if (!!(ret = parse_options(argc, argv)))
		return ret < 0;

	pthread_t *threads = calloc(num_cpus, sizeof(pthread_t));
	if (!threads) {
		fprintf(stderr, "Failed to allocate memory for threads\n");
		return 1;
	}

	for (int cpu = 0; cpu < num_cpus; cpu++)
		if (!cfg.cpu_mask || (cpu < 64 && (cfg.cpu_mask & (1UL << cpu))))
			pthread_create(&threads[thread_count++], NULL,
				       monitor_thread, (void *)(long)cpu);

	if (cfg.duration_sec > 0) {
		sleep(cfg.duration_sec);
		should_exit = 1;
	}

	for (int i = 0; i < thread_count; i++)
		pthread_join(threads[i], NULL);

	free(threads);
	return 0;
}
