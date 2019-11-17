#include "script.h"

#include "menu.h"

#include "inc/natives.h"

#include "fmt/format.h"
#include "Memory/HandlingInfo.h"
#include "Memory/VehicleExtensions.hpp"
#include "ScriptUtils.h"

extern VehicleExtensions g_ext;

float g_stepSz = 0.005f;

namespace {
NativeMenu::Menu menu;
}

namespace Offsets {
    extern uint64_t Handling;
}

NativeMenu::Menu& GetMenu() {
    return menu;
}

void UpdateMainMenu() {
    menu.Title("Handling Editor");
    menu.Subtitle("v2.0.0");

    menu.MenuOption("Edit current handling", "EditMenu");

    menu.FloatOption("Step size", g_stepSz, 0.0001f, 1.0f, 0.00005f);

    Ped playerPed = PLAYER::PLAYER_PED_ID();
    Vehicle vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

    if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        if (menu.Option("Respawn vehicle")) {
            Utils::RespawnVehicle(vehicle, playerPed);
        }
    }

    // TODO:
    // 1. Save current as .meta/.xml
    // 2. Load .meta/.xml to current vehicle
    // Pipe dream: Multiple meta/xml editing
}

template <typename T>
bool IsNear(T a, T b, T x) {
    return abs(a-b) <= x;
}

void UpdateEditMenu() {
    menu.Title("Edit Handling");

    Ped playerPed = PLAYER::GET_PLAYER_PED(PLAYER::GET_PLAYER_INDEX());
    Vehicle vehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

    if (!ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        menu.Subtitle("Not in a vehicle");
        menu.Option("Not in a vehicle");
        return;
    }

    std::string vehicleNameLabel = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(vehicle));
    std::string vehicleName = UI::_GET_LABEL_TEXT(vehicleNameLabel.c_str());
    menu.Subtitle(fmt::format("{}", vehicleName));

    RTH::CHandlingData* currentHandling = nullptr;

    uint64_t addr = g_ext.GetHandlingPtr(vehicle);
    currentHandling = reinterpret_cast<RTH::CHandlingData*>(addr);

    if (currentHandling == nullptr) {
        menu.Option("Could not find handling pointer!");
        return;
    }

    // TODO:
    // 1. Combine the ratio things to meta spec
    // 2. Reload vehicle for:
    //    - Suspension stuff
    //    - ???
    // 3. Precision handling
    menu.FloatOption("fMass", currentHandling->fMass, 0.0f, 1000000000.0f, 5.0f);
    menu.FloatOption("fDownforceModifier", currentHandling->fDownforceModifier, 0.0f, 1000.0f);

    menu.FloatOption("vecCentreOfMassOffset.x", currentHandling->vecCentreOfMassOffset.x, -1000.0f, 1000.0f);
    menu.FloatOption("vecCentreOfMassOffset.y", currentHandling->vecCentreOfMassOffset.y, -1000.0f, 1000.0f);
    menu.FloatOption("vecCentreOfMassOffset.z", currentHandling->vecCentreOfMassOffset.z, -1000.0f, 1000.0f);

    menu.FloatOption("vecInteriaMultiplier.x", currentHandling->vecInteriaMultiplier.x, -1000.0f, 1000.0f);
    menu.FloatOption("vecInteriaMultiplier.y", currentHandling->vecInteriaMultiplier.y, -1000.0f, 1000.0f);
    menu.FloatOption("vecInteriaMultiplier.z", currentHandling->vecInteriaMultiplier.z, -1000.0f, 1000.0f);

    menu.FloatOption("fPercentSubmerged", currentHandling->fPercentSubmerged, -1000.0f, 1000.0f, 0.01f);

    {
        float fDriveBiasFront = currentHandling->fDriveBiasFront;
        float fDriveBiasFrontNorm;

        if (fDriveBiasFront > 0.0f && fDriveBiasFront < 1.0f) {
            fDriveBiasFrontNorm = fDriveBiasFront / 2.0f;
        }
        else {
            fDriveBiasFrontNorm = fDriveBiasFront;
        }

        if (menu.FloatOption("fDriveBiasFront", fDriveBiasFrontNorm, 0.0f, 1.0f, 0.01f)) {
            if (IsNear(fDriveBiasFrontNorm, 1.0f, 0.005f)) {
                currentHandling->fDriveBiasFront = 1.0f;
                currentHandling->fAcceleration = 0.0f;
            }
            else if (IsNear(fDriveBiasFrontNorm, 0.0f, 0.005f)) {
                currentHandling->fDriveBiasFront = 0.0f;
                currentHandling->fAcceleration = 1.0f;
            }
            else {
                currentHandling->fDriveBiasFront = fDriveBiasFrontNorm * 2.0f;
                currentHandling->fAcceleration = 2.0f * (1.0f - (fDriveBiasFrontNorm));
            }
        }
    }

    //menu.FloatOption("fAcceleration", currentHandling->fAcceleration, -1000.0f, 1000.0f, 0.01f);

    menu.IntOption("nInitialDriveGears", currentHandling->nInitialDriveGears, 1, 7, 1);
    menu.FloatOption("fDriveIntertia", currentHandling->fDriveIntertia, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fClutchChangeRateScaleUpShift", currentHandling->fClutchChangeRateScaleUpShift, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fClutchChangeRateScaleDownShift", currentHandling->fClutchChangeRateScaleDownShift, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fInitialDriveForce", currentHandling->fInitialDriveForce, -1000.0f, 1000.0f, 0.01f);

    {
        float fInitialDriveMaxFlatVel = currentHandling->fDriveMaxFlatVel_ * 3.6f;
        if (menu.FloatOption("fInitialDriveMaxFlatVel", fInitialDriveMaxFlatVel, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fInitialDriveMaxFlatVel_ = fInitialDriveMaxFlatVel / 3.6f;
            currentHandling->fDriveMaxFlatVel_ = fInitialDriveMaxFlatVel / 3.0f;
        }
    }

    menu.FloatOption("fBrakeForce", currentHandling->fBrakeForce, -1000.0f, 1000.0f, 0.01f);

    {
        float fBrakeBiasFront = currentHandling->fBrakeBiasFront_ / 2.0f;
        if (menu.FloatOption("fBrakeBiasFront", fBrakeBiasFront, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fBrakeBiasFront_ = fBrakeBiasFront * 2.0f;
            currentHandling->fBrakeBiasRear_ = 2.0f * (1.0f - fBrakeBiasFront);
        }
    }
    
    menu.FloatOption("fHandBrakeForce", currentHandling->fHandBrakeForce2, -1000.0f, 1000.0f, 0.01f);

    {
        // rad 2 deg
        float fSteeringLock = currentHandling->fSteeringLock_ / 0.017453292f;
        if (menu.FloatOption("fSteeringLock", fSteeringLock, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fSteeringLock_ = fSteeringLock * 0.017453292f;
            currentHandling->fSteeringLockRatio_ = 1.0f / (fSteeringLock * 0.017453292f);
        }
    }

    {
        float fTractionCurveMax = currentHandling->fTractionCurveMax;
        if (menu.FloatOption("fTractionCurveMax", fTractionCurveMax, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fTractionCurveMax = fTractionCurveMax;
            currentHandling->fTractionCurveMaxRatio_ = 1.0f / fTractionCurveMax;
        }
    }

    {
        float fTractionCurveMin = currentHandling->fTractionCurveMin;
        if (menu.FloatOption("fTractionCurveMin", fTractionCurveMin, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fTractionCurveMin = fTractionCurveMin;
            currentHandling->fTractionCurveMinRatio_ = 1.0f / fTractionCurveMin;
        }
    }

    {
        // rad 2 deg
        float fTractionCurveLateral = currentHandling->fTractionCurveLateral_ / 0.017453292f;
        if (menu.FloatOption("fTractionCurveLateral", fTractionCurveLateral, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fTractionCurveLateral_ = fTractionCurveLateral * 0.017453292f;
            currentHandling->fTractionCurveLateralRatio_ = 1.0f / (fTractionCurveLateral * 0.017453292f);
        }
    }

    {
        float fTractionSpringDeltaMax = currentHandling->fTractionSpringDeltaMax;
        if (menu.FloatOption("fTractionSpringDeltaMax", fTractionSpringDeltaMax, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fTractionSpringDeltaMax = fTractionSpringDeltaMax;
            currentHandling->fTractionSpringDeltaMaxRatio_ = 1.0f / fTractionSpringDeltaMax;
        }
    }

    menu.FloatOption("fLowSpeedTractionLossMult", currentHandling->fLowSpeedTractionLossMult, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fCamberStiffnesss", currentHandling->fCamberStiffnesss, -1000.0f, 1000.0f, 0.01f);

    { // todo: ???
        float fTractionBiasFront = currentHandling->fTractionBiasFront_ / 2.0f;
        if (menu.FloatOption("fTractionBiasFront", fTractionBiasFront, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fTractionBiasFront_ = 2.0f * fTractionBiasFront;
            currentHandling->fTractionBiasRear = 2.0f * (1.0f - (fTractionBiasFront));
        }
    }

    menu.FloatOption("fTractionLossMult", currentHandling->fTractionLossMult, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fSuspensionForce", currentHandling->fSuspensionForce, -1000.0f, 1000.0f, 0.01f);

    {
        float fSuspensionCompDamp = currentHandling->fSuspensionCompDamp * 10.0f;
        if (menu.FloatOption("fSuspensionCompDamp", fSuspensionCompDamp, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fSuspensionCompDamp = fSuspensionCompDamp / 10.0f;
        }
    }

    {
        float fSuspensionReboundDamp = currentHandling->fSuspensionReboundDamp * 10.0f;
        if (menu.FloatOption("fSuspensionReboundDamp", fSuspensionReboundDamp, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fSuspensionReboundDamp = fSuspensionReboundDamp / 10.0f;
        }
    }

    menu.FloatOption("fSuspensionUpperLimit", currentHandling->fSuspensionUpperLimit, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fSuspensionLowerLimit", currentHandling->fSuspensionLowerLimit, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fSuspensionRaise_", currentHandling->fSuspensionRaise_, -1000.0f, 1000.0f, 0.01f);

    {
        float fSuspensionBiasFront = currentHandling->fSuspensionBiasFront_ / 2.0f;
        if (menu.FloatOption("fSuspensionBiasFront_", fSuspensionBiasFront, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fSuspensionBiasFront_ = fSuspensionBiasFront * 2.0f;
            currentHandling->fSuspensionBiasRear_ = 2.0f * (1.0f - (fSuspensionBiasFront));
        }
    }

    menu.FloatOption("fAntiRollBarForce", currentHandling->fAntiRollBarForce, -1000.0f, 1000.0f, 0.01f);

    {
        float fAntiRollBarBiasFront = currentHandling->fAntiRollBarBiasFront_ / 2.0f;
        if (menu.FloatOption("fAntiRollBarBiasFront_", fAntiRollBarBiasFront, -1000.0f, 1000.0f, 0.01f)) {
            currentHandling->fAntiRollBarBiasFront_ = fAntiRollBarBiasFront * 2.0f;
            currentHandling->fAntiRollBarBiasRear_ = 2.0f * (1.0f - (fAntiRollBarBiasFront));
        }
    }

    menu.FloatOption("fRollCentreHeightFront", currentHandling->fRollCentreHeightFront, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fRollCentreHeightRear", currentHandling->fRollCentreHeightRear, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fCollisionDamageMult", currentHandling->fCollisionDamageMult, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fWeaponDamageMult", currentHandling->fWeaponDamageMult, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fDeformationDamageMult", currentHandling->fDeformationDamageMult, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fEngineDamageMult", currentHandling->fEngineDamageMult, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fPetrolTankVolume", currentHandling->fPetrolTankVolume, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("fOilVolume", currentHandling->fOilVolume, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("vecSeatOffsetDistX", currentHandling->vecSeatOffsetDist.x, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("vecSeatOffsetDistY", currentHandling->vecSeatOffsetDist.y, -1000.0f, 1000.0f, 0.01f);
    menu.FloatOption("vecSeatOffsetDistZ", currentHandling->vecSeatOffsetDist.z, -1000.0f, 1000.0f, 0.01f);
    menu.IntOption("nMonetaryValue", currentHandling->nMonetaryValue, 0, 1000000, 1);

    {
        std::string strModelFlags = fmt::format("{:x}", currentHandling->strModelFlags);
        if (menu.Option(fmt::format("strModelFlags: {}", strModelFlags))) {
            
        }
    }

    {
        std::string strHandlingFlags = fmt::format("{:x}", currentHandling->strHandlingFlags);
        if (menu.Option(fmt::format("strModelFlags: {}", strHandlingFlags))) {

        }
    }

    {
        std::string strDamageFlags = fmt::format("{:x}", currentHandling->strDamageFlags);
        if (menu.Option(fmt::format("strDamageFlags: {}", strDamageFlags))) {

        }
    }

    {
        std::string AIHandling = fmt::format("{}", currentHandling->AIHandling);
        if (menu.Option(fmt::format("AIHandling: {}", AIHandling))) {

        }
    }
}

void UpdateMenu() {
    menu.CheckKeys();

    // MainMenu
    if (menu.CurrentMenu("mainmenu")) { UpdateMainMenu(); }

    // MainMenu -> EditHandlingMenu
    if (menu.CurrentMenu("EditMenu")) { UpdateEditMenu(); }

    menu.EndMenu();
}
