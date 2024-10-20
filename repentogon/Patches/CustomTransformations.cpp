#include "SigScan.h"
#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "HookSystem.h"

#include "Windows.h"
#include <string>
#include <dwmapi.h>
#include "EntityPlus.h"
#include "XMLData.h"

bool CollectibleAndPlayerFormShareCustomTags(unsigned int collectibleID, unsigned int playerForm)
{
	set<string> itemCustomTags = XMLStuff.ItemData->customtags[collectibleID];
	set<string> formCustomTags = XMLStuff.PlayerFormData->customtags[playerForm];
	set<string> sharedCustomTags;
	std::set_intersection(itemCustomTags.begin(), itemCustomTags.end(), formCustomTags.begin(), formCustomTags.end(), inserter(sharedCustomTags, sharedCustomTags.begin()));
	if (sharedCustomTags.size() > 0)
		return true;
	return false;
}

bool CollectibleContributesToForm(unsigned int collectibleID, unsigned int playerForm)
{
	if ((g_Manager->GetItemConfig()->GetCollectible(collectibleID)->tags & XMLStuff.PlayerFormData->tags[playerForm]) > 0)
		return true;
	return CollectibleAndPlayerFormShareCustomTags(collectibleID, playerForm);
}

HOOK_METHOD(Entity_Player, IncrementPlayerFormCounter, (int ePlayerForm, int num)->void)
{
	if (ePlayerForm < 14)
	{
		super(ePlayerForm, num);
		return;
	}
	if (XMLStuff.PlayerFormData->nodes.find(ePlayerForm) == XMLStuff.PlayerFormData->nodes.end())
		return;

	XMLAttributes attributes = XMLStuff.PlayerFormData->GetNodeById(ePlayerForm);
	EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(this);
	int prev = playerPlus->customPlayerForms[ePlayerForm - 14];
	playerPlus->customPlayerForms[ePlayerForm - 14] += num;
	if (playerPlus->customPlayerForms[ePlayerForm - 14] < 0)
		playerPlus->customPlayerForms[ePlayerForm - 14] = 0;
	if (playerPlus->customPlayerForms[ePlayerForm - 14] == 3 && playerPlus->customPlayerForms[ePlayerForm - 14] > prev)
	{
		g_Manager->_sfxManager.Play(132, 1.0, 0, false, 1, 0);

		wchar_t* title = new wchar_t[attributes["name"].length() + 1];
		std::copy(attributes["name"].begin(), attributes["name"].end(), title);
		title[attributes["name"].length()] = 0;
		g_Game->GetHUD()->ShowItemTextCustom(title, L"", false, false);
		delete[] title;

		g_Game->Spawn(1000, 15, *(this->GetPosition()), Vector(), this, 0, 0, 0);

		if (attributes.find("costume") != attributes.end())
		{
			this->AddNullCostume(atoi(attributes["costume"].c_str()));
		}
	}
	else if (prev == 3 && playerPlus->customPlayerForms[ePlayerForm - 14] < prev)
	{
		this->TryRemoveNullCostume(atoi(attributes["costume"].c_str()));
	}
}

HOOK_METHOD(Entity_Player, AddCollectible, (int type, int charge, bool firsttime, int slot, int vardata)->void)
{
	int previousVanillaPlayerForms[15];
	if (firsttime)
	{
		for (int i = 0; i < 14; i++)
		{
			previousVanillaPlayerForms[i] = this->_playerForms[i];
		}
		for (int i = 14; i <= XMLStuff.PlayerFormData->maxid; i++) {
			if (CollectibleContributesToForm(type, i))
			{
				this->IncrementPlayerFormCounter(i, 1);
			}
		}
	}
	super(type, charge, firsttime, slot, vardata);
	if (firsttime)
	{
		for (int i = 0; i < 14; i++)
		{
			if (previousVanillaPlayerForms[i] == this->_playerForms[i])
			{
				if (CollectibleAndPlayerFormShareCustomTags(type, i))
					this->IncrementPlayerFormCounter(i, 1);
			}
		}
	}
}

HOOK_METHOD(Entity_Player, RemoveCollectible, (unsigned int CollectibleType, bool IgnoreModifiers, unsigned int ActiveSlot, bool RemoveFromPlayerForm)->void)
{
	if (
		!RemoveFromPlayerForm ||
		g_Manager->GetItemConfig()->GetCollectible(CollectibleType)->type == 3 ||
		!this->HasCollectible(CollectibleType, true)
	)
	{
		super(CollectibleType, IgnoreModifiers, ActiveSlot, RemoveFromPlayerForm);
		return;
	}
	int previousVanillaPlayerForms[15];
	for (int i = 0; i < 14; i++)
	{
		previousVanillaPlayerForms[i] = this->_playerForms[i];
	}
	for (int i = 14; i <= XMLStuff.PlayerFormData->maxid; i++) {
		if (CollectibleContributesToForm(CollectibleType, i))
		{
			this->IncrementPlayerFormCounter(i, -1);
		}
	}
	super(CollectibleType, IgnoreModifiers, ActiveSlot, RemoveFromPlayerForm);
	for (int i = 0; i < 14; i++)
	{
		if (previousVanillaPlayerForms[i] == this->_playerForms[i])
		{
			if (CollectibleAndPlayerFormShareCustomTags(CollectibleType, i))
				this->IncrementPlayerFormCounter(i, 1);
		}
	}
}

/*HOOK_METHOD(Entity_Player, RerollAllCollectibles, (RNG* rng, bool includeActives)->void)
{

	super(rng, includeActives);
}

HOOK_METHOD(Entity_Player, TriggerCollectibleRemoved, (unsigned int collectibleID)->void)
{
	if (g_Manager->GetItemConfig()->GetCollectible(collectibleID)->type == 3)
	{
		super(collectibleID);
		return;
	}
	int previousVanillaPlayerForms[15];
	for (int i = 0; i < 14; i++)
	{
		previousVanillaPlayerForms[i] = this->_playerForms[i];
	}
	for (int i = 14; i <= XMLStuff.PlayerFormData->maxid; i++) {
		if (CollectibleContributesToForm(collectibleID, i))
		{
			this->IncrementPlayerFormCounter(i, -1);
		}
	}
	super(collectibleID);
	for (int i = 0; i < 14; i++)
	{
		if (previousVanillaPlayerForms[i] == this->_playerForms[i])
		{
			if (CollectibleAndPlayerFormShareCustomTags(collectibleID, i))
				this->IncrementPlayerFormCounter(i, 1);
		}
	}
}*/

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

	if (playerFormType >= 0 && playerFormType <= 13)
	{
		lua_pushinteger(L, player->_playerForms[playerFormType]);
	}
	else if (playerFormType > 13)
	{
		EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(player);
		int prev = playerPlus->customPlayerForms[playerFormType - 14];
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