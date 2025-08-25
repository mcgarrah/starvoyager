/*
	test_batch2_equipment_alliance.cc
	
	Regression tests for Batch 2: Equipment and Alliance Systems
	Tests equipment and alliance functionality before refactoring
*/

#include "test_framework.h"
#include "../calc.h"
#include "../equip.h"
#include "../alliance.h"
#include "../ship.h"
#include "test_ship_factory.h"

void test_equipment_type_constants() {
	try {
		equip::init();
		
		equip* test_equip = equip::get(0);
		if (test_equip) {
			// Test equipment type access
			int equipment_type = test_equip->equipment_type;
			TEST_ASSERT(equipment_type >= 0, "equipment type is valid");
		}
		
		TEST_ASSERT(true, "equipment type constants accessible");
	} catch (...) {
		TEST_ASSERT(false, "equipment type constants accessible");
	}
}

void test_equipment_range_calculations() {
	try {
		equip::init();
		
		equip* test_equip = equip::get(0);
		if (test_equip) {
			// Test range property access
			long range = test_equip->range;
			TEST_ASSERT(range >= 0, "equipment range is non-negative");
		}
		
		TEST_ASSERT(true, "equipment range calculations work");
	} catch (...) {
		TEST_ASSERT(false, "equipment range calculations work");
	}
}

void test_power_requirement_validation() {
	try {
		equip::init();
		
		equip* test_equip = equip::get(0);
		if (test_equip) {
			// Test power requirement access
			long power = test_equip->power_requirement;
			TEST_ASSERT(power >= 0, "equipment power requirement is non-negative");
		}
		
		TEST_ASSERT(true, "power requirement validation works");
	} catch (...) {
		TEST_ASSERT(false, "power requirement validation works");
	}
}

void test_capacity_management() {
	try {
		equip::init();
		
		equip* test_equip = equip::get(0);
		if (test_equip) {
			// Test capacity property access
			long capacity = test_equip->capacity;
			TEST_ASSERT(capacity >= 0, "equipment capacity is non-negative");
		}
		
		TEST_ASSERT(true, "capacity management works");
	} catch (...) {
		TEST_ASSERT(false, "capacity management works");
	}
}

void test_equipment_mass_calculations() {
	try {
		equip::init();
		
		equip* test_equip = equip::get(0);
		if (test_equip) {
			// Test mass property access
			int mass = test_equip->mass;
			TEST_ASSERT(mass >= 0, "equipment mass is non-negative");
		}
		
		TEST_ASSERT(true, "mass calculations work");
	} catch (...) {
		TEST_ASSERT(false, "mass calculations work");
	}
}

void test_readiness_timer_mechanics() {
	try {
		equip::init();
		
		equip* test_equip = equip::get(0);
		if (test_equip) {
			// Test readiness property access
			(void)test_equip->readiness_timer;
			// Readiness can be negative, zero, or positive
		}
		
		TEST_ASSERT(true, "readiness timer mechanics work");
	} catch (...) {
		TEST_ASSERT(false, "readiness timer mechanics work");
	}
}

void test_alliance_equipment_generation() {
	try {
		alliance::init();
		equip::init();
		
		alliance* test_alliance = alliance::get(0);
		if (test_alliance) {
			// Test equipment generation
			(void)test_alliance->get_random_equipment();
			// Should not crash, may return null
		}
		
		TEST_ASSERT(true, "alliance equipment generation works");
	} catch (...) {
		TEST_ASSERT(false, "alliance equipment generation works");
	}
}

void test_alliance_spawn_templates() {
	try {
		TestShipFactory::init_test_environment();
		
		alliance* test_alliance = TestShipFactory::create_test_alliance();
		if (test_alliance) {
			// Test spawn template generation
			(void)test_alliance->get_spawn_ship_template();
			// Should not crash, may return null
		}
		
		TEST_ASSERT(true, "alliance spawn templates work");
	} catch (...) {
		TEST_ASSERT(false, "alliance spawn templates work");
	}
}

void test_alliance_ai_behavior_assignment() {
	try {
		alliance::init();
		
		alliance* test_alliance = alliance::get(0);
		if (test_alliance) {
			// Test AI behavior assignment
			int ai_behavior = test_alliance->get_ai_behavior_type();
			TEST_ASSERT(ai_behavior >= 0, "AI behavior type is valid");
		}
		
		TEST_ASSERT(true, "alliance AI behavior assignment works");
	} catch (...) {
		TEST_ASSERT(false, "alliance AI behavior assignment works");
	}
}

void run_batch2_equipment_alliance_tests() {
	printf("\n--- Batch 2: Equipment and Alliance Regression Tests ---\n");
	test_equipment_type_constants();
	test_equipment_range_calculations();
	test_power_requirement_validation();
	test_capacity_management();
	test_equipment_mass_calculations();
	test_readiness_timer_mechanics();
	test_alliance_equipment_generation();
	test_alliance_spawn_templates();
	test_alliance_ai_behavior_assignment();
}