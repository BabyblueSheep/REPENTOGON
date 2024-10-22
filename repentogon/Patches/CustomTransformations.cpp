#include "SigScan.h"
#include "IsaacRepentance.h"
#include "LuaCore.h"
#include "HookSystem.h"

#include "Windows.h"
#include <string>
#include <dwmapi.h>
#include "EntityPlus.h"
#include "XMLData.h"
#include "CustomCache.h"

// While there's only 14 transformations in the game, players store up to 15 transformations, likely due to there being a scrapped transformation.
// This is why I start checking custom transformation IDs at 15 and not 14.



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

bool CollectibleAndPlayerFormShareTags(unsigned int collectibleID, unsigned int playerForm)
{
	return (g_Manager->GetItemConfig()->GetCollectible(collectibleID)->tags & XMLStuff.PlayerFormData->tags[playerForm]) > 0;
}

bool CollectibleContributesToForm(unsigned int collectibleID, unsigned int playerForm)
{
	return CollectibleAndPlayerFormShareTags(collectibleID, playerForm) || CollectibleAndPlayerFormShareCustomTags(collectibleID, playerForm);
}

HOOK_METHOD(Entity_Player, IncrementPlayerFormCounter, (int ePlayerForm, int num)->void)
{
	if (ePlayerForm < 15)
	{
		super(ePlayerForm, num);

		TriggerCustomCache(this, XMLStuff.PlayerFormData->customcache[ePlayerForm], false);
		return;
	}
	if (XMLStuff.PlayerFormData->nodes.find(ePlayerForm) == XMLStuff.PlayerFormData->nodes.end())
		return;

	XMLAttributes attributes = XMLStuff.PlayerFormData->GetNodeById(ePlayerForm);
	EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(this);
	int prev = playerPlus->customPlayerForms[ePlayerForm - 15];
	playerPlus->customPlayerForms[ePlayerForm - 15] += num;
	if (playerPlus->customPlayerForms[ePlayerForm - 15] < 0)
		playerPlus->customPlayerForms[ePlayerForm - 15] = 0;
	if (playerPlus->customPlayerForms[ePlayerForm - 15] == 3 && playerPlus->customPlayerForms[ePlayerForm - 15] > prev)
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

		this->AddCacheFlags(XMLStuff.PlayerFormData->cache[ePlayerForm]);
		this->EvaluateItems();
	}
	else if (prev == 3 && playerPlus->customPlayerForms[ePlayerForm - 15] < prev)
	{
		this->TryRemoveNullCostume(atoi(attributes["costume"].c_str()));

		this->AddCacheFlags(XMLStuff.PlayerFormData->cache[ePlayerForm]);
		this->EvaluateItems();
	}
	TriggerCustomCache(this, XMLStuff.PlayerFormData->customcache[ePlayerForm], false);
}

HOOK_METHOD(Entity_Player, AddCollectible, (int type, int charge, bool firsttime, int slot, int vardata)->void)
{
	if (!firsttime) { // Fun Isaac fact: picking up an item doesn't add transformation progress after the first time, BUT dropping an item always removes transformation progress, letting stuff like Butter! bleed transformation progress.
		super(type, charge, firsttime, slot, vardata);
		return;
	}

	for (int i = 15; i <= XMLStuff.PlayerFormData->maxid; i++) {
		if (CollectibleContributesToForm(type, i))
		{
			this->IncrementPlayerFormCounter(i, 1);
		}
	}
	super(type, charge, firsttime, slot, vardata);
	for (int i = 0; i < 15; i++)
	{
		if (!CollectibleAndPlayerFormShareTags(type, i) && CollectibleAndPlayerFormShareCustomTags(type, i))
			this->IncrementPlayerFormCounter(i, 1);
	}
}

HOOK_METHOD(Entity_Player, RemoveCollectible, (unsigned int CollectibleType, bool IgnoreModifiers, unsigned int ActiveSlot, bool RemoveFromPlayerForm)->void)
{
	if (!RemoveFromPlayerForm ||
		g_Manager->GetItemConfig()->GetCollectible(CollectibleType)->type == 3 || // active item
		!this->HasCollectible(CollectibleType, true))
	{
		super(CollectibleType, IgnoreModifiers, ActiveSlot, RemoveFromPlayerForm);
		return;
	}

	for (int i = 15; i <= XMLStuff.PlayerFormData->maxid; i++) {
		if (CollectibleContributesToForm(CollectibleType, i))
		{
			this->IncrementPlayerFormCounter(i, -1);
		}
	}
	super(CollectibleType, IgnoreModifiers, ActiveSlot, RemoveFromPlayerForm);
	for (int i = 0; i < 15; i++)
	{
		if (!CollectibleAndPlayerFormShareTags(CollectibleType, i) && CollectibleAndPlayerFormShareCustomTags(CollectibleType, i))
			this->IncrementPlayerFormCounter(i, -1);
	}
}

HOOK_METHOD(Entity_Player, RerollAllCollectibles, (RNG* rng, bool includeActives)->void)
{
	std::vector<int> oldCollectibleList = this->GetCollectiblesList();
	super(rng, includeActives);
	std::vector<int> newCollectibleList = this->GetCollectiblesList();

	for (int i = 1; i < oldCollectibleList.size(); i++)
	{
		if (g_Manager->GetItemConfig()->GetCollectible(i) == NULL)
			continue;
		if (g_Manager->GetItemConfig()->GetCollectible(i)->type == 3) // active item
				continue;
		if (newCollectibleList[i] - oldCollectibleList[i] < 0)
		{
			for (int j = 15; j <= XMLStuff.PlayerFormData->maxid; j++) {
				if (CollectibleContributesToForm(i, j))
				{
					this->IncrementPlayerFormCounter(j, -1); // Fun Isaac fact: rerolling items will remove transformation progress, but only once per unique items, so if you have 4 Bob's Brains and use D4, then you will keep Bob and have your transformation progress be equal to 3.
				}
			}
			for (int j = 0; j < 15; j++)
			{
				if (!CollectibleAndPlayerFormShareTags(i, j) && CollectibleAndPlayerFormShareCustomTags(i, j))
					this->IncrementPlayerFormCounter(j, -1);
			}
		}
	}
}

/*
As nice as it would be to let trinkets automatically count for transformations, this *would* break compatibility with vanilla, since it'd make several vanilla trinkets count for transformations, i.e. Bob's Bladder for Bob.
I'll need to think about this in the future.
HOOK_METHOD(Entity_Player, TriggerTrinketAdded, (unsigned int trinketID, bool firstTimePickingUp)->void)
{
	super(trinketID, firstTimePickingUp);

	for (int i = 15; i <= XMLStuff.PlayerFormData->maxid; i++) {
		if (CollectibleContributesToForm(trinketID, i))
		{
			this->IncrementPlayerFormCounter(i, 1);
		}
	}
	for (int i = 0; i < 15; i++)
	{
		if (!CollectibleAndPlayerFormShareTags(trinketID, i) && CollectibleAndPlayerFormShareCustomTags(trinketID, i))
			this->IncrementPlayerFormCounter(i, 1);
	}
}

HOOK_METHOD(Entity_Player, TriggerTrinketRemoved, (unsigned int trinketID)->void)
{
	super(trinketID);

	for (int i = 15; i <= XMLStuff.PlayerFormData->maxid; i++) {
		if (CollectibleContributesToForm(trinketID, i))
		{
			this->IncrementPlayerFormCounter(i, -1);
		}
	}
	for (int i = 0; i < 15; i++)
	{
		if (CollectibleContributesToForm(trinketID, i))
			this->IncrementPlayerFormCounter(i, -1);
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

	if (playerFormType >= 0 && playerFormType <= 14)
	{
		lua_pushinteger(L, player->_playerForms[playerFormType]);
	}
	else if (playerFormType > 14)
	{
		EntityPlayerPlus* playerPlus = GetEntityPlayerPlus(player);
		lua_pushinteger(L, playerPlus->customPlayerForms[playerFormType - 15]);
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

HOOK_METHOD(ModManager, LoadConfigs, ()->void)
{
	super();

	// special exception for 14th loaded playerform since it's treated as a vanilla playerform yet isn't loaded in vanilla, so game assumes first modded playerform is a vanilla one
	// fml
	g_Manager->GetItemConfig()->GetPlayerForms()->at(14)->costume = atoi(XMLStuff.PlayerFormData->GetNodeById(14)["costume"].c_str());
}