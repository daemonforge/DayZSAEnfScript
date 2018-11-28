class ActionWaterGardenSlotCB : ActionContinuousBaseCB
{
	private const float QUANTITY_USED_PER_SEC = 150;
	
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousWaterSlot(QUANTITY_USED_PER_SEC);
	}
};

class ActionWaterGardenSlot: ActionContinuousBase
{
	void ActionWaterGardenSlot()
	{
		m_CallbackClass = ActionWaterGardenSlotCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONFB_EMPTY_VESSEL;
		m_FullBody = true;
		m_MessageSuccess = "I've watered slot.";
		m_MessageStartFail = "There's not enough fertilizer.";
		m_MessageStart = "I've started fertilizing.";
		m_MessageFail = "There's not enough fertilizer.";
		m_MessageCancel = "I stoped fertilizing.";
		m_SpecialtyWeight = UASoftSkillsWeight.ROUGH_LOW;
	}
	
	override void CreateConditionComponents()  
	{
		m_ConditionTarget = new CCTDummy;
		m_ConditionItem = new CCINonRuined;
	}
	
	override int GetType()
	{
		return AT_WATER_GARDEN_SLOT;
	}
	
	override string GetText()
	{
		return "#water_slot";
	}
	
	override bool ActionCondition( PlayerBase player, ActionTarget target, ItemBase item )
	{
		Object targetObject = target.GetObject();
		
		if (item.GetQuantity() == 0)
			return false;
		
		// Get the liquid
		int liquid_type	= item.GetLiquidType();

		if (liquid_type != LIQUID_WATER)
		{
			return false; //  Forbid watering of plants with gasoline and other fluids
		}
		
		if ( targetObject.IsInherited(GardenBase) )
		{
			GardenBase garden_base = GardenBase.Cast( targetObject );
			
			string selection = targetObject.GetActionComponentName(target.GetComponentIndex());
			
			Slot slot = garden_base.GetSlotBySelection( selection );
		
			if ( slot  &&  !slot.GetPlant()  &&  (slot.IsDigged() || slot.IsPlanted())  &&  slot.NeedsWater() ) // !slot.GetPlant() is here because we have separate user action for watering plants
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		
		return false;
	}
};