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


// --- 設定値 ---
#define MOUSE_LAYER_INDEX 2              // マウスレイヤーの番号（適宜変更してください）
#define AUTO_MOUSE_LAYER_KEEP_TIME 30000 // 通常の維持時間（30000ms = 30秒）
#define MOUSE_TIMEOUT_AFTER_CLICK 500    // クリック後の維持時間（例：500ms = 0.5秒）
#define AML_ACTIVATE_THRESHOLD 10        // 起動しきい値（小さいほど敏感、大きいほど鈍感になります）

// --- 状態管理用変数 ---
uint16_t mouse_timer = 0;
uint16_t current_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
bool is_mouse_layer_active = false;
static int16_t x_cumulative = 0;
static int16_t y_cumulative = 0;

// 絶対値を計算する補助関数
#define ABS(x) ((x) < 0 ? -(x) : (x))

// トラックボールの動きを検知する関数
void report_mouse_user(report_mouse_t* mouse_report) {
    if (mouse_report->x != 0 || mouse_report->y != 0) {
        if (!is_mouse_layer_active) {
            // 【4. 意図しない接触の防止】移動量を蓄積
            x_cumulative += mouse_report->x;
            y_cumulative += mouse_report->y;
            
            // しきい値を超えたときだけレイヤーに入る
            if (ABS(x_cumulative) > AML_ACTIVATE_THRESHOLD || ABS(y_cumulative) > AML_ACTIVATE_THRESHOLD) {
                layer_on(MOUSE_LAYER_INDEX);
                is_mouse_layer_active = true;
                x_cumulative = 0;
                y_cumulative = 0;
            }
        }
        
        if (is_mouse_layer_active) {
            // 【3. トラックボールを動かすと30秒にリセット】
            current_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
            mouse_timer = timer_read();
        }
    }
}

// キー操作を検知する関数
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (is_mouse_layer_active) {
        switch (keycode) {
            // 【2. マウスキーをクリックした後の離脱制御】
            // マウスボタン（BTN1〜BTN5）が操作された場合
            case KC_MS_BTN1 ... KC_MS_BTN5:
                if (!record->event.pressed) { // ボタンを離した瞬間
                    // タイムアウトを短い値に変更
                    current_timeout = MOUSE_TIMEOUT_AFTER_CLICK;
                    mouse_timer = timer_read();
                }
                break;
        }
    }
    return true; // 他のキー操作を通常通り処理
}

// 常に動いている監視関数（タイムアウト判定）
void matrix_scan_user(void) {
    if (is_mouse_layer_active) {
        if (timer_elapsed(mouse_timer) > current_timeout) {
            layer_off(MOUSE_LAYER_INDEX);
            is_mouse_layer_active = false;
            x_cumulative = 0;
            y_cumulative = 0;
        }
    }
}
