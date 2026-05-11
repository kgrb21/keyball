/*
Copyright 2022 @Yowkees
Copyright 2022 MURAOKA Taro (aka KoRoN, @kaoriya)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include QMK_KEYBOARD_H

// --- 追加: オートマウスレイヤー用の設定 ---
#define MOUSE_LAYER_INDEX 2              // マウスレイヤーの番号（Remapの設定に合わせて変更してください）
#define AUTO_MOUSE_LAYER_KEEP_TIME 30000 // 通常の維持時間（30秒）
#define MOUSE_TIMEOUT_AFTER_CLICK 500    // クリック後の維持時間（0.5秒）
#define AML_ACTIVATE_THRESHOLD 10        // 起動しきい値（誤爆防止）

uint16_t aml_timer = 0;
uint16_t aml_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
bool aml_active = false;
static int16_t aml_x = 0;
static int16_t aml_y = 0;

#define ABS(x) ((x) < 0 ? -(x) : (x))

void report_mouse_user(report_mouse_t* mouse_report) {
    if (mouse_report->x != 0 || mouse_report->y != 0) {
        if (!aml_active) {
            // しきい値（THRESHOLD）の判定
            aml_x += mouse_report->x;
            aml_y += mouse_report->y;
            if (ABS(aml_x) > AML_ACTIVATE_THRESHOLD || ABS(aml_y) > AML_ACTIVATE_THRESHOLD) {
                layer_on(MOUSE_LAYER_INDEX);
                aml_active = true;
                aml_x = 0;
                aml_y = 0;
            }
        }
        
        if (aml_active) {
            // トラックボールを動かしている間はタイムアウトを30秒にリセット
            aml_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
            aml_timer = timer_read();
        }
    }
}

// --- 追加: クリック後の離脱制御 ---
    if (aml_active) {
        if (keycode >= KC_MS_BTN1 && keycode <= KC_MS_BTN5) {
            if (!record->event.pressed) {
                // ボタンを離した瞬間、タイムアウトを短縮(0.5秒)に変更
                aml_timeout = MOUSE_TIMEOUT_AFTER_CLICK;
                aml_timer = timer_read();
            }
        }
    }

// --- 追加: タイムアウト判定 ---
    if (aml_active) {
        if (timer_elapsed(aml_timer) > aml_timeout) {
            layer_off(MOUSE_LAYER_INDEX);
            aml_active = false;
            aml_x = 0;
            aml_y = 0;
        }
    }

#include "quantum.h"

// clang-format off
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  // keymap for default (VIA)
  [0] = LAYOUT_universal(
    KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                            KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     ,
    KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                            KC_H     , KC_J     , KC_K     , KC_L     , KC_MINS  ,
    KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                            KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  ,
    KC_LCTL  , KC_LGUI  , KC_LALT  ,LSFT_T(KC_LNG2),LT(1,KC_SPC),LT(3,KC_LNG1),KC_BSPC,LT(2,KC_ENT),LSFT_T(KC_LNG2),KC_RALT,KC_RGUI, KC_RSFT
  ),

  [1] = LAYOUT_universal(
    KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_RBRC  ,                            KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   ,
    KC_F5    , KC_EXLM  , S(KC_6)  ,S(KC_INT3), S(KC_8)  ,                           S(KC_INT1), KC_BTN1  , KC_PGUP  , KC_BTN2  , KC_SCLN  ,
    S(KC_EQL),S(KC_LBRC),S(KC_7)   , S(KC_2)  ,S(KC_RBRC),                            KC_LBRC  , KC_DLR   , KC_PGDN  , KC_BTN3  , KC_F11   ,
    KC_INT1  , KC_EQL   , S(KC_3)  , _______  , _______  , _______  ,      TO(2)    , TO(0)    , _______  , KC_RALT  , KC_RGUI  , KC_F12
  ),

  [2] = LAYOUT_universal(
    KC_TAB   , KC_7     , KC_8     , KC_9     , KC_MINS  ,                            KC_NUHS  , _______  , KC_BTN3  , _______  , KC_BSPC  ,
   S(KC_QUOT), KC_4     , KC_5     , KC_6     ,S(KC_SCLN),                            S(KC_9)  , KC_BTN1  , KC_UP    , KC_BTN2  , KC_QUOT  ,
    KC_SLSH  , KC_1     , KC_2     , KC_3     ,S(KC_MINS),                           S(KC_NUHS), KC_LEFT  , KC_DOWN  , KC_RGHT  , _______  ,
    KC_ESC   , KC_0     , KC_DOT   , KC_DEL   , KC_ENT   , KC_BSPC  ,      _______  , _______  , _______  , _______  , _______  , _______
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , AML_TO   , AML_I50  , AML_D50  , _______  ,                            _______  , _______  , SSNP_HOR , SSNP_VRT , SSNP_FRE ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , SCRL_DVI ,                            _______  , _______  , _______  , _______  , _______  ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , SCRL_DVD ,                            CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , KBC_SAVE ,
    QK_BOOT  , KBC_RST  , _______  , _______  , _______  , _______  ,      _______  , _______  , _______  , _______  , KBC_RST  , QK_BOOT
  ),
};
// clang-format on

layer_state_t layer_state_set_user(layer_state_t state) {
    // Auto enable scroll mode when the highest layer is 3
    keyball_set_scroll_mode(get_highest_layer(state) == 3);
    return state;
}

#ifdef OLED_ENABLE

#    include "lib/oledkit/oledkit.h"

void oledkit_render_info_user(void) {
    keyball_oled_render_keyinfo();
    keyball_oled_render_ballinfo();
    keyball_oled_render_layerinfo();
}
#endif
