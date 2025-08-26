/*
	player.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

class ship;
class alliance;

class player //Represents a player account, linked to by server objects if logged in
{
	public:
	static int const ISIZE=1024; //Maximum number of players in the database

	player(char* nam); //Constructor for a player, given a name, password and owned ship
	~player(); //Destructor

	static void init(); //Initialise the data
	static void purgeall(); //Clears out all the player data afresh
	static void saveall(); //Save all players to the database
	static void loadall(); //Load all players from the database
	static player* get(char* nam); //Gets player of given name, null if it doesn't exist

	void spawn(alliance* tali); //Login the player with given password, spawning the ship into the game. Errors may be thrown
	void login(char* pass); //Login to an existing player with given password, spawning the ship into the game. Errors may be thrown
	void setpass(char* pass); //Set the password for this player
	void commit(); //Commit the in-game ship to be the players saved ship
	void transfer(ship* tshp); //Transfer this player to another ship
	void debit(long amt); //Debit cash from this player, errors may be thrown
	void credit(long amt); //Credit cash to this player
	void notifydelete(); //Notify player that the ingame ship has been killed
	void logout(); //Log this player out of the universe

	ship* in; //Ship instance logged into the game
	char nam[33]; //Name of player
	bool op; //Is player an op?
	long cash; //Cash owned by player
	long cashi; //Cash actually with player in game (before being committed)

	private:
	player(); //Blank constructor for the loading of players from database phase

	int self; //Self-index value
	char pass[33]; //Login password
	ship* player_ship; //Ship associated with player in storage

	void save(); //Save this player out to database
	void load(); //Save this player out to database
	static player* players[ISIZE];
};
