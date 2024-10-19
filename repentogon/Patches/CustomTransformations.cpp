#include "SigScan.h"
#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "HookSystem.h"

#include "Windows.h"
#include <string>
#include <dwmapi.h>
#include "EntityPlus.h"
#include "XMLData.h"

HOOK_METHOD(Entity_Player, IncrementPlayerFormCounter, (int ePlayerForm, int num)->void)
{
	if (ePlayerForm > 14)
	{
		EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(this);
		int prev = playerPlus->customPlayerForms[ePlayerForm - 14];
		playerPlus->customPlayerForms[ePlayerForm - 14] += num;
		if (playerPlus->customPlayerForms[ePlayerForm - 14] < 0)
			playerPlus->customPlayerForms[ePlayerForm - 14] = 0;
		if (playerPlus->customPlayerForms[ePlayerForm - 14] == 3 && playerPlus->customPlayerForms[ePlayerForm - 14] > prev)
		{

		}
		else if (prev == 3 && playerPlus->customPlayerForms[ePlayerForm - 14] < prev)
		{

		}
	}
	else
	{
		super(ePlayerForm, num);
	}
}

HOOK_METHOD(Entity_Player, AddCollectible, (int type, int charge, bool firsttime, int slot, int vardata)->void)
{
	if (firsttime)
	{
		for (int i = 14; i <= XMLStuff.PlayerFormData->maxid; i++) {
			bool contributesToForm = false;
			if ((g_Manager->GetItemConfig()->GetCollectible(type)->tags & g_Manager->GetItemConfig()->GetPlayerForms()->at(i)->tags) > 0)
				contributesToForm = true;
			else
			{
				set<string> itemCustomTags = XMLStuff.ItemData->customtags[type];
				set<string> formCustomTags = XMLStuff.PlayerFormData->customtags[i];
				set<string> sharedCustomTags;
				std::set_intersection(itemCustomTags.begin(), itemCustomTags.end(), formCustomTags.begin(), formCustomTags.end(), inserter(sharedCustomTags, sharedCustomTags.begin()));
				if (sharedCustomTags.size() > 0)
					contributesToForm = true;
			}

			if (contributesToForm)
			{
				this->IncrementPlayerFormCounter(i, 1);
			}
		}
	}
	super(type, charge, firsttime, slot, vardata);
}

HOOK_METHOD(Entity_Player, TriggerCollectibleRemoved, (unsigned int collectibleID)->void)
{
	printf("%d\n", collectibleID);
	if (g_Manager->GetItemConfig()->GetCollectible(collectibleID)->type == 3)
	{
		super(collectibleID);
		return;
	}
	for (int i = 14; i <= XMLStuff.PlayerFormData->maxid; i++) {
		bool contributesToForm = false;
		if ((g_Manager->GetItemConfig()->GetCollectible(collectibleID)->tags & g_Manager->GetItemConfig()->GetPlayerForms()->at(i)->tags) > 0)
			contributesToForm = true;
		else
		{
			set<string> itemCustomTags = XMLStuff.ItemData->customtags[collectibleID];
			set<string> formCustomTags = XMLStuff.PlayerFormData->customtags[i];
			set<string> sharedCustomTags;
			std::set_intersection(itemCustomTags.begin(), itemCustomTags.end(), formCustomTags.begin(), formCustomTags.end(), inserter(sharedCustomTags, sharedCustomTags.begin()));
			if (sharedCustomTags.size() > 0)
				contributesToForm = true;
		}

		if (contributesToForm)
		{
			this->IncrementPlayerFormCounter(i, 1);
		}
	}
	super(collectibleID);
}

LUA_FUNCTION(Lua_GetPlayerFormByName)
{
	const string name = string(luaL_checkstring(L, 1));

	if (XMLStuff.PlayerFormData->byname.count(name) > 0)
	{
		XMLAttributes ent = XMLStuff.PlayerFormData->GetNodeByName(name);
		if ((ent.end() != ent.begin()) && (ent.count("id") > 0) && (ent["id"].length() > 0)) {
			lua_pushinteger(L, stoi(ent["id"]));
			return 1;
		}
	};
	lua_pushinteger(L, -1);
	return 1;
}

LUA_FUNCTION(Lua_PlayerGetPlayerFormCounter) {
	Entity_Player* player = lua::GetUserdata<Entity_Player*>(L, 1, lua::Metatables::ENTITY_PLAYER, "EntityPlayer");
	int playerFormType = (int)luaL_checkinteger(L, 2);

	if (playerFormType >= 0 && playerFormType <= 14)
	{
		lua_pushinteger(L, player->_playerForms[playerFormType]);
	}
	else {
		return luaL_error(L, "Invalid PlayerForm %d", playerFormType);
	}
	return 1;
}

LUA_FUNCTION(Lua_PlayerIncrementPlayerFormCounter) {
	Entity_Player* player = lua::GetUserdata<Entity_Player*>(L, 1, lua::Metatables::ENTITY_PLAYER, "EntityPlayer");
	int ePlayerForm = (int)luaL_checkinteger(L, 2);
	int num = (int)luaL_checkinteger(L, 3);

	player->IncrementPlayerFormCounter(ePlayerForm, num);
	return 0;
}

HOOK_METHOD(LuaEngine, RegisterClasses, () -> void) {
	super();

	lua::LuaStackProtector protector(_state);

	lua::RegisterGlobalClassFunction(_state, lua::GlobalClasses::Isaac, "GetPlayerFormIdByName", Lua_GetPlayerFormByName);

	lua::RegisterFunction(_state, lua::Metatables::ENTITY_PLAYER, "GetPlayerFormCounter", Lua_PlayerGetPlayerFormCounter);
	lua::RegisterFunction(_state, lua::Metatables::ENTITY_PLAYER, "IncrementPlayerFormCounter", Lua_PlayerIncrementPlayerFormCounter);
}