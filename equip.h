/*
	equip.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class equip //Equipment item
{
	public:
	enum {LIBSIZE=64}; //Maximum equipment library size
	enum {PHASER=1,LAUNCHER=2,TRANSPORTER=3,CLOAK=4,POWER=5,SHIELD=6,SENSOR=7,FUELTANK=8,CARGO=9}; //Different equipment types

	// Equipment type constants
	static const int EQUIPMENT_TYPE_PHASER = 1;
	static const int EQUIPMENT_TYPE_LAUNCHER = 2;
	static const int EQUIPMENT_TYPE_TRANSPORTER = 3;
	static const int EQUIPMENT_TYPE_CLOAK = 4;
	static const int EQUIPMENT_TYPE_POWER = 5;
	static const int EQUIPMENT_TYPE_SHIELD = 6;
	static const int EQUIPMENT_TYPE_SENSOR = 7;
	static const int EQUIPMENT_TYPE_FUELTANK = 8;
	static const int EQUIPMENT_TYPE_CARGO = 9;

	static void init(); //Initialise the equipment datastructures
	static void loadlib(); //Load the equipment library
	static equip* get(int indx); //Get an equipment item by index

	int self; //Self index in the equipment cache
	char nam[65]; //Name
	int equipment_type; //Type (see top enum);
	int mass; //Mass
	int sprite_index; //Associated sprite index
	int color_index; //Colour (makes it a beam weapon)
	int sound_index; //Associated sound index
	int power_requirement; //Power consumption
	int readiness_timer; //Readiness cycle time
	long capacity; //Capacity
	long range; //Range
	int trck; //Tracking power
	int acov; //Angle coverage
	long cost; //Base cost

	private:
	equip(int self); //Constructor, give the equipment self-index value
	void load(); //Load this equipment from the database

	static equip* equips[LIBSIZE]; //Equipment database
};
