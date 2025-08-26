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
	rot=calc::random_int(36);
	if(typ==STAR)
		spr=slo+calc::random_int(shi-slo+1);
	if(typ==UNINHABITED)
		spr=ulo+calc::random_int(uhi-ulo+1);
	if(typ==INHABITED)
		spr=ilo+calc::random_int(ihi-ilo+1);
	this->typ=typ;
	for(int j=0;j<MAX_EQUIPMENT_SLOTS;j++)
		sold[j]=all->get_random_equipment();
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

planet* planet::find_by_index(int indx)
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

planet* planet::find_random_planet(alliance* target_alliance)
{
	for(int i=0,j=0;i<ISIZE;i++)
	{
		j=calc::random_int(ISIZE);
		if(planets[j] && planets[j]->typ==INHABITED && planets[j]->all==target_alliance)
			return planets[j];
	}
	//Can't find one by this point, but maybe it's just bad luck
	//This one is more deterministic, especially as it's often vital to get a result (e.g. when spawning a player's new ship)
	for(int i=0;i<ISIZE;i++)
	{
		if(planets[i] && planets[i]->typ==INHABITED && planets[i]->all==target_alliance)
			return planets[i];
	}
	return NULL;
}

planet* planet::find_allied_planet(alliance* target_alliance)
{
	for(int i=0,j=0;i<ISIZE;i++)
	{
		j=calc::random_int(ISIZE);
		if(planets[j] && planets[j]->typ==INHABITED && !(target_alliance->opposes(planets[j]->all)))
			return planets[j];
	}
	return NULL;
}

planet* planet::find_hostile_planet(alliance* target_alliance)
{
	for(int i=0,j=0;i<ISIZE;i++)
	{
		j=calc::random_int(ISIZE);
		if(planets[j] && planets[j]->typ!=STAR && target_alliance->opposes(planets[j]->all))
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
			dx=loc.x_component-planets[i]->loc.x_component;
			dy=loc.y_component-planets[i]->loc.y_component;
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

	s1=calc::random_int(MAX_SYLLABLES);
	s2=calc::random_int(MAX_SYLLABLES);
	s3=calc::random_int(MAX_SYLLABLES);
	sprintf(put,"%s%s%s",n1[s1],n2[s2],n3[s3]);
}

void planet::shipyards()
{
	planet* tpln; //Planet to spawn at

	if(ship::has_available_ship_slot())
	{
		tpln=planets[calc::random_int(ISIZE)];
		if(tpln && tpln->typ==INHABITED)
		{
			tpln->shipyard();
		}
	}
}

int planet::interact(char* txt,short cmod,short opr,ship* player_ship)
{
	long cost; //Cost of purchase

	if(!player_ship)
		return -1;
	switch(cmod)
	{
		case CMOD_SCAN:
		if(opr==-1)
		{
			txt+=sprintf(txt,"%s\n",nam);
			if(player_ship->all->opposes(all))
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
		if(opr==1 && this==player_ship->planet_target)
		{
			player_ship->aity=ship::AI_AUTOPILOT;
		}
		break;

		case CMOD_HAIL:
		if(player_ship->can_detect(this))
		{
			if(
			typ!=INHABITED ||		
			all->trad==alliance::TRADE_NONE ||
			(all->trad==alliance::TRADE_CLOSED && player_ship->all!=this->all) ||
			(all->trad==alliance::TRADE_FRIENDS && this->all->opposes(player_ship->all))
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
			cost=player_ship->purchase(ship::PRCH_FUEL,all->ripo,false);
			if(cost)
				txt+=sprintf(txt,"[1] Refuel\nCost: %ld C\n",cost);
			cost=player_ship->purchase(ship::PRCH_HULL,all->ripo,false);
			if(cost)
				txt+=sprintf(txt,"[2] Repair hull\nCost: %ld C\n",cost);
			cost=player_ship->purchase(ship::PRCH_ARMS,all->ripo,false);
			if(cost)
				txt+=sprintf(txt,"[3] Rearm one magazine\nCost: %ld C\n",cost);
			txt+=sprintf(txt,"[4] Purchase equipment\n");
			txt+=sprintf(txt,"[5] Save location");
		}
		if(opr==1 || opr==2 || opr==3)
		{
		//	if(opr==1)
		//		cost=player_ship->purchase(ship::PRCH_FUEL,all->ripo,false);
		//	if(opr==2)
		//		cost=player_ship->purchase(ship::PRCH_HULL,all->ripo,false);
		//	if(opr==3)
		//		cost=player_ship->purchase(ship::PRCH_ARMS,all->ripo,false);
		//	if(cost>0)
		//	{
				player_ship->transport(this);
				if(opr==1)
					player_ship->purchase(ship::PRCH_FUEL,all->ripo,true);
				if(opr==2)
					player_ship->purchase(ship::PRCH_HULL,all->ripo,true);
				if(opr==3)
					player_ship->purchase(ship::PRCH_ARMS,all->ripo,true);
		//	}
		}
		break;

		case CMOD_REFIT:
		if(player_ship->can_detect(this))
		{
			if(
			typ!=INHABITED ||		
			all->trad==alliance::TRADE_NONE ||
			(all->trad==alliance::TRADE_CLOSED && player_ship->all!=this->all) ||
			(all->trad==alliance::TRADE_FRIENDS && this->all->opposes(player_ship->all))
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
					cost=player_ship->purchase(sold[i],all->ripo,false);
					txt+=sprintf(txt,"[%hd] %s \nCost: %ld C  Mass: %hd\n",i+1,sold[i]->nam,cost,sold[i]->mass);
				}
			}
			txt+=sprintf(txt,"\nAvailable mass: %hd\n",player_ship->get_available_cargo_space());
		}
		if(opr>=1 && opr<=MAX_EQUIPMENT_SLOTS && sold[opr-1])
		{
			//cost=player_ship->purchase(sold[opr-1],all->ripo,false);
			
			player_ship->transport(this);
			player_ship->purchase(sold[opr-1],all->ripo,true);
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
		calc::longtodat(loc.x_component,buf);
		buf+=4;
		calc::longtodat(loc.y_component,buf);
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
	database::select_database_object(obsc);
	load();
	this->self=self;
	planets[self]=this;
}

void planet::save()
{
	char atsc[33]; //Attribute scratchpad

	database::store_attribute("Name",nam);
	if(spr)
		database::store_attribute("Sprite",spr);
	database::store_attribute("SpriteRot",rot);
	if(spr)
		database::store_attribute("Team",all->self);
	database::store_attribute("XLoc",loc.x_component);
	database::store_attribute("YLoc",loc.y_component);
	database::store_attribute("Type",typ);
	for(int i=0;i<MAX_EQUIPMENT_SLOTS;i++)
	{
		sprintf(atsc,"Sold%d",i);
		if(sold[i])
			database::store_attribute(atsc,sold[i]->self);
		else
			database::store_attribute(atsc,-1);
	}
}

void planet::load()
{
	char atsc[33]; //Attribute scratchpad

	database::retrieve_attribute("Name",nam);
	spr=database::retrieve_attribute("Sprite");
	rot=database::retrieve_attribute("SpriteRot");
	all=alliance::get(database::retrieve_attribute("Team"));
	loc.x_component=database::retrieve_attribute("XLoc");
	loc.y_component=database::retrieve_attribute("YLoc");
	typ=database::retrieve_attribute("Type");
	for(int i=0;i<MAX_EQUIPMENT_SLOTS;i++)
	{
		sprintf(atsc,"Sold%d",i);
		sold[i]=equip::get(database::retrieve_attribute(atsc));
	}
}

void planet::shipyard()
{
	cord put; //Location to spawn
	ship* template_ship; //Ship from library
	ship* spawned_ship; //Ship being spawned
	int aity; //AI type to spawn

	put.x_component=loc.x_component+calc::random_int(SPAWN_RADIUS)-calc::random_int(SPAWN_RADIUS);
	put.y_component=loc.y_component+calc::random_int(SPAWN_RADIUS)-calc::random_int(SPAWN_RADIUS);

	template_ship=all->get_spawn_ship_template();
	aity=all->get_ai_behavior_type();
	if(template_ship)
	{
		try
		{
			spawned_ship=new ship(put,template_ship,all,aity);
			spawned_ship->insert();
			if(spawned_ship->self==-1)
				delete spawned_ship;
		}
		catch(error it)
		{
		}
	}
}

planet* planet::planets[ISIZE];
