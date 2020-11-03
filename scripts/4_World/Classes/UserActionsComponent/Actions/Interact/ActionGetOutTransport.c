class GetOutTransportActionData : ActionData
{
	Car m_Car;
	vector m_StartLocation;
	float m_CarSpeed;
	bool m_WasJumpingOut = false;
	float m_DmgTaken = 0; 				// Damage taken by the player when jumping out of vehicle
	float m_ShockTaken = 0; 			// Shock inflicted to the player when jumping out of vehicle
}

class ActionGetOutTransport: ActionBase
{

	//For the two following variables -> The HIGHER the value, the LOWER the output
	int m_DmgFactor = 60; 				//value used to translate impact strength into actual damage (impact strength -> velocity squared)
	int m_ShockFactor = 15; 			//Value used to translate impact strength into actual shock

	//Variables used to determine the different speed levels for bleeding checks
	const int LOW_SPEED_VALUE = 20;
	const int HIGH_SPEED_VALUE = 30;
	
	void ActionGetOutTransport()
	{
		m_StanceMask = DayZPlayerConstants.STANCEMASK_ALL;
		//m_HUDCursorIcon = "GetInDriver";
	}
	
	override ActionData CreateActionData()
	{
		ActionData action_data = new GetOutTransportActionData;
		return action_data;
	}


	override void CreateConditionComponents()  
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTNone;
	}

	override string GetText()
	{
		return "#leave_vehicle";
	}

	override typename GetInputType()
	{
		return ContinuousInteractActionInput;
	}
	
	override bool HasProgress()
	{
		return false;
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
 		Transport trans = null;
		int crew_index = -1;

		HumanCommandVehicle vehCommand = player.GetCommand_Vehicle();
		if ( vehCommand )
		{
			trans = vehCommand.GetTransport();
			if ( trans )
			{
				crew_index = trans.CrewMemberIndex( player );
				
/*According today testing (12.06.2020) this hack is no longer needed
				//Hack for NIVA - disabling jumpng out from the back seats
				Car car;
				if ( Class.CastTo(car, trans) )
				{
					if ( car.GetSpeedometer() > 8 && crew_index >= 2 && trans.GetType() == "OffroadHatchback" )
						return false;
				}
*/
				
				if ( crew_index >= 0 && trans.CrewCanGetThrough( crew_index ) && trans.IsAreaAtDoorFree( crew_index ) )
					return true;
			}
		}

		return false;
	}

	override void Start( ActionData action_data )
	{
		super.Start( action_data );
		HumanCommandVehicle vehCommand = action_data.m_Player.GetCommand_Vehicle();
		if( vehCommand )
		{
			Transport trans = vehCommand.GetTransport();
			
			if ( trans )
			{
				GetOutTransportActionData got_action_data = GetOutTransportActionData.Cast(action_data);
				Car car;
				if ( Class.CastTo(car, trans) )
				{
					got_action_data.m_StartLocation = got_action_data.m_Player.GetPosition();
					got_action_data.m_Car = car;
					float speed = car.GetSpeedometer();
					got_action_data.m_CarSpeed = speed;
					got_action_data.m_DmgTaken = (got_action_data.m_CarSpeed * got_action_data.m_CarSpeed) / m_DmgFactor; //When using multiplications, wrong value is returned
					got_action_data.m_ShockTaken = (got_action_data.m_CarSpeed * got_action_data.m_CarSpeed) / m_ShockFactor;
					if ( speed <= 8 )
					{
						vehCommand.GetOutVehicle();
					}
					else
					{
						got_action_data.m_WasJumpingOut = true;
						vehCommand.JumpOutVehicle();
					}
					//action_data.m_Player.TryHideItemInHands(false);
					//action_data.m_Player.GetItemAccessor().OnItemInHandsChanged();
					
					GetDayZGame().GetBacklit().OnLeaveCar();		
					if ( action_data.m_Player.GetInventory() ) 
						action_data.m_Player.GetInventory().LockInventory(LOCK_FROM_SCRIPT);							
				}
			}
		}
	}
	
	//TODO: quick'n'dirt hotfix, refactor!
	override void End( ActionData action_data )
	{
		GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(Unhide,500,false,(action_data.m_Player));
		super.End( action_data );
	}
	
	void Unhide(PlayerBase player)
	{
		player.TryHideItemInHands(false);
	}

	override void OnUpdate(ActionData action_data)
	{
		if (action_data.m_State == UA_START)
		{
			if ( !action_data.m_Player.GetCommand_Vehicle() )
			{
				End(action_data);
			}
			//TODO add some timed check for stuck possibility
			/*else
			{
				End(action_data);
			}*/
		}
	}
	
	override bool CanBeUsedInRestrain()
	{
		return true;
	}
	
	override bool CanBeUsedInVehicle()
	{
		return true;
	}
	
	override int GetActionCategory()
	{
		return AC_INTERACT;
	}
	
	override void OnEnd( ActionData action_data )
	{
		if ( action_data.m_Player.GetInventory() ) 
				action_data.m_Player.GetInventory().UnlockInventory(LOCK_FROM_SCRIPT);
	}
	
	override void OnEndServer( ActionData action_data )
	{
		GetOutTransportActionData got_action_data = GetOutTransportActionData.Cast(action_data);
		vector endLocation = action_data.m_Player.GetPosition();
		
		vector contact_pos;
		vector contact_dir;
		int contact_component;
		
		set<Object> result = new set<Object>;
		
		if (DayZPhysics.RaycastRV(got_action_data.m_StartLocation, endLocation, contact_pos, contact_dir, contact_component, result, got_action_data.m_Car, action_data.m_Player, false, false, ObjIntersectView, 0.3))
		{
			vector offset = got_action_data.m_StartLocation - contact_pos;
			offset.Normalize();
			got_action_data.m_Player.SetPosition(contact_pos + offset);
		}	

		if (got_action_data.m_WasJumpingOut)
		{
			got_action_data.m_Player.OnJumpOutVehicleFinish(got_action_data.m_CarSpeed);
			
			PlayerBase player = got_action_data.m_Player;
			
			ApplyJumpOutDmg(action_data);
			
			vector posMS = player.WorldToModel(player.GetPosition());
			player.DamageAllLegs(got_action_data.m_DmgTaken); //Additionnal leg specific damage dealing
			player.ProcessDirectDamage(DT_CUSTOM, player, "", "FallDamage", posMS, got_action_data.m_DmgTaken);
		}
	}
	
	//Manage all jumping out of vehicle damage logic
	void ApplyJumpOutDmg(ActionData action_data)
	{
		GetOutTransportActionData got_action_data = GetOutTransportActionData.Cast(action_data);
		PlayerBase player = got_action_data.m_Player;
		
		//Get all currently equipped clothing
		// ---------------------------------------------

		ClothingBase trousers = 	ClothingBase.Cast(player.GetItemOnSlot("LEGS"));
		ClothingBase bag = 			ClothingBase.Cast(player.GetItemOnSlot("BACK"));
		ClothingBase vest = 		ClothingBase.Cast(player.GetItemOnSlot("VEST"));
		ClothingBase headGear =  	ClothingBase.Cast(player.GetItemOnSlot("HeadGear"));
		ClothingBase mask =  		ClothingBase.Cast(player.GetItemOnSlot("Mask"));
		ClothingBase shirt = 		ClothingBase.Cast(player.GetItemOnSlot("BODY"));
		ClothingBase shoes =  		ClothingBase.Cast(player.GetItemOnSlot("FEET"));
		ClothingBase gloves = 		ClothingBase.Cast(player.GetItemOnSlot("GLOVES"));

		//Array used to find all relevant information about currently equipped clothes
		array<ClothingBase> equippedClothes = new array<ClothingBase>;

		equippedClothes.Insert(trousers);
		equippedClothes.Insert(bag);
		equippedClothes.Insert(vest);
		equippedClothes.Insert(headGear);
		equippedClothes.Insert(mask);
		equippedClothes.Insert(shirt);
		equippedClothes.Insert(shoes);
		equippedClothes.Insert(gloves);

		// -----------------------------------------------

		//Lower shock taken if player uses a helmet
		headGear = ClothingBase.Cast(player.GetItemOnHead());
		HelmetBase helmet;
		if (Class.CastTo(helmet, headGear))
			got_action_data.m_ShockTaken *= 0.5;

		// -----------------------------------------------
		
		int m_RandNum; //value used for probability evaluation
		m_RandNum = Math.RandomInt(0, 100);
		if (got_action_data.m_CarSpeed < LOW_SPEED_VALUE)
		{
			if (m_RandNum < 20)
				player.GiveShock(-got_action_data.m_ShockTaken); //To inflict shock, a negative value must be passed

			m_RandNum = Math.RandomInt(1, 14);
			switch (m_RandNum)
			{
				case 1:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightToeBase");
				break;

				case 2:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightFoot");
				break;

				case 3:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftToeBase");
				break;

				case 4:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftFoot");
				break;

				case 5:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightUpLegRoll");
				break;

				case 6:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightUpLeg");
				break;

				case 7:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightLegRoll");
				break;

				case 8:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightLeg");
				break;

				case 9:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftUpLegRoll");
				break;

				case 10:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftUpLeg");
				break;

				case 11:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftLegRoll");
				break;

				case 12:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftLeg");
				break;

				case 13:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightForeArmRoll");
				break;

				case 14:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftForeArmRoll");
				break;
			}
		}
		else if (got_action_data.m_CarSpeed >= LOW_SPEED_VALUE && got_action_data.m_CarSpeed < HIGH_SPEED_VALUE)
		{
			if (m_RandNum < 50)
				player.GiveShock(-got_action_data.m_ShockTaken);

			m_RandNum = Math.RandomInt(1, 14);
			switch (m_RandNum)
			{
				case 1:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightForeArm");
				break;

				case 2:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightArmRoll");
				break;

				case 3:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightArm");
				break;

				case 4:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("RightShoulder");
				break;

				case 5:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftForeArm");
				break;

				case 6:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftArmRoll");
				break;

				case 7:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftArm");
				break;

				case 8:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("LeftShoulder");
				break;

				case 9:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Spine3");
				break;

				case 10:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Spine2");
				break;

				case 11:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Spine1");
				break;

				case 12:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Spine");
				break;

				case 13:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Pelvis");
				break;

				case 14:
					player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Neck");
				break;
			}
		}
		else if (got_action_data.m_CarSpeed >= HIGH_SPEED_VALUE)
		{
			headGear = ClothingBase.Cast(player.GetItemOnHead());
			if (!Class.CastTo(helmet, headGear))
				player.m_BleedingManagerServer.AttemptAddBleedingSourceBySelection("Head");

			if (m_RandNum < 75)
				player.GiveShock(-got_action_data.m_ShockTaken);
		}
		
		int nbClothes = 0;

		//Damage all currently equipped clothes
		for (int i = 0; i < equippedClothes.Count(); i++)
		{
			//If no item is equipped on slot, slot is ignored
			if (equippedClothes[i] == null)
				continue;

			equippedClothes[i].DecreaseHealth(got_action_data.m_DmgTaken, false);
			nbClothes++;
		}
	}
};
