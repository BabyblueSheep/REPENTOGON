#pragma once

int __stdcall GetMaxCoins();
int __stdcall GetMaxKeys();
int __stdcall GetMaxBombs();

void TriggerCustomCache(Entity_Player* player, const std::set<std::string>& customcaches, const bool immediate);

void ASMPatchesForCustomCache();
