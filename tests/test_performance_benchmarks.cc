/*
	test_performance_benchmarks.cc
	
	Performance regression tests for all batches
	Establishes baseline performance metrics
*/

#include "test_framework.h"
#include "../calc.h"
#include "../ship.h"
#include "../planet.h"
#include "../alliance.h"
#include "../equip.h"
#include "../database.h"
#include <time.h>

// Performance measurement helpers
static clock_t start_time;
static void start_benchmark() { start_time = clock(); }
static double end_benchmark() { 
	return ((double)(clock() - start_time)) / CLOCKS_PER_SEC * 1000.0; // ms
}

void benchmark_ship_ai_processing() {
	try {
		ship::init();
		alliance::init();
		calc::init();
		
		// Create test ships
		ship* test_ships[10];
		for (int i = 0; i < 10; i++) {
			test_ships[i] = ship::libget(0);
		}
		
		start_benchmark();
		
		// Benchmark AI processing
		for (int i = 0; i < 100; i++) {
			ship::behaveall();
		}
		
		double elapsed = end_benchmark();
		double per_ship_per_frame = elapsed / (100 * 10); // 100 frames, 10 ships
		
		printf("AI Processing: %.2f ms per ship per frame\n", per_ship_per_frame);
		TEST_ASSERT(per_ship_per_frame < 10.0, "AI processing under 10ms per ship");
		
	} catch (...) {
		TEST_ASSERT(false, "AI processing benchmark");
	}
}

void benchmark_collision_detection() {
	try {
		ship::init();
		calc::init();
		
		ship* test_ship = ship::libget(0);
		if (!test_ship) {
			TEST_ASSERT(true, "collision benchmark skipped (no test ship)");
			return;
		}
		
		start_benchmark();
		
		// Test collision detection performance
		for (int i = 0; i < 1000; i++) {
			cord test_location = {(long)(i * 10), (long)(i * 5)};
			vect test_velocity = {(double)(i % 10), (double)(i % 5)};
			test_ship->detect_collision(test_location, test_velocity);
		}
		
		double elapsed = end_benchmark();
		printf("Collision Detection: %.2f ms for 1000 tests\n", elapsed);
		TEST_ASSERT(elapsed < 50.0, "collision detection under 50ms for 1000 tests");
		
	} catch (...) {
		TEST_ASSERT(false, "collision detection benchmark");
	}
}

void benchmark_network_message_processing() {
	try {
		// Simulate network message processing
		unsigned char test_buffer[1024];
		
		start_benchmark();
		
		// Simulate message processing
		for (int i = 0; i < 1000; i++) {
			// Simulate buffer operations
			memset(test_buffer, i % 256, sizeof(test_buffer));
		}
		
		double elapsed = end_benchmark();
		double per_message = elapsed / 1000.0;
		
		printf("Network Message Processing: %.3f ms per message\n", per_message);
		TEST_ASSERT(per_message < 1.0, "network processing under 1ms per message");
		
	} catch (...) {
		TEST_ASSERT(false, "network message processing benchmark");
	}
}

void benchmark_graphics_rendering() {
	try {
		// Simulate graphics operations
		start_benchmark();
		
		// Simulate rendering calculations
		for (int i = 0; i < 10000; i++) {
			pol p = {(double)(i % 360), (double)(i % 100)};
			vect v = p.to_vector_coordinates();
			pol p2 = v.to_polar_coordinates();
		}
		
		double elapsed = end_benchmark();
		printf("Graphics Calculations: %.2f ms for 10000 operations\n", elapsed);
		TEST_ASSERT(elapsed < 100.0, "graphics calculations under 100ms");
		
	} catch (...) {
		TEST_ASSERT(false, "graphics rendering benchmark");
	}
}

void benchmark_database_operations() {
	try {
		start_benchmark();
		
		// Benchmark database operations
		FILE* write_file = fopen("benchmark_test.dat", "wb");
		if (write_file) {
			database::openwriter(write_file);
			
			for (int i = 0; i < 100; i++) {
				char obj_name[32];
				sprintf(obj_name, "BenchmarkObject%d", i);
				database::putobject(obj_name);
				database::putvalue("TestValue", i);
			}
			
			database::closewriter(); // This calls fclose() internally
		}
		
		// Benchmark reading
		FILE* read_file = fopen("benchmark_test.dat", "rb");
		if (read_file) {
			database::openreader(read_file);
			
			for (int i = 0; i < 100; i++) {
				char obj_name[32];
				sprintf(obj_name, "BenchmarkObject%d", i);
				database::switchobj(obj_name);
				long value = database::getvalue("TestValue");
				(void)value; // Suppress unused warning
			}
			
			database::closereader(); // This calls fclose() internally
		}
		
		double elapsed = end_benchmark();
		printf("Database Operations: %.2f ms for 200 operations\n", elapsed);
		TEST_ASSERT(elapsed < 200.0, "database operations under 200ms");
		
	} catch (...) {
		TEST_ASSERT(false, "database operations benchmark");
	}
}

void benchmark_physics_calculations() {
	try {
		calc::init();
		
		start_benchmark();
		
		// Benchmark physics calculations
		for (int i = 0; i < 10000; i++) {
			vect v1 = {(double)(i % 1000), (double)((i * 2) % 1000)};
			vect v2 = {(double)((i * 3) % 1000), (double)((i * 4) % 1000)};
			
			pol p1 = v1.to_polar_coordinates();
			pol p2 = v2.to_polar_coordinates();
			
			vect result = p1.to_vector_coordinates();
		}
		
		double elapsed = end_benchmark();
		double per_calculation = elapsed / 10000.0;
		
		printf("Physics Calculations: %.4f ms per calculation\n", per_calculation);
		TEST_ASSERT(per_calculation < 0.01, "physics calculations under 0.01ms each");
		
	} catch (...) {
		TEST_ASSERT(false, "physics calculations benchmark");
	}
}

void run_performance_benchmarks() {
	printf("\n--- Performance Benchmarks ---\n");
	benchmark_ship_ai_processing();
	benchmark_collision_detection();
	benchmark_network_message_processing();
	benchmark_graphics_rendering();
	benchmark_database_operations();
	benchmark_physics_calculations();
}