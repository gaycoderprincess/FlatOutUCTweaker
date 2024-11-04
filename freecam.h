CNyaTimer gFreecamTimer;

const float fFreeCamPanSpeedBase = 0.005;

bool bFreeCam = false;
int nFreeCamCopy = -1;

double fMouse[2] = {};
NyaVec3 vFreeCamPos = {0,0,0};
NyaVec3 vFreeCamLookatOffset = {0,0,0};
NyaVec3 vFreeCamLookatOffsetAbs = {0,0,0};
NyaVec3 vFreeCamFollowOffset = {0,0,0};
NyaVec3 vFreeCamFollowOffsetAbs = {0,0,0};
NyaVec3 vFreeCamAngle = {0,0,0};
NyaVec3 vFreeCamLastPlayerPosition = {0,0,0};
float fFreeCamFOV = 90;
bool bFreeCamFirstMove = false;
float fFreeCamSpeed = 10;
float fFreeCamPanSpeed = 1;
float fFreeCamMouseSpeed = 0.05;
float fFreeCamMouseScrollSpeed = 0.5;
int nFreeCamLookAtPlayer = -1;
int nFreeCamFollowPlayer = -1;
bool bFreeCamFollowString = false;
float fFreeCamStringMaxDistance = 7;
float fFreeCamStringMinDistance = 4;

Player* GetFreecamLookatPlayer() {
	if (nFreeCamLookAtPlayer >= 0) return GetPlayer(nFreeCamLookAtPlayer);
	return nullptr;
}

Player* GetFreecamFollowPlayer() {
	if (nFreeCamFollowPlayer >= 0) return GetPlayer(nFreeCamFollowPlayer);
	return nullptr;
}

NyaVec3* GetFreecamTargetPosition() {
	static NyaVec3 vec;
	if (auto lookatPlayer = GetFreecamLookatPlayer()) {
		auto plyMat = *lookatPlayer->pCar->GetMatrix();
		vec = plyMat.p;
		plyMat.p = {0,0,0};
		vec += plyMat * vFreeCamLookatOffset;
		vec += vFreeCamLookatOffsetAbs;
		return &vec;
	}
	return nullptr;
}

NyaVec3* GetFreecamFollowPosition() {
	static NyaVec3 vec;
	if (auto lookatPlayer = GetFreecamFollowPlayer()) {
		auto plyMat = *lookatPlayer->pCar->GetMatrix();
		vec = plyMat.p;
		plyMat.p = {0,0,0};
		vec += plyMat * vFreeCamFollowOffset;
		vec += vFreeCamFollowOffsetAbs;
		return &vec;
	}
	return nullptr;
}

void SetFreecamRotation(Camera* cam) {
	auto mat = cam->GetMatrix();
	if (auto plyPos = GetFreecamTargetPosition()) {
		auto lookat = *plyPos - vFreeCamPos;
		lookat.Normalize();
		*mat = NyaMat4x4::LookAt(lookat);
	}
	else {
		*mat = NyaMat4x4::LookAt({std::sin(vFreeCamAngle[0]) * std::cos(vFreeCamAngle[1]), std::sin(vFreeCamAngle[1]),
								  std::cos(vFreeCamAngle[0]) * std::cos(vFreeCamAngle[1])});
		//vFreeCamLookatOffset = {0,0,0};
	}

	NyaMat4x4 tilt;
	tilt.Rotate({0,0,vFreeCamAngle[2]});
	*mat *= tilt;
	mat->p = vFreeCamPos;
}

float fFreecamBackupDist;
void BackupFreecamDistance() {
	auto plyPos = GetFreecamTargetPosition();
	if (!plyPos) return;

	fFreecamBackupDist = (vFreeCamPos - *plyPos).length();
}

void RestoreFreecamDistance(Camera* cam) {
	auto plyPos = GetFreecamTargetPosition();
	if (!plyPos) return;

	auto dist = (vFreeCamPos - *plyPos).length();
	vFreeCamPos += cam->GetMatrix()->z * dist;
	vFreeCamPos -= cam->GetMatrix()->z * fFreecamBackupDist;
}

void DoFreecamMovement(Camera* cam) {
	auto lookatPlayer = GetFreecamLookatPlayer();
	auto panKey = lookatPlayer ? VK_MBUTTON : VK_LBUTTON;
	auto rotateKey = lookatPlayer ? VK_LBUTTON : VK_MBUTTON;

	auto mat = cam->GetMatrix();
	if (IsKeyPressed(panKey)) {
		BackupFreecamDistance();
		vFreeCamPos -= mat->x * fMouse[0] * fFreeCamPanSpeed * fFreeCamPanSpeedBase;
		vFreeCamPos -= mat->y * fMouse[1] * -fFreeCamPanSpeed * fFreeCamPanSpeedBase;
		RestoreFreecamDistance(cam);
	}
	if (IsKeyPressed(VK_RBUTTON)) {
		vFreeCamAngle[2] += fMouse[0]*-fFreeCamMouseSpeed*(std::numbers::pi/180.0);
		fFreeCamFOV += fMouse[1]*fFreeCamMouseSpeed*(std::numbers::pi/180.0);
	}
	if (IsKeyPressed(rotateKey)) {
		if (lookatPlayer) {
			//NyaVec3 moveAmount = {0,0,0};
			//moveAmount -= mat->x * fMouse[0] * fFreeCamPanSpeed * fFreeCamPanSpeedBase;
			//moveAmount -= mat->y * fMouse[1] * -fFreeCamPanSpeed * fFreeCamPanSpeedBase;
			//auto carMat = *lookatPlayer->pCar->GetMatrix();
			//carMat.p = {0,0,0};
			//carMat.Invert();
			//vFreeCamLookatOffset += carMat * moveAmount;
		}
		else {
			vFreeCamAngle[0] += fMouse[0] * fFreeCamMouseSpeed * (std::numbers::pi / 180.0);
			vFreeCamAngle[1] += fMouse[1] * -fFreeCamMouseSpeed * (std::numbers::pi / 180.0);
		}
	}
	if (abs(nMouseWheelState) > 0) {
		vFreeCamPos += mat->z * nMouseWheelState * fFreeCamMouseScrollSpeed;
	}

	float maxPitch = std::numbers::pi*0.4999;
	if (vFreeCamAngle[1] < -maxPitch) vFreeCamAngle[1] = -maxPitch;
	if (vFreeCamAngle[1] > maxPitch) vFreeCamAngle[1] = maxPitch;

	float minFOV = 1*(std::numbers::pi/180.0);
	float maxFOV = 179*(std::numbers::pi/180.0);
	if (fFreeCamFOV < minFOV) fFreeCamFOV = minFOV;
	if (fFreeCamFOV > maxFOV) fFreeCamFOV = maxFOV;

	if (bFreeCamFirstMove) {
		gFreecamTimer.Process();

		float swd = 0; // x
		float uwd = 0; // y
		float fwd = 0; // z
		if (IsKeyPressed(VK_LEFT)) swd -= fFreeCamSpeed;
		if (IsKeyPressed(VK_RIGHT)) swd += fFreeCamSpeed;
		if (IsKeyPressed(VK_SPACE)) uwd += fFreeCamSpeed;
		if (IsKeyPressed(VK_CONTROL)) uwd -= fFreeCamSpeed;
		if (IsKeyPressed(VK_UP)) fwd += fFreeCamSpeed;
		if (IsKeyPressed(VK_DOWN)) fwd -= fFreeCamSpeed;

		if (swd != 0.0 || uwd != 0.0) {
			BackupFreecamDistance();
			vFreeCamPos += mat->x * swd * gFreecamTimer.fDeltaTime;
			vFreeCamPos += mat->y * uwd * gFreecamTimer.fDeltaTime;
			RestoreFreecamDistance(cam);
		}
		vFreeCamPos += mat->z * fwd * gFreecamTimer.fDeltaTime;
	}

	if (auto follow = GetFreecamFollowPlayer()) {
		if (bFreeCamFollowString) {
			auto plyPos = GetFreecamFollowPosition();
			auto lookatFront = -(vFreeCamPos - *plyPos);
			auto dist = lookatFront.length();
			lookatFront.Normalize();
			if (dist > fFreeCamStringMaxDistance) {
				vFreeCamPos += lookatFront * dist;
				vFreeCamPos -= lookatFront * fFreeCamStringMaxDistance;
			} else if (dist < fFreeCamStringMinDistance) {
				vFreeCamPos += lookatFront * dist;
				vFreeCamPos -= lookatFront * fFreeCamStringMinDistance;
			}
		}
		else {
			vFreeCamPos -= vFreeCamLastPlayerPosition;
			vFreeCamPos += follow->pCar->GetMatrix()->p;
		}
		vFreeCamLastPlayerPosition = follow->pCar->GetMatrix()->p;
	}

	mat->p = vFreeCamPos;
}

void DoFreeCamera(Camera* cam) {
	if (!cam) return;
	if (nFreeCamCopy != pGameFlow->nGameState) {
		vFreeCamPos = cam->GetMatrix()->p;
		vFreeCamAngle = {0,0,0};
		//vFreeCamLookatOffset = {0,0,0};
		vFreeCamLastPlayerPosition = {0,0,0};
		fFreeCamFOV = cam->fFOV;
		nFreeCamCopy = pGameFlow->nGameState;
		if (auto follow = GetFreecamFollowPlayer()) {
			vFreeCamLastPlayerPosition = follow->pCar->GetMatrix()->p;
		}
	}

	SetFreecamRotation(cam);
	DoFreecamMovement(cam);
	SetFreecamRotation(cam);

	fMouse[0] = 0;
	fMouse[1] = 0;
	nMouseWheelState = 0;

	cam->fFOV = fFreeCamFOV;
	bFreeCamFirstMove = false;
}