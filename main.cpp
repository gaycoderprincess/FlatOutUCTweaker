#include <windows.h>
#include <d3d9.h>
#include <format>
#include "toml++/toml.hpp"

#include "nya_dx9_hookbase.h"
#include "nya_commonmath.h"
#include "nya_commonhooklib.h"

#include "fouc.h"
#include "chloemenulib.h"

void ValueEditorMenu(float& value) {
	ChloeMenuLib::BeginMenu();

	static char inputString[1024] = {};
	ChloeMenuLib::AddTextInputToString(inputString, 1024, true);
	ChloeMenuLib::SetEnterHint("Apply");

	if (DrawMenuOption(inputString + (std::string)"...", "", false, false) && inputString[0]) {
		value = std::stof(inputString);
		memset(inputString,0,sizeof(inputString));
		ChloeMenuLib::BackOut();
	}

	ChloeMenuLib::EndMenu();
}

#include "freecam.h"

void HookLoop() {}

bool GetResetMapState() {
	return *(uint8_t*)0x4D8460 == 0xEB;
}

void DisableResetMap(bool apply) {
	NyaHookLib::Patch<uint8_t>(0x4D8460, apply ? 0xEB : 0x77);
}

bool GetAutoResetState() {
	return *(uint8_t*)0x43D69E == 0xEB;
}

void DisableAutoReset(bool apply) {
	NyaHookLib::Patch<uint8_t>(0x43D69E, apply ? 0xEB : 0x75);
}

bool GetNoHUDState() {
	return *(uint8_t*)0x489864 == 0xEB;
}

void DisableHUD(bool apply) {
	NyaHookLib::Patch<uint8_t>(0x489864, apply ? 0xEB : 0x74); // ingame
	NyaHookLib::Patch<uint8_t>(0x5F08A0, apply ? 0xC3 : 0x81); // menu selected option name
	NyaHookLib::Patch<uint8_t>(0x4D2D80, apply ? 0xC3 : 0x83);
	NyaHookLib::Patch<uint8_t>(0x4CCC87, apply ? 0xEB : 0x74);
	NyaHookLib::Patch<uint8_t>(0x4CCBDC, apply ? 0xEB : 0x74); // menu lua bits
	NyaHookLib::Patch<uint8_t>(0x4CD920, apply ? 0xC3 : 0x81); // menu text
	NyaHookLib::Patch<uint8_t>(0x5F0250, apply ? 0xC3 : 0x8B); // menu bottom line
	NyaHookLib::Patch<uint8_t>(0x4D29F0, apply ? 0xC3 : 0x55);
	NyaHookLib::Patch<uint8_t>(0x4AA2D0, apply ? 0xC3 : 0x55);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4CCC6C, apply ? 0x5F088E : 0x5F0480);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4CCB27, apply ? 0x5AC5CC : 0x5AC480);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4CCC78, apply ? 0x4CB7C6 : 0x4CADC0); // menu button prompts
}

bool GetNoMenuBordersState() {
	return *(uint8_t*)0x4CCD10 == 0xC3;
}

void DisableMenuBorders(bool apply) {
	NyaHookLib::Patch<uint8_t>(0x4CCD10, apply ? 0xC3 : 0x83);
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x4CCB82, apply ? 0x5F0245 : 0x5F0090);
}

bool bAirbreak = false;
bool bFunnyButton = false;
int nForceCarId = 0;
bool bForceTrack = false;
int nForceTrackId = 1;
bool bUnlockAllCareer = false;
bool bForceCar = false;

void SetForceCar(bool apply) {
	bForceCar = apply;
	NyaHookLib::Patch<uint8_t>(0x46937D, apply ? 0xEB : 0x74); // never use career active car
}

int lua_pushfalse(void* a1, bool a2) {
	return lua_pushboolean(a1, false);
}

void SetUnlockAllCareer(bool apply) {
	bUnlockAllCareer = apply;
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x483CEA, apply ? (uintptr_t)&lua_pushfalse : 0x633870); // cup
	NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x483FE8, apply ? (uintptr_t)&lua_pushfalse : 0x633870); // event
}

void MenuLoop() {
	ChloeMenuLib::BeginMenu();

	if (DrawMenuOption(std::format("No Auto-Reset - {}", GetAutoResetState()), "Disables airtime and derby OOB reset", false, false)) {
		DisableAutoReset(!GetAutoResetState());
	}

	if (DrawMenuOption(std::format("No Reset Zones - {}", GetResetMapState()), "Disables out of map reset zones", false, false)) {
		DisableResetMap(!GetResetMapState());
	}

	if (DrawMenuOption(std::format("No Main Menu Borders - {}", GetNoMenuBordersState()), "", false, false)) {
		DisableMenuBorders(!GetNoMenuBordersState());
	}

	if (DrawMenuOption(std::format("No HUD - {}", GetNoHUDState()), "", false, false)) {
		DisableHUD(!GetNoHUDState());
	}

	if (DrawMenuOption(std::format("Airbreak - {}", bAirbreak), "", false, false)) {
		bAirbreak = !bAirbreak;
	}

	if (DrawMenuOption(std::format("Funny Button - {}", bFunnyButton), "Hold the scrollwheel for the funny", false, false)) {
		bFunnyButton = !bFunnyButton;
	}

	if (DrawMenuOption(std::format("Force Car - {}", bForceCar ? GetCarName(nForceCarId) : "false"), "")) {
		ChloeMenuLib::BeginMenu();

		if (DrawMenuOption(std::format("Active - {}", bForceCar), "")) {
			SetForceCar(!bForceCar);
		}

		if (DrawMenuOption(std::format("Car < {} >", GetCarName(nForceCarId)), "", false, false, true)) {
			auto count = GetNumCars();
			nForceCarId += ChloeMenuLib::GetMoveLR();
			if (nForceCarId < 0) nForceCarId = count - 1;
			if (nForceCarId >= count) nForceCarId = 0;
		}

		ChloeMenuLib::EndMenu();
	}

	if (DrawMenuOption(std::format("Unlock All Career Events - {}", bUnlockAllCareer), "", false, false)) {
		SetUnlockAllCareer(!bUnlockAllCareer);
	}

	if (DrawMenuOption(std::format("Free Camera - {}", FreeCam::bEnabled), "")) {
		FreeCam::ProcessMenu();
	}

	std::string trackName = "*EMPTY " + std::to_string(nForceTrackId) + "*";
	if (auto name = GetTrackName(nForceTrackId)) trackName = name;
	if (DrawMenuOption(std::format("Force Track - {}", bForceTrack ? trackName : "false"), "")) {
		ChloeMenuLib::BeginMenu();

		if (DrawMenuOption(std::format("Active - {}", bForceTrack), "")) {
			bForceTrack = !bForceTrack;
		}

		if (DrawMenuOption(std::format("Track < {} >", trackName), "", false, false, true)) {
			auto count = GetNumTracks();
			nForceTrackId += ChloeMenuLib::GetMoveLR();
			if (nForceTrackId < 1) nForceTrackId = count;
			if (nForceTrackId > count) nForceTrackId = 1;
		}

		ChloeMenuLib::EndMenu();
	}

	if (pGameFlow->nGameState == GAME_STATE_RACE) {
		if (DrawMenuOption("Fix Car", "", false, false)) {
			if (auto ply = GetPlayer(0)) {
				ply->pCar->Fix(false);
			}
		}
	}

	ChloeMenuLib::EndMenu();
}

void UpdateD3DProperties() {
	g_pd3dDevice = pDeviceD3d->pD3DDevice;
	ghWnd = pDeviceD3d->hWnd;
	nResX = nGameResolutionX;
	nResY = nGameResolutionY;
}

void MainLoop() {
	UpdateD3DProperties();

	CNyaTimer gTimer;
	gTimer.Process();

	if (!pGameFlow) return;

	FreeCam::bFirstMove = true;

	if (bForceCar) {
		*(int*)GetLiteDB()->GetTable("GameFlow.PreRace")->GetPropertyPointer("Car") = nForceCarId;
		pGameFlow->nInstantActionCar = nForceCarId;
		NyaHookLib::Patch<uint8_t>(0x46937D, 0xEB); // never use career active car
	}

	if (bForceTrack) {
		*(int*)GetLiteDB()->GetTable("GameFlow.PreRace")->GetPropertyPointer("Level") = nForceTrackId;
	}

	if (FreeCam::bEnabled && pGameFlow->nGameState == GAME_STATE_MENU) {
		FreeCam::Process(pGameFlow->pMenuInterface->Scene.pCamera);
	}

	if (!pLoadingScreen && pGameFlow->nGameState == GAME_STATE_RACE) {
		if (bAirbreak) {
			auto ply = GetPlayer(0);
			auto car = ply->pCar;
			*car->GetVelocity() = {0, 0, 0};
			*car->GetAngVelocity() = {0, 0, 0};

			float fwd = 0;
			float uwd = 0;
			float swd = 0;
			if (IsKeyPressed(VK_LEFT)) swd -= 5;
			if (IsKeyPressed(VK_RIGHT)) swd += 5;
			if (IsKeyPressed(VK_UP)) fwd += 5;
			if (IsKeyPressed(VK_DOWN)) fwd -= 5;
			if (IsKeyPressed('W')) uwd += 5;
			if (IsKeyPressed('S')) uwd -= 5;
			if (IsKeyPressed(VK_LSHIFT)) {
				fwd *= 3;
				swd *= 3;
				uwd *= 3;
			}

			auto mat = car->GetMatrix();
			mat->p += fwd * mat->z * gTimer.fDeltaTime;
			mat->p += uwd * mat->y * gTimer.fDeltaTime;
			mat->p += swd * mat->x * gTimer.fDeltaTime;

			FO2MatrixToQuat(car->mMatrix, car->qQuaternion);
		}

		if (bFunnyButton && IsKeyPressed(VK_MBUTTON)) {
			auto ply = GetPlayer(0);
			auto car = ply->pCar;

			float fwd = (ply->fGasPedal - ply->fBrakePedal) * 5;
			float uwd = 0;
			float swd = ply->fSteeringController * 5;
			if (ply->nIsUsingKeyboard) {
				swd = 0;
				if (ply->nSteeringKeyboardLeft) swd -= 5;
				if (ply->nSteeringKeyboardRight) swd += 5;
			}
			if (IsKeyPressed('Q')) uwd += 5;
			if (IsKeyPressed('E')) uwd -= 5;

			auto mat = car->GetMatrix();
			*car->GetVelocity() += fwd * mat->z * gTimer.fDeltaTime;
			*car->GetVelocity() += uwd * mat->y * gTimer.fDeltaTime;
			*car->GetVelocity() += swd * mat->x * gTimer.fDeltaTime;
		}
	}

	CommonMain(false);
}

uintptr_t DrawHook_jmp = 0x4F49FF;
void __attribute__((naked)) DrawHook() {
	__asm__ (
		"mov eax, [ebx+0x1B4]\n\t"
		"push 0\n\t"
		"push eax\n\t"
		"jmp %0\n\t"
			:
			:  "m" (DrawHook_jmp)
	);
}

void DoForceTrack() {
	if (!bForceTrack) return;
	pGameFlow->nLevelId = nForceTrackId;
}

uintptr_t ForceTrackASM_jmp = 0x46A3F0;
void __attribute__((naked)) ForceTrackASM() {
	__asm__ (
		"pushad\n\t"
		"call %1\n\t"
		"popad\n\t"
		"jmp %0\n\t"
			:
			:  "m" (ForceTrackASM_jmp), "i" (DoForceTrack)
	);
}

auto UpdateCameraHooked_call = (void(__thiscall*)(void*, float))0x4FAEA0;
void __fastcall UpdateCameraHooked(void* a1, void*, float a2) {
	UpdateCameraHooked_call(a1, a2);
	if (FreeCam::bEnabled) {
		FreeCam::Process(pCameraManager->pCamera);
	}
}

void MouseWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	static bool bOnce = true;
	if (bOnce) {
		RAWINPUTDEVICE device;
		device.usUsagePage = 1;
		device.usUsage = 2;
		device.dwFlags = 256;
		device.hwndTarget = hWnd;
		RegisterRawInputDevices(&device, 1, sizeof(RAWINPUTDEVICE));
		bOnce = false;
	}

	switch (msg) {
		case WM_MOUSEWHEEL:
			nMouseWheelState += GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
			break;
		case WM_INPUT: {
			RAWINPUT raw;
			UINT size = sizeof(raw);
			GetRawInputData((HRAWINPUT) lParam, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER));
			if (raw.header.dwType != RIM_TYPEMOUSE) return;
			FreeCam::fMouse[0] += raw.data.mouse.lLastX;
			FreeCam::fMouse[1] += raw.data.mouse.lLastY;
		} break;
		default:
			break;
	}
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch( fdwReason ) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x24CEF7) {
				MessageBoxA(nullptr, aFOUCVersionFail, "nya?!~", MB_ICONERROR);
				exit(0);
				return TRUE;
			}

			ChloeMenuLib::RegisterMenu("UC Tweaker - gaycoderprincess", MenuLoop);
			NyaFO2Hooks::PlaceD3DHooks();
			NyaFO2Hooks::aEndSceneFuncs.push_back(MainLoop);
			if (*(uint64_t*)0x4F49F6 == 0x006A000001B4838B) {
				NyaHookLib::PatchRelative(NyaHookLib::JMP, 0x4F49F6, &DrawHook);
			}
			NyaFO2Hooks::PlaceWndProcHook();
			NyaFO2Hooks::aWndProcFuncs.push_back(MouseWndProc);
			ForceTrackASM_jmp = NyaHookLib::PatchRelative(NyaHookLib::CALL, 0x468234, &ForceTrackASM);

			UpdateCameraHooked_call = (void(__thiscall*)(void*, float))(*(uintptr_t*)0x6EB7DC);
			NyaHookLib::Patch(0x6EB7DC, &UpdateCameraHooked);
		} break;
		default:
			break;
	}
	return TRUE;
}