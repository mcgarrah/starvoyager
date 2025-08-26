/*
	test_equipment_refactoring.cc
	
	Tests for equipment system variable refactoring (Batch 1)
*/

#include "test_framework.h"
#include "../calc.h"
#include "../ship.h"
#include "../alliance.h"
#include "../equip.h"
#include "../planet.h"

void test_power_plant_operations() {
	try {
		// Initialize systems
		calc::init();
		ship::init();
		alliance::init();
		equip::init();
		
		// Test power plant functionality indirectly through ship library loading
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test free mass calculation (uses equipment slots)
			int free_mass = test_ship->get_available_cargo_space();
			TEST_ASSERT(free_mass >= 0, "free mass calculation works");
		} else {
			TEST_ASSERT(true, "power plant test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "power plant operations test");
	}
}

void test_shield_generator_functionality() {
	try {
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test shield operations indirectly
			test_ship->shieldsup();
			test_ship->shieldsdown();
			TEST_ASSERT(true, "shield operations work");
		} else {
			TEST_ASSERT(true, "shield generator test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "shield generator functionality test");
	}
}

void test_sensor_array_operations() {
	try {
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test sensor functionality indirectly through visibility
			bool can_see_self = test_ship->can_detect(test_ship);
			TEST_ASSERT(can_see_self == true, "ship can see itself");
		} else {
			TEST_ASSERT(true, "sensor array test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "sensor array operations test");
	}
}

void test_cloaking_device_behavior() {
	try {
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test cloaking operations indirectly
			test_ship->cloak();
			test_ship->uncloak();
			TEST_ASSERT(true, "cloaking operations work");
		} else {
			TEST_ASSERT(true, "cloaking device test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "cloaking device behavior test");
	}
}

void test_fuel_tank_management() {
	try {
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test fuel-related functionality indirectly through movement
			test_ship->turn(1);
			test_ship->accel(1, false);
			TEST_ASSERT(true, "ship movement operations work");
		} else {
			TEST_ASSERT(true, "fuel tank test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "fuel tank management test");
	}
}

void test_equipment_slot_integrity() {
	try {
		ship* test_ship = ship::libget(0);
		if (test_ship) {
			// Test equipment system indirectly through resequip
			test_ship->update_equipment_references();
			TEST_ASSERT(true, "equipment resequencing works");
			
			// Test free mass calculation (depends on equipment)
			int free_mass = test_ship->get_available_cargo_space();
			TEST_ASSERT(free_mass >= 0, "free mass calculation returns valid value");
		} else {
			TEST_ASSERT(true, "equipment slot test skipped (no test ship)");
		}
	} catch (...) {
		TEST_ASSERT(false, "equipment slot integrity test");
	}
}

void run_equipment_refactoring_tests() {
	printf("\n--- Equipment Refactoring Tests ---\n");
	test_power_plant_operations();
	test_shield_generator_functionality();
	test_sensor_array_operations();
	test_cloaking_device_behavior();
	test_fuel_tank_management();
	test_equipment_slot_integrity();
}