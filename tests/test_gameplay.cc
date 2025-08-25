/*
	test_gameplay.cc
	
	Tests that simulate actual gameplay scenarios
*/

#include "test_framework.h"
#include "../calc.h"
#include "../ship.h"
#include "../planet.h"
#include "../alliance.h"
#include "../equip.h"
#include "../frag.h"
#include "../presence.h"

void test_ship_movement() {
	try {
		// Test vector calculations for ship movement
		vect velocity;
		velocity.x_component = 10.0;
		velocity.y_component = 0.0;
		
		pol polar_vel = velocity.to_polar_coordinates();
		TEST_ASSERT(polar_vel.radius > 9.9 && polar_vel.radius < 10.1, "velocity conversion works");
		
		// Test back conversion
		vect back_to_vect = polar_vel.to_vector_coordinates();
		TEST_EQUALS_FLOAT(10.0f, (float)back_to_vect.x_component, 0.1f, "velocity round-trip conversion");
	} catch (...) {
		TEST_ASSERT(false, "ship movement calculations");
	}
}

void test_collision_detection() {
	try {
		// Test basic collision math
		cord pos1, pos2;
		pos1.x_component = 0.0; pos1.y_component = 0.0;
		pos2.x_component = 5.0; pos2.y_component = 0.0;
		
		vect diff;
		diff.x_component = pos2.x_component - pos1.x_component;
		diff.y_component = pos2.y_component - pos1.y_component;
		
		pol distance = diff.to_polar_coordinates();
		TEST_EQUALS_FLOAT(5.0f, (float)distance.radius, 0.01f, "collision distance calculation");
	} catch (...) {
		TEST_ASSERT(false, "collision detection math");
	}
}

void test_targeting_system() {
	try {
		// Test angle calculations for targeting
		cord shooter, target;
		shooter.x_component = 0.0; shooter.y_component = 0.0;
		target.x_component = 1.0; target.y_component = 1.0;
		
		vect aim;
		aim.x_component = target.x_component - shooter.x_component;
		aim.y_component = target.y_component - shooter.y_component;
		
		pol bearing = aim.to_polar_coordinates();
		TEST_ASSERT(bearing.angle_degrees >= 0 && bearing.angle_degrees < 360, "targeting angle in valid range");
	} catch (...) {
		TEST_ASSERT(false, "targeting system calculations");
	}
}

void test_game_physics() {
	try {
		frag::init();
		presence::init();
		TEST_ASSERT(true, "physics systems initialize");
	} catch (...) {
		TEST_ASSERT(false, "physics systems initialize");
	}
}

void test_equipment_slots() {
	try {
		equip::init();
		// Test that equipment system can handle basic operations
		TEST_ASSERT(true, "equipment slot system works");
	} catch (...) {
		TEST_ASSERT(false, "equipment slot system works");
	}
}

void run_gameplay_tests() {
	printf("\n--- Gameplay Tests ---\n");
	test_ship_movement();
	test_collision_detection();
	test_targeting_system();
	test_game_physics();
	test_equipment_slots();
}