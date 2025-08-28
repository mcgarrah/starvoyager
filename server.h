/*
	server.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL_net.h>
#include <stdio.h>

class ship;
class sockhelper;
class player;

class server //Controls connections to players
{
	public:
	static int const ISIZE=32; //Maximum number of connections

	static void init(); //Initialise server datastructures
	static void start(bool locg); //Start the server listening, in local game mode or not
	static void stop(); //Stop the server and clean up all data
	static void cycle(); //Run one tick of the server
	static void notifydelete(player* ply); //Tells the server system that a given player has been deleted
	static void notifykill(player* ply); //Tells the server system that the given player has just been destroyed (does final preperations before ship is deleted)
	static void hail(player* fr,player* to,const char* msg); //Hail from one player to another with given message
	static void bulletin(const char* fmt,...); //Global bulletin to all players logged in
	static void registernoise(ship* fr,int snd); //Register a noise emitted from the given ship, of given sound index
	static void registersound(ship* to,int snd); //Register a sound of given index, internal to the given ship
	static void registershake(ship* to,int mag); //Command given ship to shake
	static void quitsignal(int sig); //Signal handle to request server quit

	private:
	server(int self,TCPsocket sock); //Constructor for a client handler, give it the player number and the new incoming socket
	~server(); //Destructor, closes socket

	void log(const char* fmt,...); //Output to the server log from this connection
	void process_network_messages(); //Poll for client data and act on it	
	void action(int typ,short operation_code); //Process an action from the client, given action type and operand
	void changecmod(int operation_code); //Change to given client<->server console mode
	void cons(int operation_code); //Receive a console impulse from the client, number given as operand
	void requestline(bool hide); //Request a line of input from the client console, if secret (password mode) then specify
	void input(); //Register that readline input has completed from the client, it's time to process it
	void printtocons(const char* fmt,...); //Print to the client's console display
	void spritetocons(int indx); //Bring up given index of sprite on clients console
	void printtomesg(const char* fmt,...); //Print an instant message to the client
	void uploads(); //Handle presence uploads to the client
	void uploadplanets(); //Handle planet uploads
	void uploadships(); //Handle ship uploads
	void uploadfrags(); //Handle frag uploads
	void kill(); //Kill this player, and restart his/her game
	void hilight(ship* tshp); //Hilight the given ship to this player

	static server* connections[ISIZE]; //Server objects for each possible client
	static long tcks; //Server ticks so far
	static TCPsocket lstn; //Listener socket
	static bool qsig; //Quit signal?
	static FILE* logf; //Log file
	static bool locg; //Local game?
	static bool locl; //Local lock (one player only)

	int self; //Player number
	TCPsocket sock; //Socket associated with this player connection
	sockhelper* socket_helper; //Helper for above
	player* ply; //Player associated with this connection
	bool auth; //Authorised yet?
	int communication_mode; //Current communications mode; just statekeeping
	char network_buffer[65]; //Incoming text input buffer
	char tpas[33]; //Temporary password store, for double confirmation
	bool acth[32]; //Action history, prevents multiple repetition cheating
	bool* shpu;
	bool* plnu;
	bool* frgu; //Arrays to indicate if object uploading to client has been dealt with
	int lsnd; //Last sound registered, to prevent repetitions
	int foc; //Focus target of this player's clientside presence
	int urat; //Upload rate divider
	long cbnd; //Client bandwidth report
	int tout; //Connection timeout
};
