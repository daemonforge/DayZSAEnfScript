class ActionGetInTransport: ActionInteractBase
{
	private Transport m_transport;
	private int       m_crewIdx;


	void ActionGetInTransport()
	{
		m_MessageSuccess = "";
		//m_CommandUID = DayZPlayerConstants.;
		m_StanceMask = DayZPlayerConstants.STANCEMASK_CROUCH | DayZPlayerConstants.STANCEMASK_ERECT;
		m_HUDCursorIcon = "GetInDriver";
	}


	override void CreateConditionComponents()  
	{
		m_ConditionItem = new CCINone;
		m_ConditionTarget = new CCTNone;
	}

	override int GetType()
	{
		return AT_GETIN_TRANSPORT;
	}

	override string GetText()
	{
		return "Get in driver";
	}

	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		m_transport = null;
		m_crewIdx   = -1;

		if ( !target )
			return false;

		if ( !Class.CastTo(m_transport, target.GetObject()) )
			return false;

		//if ( !IsInReach(player, target, UAMaxDistances.DEFAULT) )
		//	return false;

		int componentIndex = target.GetComponentIndex();
		//Print( " component( " + componentIndex + " ) " );

		// check if doors are present and opened
		//if ( car.IsActionComponentPartOfSelection(componentIndex, "doors_driver") )
		//{
		//	if ( !car.GetAnimationPhase("DoorsDriver") >= 0.5 )
		//	{
		//		return false;
		//	}
		//}

		m_crewIdx = m_transport.CrewPositionIndex(componentIndex);
		if ( m_crewIdx < 0 )
			return false;

		return true;
	}

	override void Start( PlayerBase player, ActionTarget target, ItemBase item )
	{
		HumanCommandVehicle vehCommand = player.StartCommand_Vehicle(m_transport, m_crewIdx, DayZPlayerConstants.VEHICLESEAT_DRIVER);
		if( vehCommand )
		{
			vehCommand.SetVehicleType(3);
		}
		m_transport.CrewGetIn(player, m_crewIdx);
	}
	
	override bool IsLocal()
	{
		return true;
	}
	
	override bool IsInstant()
	{
		return true;
	}
/*	
	override void OnCompleteServer( PlayerBase player, ActionTarget target, ItemBase item, Param acdata )
	{
		if ( m_crewIdx >= 0 )
			m_transport.CrewGetIn(player, m_crewIdx);
	}
	override void OnCompleteClient( PlayerBase player, ActionTarget target, ItemBase item, Param acdata )
	{
		if ( m_crewIdx >= 0 )
			m_transport.CrewGetIn(player, m_crewIdx);
	}*/
};
