/*
	client.cc	
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <cstring>
#include "calc.h"
#include "sockhelper.h"
#include "protocol.h"
#include "sound.h"
#include "presence.h"
#include "camera.h"
#include "error.h"
#include "interface.h"
#include "graphic.h"
#include "client.h"

void client::init()
{
	sock=NULL;
	socket_helper=NULL;
	edit=false;
}

void client::stop()
{
	if(sock)
	{
		try {
			SDLNet_TCP_Close(sock);
		} catch(...) {
			// Ignore socket close errors during cleanup
		}
		sock=NULL;
	}
	if(socket_helper)
	{
		try {
			delete socket_helper;
		} catch(...) {
			// Ignore helper deletion errors during cleanup
		}
		socket_helper=NULL;
	}
}

void client::connect(const char* host)
{
	IPaddress serv; //Server address

	SDLNet_ResolveHost(&serv,host,PORT);
	if(serv.host==INADDR_NONE)
		throw error("Couldn't resolve hostname");
	sock=SDLNet_TCP_Open(&serv);
	if(!sock)
		throw error("Could not connect to server");
	try {
		socket_helper=new sockhelper(sock);
	} catch(...) {
		SDLNet_TCP_Close(sock);
		sock=NULL;
		throw error("Failed to create socket helper");
	}
	socket_helper->send((unsigned char*)SIGN,strlen(SIGN));
	btck=0;
}

void client::flush()
{
	socket_helper->pump();
}

void client::process_network_messages()
{
	unsigned char otyp; //Incoming object type
	unsigned char* network_buffer; //Incoming buffer to use
	sound* snd; //Sound to play
	graphic* console_sprite_index; //Console sprite
	int n; //Array subscript to access
	long cbnd; //Client bandwidth usage to report
	presence* from; //Sound source if applicable
	int len; //Length of console/message text if applicable
	char txt[1025]; //Text to print to console
	bool exl; //Loop exiting flag

	socket_helper->pump();
	btck=(btck+1)%24;
	if(btck==0)
	{
		cbnd=socket_helper->getcount();
		action(CLIENT_BANDWIDTH,cbnd);
	}
	exl=false;
	while(!exl && socket_helper->request(1))
	{
		otyp=*socket_helper->request(1);
		switch(otyp)
		{
			case SERV_SELF:
			network_buffer=socket_helper->request(SERV_SELF_SZ);
			if(network_buffer)
			{
				presence::update_display_data(network_buffer);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_NEW:
			network_buffer=socket_helper->request(SERV_NEW_SZ);
			if(network_buffer)
			{
				presence::update_display_data(network_buffer);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_NAME:
			network_buffer=socket_helper->request(SERV_NAME_SZ);
			if(network_buffer)
			{
				presence::update_display_data(network_buffer);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_UPD:
			network_buffer=socket_helper->request(SERV_UPD_SZ);
			if(network_buffer)
			{
				presence::update_display_data(network_buffer);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_DEL:
			network_buffer=socket_helper->request(SERV_DEL_SZ);
			if(network_buffer)
			{
				presence::update_display_data(network_buffer);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_HILIGHT:
			network_buffer=socket_helper->request(SERV_HILIGHT_SZ);
			if(network_buffer)
			{
				presence::update_display_data(network_buffer);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_SND:
			network_buffer=socket_helper->request(SERV_SND_SZ);
			if(network_buffer)
			{
				snd=sound::get(calc::dattoint(network_buffer+1));
				if(snd)
					snd->play(1);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_NOISE:
			network_buffer=socket_helper->request(SERV_NOISE_SZ);
			if(network_buffer)
			{
				snd=sound::get(calc::dattoint(network_buffer+1));
				n=calc::dattoint(network_buffer+3);
				if(n>=0 && n<presence::ISIZE)
					from=presence::get(n);
				else
					from=NULL;
				if(snd && from)
					camera::noise(snd,from);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_SHAKE:
			network_buffer=socket_helper->request(SERV_SHAKE_SZ);
			if(network_buffer)
			{
				camera::shake(calc::dattoint(network_buffer+1));
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_CONS:
			network_buffer=socket_helper->request(3);
			if(network_buffer)
			{
				edit=false;
				len=calc::dattoint(network_buffer+1);
				if(len>1024)
					throw error("Overflow from server in SERV_CONS");
				network_buffer=socket_helper->request(3+len);
				if(network_buffer)
				{
					memcpy(txt,network_buffer+3,len);
					txt[len]='\0';
					interface::printtocons("%s\n",txt);
					socket_helper->receive_data();
				}
				else
					exl=true;
			}
			else
				exl=true;
			break;

			case SERV_READLN:
			network_buffer=socket_helper->request(SERV_READLN_SZ);
			if(network_buffer)
			{
				edit=true;
				if(network_buffer[1])
					hide=true;
				else
					hide=false;
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_MESG:
			network_buffer=socket_helper->request(3);
			if(network_buffer)
			{
				len=calc::dattoint(network_buffer+1);
				if(len>128)
					throw error("Overflow from server in SERV_MESG");
				network_buffer=socket_helper->request(3+len);
				if(network_buffer)
				{
					memcpy(txt,network_buffer+3,len);
					txt[len]='\0';
					interface::printtomesg("%s",txt);
					socket_helper->receive_data();
				}
				else
					exl=true;
			}
			else
				exl=true;
			break;

			case SERV_CSPR:
			network_buffer=socket_helper->request(3);
			if(network_buffer)
			{
				console_sprite_index=graphic::get(calc::dattoint(network_buffer+1));
				if(console_sprite_index)
					interface::spritetocons(console_sprite_index);
				socket_helper->receive_data();
			}
			else
				exl=true;
			break;

			case SERV_FLOOD:
			network_buffer=socket_helper->request(SERV_FLOOD_SZ);
			if(network_buffer)
			{
				socket_helper->receive_data();
			}
			socket_helper->pump();
			break;

			default:
			error::debug("Strange default packet",otyp);
			break;
		}
	}
	socket_helper->pump();
	if(edit)
		readln();
}

void client::action(int typ,long operation_code)
{
	unsigned char buf[3]; //Outgoing buffer

	if(operation_code>32767)
		operation_code=32767;
	if(operation_code<-32766)
		operation_code=-32766;
	buf[0]=(unsigned char)typ;
	calc::inttodat(operation_code,buf+1);
	socket_helper->send(buf,3);
}

void client::readln()
{
	char txt[65]; //Readline text

	if(interface::getline(txt,hide))
	{
		edit=false;
		for(int i=0;i<64;i++)
		{
			action(CLIENT_CHAR,txt[i]);
			if(txt[i]=='\0')
				break;
		}
	}
}

bool client::edit;
bool client::hide;
TCPsocket client::sock;
sockhelper* client::socket_helper;
int client::btck;
