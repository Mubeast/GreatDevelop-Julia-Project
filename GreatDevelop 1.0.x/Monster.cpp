// ================================================== //
// # GameServer 1.00.90 WzAG.dll					# //
// # GreatDevelop 1.0.1								# //
// ================================================== //

#include "StdAfx.h"
#include "User.h"
#include "DropSystem.h"
#include "Utilits.h"
#include "PCPoint.h"
#include "Configs.h"
#include "Logger.h"
#include "ChatCommands.h"



// Monster die Handler (hooked replacement for gObjMonsterDieGiveItem)
void __cdecl MonsterDie(LPOBJ lpObj, LPOBJ lpTargetObj)
{
	PCPoint.RewardsPointsKillMob(lpTargetObj,lpObj,PCPOINT);
	PCPoint.RewardsPointsKillMob(lpTargetObj,lpObj,WCOIN);
	// Original function
	gObjMonsterDieGiveItem(lpObj, lpTargetObj);
}

#ifdef _GS
int MonsterAddAndSpawn(WORD Monster,BYTE Speed,BYTE Map,BYTE X, BYTE Y)
{
	int MobCount = *(DWORD *)(MonsterCount);

	if((MobCount + 1) < MonsterCount)
	{
		MobCount++;
		*(DWORD *)(MonsterCount) = MobCount;
		*(WORD *)(12 * MobCount + MonsterReads) = Monster;
		*(BYTE *)(12 * MobCount + MonsterReads+2) = Map;
		*(BYTE *)(12 * MobCount + MonsterReads+3) = Speed;
		*(BYTE *)(12 * MobCount + MonsterReads+4) = X;
		*(BYTE *)(12 * MobCount + MonsterReads+5) = Y;
		*(BYTE *)(12 * MobCount + MonsterReads+6) = 2;
		*(BYTE *)(12 * MobCount + MonsterReads+7) = X;
		*(BYTE *)(12 * MobCount + MonsterReads+8) = Y;

		int MobID = gObjAddMonster(*(BYTE *)(12 * MobCount + (MonsterReads+2)));
		if(MobID>=0)
		{
			int MobNr = *(WORD *)(12 * MobCount + MonsterReads);
			gObjSetPosMonster(MobID, MobCount);
			gObjSetMonster(MobID, MobNr);
		}

		return MobID;
	}else
	{		
		MessageBox(NULL,"Monster attribute max over!!", "Monsters overflow", 0);
		return -1;
	}
}

void ReadMonsterAdd()
{ 
		FILE* MonsterFile;
		if((MonsterFile = fopen( GreatDevelopMobAdd, "r")) == NULL)
		{						   
			MessageBox(NULL, "Cant Find MonsterSpawn.ini", "Error", 0);
			return;
		}
								
		rewind(MonsterFile);
		char Buff[255];	
		while(!feof(MonsterFile))
		{							
			Buff[0] = 0;
			fgets (Buff, 255, MonsterFile);	 
			if(Buff[0] == '/' || Buff[0] == '#' || Buff[0] == ' ' || strlen(Buff) < 9)
				continue;			

			int Mob = -1, Cnt = -1, Map = -1, Speed = -1, X = -1, Y = -1;
			sscanf(Buff, "%d %d %d %d %d %d", &Mob, &Cnt, &Speed, &Map, &X, &Y);

			if(Mob == -1 || Cnt == -1 || Map == -1 || X == -1 || Y == -1)
				continue;
								  
			for(int i = 0; i < Cnt; i++)
				MonsterAddAndSpawn(Mob,Speed,Map,X,Y);
		}	 
		rewind(MonsterFile);
		fclose(MonsterFile); 
}
#endif
//Monster Death Control
int MygEventMonsterItemDrop(BYTE *b_MonsterDataAddr,BYTE *a_gObjAddr)
{
	// Get mIndex, aIndex & Monster & Player Objects
	PBYTE a_aIndex = 0;
	PBYTE b_mIndex = 0;

	a_aIndex = (PBYTE)a_gObjAddr;
	b_mIndex = (PBYTE)b_MonsterDataAddr;

	WORD mIndex = 0;
	WORD aIndex = 0;

	memcpy(&mIndex, b_mIndex+0x00,sizeof(WORD));
	memcpy(&aIndex, a_aIndex+0x00,sizeof(WORD));

	OBJECTSTRUCT *mObj = (OBJECTSTRUCT*) OBJECT_POINTER (mIndex);
	OBJECTSTRUCT *pObj = (OBJECTSTRUCT*) OBJECT_POINTER (aIndex);

	//ZenFixes
	if(Config.Zen.Enabled)
	{
		int NewMoney = Utilits.gObjZenSingle(pObj,mObj,500,700);
		if(pObj->ChangeUP2)mObj->Money = (NewMoney/1000) * Config.Zen.MasterZen; 
		else mObj->Money = (NewMoney/1000) * Config.Zen.NormalZen;
	}
	//PartyZenFixes
	if(Config.PartyZen.Enabled)
	{
		if(pObj->PartyNumber != -1)
		{
			int AllPartyLevel = 0;
			int Count = 0;

			for(int i=OBJECT_MIN;i<OBJECT_MAX;i++)
			{
				OBJECTSTRUCT *gObj = (OBJECTSTRUCT*) OBJECT_POINTER (i);

				if(gObj->Connected == PLAYER_PLAYING)
				{
					if(gObj->PartyNumber == pObj->PartyNumber)
					{
						AllPartyLevel += gObj->Level;
						Count++;
					}
				}
			} 
			//Master Party
			if(pObj->ChangeUP2)
				mObj->Money = (mObj->Money * Config.PartyZen.MasterZen)*((AllPartyLevel/Count)/(1000)); 
			//Normal
			else  
				mObj->Money = (mObj->Money * Config.PartyZen.NormalZen)*((AllPartyLevel/Count)/1000);
		}
	}

	// Drop System
	if(DropSystem.DropItem(mObj,pObj))
	{
		return 1;
	}
	else // Default drop
	{
		int rValue = gEventMonsterItemDrop(b_MonsterDataAddr, a_gObjAddr);
		return rValue;
	}

	return 1;
}
 
char Messages1[1024];

void NPCMessage(int IndexPlayer, LPOBJ mObj, char* Msg,...)
{						 
	Messages1[0] = 0;
	va_list pArguments1;
	va_start(pArguments1, Msg);
	vsprintf_s(Messages1, Msg, pArguments1);
	va_end(pArguments1);

	ChatTargetSend(mObj, Messages1, IndexPlayer);
}

void NPCMessageLog(sColor LogColor, sLogType LogType, LPOBJ gObj, LPOBJ mObj, char* Msg,...)
{						  
	Messages1[0] = 0;
	va_list pArguments1;
	va_start(pArguments1, Msg);
	vsprintf_s(Messages1, Msg, pArguments1);
	va_end(pArguments1);

	ChatTargetSend(mObj, Messages1, gObj->m_Index); 
	Log.ConsoleOutPut(1, LogColor, LogType, "[%s]: %s", gObj->Name, Messages1);
}

void NPCMessageNear(LPOBJ mObj, char* Msg,...)
{						   
	Messages1[0] = 0;
	va_list pArguments1;
	va_start(pArguments1, Msg);
	vsprintf_s(Messages1, Msg, pArguments1);
	va_end(pArguments1);

	for(int i = OBJECT_MIN; i <= OBJECT_MAX; i++)
	{						   
		OBJECTSTRUCT *gObj = (OBJECTSTRUCT*)OBJECT_POINTER(i);	   
		if(gObj->Connected < 3) continue; 	   
		if(mObj->MapNumber != gObj->MapNumber) continue;	 		 
		if(mObj->X <= gObj->X-10 || mObj->X >= gObj->X+10) continue;
		if(mObj->Y <= gObj->Y-10 || mObj->Y >= gObj->Y+10) continue;
		ChatTargetSend(mObj, Messages1, i);
	}
}