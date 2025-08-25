/*
	alliance.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "database.h"
#include "calc.h"
#include "error.h"
#include "constants.h"
#include "planet.h"
#include "equip.h"
#include "ship.h"
#include "alliance.h"

void alliance::init()
{
	for(int i=0;i<LIBSIZE;i++)
		alliances[i]=NULL;
}

void alliance::loadlib()
{
	char nam[12]; //Object name to get

	for(int i=0;i<LIBSIZE;i++)
	{
		sprintf(nam,"Alliance%hd",i);
		try
		{
			database::switchobj(nam);
			alliances[i]=new alliance(i);
			alliances[i]->load();
		}
		catch(error it)
		{
		}
	}
}

void alliance::purgeall()
{
	for(int i=0;i<LIBSIZE;i++)
		if(alliances[i])
			delete alliances[i];
}

alliance* alliance::get(int indx)
{
	if(indx>=0 && indx<LIBSIZE)
	{
		if(alliances[indx])
			return alliances[indx];
		else
			return NULL;
	}
	else
		return NULL;
}

void alliance::maketerritories()
{
	cord seed; //Where to seed each territory from

	for(int i=0;i<LIBSIZE;i++)
	{
		if(alliances[i])
		{
			seed.x=calc::rnd(LIMIT/2)-calc::rnd(LIMIT/2);
			seed.y=calc::rnd(LIMIT/2)-calc::rnd(LIMIT/2);
			alliances[i]->maketerritory(seed);
		}
	}
}

bool alliance::opposes(alliance* all)
{
	if(all==NULL)
		return false;
	if(this->grp!=all->grp)
		return true;
	else
		return false;
}

equip* alliance::get_random_equipment()
{
	return eqps[calc::rnd(16)];
}

ship* alliance::get_spawn_ship_template()
{
	int shpc[ship::LIBSIZE]; //Count of each ship type
	ship* tshp; //Ship being examined

	for(int i=0;i<ship::LIBSIZE;i++)
		shpc[i]=shpq[i];
	for(int i=0;i<ship::ISIZE;i++)
	{
		tshp=ship::get(i);
		if(tshp && tshp->typ>=0 && tshp->typ<=ship::LIBSIZE)
			shpc[tshp->typ]--;
	}
	for(int i=0,j=0;i<ship::LIBSIZE*2;i++)
	{
		j=calc::rnd(ship::LIBSIZE);
		if(shpc[j]>0)
			return ship::libget(j);
	}
	return NULL;
}

int alliance::get_ai_behavior_type()
{
	int aic[32]; //Count of each AI type
	ship* tshp; //Ship being examined

	for(int i=0;i<32;i++)
		aic[i]=aiq[i];
	for(int i=0;i<ship::ISIZE;i++)
	{
		tshp=ship::get(i);
		if(tshp && tshp->aity>=0 && tshp->aity<=32)
			aic[tshp->aity]--;
	}
	for(int i=0,j=0;i<64;i++)
	{
		j=calc::rnd(32);
		if(aic[j]>0)
			return j;
	}
	return ship::AI_PATROLLER;
}

alliance::alliance(int self) //Constructor, giving the alliance its own index in the database
{
	this->self=self;
}

alliance::~alliance() //Destructor
{
	if(self>=0 && self<LIBSIZE)
		alliances[self]=NULL;
}

void alliance::maketerritory(cord seed) //Generate the star systems for this alliance at given seed co-ordinates
{
	cord lsys; //Star system location
	cord lpln; //Location of planet
	char snam[65]; //Name of system
	char pnam[80]; //Name of planet
	int ptyp; //Type of planet

	for(int i=0;i<nsys;i++)
	{
		lsys=seed;
		lsys.x=seed.x+calc::rnd(LIMIT/5)-calc::rnd(LIMIT/5);
		lsys.y=seed.y+calc::rnd(LIMIT/5)-calc::rnd(LIMIT/5);
		lpln=lsys;
		for(int j=0,n=calc::rnd(14)+2;j<n;j++)
		{
			if(j==0)
			{
				ptyp=planet::STAR;
				planet::generatename(snam);
				sprintf(pnam,"%s",snam);
			}
			else
			{
				if(calc::rnd(100)<pinh)
				{
					ptyp=planet::INHABITED;
					planet::generatename(pnam);
				}
				else
				{
					ptyp=planet::UNINHABITED;
					snprintf(pnam,sizeof(pnam),"%s %hd",snam,j);
				}
			}
			try
			{
				new planet(pnam,lpln,ptyp,this);
			}
			catch(error it)
			{
			}
			lpln.x=lsys.x+calc::rnd(10000)-5000;
			lpln.y=lsys.y+calc::rnd(10000)-5000;
		}
	}
}

void alliance::load()
{
	char atsc[33]; //Attribute scratchpad

	database::getvalue("Name",nam);
	grp=database::getvalue("Group");
	nsys=database::getvalue("Systems");
	pinh=database::getvalue("PercentInhabited");
	ripo=database::getvalue("Ripoff");
	trad=database::getvalue("Trading");
	spw=ship::libget(database::getvalue("Spawn"));
	for(int i=0;i<ship::LIBSIZE;i++)
	{
		sprintf(atsc,"ShipQuota%hd",i);
		shpq[i]=database::getvalue(atsc);
	}
	for(int i=0;i<32;i++)
	{
		sprintf(atsc,"AIQuota%hd",i);
		aiq[i]=database::getvalue(atsc);
	}
	for(int i=0;i<16;i++)
	{
		sprintf(atsc,"Equipment%hd",i);
		eqps[i]=equip::get(database::getvalue(atsc));
	}
}

alliance* alliance::alliances[LIBSIZE];
