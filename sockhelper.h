/*
	sockhelper.h
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/
#include <SDL_net.h>

class sockhelper //Class that helps with socket operations, giving them much easier non-blocking semantics
{
	public:
	sockhelper(TCPsocket sock); //Constructor, give it the socket to help with
	~sockhelper(); //Destructor, cleans up any dynamic data

	void pump(); //Pump scheduled outgoing data, fill incoming buffer
	void send(unsigned char* data,int len); //Schedule data of given length to be sent
	unsigned char* request(int len); //Request incoming data of given length, returns null if not enough in the buffer
	void receive_data(); //Way to roll on the stream after a request has been used and totally done with
	long getcount(); //Get the socket throughput byte count, and reset it to zero

	private:
	static Uint32 alarmcallback(Uint32 dly,void* from); //Callback handling the alarm when pumping blocks for too long
	TCPsocket sock; //Socket to help with
	SDLNet_SocketSet poll; //For polling input
	unsigned char in[2048]; //Input buffer
	int ins; //Size of input buffer used
	unsigned char out[1024]; //Output buffer
	int outs; //Size of output buffer used
	int take; //Record how much was requested (for the suck operation)
	long cnt; //Count of bytes come through the socket
	SDL_TimerID alrm; //The blocking alarm being set
	bool blck; //Flag to indicate that the socket outgoing has blocked and should be deleted as soon as safe
};
