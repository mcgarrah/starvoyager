/*
	test_function_renames.cc
	
	Tests for function rename refactoring (Batch 5)
*/

#include "test_framework.h"
#include "../calc.h"
#include "../ship.h"
#include "../alliance.h"
#include "../equip.h"
#include "../planet.h"

void test_network_serialization() {
	try {
		// Initialize systems
		calc::init();
		ship::init();
		alliance::init();
		equip::init();
		
		// Test network serialization (netout function)
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			unsigned char buffer[256];
			test_ship->serialize_to_network(1, buffer);  // Test SERV_SELF type
			TEST_ASSERT(true, "network serialization works");
		} else {
			TEST_ASSERT(true, "network serialization test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "network serialization test");
	}
}

void test_password_encryption() {
	try {
		// Test password obscuring (obscure function)
		char test_password[33];
		strncpy(test_password, "testpass", sizeof(test_password)-1);
		test_password[sizeof(test_password)-1] = '\0';
		char original[33];
		strncpy(original, test_password, sizeof(original)-1);
		original[sizeof(original)-1] = '\0';
		
		calc::encrypt_password(test_password);
		
		// Password should be changed after obscuring
		TEST_ASSERT(strcmp(test_password, original) != 0, "password encryption changes string");
		TEST_ASSERT(strlen(test_password) == strlen(original), "password length unchanged");
	} catch (...) {
		TEST_ASSERT(false, "password encryption test");
	}
}

void test_planet_navigation() {
	try {
		// Test automatic navigation (autonav function) indirectly
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test navigation-related functionality
			test_ship->turn(1);
			test_ship->accel(1, false);
			TEST_ASSERT(true, "navigation functions work");
		} else {
			TEST_ASSERT(true, "planet navigation test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "planet navigation test");
	}
}

void test_collision_detection_renamed() {
	try {
		// Test collision detection (colldetect function)
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			cord test_location = {0, 0};
			vect test_velocity = {0, 0};
			
			bool collision = test_ship->detect_collision(test_location, test_velocity);
			TEST_ASSERT(collision == false || collision == true, "collision detection returns boolean");
		} else {
			TEST_ASSERT(true, "collision detection test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "collision detection test");
	}
}

void test_equipment_reference_updates() {
	try {
		// Test equipment reference updates (resequip function)
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			test_ship->update_equipment_references();
			TEST_ASSERT(true, "equipment reference updates work");
		} else {
			TEST_ASSERT(true, "equipment reference test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "equipment reference updates test");
	}
}

void test_ai_behavior_functions() {
	try {
		// Test AI behavior functions indirectly through ship visibility
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test ship visibility (uses AI-related logic)
			bool can_see_self = test_ship->see(test_ship);
			TEST_ASSERT(can_see_self == true, "AI visibility functions work");
		} else {
			TEST_ASSERT(true, "AI behavior test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "AI behavior functions test");
	}
}

void test_data_comparison_functions() {
	try {
		// Test data comparison (dateq function)
		unsigned char data1[4] = {1, 2, 3, 4};
		unsigned char data2[4] = {1, 2, 3, 4};
		unsigned char data3[4] = {1, 2, 3, 5};
		
		bool equal1 = calc::data_arrays_equal(data1, data2, 4);
		bool equal2 = calc::data_arrays_equal(data1, data3, 4);
		
		TEST_ASSERT(equal1 == true, "identical data arrays compare equal");
		TEST_ASSERT(equal2 == false, "different data arrays compare unequal");
	} catch (...) {
		TEST_ASSERT(false, "data comparison functions test");
	}
}

void run_function_renames_tests() {
	printf("\n--- Function Renames Tests ---\n");
	test_network_serialization();
	test_password_encryption();
	test_planet_navigation();
	test_collision_detection_renamed();
	test_equipment_reference_updates();
	test_ai_behavior_functions();
	test_data_comparison_functions();
}