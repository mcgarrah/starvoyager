/*
	test_framework.cc
	
	Implementation of minimal testing framework
*/

#include "test_framework.h"
#include <math.h>

int TestFramework::total_tests = 0;
int TestFramework::passed_tests = 0;
int TestFramework::failed_tests = 0;
int TestFramework::skipped_tests = 0;
char TestFramework::last_failure[256] = "";

void TestFramework::init() {
	total_tests = 0;
	passed_tests = 0;
	failed_tests = 0;
	skipped_tests = 0;
	last_failure[0] = '\0';
	printf("Starting Star Voyager Test Suite\n");
	printf("================================\n");
}

void TestFramework::assert_true(bool condition, const char* test_name) {
	total_tests++;
	if (condition) {
		// Check if this is a skipped test
		if (strstr(test_name, "skipped") != nullptr) {
			skipped_tests++;
		} else {
			passed_tests++;
		}
		printf("PASS: %s\n", test_name);
	} else {
		failed_tests++;
		printf("FAIL: %s\n", test_name);
		snprintf(last_failure, sizeof(last_failure), "%s", test_name);
	}
}

void TestFramework::assert_equals(int expected, int actual, const char* test_name) {
	total_tests++;
	if (expected == actual) {
		passed_tests++;
		printf("PASS: %s\n", test_name);
	} else {
		failed_tests++;
		printf("FAIL: %s (expected %d, got %d)\n", test_name, expected, actual);
		snprintf(last_failure, sizeof(last_failure), "%s", test_name);
	}
}

void TestFramework::assert_equals(float expected, float actual, float tolerance, const char* test_name) {
	total_tests++;
	if (fabs(expected - actual) <= tolerance) {
		passed_tests++;
		printf("PASS: %s\n", test_name);
	} else {
		failed_tests++;
		printf("FAIL: %s (expected %.3f, got %.3f)\n", test_name, expected, actual);
		snprintf(last_failure, sizeof(last_failure), "%s", test_name);
	}
}

void TestFramework::print_results() {
	printf("\n================================\n");
	printf("Test Results:\n");
	printf("Total: %d, Passed: %d (Skipped: %d), Failed: %d\n", total_tests, passed_tests, skipped_tests, failed_tests);
	if (failed_tests > 0) {
		printf("Last failure: %s\n", last_failure);
	}
	printf("Success rate: %.1f%%\n", (float)passed_tests / total_tests * 100.0f);
}

int TestFramework::get_failed_count() {
	return failed_tests;
}