#include "Body.h"
#include "..\Side\Body.h"
#include "..\..\Enum\Prerequisites.h"
#include "..\..\Misc\Debug.h"

#include <AnimTypeClass.h>
#include <Theater.h>

const DWORD Extension<TechnoTypeClass>::Canary = 0x44444444;
Container<TechnoTypeExt> TechnoTypeExt::ExtMap;

// =============================
// member funcs

void TechnoTypeExt::ExtData::Initialize(TechnoTypeClass *pThis) {
	this->Survivors_PilotChance = (int)(RulesClass::Global()->CrewEscape * 100);
	this->Survivors_PassengerChance = (int)(RulesClass::Global()->CrewEscape * 100);

	this->Survivors_Pilots.SetCapacity(SideClass::Array->Count, NULL);

	for(int i = 0; i < SideClass::Array->Count; ++i) {
		this->Survivors_Pilots[i] = SideExt::ExtMap.Find(SideClass::Array->Items[i])->Crew;
	}

	this->PrerequisiteLists.SetCapacity(0, NULL);
	this->PrerequisiteLists.AddItem(new DynamicVectorClass<int>);

	this->PrerequisiteTheaters = 0xFFFFFFFF;

	this->Secret_RequiredHouses = 0xFFFFFFFF;
	this->Secret_ForbiddenHouses = 0;

	this->Is_Deso = this->Is_Deso_Radiation = !strcmp(pThis->get_ID(), "DESO");
	this->Is_Cow = !strcmp(pThis->get_ID(), "COW");

	this->Parachute_Anim = RulesClass::Global()->Parachute;

	this->_Initialized = is_Inited;
}

/*
EXT_LOAD(TechnoTypeClass)
{
	if(CONTAINS(Ext_p, pThis))
	{
		Create(pThis);

		ULONG out;
		pStm->Read(&Ext_p[pThis], sizeof(ExtData), &out);

		Ext_p[pThis]->Survivors_Pilots.Load(pStm);
		Ext_p[pThis]->Weapons.Load(pStm);
		Ext_p[pThis]->EliteWeapons.Load(pStm);
		for ( int ii = 0; ii < Ext_p[pThis]->Weapons.get_Count(); ++ii )
			SWIZZLE(Ext_p[pThis]->Weapons[ii].WeaponType);
		for ( int ii = 0; ii < Ext_p[pThis]->EliteWeapons.get_Count(); ++ii )
			SWIZZLE(Ext_p[pThis]->EliteWeapons[ii].WeaponType);
		SWIZZLE(Ext_p[pThis]->Insignia_R);
		SWIZZLE(Ext_p[pThis]->Insignia_V);
		SWIZZLE(Ext_p[pThis]->Insignia_E);
	}
}

EXT_SAVE(TechnoTypeClass)
{
	if(CONTAINS(Ext_p, pThis))
	{
		ULONG out;
		pStm->Write(&Ext_p[pThis], sizeof(ExtData), &out);

		Ext_p[pThis]->Survivors_Pilots.Save(pStm);
		Ext_p[pThis]->Weapons.Save(pStm);
		Ext_p[pThis]->EliteWeapons.Save(pStm);
	}
}
*/

void TechnoTypeExt::ExtData::LoadFromINI(TechnoTypeClass *pThis, CCINIClass *pINI)
{
	const char * section = pThis->get_ID();
//	TechnoTypeExt::ExtData *pData = TechnoTypeExt::ExtMap.Find(pThis);

	if(!pINI->GetSection(section)) {
		return;
	}

	if(this->_Initialized == is_Constanted && RulesClass::Initialized) {
		this->InitializeRuled(pThis);
	}

	if(this->_Initialized == is_Ruled) {
		this->Initialize(pThis);
	}

	if(this->_Initialized != is_Inited) {
		return;
	}

	// survivors
	this->Survivors_Pilots.SetCapacity(SideClass::Array->Count, NULL);

	this->Survivors_PilotChance = pINI->ReadInteger(section, "Survivor.PilotChance", this->Survivors_PilotChance);
	this->Survivors_PassengerChance = pINI->ReadInteger(section, "Survivor.PassengerChance", this->Survivors_PassengerChance);

	char *buffer = Ares::readBuffer;
	char flag[256];
	for(int i = 0; i < SideClass::Array->Count; ++i) {
		_snprintf(flag, 256, "Survivor.Side%d", i);
		PARSE_INFANTRY(flag, this->Survivors_Pilots[i]);
	}

	// prereqs
	int PrereqListLen = pINI->ReadInteger(section, "Prerequisite.Lists", this->PrerequisiteLists.Count - 1);

	if(PrereqListLen < 1) {
		PrereqListLen = 0;
	}
	++PrereqListLen;
	while(PrereqListLen > this->PrerequisiteLists.Count) {
		this->PrerequisiteLists.AddItem(new DynamicVectorClass<int>);
	}

	DynamicVectorClass<int> *dvc = this->PrerequisiteLists.GetItem(0);
	if(pINI->ReadString(section, "Prerequisite", "", buffer, BUFLEN)) {
		Prereqs::Parse(buffer, dvc);
	}
	for(int i = 0; i < this->PrerequisiteLists.Count; ++i) {
		_snprintf(flag, 256, "Prerequisite.List%d", i);
		if(pINI->ReadString(section, flag, "", buffer, BUFLEN)) {
			dvc = this->PrerequisiteLists.GetItem(i);
			Prereqs::Parse(buffer, dvc);
		}
	}

	dvc = &this->PrerequisiteNegatives;
	if(pINI->ReadString(section, "Prerequisite.Negative", "", buffer, BUFLEN)) {
		Prereqs::Parse(buffer, dvc);
	}

	if(pINI->ReadString(section, "Prerequisite.RequiredTheaters", "", buffer, BUFLEN)) {
		this->PrerequisiteTheaters = 0;
		for(char *cur = strtok(buffer, ","); cur; cur = strtok(NULL, ",")) {
			signed int idx = Theater::FindIndex(cur);
			if(idx > -1) {
				this->PrerequisiteTheaters |= (1 << idx);
			}
		}
	}

	// new secret lab
	this->Secret_RequiredHouses
		= pINI->ReadHouseTypesList(section, "SecretLab.RequiredHouses", this->Secret_RequiredHouses);

	this->Secret_ForbiddenHouses
		= pINI->ReadHouseTypesList(section, "SecretLab.ForbiddenHouses", this->Secret_ForbiddenHouses);

	this->Is_Deso = pINI->ReadBool(section, "IsDesolator", this->Is_Deso);
	this->Is_Deso_Radiation = pINI->ReadBool(section, "IsDesolator.RadDependant", this->Is_Deso_Radiation);
	this->Is_Cow  = pINI->ReadBool(section, "IsCow", this->Is_Cow);

	this->Is_Spotlighted = pINI->ReadBool(section, "HasSpotlight", this->Is_Spotlighted);
	this->Spot_Height = pINI->ReadInteger(section, "Spotlight.StartHeight", this->Spot_Height);
	this->Spot_Distance = pINI->ReadInteger(section, "Spotlight.Distance", this->Spot_Distance);
	if(pINI->ReadString(section, "Spotlight.AttachedTo", "", buffer, 256)) {
		if(!_strcmpi(buffer, "body")) {
			this->Spot_AttachedTo = sa_Body;
		} else if(!_strcmpi(buffer, "turret")) {
			this->Spot_AttachedTo = sa_Turret;
		} else if(!_strcmpi(buffer, "barrel")) {
			this->Spot_AttachedTo = sa_Barrel;
		}
	}
	this->Spot_DisableR = pINI->ReadBool(section, "Spotlight.DisableRed", this->Spot_DisableR);
	this->Spot_DisableG = pINI->ReadBool(section, "Spotlight.DisableGreen", this->Spot_DisableG);
	this->Spot_DisableB = pINI->ReadBool(section, "Spotlight.DisableBlue", this->Spot_DisableB);
	this->Spot_Reverse = pINI->ReadBool(section, "Spotlight.IsInverted", this->Spot_Reverse);

	// insignia
	SHPStruct *image = NULL;
	if(pINI->ReadString(section, "Insignia.Rookie", "", buffer, 256)) {
		_snprintf(flag, 256, "%s.shp", buffer);
		image = FileSystem::LoadSHPFile(flag);
		if(image) {
			this->Insignia_R = image;
		} else {
		}
	}
	if(pINI->ReadString(section, "Insignia.Veteran", "", buffer, 256)) {
		_snprintf(flag, 256, "%s.shp", buffer);
		image = FileSystem::LoadSHPFile(flag);
		if(image) {
			this->Insignia_V = image;
		}
	}
	if(pINI->ReadString(section, "Insignia.Elite", "", buffer, 256)) {
		_snprintf(flag, 256, "%s.shp", buffer);
		image = FileSystem::LoadSHPFile(flag);
		if(image) {
			this->Insignia_E = image;
		}
	}

	PARSE_ANIM("Parachute.Anim", this->Parachute_Anim);

	// quick fix - remove after the rest of weapon selector code is done
	return;
}

/*
	// weapons
	int WeaponCount = pINI->ReadInteger(section, "WeaponCount", pData->Weapons.get_Count());

	if(WeaponCount < 2)
	{
		WeaponCount = 2;
	}

	while(WeaponCount < pData->Weapons.get_Count())
	{
		pData->Weapons.RemoveItem(pData->Weapons.get_Count() - 1);
	}
	if(WeaponCount > pData->Weapons.get_Count())
	{
		pData->Weapons.SetCapacity(WeaponCount, NULL);
		pData->Weapons.set_Count(WeaponCount);
	}

	while(WeaponCount < pData->EliteWeapons.get_Count())
	{
		pData->EliteWeapons.RemoveItem(pData->EliteWeapons.get_Count() - 1);
	}
	if(WeaponCount > pData->EliteWeapons.get_Count())
	{
		pData->EliteWeapons.SetCapacity(WeaponCount, NULL);
		pData->EliteWeapons.set_Count(WeaponCount);
	}

	WeaponStruct *W = &pData->Weapons[0];
	ReadWeapon(W, "Primary", section, pINI);

	W = &pData->EliteWeapons[0];
	ReadWeapon(W, "ElitePrimary", section, pINI);

	W = &pData->Weapons[1];
	ReadWeapon(W, "Secondary", section, pINI);

	W = &pData->EliteWeapons[1];
	ReadWeapon(W, "EliteSecondary", section, pINI);

	for(int i = 0; i < WeaponCount; ++i)
	{
		W = &pData->Weapons[i];
		_snprintf(flag, 256, "Weapon%d", i);
		ReadWeapon(W, flag, section, pINI);

		W = &pData->EliteWeapons[i];
		_snprintf(flag, 256, "EliteWeapon%d", i);
		ReadWeapon(W, flag, section, pINI);
	}

void TechnoTypeClassExt::ReadWeapon(WeaponStruct *pWeapon, const char *prefix, const char *section, CCINIClass *pINI)
{
	char buffer[256];
	char flag[64];

	pINI->ReadString(section, prefix, "", buffer, 0x100);

	if(strlen(buffer))
	{
		pWeapon->WeaponType = WeaponTypeClass::FindOrAllocate(buffer);
	}

	CCINIClass *pArtINI = CCINIClass::INI_Art;

	CoordStruct FLH;
	// (Elite?)(Primary|Secondary)FireFLH - FIRE suffix
	// (Elite?)(Weapon%d)FLH - no suffix
	if(prefix[0] == 'W' || prefix[5] == 'W') // W EliteW
	{
		_snprintf(flag, 64, "%sFLH", prefix);
	}
	else
	{
		_snprintf(flag, 64, "%sFireFLH", prefix);
	}
	pArtINI->Read3Integers((int *)&FLH, section, flag, (int *)&pWeapon->FLH);
	pWeapon->FLH = FLH;

	_snprintf(flag, 64, "%sBarrelLength", prefix);
	pWeapon->BarrelLength = pArtINI->ReadInteger(section, flag, pWeapon->BarrelLength);
	_snprintf(flag, 64, "%sBarrelThickness", prefix);
	pWeapon->BarrelThickness = pArtINI->ReadInteger(section, flag, pWeapon->BarrelThickness);
	_snprintf(flag, 64, "%sTurretLocked", prefix);
	pWeapon->TurretLocked = pArtINI->ReadBool(section, flag, pWeapon->TurretLocked);
}
*/

void TechnoTypeExt::PointerGotInvalid(void *ptr) {

}

// =============================
// container hooks

DEFINE_HOOK(711835, TechnoTypeClass_CTOR, 5)
{
	GET(TechnoTypeClass*, pItem, ESI);
	TechnoTypeExt::ExtMap.FindOrAllocate(pItem);
	return 0;
}

DEFINE_HOOK(711AE0, TechnoTypeClass_DTOR, 5)
{
	GET(TechnoTypeClass*, pItem, ECX);

	TechnoTypeExt::ExtMap.Remove(pItem);
	return 0;
}

DEFINE_HOOK(716DAC, TechnoTypeClass_Load, A)
{
	GET_STACK(TechnoTypeClass*, pItem, 0x224);
	GET_STACK(IStream*, pStm, 0x228);

	TechnoTypeExt::ExtMap.Load(pItem, pStm);
	return 0;
}

DEFINE_HOOK(717094, TechnoTypeClass_Save, 5)
{
	GET_STACK(TechnoTypeClass*, pItem, 0xC);
	GET_STACK(IStream*, pStm, 0x10);

	TechnoTypeExt::ExtMap.Save(pItem, pStm);
	return 0;
}

DEFINE_HOOK(716123, TechnoTypeClass_LoadFromINI, 5)
DEFINE_HOOK_AGAIN(716132, TechnoTypeClass_LoadFromINI, 5)
{
	GET(TechnoTypeClass*, pItem, EBP);
	GET_STACK(CCINIClass*, pINI, 0x380);

	TechnoTypeExt::ExtMap.LoadFromINI(pItem, pINI);
	return 0;
}
