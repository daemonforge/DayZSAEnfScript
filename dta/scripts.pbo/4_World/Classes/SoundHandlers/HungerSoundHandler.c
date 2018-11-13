class HungerSoundHandlerBase extends SoundHandlerBase
{
	override void Init()
	{
		m_Id = eSoundHandlers.HUNGER;
	}
	
}

//---------------------------
// Client
//---------------------------
class HungerSoundHandlerClient extends HungerSoundHandlerBase
{
	const float SOUND_INTERVALS_LIGHT_MIN = 3;	const float SOUND_INTERVALS_LIGHT_MAX = 20;
	float m_SoundTime;
	EffectSound m_Sound;
	
	override void Update()
	{
		if( m_Player.GetMixedSoundStates() & eMixedSoundStates.HUNGRY )
		{
			ProcessSound();
		}
	}
	
	void ProcessSound()
	{
		if( GetGame().GetTime() > m_SoundTime)
		{
			float offset_time = Math.RandomFloatInclusive(SOUND_INTERVALS_LIGHT_MIN, SOUND_INTERVALS_LIGHT_MAX) * 1000;
			m_SoundTime = GetGame().GetTime() + offset_time;
			PlaySound();
		}
	}
	
	void PlaySound()
	{
		//Print("------------- hungry_uni_Char_SoundSet  --------------");
		m_Sound = SEffectManager.PlaySoundOnObject("hungry_uni_Char_SoundSet", m_Player);
	}
}


//---------------------------
// Server
//---------------------------
class HungerSoundHandlerServer extends HungerSoundHandlerBase
{
	
}