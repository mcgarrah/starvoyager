/*
	client.h	
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <SDL_net.h>

class sockhelper;

class client //Module to handle connection to servers
{
	public:
	static void init(); //Initialise the client datastructures
	static void stop(); //Clean up and terminate client, disconnecting from the server
	static void connect(const char* host); //Connect to given host
	static void flush(); //Flush connection buffers
	static void process_network_messages(); //Run the client for a cycle, polling for new objects and acting on them
	static void action(int typ,long operation_code); //Send an action to the server, given action number and operand

	private:
	static void readln(); //Handles a server readline request until it's done, with password hiding style optional

	static bool edit; //In edit mode?
	static bool hide; //In passworded edit?
	static TCPsocket sock; //TCP socket to server
	static sockhelper* socket_helper; //Socket wrapper
	static int btck; //Byte count ticker
	static int sync; //Synchronisation counter
};
