// ===========================================================================
// VA 0x00430510 with helpers 0x00415E90 / 0x00416090
// ===========================================================================
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstdint>

#include "mikudancestudio/mmd_app.hpp"
#include "mikudancestudio/model.hpp"
#include "mikudancestudio/offsets.hpp"
#include "mikudancestudio/ported_funcs.hpp"

namespace mikudancestudio {
namespace {

struct Curve {
    int x1;
    int y1;
    int x2;
    int y2;
};

bool operator==(const Curve& left, const Curve& right) {
    return left.x1 == right.x1 && left.y1 == right.y1 &&
           left.x2 == right.x2 && left.y2 == right.y2;
}

void DrawCurve(MMDApp* app, const Curve& curve) {  // 0x415E90
    HDC dc = app->CurveDC();
    HPEN pen = CreatePen(PS_SOLID, 1, 0xFF0000u);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    int oldX = 0;
    int oldY = 126;
    for (int step = 1; step < 100; ++step) {
        const float t = static_cast<float>(step) / 100.0f;
        const float ax = static_cast<float>(curve.x1) * t;
        const float ay = static_cast<float>(curve.y1) * t;
        const float bx = static_cast<float>(curve.x1) +
                         static_cast<float>(curve.x2 - curve.x1) * t;
        const float by = static_cast<float>(curve.y1) +
                         static_cast<float>(curve.y2 - curve.y1) * t;
        const float cx = static_cast<float>(curve.x2) +
                         static_cast<float>(128 - curve.x2) * t;
        const float cy = static_cast<float>(curve.y2) +
                         static_cast<float>(128 - curve.y2) * t;
        const float dx = ax + (bx - ax) * t;
        const float dy = ay + (by - ay) * t;
        const float ex = bx + (cx - bx) * t;
        const float ey = by + (cy - by) * t;
        const int x = static_cast<int>(dx + (ex - dx) * t);
        const int y = 126 - static_cast<int>(dy + (ey - dy) * t);
        MoveToEx(dc, oldX, oldY, nullptr);
        LineTo(dc, x, y);
        oldX = x;
        oldY = y;
    }
    SelectObject(dc, oldPen);
    DeleteObject(pen);
}

void DrawControlPoints(MMDApp* app, const Curve& curve) {  // 0x416090
    HDC dc = app->CurveDC();
    HPEN pen = CreatePen(PS_SOLID, 2, 0x000000FFu);
    HGDIOBJ oldPen = SelectObject(dc, pen);

    app->raw<std::uint8_t>(645637) = static_cast<std::uint8_t>(curve.x1);
    app->raw<std::uint8_t>(645638) =
        static_cast<std::uint8_t>(127 - curve.y1);
    app->raw<std::uint8_t>(645639) = static_cast<std::uint8_t>(curve.x2);
    app->raw<std::uint8_t>(645640) =
        static_cast<std::uint8_t>(127 - curve.y2);

    MoveToEx(dc, curve.x1 - 3, 124 - curve.y1, nullptr);
    LineTo(dc, curve.x1 + 3, 130 - curve.y1);
    MoveToEx(dc, curve.x1 + 3, 124 - curve.y1, nullptr);
    LineTo(dc, curve.x1 - 3, 130 - curve.y1);
    MoveToEx(dc, curve.x2 - 3, 124 - curve.y2, nullptr);
    LineTo(dc, curve.x2 + 3, 130 - curve.y2);
    MoveToEx(dc, curve.x2 + 3, 124 - curve.y2, nullptr);
    LineTo(dc, curve.x2 - 3, 130 - curve.y2);
    SelectObject(dc, oldPen);
    DeleteObject(pen);

    HPEN guide = CreatePen(PS_SOLID, 0, 0);
    SelectObject(dc, guide);
    MoveToEx(dc, 0, 127, nullptr);
    LineTo(dc, curve.x1, 127 - curve.y1);
    MoveToEx(dc, 127, 0, nullptr);
    LineTo(dc, curve.x2, 127 - curve.y2);
    SelectObject(dc, oldPen);
    DeleteObject(guide);
}

Curve ReadCurve(const unsigned char* record, int channel,
                int x1Base, int y1Base, int x2Base, int y2Base) {
    const auto value = [record](int offset) {
        return static_cast<int>(*reinterpret_cast<const std::int8_t*>(
            record + offset));
    };
    return {value(x1Base + channel), value(y1Base + channel),
            value(x2Base + channel), value(y2Base + channel)};
}

void AcceptCurve(MMDApp* app, const Curve& curve, bool& found,
                 bool& uniform, Curve& baseline) {
    DrawCurve(app, curve);
    if (!found) {
        baseline = curve;
        found = true;
    } else if (!(curve == baseline)) {
        uniform = false;
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// VA 0x00416280 - Sub416280(app): interpolation-curve panel drag handler
// (mouse move while dragging a control point).  Repaints the 128x128 curve
// cache white, clamps the pointer to the panel (x = mouse X-8, y =
// client.bottom - mouse Y - 8, both clamped 0..127), then rewrites the
// dragged control point on every selected record and redraws the curve
// from the last record touched:
//   byte 0x2F8 != 0  -> camera/global records app+0x374 (84B, 10000,
//                       gate +72): mode byte 0x9D939 == 1 edits the x1/y1
//                       groups (+40/+46, all 6 channels when the channel
//                       combo 433 selection >= 6, else only the selected
//                       channel), == 2 edits x2/y2 (+52/+58); curve drawn
//                       with the opposite group's bytes.
//   else             -> selected model's BoneKey records (300000 entries):
//                       mode 1 edits interpolation x1/y1, mode 2 edits
//                       x2/y2 (four channels).
// The original passes uninitialized stack values to the draw calls when no
// record matched; the port passes a zeroed curve (deterministic stand-in).
// Ends with DrawCurve + DrawControlPoints (0x415E90/0x416090) and the
// 8/bottom-135/137/bottom-6 invalidate rect.
// ---------------------------------------------------------------------------
void Sub416280(MMDApp* app) {
    const HWND window = static_cast<HWND>(app->Hwnd());
    HDC dc = app->CurveDC();
    HPEN pen = CreatePen(PS_SOLID, 1, 0x00FFFFFFu);              // 0x41629D
    HBRUSH brush = CreateSolidBrush(0x00FFFFFFu);                // 0x4162AB
    SelectObject(dc, pen);
    SelectObject(dc, brush);
    Rectangle(dc, 0, 0, 128, 128);                               // 0x4162D6
    DeleteObject(pen);
    DeleteObject(brush);

    RECT client{};
    GetClientRect(window, &client);                              // 0x4162F4
    int x = app->MouseX() - 8;                                   // 0x4162FD
    if (x < 0) x = 0; else if (x > 127) x = 127;
    int y = client.bottom - app->MouseY() - 8;                   // 0x416317
    if (y < 0) y = 0; else if (y > 127) y = 127;

    Curve drawn{};  // (x1, y1, x2, y2) fed to both draw helpers
    const int selected = static_cast<int>(SendMessageA(
        GetDlgItem(window, 433), CB_GETCURSEL, 0, 0));

    if (app->state.optflag0 != 0) {   // 0x416337
        unsigned char* records = app->raw<unsigned char*>(0x374);
        if (records != nullptr) {
            for (int off = 0; off < 840000; off += 84) {
                if (records[off + 72] == 0)
                    continue;
                const std::uint8_t mode =
                    app->raw<std::uint8_t>(645641);
                if (mode == 1) {
                    unsigned char* base;
                    if (selected >= 6) {                         // 0x416397
                        for (int c = 0; c < 6; ++c) {
                            records[off + 40 + c] =
                                static_cast<std::uint8_t>(x);
                            records[off + 46 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        base = records + off;
                    } else {                                     // 0x4163A2
                        records[off + selected + 40] =
                            static_cast<std::uint8_t>(x);
                        records[off + selected + 46] =
                            static_cast<std::uint8_t>(y);
                        base = records + off + selected;
                    }
                    drawn = {x, y,
                             static_cast<std::int8_t>(base[52]),
                             static_cast<std::int8_t>(base[58])};
                } else if (mode == 2) {
                    unsigned char* base;
                    if (selected >= 6) {                         // 0x41648F
                        for (int c = 0; c < 6; ++c) {
                            records[off + 52 + c] =
                                static_cast<std::uint8_t>(x);
                            records[off + 58 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        base = records + off;
                    } else {                                     // 0x41649A
                        records[off + selected + 52] =
                            static_cast<std::uint8_t>(x);
                        records[off + selected + 58] =
                            static_cast<std::uint8_t>(y);
                        base = records + off + selected;
                    }
                    drawn = {static_cast<std::int8_t>(base[40]),
                             static_cast<std::int8_t>(base[46]), x, y};
                }
            }
        }
    } else {                                                     // 0x41656B
        unsigned char* model = app->SelectedModel();
        mdl::BoneKey* records = model != nullptr
            ? mdl::BoneKeys(model) : nullptr;
        if (records != nullptr) {
            for (int index = 0; index < 300000; ++index) {
                mdl::BoneKey& record = records[index];
                if (record.allocated == 0)
                    continue;
                const std::uint8_t mode =
                    app->raw<std::uint8_t>(645641);
                if (mode == 1) {
                    int channel;
                    if (selected >= 4) {                         // 0x4165C7
                        for (int c = 0; c < 4; ++c) {
                            record.interpolation[c] =
                                static_cast<std::uint8_t>(x);
                            record.interpolation[4 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        channel = 0;
                    } else {                                     // 0x4165CB
                        record.interpolation[selected] =
                            static_cast<std::uint8_t>(x);
                        record.interpolation[4 + selected] =
                            static_cast<std::uint8_t>(y);
                        channel = selected;
                    }
                    drawn = {x, y,
                             static_cast<std::int8_t>(
                                 record.interpolation[8 + channel]),
                             static_cast<std::int8_t>(
                                 record.interpolation[12 + channel])};
                } else if (mode == 2) {
                    int channel;
                    if (selected >= 4) {                         // 0x416725
                        for (int c = 0; c < 4; ++c) {
                            record.interpolation[8 + c] =
                                static_cast<std::uint8_t>(x);
                            record.interpolation[12 + c] =
                                static_cast<std::uint8_t>(y);
                        }
                        channel = 0;
                    } else {                                     // 0x416729
                        record.interpolation[8 + selected] =
                            static_cast<std::uint8_t>(x);
                        record.interpolation[12 + selected] =
                            static_cast<std::uint8_t>(y);
                        channel = selected;
                    }
                    drawn = {
                        static_cast<std::int8_t>(
                            record.interpolation[channel]),
                        static_cast<std::int8_t>(
                            record.interpolation[4 + channel]), x, y};
                }
            }
        }
    }

    DrawCurve(app, drawn);                                       // 0x41684F
    DrawControlPoints(app, drawn);                               // 0x41686C
    GetClientRect(window, &client);                              // 0x41687D
    RECT dirty{8, client.bottom - 135, 137, client.bottom - 6};
    InvalidateRect(window, &dirty, FALSE);                       // 0x4168BC
}

void SelectionReeval(MMDApp* app) {  // 0x430510
    app->raw<std::uint8_t>(645636) = 0;
    HDC dc = app->CurveDC();
    HPEN whitePen = CreatePen(PS_SOLID, 1, 0x00FFFFFFu);
    HBRUSH whiteBrush = CreateSolidBrush(0x00FFFFFFu);
    HGDIOBJ oldPen = SelectObject(dc, whitePen);
    HGDIOBJ oldBrush = SelectObject(dc, whiteBrush);
    Rectangle(dc, 0, 0, 128, 128);
    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
    DeleteObject(whitePen);
    DeleteObject(whiteBrush);

    const HWND window = static_cast<HWND>(app->Hwnd());
    const int selectedChannel = static_cast<int>(SendMessageA(
        GetDlgItem(window, 433), CB_GETCURSEL, 0, 0));
    bool found = false;
    bool uniform = true;
    Curve baseline{};

    if (app->state.optflag0 != 0) {
        unsigned char* records = app->raw<unsigned char*>(0x374);
        if (records != nullptr) {
            for (int index = 0; index < 10000; ++index) {
                const unsigned char* record = records + 84 * index;
                if (record[72] == 0)
                    continue;
                if (selectedChannel < 6) {
                    AcceptCurve(app,
                        ReadCurve(record, selectedChannel, 40, 46, 52, 58),
                        found, uniform, baseline);
                } else {
                    for (int channel = 0; channel < 6; ++channel) {
                        AcceptCurve(app,
                            ReadCurve(record, channel, 40, 46, 52, 58),
                            found, uniform, baseline);
                    }
                }
            }
        }
    } else {
        unsigned char* model = app->SelectedModel();
        mdl::BoneKey* records = model != nullptr
            ? mdl::BoneKeys(model) : nullptr;
        if (records != nullptr) {
            for (int index = 0; index < 300000; ++index) {
                const mdl::BoneKey& record = records[index];
                if (record.allocated == 0)
                    continue;
                if (selectedChannel < 4) {
                    AcceptCurve(app,
                        ReadCurve(record.interpolation, selectedChannel,
                                  0, 4, 8, 12),
                        found, uniform, baseline);
                } else {
                    for (int channel = 0; channel < 4; ++channel) {
                        AcceptCurve(app,
                            ReadCurve(record.interpolation, channel,
                                      0, 4, 8, 12),
                            found, uniform, baseline);
                    }
                }
            }
        }
    }

    if (found && uniform) {
        app->raw<std::uint8_t>(645636) = 1;
        DrawControlPoints(app, baseline);
    }
    EnableWindow(GetDlgItem(window, 430), uniform ? TRUE : FALSE);
    EnableWindow(GetDlgItem(window, 432), found ? TRUE : FALSE);
    EnableWindow(GetDlgItem(window, 431),
        found && app->raw<std::int8_t>(645642) >= 0 ? TRUE : FALSE);

    RECT client{};
    GetClientRect(window, &client);
    RECT dirty{8, client.bottom - 135, 137, client.bottom - 6};
    InvalidateRect(window, &dirty, FALSE);
}

}  // namespace mikudancestudio
