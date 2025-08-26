/*
	planet.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class sprite;
class alliance;
class equip;
class ship;
class graphic;
struct cord;

class planet //Planet object
{
	public:
	const static int ISIZE=256; //Count of available planets in the index
	enum {STAR=0,INHABITED=1,UNINHABITED=2}; //Planetary object types

	planet(char* nam,cord put,int typ,alliance* all); //Spawn a new planet
	~planet(); //Destructor, resolves dependencies

	static void init(); //Initialise datastructures for the planet module
	static void purgeall(); //Cleans up and purges planet data, used after a game
	static planet* find_by_index(int indx); //Return the planet of given index
	static planet* find_random_planet(alliance* target_alliance); //Pick a random planet of given alliance
	static planet* find_allied_planet(alliance* target_alliance); //Pick a random planet allied to given alliance
	static planet* find_hostile_planet(alliance* target_alliance); //Pick a random planet hostile to given alliance
	static bool masslock(cord loc); //Return whether or not given location is mass-locked by planets
	static void saveall(); //Save all the planets to the database
	static void loadall(); //Load all the planets from the database
	static void generatename(char* put); //Generate a planetary name, put it in put
	static void shipyards(); //Randomly spawn ships

	int interact(char* txt,short cmod,short opr,ship* player_ship); //Handles a server request for information/action from this planet, with the given comm mode, operand and player's ship, writing the text into txt and returning the sprite index (-1 if n/a)
	void serialize_to_network(int typ,unsigned char* buf); //Get type of data from planet into a network buffer

	int self; //Self index
	int spr; //Sprite to display for this
	cord loc; //Location
	alliance* all; //Alliance
	int typ; //Planet type 1:Sun 2:Uninhabited 3:Inhabited

	private:
	planet(int self); //Constructor, given self index loads the planet from the database

	void save(); //Save this planet to the database
	void load(); //Load this planet from the database
	void shipyard(); //Spawn a ship from the library at this planet

	static planet* planets[ISIZE]; //Planets in the index

	char nam[65]; //Name
	int rot; //Version of sprite to use
	equip* sold[8]; //Sold equipment
};
