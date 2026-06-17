#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <iomanip>
#include <sstream>

using namespace geode::prelude;
using namespace cocos2d;

bool g_colorPickerActive = false;

class $modify(MyEditorUI, EditorUI) {
    struct Fields {
        CCSprite* pickerDot = nullptr;
    };

    void createMoveMenu() {
        EditorUI::createMoveMenu();

        if (!m_editButtonBar) return;


        auto toggleSprite = CCSprite::createWithSpriteFrameName("pickerdos.png"_spr);
        toggleSprite->setScale(1.0f);

        auto toggleBtn = this->getSpriteButton(
            toggleSprite,
            menu_selector(MyEditorUI::onColorPickerToggle),
            nullptr,
            0.9f,
            0,
            {0, 0}
        );
        toggleBtn->setID("picker-toggle"_spr);

        m_editButtonBar->m_buttonArray->addObject(toggleBtn);

        auto rows = GameManager::get()->getIntGameVariable("0049");
        auto cols = GameManager::get()->getIntGameVariable("0050");
        m_editButtonBar->reloadItems(rows, cols);
    }

    bool init(LevelEditorLayer* lel) {
        if (!EditorUI::init(lel)) return false;

        // pointer selector
        m_fields->pickerDot = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
        m_fields->pickerDot->setScale(0.3f);
        m_fields->pickerDot->setVisible(false);
        m_fields->pickerDot->setZOrder(99999);
        this->addChild(m_fields->pickerDot);

        return true;
    }

    void onColorPickerToggle(CCObject* sender) {
        g_colorPickerActive = !g_colorPickerActive;

        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        btn->setColor(g_colorPickerActive ? ccColor3B{0, 255, 0} : ccColor3B{255, 255, 255});

        if (m_fields->pickerDot) {
            m_fields->pickerDot->setVisible(g_colorPickerActive);
        }

        Notification::create(
            g_colorPickerActive ? "Picker Activated" : "Picker Disabled",
            NotificationIcon::None
        )->show();
    }

    // Read the pixel color at the pointer's position
    ccColor3B getPixelAt(CCPoint touchPos) {
        auto glView = CCEGLView::sharedOpenGLView();

        float frameScaleX = glView->getFrameSize().width / glView->getDesignResolutionSize().width;
        float frameScaleY = glView->getFrameSize().height / glView->getDesignResolutionSize().height;

        int x = static_cast<int>(touchPos.x * frameScaleX);
        int y = static_cast<int>(touchPos.y * frameScaleY);

        unsigned char data[4];
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);

        return { data[0], data[1], data[2] };
    }

    // Convert ccColor3B to HEX
    std::string toHex(ccColor3B color) {
        std::stringstream ss;
        ss << "#" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << (int)color.r
           << std::setfill('0') << std::setw(2) << std::hex << (int)color.g
           << std::setfill('0') << std::setw(2) << std::hex << (int)color.b;
        return ss.str();
    }

    // pointer update
    void updatePicker(CCTouch* touch) {
        auto location = touch->getLocation();
        auto nodePos = this->convertToNodeSpace(location);

        m_fields->pickerDot->setPosition(nodePos);
        m_fields->pickerDot->setColor(this->getPixelAt(location));
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent* event) override {
        if (!g_colorPickerActive) return EditorUI::ccTouchBegan(touch, event);
        this->updatePicker(touch);
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent* event) override {
        if (g_colorPickerActive) {
            this->updatePicker(touch);
        } else {
            EditorUI::ccTouchMoved(touch, event);
        }
    }

    void ccTouchEnded(CCTouch* touch, CCEvent* event) override {
        if (!g_colorPickerActive) {
            EditorUI::ccTouchEnded(touch, event);
            return;
        }

        ccColor3B finalColor = this->getPixelAt(touch->getLocation());
        std::string hexStr = this->toHex(finalColor);

        // copy the HEX
        geode::utils::clipboard::write(hexStr);

        auto colorIcon = CCSprite::createWithSpriteFrameName("GJ_colorBtn_001.png");
        colorIcon->setColor(finalColor);
        colorIcon->setScale(0.5f);

        Notification::create((hexStr + " Copied").c_str(), colorIcon)->show();
    }
};
