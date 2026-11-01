#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/Notification.hpp>
#include <cstdlib>
#include <cmath>

using namespace geode::prelude;

namespace {
	// 1 block = 30 GD units. The panel works in blocks.
	constexpr double UNITS = 30.0;

	bool parseNum(std::string const& s, double& out) {
		if (s.empty()) return false;
		char* end = nullptr;
		double v = std::strtod(s.c_str(), &end);
		if (end == s.c_str() || *end != '\0' || !std::isfinite(v)) return false;
		out = v;
		return true;
	}

	std::string fmtNum(double v) {
		auto s = fmt::format("{:.7f}", v);
		if (s == "-0.0000000") s = "0.0000000";
		return s;
	}

	void err(char const* text) {
		Notification::create(text, NotificationIcon::Error)->show();
	}
}

class PrecisePanel : public CCNode {
protected:
	struct Sel {
		int count = -1;
		double cx = 0, cy = 0;
	};

	EditorUI* m_ui = nullptr;
	TextInput* m_xInput = nullptr;
	TextInput* m_yInput = nullptr;
	TextInput* m_stepInput = nullptr;
	CCLabelBMFont* m_info = nullptr;
	std::string m_shownX, m_shownY;
	Sel m_snap;

	Sel getSel() {
		Sel s;
		s.count = 0;
		auto arr = m_ui->getSelectedObjects();
		if (!arr || arr->count() == 0) return s;

		double minX = 1e300, maxX = -1e300, minY = 1e300, maxY = -1e300;
		for (auto obj : CCArrayExt<GameObject*>(arr)) {
			auto p = obj->getPosition();
			minX = std::min(minX, (double)p.x);
			maxX = std::max(maxX, (double)p.x);
			minY = std::min(minY, (double)p.y);
			maxY = std::max(maxY, (double)p.y);
			s.count++;
		}
		s.cx = (minX + maxX) / 2.0;
		s.cy = (minY + maxY) / 2.0;
		return s;
	}

	void showSel() {
		if (m_snap.count <= 0) {
			m_info->setString("Select object(s)");
			m_xInput->setString("");
			m_yInput->setString("");
			m_shownX.clear();
			m_shownY.clear();
			return;
		}
		if (m_snap.count == 1) m_info->setString("1 object (blocks)");
		else m_info->setString(fmt::format("{} objects, center (blocks)", m_snap.count).c_str());

		m_shownX = fmtNum(m_snap.cx / UNITS);
		m_shownY = fmtNum(m_snap.cy / UNITS);
		m_xInput->setString(m_shownX);
		m_yInput->setString(m_shownY);
	}

	void moveSel(double dx, double dy) {
		auto arr = m_ui->getSelectedObjects();
		if (!arr || arr->count() == 0) {
			Notification::create("Nothing selected", NotificationIcon::Warning)->show();
			return;
		}
		CCPoint d(static_cast<float>(dx), static_cast<float>(dy));
		for (auto obj : CCArrayExt<GameObject*>(arr)) {
			m_ui->moveObject(obj, d);
		}
		m_snap.count = -1;
	}

	void onSet() {
		auto s = getSel();
		if (s.count <= 0) {
			Notification::create("Nothing selected", NotificationIcon::Warning)->show();
			return;
		}
		double dx = 0, dy = 0;
		auto xs = m_xInput->getString();
		auto ys = m_yInput->getString();

		if (xs != m_shownX) {
			double v;
			if (!parseNum(xs, v)) return err("Invalid X");
			dx = v * UNITS - s.cx;
		}
		if (ys != m_shownY) {
			double v;
			if (!parseNum(ys, v)) return err("Invalid Y");
			dy = v * UNITS - s.cy;
		}
		if (dx == 0 && dy == 0) return;
		this->moveSel(dx, dy);
	}

	void nudge(int sx, int sy) {
		double step;
		if (!parseNum(m_stepInput->getString(), step)) return err("Invalid step");
		this->moveSel(sx * step * UNITS, sy * step * UNITS);
	}

	bool init(EditorUI* ui) {
		if (!CCNode::init()) return false;
		m_ui = ui;

		CCSize size{250.f, 150.f};
		this->setContentSize(size);
		this->setAnchorPoint({0.5f, 0.5f});

		auto bg = CCScale9Sprite::create("square02b_001.png", {0, 0, 80, 80});
		bg->setColor({0, 0, 0});
		bg->setOpacity(140);
		bg->setContentSize(size);
		bg->setPosition(size / 2);
		this->addChild(bg, -1);

		m_info = CCLabelBMFont::create("", "bigFont.fnt");
		m_info->setScale(0.35f);
		m_info->setPosition({size.width / 2, 134.f});
		this->addChild(m_info);

		auto mkLabel = [&](char const* text, CCPoint pos, float scale) {
			auto l = CCLabelBMFont::create(text, "bigFont.fnt");
			l->setScale(scale);
			l->setPosition(pos);
			this->addChild(l);
		};
		mkLabel("X", {18.f, 108.f}, 0.55f);
		mkLabel("Y", {18.f, 80.f}, 0.55f);
		mkLabel("Step", {28.f, 52.f}, 0.4f);

		m_xInput = TextInput::create(150.f, "X", "chatFont.fnt");
		m_xInput->setFilter("0123456789.-");
		m_xInput->setMaxCharCount(24);
		m_xInput->setPosition({112.f, 108.f});
		this->addChild(m_xInput);

		m_yInput = TextInput::create(150.f, "Y", "chatFont.fnt");
		m_yInput->setFilter("0123456789.-");
		m_yInput->setMaxCharCount(24);
		m_yInput->setPosition({112.f, 80.f});
		this->addChild(m_yInput);

		m_stepInput = TextInput::create(100.f, "1", "chatFont.fnt");
		m_stepInput->setFilter("0123456789.");
		m_stepInput->setMaxCharCount(24);
		m_stepInput->setString("1");
		m_stepInput->setPosition({100.f, 52.f});
		this->addChild(m_stepInput);

		m_xInput->getInputNode()->setTouchPriority(-129);
		m_yInput->getInputNode()->setTouchPriority(-129);
		m_stepInput->getInputNode()->setTouchPriority(-129);

		auto menu = CCMenu::create();
		menu->setPosition({0.f, 0.f});
		this->addChild(menu);

		auto setSpr = ButtonSprite::create("Set");
		setSpr->setScale(0.7f);
		auto setBtn = CCMenuItemExt::createSpriteExtra(setSpr, [this](CCMenuItemSpriteExtra*) {
			this->onSet();
		});
		setBtn->setPosition({222.f, 94.f});
		menu->addChild(setBtn);

		auto mkArrow = [&](char const* frame, float x, int sx, int sy) {
			auto spr = CCSprite::createWithSpriteFrameName(frame);
			spr->setScale(0.7f);
			auto btn = CCMenuItemExt::createSpriteExtra(spr, [this, sx, sy](CCMenuItemSpriteExtra*) {
				this->nudge(sx, sy);
			});
			btn->setPosition({x, 20.f});
			menu->addChild(btn);
		};
		mkArrow("edit_leftBtn_001.png", 45.f, -1, 0);
		mkArrow("edit_rightBtn_001.png", 95.f, 1, 0);
		mkArrow("edit_upBtn_001.png", 145.f, 0, 1);
		mkArrow("edit_downBtn_001.png", 195.f, 0, -1);

		this->scheduleUpdate();
		return true;
	}

public:
	static PrecisePanel* create(EditorUI* ui) {
		auto ret = new PrecisePanel();
		if (ret->init(ui)) {
			ret->autorelease();
			return ret;
		}
		delete ret;
		return nullptr;
	}

	void update(float) override {
		auto s = getSel();
		if (s.count == m_snap.count && s.cx == m_snap.cx && s.cy == m_snap.cy) return;
		m_snap = s;
		this->showSel();
	}
};

class $modify(PreciseEditorUI, EditorUI) {
	bool init(LevelEditorLayer* lel) {
		if (!EditorUI::init(lel)) return false;

		auto panel = PrecisePanel::create(this);
		if (!panel) return true;
		panel->setID("precise-pos-panel"_spr);

		auto win = CCDirector::get()->getWinSize();
		float left = static_cast<float>(Mod::get()->getSettingValue<double>("panel-left"));
		float top = static_cast<float>(Mod::get()->getSettingValue<double>("panel-top"));
		auto sz = panel->getContentSize();
		panel->setPosition({left + sz.width / 2, win.height - top - sz.height / 2});

		this->addChild(panel, 100);
		return true;
	}
};
