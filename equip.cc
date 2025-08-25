/*
	equip.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "database.h"
#include "error.h"
#include "equip.h"

void equip::init()
{
}

void equip::loadlib()
{
	char nam[16]; //Object name to load equipment data from

	for(int i=0;i<LIBSIZE;i++)
	{
		sprintf(nam,"Equipment%hd",i);
		try
		{
			database::switchobj(nam);
			equips[i]=new equip(i);
			equips[i]->load();
		}
		catch(error it)
		{
		}
	}
}

equip* equip::get(int indx)
{
	if(indx>=0 && indx<LIBSIZE)
	{
		if(equips[indx])
			return equips[indx];
		else
			return NULL;
	}
	else
		return NULL;
}

equip::equip(int self)
{
	this->self=self;
}

void equip::load()
{
	database::getvalue("Name",nam);
	equipment_type=database::getvalue("Type");
	mass=database::getvalue("Mass");
	sprite_index=database::getvalue("Sprite");
	color_index=database::getvalue("Colour");
	sound_index=database::getvalue("Sound");
	power_requirement=database::getvalue("Power");
	readiness_timer=database::getvalue("CycleTime");
	capacity=database::getvalue("Capacity");
	range=database::getvalue("Range");
	trck=database::getvalue("Tracking");
	acov=database::getvalue("Coverage");
	cost=database::getvalue("Cost");
}

equip* equip::equips[LIBSIZE];
