/*
	sockhelper.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <string.h>
#include "constants.h"
#include "error.h"
#include "sockhelper.h"

sockhelper::sockhelper(TCPsocket sock)
{
	this->sock=sock;
	poll=SDLNet_AllocSocketSet(1);
	if(!poll)
		throw error("Error creating a socket set");
	SDLNet_TCP_AddSocket(poll,sock);
	ins=0;
	outs=0;	
	take=0;
	cnt=0;
	blck=false;
	alrm=NULL;
}

sockhelper::~sockhelper()
{
	SDLNet_FreeSocketSet(poll);
	if(alrm)
		SDL_RemoveTimer(alrm);
}

void sockhelper::pump()
{
	int r; //Data received value

	blck=false;
	alrm=SDL_AddTimer(1000,alarmcallback,(void*)this);
	if(outs>0)
	{
		if(SDLNet_TCP_Send(sock,out,outs)<outs)
			throw error("Other end dropped connection (TCP_Send)");
		cnt+=outs;
		outs=0;
	}
	SDL_RemoveTimer(alrm);
	SDLNet_CheckSockets(poll,0);
	if(SDLNet_SocketReady(sock) && ins<1024)
	{
		r=SDLNet_TCP_Recv(sock,in+ins,1024-ins);
		if(r<=0)
			throw error("Other end dropped connection (TCP_Recv)");
		cnt+=r;
		ins+=r;
	}
	alrm=NULL;
	if(blck)
		throw error("Socket blocked: probably a laggy/hung/cracked client");
}

void sockhelper::send(unsigned char* data,int len)
{
	if(outs+len>=1024)
	{
		try
		{
			pump();
		}
		catch(error it)
		{
		}
	}
	for(int i=0;i<len && outs<1024;i++)
	{
		out[outs]=data[i];
		outs++;
	}
}

unsigned char* sockhelper::request(int len)
{
	if(len>ins)
		return NULL;
	else
	{
		take=len;
		return in;
	}
}

void sockhelper::suck()
{
	if(take>0)
	{
		for(int i=take;i<ins;i++)
			in[i-take]=in[i];
		ins-=take;
			take=0;
	}
}

long sockhelper::getcount()
{
	long out; //Value to return

	out=cnt;
	cnt=0;
	return out;
}

Uint32 sockhelper::alarmcallback(Uint32 dly,void* from)
{
	static_cast<sockhelper*>(from)->blck=true;
	return 0;
}
