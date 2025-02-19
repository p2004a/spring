/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "TooltipConsole.h"
#include "MouseHandler.h"
#include "Game/GlobalUnsynced.h"
#include "Game/Players/PlayerHandler.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Map/MetalMap.h"
#include "Map/ReadMap.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Fonts/glFont.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureDef.h"
#include "Sim/Misc/ResourceHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Misc/Wind.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitToolTipMap.hpp"
#include "System/EventHandler.h"
#include "System/Config/ConfigHandler.h"
#include "System/StringUtil.h"

#include "System/Misc/TracyDefs.h"
#include <format>


CONFIG(std::string, TooltipGeometry).defaultValue("0.0 0.125 0.41 0.1");
CONFIG(bool, TooltipOutlineFont).defaultValue(true).headlessValue(false);

CTooltipConsole* tooltip = NULL;


CTooltipConsole::CTooltipConsole()
	: enabled(true)
{
	const std::string geo = configHandler->GetString("TooltipGeometry");
	const int vars = sscanf(geo.c_str(), "%f %f %f %f", &x, &y, &w, &h);
	if (vars != 4) {
		x = 0.00f;
		y = 0.00f;
		w = 0.41f;
		h = 0.10f;
	}

	outFont = configHandler->GetBool("TooltipOutlineFont");
}


CTooltipConsole::~CTooltipConsole()
{
}


void CTooltipConsole::Draw()
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!enabled) {
		return;
	}

	const std::string& s = mouse->GetCurrentTooltip();

	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (!outFont) {
		glColor4f(0.2f, 0.2f, 0.2f, CInputReceiver::guiAlpha);
		glRectf(x, y, (x + w), (y + h));
	}

	const float fontSize   = (h * globalRendering->viewSizeY) * (smallFont->GetLineHeight() / 5.75f);

	float curX = x + 0.01f;
	float curY = y + h - 0.5f * fontSize * smallFont->GetLineHeight() / globalRendering->viewSizeY;
	glColor4f(1.0f, 1.0f, 1.0f, 0.8f);

	smallFont->Begin();
	smallFont->SetColors(); //default

	if (outFont) {
		smallFont->glPrint(curX, curY, fontSize, FONT_ASCENDER | FONT_OUTLINE | FONT_NORM, s);
	} else {
		smallFont->glPrint(curX, curY, fontSize, FONT_ASCENDER | FONT_NORM, s);
	}

	smallFont->End();
}


bool CTooltipConsole::IsAbove(int x, int y)
{
	RECOIL_DETAILED_TRACY_ZONE;
	if (!enabled) {
		return false;
	}

	const float mx = MouseX(x);
	const float my = MouseY(y);

	return ((mx > (x + 0.01f)) && (mx < (x + w)) &&
	        (my > (y + 0.01f)) && (my < (y + h)));
}


/******************************************************************************/

#define RED       "\xff\xff\x50\x01"
#define BLUE      "\xff\xd3\xdb\xff"
#define GREEN     "\xff\x50\xff\x50"
#define GREY      "\xff\x90\x90\x90"
#define DARKBLUE  "\xff\xc0\xc0\xff"


static void GetDecoyResources(const CUnit* unit,
                              SResourcePack& make,
                              SResourcePack& use)
{
	make = use = 0.0f;

	const UnitDef* rd = unit->unitDef;;
	const UnitDef* ud = rd->decoyDef;
	if (ud == nullptr)
		return;

	make += ud->resourceMake;
	make.energy += (ud->tidalGenerator * envResHandler.GetCurrentTidalStrength() * (ud->tidalGenerator > 0.0f));

	bool active = ud->activateWhenBuilt;
	if (rd->onoffable && ud->onoffable) {
		active = unit->activated;
	}

	if (active) {
		make.metal += ud->makesMetal;
		if (ud->extractsMetal > 0.0f) {
			if (rd->extractsMetal > 0.0f) {
				make.metal += unit->metalExtract * (ud->extractsMetal / rd->extractsMetal);
			}
		}
		use += ud->upkeep;

		if (ud->windGenerator > 0.0f) {
			if (envResHandler.GetCurrentWindStrength() > ud->windGenerator) {
				make.energy += ud->windGenerator;
			} else {
				make.energy += envResHandler.GetCurrentWindStrength();
			}
		}
	}
}


std::string CTooltipConsole::MakeUnitString(const CUnit* unit)
{
	RECOIL_DETAILED_TRACY_ZONE;
	string custom = eventHandler.WorldTooltip(unit, nullptr, nullptr);
	if (!custom.empty())
		return custom;

	std::string s;
	s.reserve(512);

	const bool enemyUnit = (teamHandler.AllyTeam(unit->team) != gu->myAllyTeam) && !gu->spectatingFullView;

	const UnitDef* unitDef = unit->unitDef;
	const UnitDef* decoyDef = enemyUnit ? unitDef->decoyDef : nullptr;
	const UnitDef* effectiveDef = !enemyUnit ? unitDef : (decoyDef ? decoyDef : unitDef);
	const CTeam* team = teamHandler.Team(unit->team);

	// don't show the unit type if it is not known
	const unsigned short losStatus = unit->losStatus[gu->myAllyTeam];
	const unsigned short prevMask = (LOS_PREVLOS | LOS_CONTRADAR);
	if (enemyUnit &&
	    !(losStatus & LOS_INLOS) &&
	    ((losStatus & prevMask) != prevMask)) {
		return "Enemy unit";
	}

	// show the player name instead of unit name if it has FBI tag showPlayerName
	if (effectiveDef->showPlayerName) {
		s = team->GetControllerName();
	} else {
		s = unitToolTipMap.Get(unit->id);

		if (decoyDef) {
			s = decoyDef->humanName + " - " + decoyDef->tooltip;
		}
	}

	// don't show the unit health and other info if it has
	// the FBI tag hideDamage and is not on our ally team or
	// is not in LOS
	if (!enemyUnit || (!effectiveDef->hideDamage && (losStatus & LOS_INLOS))) {
		SUnitStats stats;
		stats.AddUnit(unit, enemyUnit);
		s += MakeUnitStatsString(stats);
	}

	s += "\n";
	s += team->GetControllerName();
	return s;
}


std::string CTooltipConsole::MakeUnitStatsString(const SUnitStats& stats)
{
	RECOIL_DETAILED_TRACY_ZONE;
	string s;
	s.reserve(512);

	s += std::format
		( "\nHealth {:.0f}/{:.0f}"
		  "\nExperience {:.2f} Cost {:.0f} Range {:.0f}"
		, stats.health, stats.maxHealth
		, stats.experience, stats.cost, stats.maxRange
	);

	for (int i = 0; i < SResourcePack::MAX_RESOURCES; ++i) {
		s += std::format("\n" BLUE "{}: " GREEN "+{:.1f}" GREY "/" RED "-{:.1f}"
			, resourceHandler->GetResource(i)->name, stats.resourceMake[i], stats.resourceUse[i]
		);
		if (stats.resourceHarvestMax[i] > 0.0f) {
			s += std::format(GREY " (" GREEN "{:.1f}" GREY "/" BLUE "{:.1f}" GREY ")"
				, stats.resourceHarvest[i], stats.resourceHarvestMax[i]
			);
		}
	}
	s += '\x08';
	return s;
}


std::string CTooltipConsole::MakeFeatureString(const CFeature* feature)
{
	RECOIL_DETAILED_TRACY_ZONE;
	string custom = eventHandler.WorldTooltip(NULL, feature, NULL);
	if (!custom.empty()) {
		return custom;
	}

	std::string s = feature->def->description;
	if (s.empty()) {
		s = "Feature";
	}

	const float remainingMetal  = feature->resources.metal;
	const float remainingEnergy = feature->resources.energy;

	const char* metalColor  = (remainingMetal  > 0) ? GREEN : RED;
	const char* energyColor = (remainingEnergy > 0) ? GREEN : RED;

	char tmp[512];
	sprintf(tmp,"\n" BLUE "Metal: %s%.0f  " BLUE "Energy: %s%.0f\x08",
		metalColor,  remainingMetal,
		energyColor, remainingEnergy);

	s += tmp;

	return s;
}


std::string CTooltipConsole::MakeGroundString(const float3& pos)
{
	RECOIL_DETAILED_TRACY_ZONE;
	string custom = eventHandler.WorldTooltip(NULL, NULL, &pos);
	if (!custom.empty()) {
		return custom;
	}

	const int px = pos.x / 16;
	const int pz = pos.z / 16;
	const int typeMapIdx = std::clamp(pz * mapDims.hmapx + px, 0, mapDims.hmapx * mapDims.hmapy - 1);
	const unsigned char* typeMap = readMap->GetTypeMapSynced();
	const CMapInfo::TerrainType* tt = &mapInfo->terrainTypes[typeMap[typeMapIdx]];

	char tmp[512];
	sprintf(tmp,
		"Pos %.0f %.0f Elevation %.0f\n"
		"Terrain type: %s\n"
		"Speeds T/K/H/S %.2f %.2f %.2f %.2f\n"
		"Hardness %.0f Metal %.1f",
		pos.x, pos.z, pos.y, tt->name.c_str(),
		tt->tankSpeed, tt->kbotSpeed, tt->hoverSpeed, tt->shipSpeed,
		tt->hardness * mapDamage->mapHardness,
		metalMap.GetMetalAmount(px, pz)
	);
	return tmp;
}


/***********************************************************************/
/***********************************************************************/

SUnitStats::SUnitStats()
: health(0.0f)
, maxHealth(0.0f)
, experience(0.0f)
, cost(0.0f)
, maxRange(0.0f)
, resourceMake(0.0f)
, resourceUse(0.0f)
, resourceHarvest(0.0f)
, resourceHarvestMax(0.0f)
, count(0)
{}


void SUnitStats::AddUnit(const CUnit* unit, bool enemy)
{
	RECOIL_DETAILED_TRACY_ZONE;
	const UnitDef* decoyDef = enemy ? unit->unitDef->decoyDef : nullptr;

	++count;

	if (!decoyDef) {
		health           += unit->health;
		maxHealth        += unit->maxHealth;
		experience        = (experience * (count - 1) + unit->experience) / count; // average xp
		cost             += unit->cost.metal + (unit->cost.energy / 60.0f);
		maxRange          = std::max(maxRange, unit->maxRange);
		resourceMake     += unit->resourcesMake;
		resourceUse      += unit->resourcesUse;
		resourceHarvest  += unit->harvested;
		resourceHarvestMax += unit->harvestStorage;
	} else {
		// display adjusted decoy stats
		const float healthScale = (decoyDef->health / unit->unitDef->health);

		SResourcePack make_, use_;
		GetDecoyResources(unit, make_, use_);

		health           += unit->health * healthScale;
		maxHealth        += unit->maxHealth * healthScale;
		experience        = (experience * (count - 1) + unit->experience) / count;
		cost             += decoyDef->cost.metal + (decoyDef->cost.energy / 60.0f);
		maxRange          = std::max(maxRange, decoyDef->maxWeaponRange);
		resourceMake     += make_;
		resourceUse      += use_;
		//resourceHarvest    += unit->harvested;
		//resourceHarvestMax += unit->harvestStorage;
	}
}
