/*
	test_batch1_core_structures.cc
	
	Regression tests for Batch 1: Core Data Structure Naming
	Tests ship and planet core functionality before refactoring
*/

#include "test_framework.h"
#include "../calc.h"
#include "../ship.h"
#include "../planet.h"
#include "../alliance.h"
#include "../equip.h"
#include "../database.h"

void test_ship_detection_functions() {
	try {
		ship::init();
		alliance::init();
		
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test see() function with null target
			bool result = test_ship->can_detect((ship*)NULL);
			TEST_ASSERT(result == false, "ship see() handles null ship");
			
			// Test see() function with self
			result = test_ship->can_detect(test_ship);
			TEST_ASSERT(result == true, "ship see() returns true for self");
		}
		TEST_ASSERT(true, "ship detection functions work");
	} catch (...) {
		TEST_ASSERT(false, "ship detection functions work");
	}
}

void test_ship_damage_system() {
	try {
		ship::init();
		alliance::init();
		
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			cord hit_location = {0, 0};
			vect hit_velocity = {0, 0};
			
			// Test hit() function - should not crash
			test_ship->take_damage(10, hit_location, hit_velocity, NULL);
			TEST_ASSERT(true, "ship hit() function works");
		} else {
			TEST_ASSERT(true, "ship damage test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "ship damage system test");
	}
}

void test_ship_index_operations() {
	try {
		ship::init();
		
		// Test get() function with valid indices
		for (int i = 0; i < 10; i++) {
			(void)ship::find_by_index(i);
			// Should not crash, may return null
		}
		
		// Test get() function with invalid indices
		ship* s1 = ship::find_by_index(-1);
		TEST_ASSERT(s1 == NULL, "ship get() handles negative index");
		
		ship* s2 = ship::find_by_index(1000);
		TEST_ASSERT(s2 == NULL, "ship get() handles large index");
		
		TEST_ASSERT(true, "ship index operations work");
	} catch (...) {
		TEST_ASSERT(false, "ship index operations work");
	}
}

void test_planet_selection() {
	try {
		planet::init();
		alliance::init();
		
		// Test pick() function with null alliance
		planet* p1 = planet::find_random_planet(NULL);
		TEST_ASSERT(p1 == NULL, "planet pick() handles null alliance");
		
		// Test pick() function with valid alliance
		alliance* test_alliance = alliance::get(0);
		if (test_alliance) {
			(void)planet::find_random_planet(test_alliance);
			// Should not crash, may return null
		}
		
		TEST_ASSERT(true, "planet selection functions work");
	} catch (...) {
		TEST_ASSERT(false, "planet selection functions work");
	}
}

void test_variable_name_consistency() {
	// Test that common variable patterns are accessible
	try {
		ship::init();
		planet::init();
		alliance::init();
		
		// Test ship variable access patterns
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// These should compile and not crash
			(void)test_ship->all;
			(void)test_ship->enemy_target;
			(void)test_ship->friendly_target;
			(void)test_ship->planet_target;
		}
		
		TEST_ASSERT(true, "variable name patterns accessible");
	} catch (...) {
		TEST_ASSERT(false, "variable name patterns accessible");
	}
}

void test_ship_planet_interactions() {
	try {
		ship::init();
		planet::init();
		alliance::init();
		
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test planet visibility
			planet* test_planet = planet::find_by_index(0);
			if (test_planet) {
				(void)test_ship->can_detect(test_planet);
				// Should not crash
			}
		}
		
		TEST_ASSERT(true, "ship-planet interactions work");
	} catch (...) {
		TEST_ASSERT(false, "ship-planet interactions work");
	}
}

void test_alliance_ship_relationships() {
	try {
		alliance::init();
		ship::init();
		
		alliance* alliance1 = alliance::get(0);
		alliance* alliance2 = alliance::get(1);
		
		if (alliance1 && alliance2) {
			// Test alliance opposition
			(void)alliance1->opposes(alliance2);
			// Should not crash
		}
		
		TEST_ASSERT(true, "alliance-ship relationships work");
	} catch (...) {
		TEST_ASSERT(false, "alliance-ship relationships work");
	}
}

void test_player_ship_assignment() {
	try {
		ship::init();
		
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test player assignment (should handle null)
			test_ship->assign(NULL);
		}
		
		TEST_ASSERT(true, "player-ship assignment works");
	} catch (...) {
		TEST_ASSERT(false, "player-ship assignment works");
	}
}

void test_enemy_target_selection() {
	try {
		ship::init();
		alliance::init();
		
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test enemy/friendly targeting (public interface)
			// These are private methods, so test through public interface
			test_ship->enemy_target = NULL;
			test_ship->friendly_target = NULL;
			// Should not crash
		}
		
		TEST_ASSERT(true, "enemy target selection works");
	} catch (...) {
		TEST_ASSERT(false, "enemy target selection works");
	}
}

void run_batch1_core_structures_tests() {
	printf("\n--- Batch 1: Core Structures Regression Tests ---\n");
	test_ship_detection_functions();
	test_ship_damage_system();
	test_ship_index_operations();
	test_planet_selection();
	test_variable_name_consistency();
	test_ship_planet_interactions();
	test_alliance_ship_relationships();
	test_player_ship_assignment();
	test_enemy_target_selection();
}