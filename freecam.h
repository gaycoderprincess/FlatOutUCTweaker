namespace FreeCam {
	CNyaTimer gTimer;

	const float fPanSpeedBase = 0.005;

	bool bEnabled = false;
	int nLastGameState = -1;

	double fMouse[2] = {};
	NyaVec3 vPos = {0, 0, 0};
	NyaVec3 vLookatOffset = {0, 0, 0};
	NyaVec3 vLookatOffsetAbs = {0, 0, 0};
	NyaVec3 vFollowOffset = {0, 0, 0};
	NyaVec3 vFollowOffsetAbs = {0, 0, 0};
	NyaVec3 vAngle = {0, 0, 0};
	NyaVec3 vLastPlayerPosition = {0, 0, 0};
	float fFOV = 90;
	bool bFirstMove = false;
	float fMoveSpeed = 10;
	float fPanSpeed = 1;
	float fMouseRotateSpeed = 0.05;
	float fMouseScrollSpeed = 0.5;
	int nLookAtPlayer = -1;
	int nFollowPlayer = -1;
	bool bFollowString = false;
	float fStringMaxDistance = 7;
	float fStringMinDistance = 4;

	Player* GetLookatPlayer() {
		if (nLookAtPlayer >= 0) return GetPlayer(nLookAtPlayer);
		return nullptr;
	}

	Player* GetFollowPlayer() {
		if (nFollowPlayer >= 0) return GetPlayer(nFollowPlayer);
		return nullptr;
	}

	NyaVec3* GetTargetPosition() {
		static NyaVec3 vec;
		if (auto lookatPlayer = GetLookatPlayer()) {
			auto plyMat = *lookatPlayer->pCar->GetMatrix();
			vec = plyMat.p;
			plyMat.p = {0, 0, 0};
			vec += plyMat * vLookatOffset;
			vec += vLookatOffsetAbs;
			return &vec;
		}
		return nullptr;
	}

	NyaVec3* GetFollowPosition() {
		static NyaVec3 vec;
		if (auto lookatPlayer = GetFollowPlayer()) {
			auto plyMat = *lookatPlayer->pCar->GetMatrix();
			vec = plyMat.p;
			plyMat.p = {0, 0, 0};
			vec += plyMat * vFollowOffset;
			vec += vFollowOffsetAbs;
			return &vec;
		}
		return nullptr;
	}

	void SetRotation(Camera *cam) {
		auto mat = cam->GetMatrix();
		if (auto plyPos = GetTargetPosition()) {
			auto lookat = *plyPos - vPos;
			lookat.Normalize();
			*mat = NyaMat4x4::LookAt(lookat);
		} else {
			mat->SetIdentity();
			mat->Rotate({vAngle[1], vAngle[2], vAngle[0]});
			//vLookatOffset = {0,0,0};
		}
		mat->p = vPos;
	}

	float fBackupDist;

	void BackupDistance() {
		auto plyPos = GetTargetPosition();
		if (!plyPos) return;

		fBackupDist = (vPos - *plyPos).length();
	}

	void RestoreDistance(Camera *cam) {
		auto plyPos = GetTargetPosition();
		if (!plyPos) return;

		auto dist = (vPos - *plyPos).length();
		vPos += cam->GetMatrix()->z * dist;
		vPos -= cam->GetMatrix()->z * fBackupDist;
	}

	void DoMovement(Camera *cam) {
		auto lookatPlayer = GetLookatPlayer();
		auto panKey = lookatPlayer ? VK_MBUTTON : VK_LBUTTON;
		auto rotateKey = lookatPlayer ? VK_LBUTTON : VK_MBUTTON;

		auto mat = cam->GetMatrix();
		if (IsKeyPressed(panKey)) {
			BackupDistance();
			vPos -= mat->x * fMouse[0] * fPanSpeed * fPanSpeedBase;
			vPos -= mat->y * fMouse[1] * -fPanSpeed * fPanSpeedBase;
			RestoreDistance(cam);
		}
		if (IsKeyPressed(VK_RBUTTON)) {
			vAngle[2] += fMouse[0] * fMouseRotateSpeed * (std::numbers::pi / 180.0);
			fFOV += fMouse[1] * fMouseRotateSpeed * (std::numbers::pi / 180.0);
		}
		if (IsKeyPressed(rotateKey)) {
			if (lookatPlayer) {
				//NyaVec3 moveAmount = {0,0,0};
				//moveAmount -= mat->x * fMouse[0] * fPanSpeed * fPanSpeedBase;
				//moveAmount -= mat->y * fMouse[1] * -fPanSpeed * fPanSpeedBase;
				//auto carMat = *lookatPlayer->pCar->GetMatrix();
				//carMat.p = {0,0,0};
				//carMat.Invert();
				//vLookatOffset += carMat * moveAmount;
			} else {
				vAngle[0] += fMouse[0] * -fMouseRotateSpeed * (std::numbers::pi / 180.0);
				vAngle[1] += fMouse[1] * -fMouseRotateSpeed * (std::numbers::pi / 180.0);
			}
		}
		if (abs(nMouseWheelState) > 0) {
			vPos += mat->z * nMouseWheelState * fMouseScrollSpeed;
		}

		float maxPitch = std::numbers::pi * 0.4999;
		if (vAngle[1] < -maxPitch) vAngle[1] = -maxPitch;
		if (vAngle[1] > maxPitch) vAngle[1] = maxPitch;

		float minFOV = 1 * (std::numbers::pi / 180.0);
		float maxFOV = 179 * (std::numbers::pi / 180.0);
		if (fFOV < minFOV) fFOV = minFOV;
		if (fFOV > maxFOV) fFOV = maxFOV;

		if (bFirstMove) {
			gTimer.Process();

			float swd = 0; // x
			float uwd = 0; // y
			float fwd = 0; // z
			if (IsKeyPressed(VK_LEFT)) swd -= fMoveSpeed;
			if (IsKeyPressed(VK_RIGHT)) swd += fMoveSpeed;
			if (IsKeyPressed(VK_SPACE)) uwd += fMoveSpeed;
			if (IsKeyPressed(VK_CONTROL)) uwd -= fMoveSpeed;
			if (IsKeyPressed(VK_UP)) fwd += fMoveSpeed;
			if (IsKeyPressed(VK_DOWN)) fwd -= fMoveSpeed;

			if (swd != 0.0 || uwd != 0.0) {
				BackupDistance();
				vPos += mat->x * swd * gTimer.fDeltaTime;
				vPos += mat->y * uwd * gTimer.fDeltaTime;
				RestoreDistance(cam);
			}
			vPos += mat->z * fwd * gTimer.fDeltaTime;
		}

		if (auto follow = GetFollowPlayer()) {
			if (bFollowString) {
				auto plyPos = GetFollowPosition();
				auto lookatFront = -(vPos - *plyPos);
				auto dist = lookatFront.length();
				lookatFront.Normalize();
				if (dist > fStringMaxDistance) {
					vPos += lookatFront * dist;
					vPos -= lookatFront * fStringMaxDistance;
				} else if (dist < fStringMinDistance) {
					vPos += lookatFront * dist;
					vPos -= lookatFront * fStringMinDistance;
				}
			} else {
				vPos -= vLastPlayerPosition;
				vPos += follow->pCar->GetMatrix()->p;
			}
			vLastPlayerPosition = follow->pCar->GetMatrix()->p;
		}

		mat->p = vPos;
	}

	void Process(Camera *cam) {
		if (!cam) return;
		if (nLastGameState != pGameFlow->nGameState) {
			vPos = cam->GetMatrix()->p;
			vAngle = {0, 0, 0};
			//vLookatOffset = {0,0,0};
			vLastPlayerPosition = {0, 0, 0};
			fFOV = cam->fFOV;
			nLastGameState = pGameFlow->nGameState;
			if (auto follow = GetFollowPlayer()) {
				vLastPlayerPosition = follow->pCar->GetMatrix()->p;
			}
		}

		SetRotation(cam);
		DoMovement(cam);
		SetRotation(cam);

		fMouse[0] = 0;
		fMouse[1] = 0;
		nMouseWheelState = 0;

		cam->fFOV = fFOV;
		bFirstMove = false;
	}

	void ProcessMenu() {
		ChloeMenuLib::BeginMenu();

		if (DrawMenuOption(std::format("Active - {}", bEnabled), "")) {
			bEnabled = !bEnabled;
			nLastGameState = -1;
		}

		if (DrawMenuOption(std::format("Move Speed - {}", fMoveSpeed), "")) {
			ValueEditorMenu(fMoveSpeed);
		}

		if (DrawMenuOption(std::format("Pan Speed - {}", fPanSpeed), "")) {
			ValueEditorMenu(fPanSpeed);
		}

		if (DrawMenuOption(std::format("Rotate Speed - {}", fMouseRotateSpeed), "")) {
			ValueEditorMenu(fMouseRotateSpeed);
		}

		if (DrawMenuOption(std::format("Scroll Speed - {}", fMouseScrollSpeed), "")) {
			ValueEditorMenu(fMouseScrollSpeed);
		}

		if (DrawMenuOption(std::format("Point & Follow"))) {
			ChloeMenuLib::BeginMenu();

			if (DrawMenuOption(std::format("Point at Player < {} >", nLookAtPlayer >= 0 ? std::format("Player {}", nLookAtPlayer+1) : "false"), "", false, false, true)) {
				auto count = 0;
				if (pGameFlow->nGameState == GAME_STATE_RACE && pGameFlow->pHost) count = pGameFlow->pHost->GetNumPlayers();

				nLookAtPlayer += ChloeMenuLib::GetMoveLR();
				if (nLookAtPlayer < -1) nLookAtPlayer = count-1;
				if (nLookAtPlayer >= count) nLookAtPlayer = -1;
			}

			if (DrawMenuOption(std::format("Point Relative Offset X - {}", vLookatOffset.x), "")) {
				ValueEditorMenu(vLookatOffset.x);
			}

			if (DrawMenuOption(std::format("Point Relative Offset Y - {}", vLookatOffset.y), "")) {
				ValueEditorMenu(vLookatOffset.y);
			}

			if (DrawMenuOption(std::format("Point Relative Offset Z - {}", vLookatOffset.z), "")) {
				ValueEditorMenu(vLookatOffset.z);
			}

			if (DrawMenuOption(std::format("Point Absolute Offset X - {}", vLookatOffsetAbs.x), "")) {
				ValueEditorMenu(vLookatOffsetAbs.x);
			}

			if (DrawMenuOption(std::format("Point Absolute Offset Y - {}", vLookatOffsetAbs.y), "")) {
				ValueEditorMenu(vLookatOffsetAbs.y);
			}

			if (DrawMenuOption(std::format("Point Absolute Offset Z - {}", vLookatOffsetAbs.z), "")) {
				ValueEditorMenu(vLookatOffsetAbs.z);
			}

			if (DrawMenuOption(std::format("Follow Player < {} >", nFollowPlayer >= 0 ? std::format("Player {}", nFollowPlayer+1) : "false"), "", false, false, true)) {
				auto count = 0;
				if (pGameFlow->nGameState == GAME_STATE_RACE && pGameFlow->pHost) count = pGameFlow->pHost->GetNumPlayers();

				nFollowPlayer += ChloeMenuLib::GetMoveLR();
				if (nFollowPlayer < -1) nFollowPlayer = count-1;
				if (nFollowPlayer >= count) nFollowPlayer = -1;

				if (auto follow = GetFollowPlayer()) {
					vLastPlayerPosition = follow->pCar->GetMatrix()->p;
				}
			}

			if (DrawMenuOption(std::format("Follow Relative Offset X - {}", vFollowOffset.x), "")) {
				ValueEditorMenu(vFollowOffset.x);
			}

			if (DrawMenuOption(std::format("Follow Relative Offset Y - {}", vFollowOffset.y), "")) {
				ValueEditorMenu(vFollowOffset.y);
			}

			if (DrawMenuOption(std::format("Follow Relative Offset Z - {}", vFollowOffset.z), "")) {
				ValueEditorMenu(vFollowOffset.z);
			}

			if (DrawMenuOption(std::format("Follow Absolute Offset X - {}", vFollowOffsetAbs.x), "")) {
				ValueEditorMenu(vFollowOffsetAbs.x);
			}

			if (DrawMenuOption(std::format("Follow Absolute Offset Y - {}", vFollowOffsetAbs.y), "")) {
				ValueEditorMenu(vFollowOffsetAbs.y);
			}

			if (DrawMenuOption(std::format("Follow Absolute Offset Z - {}", vFollowOffsetAbs.z), "")) {
				ValueEditorMenu(vFollowOffsetAbs.z);
			}

			if (DrawMenuOption(std::format("Follow on String - {}", bFollowString), "")) {
				bFollowString = !bFollowString;
			}

			if (DrawMenuOption(std::format("String Min Distance - {}", fStringMinDistance), "")) {
				ValueEditorMenu(fStringMinDistance);
			}

			if (DrawMenuOption(std::format("String Max Distance - {}", fStringMaxDistance), "")) {
				ValueEditorMenu(fStringMaxDistance);
			}

			ChloeMenuLib::EndMenu();
		}

		ChloeMenuLib::EndMenu();
	}
}