/*
	alliance.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class equip;
class ship;
struct cord;

class alliance //Alliance type
{
	public:
	static const int LIBSIZE=8; //Size of alliance library
	enum {COWARD,ATTACK,DEFENCE,MAKEFRIEND,DESERT,TRAVELHOME,TRAVELFRIEND,TRAVELENEMY,TORPEDO,ATTACKWILD,ATTACKSTRAIGHT,ATTACKFORMATION}; //Different behaviour actions
	enum {TRADE_NONE=0,TRADE_OPEN=1,TRADE_FRIENDS=2,TRADE_CLOSED=3}; //Trading standards constants

	static void init(); //Initialise the datastructures
	static void loadlib(); //Load the alliances from the library in the database
	static void purgeall(); //Purges the alliance data clean
	static alliance* get(int indx); //Returns alliance of given index
	static void maketerritories(); //Generate the territories for all alliances

	bool opposes(alliance* all); //Find out if this alliance opposes the given alliance
	equip* get_random_equipment(); //Get a random equipment item that this alliance stocks
	ship* get_spawn_ship_template(); //Gets an appropriate ship template to spawn from, by examining the quotas
	int get_ai_behavior_type(); //Gets an appropriate AI key to spawn in, also by examining the quotas

	int self; //Self index
	char nam[65]; //Name
	int shpq[32]; //Ship quotas for this alliance
	int aiq[32]; //AI type quotas
	ship* spw; //Default spawn type for this alliance
	int ripo; //Ripoff percentage for purchases (100 is normal)
	int trad; //Trading standards (see enum at top)

	private:
	alliance(int self); //Constructor, giving the alliance its own index in the database
	~alliance(); //Destructor

	void maketerritory(cord seed); //Generate the star systems for this alliance at given seed co-ordinates
	void load(); //Load alliance from database

	int grp; //Ally group
	int nsys; //Number of star systems
	int pinh; //Percentage of inhabited planets
	equip* eqps[16]; //Available equipment	

	static alliance* alliances[LIBSIZE]; //Different available alliances
};
