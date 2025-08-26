/*
	ship.cc
	
	(c) Richard Thrippleton
	Licensing terms are in the 'LICENSE' file
	If that file is not included with this source then permission is not given to use this source in any way whatsoever.
*/

#include <stdio.h>
#include "calc.h"
#include "error.h"
#include "frag.h"
#include "database.h"
#include "planet.h"
#include "equip.h"
#include "server.h"
#include "alliance.h"
#include "constants.h"
#include "protocol.h"
#include "player.h"
#include "ship.h"

ship::ship(cord loc,ship* template_ship,alliance* target_alliance,int aity)
{
	self=-1;
	if(!target_alliance)
		throw error("Null alliance given");
 	if(!template_ship)
		throw error("Null ship template given");
	*this=*template_ship;
	if(strlen(cls) == 0)
		throw error("Invalid ship template - missing class name");
	vel.angle_degrees=calc::random_int(360);
	vel.radius=0;
	this->loc=loc;
	this->all=target_alliance;
	mass_locked=true;
	is_crippled=false;
	assigned_player=NULL;
	this->aity=aity;
	update_equipment_references();
	if(get_available_cargo_space()<=0)
	{
		error::debug(cls,get_available_cargo_space());
	}
}

ship::ship()
{
	self=-1;
	assigned_player=NULL;
	memset(cls, 0, sizeof(cls));
	memset(slots, 0, sizeof(slots));
	for(int i=0;i<32;i++)
		slots[i].pos.radius=-1;
}

ship::~ship()
{
	if(self>=0 && self<ISIZE)
		ships[self]=NULL;
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			if(ships[i]->enemy_target==this)
				ships[i]->enemy_target=NULL;
			if(ships[i]->friendly_target==this)
				ships[i]->friendly_target=NULL;
		}
	}
	frag::notifydelete(this);
	if(assigned_player)
		assigned_player->notifydelete();
}

// ============================================================================
// SHIP CREATION AND MANAGEMENT
// ============================================================================

void ship::init()
{
	for(int i=0;i<ISIZE;i++)
		ships[i]=NULL;
}

void ship::loadlib()
{
	char obnm[33]; //Object name
	
	for(int i=0;i<LIBSIZE;i++)
	{
		try
		{
			lib[i]=new ship();
			try
			{
				sprintf(obnm,"ShipLib%hd",i);
				database::select_database_object(obnm);
				lib[i]->load();
				lib[i]->typ=i;
			}
			catch(error it)
			{
				if(lib[i])
					delete lib[i];
				lib[i]=NULL;
			}
		}
		catch(error it)
		{
			lib[i]=NULL;
		}
	}
}

void ship::purgeall()
{
	for(int i=0;i<ISIZE;i++)
		if(ships[i])
			delete ships[i];
}

void ship::simulateall()
{
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			ships[i]->physics();
			ships[i]->maintain();
		}
	}
}

void ship::behaveall()
{
	master_strobe=(master_strobe+1)%1000; //Increment the master strobe
	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i] && !ships[i]->is_crippled) //Ships that exist and are non-crippled should do behaviour
			ships[i]->execute_ai_behavior();
	}
	for(int i=0,j=0;i<10;i++)
	{
		j=(master_strobe*(i+1))%ISIZE;
		if(ships[j] && !ships[j]->is_crippled) //Non-crippled ships need masslock checking periodically
			ships[j]->mass_locked=planet::masslock(ships[j]->loc);
	}
}

void ship::saveall()
{
	char obsc[16]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		if(ships[i])
		{
			sprintf(obsc,"Ship%hd",i);
			database::putobject(obsc);
			ships[i]->save();
		}
	}
}

void ship::loadall()
{
	char obsc[16]; //Object name scratchpad

	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			new ship(i);
		}
		catch(error it)
		{
		}
	}
	for(int i=0;i<ISIZE;i++)
	{
		try
		{
			if(ships[i])
			{
				sprintf(obsc,"Ship%hd",i);
				database::select_database_object(obsc);
				ships[i]->resolve_object_references();
			}
		}
		catch(error it)
		{
		}
	}
}

ship* ship::find_by_index(int indx)
{
	if(indx>=0 && indx<ISIZE)
	{
		if(ships[indx])
			return ships[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

ship* ship::libget(int indx)
{
	if(indx>=0 && indx<LIBSIZE)
	{
		if(lib[indx])
			return lib[indx];
		else
			return NULL;
	}
	else
	{
		return NULL;
	}
}

bool ship::has_available_ship_slot()
{
	for(int i=0;(i<(ISIZE-server::ISIZE));i++)
		if(!ships[i])
			return true;
	return false;
}

void ship::turn(int dir)
{
	if(power_plant && power_plant->cap>0)
	{
		if(dir==+1)
			vel.angle_degrees+=turn_rate;
		if(dir==-1)
			vel.angle_degrees-=turn_rate;
		if(vel.angle_degrees>=360)
			vel.angle_degrees-=360;
		if(vel.angle_degrees<0)
			vel.angle_degrees+=360;
	}
}

void ship::accel(int dir,bool wrp)
{
	int pcon; //Power consumption
	double nsp; //New speed
	
	nsp=vel.radius;
	//Handle acceleration while at warp
	if(vel.radius>99)
	{
		if(dir==+1)
			nsp=vel.radius+warp_acceleration;
		if(dir==-1)
			nsp=vel.radius-warp_acceleration;
		if(nsp<100) {
			if(wrp)
				nsp=max_impulse_speed;
			else
				nsp=100;
		}
	}
	//Handle acceleration if currently at impulse
	//if(vel.radius<=mip)
	else
	{
		if(dir==+1)
		{
			nsp=vel.radius+impulse_acceleration;
		}
		if(dir==-1)
		{
			nsp=vel.radius-impulse_acceleration;
		}
		if(nsp>max_impulse_speed)
		{
			if(wrp)
				nsp=100;
			else
				nsp=max_impulse_speed;
		}
	}

	if(nsp>max_impulse_speed && nsp<100)
		nsp=max_impulse_speed;
	if(nsp>max_warp_speed)
		nsp=max_warp_speed;

	if(nsp<-(max_impulse_speed/3)) //Prevent going faster backwards than reverse speed
		nsp=-max_impulse_speed/3;

	if(nsp<0 && !wrp && vel.radius>=0) //Prevent moving into reverse if transition not specified
		nsp=0;

	if(nsp>=100)
		pcon=(int)(((vel.radius-nsp)*mass)/2000);
	else
		pcon=(int)(((vel.radius-nsp)*mass)/2);
	if(pcon<0)
		pcon=-pcon;
	if((nsp>=100 && vel.radius<100) || (nsp<100 && vel.radius>=100))
		pcon=0;

	if(power_plant && power_plant->cap>=pcon)
	{
		power_plant->cap-=pcon;
		vel.radius=nsp;
	}
}

// ============================================================================
// SHIP COMBAT SYSTEMS
// ============================================================================

void ship::shoot(bool torp)
{
	pol ptmp;
	vect vtmp; //Temporaries for calculations
	vect corr; //Correction vector
	pol polar_to_target; //Polar to target
	double distance_to_target; //Distance to target
	vect vector_to_target; //Vector to target
	cord emission_location; //Location to emit frag at
	pol emission_polar; //Polar velocity to emit frag at
	vect emission_vector; //Vector to emit frag at
	double angle_difference; //Angle difference
	long weapon_range; //Range
	bool can; //Can shoot from this slot?
	equip* launcher_equipment; //Equipment doing the launching

	if(!can_detect(enemy_target))
		return;
	if(vel.radius>=100)
		return;
	weapon_range=0;
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && ((torp && slots[i].item->equipment_type==equip::LAUNCHER) || (!torp && slots[i].item->equipment_type==equip::PHASER)))
		{
			launcher_equipment=slots[i].item;
			vector_to_target.x_component=enemy_target->loc.x_component-loc.x_component;
			vector_to_target.y_component=enemy_target->loc.y_component-loc.y_component;
			polar_to_target=vector_to_target.to_polar_coordinates();
			distance_to_target=polar_to_target.radius;

			can=false;
			if(launcher_equipment->equipment_type==equip::PHASER)
			{
				weapon_range=launcher_equipment->range*launcher_equipment->trck;
				if(launcher_equipment->trck)
				{
					corr.x_component=((enemy_target->mov.x_component-mov.x_component)*(polar_to_target.radius/launcher_equipment->trck));
					corr.y_component=((enemy_target->mov.y_component-mov.y_component)*(polar_to_target.radius/launcher_equipment->trck));
					if(vector_to_target.x_component && corr.x_component*10/vector_to_target.x_component<10)
						vector_to_target.x_component+=corr.x_component;
					if(vector_to_target.y_component && corr.y_component*10/vector_to_target.y_component<10)
						vector_to_target.y_component+=corr.y_component;
					polar_to_target=vector_to_target.to_polar_coordinates();
					polar_to_target.radius=distance_to_target;
				}
			}
			if(launcher_equipment->equipment_type==equip::LAUNCHER)
				weapon_range=((launcher_equipment->range*launcher_equipment->range)/2)*launcher_equipment->trck;

			if(polar_to_target.radius<=weapon_range)
				can=true;
			if(can)
			{
				angle_difference=polar_to_target.angle_degrees-(vel.angle_degrees+slots[i].face);
				if(angle_difference>180)
					angle_difference=angle_difference-360;
				if(angle_difference<-180)
					angle_difference=angle_difference+360;
				if(angle_difference>(launcher_equipment->acov) || angle_difference<-(launcher_equipment->acov))
					can=false;	
				if(angle_difference>(launcher_equipment->acov) || angle_difference<-(launcher_equipment->acov))
					can=false;		
				if(launcher_equipment->equipment_type==equip::PHASER && power_plant->cap<launcher_equipment->power_requirement)
					can=false;
				if(launcher_equipment->equipment_type==equip::LAUNCHER && !(slots[i].cap>0))
					can=false;
				if(slots[i].rdy!=0)
					can=false;
			}
			if(can)
			{
				uncloak();
				
				ptmp=slots[i].pos;
				ptmp.angle_degrees=(ptmp.angle_degrees+vel.angle_degrees);
				if(ptmp.angle_degrees>=360)
					ptmp.angle_degrees-=360;
				vtmp=ptmp.to_vector_coordinates();
				
				emission_location.x_component=vtmp.x_component+loc.x_component;
				emission_location.y_component=vtmp.y_component+loc.y_component;

				if(torp)
				{
					emission_polar.angle_degrees=slots[i].face+vel.angle_degrees; //Sort out the angle a torpedo is shot at
					if(emission_polar.angle_degrees>=360)
						emission_polar.angle_degrees-=360;
					emission_polar.radius=launcher_equipment->trck*2; //and the speed
				}
				else
				{
					emission_polar.angle_degrees=polar_to_target.angle_degrees; //velocity of phaser fire emission, towards target at fixed speed for this weapon
					emission_polar.radius=launcher_equipment->trck;
				}

				emission_vector=emission_polar.to_vector_coordinates();
				emission_vector.x_component+=mov.x_component;
				emission_vector.y_component+=mov.y_component;

				if(launcher_equipment->equipment_type==equip::PHASER)
				{
					power_plant->cap-=launcher_equipment->power_requirement;
					try
					{
						new frag(emission_location,frag::ENERGY,launcher_equipment->sprite_index,launcher_equipment->color_index,enemy_target,this,emission_vector,((polar_to_target.angle_degrees+5)/10),launcher_equipment->power_requirement,0,launcher_equipment->range);
					}
					catch(error it)
					{
					}
				}
				if(launcher_equipment->equipment_type==equip::LAUNCHER)
				{
					slots[i].cap--;
					try
					{
						new frag(emission_location,frag::HOMER,launcher_equipment->sprite_index,launcher_equipment->color_index,enemy_target,this,emission_vector,0,launcher_equipment->power_requirement,launcher_equipment->trck,launcher_equipment->range);
					}
					catch(error it)
					{
					}
				}
				server::registernoise(this,launcher_equipment->sound_index);
				slots[i].rdy=launcher_equipment->readiness_timer;
				if(torp)
					break;
			}
		}
	}
}

bool ship::can_detect(ship* target_ship)
{
	double weapon_range; //Effective range

	if(!target_ship) //Null ship
		return false;
	if(target_ship==this) //Can always see self
		return true;
	if(sensor_array) //Set the sensor range, or default if no sensor suite
		weapon_range=sensor_array->item->range;
	else
		weapon_range=1000;
	if(target_ship->vel.radius<20) //Slower ships less visible
		weapon_range-=(((weapon_range/2)*(20-target_ship->vel.radius))/20);
	if(target_ship->cloaking_device && target_ship->cloaking_device->cap==target_ship->cloaking_device->item->capacity) //Cloaked ships even less visible
		weapon_range/=8;
	if((target_ship->loc.x_component-loc.x_component)>weapon_range) //Bounds checking
		return false;
	if((target_ship->loc.x_component-loc.x_component)<-weapon_range)
		return false;
	if((target_ship->loc.y_component-loc.y_component)>weapon_range)
		return false;
	if((target_ship->loc.y_component-loc.y_component)<-weapon_range)
		return false;
	return true;
}

bool ship::can_detect(planet* target_planet)
{
	double weapon_range; //Effective range

	if(!target_planet) //Null planet
		return false;
	if(sensor_array) //Set the sensor range, or default if no sensor suite
		weapon_range=sensor_array->item->range;
	else
		weapon_range=1000;
	if(target_planet->typ==planet::STAR) //Can always see stars
		return true;
	if((target_planet->loc.x_component-loc.x_component)>weapon_range) //Bounds checking
		return false;
	if((target_planet->loc.x_component-loc.x_component)<-weapon_range)
		return false;
	if((target_planet->loc.y_component-loc.y_component)>weapon_range)
		return false;
	if((target_planet->loc.y_component-loc.y_component)<-weapon_range)
		return false;
	return true;		
}

bool ship::can_detect(frag* tfrg)
{
	double weapon_range; //Effective range

	if(!tfrg) //Null frag
		return false;
	if(sensor_array) //Set the sensor range, or default if no sensor suite
		weapon_range=sensor_array->item->range;
	else
		weapon_range=1000;
	if(tfrg->trg==this || tfrg->own==this) //For bandwidth spamming reasons, only see frags when really close unless they concern you
	{
		if((tfrg->loc.x_component-loc.x_component)>weapon_range)
			return false;
		if((tfrg->loc.x_component-loc.x_component)<-weapon_range)
			return false;
		if((tfrg->loc.y_component-loc.y_component)>weapon_range)
			return false;
		if((tfrg->loc.y_component-loc.y_component)<-weapon_range)
			return false;
	}
	else
	{
		if((tfrg->loc.x_component-loc.x_component)>500)
			return false;
		if((tfrg->loc.x_component-loc.x_component)<-500)
			return false;
		if((tfrg->loc.y_component-loc.y_component)>500)
			return false;
		if((tfrg->loc.y_component-loc.y_component)<-500)
			return false;
	}
	return true;		
}

int ship::interact(char* txt,short cmod,short opr,ship* player_ship)
{
	char spd[32]; //Speed

	if(!(player_ship && player_ship->assigned_player))
		return -1;
	switch(cmod)
	{
		case CMOD_STAT:
		case CMOD_SCAN:
		if(opr==-1)
		{
			if(player_ship->can_detect(this))
			{
				txt+=sprintf(txt,"%s\n",cls);
				if(player_ship->all->opposes(all))
					txt+=sprintf(txt,"Alignment:%s [hostile]\n",all->nam);
				else
					txt+=sprintf(txt,"Alignment:%s\n",all->nam);
				if(assigned_player)
					txt+=sprintf(txt,"Commanded by %s\n",assigned_player->nam);
				if(shield_generator && shield_generator->cap>0)
					txt+=sprintf(txt,"\nShields: Raised\n");
				else
					txt+=sprintf(txt,"\nShields: Down\n");
				calc::getspeed(max_warp_speed,spd);
				txt+=sprintf(txt,"Maximum velocity: %s\n",spd);
				if(shield_generator)
					txt+=sprintf(txt,"Shield capability: %ld\n",shield_generator->item->capacity);
				else
					txt+=sprintf(txt,"No shields");
				if(power_plant)
					txt+=sprintf(txt,"Maximum power capacity: %ld\n",power_plant->item->capacity);
				else
					txt+=sprintf(txt,"No power plant");
				if(fuel_tank)
					txt+=sprintf(txt,"Maximum fuel storage: %ld\n",fuel_tank->item->capacity);
				else
					txt+=sprintf(txt,"No fuel storage");

				txt+=sprintf(txt,"\nAvailable mass: %hd\n",get_available_cargo_space());

				if(this==player_ship)
				{
//					txt+=sprintf(txt,"\nAvailable mass: %hd\n",get_available_cargo_space());
					txt+=sprintf(txt,"\nCredits: %ld\n",assigned_player->cashi);
				}
				if(this==player_ship->enemy_target)
					txt+=sprintf(txt,"\n[1] Lay in an intercept course\n");
				return spr;
			}
			else
			{
				txt+=sprintf(txt,"Target not visible\n");
				if(this==player_ship->enemy_target)
					txt+=sprintf(txt,"\n[1] Lay in an intercept course\n");
				return -1;
			}
		}
		if(opr==1 && this==player_ship->enemy_target)
		{
			player_ship->aity=AI_AUTOPILOT;
		}
		break;

		
		case CMOD_EQUIP:
		if(!(selected_equipment_index>=0 && selected_equipment_index<32 && slots[selected_equipment_index].item))
		{
			selected_equipment_index=-1;
			for(int i=0;i<32;i++)
			{
				if(slots[i].item)
				{
					selected_equipment_index=i;
					break;
				}
			}
		}
		if(opr==-1)
		{
			txt+=sprintf(txt,"Internal systems\n\n");
			for(int i=0;i<32;i++)
			{
				if(slots[i].item)
				{
					if(slots[i].item->equipment_type==equip::LAUNCHER)
						if(i==selected_equipment_index)
							txt+=sprintf(txt,">%s [%ld]<\n",slots[i].item->nam,slots[i].cap);
						else
							txt+=sprintf(txt," %s [%ld]\n",slots[i].item->nam,slots[i].cap);
					else if(slots[i].item->equipment_type==equip::FUELTANK && slots[i].cap==0)
						if(i==selected_equipment_index)
							txt+=sprintf(txt,">%s< [empty]\n",slots[i].item->nam);
						else
							txt+=sprintf(txt," %s [empty]\n",slots[i].item->nam);
					else
						if(i==selected_equipment_index)
							txt+=sprintf(txt,">%s<\n",slots[i].item->nam);
						else
							txt+=sprintf(txt," %s\n",slots[i].item->nam);
				}
				else
				{
					if(slots[i].pos.radius>=0)
					{
						if(slots[i].face<=90 || slots[i].face>=270)
							txt+=sprintf(txt," <Free forward port>\n");
						else
							txt+=sprintf(txt," <Free rear port>\n");
					}
				}
			}
			txt+=sprintf(txt,"\n");
			if(shield_generator)
				txt+=sprintf(txt,"[1] Toggle shields\n");
			if(cloaking_device)
				txt+=sprintf(txt,"[2] Toggle cloak\n");
			txt+=sprintf(txt,"\n[3] Select equipment\n");
			txt+=sprintf(txt,"[4] Jettison selection\n");
		}
		if(opr==1) {
			if(shield_generator && shield_generator->rdy==-1)
				shieldsup();
			else
				shieldsdown();
		}
		if(opr==2) {
			if(cloaking_device && cloaking_device->rdy==-1)
				cloak();
			else
				uncloak();
		}
		if(opr==3)
		{
			for(int i=0,j=selected_equipment_index+1;i<32;i++,j++)
			{
				if(j>=32)
					j=0;
				if(slots[j].item)
				{
					selected_equipment_index=j;
					break;
				}
			}
		}
		if(opr==4)
		{
			if(selected_equipment_index>=0 && selected_equipment_index<32 && slots[selected_equipment_index].item)
			{
				if(slots[selected_equipment_index].item->equipment_type==equip::TRANSPORTER)
				{
					sprintf(txt,"Cannot jettison transporters");
				}
				else
				{
					sprintf(txt,"%s jettisoned",slots[selected_equipment_index].item->nam);
					slots[selected_equipment_index].item=NULL;
					slots[selected_equipment_index].rdy=0;
					slots[selected_equipment_index].cap=0;
					update_equipment_references();
				}
			}
		}
		break;

		case CMOD_HAIL:
		if(is_crippled)
		{
			if(opr==-1)
			{
				txt+=sprintf(txt,"Hailing ship\n\n");
				txt+=sprintf(txt,"Vessel is disabled\n\n[1] Attempt to recover it");
			}
			if(opr==1)
			{
				player_ship->transport(this);
				enemy_target=NULL;
				planet_target=NULL;
				friendly_target=player_ship;
				for(int i=0;i<ISIZE;i++)
					if(ships[i] && ships[i]->enemy_target==this)
						ships[i]->enemy_target=NULL;
				txt+=sprintf(txt,"Vessel successfully acquired");
				player_ship->assigned_player->transfer(this);
			}
		}
		else
		{
			if(friendly_target==player_ship)
			{
				if(opr==-1)
				{
					txt+=sprintf(txt,"Hailing ship\n\n");
					txt+=sprintf(txt,"Vessel is under your command\n\n[1] Transfer to this vessel");
				}
				if(opr==1)
				{
					try
					{
						player_ship->transport(this);	
						player_ship->assigned_player->transfer(this);
						txt+=sprintf(txt,"Transfer of command successful");
					}
					catch(error it)
					{
						try
						{
							transport(player_ship);
							player_ship->assigned_player->transfer(this);
						}
						catch(error iti)
						{
							throw it;
						}
					}
				}
			}
			else
			{
				txt+=sprintf(txt,"Hailing ship\n\n");
				txt+=sprintf(txt,"No reply");
			}
		}
		break;
		
		case CMOD_WHOIS:
		if(assigned_player)
		{
			txt+=sprintf(txt,"Player: %s\n",assigned_player->nam);
			txt+=sprintf(txt,"Alliance: %s\n",all->nam);
			return spr;
		}
		else
			txt+=sprintf(txt,"Target not player controlled\n");
		break;
	}
	return -1;
}

int ship::get_available_cargo_space()
{
	int out; //Outputted free space

	out=mass;
	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
			out-=slots[i].item->mass;
	}
	return out;
}

void ship::cloak()
{
	if(cloaking_device && cloaking_device->rdy!=0)
	{
		cloaking_device->rdy=0;
		server::registernoise(this,cloaking_device->item->sound_index);
	}
}

void ship::uncloak()
{
	if(cloaking_device && cloaking_device->rdy!=-1)
	{
		cloaking_device->rdy=-1;
		if(cloaking_device->cap>0)
			cloaking_device->cap=-cloaking_device->cap;
		server::registernoise(this,cloaking_device->item->sound_index);
	}
}

void ship::shieldsup()
{
	if(shield_generator)
		shield_generator->rdy=0;
}

void ship::shieldsdown()
{
	if(shield_generator)
		shield_generator->rdy=-1;
}

void ship::serialize_to_network(int typ,unsigned char* buf)
{
	buf[0]=typ;
	buf+=1;

	calc::inttodat(ship2pres(self),buf);
	buf+=2;
	switch(typ)
	{
		case SERV_SELF:
		if(mass>0)
			calc::inttodat((100*hull_integrity)/max_hull_integrity,buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(power_plant && power_plant->cap>0)
			calc::inttodat((100*power_plant->cap)/(power_plant->item->capacity),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(shield_generator && shield_generator->cap>0)
			calc::inttodat((100*shield_generator->cap)/(shield_generator->item->capacity),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;

		if(fuel_tank && fuel_tank->cap>0)
			calc::inttodat((100*fuel_tank->cap)/(fuel_tank->item->capacity),buf);
		else
			calc::inttodat(0,buf);
		buf+=2;
		if(sensor_array)
			calc::longtodat(sensor_array->item->range,buf);
		else
			calc::longtodat(0,buf);
		buf+=4;
		calc::longtodat(LIMIT,buf);
		buf+=4;
		if(planet_target)
		{
			calc::inttodat(planet2pres(planet_target->self),buf);
		}
		else
		{
			if(enemy_target)
				calc::inttodat(ship2pres(enemy_target->self),buf);
			else
				calc::inttodat(-1,buf);
		}
		buf+=2;
		calc::inttodat(-1,buf);
		buf+=2;
		calc::inttodat(-1,buf);
		break;

		case SERV_NEW:
		*buf=PT_SHIP;
		buf+=1;
		calc::inttodat(spr,buf);
		buf+=2;
		calc::inttodat(-1,buf);
		buf+=2;
		break;

		case SERV_NAME:
		sprintf((char*)buf,"%s",cls);
		buf+=64;
		sprintf((char*)buf,"%s",all->nam);
		buf+=64;
		break;

		case SERV_UPD:
		calc::longtodat(loc.x_component,buf);
		buf+=4;
		calc::longtodat(loc.y_component,buf);
		buf+=4;
		calc::longtodat(mov.x_component,buf);
		buf+=4;
		calc::longtodat(mov.y_component,buf);
		buf+=4;
		calc::inttodat(vel.angle_degrees,buf);
		buf+=2;
		*buf=0;
		buf+=1;
		if(cloaking_device && cloaking_device->item->capacity)
		{
			if(cloaking_device->cap>=0)
				*buf=100-((100*cloaking_device->cap)/cloaking_device->item->capacity);
			else
				*buf=100+((100*cloaking_device->cap)/cloaking_device->item->capacity);
		}
		else
			*buf=100;
		buf+=1;
		break;

		default:
		break;
	}
}

bool ship::detect_collision(cord fragment_location,vect fragment_velocity)
{
	int rot; //Target rotation
	double x1,y1,x2,y2,xx,yy; //Target bounding box

	rot=(int)(((vel.angle_degrees+5)/10))%36;
	xx=(fragment_velocity.x_component-mov.x_component)/2;
	yy=(fragment_velocity.y_component-mov.y_component)/2;
	if(xx<0)
		xx=-xx;
	if(yy<0)
		yy=-yy;
	x1=loc.x_component-(w[rot]*3)/2-xx;
	y1=loc.y_component-(h[rot]*3)/2-yy;
	x2=loc.x_component+(w[rot]*3)/2+xx;
	y2=loc.y_component+(h[rot]*3)/2+yy;
	if(fragment_location.x_component>x1 && fragment_location.x_component<x2 && fragment_location.y_component>y1 && fragment_location.y_component<y2)
		return true;
	else
		return false;
}

void ship::take_damage(int mag,cord fragment_location,vect fragment_velocity,ship* src)
{
	cord tmpc;
	vect tmpv; //Temporary for working out detonations
	int rot; //Target rotation
	int debris_count; //Number of debris bits

	uncloak();
	rot=(int)(((vel.angle_degrees+5)/10))%36;
	if(shield_generator)
		shield_generator->cap-=mag;
	server::registershake(this,mag/100);
	if(src && enemy_target!=src && !(all->opposes(src->all)) && src->assigned_player)
		src->alert_nearby_ships();
	if(shield_generator && shield_generator->cap>0)
	{
		try
		{
			new frag(fragment_location,frag::DEBRIS,shield_generator->item->sprite_index,-1,NULL,this,mov,calc::random_int(36),0,0,2);
		}
		catch(error it)
		{
		}
	}
	else
	{
		if(shield_generator)
			shield_generator->cap=0;
		fragment_location.x_component=(fragment_location.x_component+2*loc.x_component)/3;
		fragment_location.y_component=(fragment_location.y_component+2*loc.y_component)/3;
		for(int i=0;i<5;i++)
		{
			tmpv=mov;
			fragment_location.x_component+=calc::random_int(2)-calc::random_int(2);
			fragment_location.y_component+=calc::random_int(2)-calc::random_int(2);
			tmpv.x_component+=calc::random_int(2)-calc::random_int(2);
			tmpv.y_component+=calc::random_int(2)-calc::random_int(2);
			try
			{
				new frag(fragment_location,frag::DEBRIS,frag::FIRE,-1,NULL,this,tmpv,calc::random_int(36),0,0,calc::random_int(5)+5);
			}
			catch(error it)
			{
			}
		}
		server::registernoise(this,fsnd);
	}

	if(shield_generator && shield_generator->cap!=-10)
		hull_integrity-=(mag*4)/(shield_generator->cap+10);
	else
		hull_integrity-=(mag*4)/10;

	if(hull_integrity<=0)
	{
		hull_integrity=0;
		debris_count=mass/8+4;
		if(debris_count>70)
			debris_count=70;
		for(int i=0;i<debris_count;i++)
		{	
			if(i==0 || calc::random_int(5)==0)
			{
				tmpc=loc;
				tmpc.x_component+=calc::random_int(2*w[rot])-calc::random_int(2*w[rot]);
				tmpc.y_component+=calc::random_int(2*h[rot])-calc::random_int(2*h[rot]);
			}
			tmpv=mov;
			try
			{
				if(calc::random_int(10)<3)
				{
					new frag(tmpc,frag::DEBRIS,frag::FIRE,-1,NULL,this,tmpv,calc::random_int(36),0,0,calc::random_int(20)+5);
				}
				else
				{
					tmpv.x_component+=calc::random_int(2)-calc::random_int(2);
					tmpv.y_component+=calc::random_int(2)-calc::random_int(2);
					new frag(tmpc,frag::DEBRIS,fspr,-1,NULL,this,tmpv,calc::random_int(36),0,0,calc::random_int(20)+5);
				}
			}
			catch(error it)
			{
			}
		}
		if(src && src->assigned_player && src->all->opposes(all))
		{
			server::hail(NULL,src->assigned_player,"Target destroyed; bounty paid");
			src->assigned_player->credit(mass/2);
		}
		server::registernoise(this,dsnd);
		if(assigned_player)
			server::bulletin("%s has been destroyed",assigned_player->nam);
		delete this;
	}
	else
	{
		if(hull_integrity<max_hull_integrity/2 && src && src->assigned_player && !assigned_player && !is_crippled)
		{
			server::hail(NULL,src->assigned_player,"Target crippled");
			is_crippled=true;
		}
	}
}

void ship::assign(player* assigned_player)
{
	this->assigned_player=assigned_player;
	enemy_target=NULL;
	friendly_target=NULL;
	is_crippled=false;
	aity=AI_NULL;
	update_equipment_references();
}

long ship::purchase(int prch,short ripo,bool buy)
{
	long cost; //Value to output

	cost=0;
	if(prch==PRCH_FUEL)
	{
		if(fuel_tank)
		{
			if((fuel_tank->cap)<(fuel_tank->item->capacity))
				cost=ripo/4;
			if(buy)
			{
				fuel_tank->cap=fuel_tank->item->capacity;
				assigned_player->debit(cost);
			}
		}
	}
	if(prch==PRCH_ARMS)
	{
		for(int i=0;i<32;i++)
		{
			if(slots[i].item && slots[i].item->equipment_type==equip::LAUNCHER)
			{
				if(slots[i].cap<slots[i].item->capacity)
				{
					cost=(slots[i].item->cost*ripo)/1000;
					if(buy)
					{
						assigned_player->debit(cost);
						slots[i].cap=slots[i].item->capacity;
					}
					break;
				}
			}
		}
	}
	if(prch==PRCH_HULL)
	{
		cost=((max_hull_integrity-hull_integrity)*ripo)/100;
		if(buy)
		{
			assigned_player->debit(cost);
			hull_integrity=max_hull_integrity;
		}
	}
	return cost;
}

long ship::purchase(equip* prch,int ripo,bool buy)
{
	long cost; //Value to output

	if(!prch)
		return 0;
	cost=(prch->cost*ripo)/100;
	if(buy && !(prch->mass>get_available_cargo_space()))
	{
		for(int i=0;i<32;i++)
		{
			if(!slots[i].item)
			{
				if((slots[i].pos.radius>=0 && (prch->equipment_type==equip::PHASER || prch->equipment_type==equip::LAUNCHER)) || (slots[i].pos.radius==-1 && prch->equipment_type!=equip::PHASER && prch->equipment_type!=equip::LAUNCHER))
				{
					assigned_player->debit(cost);
					slots[i].item=prch;
					slots[i].cap=prch->capacity;
					slots[i].rdy=prch->readiness_timer;
					update_equipment_references();
					break;
				}
			}
		}
	}
	return cost;
}

void ship::transport(planet* to)
{
	vect vto;
	pol pto; //Vectors to the target

	vto.x_component=to->loc.x_component-loc.x_component;
	vto.y_component=to->loc.y_component-loc.y_component;
	pto=vto.to_polar_coordinates();
	if(shield_generator && shield_generator->cap>0)
		throw error("Cannot transport with shields up");
	if(!power_plant)
		throw error("Not enough power to transport");
	if(cloaking_device && cloaking_device->cap!=0)
		throw error("Cannot transport while cloaked");
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && slots[i].item->equipment_type==equip::TRANSPORTER && slots[i].rdy==0 && power_plant->cap>=slots[i].item->power_requirement && slots[i].item->range>=pto.radius)
		{
			power_plant->cap-=slots[i].item->power_requirement;
			slots[i].rdy=slots[i].item->readiness_timer;
			server::registersound(this,slots[i].item->sound_index);
			return;
		}
	}
	throw error("No available transporters ready or powered");
}

void ship::transport(ship* to)
{
	vect vto;
	pol pto; //Vectors to the target

	vto.x_component=to->loc.x_component-loc.x_component;
	vto.y_component=to->loc.y_component-loc.y_component;
	pto=vto.to_polar_coordinates();
	if(shield_generator && shield_generator->cap>0)
		throw error("Cannot transport with shields up");
	if(!power_plant)
		throw error("Not enough power to transport");
	if(cloaking_device && cloaking_device->cap!=0)
		throw error("Cannot transport while cloaked");
	if(to->shield_generator && to->shield_generator->cap>0)
		throw error("Cannot transport through destination's shields");
	if(to->cloaking_device && to->cloaking_device->cap!=0)
		throw error("Cannot transport through destination's cloak");
	for(int i=0;i<32;i++)
	{
		if(slots[i].item && slots[i].item->equipment_type==equip::TRANSPORTER && slots[i].rdy==0 && power_plant->cap>=slots[i].item->power_requirement && slots[i].item->range>=pto.radius)
		{
			power_plant->cap-=slots[i].item->power_requirement;
			slots[i].rdy=slots[i].item->readiness_timer;
			server::registersound(this,slots[i].item->sound_index);
			return;
		}
	}
	throw error("No available transporters ready or powered");
}

void ship::save()
{
	char atsc[33]; //Attribute scratchpad

	database::store_attribute("Class",cls);
	database::store_attribute("Type",typ);
	database::store_attribute("ShipSprite",spr);
	database::store_attribute("Width",w[0]);
	database::store_attribute("Height",h[0]);
	if(fspr)
		database::store_attribute("FragSprite",fspr);
	if(fsnd)
		database::store_attribute("FragSound",fsnd);
	if(dsnd)
		database::store_attribute("DeathSound",dsnd);
	if(all)
		database::store_attribute("Team",all->self);
	database::store_attribute("AIType",aity);
	database::store_attribute("XLoc",loc.x_component);
	database::store_attribute("YLoc",loc.y_component);
	database::store_attribute("Heading",vel.angle_degrees);
	database::store_attribute("Speed",vel.radius);
	database::store_attribute("TurnRate",turn_rate);
	database::store_attribute("SublightLimit",max_impulse_speed);
	database::store_attribute("SublightAcceleration",impulse_acceleration*10);
	database::store_attribute("WarpLimit",max_warp_speed);
	database::store_attribute("WarpAcceleration",warp_acceleration);
	database::store_attribute("Mass",mass);
	database::store_attribute("HullStrength",hull_integrity);
	database::store_attribute("HullStrengthLimit",max_hull_integrity);
	if(friendly_target)
		database::store_attribute("FriendTarget",friendly_target->self);
	if(enemy_target)
		database::store_attribute("EnemyTarget",enemy_target->self);
	if(planet_target)
		database::store_attribute("PlanetTarget",planet_target->self);
	database::store_attribute("MassLock",mass_locked);
	database::store_attribute("Crippled",is_crippled);
	for(int i=0;i<32;i++)
	{
		if(slots[i].item || slots[i].pos.radius!=-1)
		{
			sprintf(atsc,"Slot%hdAngle",i);
			database::store_attribute(atsc,slots[i].pos.angle_degrees);
			sprintf(atsc,"Slot%hdRadius",i);
			database::store_attribute(atsc,slots[i].pos.radius);
			sprintf(atsc,"Slot%hdFace",i);
			database::store_attribute(atsc,slots[i].face);
			sprintf(atsc,"Slot%hdItem",i);
			if(slots[i].item)
				database::store_attribute(atsc,slots[i].item->self);
			else
				database::store_attribute(atsc,-1);
			sprintf(atsc,"Slot%hdReadiness",i);
			database::store_attribute(atsc,slots[i].rdy);
			sprintf(atsc,"Slot%hdCapacity",i);
			database::store_attribute(atsc,slots[i].cap);
		}
	}
}

void ship::load()
{
	char atsc[33]; //Attribute scratchpad
	pol bpol;
	vect vct1,vct2; //Temporaries for calculating the bounding box

	database::retrieve_attribute("Class",cls);
	typ=database::retrieve_attribute("Type");
	spr=database::retrieve_attribute("ShipSprite");
	w[0]=database::retrieve_attribute("Width");
	h[0]=database::retrieve_attribute("Height");
	for(int i=1;i<36;i++)
	{
		bpol.angle_degrees=i*10;
		bpol.radius=h[0];
		vct1=bpol.to_vector_coordinates();
		bpol.angle_degrees=(i*10+90)%360;
		bpol.radius=w[0];
		vct2=bpol.to_vector_coordinates();
		if(vct1.x_component<0)
			vct1.x_component=-vct1.x_component;
		if(vct2.x_component<0)
			vct2.x_component=-vct2.x_component;
		if(vct1.y_component<0)
			vct1.y_component=-vct1.y_component;
		if(vct2.y_component<0)
			vct2.y_component=-vct2.y_component;
		if(vct1.x_component>vct2.x_component)
			w[i]=(int)vct1.x_component;
		else
			w[i]=(int)vct2.x_component;
		if(vct1.y_component>vct2.y_component)
			h[i]=(int)vct1.y_component;
		else
			h[i]=(int)vct2.y_component;
	}
	fspr=database::retrieve_attribute("FragSprite");
	fsnd=database::retrieve_attribute("FragSound");
	dsnd=database::retrieve_attribute("DeathSound");
	all=alliance::get(database::retrieve_attribute("Team"));
	aity=database::retrieve_attribute("AIType");
	loc.x_component=database::retrieve_attribute("XLoc");
	loc.y_component=database::retrieve_attribute("YLoc");
	vel.angle_degrees=database::retrieve_attribute("Heading");
	vel.radius=database::retrieve_attribute("Speed");
	turn_rate=database::retrieve_attribute("TurnRate");
	max_impulse_speed=database::retrieve_attribute("SublightLimit");
	impulse_acceleration=(double)database::retrieve_attribute("SublightAcceleration")/10;
	max_warp_speed=database::retrieve_attribute("WarpLimit");
	warp_acceleration=database::retrieve_attribute("WarpAcceleration");
	mass=database::retrieve_attribute("Mass");
	hull_integrity=database::retrieve_attribute("HullStrength");
	max_hull_integrity=database::retrieve_attribute("HullStrengthLimit");
	
	mass_locked=database::retrieve_attribute("MassLock");
	is_crippled=database::retrieve_attribute("Crippled");

	selected_equipment_index=-1;
	for(int i=0;i<32;i++)
	{
		// Initialize slot to safe defaults
		slots[i].pos.angle_degrees=0;
		slots[i].pos.radius=-1;
		slots[i].face=0;
		slots[i].item=NULL;
		slots[i].rdy=0;
		slots[i].cap=0;
		
		try {
			sprintf(atsc,"Slot%hdAngle",i);
			slots[i].pos.angle_degrees=database::retrieve_attribute(atsc);
			sprintf(atsc,"Slot%hdRadius",i);
			slots[i].pos.radius=database::retrieve_attribute(atsc);
			sprintf(atsc,"Slot%hdFace",i);
			slots[i].face=database::retrieve_attribute(atsc);
			if(slots[i].face==-1)
				slots[i].face=slots[i].pos.angle_degrees;
			sprintf(atsc,"Slot%hdItem",i);
			long item_id = database::retrieve_attribute(atsc);
			if(item_id >= 0) {
				slots[i].item=equip::get(item_id);
			}
			sprintf(atsc,"Slot%hdReadiness",i);
			slots[i].rdy=database::retrieve_attribute(atsc);
			sprintf(atsc,"Slot%hdCapacity",i);
			slots[i].cap=database::retrieve_attribute(atsc);
			if(slots[i].cap==-1 && slots[i].item)
				slots[i].cap=slots[i].item->capacity;
		} catch(...) {
			// If loading fails, leave slot in safe default state
		}
	}

	if(hull_integrity==-1)
		hull_integrity=max_hull_integrity;
	assigned_player=NULL;

	mov.x_component=0;
	mov.y_component=0;

	friendly_target=NULL;
	enemy_target=NULL;
	planet_target=NULL;

	if(aity==-1)
		aity=AI_NULL;

	update_equipment_references();
}

void ship::insert()
{
	self=-1;
	if(assigned_player)
	{
		for(int i=0;i<ISIZE && self==-1;i++)
			if(!ships[i])
				self=i;
	}
	else
	{
		for(int i=0;(i<(ISIZE-server::ISIZE)) && self==-1;i++)
			if(!ships[i])
				self=i;
	}
	if(self==-1)
		throw error("No free ship index available");
	else
		ships[self]=this;
}

void ship::insert(int self)
{
	this->self=-1;
	if(!(self>=0 && self<ISIZE))
		return;
	this->self=self;
	if(ships[self])
		delete ships[self];
	ships[self]=this;
}

ship::ship(int self)
{
	char obsc[16]; //Object name scratchpad

	sprintf(obsc,"Ship%hd",self);
	database::select_database_object(obsc);
	load();
	insert(self);
}

// ============================================================================
// SHIP PHYSICS AND MOVEMENT
// ============================================================================

void ship::physics()
{
	vect nmov; //New movement vector

	//Slow down vessels at warp under masslock influence
	if(vel.radius>=100 && mass_locked)
		vel.radius=max_impulse_speed;
	//Handle ships in between warp 1 and maximum impulse
	if(vel.radius<100 && vel.radius>max_impulse_speed)
		vel.radius=max_impulse_speed;

	//Handle ships going beyond the boundaries of the 'universe'; they bounce
	if(loc.x_component>LIMIT || loc.x_component<-LIMIT || loc.y_component>LIMIT || loc.y_component<-LIMIT)
	{
		vel.angle_degrees=(vel.angle_degrees+180);
		if(vel.angle_degrees>=360)
			vel.angle_degrees-=360;
	}
	if(loc.x_component>LIMIT)
		loc.x_component=LIMIT;
	if(loc.x_component<-LIMIT)
		loc.x_component=-LIMIT;
	if(loc.y_component>LIMIT)
		loc.y_component=LIMIT;
	if(loc.y_component<-LIMIT)
		loc.y_component=-LIMIT;
	
	nmov=vel.to_vector_coordinates();
	if(vel.radius<100 && (mass/100)!=0)
	{
		mov.x_component+=(nmov.x_component-mov.x_component)/(mass/100);
		mov.y_component+=(nmov.y_component-mov.y_component)/(mass/100);
		//loc.x_component+=(nmov.x_component-mov.x_component);
		//loc.y_component+=(nmov.y_component-mov.y_component);
	}
	else
	{
		mov=nmov;
	}

	loc.x_component+=mov.x_component;
	loc.y_component+=mov.y_component;
}


void ship::navigate_to_planet(planet* target_planet)
{
	vect vector_to_target; //Vector to target
	pol polar_to_target; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	vector_to_target.x_component=(self*497)%800-400+target_planet->loc.x_component-loc.x_component;
	vector_to_target.y_component=(self*273)%800-400+target_planet->loc.y_component-loc.y_component; //Vector to deterministic but arbitrary location near target planet

	polar_to_target=vector_to_target.to_polar_coordinates(); //...make polar

	dd=polar_to_target.angle_degrees-vel.angle_degrees;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	tol=turn_rate+2;
	if(vel.radius<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	polar_to_target.radius-=150; //Stand off distance

	if(polar_to_target.radius<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && polar_to_target.radius>0) //Only accelerate when heading at target
	{
		if(vel.radius>=100)
		{
			if(vel.radius<sqrt(2*warp_acceleration*polar_to_target.radius)-warp_acceleration)
				accel(+1,true);
			else if(vel.radius>sqrt(2*warp_acceleration*polar_to_target.radius)+warp_acceleration)
				accel(-1,true);
		}
		else
		{
			if(vel.radius<(sqrt(2*impulse_acceleration*polar_to_target.radius)-impulse_acceleration))  //Intended speed is sqrt(2as)
				accel(+1,true);
			else if(vel.radius>(sqrt(3*impulse_acceleration*polar_to_target.radius))+impulse_acceleration)
				accel(-1,true);
		}
	}
	else
		accel(-1,false);
	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::follow(ship* target_ship)
{
	vect vector_to_target; //Vector to target
	pol polar_to_target; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	polar_to_target.angle_degrees=target_ship->vel.angle_degrees+90+(self*29)%180; //Find deterministic formation angle to hold at around target ship
	polar_to_target.radius=100+(self*17)%((sensor_array ? sensor_array->item->range : 1000)/16); //Deterministic range to hold based on sensor range
	vector_to_target=polar_to_target.to_vector_coordinates();
		
	vector_to_target.x_component+=target_ship->loc.x_component-loc.x_component;
	vector_to_target.y_component+=target_ship->loc.y_component-loc.y_component;
	polar_to_target=vector_to_target.to_polar_coordinates(); //Get polar vector to this formation position

	dd=polar_to_target.angle_degrees-vel.angle_degrees;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	if(!can_detect(target_ship))
		polar_to_target.radius-=(sensor_array ? sensor_array->item->range : 1000)/3; //If you can't see the target, stand off a little

	tol=turn_rate*2+2;
	if(vel.radius<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	polar_to_target.radius-=150; //Default stand off

	if(polar_to_target.radius<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && polar_to_target.radius>0) //Only accelerate when heading at target
	{
		if(vel.radius>=100)
		{
			if(vel.radius<sqrt(2*warp_acceleration*polar_to_target.radius)-warp_acceleration)
				accel(+1,true);
			else if(vel.radius>sqrt(2*warp_acceleration*polar_to_target.radius)+warp_acceleration)
				accel(-1,true);
		}
		else
		{
			if(vel.radius<(sqrt(2*impulse_acceleration*polar_to_target.radius)-2*impulse_acceleration))  //Intended speed is sqrt(2as)
				accel(+1,true);
			else if(vel.radius>(sqrt(2*impulse_acceleration*polar_to_target.radius)+2*impulse_acceleration))
				accel(-1,true);
		}
	}
	else
		accel(-1,false);

	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::execute_attack_maneuvers(ship* target_ship,int str)
{
	vect vector_to_target; //Vector to target
	pol polar_to_target; //Polar to target
	double dd; //Directional difference
	double tol; //Angular tolerance

	if(!can_detect(target_ship) || vel.radius>=100) //If you can't see or are warp pursuing the target ship, default to the follow method
	{
		follow(target_ship);
		return;
	}
	if(str>(200+calc::random_int((self%7)*12)-50)) //Alternate on tailing target from one of two sides
		polar_to_target.angle_degrees=target_ship->vel.angle_degrees+45+(str-self*29)%135;
	else
		polar_to_target.angle_degrees=target_ship->vel.angle_degrees-45-(str+self*29)%135;
	polar_to_target.radius=100+(self*17)%((str+(sensor_array ? sensor_array->item->range : 1000))/16); //Back off a little depending on sensor range
	vector_to_target=polar_to_target.to_vector_coordinates();
		
	vector_to_target.x_component+=target_ship->loc.x_component-loc.x_component;
	vector_to_target.y_component+=target_ship->loc.y_component-loc.y_component;
	polar_to_target=vector_to_target.to_polar_coordinates(); //And finally get a polar to the 'formation' position

	dd=polar_to_target.angle_degrees-vel.angle_degrees;
	if(dd>180)
		dd=dd-360;
	if(dd<-180)
		dd=dd+360; //Evaluate angle between current heading and target bearing

	if(!can_detect(target_ship))
		polar_to_target.radius-=(sensor_array ? sensor_array->item->range : 1000)/3; //If you can't see the target, stand off a little
	tol=turn_rate*2+2;

	if(vel.radius<=5) 
		tol=20; //Low speed; not too fussed about fine direction finding

	tol+=45; //Widen angle tolerance for close combat flair

	if(polar_to_target.radius<0) //Don't turn when too close
		dd=0;

	if(dd<tol && dd>-tol) //Don't turn when within angle tolerance
		dd=0;

	if(dd==0 && polar_to_target.radius>0) //Only accelerate when heading at target
	{
		if(vel.radius>=100)
		{
			if(polar_to_target.radius && (polar_to_target.radius/vel.radius) && ((vel.radius)/(12*polar_to_target.radius/vel.radius))>=warp_acceleration-30)
				accel(-1,true);
			else
				accel(+1,true);
		}
		else
		{
			if(polar_to_target.radius && ((5*vel.radius*vel.radius)/(polar_to_target.radius))>=impulse_acceleration)
				accel(-1,true);
			else
				accel(+1,true);
		}
	}
	else
		accel(-1,false);
	if(dd>0)
		turn(+1);
	if(dd<0)
		turn(-1);
}

void ship::resolve_object_references()
{
	friendly_target=ship::find_by_index(database::retrieve_attribute("FriendTarget"));
	enemy_target=ship::find_by_index(database::retrieve_attribute("EnemyTarget"));
	planet_target=planet::find_by_index(database::retrieve_attribute("PlanetTarget"));
}

void ship::maintain()
{
	if(is_crippled)
	{
		if(shield_generator)
			shield_generator->rdy=-1;	
		if(cloaking_device)
			cloaking_device->rdy=-1;
		if(calc::random_int(402)==0)
			take_damage(1000,loc,mov,NULL);
	}
	else
	{
		if(power_plant && fuel_tank)
		{
			if((power_plant->cap)<(power_plant->item->capacity) && fuel_tank->cap>0)
			{
				fuel_tank->cap-=power_plant->item->power_requirement;
				power_plant->cap+=power_plant->item->power_requirement;
				if(fuel_tank->cap<0)
					fuel_tank->cap=0;
			}
			if((power_plant->cap)>(power_plant->item->capacity))
			{
				fuel_tank->cap+=(power_plant->cap)-(power_plant->item->capacity);
				power_plant->cap=power_plant->item->capacity;
			}
		}
	}
	
	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
		{
			if(slots[i].rdy>0)
				slots[i].rdy--;
			else
				if(slots[i].rdy<0 && (slots[i].item->equipment_type==equip::PHASER || slots[i].item->equipment_type==equip::LAUNCHER || slots[i].item->equipment_type==equip::TRANSPORTER))
					slots[i].rdy=0;
		}
	}

	if(shield_generator && shield_generator->rdy==0 && power_plant && (power_plant->cap)>=shield_generator->item->power_requirement)
	{
		shield_generator->cap+=shield_generator->item->power_requirement;
		power_plant->cap-=shield_generator->item->power_requirement;
		if(shield_generator->cap>shield_generator->item->capacity)
			shield_generator->cap=shield_generator->item->capacity;
	}
	else
	{
		if(shield_generator)
			shield_generator->cap/=2;
	}
	if(cloaking_device)
	{
		if(cloaking_device->rdy==0)
		{
			if(power_plant && (power_plant->cap)>=(cloaking_device->item->power_requirement*mass)/20)
				power_plant->cap-=(cloaking_device->item->power_requirement*mass)/20;
			else
				uncloak();
			if(cloaking_device->cap<cloaking_device->item->capacity)
				cloaking_device->cap++;
		}
		else
		{
			if(cloaking_device->cap>=0)
				cloaking_device->cap=0;
			else
				cloaking_device->cap++;
		}
	}

	if(power_plant && power_plant->cap<0)
		power_plant->cap=0;
	if((!fuel_tank || (fuel_tank && fuel_tank->cap==0)) && !assigned_player && calc::random_int(100)==0)
		delete this;
}

// ============================================================================
// SHIP AI BEHAVIOR SYSTEMS
// ============================================================================
//
// AI Behavior Types:
// - AI_NULL: No AI behavior (player controlled)
// - AI_AUTOPILOT: Navigate to selected planet
// - AI_PATROLLER: Patrol area and engage hostiles
// - AI_INVADER: Aggressive attack behavior
// - AI_CARAVAN: Trade route behavior
// - AI_BUDDY: Follow and assist allied ships
// - AI_FLEET: Coordinated fleet behavior
//

void ship::execute_ai_behavior()
{
	int individual_strobe; //Individual strobe for this ship
	bool run_amortized_code; //Run amortised cost code for this state?
	planet* target_planet; //A planet for use in ai code
	ship* target_ship; //A ship for use in ai code

	individual_strobe=(master_strobe+self*7)%400;
	if(individual_strobe%40==0)
		run_amortized_code=true;
	else
		run_amortized_code=false;

	//Run behaviours for each case of this ship's behaviour state
	switch(aity)
	{
		case AI_AUTOPILOT:
		if(enemy_target)
			follow(enemy_target);
		else if(planet_target)
			navigate_to_planet(planet_target);
		break;

		case AI_PATROLLER:
		if(enemy_target)
		{
			execute_attack_maneuvers(enemy_target,individual_strobe);
			handle_weapon_targeting(individual_strobe);
		}
		else if(planet_target)
			navigate_to_planet(planet_target);

		if(run_amortized_code)
		{
			if(!enemy_target)
			{
				shieldsdown();
				enemy_target=find_hostile_target();
			}
			else
			{
				shieldsup();
				if(planet_target && !can_detect(planet_target))
					enemy_target=NULL;
			}
			if(!planet_target || planet_target->all!=all || vel.radius<=5)
			{
				target_planet=planet::find_random_planet(all);
				if(target_planet && target_planet->typ!=planet::STAR && can_detect(target_planet))
					planet_target=target_planet;
			}
		}
		break;

		case AI_INVADER:
		if(enemy_target)
		{
			execute_attack_maneuvers(enemy_target,individual_strobe);
			handle_weapon_targeting(individual_strobe);
		}
		else if(planet_target)
			navigate_to_planet(planet_target);

		if(run_amortized_code)
		{
			if(planet_target)
				cloak();
			if(enemy_target && calc::random_int(10)==0)
				enemy_target=NULL;
			if(!enemy_target)
			{
				shieldsdown();
				target_ship=find_hostile_target();
				if(target_ship)
				{
					enemy_target=target_ship;
					planet_target=NULL;
				}
			}
			else
				shieldsup();
			if(!enemy_target && !planet_target)
				planet_target=planet::find_hostile_planet(all);
		}
		break;

		case AI_CARAVAN:
		if(enemy_target)
		{
			handle_weapon_targeting(individual_strobe);
			if((planet_target && individual_strobe<200) || !planet_target)
				execute_attack_maneuvers(enemy_target,individual_strobe);
			else
				navigate_to_planet(planet_target);
		}
		else if(planet_target)
			navigate_to_planet(planet_target);

		if(run_amortized_code)
		{
			if(!enemy_target)
			{
				shieldsdown();
				enemy_target=find_hostile_target();
			}
			else
				shieldsup();
			if(!planet_target || all->opposes(planet_target->all) || vel.radius<=5)
			{
				target_planet=planet::find_allied_planet(all);
				if(target_planet && target_planet->typ!=planet::STAR)
					planet_target=target_planet;
			}
		}
		break;

		case AI_BUDDY:
		if(enemy_target)
		{
			execute_attack_maneuvers(enemy_target,individual_strobe);	
			handle_weapon_targeting(individual_strobe);
		}
		else if(friendly_target)
			follow(friendly_target);

		if(run_amortized_code)
		{
			if(!enemy_target)
				shieldsdown();
			if(enemy_target)
			{
				shieldsup();
				if(calc::random_int(10)==0)
					enemy_target=NULL;
			}
			if(!enemy_target)
				enemy_target=find_hostile_target();
			if(friendly_target)
			{
				if(friendly_target->cloaking_device && friendly_target->cloaking_device->cap!=0)
					cloak();
				if(enemy_target && !can_detect(friendly_target))
					enemy_target=NULL;
				if(friendly_target->friendly_target)
					friendly_target=friendly_target->friendly_target;
				if(friendly_target==this)
					friendly_target=NULL;
			}
			else
				friendly_target=find_allied_ship();
		}
		break;

		case AI_FLEET:
		if(enemy_target)
		{
			if(shield_generator && shield_generator->cap<shield_generator->item->capacity)
				execute_attack_maneuvers(enemy_target,individual_strobe);	
			else if(friendly_target)
				follow(friendly_target);
			handle_weapon_targeting(individual_strobe);
		}
		else if(friendly_target)
			follow(friendly_target);

		if(run_amortized_code)
		{
			if(!enemy_target)
				shieldsdown();
			if(enemy_target)
			{
				shieldsup();
				if(calc::random_int(10)==0)
					enemy_target=NULL;
			}
			if(!enemy_target) {
				if(friendly_target && friendly_target->enemy_target)
					enemy_target=friendly_target->enemy_target;
				else
					enemy_target=find_hostile_target();
		}
			if(friendly_target)
			{
				if(friendly_target->cloaking_device && friendly_target->cloaking_device->cap!=0)
					cloak();
				if(enemy_target && !can_detect(friendly_target))
					enemy_target=NULL;
				if(friendly_target->friendly_target)
					friendly_target=friendly_target->friendly_target;
				if(friendly_target==this)
					friendly_target=NULL;
			}
			else
				friendly_target=find_allied_ship();
		}
		break;
	}
}

ship* ship::find_hostile_target()
{
        for(int i=0,j=0;i<ISIZE;i++)
        {
                j=calc::random_int(ISIZE);
                if(ships[j] && ships[j]!=this && all->opposes(ships[j]->all) && can_detect(ships[j]))
                        return ships[j];
        }
        return NULL;
}

ship* ship::find_allied_ship()
{
        for(int i=0,j=0;i<ISIZE;i++)
        {
                j=calc::random_int(ISIZE);
                if(ships[j] && ships[j]!=this && !ships[j]->assigned_player && all==ships[j]->all && can_detect(ships[j]))
                        return ships[j];
        }
        return NULL;
}

void ship::alert_nearby_ships()
{
	for(int i=0;i<ISIZE;i++)
		if(ships[i] && !ships[i]->assigned_player && ships[i]!=this && !ships[i]->enemy_target && ships[i]->can_detect(this))
			ships[i]->enemy_target=this;
}

void ship::handle_weapon_targeting(int str)
{
	if(enemy_target && can_detect(enemy_target))
	{
		shoot(false);
		//Shoot torpedoes if they appear more threatening; i.e. have a greater maximum shield capacity
		if(str>50 && enemy_target->shield_generator && this->shield_generator && ((enemy_target->shield_generator->item->capacity)*2)>(this->shield_generator->item->capacity))
		{
			shoot(true);
		}
	}
}

// ============================================================================
// SHIP EQUIPMENT MANAGEMENT
// ============================================================================

void ship::update_equipment_references()
{
	power_plant=NULL;
	shield_generator=NULL;
	sensor_array=NULL;
	cloaking_device=NULL;
	fuel_tank=NULL;

	for(int i=0;i<32;i++)
	{
		if(slots[i].item)
		{
			switch(slots[i].item->equipment_type)
			{
				case equip::POWER:
				power_plant=&slots[i];
				break;

				case equip::SHIELD:
				shield_generator=&slots[i];
				break;

				case equip::SENSOR:
				sensor_array=&slots[i];
				break;

				case equip::CLOAK:
				cloaking_device=&slots[i];
				break;

				case equip::FUELTANK:
				fuel_tank=&slots[i];
				break;
			}
		}
	}
}

// ============================================================================
// SHIP CLASS - STATIC DATA AND INITIALIZATION
// ============================================================================

ship* ship::ships[ISIZE];
ship* ship::lib[LIBSIZE];
int ship::master_strobe;
