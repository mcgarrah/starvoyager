/*
	ship.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include "calc.h"
class alliance;
class planet;
class equip;
class frag;
class player;

struct slot //Internal equipment slot of ship
{
	pol pos; //Position (ang=-1 indicates not external)
	double face; //Facing direction
	equip* item; //Equipment item index
	int rdy; //Readiness
	long cap; //Capacity
};

class ship //Spaceship type in game
{
	public:
	const static int ISIZE=256; //Size of ship index
	const static int LIBSIZE=32; //Size of ship library
	enum {AI_NULL=0,AI_AUTOPILOT=1,AI_PATROLLER=2,AI_INVADER=3,AI_CARAVAN=4,AI_BUDDY=5,AI_FLEET=6}; //AI behaviour types
	enum {PRCH_FUEL,PRCH_ARMS,PRCH_HULL}; //Codes for different purchases

	ship(cord loc,ship* template_ship,alliance* target_alliance,int aity); //Creates a new ship at given place, with given alliance and given ai type, loaded from given library ship
	ship(); //Blank constructor

	~ship(); //Destructor, deletes ship resolving all dependencies (like frags and other ships)

	static void init(); //Initialise all ship datastructures
	static void loadlib(); //Load the library ships
	static void purgeall(); //Clean up the ship index (erasure)
	static void simulateall(); //Simulate the physics of all the ships
	static void behaveall(); //Simulates primitive behaviour for all ships
	static void saveall(); //Save all ships to database
	static void loadall(); //Load all ships from database
	static ship* find_by_index(int indx); //Returns a pointer to a ship of given index
	static ship* libget(int indx); //Returns a ship from the library
	static bool has_available_ship_slot(); //Quickly looks for a free slot if possible for a (non-player) ship, returning true if ok to go ahead and do the hard work

	void turn(int dir); //Turns the ship by specified amount(in 1/100ths of an angle)
	void accel(int dir,bool wrp); //Accelerates the ship positively or negatively (+1 or -1 in dir), wrp deciding if you are willing to make the impulse<=>warp transition
	void shoot(bool torp); //Shoot at target with torpedoes if torp, otherwise phasers
	bool can_detect(ship* target_ship); //Returns true if the given ship is visible to the current one, otherwise false
	bool can_detect(planet* target_planet); //Returns true if the given planet is visible to the current ship, otherwise false
	bool can_detect(frag* tfrg); //Returns true if the given frag is visible to the current ship, otherwise false
	int interact(char* txt,short cmod,short opr,ship* player_ship); //Handles a server request for information/action from this ship, with the given comm mode, operand and player's ship, writing the text into txt and returning the sprite index (-1 if n/a)
	int get_available_cargo_space(); //Returns free mass on board
	void cloak(); //Cloak the ship
	void uncloak(); //Uncloak the ship
	void shieldsup(); //Raise the shields
	void shieldsdown(); //Drop the shields
	void serialize_to_network(int typ,unsigned char* buf); //Get type of data from ship into a network buffer
	bool detect_collision(cord fragment_location,vect fragment_velocity); //Given an intruder location and vector, determines if a collision occurs, returning true if it does, false otherwise
	void take_damage(int mag,cord fragment_location,vect fragment_velocity,ship* src); //Damages the ship with given magnitude, also location and vector of striking object, plus the source of the attack (expected to be frag owner)
	void assign(player* assigned_player); //Sets the ship up for entrance of given player
	long purchase(int prch,short ripo,bool buy);
	long purchase(equip* prch,int ripo,bool buy); //Returns the cost of a given purchase type at given rip-off index, with buy set actually deducts the cash
	void transport(planet* to);
	void transport(ship* to); //Attempts transport to destination, deducting power and generating sound as appropriate, error may be thrown if not possible
	void save(); //Saves the ship data to database
	void load(); //Load ship data from database, object should already have been selected
	void insert(); //Insert ship into the game
	void insert(int self); //Insert ship into the game, with given index in slot demanded (say for loading a universe)
	void update_equipment_references(); //Resolve key equipment within the ship, should be called whenever the equipment loadout is changed at all

	int self; //Self index in the ship database
	player* assigned_player; //Pointer to player associated, null if none
	alliance* all; //Alliance
	cord loc; //Co-ordinates 
	char cls[65]; //Ship class 
	int typ; //Ship type (index in library)
	int spr; //Sprite index
	ship* friendly_target; //Friend target
	ship* enemy_target; //Enemy target
	planet* planet_target; //Planetary target
	int aity; //AI type
	vect mov; //Current actual velocity vector

	private:
	ship(int self); //Constructor that loads ship of given index out of the database into the universe

	void physics(); //Handles physics of motion on this ship
	void navigate_to_planet(planet* target_planet); //Autonavigate to given planet
	void follow(ship* target_ship); //Follow given ship
	void execute_attack_maneuvers(ship* target_ship,int str); //Run attack pattern on given ship, given a strobing value
	void resolve_object_references(); //Extension to load that only resolves the links between ships
	void maintain(); //Does routine work such as recharging shields and power, reloading weapons
	void execute_ai_behavior(); //Execute characteristic behaviour for this ship
	void act(int actn); //Perform given action type, called by behave()
	ship* find_hostile_target(); //Return a nearby hostile ship, null if none found
	ship* find_allied_ship(); //Return a nearby allied ship, null if none found
	void alert_nearby_ships(); //Get all ships in the area to come attack this ship, usually as a result of friendly fire
	void handle_weapon_targeting(int str); //Makes AI decisions concerning phaser fire, given a strobing value

	static ship* ships[ISIZE]; //Main ship list
	static ship* lib[LIBSIZE]; //Ship library
	static int master_strobe; //Master strobe and individual strobe, used for periodic behaviours

	int w[36],h[36]; //Width and height (halved)
	int fspr; //Frag sprite
	int fsnd; //Frag sound
	int dsnd; //Destruction sound
	pol vel; //Velocity
	int turn_rate; //Turning speed
	double max_impulse_speed; //Maximum impulse velocity (<100)
	double impulse_acceleration; //Impulse acceleration
	double max_warp_speed; //Maximum warp velocity
	double warp_acceleration; //Warp acceleration
	int mass; //Mass
	int hull_integrity; //Hull integrity
	int max_hull_integrity; //Maximum hull integrity
	
	slot* power_plant; //Power plant
	slot* shield_generator; //Shield generator
	slot* sensor_array; //Sensor platform
	slot* cloaking_device; //Cloaking device
	slot* fuel_tank; //Fuel tank
	
	int selected_equipment_index; //Equipment selection
	slot slots[32]; //Equipment
	bool mass_locked; //Mass locked?
	bool is_crippled; //Crippled?
};
