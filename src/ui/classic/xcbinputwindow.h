/*
 * SPDX-FileCopyrightText: 2017-2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_
#define _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_

#include "inputwindow.h"
#include "xcbwindow.h"

namespace fcitx {
namespace classicui {

class XCBInputWindow : public XCBWindow, protected InputWindow {
public:
    XCBInputWindow(XCBUI *ui);

    void postCreateWindow() override;
    void update(InputContext *inputContext);
    void updatePosition(InputContext *inputContext);

    bool filterEvent(xcb_generic_event_t *event) override;

    void updateDPI(InputContext *inputContext);

private:
    void repaint();
    const Rect *getClosestScreen(const Rect &cursorRect) const;
    int calculatePositionX(const Rect &cursorRect,
                           const Rect *closestScreen) const;
    int calculatePositionY(const Rect &cursorRect,
                           const Rect *closestScreen) const;

    xcb_atom_t atomBlur_;
    int dpi_ = -1;
};
} // namespace classicui
} // namespace fcitx

#endif // _FCITX_UI_CLASSIC_XCBINPUTWINDOW_H_
