/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_VIRTUALKEYBOARD_VIRTUALBOARD_H_
#define _FCITX_UI_VIRTUALKEYBOARD_VIRTUALBOARD_H_

#include "fcitx-utils/dbus/bus.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/event.h"
#include "fcitx-utils/fs.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/focusgroup.h"
#include "fcitx/icontheme.h"
#include "fcitx/instance.h"
#include "fcitx/userinterface.h"

namespace fcitx {

class VirtualKeyboardProxy;
class Action;

class VirtualKeyboard : public UserInterface {
public:
    VirtualKeyboard(Instance *instance);
    ~VirtualKeyboard();

    Instance *instance() { return instance_; }
    void suspend() override;
    void resume() override;
    bool available() override { return available_; }
    void update(UserInterfaceComponent component,
                InputContext *inputContext) override;
    void updateInputPanel(InputContext *inputContext);
    void updateCurrentInputMethod(InputContext *ic);

    void msgV1Handler(dbus::Message &msg);
    void msgV2Handler(dbus::Message &msg);

    void registerAllProperties(InputContext *ic = nullptr);
    std::string inputMethodStatus(InputContext *ic);
    std::string actionToStatus(Action *action, InputContext *ic);

private:
    void setAvailable(bool available);

    std::string iconName(const std::string &icon) {
        return IconTheme::iconName(icon, inFlatpak_);
    }

    FCITX_ADDON_DEPENDENCY_LOADER(dbus, instance_->addonManager());

    Instance *instance_;
    dbus::Bus *bus_;
    dbus::ServiceWatcher watcher_;
    std::unique_ptr<VirtualKeyboardProxy> proxy_;
    std::unique_ptr<dbus::ServiceWatcherEntry> entry_;
    std::vector<std::unique_ptr<HandlerTableEntry<EventHandler>>>
        eventHandlers_;
    TrackableObjectReference<InputContext> lastInputContext_;
    bool auxDownIsEmpty_ = true;
    std::unique_ptr<EventSourceTime> timeEvent_;
    bool available_ = true;
    std::unique_ptr<dbus::Slot> relativeQuery_;
    bool hasRelative_ = false;
    bool hasRelativeV2_ = false;
    const bool inFlatpak_ = fs::isreg("/.flatpak-info");
};
} // namespace fcitx

#endif // _FCITX_UI_VIRTUALKEYBOARD_VIRTUALBOARD_H_
