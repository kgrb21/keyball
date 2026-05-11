#include QMK_KEYBOARD_H

/* ==========================================================================
   1. オートマウスレイヤー(AML) の設定値
   ========================================================================== */
#define MOUSE_LAYER_INDEX 2              // マウスレイヤーの番号
#define AUTO_MOUSE_LAYER_KEEP_TIME 30000 // 通常の維持時間（30秒）
#define MOUSE_TIMEOUT_AFTER_CLICK 500    // クリック後の維持時間（0.5秒）
#define AML_ACTIVATE_THRESHOLD 10        // 起動しきい値（意図しない接触を防止）

// 内部管理用変数
uint16_t mouse_timer = 0;
uint16_t current_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
bool is_mouse_layer_active = false;
static int16_t x_cumulative = 0;
static int16_t y_cumulative = 0;

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* ==========================================================================
   2. キーマップ定義
   ※Remapで変更可能ですが、ビルド用に標準配置を記述しています。
   ========================================================================== */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        KC_TAB,   KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,                         KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,    KC_BSPC,
        KC_LSFT,  KC_A,    KC_S,    KC_D,    KC_F,    KC_G,                         KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN, KC_QUOT,
        KC_LCTL,  KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,    KC_LBRC,    KC_RBRC, KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH, KC_RSFT,
                                    KC_LGUI, KC_SPC,  MO(1),                        MO(3),   KC_ENT,  KC_RALT
    ),
    [1] = LAYOUT(
        KC_ESC,   KC_1,    KC_2,    KC_3,    KC_4,    KC_5,                         KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_DEL,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                      KC_LEFT, KC_DOWN, KC_UP,   KC_RGHT, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [2] = LAYOUT(
        // ここがオートマウスレイヤー
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, KC_WH_L, KC_WH_U, KC_WH_D, KC_WH_R, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, KC_BTN1, KC_BTN3, KC_BTN2, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, KC_TRNS, KC_TRNS
    ),
    [3] = LAYOUT(
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
        KC_TRNS,  KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,    KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS,
                                    KC_TRNS, KC_TRNS, KC_TRNS,                      KC_TRNS, KC_TRNS, KC_TRNS
    )
};

/* ==========================================================================
   3. ロジック実装部 (ここが重要です)
   ========================================================================== */

// トラックボールが動いた時の処理
void report_mouse_user(report_mouse_t* mouse_report) {
    if (mouse_report->x != 0 || mouse_report->y != 0) {
        if (!is_mouse_layer_active) {
            // しきい値の判定
            x_cumulative += mouse_report->x;
            y_cumulative += mouse_report->y;
            if (ABS(x_cumulative) > AML_ACTIVATE_THRESHOLD || ABS(y_cumulative) > AML_ACTIVATE_THRESHOLD) {
                layer_on(MOUSE_LAYER_INDEX);
                is_mouse_layer_active = true;
                x_cumulative = 0;
                y_cumulative = 0;
            }
        }
        
        if (is_mouse_layer_active) {
            // 動かしている間はタイムアウトを30秒にリセット
            current_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
            mouse_timer = timer_read();
        }
    }
}

// キー入力があった時の処理 (クリック後の挙動制御)
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (is_mouse_layer_active) {
        // マウスボタンが離された瞬間を検知
        if (keycode >= KC_MS_BTN1 && keycode <= KC_MS_BTN5) {
            if (!record->event.pressed) {
                // タイムアウトを短い時間（0.5秒）に変更
                current_timeout = MOUSE_TIMEOUT_AFTER_CLICK;
                mouse_timer = timer_read();
            }
        }
    }
    return true; 
}

// 常に動作する処理 (タイムアウトの監視)
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

/* ==========================================================================
   4. OLEDディスプレイ表示 (オプション)
   ========================================================================== */
#ifdef OLED_ENABLE
bool oled_task_user(void) {
    // 現在のレイヤー表示
    oled_write_P(PSTR("Layer: "), false);
    switch (get_highest_layer(layer_state)) {
        case 0: oled_write_ln_P(PSTR("Default"), false); break;
        case 1: oled_write_ln_P(PSTR("Lower"), false); break;
        case 2: oled_write_ln_P(PSTR("MOUSE"), false); break;
        case 3: oled_write_ln_P(PSTR("Raise"), false); break;
        default: oled_write_ln_P(PSTR("LayerX"), false);
    }
    // AMLの状態表示
    oled_write_P(is_mouse_layer_active ? PSTR("AML: ON \n") : PSTR("AML: OFF\n"), false);
    return false;
}
#endif
