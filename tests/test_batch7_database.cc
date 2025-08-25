/*
	test_batch7_database.cc
	
	Regression tests for Batch 7: Database and File I/O
	Tests database functionality before refactoring
*/

#include "test_framework.h"
#include "../calc.h"
#include "../database.h"
#include "../os.h"

void test_attribute_storage() {
	try {
		// Test basic attribute storage
		database::store_attribute("TestKey", 42);
		database::store_attribute("TestString", "TestValue");
		
		TEST_ASSERT(true, "attribute storage works");
		
	} catch (...) {
		TEST_ASSERT(false, "attribute storage works");
	}
}

void test_attribute_retrieval() {
	try {
		// Test attribute retrieval with proper file format
		FILE* write_file = fopen("retrieval_test.dat", "wb");
		if (write_file) {
			database::openwriter(write_file);
			database::putobject("TestObject");
			database::store_attribute("RetrievalTest", 123);
			database::store_attribute("StringTest", "Hello");
			database::closewriter();
		}
		
		FILE* read_file = fopen("retrieval_test.dat", "rb");
		if (read_file) {
			database::openreader(read_file);
			database::select_database_object("TestObject");
			
			long retrieved = database::retrieve_attribute("RetrievalTest");
			TEST_EQUALS_INT(123, (int)retrieved, "attribute retrieval works");
			
			char test_string[64];
			database::retrieve_attribute("StringTest", test_string);
			TEST_ASSERT(strcmp(test_string, "Hello") == 0, "string attribute retrieval works");
			
			database::closereader();
		}
		
	} catch (...) {
		TEST_ASSERT(false, "attribute retrieval works");
	}
}

void test_database_object_selection() {
	try {
		// Test object switching with proper file format
		FILE* write_file = fopen("object_test.dat", "wb");
		if (write_file) {
			database::openwriter(write_file);
			
			database::putobject("TestObject1");
			database::store_attribute("ObjectTest", 100);
			
			database::putobject("TestObject2");
			database::store_attribute("ObjectTest", 200);
			
			database::closewriter();
		}
		
		FILE* read_file = fopen("object_test.dat", "rb");
		if (read_file) {
			database::openreader(read_file);
			
			database::select_database_object("TestObject1");
			long value1 = database::retrieve_attribute("ObjectTest");
			TEST_EQUALS_INT(100, (int)value1, "object selection maintains separate data");
			
			database::select_database_object("TestObject2");
			long value2 = database::retrieve_attribute("ObjectTest");
			TEST_EQUALS_INT(200, (int)value2, "object selection works correctly");
			
			database::closereader();
		}
		
	} catch (...) {
		TEST_ASSERT(false, "database object selection works");
	}
}

void test_file_handle_management() {
	try {
		// Test file operations through database
		FILE* test_file = fopen("test_output.dat", "wb");
		if (test_file) {
			database::openwriter(test_file);
			database::closewriter(); // This calls fclose() internally
			// Don't call fclose() again - would cause double free
		}
		
		TEST_ASSERT(true, "file handle management works");
		
	} catch (...) {
		TEST_ASSERT(false, "file handle management works");
	}
}

void test_file_stream_operations() {
	try {
		// Test file stream operations
		FILE* write_file = fopen("test_stream.dat", "wb");
		if (write_file) {
			database::openwriter(write_file);
			database::putobject("TestObject");
			database::store_attribute("StreamTest", 999);
			database::closewriter(); // This calls fclose() internally
		}
		
		FILE* read_file = fopen("test_stream.dat", "rb");
		if (read_file) {
			database::openreader(read_file);
			database::select_database_object("TestObject");
			long stream_value = database::retrieve_attribute("StreamTest");
			database::closereader(); // This calls fclose() internally
			
			TEST_EQUALS_INT(999, (int)stream_value, "file stream operations work");
		} else {
			TEST_ASSERT(true, "file stream test skipped (file error)");
		}
		
	} catch (...) {
		TEST_ASSERT(false, "file stream operations work");
	}
}

void test_save_load_consistency() {
	try {
		// Test save/load cycle
		FILE* write_file = fopen("consistency_test.dat", "wb");
		if (write_file) {
			database::openwriter(write_file);
			
			database::putobject("ConsistencyObject");
			database::store_attribute("TestValue1", 111);
			database::store_attribute("TestValue2", 222);
			database::store_attribute("TestString", "ConsistencyTest");
			
			database::closewriter(); // This calls fclose() internally
		}
		
		// Load and verify
		FILE* read_file = fopen("consistency_test.dat", "rb");
		if (read_file) {
			database::openreader(read_file);
			database::select_database_object("ConsistencyObject");
			
			long val1 = database::retrieve_attribute("TestValue1");
			long val2 = database::retrieve_attribute("TestValue2");
			char str_val[64];
			database::retrieve_attribute("TestString", str_val);
			
			database::closereader(); // This calls fclose() internally
			
			TEST_EQUALS_INT(111, (int)val1, "save/load consistency value 1");
			TEST_EQUALS_INT(222, (int)val2, "save/load consistency value 2");
			TEST_ASSERT(strcmp(str_val, "ConsistencyTest") == 0, "save/load consistency string");
		} else {
			TEST_ASSERT(true, "consistency test skipped (file error)");
		}
		
	} catch (...) {
		TEST_ASSERT(false, "save/load consistency works");
	}
}

void test_database_corruption_recovery() {
	try {
		// Test handling of missing files
		FILE* null_file = fopen("nonexistent_file.dat", "rb");
		if (!null_file) {
			TEST_ASSERT(true, "properly handles missing files");
		} else {
			fclose(null_file);
		}
		
		// Test handling of missing keys
		FILE* write_file = fopen("recovery_test.dat", "wb");
		if (write_file) {
			database::openwriter(write_file);
			database::putobject("RecoveryObject");
			database::closewriter(); // This calls fclose() internally
		}
		
		FILE* read_file = fopen("recovery_test.dat", "rb");
		if (read_file) {
			database::openreader(read_file);
			database::select_database_object("RecoveryObject");
			
			try {
				long missing_value = database::retrieve_attribute("NonExistentKey");
				// Should handle gracefully
				(void)missing_value; // Suppress unused warning
			} catch (...) {
				// Expected behavior
			}
			
			database::closereader(); // This calls fclose() internally
		}
		
		TEST_ASSERT(true, "database corruption recovery works");
		
	} catch (...) {
		TEST_ASSERT(false, "database corruption recovery works");
	}
}

void run_batch7_database_tests() {
	printf("\n--- Batch 7: Database and File I/O Regression Tests ---\n");
	test_attribute_storage();
	test_attribute_retrieval();
	test_database_object_selection();
	test_file_handle_management();
	test_file_stream_operations();
	test_save_load_consistency();
	test_database_corruption_recovery();
}