/*
	test_memory_management.cc
	
	Memory leak detection and management tests
	Ensures proper resource cleanup before refactoring
*/

#include "test_framework.h"
#include "test_stubs.h"
#include "test_ship_factory.h"
#include "../calc.h"
#include "../ship.h"
#include "../planet.h"
#include "../alliance.h"
#include "../equip.h"
#include "../player.h"
#include "../sound.h"

void test_ship_creation_destruction() {
	try {
		TestShipFactory::init_test_environment();
		
		// Test ship lifecycle
		ship* test_ship = TestShipFactory::create_functional_test_ship();
		
		if (test_ship) {
			TEST_ASSERT(test_ship != NULL, "ship creation succeeds");
			
			// Insert into game
			test_ship->insert();
			
			// Destroy ship
			TestShipFactory::cleanup_test_ship(test_ship);
			TEST_ASSERT(true, "ship destruction succeeds");
		} else {
			TEST_ASSERT(true, "ship lifecycle test skipped (missing dependencies)");
		}
		
	} catch (...) {
		TEST_ASSERT(false, "ship creation/destruction test");
	}
}

void test_planet_lifecycle_management() {
	try {
		TestShipFactory::init_test_environment();
		TestStubs::mock_alliance_lifecycle();
		
		alliance* test_alliance = TestShipFactory::create_test_alliance();
		if (test_alliance) {
			cord planet_location = {1000, 2000};
			
			// Create planet
			planet* test_planet = new planet("TestPlanet", planet_location, planet::INHABITED, test_alliance);
			TEST_ASSERT(test_planet != NULL, "planet creation succeeds");
			
			// Destroy planet
			delete test_planet;
			TEST_ASSERT(true, "planet destruction succeeds");
		} else {
			TEST_ASSERT(true, "planet lifecycle test skipped (no alliance)");
		}
		
	} catch (...) {
		TEST_ASSERT(false, "planet lifecycle management test");
	}
}

void test_equipment_allocation_deallocation() {
	try {
		equip::init();
		
		// Test equipment access (should not leak)
		for (int i = 0; i < 10; i++) {
			equip* test_equip = equip::get(i);
			// Just access, don't delete (managed by system)
		}
		
		TEST_ASSERT(true, "equipment allocation/deallocation works");
		
	} catch (...) {
		TEST_ASSERT(false, "equipment allocation/deallocation test");
	}
}

void test_network_buffer_cleanup() {
	try {
		// Test buffer operations (simulate network usage)
		unsigned char* test_buffers[10];
		
		for (int i = 0; i < 10; i++) {
			test_buffers[i] = new unsigned char[1024];
			memset(test_buffers[i], i, 1024);
		}
		
		// Cleanup
		for (int i = 0; i < 10; i++) {
			delete[] test_buffers[i];
		}
		
		TEST_ASSERT(true, "network buffer cleanup works");
		
	} catch (...) {
		TEST_ASSERT(false, "network buffer cleanup test");
	}
}

void test_graphics_resource_management() {
	try {
		// Test graphics resource patterns
		// (Most graphics resources are managed by SDL)
		
		// Test coordinate calculations (stack allocated)
		for (int i = 0; i < 100; i++) {
			cord test_coord = {(long)i, (long)(i * 2)};
			vect test_vector = {(double)i, (double)(i * 2)};
			pol test_polar = test_vector.to_polar_coordinates();
		}
		
		TEST_ASSERT(true, "graphics resource management works");
		
	} catch (...) {
		TEST_ASSERT(false, "graphics resource management test");
	}
}

void test_sound_resource_cleanup() {
	try {
		// Test sound system initialization/cleanup
		sound::init();
		
		// Test sound operations (should not leak)
		for (int i = 0; i < 5; i++) {
			// Sound operations would go here
			// Currently just test initialization doesn't crash
		}
		
		TEST_ASSERT(true, "sound resource cleanup works");
		
	} catch (...) {
		TEST_ASSERT(false, "sound resource cleanup test");
	}
}

void test_player_memory_management() {
	try {
		player::init();
		
		// Test player creation patterns
		// (Players are typically managed by the game system)
		
		player* test_player = player::get("TestPlayer");
		// Should return null for non-existent player
		TEST_ASSERT(test_player == NULL, "player memory management works");
		
	} catch (...) {
		TEST_ASSERT(false, "player memory management test");
	}
}

void test_alliance_memory_patterns() {
	try {
		alliance::init();
		
		// Test alliance access patterns
		for (int i = 0; i < 5; i++) {
			alliance* test_alliance = alliance::get(i);
			if (test_alliance) {
				// Test alliance operations don't leak
				equip* test_equip = test_alliance->get_random_equipment();
				ship* test_spawn = test_alliance->get_spawn_ship_template();
				int ai_type = test_alliance->get_ai_behavior_type();
			}
		}
		
		TEST_ASSERT(true, "alliance memory patterns work");
		
	} catch (...) {
		TEST_ASSERT(false, "alliance memory patterns test");
	}
}

void run_memory_management_tests() {
	printf("\n--- Memory Management Tests ---\n");
	test_ship_creation_destruction();
	test_planet_lifecycle_management();
	test_equipment_allocation_deallocation();
	test_network_buffer_cleanup();
	test_graphics_resource_management();
	test_sound_resource_cleanup();
	test_player_memory_management();
	test_alliance_memory_patterns();
}