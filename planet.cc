/*
	planet.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "calc.h"
#include "error.h"
#include "alliance.h"
#include "database.h"
#include "equip.h"
#include "ship.h"
#include "protocol.h"
#include "planet.h"

// Planet sprite constants
const int STAR_SPRITE_LOW = 100;
const int STAR_SPRITE_HIGH = 103;
const int UNINHABITED_SPRITE_LOW = 110;
const int UNINHABITED_SPRITE_HIGH = 116;
const int INHABITED_SPRITE_LOW = 120;
const int INHABITED_SPRITE_HIGH = 124;
const int MASS_LOCK_DISTANCE = 20000;
const int SPAWN_RADIUS = 150;
const int MAX_EQUIPMENT_SLOTS = 8;
const int MAX_SYLLABLES = 16;

planet::planet(char* nam,cord put,int typ,alliance* all)
{
	const int slo=STAR_SPRITE_LOW,shi=STAR_SPRITE_HIGH,ulo=UNINHABITED_SPRITE_LOW,uhi=UNINHABITED_SPRITE_HIGH,ilo=INHABITED_SPRITE_LOW,ihi=INHABITED_SPRITE_HIGH; //Boundings for planet sprites

	self=-1;
	for(int i=0;i<ISIZE && self==-1;i++)
	{
		if(!planets[i])
		{
			self=i;
			planets[i]=this;
		}
	}
	if(self==-1)
		throw error("No free slots for planet");
	snprintf(this->nam, sizeof(this->nam), "%s", nam);
	this->loc=put;
	this->all=all;
	rot=calc::rnd(36);
	if(typ==STAR)
		spr=slo+calc::rnd(shi-slo+1);
	if(typ==UNINHABITED)
		spr=ulo+calc::rnd(uhi-ulo+1);
	if(typ==INHABITED)
		spr=ilo+calc::rnd(ihi-ilo+1);
	this->typ=typ;
	for(int j=0;j<MAX_EQUIPMENT_SLOTS;j++)
		sold[j]=all->getequip();
}

planet::~planet()
{
	if(self!=-1)
		planets[self]=NULL;
}

void planet::init()
{
	for(int i=0;i<ISIZE;i++)
		planets[i]=NULL;
}

void planet::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(planets[i])
			delete planets[i];
}

planet* planet::get(int indx)
{
	if(indx>=0 && indx<ISIZE)
	{
		if(planets[indx])
			return planets[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

planet* planet::pick(alliance* tali)
{
	for(int i=0,j=0;i<ISIZE;i++)
	{
		j=calc::rnd(ISIZE);
		if(planets[j] && planets[j]->typ==INHABITED && planets[j]->all==tali)
			return planets[j];
	}
	//Can't find one by this point, but maybe it's just bad luck
	//This one is more deterministic, especially as it's often vital to get a result (e.g. when spawning a player's new ship)
	for(int i=0;i<ISIZE;i++)
	{
		if(planets[i] && planets[i]->typ==INHABITED && planets[i]->all==tali)
			return planets[i];
	}
	return NULL;
}

planet* planet::find_allied_planet(alliance* tali)
{
	for(int i=0,j=0;i<ISIZE;i++)
	{
		j=calc::rnd(ISIZE);
		if(planets[j] && planets[j]->typ==INHABITED && !(tali->opposes(planets[j]->all)))
			return planets[j];
	}
	return NULL;
}

planet* planet::find_hostile_planet(alliance* tali)
{
	for(int i=0,j=0;i<ISIZE;i++)
	{
		j=calc::rnd(ISIZE);
		if(planets[j] && planets[j]->typ!=STAR && tali->opposes(planets[j]->all))
			return planets[j];
	}
	return NULL;
}

bool planet::masslock(cord loc)
{
	double dx,dy; //Co-ordinate differences
	
	for(int i=0;i<ISIZE;i++)
	{
		if(planets[i] && planets[i]->typ==STAR)
		{
			dx=loc.x-planets[i]->loc.x;
			dy=loc.y-planets[i]->loc.y;
			if(dx<MASS_LOCK_DISTANCE && dx>-MASS_LOCK_DISTANCE && dy>-MASS_LOCK_DISTANCE && dy<MASS_LOCK_DISTANCE)
				return true;
		}
	}
	return false;
}

void planet::saveall()
{
	char obsc[33]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		if(planets[i])
		{
			sprintf(obsc,"Planet%d",i);
			database::putobject(obsc);
			planets[i]->save();
		}
	}
}

void planet::loadall()
{
	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			new planet(i);
		}
		catch(error it)
		{
		}
	}
}

void planet::generatename(char* put)
{
	const char n1[][16]={"Vul","Et","Fer","Man","Clo","Jen","Reb","Joy","Dan","Sat","Mat","Cor","Wat","An","Oct","Ack"};
	const char n2[][16]={"tan","can","an","en","in","lat","pil","op","ep","ast","tyn","syn","blat","av","",""};
	const char n3[][16]={"is","ia","ol","ion","on","ath","ur","e","ise","at","","","","","",""};
	int s1,s2,s3; //Syllable selectors

	s1=calc::rnd(MAX_SYLLABLES);
	s2=calc::rnd(MAX_SYLLABLES);
	s3=calc::rnd(MAX_SYLLABLES);
	sprintf(put,"%s%s%s",n1[s1],n2[s2],n3[s3]);
}

void planet::shipyards()
{
	planet* tpln; //Planet to spawn at

	if(ship::has_available_ship_slot())
	{
		tpln=planets[calc::rnd(ISIZE)];
		if(tpln && tpln->typ==INHABITED)
		{
			tpln->shipyard();
		}
	}
}

int planet::interact(char* txt,short cmod,short opr,ship* mshp)
{
	long cost; //Cost of purchase

	if(!mshp)
		return -1;
	switch(cmod)
	{
		case CMOD_SCAN:
		if(opr==-1)
		{
			txt+=sprintf(txt,"%s\n",nam);
			if(mshp->all->opposes(all))
				txt+=sprintf(txt,"Alignment:%s [hostile]\n",all->nam);
			else
				txt+=sprintf(txt,"Alignment:%s\n",all->nam);
			switch(typ)
			{
				case STAR:
				txt+=sprintf(txt,"Star\n");
				break;

				case UNINHABITED: 
				txt+=sprintf(txt,"Uninhabited planet\n");
				break;

				case INHABITED:
				txt+=sprintf(txt,"Inhabited planet\n");
				break;
			}
			txt+=sprintf(txt,"\n[1] Lay in a course\n");
			return spr;
		}
		if(opr==1 && this==mshp->plnt)
		{
			mshp->aity=ship::AI_AUTOPILOT;
		}
		break;

		case CMOD_HAIL:
		if(mshp->see(this))
		{
			if(
			typ!=INHABITED ||		
			all->trad==alliance::TRADE_NONE ||
			(all->trad==alliance::TRADE_CLOSED && mshp->all!=this->all) ||
			(all->trad==alliance::TRADE_FRIENDS && this->all->opposes(mshp->all))
			)
			{
				txt+=sprintf(txt,"No response");
				return -1;
			}
		}
		else
		{
			txt+=sprintf(txt,"Out of range");
			return -1;
		}
		if(opr==-1)
		{
			txt+=sprintf(txt,"Hailing %s\n\nServices\n\n",nam);
			cost=mshp->purchase(ship::PRCH_FUEL,all->ripo,false);
			if(cost)
				txt+=sprintf(txt,"[1] Refuel\nCost: %ld C\n",cost);
			cost=mshp->purchase(ship::PRCH_HULL,all->ripo,false);
			if(cost)
				txt+=sprintf(txt,"[2] Repair hull\nCost: %ld C\n",cost);
			cost=mshp->purchase(ship::PRCH_ARMS,all->ripo,false);
			if(cost)
				txt+=sprintf(txt,"[3] Rearm one magazine\nCost: %ld C\n",cost);
			txt+=sprintf(txt,"[4] Purchase equipment\n");
			txt+=sprintf(txt,"[5] Save location");
		}
		if(opr==1 || opr==2 || opr==3)
		{
		//	if(opr==1)
		//		cost=mshp->purchase(ship::PRCH_FUEL,all->ripo,false);
		//	if(opr==2)
		//		cost=mshp->purchase(ship::PRCH_HULL,all->ripo,false);
		//	if(opr==3)
		//		cost=mshp->purchase(ship::PRCH_ARMS,all->ripo,false);
		//	if(cost>0)
		//	{
				mshp->transport(this);
				if(opr==1)
					mshp->purchase(ship::PRCH_FUEL,all->ripo,true);
				if(opr==2)
					mshp->purchase(ship::PRCH_HULL,all->ripo,true);
				if(opr==3)
					mshp->purchase(ship::PRCH_ARMS,all->ripo,true);
		//	}
		}
		break;

		case CMOD_REFIT:
		if(mshp->see(this))
		{
			if(
			typ!=INHABITED ||		
			all->trad==alliance::TRADE_NONE ||
			(all->trad==alliance::TRADE_CLOSED && mshp->all!=this->all) ||
			(all->trad==alliance::TRADE_FRIENDS && this->all->opposes(mshp->all))
			)
			{
				txt+=sprintf(txt,"No response");
				return -1;
			}

			
		}
		else
		{
			txt+=sprintf(txt,"Out of range");
			return -1;
		}
		if(opr==-1)
		{
			txt+=sprintf(txt,"Hailing %s\n\nEquipment\n\n",nam);
			for(int i=0;i<MAX_EQUIPMENT_SLOTS;i++)
			{
				if(sold[i])
				{
					cost=mshp->purchase(sold[i],all->ripo,false);
					txt+=sprintf(txt,"[%hd] %s \nCost: %ld C  Mass: %hd\n",i+1,sold[i]->nam,cost,sold[i]->mss);
				}
			}
			txt+=sprintf(txt,"\nAvailable mass: %hd\n",mshp->get_available_cargo_space());
		}
		if(opr>=1 && opr<=MAX_EQUIPMENT_SLOTS && sold[opr-1])
		{
			//cost=mshp->purchase(sold[opr-1],all->ripo,false);
			
			mshp->transport(this);
			mshp->purchase(sold[opr-1],all->ripo,true);
			txt+=sprintf(txt,"%s purchased and installed",sold[opr-1]->nam);
		}
		break;

		default:
		break;
	}
	return -1;
}

void planet::serialize_to_network(int typ,unsigned char* buf)
{
	if(!buf)
		return;
	
	buf[0]=typ;
	buf+=1;

	calc::inttodat(planet2pres(self),buf);
	buf+=2;
	switch(typ)
	{
		case SERV_NEW:
		*buf=PT_PLANET;
		buf+=1;
		calc::inttodat(spr,buf);
		buf+=2;
		calc::inttodat(-1,buf);
		buf+=2;
		break;

		case SERV_NAME:
		snprintf((char*)buf,64,"%s",nam);
		buf+=64;
		snprintf((char*)buf,64,"%s",all->nam);
		buf+=64;
		break;

		case SERV_UPD:
		calc::longtodat(loc.x,buf);
		buf+=4;
		calc::longtodat(loc.y,buf);
		buf+=4;
		calc::longtodat(0,buf);
		buf+=4;
		calc::longtodat(0,buf);
		buf+=4;
		calc::inttodat(rot*10,buf);
		buf+=2;
		*buf=0;
		buf+=1;
		*buf=100;
		buf+=1;
		break;
	}
}

planet::planet(int self)
{
	char obsc[16]; //Object name scratchpad

	this->self=-1;
	if(planets[self])
		throw error("This planet slot already taken");
	sprintf(obsc,"Planet%d",self);
	database::switchobj(obsc);
	load();
	this->self=self;
	planets[self]=this;
}

void planet::save()
{
	char atsc[33]; //Attribute scratchpad

	database::putvalue("Name",nam);
	if(spr)
		database::putvalue("Sprite",spr);
	database::putvalue("SpriteRot",rot);
	if(spr)
		database::putvalue("Team",all->self);
	database::putvalue("XLoc",loc.x);
	database::putvalue("YLoc",loc.y);
	database::putvalue("Type",typ);
	for(int i=0;i<MAX_EQUIPMENT_SLOTS;i++)
	{
		sprintf(atsc,"Sold%d",i);
		if(sold[i])
			database::putvalue(atsc,sold[i]->self);
		else
			database::putvalue(atsc,-1);
	}
}

void planet::load()
{
	char atsc[33]; //Attribute scratchpad

	database::getvalue("Name",nam);
	spr=database::getvalue("Sprite");
	rot=database::getvalue("SpriteRot");
	all=alliance::get(database::getvalue("Team"));
	loc.x=database::getvalue("XLoc");
	loc.y=database::getvalue("YLoc");
	typ=database::getvalue("Type");
	for(int i=0;i<MAX_EQUIPMENT_SLOTS;i++)
	{
		sprintf(atsc,"Sold%d",i);
		sold[i]=equip::get(database::getvalue(atsc));
	}
}

void planet::shipyard()
{
	cord put; //Location to spawn
	ship* lshp; //Ship from library
	ship* tshp; //Ship being spawned
	int aity; //AI type to spawn

	put.x=loc.x+calc::rnd(SPAWN_RADIUS)-calc::rnd(SPAWN_RADIUS);
	put.y=loc.y+calc::rnd(SPAWN_RADIUS)-calc::rnd(SPAWN_RADIUS);

	lshp=all->getspawn();
	aity=all->getai();
	if(lshp)
	{
		try
		{
			tshp=new ship(put,lshp,all,aity);
			tshp->insert();
			if(tshp->self==-1)
				delete tshp;
		}
		catch(error it)
		{
		}
	}
}

planet* planet::planets[ISIZE];
