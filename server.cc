/*
	server.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "sockhelper.h"
#include "calc.h"
#include "error.h"
#include "protocol.h"
#include "ship.h"
#include "frag.h"
#include "planet.h"
#include "alliance.h"
#include "constants.h"
#include "player.h"
#include "os.h"
#include "server.h"

struct octets //Structure for extraction of octets from an IP
{
	unsigned char o1;
	unsigned char o2;
	unsigned char o3;
	unsigned char o4;
};

void server::init()
{
	for(int i=0;i<ISIZE;i++)
		connections[i]=NULL;
	lstn=NULL;
}

void server::start(bool locg)
{
	IPaddress serv; //Server listening interface

	tcks=0;
	qsig=false;

	try
	{
		logf=os::openpersonal("log","a");
	}
	catch(error it)
	{
		logf=stdout;
	}
	
	SDLNet_ResolveHost(&serv,NULL,PORT);
	lstn=SDLNet_TCP_Open(&serv);
	if(!lstn)
		throw error("Can't open listening socket");
	// fprintf(logf,"%s: Server started listening on port %d\n",os::gettime(),(int)PORT);
	fprintf(logf,"%s: Server started listening on port %hd\n",os::gettime(),PORT);
	fflush(logf);
	server::locg=locg;
	locl=false;
}

void server::stop()
{
	for(int i=0;i<ISIZE;i++)
		if(connections[i])
			delete connections[i];
	if(lstn!=NULL)
	{
		SDLNet_TCP_Close(lstn);
		lstn=NULL;
		fprintf(logf,"%s: Server shutdown\n",os::gettime());
		fflush(logf);
	}
	if(logf && logf!=stdout)
		fclose(logf);
}

void server::cycle()
{
	TCPsocket sock; //Sample socket

	tcks++;
	if(tcks>32766)
		tcks=1;
	sock=NULL;
	if(!locl)
		sock=SDLNet_TCP_Accept(lstn);
	if(sock)
	{
		for(int i=0;i<ISIZE;i++)
		{
			if(!connections[i])
			{
				connections[i]=new server(i,sock);
				if(locg)
					locl=true;
				break;
			}
		}
	}
	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i])
		{
			try
			{
				connections[i]->poll();
				connections[i]->uploads();
			}
			catch(error it)
			{
				connections[i]->log("Dropped from the game: %s",it.str);
				delete connections[i];
				connections[i] = NULL;
			}
		}
	}
	if(qsig)
		throw error("Server caught quit signal");
}

void server::notifydelete(player* ply)
{
	for(int i=0;i<ISIZE;i++)
		if(connections[i] && connections[i]->ply==ply)
			connections[i]->ply=NULL;
}

void server::notifykill(player* ply)
{
	for(int i=0;i<ISIZE;i++)
		if(connections[i] && connections[i]->ply==ply && connections[i]->ply->in)
			connections[i]->kill();
}


void server::hail(player* fr,player* to,const char* msg)
{
	char txt[256]; //Communications text
	char* frnm; //Name of from

	if(!to)
		return;

	frnm=NULL;
	if(fr)
	{
		frnm=fr->nam;
		if(!frnm) {
			if(fr->in)
				frnm=fr->in->cls;
			else
				frnm=const_cast<char*>("");
		}
		snprintf(txt,sizeof(txt),"%s: %s",frnm,msg);
	}
	else
		snprintf(txt,sizeof(txt),"%s",msg);

	for(int i=0;i<ISIZE;i++)
	{	
		if(connections[i] && (connections[i]->ply==to || (fr && connections[i]->ply==fr)))
		{
			if(fr && fr->in)
				connections[i]->hilight(fr->in);
			connections[i]->printtomesg("%s",txt);
			if(connections[i]->ply==to)
			{
				if(connections[i]->ply->in)
					server::registersound(connections[i]->ply->in,SND_COMM);
				{
					char sanitized[256];
					int k=0;
					for(int j=0; j<255 && txt[j]!='\0'; j++)
					{
						if(txt[j]>=32 && txt[j]<=126 && txt[j]!='\n' && txt[j]!='\r')
							sanitized[k++] = txt[j];
					}
					sanitized[k] = '\0';
					connections[i]->log("Received hail \"%s\"",sanitized);
				}
			}
		}
	}
}

void server::bulletin(const char* fmt,...)
{
	char buf[132]; //Outgoing buffer
	va_list fmts;

	if(fmt[0]!='\0')
	{
		va_start(fmts,fmt);
		int len = vsnprintf(buf,sizeof(buf),fmt,fmts);
		va_end(fmts);
		if(len < 0 || len >= (int)sizeof(buf)) {
			throw error("Bulletin message too long");
		}

		for(int i=0;i<ISIZE;i++)
			if(connections[i])
				connections[i]->printtomesg("%s",buf);
		
		{
			char sanitized_bulletin[132];
			int k=0;
			for(int j=0; j<131 && buf[j]!='\0'; j++)
			{
				// if(buf[j]>=32 && buf[j]<=126 && buf[j]!='\n' && buf[j]!='\r')
				if((buf[j]>=32 && buf[j]<=126) || buf[j]==' ')
					sanitized_bulletin[k++] = buf[j];
			}
			sanitized_bulletin[k] = '\0';
			fprintf(logf,"%s: Bulletin - %s\n",os::gettime(),sanitized_bulletin);
		}
		fflush(logf);
	}
}

void server::registernoise(ship* fr,int snd)
{
	unsigned char buf[SERV_NOISE_SZ]; //Buffer for sending sound

	buf[0]=SERV_NOISE;
	calc::inttodat(snd,buf+1);
	calc::inttodat(ship2pres(fr->self),buf+3);

	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i] && connections[i]->ply && connections[i]->ply->in && connections[i]->lsnd!=snd && connections[i]->ply->in->can_detect(fr))
		{	
			connections[i]->lsnd=snd;	
			if(connections[i]->hlpr)
				connections[i]->hlpr->send(buf,SERV_NOISE_SZ);
		}
	}
}

void server::registersound(ship* to,int snd)
{
	unsigned char buf[SERV_SND_SZ]; //Buffer for sending sound

	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i] && connections[i]->ply && connections[i]->ply->in==to && connections[i]->lsnd!=snd)
		{
			connections[i]->lsnd=snd;
			buf[0]=SERV_SND;
			calc::inttodat(snd,buf+1);
			if(connections[i]->hlpr)
				connections[i]->hlpr->send(buf,SERV_SND_SZ);
		}

	}
}

void server::registershake(ship* to,int mag)
{
	unsigned char buf[SERV_SHAKE_SZ]; //Buffer for sending shake

	for(int i=0;i<ISIZE;i++)
	{
		if(connections[i] && connections[i]->ply && connections[i]->ply->in==to)
		{
			buf[0]=SERV_SHAKE;
			calc::inttodat(mag,buf+1);
			if(connections[i]->hlpr)
				connections[i]->hlpr->send(buf,SERV_SHAKE_SZ);
		}

	}
}

void server::quitsignal(int sig)
{
	qsig=true;
}

server::server(int self,TCPsocket sock)
{
	unsigned long ip; //ip of connecting client
	octets* ip_octets; //Ip as octets, for writing to Ip record

	this->self=self;
	this->sock=sock;
	hlpr=NULL;
	hlpr=new sockhelper(sock);
	if(!hlpr)
		throw error("Failed to allocate sockhelper");
	hlpr->pump();
	auth=false;
	lsnd=-1;
	inpb[0]='\0';
	ply=NULL;
	foc=-1;
	shpu=new bool[ship::ISIZE];
	for(int i=0;i<ship::ISIZE;i++)
		shpu[i]=false;
	plnu=new bool[planet::ISIZE];
	for(int i=0;i<planet::ISIZE;i++)
		plnu[i]=false;
	frgu=new bool[frag::ISIZE];
	for(int i=0;i<frag::ISIZE;i++)
		frgu[i]=false;
	log("Connection opened");
	urat=10;
	ip=SDLNet_TCP_GetPeerAddress(sock)->host;
	ip_octets=(octets*)&ip;
	if(ip_octets->o1==127 && ip_octets->o2==0 && ip_octets->o3==0 && ip_octets->o4==1)
		cbnd=999999999;
	else
		cbnd=0;
	tout=0;
}

server::~server()
{
	if(ply)
		ply->logout();

	log("Connection terminated");
	connections[self]=NULL;
	if(hlpr)
		delete hlpr;
	SDLNet_TCP_Close(sock);
}

void server::log(const char* fmt,...)
{
	unsigned long ip; //ip of connecting client
	octets* oip; //Ip as octets, for writing to Ip record
	va_list fmts; //For resolving the format string
	char sanitized_msg[512];
	char temp_msg[512];

	char log_prefix[256];
	char sanitized_name[64] = "";
	if(ply)
	{
		int k=0;
		for(int j=0; j<63 && ply->nam[j]!='\0'; j++)
		{
			if(ply->nam[j]>=32 && ply->nam[j]<=126 && ply->nam[j]!='\n' && ply->nam[j]!='\r')
				sanitized_name[k++] = ply->nam[j];
		}
		sanitized_name[k] = '\0';
	}
	if(sock && SDLNet_TCP_GetPeerAddress(sock))
	{
		ip=SDLNet_TCP_GetPeerAddress(sock)->host;
		oip=(octets*)&ip;
	}
	else
	{
		ip=0;
		oip=(octets*)&ip;
	}
	snprintf(log_prefix, sizeof(log_prefix), "%s: [%d] %s (%d.%d.%d.%d) ", 
		os::gettime(), self, ply ? sanitized_name : "", 
		(int)oip->o1, (int)oip->o2, (int)oip->o3, (int)oip->o4);
	va_start(fmts,fmt);
	int len = vsnprintf(temp_msg,sizeof(temp_msg),fmt,fmts);
	va_end(fmts);
	if(len < 0 || len >= (int)sizeof(temp_msg)) {
		fprintf(logf,"[LOG MESSAGE TOO LONG]\n");
		fflush(logf);
		return;
	}
	int k=0;
	for(int j=0; j<len && j<511; j++)
	{
		if((temp_msg[j]>=32 && temp_msg[j]<=126) || temp_msg[j]==' ')
			sanitized_msg[k++] = temp_msg[j];
	}
	sanitized_msg[k] = '\0';
	fprintf(logf,"%s%s\n",log_prefix,sanitized_msg);
	fflush(logf);
}

void server::poll()
{
	int action_type, operation; //Action description
	unsigned char* buf; //Incoming data buffer
	long sbnd; //Server bandwidth used by this player
	unsigned char fldb[SERV_FLOOD_SZ]; //Flooding buffer

	hlpr->pump();
	tout++;
	if(tout>200)
		throw error("Activity timeout");
	if(auth)
	{
		lsnd=-1;
		for(int i=0;i<32;i++)
			acth[i]=false;
		for(int i=0;i<4;i++)
		{
			buf=hlpr->request(3);
			if(buf)
			{
				action_type=((int)buf[0]);
				operation=calc::dattoint(buf+1);

				if(action_type>=0 && action_type<32)
				{
					if(action_type==CLIENT_CHAR || !acth[action_type])
					{
						action(action_type,operation);
						hlpr->suck();
					}
					acth[action_type]=true;
				}
				else
					hlpr->suck();
			}
			else
				break;
			tout=0;
		}
	}
	else
	{
		buf=hlpr->request(6);
		if(buf)
		{
			if(calc::data_arrays_equal((unsigned char*)SIGN,buf,6))
				auth=true;
			else
				throw error("Using incorrect protocol");
			hlpr->suck();
			fldb[0]=SERV_FLOOD;
			memset(fldb+1, 0, SERV_FLOOD_SZ-1);
			for(int i=0;i<110;i++)
				hlpr->send(fldb,SERV_FLOOD_SZ);
			changecmod(CMOD_NAME);
		}
	}
	if(tcks%24==0)
	{
		sbnd=hlpr->getcount();
		if(sbnd>(cbnd/2))
			urat+=6;
		if(sbnd<(cbnd/2))
			urat-=2;
		if(urat<10)
			urat=10;
		if(urat>70)
			urat=70;
	}
}

void server::action(int typ,short opr)
{
	if((typ==CLIENT_CONS || typ==CLIENT_CHAR || typ==CLIENT_BANDWIDTH) || (ply && ply->in))
	{
		try
		{
			switch(typ)
			{
				case CLIENT_ACCEL:
				ply->in->aity=ship::AI_NULL;
				if(opr==-2)
					ply->in->accel(-1,true);	
				if(opr==-1)
					ply->in->accel(-1,false);	
				if(opr==1)	
					ply->in->accel(+1,false);	
				if(opr==2)	
					ply->in->accel(+1,true);	
				break;

				case CLIENT_TURN:
				ply->in->aity=ship::AI_NULL;
				if(opr==-1)
					ply->in->turn(-1);
				if(opr==+1)
					ply->in->turn(+1);
				break;

				case CLIENT_SHOOT:
				if(opr==0)
					ply->in->shoot(false);
				if(opr==1)
					ply->in->shoot(true);
				break;

				case CLIENT_TRG:
				ply->in->aity=ship::AI_NULL;
				ply->in->enemy_target=NULL;
				ply->in->friendly_target=NULL;
				ply->in->planet_target=NULL;
				if(opr>=0 && opr<(planet::ISIZE+ship::ISIZE))
				{
					if(opr<planet::ISIZE)
						ply->in->planet_target=planet::find_by_index(opr);
					else
					{
						ply->in->enemy_target=ship::find_by_index(opr-planet::ISIZE);
						if(ply->in->enemy_target==ply->in)
							ply->in->enemy_target=NULL;
						if(ply->in->enemy_target) {
							if(ply->in->all->opposes(ply->in->enemy_target->all))
								registersound(ply->in,SND_PROXIMITY);
							else
								registersound(ply->in,SND_BEEP2);
						}
					}
				}
				if(cmod==CMOD_SCAN || cmod==CMOD_HAIL || cmod==CMOD_WHOIS)
					changecmod(cmod);
				if(cmod==CMOD_REFIT)
					changecmod(CMOD_HAIL);
				break;

				case CLIENT_CMOD:
				switch(opr)
				{
					case REQ_STAT:
					changecmod(CMOD_STAT);
					break;

					case REQ_EQUIP:
					changecmod(CMOD_EQUIP);
					break;

					case REQ_SCAN:
					changecmod(CMOD_SCAN);
					break;

					case REQ_HAIL:
					changecmod(CMOD_HAIL);
					break;

					case REQ_CHAT:
					changecmod(CMOD_CHAT);
					break;

					case REQ_WHOIS:
					changecmod(CMOD_WHOIS);
					break;

					case REQ_HACK:
					changecmod(CMOD_HACK);
					break;

					default:
					break;
				}
				break;

				case CLIENT_CONS:
				cons(opr);
				break;

				case CLIENT_CHAR:
				if(opr==0)
				{
					input();
					inpb[0]='\0';
				}
				if(opr>=32 && opr<127)
				{
					if(opr>=65 && opr<=90)
						opr+=32;
					for(int i=0;i<64;i++)
					{
						if(inpb[i]=='\0')
						{
							inpb[i]=opr;
							inpb[i+1]='\0';
							break;
						}
					}
				}
				break;

				case CLIENT_BANDWIDTH:
				if(opr>cbnd && opr>0 && opr<999999999)
				{
					cbnd=opr;
					log("Bandwidth exploration reports %ld maximum",(long)cbnd);
				}
				break;

				default:
				break;
			}
		}
		catch(error it)
		{
			printtomesg(it.str);
			changecmod(cmod);
		}
	}
}

void server::changecmod(int opr)
{
	char* txtp;
	char txt[1024]; //Text buffer for constructing output
	alliance* target_alliance; //Alliance for possible choosing
	int spr; //Sprite to spritetocons with

	cmod=opr;
	txtp=txt;
	spr=-1;
	txt[0]='\0';
	if(ply && ply->in)
	{
		try {
			registersound(ply->in,SND_BEEP1);
		} catch(...) {
			// Ignore sound registration errors
		}
	}
	if(cmod!=CMOD_NAME && cmod!=CMOD_PASS && cmod!=CMOD_CHOOSE && !(ply && ply->in))
		return;
 // amazonq-ignore-next-line
	switch(opr)
	{
		case CMOD_NAME:
		printtocons("Choose player name\n");
		requestline(false);
		break;

		case CMOD_PASS:
		printtocons("Player name currently exists\nPlease input player password");
		requestline(true);
		break;

		case CMOD_CHOOSE:
		{
			int remaining = sizeof(txt) - (txtp - txt);
			int written = snprintf(txtp, remaining, "Choose alliance\n");
			if(written > 0 && written < remaining) txtp += written;
			for(int i=0;i<alliance::LIBSIZE;i++)
			{
				target_alliance=alliance::get(i);
				if(target_alliance)
				{
					remaining = sizeof(txt) - (txtp - txt);
					written = snprintf(txtp, remaining, "[%d] %s\n",i,target_alliance->nam);
					if(written > 0 && written < remaining) txtp += written;
				}
			}
			printtocons(txt);
		}
		break;

		case CMOD_STAT:
		spr=ply->in->interact(txt,CMOD_STAT,-1,ply->in);
		printtocons(txt);
		spritetocons(spr);
		break;
		case CMOD_EQUIP:
		ply->in->interact(txt,CMOD_EQUIP,-1,ply->in);
		printtocons(txt);
		break;

		case CMOD_SCAN:
		if(ply->in->enemy_target)
		{
			spr=ply->in->enemy_target->interact(txt,CMOD_SCAN,-1,ply->in);
			printtocons(txt);
			spritetocons(spr);
		}
		else
		{
			if(ply->in->planet_target)
			{
				spr=ply->in->planet_target->interact(txt,CMOD_SCAN,-1,ply->in);
				printtocons(txt);
				spritetocons(spr);
			}
			else
			{
				printtocons("No target");
			}
		}
		break;

		case CMOD_HAIL:
		if(ply->in->enemy_target)
		{
			ply->in->enemy_target->interact(txt,CMOD_HAIL,-1,ply->in);
			printtocons(txt);
		}
		else
		{
			if(ply->in->planet_target)
			{
				ply->in->planet_target->interact(txt,CMOD_HAIL,-1,ply->in);
				printtocons(txt);
			}
			else
			{
				printtocons("No target");
			}
		}
		break;
		
		case CMOD_REFIT:
		if(ply->in->planet_target)
		{
			ply->in->planet_target->interact(txt,CMOD_REFIT,-1,ply->in);
			printtocons(txt);
		}
		break;

		case CMOD_CHAT:
		{
			int remaining = sizeof(txt) - (txtp - txt);
			int written = snprintf(txtp, remaining, "Messaging\n\n");
			if(written > 0 && written < remaining) txtp += written;
			if(ply->in->enemy_target && ply->in->enemy_target->assigned_player) {
				remaining = sizeof(txt) - (txtp - txt);
				written = snprintf(txtp, remaining, "[1] Chat with target player\n");
				if(written > 0 && written < remaining) txtp += written;
			}
			remaining = sizeof(txt) - (txtp - txt);
			written = snprintf(txtp, remaining, "[2] Chat with team\n");
			if(written > 0 && written < remaining) txtp += written;
			remaining = sizeof(txt) - (txtp - txt);
			written = snprintf(txtp, remaining, "[3] Chat with all\n");
			if(written > 0 && written < remaining) txtp += written;
		}
		printtocons(txt);
		break;

		case CMOD_CHATPRIVATE:
		printtocons("Chat with target player\n");
		requestline(false);
		break;

		case CMOD_CHATTEAM:
		printtocons("Chat with team");
		requestline(false);
		break;

		case CMOD_CHATALL:
		printtocons("Chat with all");
		requestline(false);
		break;

		case CMOD_WHOIS:
		{
			int remaining = sizeof(txt) - (txtp - txt);
			int written = snprintf(txtp, remaining, "WHOIS\n\n");
			if(written > 0 && written < remaining) txtp += written;
			if(ply->in->enemy_target)
			{
				remaining = sizeof(txt) - (txtp - txt);
				spr=ply->in->enemy_target->interact(txtp,CMOD_WHOIS,-1,ply->in);
				txtp=txt+strlen(txt);
			}
			else {
				remaining = sizeof(txt) - (txtp - txt);
				written = snprintf(txtp, remaining, "No target\n");
				if(written > 0 && written < remaining) txtp += written;
			}
			remaining = sizeof(txt) - (txtp - txt);
			written = snprintf(txtp, remaining, "\n[1] Cycle to next player");
			if(written > 0 && written < remaining) txtp += written;
		}
		printtocons(txt);
		spritetocons(spr);
		break;

		case CMOD_HACK:
		if(ply->op && !locg)
		{
			printtocons("Admin\n\n[1] Set account password\n[2] Kick user\n[3] Delete user\n[4] Shutdown server");
		}
		else
		{
			printtocons("Admin\n\n[1] Set account password");
		}
		break;
		
		case CMOD_PASS1:
		printtocons("Choose your password");
		requestline(true);
		break;

		case CMOD_PASS2:
		printtocons("Confirm your password");
		requestline(true);
		break;

		case CMOD_KICK:
		printtocons("Input username to kick");
		requestline(false);
		break;

		case CMOD_DELETE:
		printtocons("Input username to delete");
		requestline(false);
		break;

		default:
		break;
	}
}

void server::cons(int opr)
{
	char txt[1024]; //Text buffer for constructing output
	alliance* target_alliance; //Alliance to choose
	ship* spawned_ship; //Temporary ship scratchpad

	if(opr<0)
		return;
	if(cmod!=CMOD_CHOOSE && !(ply && ply->in))
		return;
	txt[0]='\0';
	switch(cmod)
	{
		case CMOD_CHOOSE:
		target_alliance=alliance::get(opr);
		if(target_alliance)
		{
			try {
				ply->spawn(target_alliance);
				log("Spawned as %s(%s)",ply->in->cls,ply->in->all->nam);
				changecmod(CMOD_HACK);
			} catch(error it) {
				printtomesg(it.str);
				changecmod(CMOD_CHOOSE);
			}
		}
		else
		{
			printtomesg("No such alliance");
			changecmod(CMOD_CHOOSE);
		}
		break;

		case CMOD_EQUIP:
		ply->in->interact(txt,CMOD_EQUIP,opr,ply->in);
		printtomesg(txt);
		changecmod(CMOD_EQUIP);
		break;

		case CMOD_SCAN:
		if(ply->in->enemy_target)
			ply->in->enemy_target->interact(txt,CMOD_SCAN,opr,ply->in);
		else
			if(ply->in->planet_target)
				ply->in->planet_target->interact(txt,CMOD_SCAN,opr,ply->in);
		break;

		case CMOD_HAIL:
		if(ply->in->enemy_target)
		{
			ply->in->enemy_target->interact(txt,CMOD_HAIL,opr,ply->in);
			printtomesg(txt);
			changecmod(CMOD_HAIL);
		}
		else
		{
			if(ply->in->planet_target)
			{
				if(opr==5)
				{
					try {
						if(!ply || !ply->in || !ply->in->planet_target) {
							throw error("Invalid player or planet state");
						}
						if(ply->in->can_detect(ply->in->planet_target))
						{
							if(ply->in->planet_target->all==ply->in->all)
							{
								ply->in->transport(ply->in->planet_target);
								ply->commit();
								printtomesg("Restore position saved");
							}
							else
								throw error("Cannot save with a different allegiance");
						}
						else
						{
							throw error("Out of range");
						}
					} catch(error it) {
						printtomesg(it.str);
						log("Transport error: %s", it.str);
					} catch(...) {
						printtomesg("Unknown error during transport operation");
						log("Unknown transport error");
					}
				}
				else if(opr==4)
				{
					changecmod(CMOD_REFIT);
				}
				else
				{
					ply->in->planet_target->interact(txt,CMOD_HAIL,opr,ply->in);
					printtomesg(txt);
					changecmod(CMOD_HAIL);
				}
			}
		}
		break;

		case CMOD_REFIT:
		if(ply->in->planet_target)
		{
			ply->in->planet_target->interact(txt,CMOD_REFIT,opr,ply->in);
			printtomesg(txt);
			changecmod(CMOD_REFIT);
		}
		break;

		case CMOD_CHAT:
		if(opr==1 && ply->in->enemy_target && ply->in->enemy_target->assigned_player)
			changecmod(CMOD_CHATPRIVATE);
		if(opr==2)
			changecmod(CMOD_CHATTEAM);
		if(opr==3)
			changecmod(CMOD_CHATALL);
		break;

		case CMOD_WHOIS:
		if(opr==1)
		{
			for(int i=0,j=(ply->in->enemy_target ? ply->in->enemy_target->self+1 : 0);i<ship::ISIZE;i++,j++)
			{
				if(j>=ship::ISIZE)
					j=0;
				spawned_ship=ship::find_by_index(j);
				if(spawned_ship && spawned_ship->assigned_player && spawned_ship!=ply->in)
				{
					ply->in->enemy_target=spawned_ship;
					ply->in->friendly_target=NULL;
					ply->in->planet_target=NULL;
					break;
				}
			}
			changecmod(CMOD_WHOIS);
		}
		break;

		case CMOD_HACK:
		if(opr==1)
		{
			changecmod(CMOD_PASS1);
		}
		if(ply->op && !locg)
		{
			if(opr==2)
				changecmod(CMOD_KICK);
			if(opr==3)
				changecmod(CMOD_DELETE);
			if(opr==4)
			{
				log("Server shutdown requested");
				qsig=true;
			}
		}
		break;

		default:
		log("Unknown command mode: %d", cmod);
		break;
	}
}

void server::requestline(bool hide)
{
	unsigned char buf[SERV_READLN_SZ]; //For sending the request byte

	buf[0]=SERV_READLN;
	buf[1]=(unsigned char)hide;
	if(hlpr)
		hlpr->send(buf,SERV_READLN_SZ);
}

void server::input()
{
	player* delp; //Player to delete

	switch(cmod)
	{
		case CMOD_NAME:
		if(inpb[0]=='\0')
			throw error("Aborted name entry");
		try
		{
			if(inpb[0]=='\0' || inpb[1]=='\0')
			{
				inpb[0]='\0';
				throw error("Name too short");
			}
			ply=player::get(inpb);
			if(ply)
			{
				log("Attempting login");
				if(locg)
				{
					ply->login(NULL);
					log("Local game login");
					changecmod(CMOD_STAT);
				}
				else
				{
					changecmod(CMOD_PASS);
				}
			}
			else
			{
				ply=new player(inpb);
				log("New player created");
				changecmod(CMOD_CHOOSE);
			}
		}
		catch(error it)
		{
			ply=NULL;	
			{
				char sanitized_error[256];
				int k=0;
				for(int j=0; j<255 && it.str[j]!='\0'; j++)
				{
					if(it.str[j]>=32 && it.str[j]<=126 && it.str[j]!='\n' && it.str[j]!='\r')
						sanitized_error[k++] = it.str[j];
				}
				sanitized_error[k] = '\0';
				log("%s", sanitized_error);
			}
			printtomesg(it.str);
			changecmod(CMOD_NAME);
		}
		break;

		case CMOD_PASS:
		try
		{
			ply->login(inpb);
			log("Login succeeded");
			changecmod(CMOD_STAT);
		}
		catch(error it)
		{
			ply=NULL;
			// log("%s", it.str);
			log(it.str);
			printtomesg(it.str);
			changecmod(CMOD_NAME);
		}
		break;

		case CMOD_PASS1:
		inpb[32]='\0';
		strncpy(tpas,inpb,32);
		tpas[32]='\0';
		changecmod(CMOD_PASS2);
		break;

		case CMOD_PASS2:
		inpb[32]='\0';
		if(strcmp(inpb,tpas)==0)
		{
			ply->setpass(inpb);
			printtomesg("Password set successfully");
			log("Set password");
		}
		else
		{
			printtomesg("Passwords don't match!");
		}
		changecmod(CMOD_HACK);
		break;

		case CMOD_CHATPRIVATE:
		if(ply && ply->in && ply->in->enemy_target && ply->in->enemy_target->assigned_player)
		{
			hail(ply,ply->in->enemy_target->assigned_player,inpb);
			changecmod(CMOD_CHATPRIVATE);
		}
		break;
		
		case CMOD_CHATTEAM:
		if(ply && ply->in)
		{
			for(int i=0;i<ISIZE;i++)
				if(connections[i] && connections[i]->ply && connections[i]->ply->in && connections[i]->ply->in->all==ply->in->all)
					hail(ply,connections[i]->ply,inpb);
			changecmod(CMOD_CHATTEAM);
		}
		break;
		
		case CMOD_CHATALL:
		if(ply->in)
		{
			for(int i=0;i<ISIZE;i++)
				if(connections[i] && connections[i]->ply)
					hail(ply,connections[i]->ply,inpb);
			changecmod(CMOD_CHATALL);
		}
		break;

		case CMOD_KICK:
		{
			char sanitized_name[65];
			int k=0;
			for(int j=0; j<64 && inpb[j]!='\0'; j++)
			{
				if(inpb[j]>=32 && inpb[j]<=126 && inpb[j]!='\n' && inpb[j]!='\r')
					sanitized_name[k++] = inpb[j];
			}
			sanitized_name[k] = '\0';
			log("Attemped kick of %s",sanitized_name);
		}
		for(int i=0;i<ISIZE;i++)
		{
			if(connections[i] && connections[i]!=this && connections[i]->ply && strcmp(inpb,connections[i]->ply->nam)==0)
			{
				bulletin("%s was kicked from the server",inpb);
				delete connections[i];
				break;
			}
		}
		changecmod(CMOD_HACK);
		break;
		
		case CMOD_DELETE:
		{
			char sanitized_name[65];
			int k=0;
			for(int j=0; j<64 && inpb[j]!='\0'; j++)
			{
				if(inpb[j]>=32 && inpb[j]<=126 && inpb[j]!='\n' && inpb[j]!='\r')
					sanitized_name[k++] = inpb[j];
			}
			sanitized_name[k] = '\0';
			log("Attemped deletion of %s",sanitized_name);
		}
 		for(int i=0;i<ISIZE;i++)
		{
			if(connections[i] && connections[i]!=this && connections[i]->ply && strcmp(inpb,connections[i]->ply->nam)==0)
			{
				bulletin("%s was kicked from the server",inpb);
				delete connections[i];
				break;
			}
		}
		delp=player::get(inpb);
		if(delp && delp!=ply)
			delete delp;
		changecmod(CMOD_HACK);
		break;

		default:
		log("Unknown input mode: %d", cmod);
		changecmod(CMOD_NAME);
		break;
	}
}

void server::printtocons(const char* fmt,...)
{
	unsigned char buf[1028]; //Outgoing buffer
	va_list fmts;

	va_start(fmts,fmt);
	int len = vsnprintf((char*)buf+3,1024,fmt,fmts);
	va_end(fmts);
	if(len < 0 || len >= 1024) {
		throw error("Console message too long");
	}

	buf[0]=SERV_CONS;
	calc::inttodat(len,buf+1);

	if(hlpr)
		hlpr->send(buf,len+3);
}

void server::spritetocons(int indx)
{
	unsigned char buf[SERV_CSPR_SZ]; //Outgoing buffer

	if(indx>=0 && hlpr)
	{
		buf[0]=SERV_CSPR;
		calc::inttodat(indx,buf+1);
		hlpr->send(buf,SERV_CSPR_SZ);
	}
}

void server::printtomesg(const char* fmt,...)
{
	unsigned char buf[132]; //Outgoing buffer
	va_list fmts;

	if(fmt[0]!='\0')
	{
		va_start(fmts,fmt);
		int len = vsnprintf((char*)buf+3,128,fmt,fmts);
		va_end(fmts);
		if(len < 0 || len >= 128) {
			throw error("Message too long");
		}

		buf[0]=SERV_MESG;
		calc::inttodat(len,buf+1);

		if(hlpr)
			hlpr->send(buf,len+3);
	}
}

void server::uploads()
{
	unsigned char buf[256]; //Buffer for outgoing data

	if(ply && ply->in)
	{
		if(!hlpr) return;
		ply->in->serialize_to_network(SERV_SELF,buf);
		hlpr->send(buf,SERV_SELF_SZ);
		calc::inttodat(foc,buf+21);
		/*if(!shpu[ply->in->self])
		{
			ply->in->serialize_to_network(SERV_NEW,buf);
			hlpr->send(buf,SERV_NEW_SZ);
			ply->in->serialize_to_network(SERV_NAME,buf);
			hlpr->send(buf,SERV_NAME_SZ);
			shpu[ply->in->self]=true;
		}
		ply->in->serialize_to_network(SERV_UPD,buf);
		buf[21]=0;
		hlpr->send(buf,SERV_UPD_SZ);*/
		
		if(tcks%(urat/10)==0)
		{
			if(tcks%((urat*4)/10)==0)
				uploadplanets();
			uploadships();
			uploadfrags();
		}
		hlpr->pump();
	}
}

void server::uploadplanets()
{
	unsigned char buf[256]; //Outgoing scratchpad buffer to use
	planet* tpln; //Concerned planet

	if(!hlpr) return;
	for(int i=0;i<planet::ISIZE;i++)
	{
		tpln=planet::find_by_index(i);
		if(tpln)
		{
			if(ply->in->can_detect(tpln))
			{
				if(!plnu[i])
				{
					tpln->serialize_to_network(SERV_NEW,buf);
					hlpr->send(buf,SERV_NEW_SZ);
					tpln->serialize_to_network(SERV_NAME,buf);
					hlpr->send(buf,SERV_NAME_SZ);
					plnu[i]=true;
				}
				tpln->serialize_to_network(SERV_UPD,buf);
				hlpr->send(buf,SERV_UPD_SZ);
			}
			else
			{
				if(plnu[i])
				{
					buf[0]=SERV_DEL;
					calc::inttodat(planet2pres(i),buf+1);
					hlpr->send(buf,SERV_DEL_SZ);
					plnu[i]=false;
				}
			}
		}
		else
		{
			if(plnu[i])
			{
				buf[0]=SERV_DEL;
				calc::inttodat(planet2pres(i),buf+1);
				hlpr->send(buf,SERV_DEL_SZ);
				plnu[i]=false;
			}
		}
	}
}

void server::uploadships()
{
	unsigned char buf[256]; //Outgoing scratchpad buffer to use
	ship* spawned_ship; //Concerned ship

	if(!hlpr) return;
	for(int i=0;i<ship::ISIZE;i++)
	{
		spawned_ship=ship::find_by_index(i);
		//if(spawned_ship!=ply->in)
		{
			if(spawned_ship)
			{
				if(ply->in->can_detect(spawned_ship))
				{
					if(!shpu[i])
					{
						spawned_ship->serialize_to_network(SERV_NEW,buf);
						hlpr->send(buf,SERV_NEW_SZ);
						spawned_ship->serialize_to_network(SERV_NAME,buf);
						hlpr->send(buf,SERV_NAME_SZ);
						shpu[i]=true;
					}
					spawned_ship->serialize_to_network(SERV_UPD,buf);
					if(ply->in->all->opposes(spawned_ship->all))
						buf[21]=1;
					hlpr->send(buf,SERV_UPD_SZ);
				}
				else
				{
					if(shpu[i])
					{
						buf[0]=SERV_DEL;
						calc::inttodat(ship2pres(i),buf+1);
						hlpr->send(buf,SERV_DEL_SZ);
						shpu[i]=false;
					}
				}
			}
			else
			{
				if(shpu[i])
				{
					buf[0]=SERV_DEL;
					calc::inttodat(ship2pres(i),buf+1);
					hlpr->send(buf,SERV_DEL_SZ);
					shpu[i]=false;
				}
			}
		}
	}
}

void server::uploadfrags()
{
	unsigned char buf[256]; //Outgoing scratchpad buffer to use
	frag* tfrg; //Concerned frag

	if(!hlpr) return;
	for(int i=0;i<frag::ISIZE;i++)
	{
		tfrg=frag::get(i);
		if(tfrg)
		{
			if(ply->in->can_detect(tfrg))
			{
				if(frgu[i])
				{
					tfrg->serialize_to_network(SERV_UPD,buf);
					hlpr->send(buf,SERV_UPD_SZ);
				}
				else
				{
					tfrg->serialize_to_network(SERV_NEW,buf);
					hlpr->send(buf,SERV_NEW_SZ);
					tfrg->serialize_to_network(SERV_UPD,buf);
					hlpr->send(buf,SERV_UPD_SZ);
					frgu[i]=true;
				}
			}
			else
			{
				if(frgu[i])
				{
					buf[0]=SERV_DEL;
					calc::inttodat(frag2pres(i),buf+1);
					hlpr->send(buf,SERV_DEL_SZ);
					frgu[i]=false;
				}
			}
		}
		else
		{
			if(frgu[i])
			{
				buf[0]=SERV_DEL;
				calc::inttodat(frag2pres(i),buf+1);
				hlpr->send(buf,SERV_DEL_SZ);	
				frgu[i]=false;
			}
		}
	}
}

void server::kill()
{
	unsigned char buf[SERV_DEL_SZ]; //Buffer for deleting player's ship clientside

	try
	{
		uploadplanets();
		uploadships();
		uploadfrags();
		buf[0]=SERV_DEL;
		calc::inttodat(ship2pres(ply->in->self),buf+1);
		if(hlpr)
			hlpr->send(buf,SERV_DEL_SZ);
		printtomesg("You have been destroyed: Game Over");
		printtocons("Game over");
	}
	catch(error it)
	{
		log("Error in kill(): %s", it.str);
	}
}

void server::hilight(ship* spawned_ship)
{
	unsigned char buf[SERV_HILIGHT_SZ]; //Buffer for sending hilight information

	if(spawned_ship && hlpr)
	{
		buf[0]=SERV_HILIGHT;
		calc::inttodat(ship2pres(spawned_ship->self),buf+1);
		hlpr->send(buf,SERV_HILIGHT_SZ);
	}
}

server* server::connections[ISIZE];
long server::tcks;
TCPsocket server::lstn;
bool server::qsig;
FILE* server::logf;
bool server::locg;
bool server::locl;
