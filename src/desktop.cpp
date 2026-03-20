#ifdef GEODE_IS_DESKTOP

#include "utils.hpp"
#include "desktop.hpp"
#include "settings.hpp"
#include "constants.hpp"

#include <geode.custom-keybinds/include/Keybinds.hpp>

#include <Geode/Geode.hpp>

#include <Geode/modify/PauseLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/PlayLayer.hpp>
#ifdef GEODE_IS_WINDOWS
#include <Geode/modify/CCEGLView.hpp>
#else
#include <objc/message.h>
#endif // GEODE_IS_WINDOWS
#include <Geode/modify/CCScheduler.hpp>

using namespace geode::prelude;
using namespace keybinds;

WindowsZoomManager* WindowsZoomManager::get() {
	static auto inst = new WindowsZoomManager;
	return inst;
}

CCNode* WindowsZoomManager::getPlayLayer() {
	return CCScene::get()->getChildByID("PlayLayer");
}

void WindowsZoomManager::togglePauseMenu() {
	if (!isPaused) return;

	CCNode* pauseLayer = CCScene::get()->getChildByID("PauseLayer");
	if (!pauseLayer) return;

	pauseLayer->setVisible(!pauseLayer->isVisible());
}

void WindowsZoomManager::setPauseMenuVisible(bool visible) {
	CCNode* pauseLayer = CCScene::get()->getChildByID("PauseLayer");
	if (!pauseLayer) return;

	pauseLayer->setVisible(visible);
}

void WindowsZoomManager::setZoom(float zoom) {
	auto* playLayer = getPlayLayer();
	if (!playLayer) return;

	playLayer->setScale(zoom);
	onScreenModified();
}

void WindowsZoomManager::zoom(float delta) {
	auto* playLayer = getPlayLayer();
	if (!playLayer) return;

	CCPoint mousePos = getMousePos();
	zoomPlayLayer(playLayer, delta, mousePos);
	onScreenModified();
}

void WindowsZoomManager::move(CCPoint delta) {
	auto* playLayer = getPlayLayer();
	if (!playLayer) return;

	CCPoint pos = playLayer->getPosition();
	playLayer->setPosition(pos + delta);

	onScreenModified();
}

void WindowsZoomManager::setPos(float x, float y) {
	auto* playLayer = getPlayLayer();
	if (!playLayer) return;

	playLayer->setPosition(CCPoint{ x, y });

	onScreenModified();
}

float WindowsZoomManager::getZoom() {
	auto* playLayer = getPlayLayer();
	if (!playLayer) return zoom::kDefaultZoom;

	return playLayer->getScale();
}

CCPoint WindowsZoomManager::screenToWorld(CCPoint pos) {
	CCSize screenSize = getScreenSize();
	CCSize winSize = CCEGLView::get()->getFrameSize();

	return CCPoint{
		pos.x * (screenSize.width / winSize.width),
		pos.y * (screenSize.height / winSize.height)
	};
}

CCPoint WindowsZoomManager::getMousePosOnNode(CCNode* node) {
	return node->convertToNodeSpace(getMousePos());
}

void WindowsZoomManager::update(float dt) {
	auto mousePos = getMousePos();

	deltaMousePos = CCPoint{ mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y };
	lastMousePos = mousePos;

	if (!isPaused) return;

	if (isPanning) {
		move(deltaMousePos);
	}
}

void WindowsZoomManager::onResume() {
	setZoom(zoom::kDefaultZoom);
	setPos(0.0f, 0.0f);

	isPaused = false;
	isPanning = false;
}

void WindowsZoomManager::onPause() {
	isPaused = true;
	isPanning = false;
}

void WindowsZoomManager::onScroll(float y, float x) {
	if (!isPaused) return;

	auto* playLayer = getPlayLayer();
	if (!playLayer) return;

	if (SettingsManager::get()->altDisablesZoom) {
		auto kb = CCKeyboardDispatcher::get();
		if (kb->getAltKeyPressed()) {
			return;
		}
	}

	float baseDelta = SettingsManager::get()->zoomSensitivity * zoom::kSensitivityScale;

	if (Loader::get()->isModLoaded("prevter.smooth-scroll")) {
		zoom(-y * baseDelta * zoom::kSmoothScrollDamping);
	} else if (y > 0) {
		zoom(-baseDelta);
	} else {
		zoom(baseDelta);
	}

	if (y > 0) {
		if (SettingsManager::get()->autoShowMenu &&
			playLayer->getScale() <= zoom::kAutoShowZoomThreshold) {
			setPauseMenuVisible(true);
		}
	} else {
		if (SettingsManager::get()->autoHideMenu) setPauseMenuVisible(false);
	}
}

void WindowsZoomManager::onScreenModified() {
	auto* playLayer = getPlayLayer();
	if (!playLayer) return;

	clampPlayLayerPos(playLayer);
}

class $modify(PauseLayer) {
	void customSetup() {
		this->template addEventListener<InvokeBindFilter>([=](InvokeBindEvent* event) {
			if (event->isDown()) {
				WindowsZoomManager::get()->togglePauseMenu();
			}

			return ListenerResult::Propagate;
		}, "toggle_menu"_spr);

		PauseLayer::customSetup();
	}

	void onResume(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onResume(sender);
	}

	void onRestart(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onRestart(sender);
	}

	void onRestartFull(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onRestartFull(sender);
	}

	void onNormalMode(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onNormalMode(sender);
	}

	void onPracticeMode(CCObject* sender) {
		WindowsZoomManager::get()->onResume();
		PauseLayer::onPracticeMode(sender);
	}
};

class $modify(PlayLayer) {
	void pauseGame(bool p0) {
		WindowsZoomManager::get()->onPause();
		PlayLayer::pauseGame(p0);
	}

	void startGame() {
		WindowsZoomManager::get()->onResume();
		PlayLayer::startGame();
	}

	bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
		WindowsZoomManager::get()->onResume();
		return PlayLayer::init(level, useReplay, dontCreateObjects);
	}

	void levelComplete() {
		WindowsZoomManager::get()->onResume();
		PlayLayer::levelComplete();
	}
};

class $modify(CCScheduler) {
	virtual void update(float dt) {
		WindowsZoomManager::get()->update(dt);
		CCScheduler::update(dt);
	}
};

#ifdef GEODE_IS_WINDOWS
class $modify(CCEGLView) {
	void onGLFWMouseCallBack(GLFWwindow* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
			if (action == GLFW_PRESS) {
				WindowsZoomManager::get()->isPanning = true;
			}
			else if (action == GLFW_RELEASE) {
				WindowsZoomManager::get()->isPanning = false;
			}
		}

		CCEGLView::onGLFWMouseCallBack(window, button, action, mods);
	}
};
#else
// macOS requires reinterpret_cast for objc_msgSend trampolines —
// this is the standard pattern for calling ObjC methods from C++.
void otherMouseDownHook(void* self, SEL sel, void* event) {
	WindowsZoomManager::get()->isPanning = true;
	reinterpret_cast<void(*)(void*, SEL, void*)>(objc_msgSend)(self, sel, event);
}

void otherMouseUpHook(void* self, SEL sel, void* event) {
	WindowsZoomManager::get()->isPanning = false;
	reinterpret_cast<void(*)(void*, SEL, void*)>(objc_msgSend)(self, sel, event);
}

$execute {
	if (auto hook = ObjcHook::create("EAGLView", "otherMouseDown:", &otherMouseDownHook)) {
		(void) Mod::get()->claimHook(hook.unwrap());
	}

	if (auto hook = ObjcHook::create("EAGLView", "otherMouseUp:", &otherMouseUpHook)) {
		(void) Mod::get()->claimHook(hook.unwrap());
	}
}
#endif // GEODE_IS_WINDOWS

class $modify(CCMouseDispatcher) {
	bool dispatchScrollMSG(float y, float x) {
		WindowsZoomManager::get()->onScroll(y, x);
		return CCMouseDispatcher::dispatchScrollMSG(y, x);
	}
};
#endif // GEODE_IS_DESKTOP
