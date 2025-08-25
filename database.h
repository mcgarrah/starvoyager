/*
	database.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>

struct obj //An object entry in the hash table
{
	char* nam; //Name of object
	FILE* file_stream; //Filestream containing object
	long pos; //Position in file
	long len; //Length of entire object entry
	obj* next; //Pointer to next element in linked list
	obj* prev; //Pointer to previous element in linked list
};

struct reader //Part of a stack of opened readers
{
	FILE* file_stream; //Associated filestream
	reader* next; //Next down in stack
};

class database //Object database spanning multiple files
{
	public:
	static void init(); //Initialise and blank the datastructures
	static void openreader(FILE* dbf); //Opens a data file and parses it into the database
	static void openwriter(FILE* dbf); //Opens a file to write objects to
	static void closereader(); //Closes the current writer
	static void closewriter(); //Closes the current writer
	static void select_database_object(const char* name); //Jump to the named object in the database, returning if it was found or not
	static char* retrieve_attribute(const char* key,char* val); //Get the value of the given attribute (space to put value should be specified)
	static long retrieve_attribute(const char* key); //As above, but value parsed to be an integer
	static void putobject(const char* name); //Write the header of an object to the open writer
	static void store_attribute(const char* key,const char* val); //Write key=value to the open writer
	static void store_attribute(const char* key,long val);

	private:
	static void submitobj(FILE* file_stream,const char* nam,long pos,long len); //Submit an object to the location hash, detailing which file it's in, its name, its position within the file and length within the file (file_stream is a filestream, dodgy workaround necessitates 'void')
	static obj* locateobj(const char* nam); //Locate and return a pointer to the object holder referring to object of given name, NULL if not found
	static int hashstring(const char* str); //Hashes the given string to a 0-1023 value

	static obj* bcks[1024]; //Hash table buckets for object entries
	static char* output_string; //Temporary loaded object store
	static reader* ostk; //Stack of open readers
	static FILE* owrt; //Writer filestream
};

#define MAGIC "SV0040" //Magic number for database files
