/*
 * SPDX-FileCopyrightText: 2016-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "virtualkeyboard.h"
#include <fcitx/inputmethodengine.h>
#include "fcitx-utils/dbus/objectvtable.h"
#include "fcitx-utils/dbus/servicewatcher.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/log.h"
#include "fcitx-utils/stringutils.h"
#include "fcitx-utils/utf8.h"
#include "fcitx/action.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputmethodentry.h"
#include "fcitx/inputmethodmanager.h"
#include "fcitx/instance.h"
#include "fcitx/menu.h"
#include "fcitx/misc_p.h"
#include "fcitx/userinterfacemanager.h"
#include "dbus_public.h"

namespace fcitx {

class VirtualKeyboardProxy : public dbus::ObjectVTable<VirtualKeyboardProxy> {

public:
    VirtualKeyboardProxy(VirtualKeyboard *parent, dbus::Bus *bus)
        : bus_(bus) {}

    bool processKeyEvent(uint32_t keyval, uint32_t keycode, uint32_t state,
                         bool isRelease, uint32_t time) {

        return true;
    }
    
    FCITX_OBJECT_VTABLE_METHOD(processKeyEvent, "ProcessKeyEvent", "uuubu",
                               "b");

private:
    dbus::Bus *bus_;
};

VirtualKeyboard::VirtualKeyboard(Instance *instance)
    : instance_(instance), bus_(dbus()->call<IDBusModule::bus>()),
      watcher_(*bus_) {
    entry_ = watcher_.watchService(
        "org.fcitx.virtualkeyboard.inputpanel", [this](const std::string &, const std::string &,
                                  const std::string &newOwner) {
            FCITX_INFO() << "VirtualKeyboard new owner: " << newOwner;
            //setAvailable(!newOwner.empty());
        });
}

VirtualKeyboard::~VirtualKeyboard() {}

void VirtualKeyboard::suspend() {
    eventHandlers_.clear();
    proxy_.reset();
    bus_->releaseName("org.fcitx.virtualkeyboard.inputmethod");
}

void VirtualKeyboard::resume() {
    proxy_ = std::make_unique<VirtualKeyboardProxy>(this, bus_);
    bus_->addObjectVTable("/virtualkeyboard", "org.fcitx.virtualkeyboard.inputmethod", *proxy_);
    bus_->requestName(
        "org.fcitx.virtualkeyboard.inputmethod",
        Flags<dbus::RequestNameFlag>{dbus::RequestNameFlag::ReplaceExisting,
                                     dbus::RequestNameFlag::Queue});
    bus_->flush();

    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextSwitchInputMethod, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &icEvent = static_cast<InputContextEvent &>(event);
            updateCurrentInputMethod(icEvent.inputContext());
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputMethodGroupChanged, EventWatcherPhase::Default,
        [this](Event &) {
            if (auto *ic = instance_->lastFocusedInputContext()) {
                updateCurrentInputMethod(ic);
            }
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusIn, EventWatcherPhase::Default,
        [this](Event &event) {
            // Difference IC has difference set of actions.
            auto &icEvent = static_cast<InputContextEvent &>(event);
            updateCurrentInputMethod(icEvent.inputContext());
        }));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::FocusGroupFocusChanged, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &focusEvent =
                static_cast<FocusGroupFocusChangedEvent &>(event);
            if (!focusEvent.newFocus() &&
                lastInputContext_.get() == focusEvent.oldFocus()) {
                bus_->flush();
            }
        }));
}

void VirtualKeyboard::update(UserInterfaceComponent component,
                      InputContext *inputContext) {
    if (component == UserInterfaceComponent::InputPanel) {
        updateInputPanel(inputContext);
    }
}

void VirtualKeyboard::updateInputPanel(InputContext *inputContext) {
    lastInputContext_ = inputContext->watch();
    auto *instance = this->instance();
    auto &inputPanel = inputContext->inputPanel();

    auto preedit = instance->outputFilter(inputContext, inputPanel.preedit());
    auto auxUp = instance->outputFilter(inputContext, inputPanel.auxUp());
    auto preeditString = preedit.toString();
    auto auxUpString = auxUp.toString();
    if (!preeditString.empty() || !auxUpString.empty()) {
        auto text = auxUpString + preeditString;
        if (preedit.cursor() >= 0 &&
            static_cast<size_t>(preedit.cursor()) <= preeditString.size()) {
            auto cursor = preedit.cursor() + auxUpString.size();
            auto utf8Cursor = utf8::lengthValidated(
                text.begin(), std::next(text.begin(), cursor));
            // proxy_->updateAux("", "");
            // proxy_->updatePreeditText(text, "");
            if (utf8Cursor != utf8::INVALID_LENGTH) {
                // proxy_->updatePreeditCaret(utf8Cursor);
            } else {
                // proxy_->updatePreeditCaret(0);
            }
            // proxy_->showPreedit(true);
            // proxy_->showAux(false);
        } else {
            // proxy_->updateAux(text, "");
            // proxy_->updatePreeditText("", "");
            // proxy_->showPreedit(false);
            // proxy_->showAux(true);
        }
    } else {
        // proxy_->showAux(false);
        // proxy_->showPreedit(false);
    }

    // auto auxDown = instance->outputFilter(inputContext, inputPanel.auxDown());
    // auto auxDownString = auxDown.toString();
    // auto candidateList = inputPanel.candidateList();

    // auto msg = bus_->createMethodCall("org.kde.impanel", "/org/kde/impanel",
    //                                   "org.kde.impanel2", "SetLookupTable");
    // auto visible =
    //     !auxDownString.empty() || (candidateList && candidateList->size());
    // if (visible) {
    //     std::vector<std::string> labels;
    //     std::vector<std::string> texts;
    //     std::vector<std::string> attrs;
    //     bool hasPrev = false, hasNext = false;
    //     int pos = -1;
    //     int layout = static_cast<int>(CandidateLayoutHint::NotSet);
    //     if (!auxDownString.empty()) {
    //         labels.emplace_back("");
    //         texts.push_back(auxDownString);
    //         attrs.emplace_back("");
    //     }
    //     auxDownIsEmpty_ = auxDownString.empty();
    //     if (candidateList) {
    //         for (int i = 0, e = candidateList->size(); i < e; i++) {
    //             const auto &candidate = candidateList->candidate(i);
    //             if (candidate.isPlaceHolder()) {
    //                 continue;
    //             }
    //             Text labelText = candidate.hasCustomLabel()
    //                                  ? candidate.customLabel()
    //                                  : candidateList->label(i);

    //             labelText = instance->outputFilter(inputContext, labelText);
    //             labels.push_back(labelText.toString());
    //             auto candidateText =
    //                 instance->outputFilter(inputContext, candidate.text());
    //             texts.push_back(candidateText.toString());
    //             attrs.emplace_back("");
    //         }
    //         if (auto *pageable = candidateList->toPageable()) {
    //             hasPrev = pageable->hasPrev();
    //             hasNext = pageable->hasNext();
    //         }
    //         pos = candidateList->cursorIndex();
    //         if (pos >= 0) {
    //             pos += (auxDownString.empty() ? 0 : 1);
    //         }
    //         layout = static_cast<int>(candidateList->layoutHint());
    //     }

    //     msg << labels << texts << attrs;
    //     msg << hasPrev << hasNext << pos << layout;
    // } else {
    //     std::vector<std::string> labels;
    //     std::vector<std::string> texts;
    //     std::vector<std::string> attrs;
    //     msg << labels << texts << attrs;
    //     msg << false << false << -1 << 0;
    // }
    // msg.send();
    // proxy_->showLookupTable(visible);
    // bus_->flush();
}

std::string VirtualKeyboard::inputMethodStatus(InputContext *ic) {
    std::string label;
    std::string description = _("Not available");
    std::string altDescription = "";
    std::string icon = "input-keyboard";
    if (ic) {
        icon = instance_->inputMethodIcon(ic);
        if (auto entry = instance_->inputMethodEntry(ic)) {
            label = entry->label();
            if (auto engine = instance_->inputMethodEngine(ic)) {
                auto subModeLabel = engine->subModeLabel(*entry, *ic);
                if (!subModeLabel.empty()) {
                    label = subModeLabel;
                }
                altDescription = engine->subMode(*entry, *ic);
            }
            description = entry->name();
        }
    }

    static const bool preferSymbolic = !isKDE();
    if (preferSymbolic && icon == "input-keyboard") {
        icon = "input-keyboard-symbolic";
    }

    return stringutils::concat("/Fcitx/im:", description, ":", iconName(icon),
                               ":", altDescription, ":menu,label=", label);
}

void VirtualKeyboard::updateCurrentInputMethod(InputContext *ic) {
    if (!proxy_) {
        return;
    }
    // proxy_->updateProperty(inputMethodStatus(ic));
    // proxy_->enable(true);
}

void VirtualKeyboard::setAvailable(bool available) {
    if (available != available_) {
        available_ = available;
        instance()->userInterfaceManager().updateAvailability();
    }
}

class VirtualKeyboardFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        return new VirtualKeyboard(manager->instance());
    }
};
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::VirtualKeyboardFactory);
