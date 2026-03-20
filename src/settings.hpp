#pragma once

class SettingsManager {
public:
	static SettingsManager* get();
	void init();

	#ifdef GEODE_IS_DESKTOP
	bool autoHideMenu = true;
	bool autoShowMenu = true;
	bool altDisablesZoom = true;
	float zoomSensitivity = 1.0f;
	#endif
};
