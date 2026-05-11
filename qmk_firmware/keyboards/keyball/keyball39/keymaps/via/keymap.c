#include QMK_KEYBOARD_H

/* ==========================================================================
   1. オートマウスレイヤー(AML) の設定値
   ========================================================================== */
#define MOUSE_LAYER_INDEX 2              
#define AUTO_MOUSE_LAYER_KEEP_TIME 30000 
#define MOUSE_TIMEOUT_AFTER_CLICK 500    
#define AML_ACTIVATE_THRESHOLD 10        

uint16_t aml_timer = 0;
uint16_t aml_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
bool aml_active = false;      
bool aml_enable_sw = true;    
static int16_t aml_x = 0;
static int16_t aml_y = 0;

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* ==========================================================================
   2. キーマップ定義 (LAYOUT_universal)
   ========================================================================== */
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [0] = LAYOUT_universal(
    KC_Q     , KC_W     , KC_E     , KC_R     , KC_T     ,                            KC_Y     , KC_U     , KC_I     , KC_O     , KC_P     ,
    KC_A     , KC_S     , KC_D     , KC_F     , KC_G     ,                            KC_H     , KC_J     , KC_K     , KC_L     , KC_MINS  ,
    KC_Z     , KC_X     , KC_C     , KC_V     , KC_B     ,                            KC_N     , KC_M     , KC_COMM  , KC_DOT   , KC_SLSH  ,
    KC_LCTL  , KC_LALT  , KC_LGUI  , KC_SPC   , MO(1)    , KC_BSPC  ,      _______  , MO(3)    , KC_ENT   , KC_TAB   , KC_RSFT  , KC_LSFT
  ),

  [1] = LAYOUT_universal(
    KC_ESC   , KC_7     , KC_8     , KC_9     , KC_PLUS  ,                            KC_CIRC  , KC_AMPR  , KC_ASTR  , KC_LPRN  , KC_RPRN  ,
    KC_LSFT  , KC_4     , KC_5     , KC_6     , KC_MINS  ,                            KC_HOME  , KC_UP    , KC_END   , KC_PGUP  , KC_BSLS  ,
    KC_LCTL  , KC_1     , KC_2     , KC_3     , KC_ENT   ,                            KC_LEFT  , KC_DOWN  , KC_RGHT  , KC_PGDN  , KC_GRV   ,
    _______  , _______  , KC_0     , KC_DOT   , _______  , KC_BSPC  ,      _______  , _______  , _______  , _______  , _______  , _______
  ),

  [2] = LAYOUT_universal(
    KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_F5    ,                            KC_F6    , KC_F7    , KC_F8    , KC_F9    , KC_F10   ,
    KC_LSFT  , _______  , KC_BTN3  , _______  , KC_BSPC  ,                            S(KC_9)  , KC_BTN1  , KC_UP    , KC_BTN2  , KC_QUOT  ,
    KC_SLSH  , KC_1     , KC_2     , KC_3     , S(KC_MINS),                           S(KC_NUHS), KC_LEFT  , KC_DOWN  , KC_RGHT  , _______  ,
    KC_ESC   , KC_0     , KC_DOT   , KC_DEL   , KC_ENT   , KC_BSPC  ,      _______  , _______  , _______  , _______  , _______  , _______
  ),

  [3] = LAYOUT_universal(
    RGB_TOG  , AML_TO   , AML_I50  , AML_D50  , _______  ,                            _______  , _______  , _______  , _______  , _______  ,
    RGB_MOD  , RGB_HUI  , RGB_SAI  , RGB_VAI  , SCRL_DVI ,                            _______  , _______  , _______  , _______  , _______  ,
    RGB_RMOD , RGB_HUD  , RGB_SAD  , RGB_VAD  , SCRL_DVD ,                            CPI_D1K  , CPI_D100 , CPI_I100 , CPI_I1K  , KBC_SAVE ,
    QK_BOOT  , KBC_RST  , _______  , _______  , _______  , _______  ,      _______  , _______  , _______  , _______  , _______  , _______
  )
};

/* ==========================================================================
   3. ロジック実装部 (AML)
   ========================================================================== */

void report_mouse_user(report_mouse_t* mouse_report) {
    if (!aml_enable_sw) return;

    if (mouse_report->x != 0 || mouse_report->y != 0) {
        if (!aml_active) {
            aml_x += mouse_report->x;
            aml_y += mouse_report->y;
            if (ABS(aml_x) > AML_ACTIVATE_THRESHOLD || ABS(aml_y) > AML_ACTIVATE_THRESHOLD) {
                layer_on(MOUSE_LAYER_INDEX);
                aml_active = true;
                aml_x = 0; aml_y = 0;
            }
        }
        if (aml_active) {
            aml_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
            aml_timer = timer_read();
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case AML_TO:
            if (record->event.pressed) { aml_enable_sw = !aml_enable_sw; }
            break;
        case KC_MS_BTN1 ... KC_MS_BTN5:
            if (aml_active && !record->event.pressed) {
                aml_timeout = MOUSE_TIMEOUT_AFTER_CLICK;
                aml_timer = timer_read();
            }
            break;
    }
    return true; 
}

void matrix_scan_user(void) {
    if (aml_active) {
        if (timer_elapsed(aml_timer) > aml_timeout) {
            layer_off(MOUSE_LAYER_INDEX);
            aml_active = false;
            aml_x = 0; aml_y = 0;
        }
    }
}

/* ==========================================================================
   4. OLED 表示設定 (ここを追加しました)
   ========================================================================== */
#ifdef OLED_ENABLE
bool oled_task_user(void) {
    // USBケーブルが刺さっている側（Master）の表示
    if (is_keyboard_master()) {
        oled_write_P(PSTR("Layer: "), false);
        switch (get_highest_layer(layer_state)) {
            case 0:  oled_write_ln_P(PSTR("Default"), false); break;
            case 1:  oled_write_ln_P(PSTR("Lower  "), false); break;
            case 2:  oled_write_ln_P(PSTR("MOUSE  "), false); break; // AML起動中
            case 3:  oled_write_ln_P(PSTR("Raise  "), false); break;
            default: oled_write_ln_P(PSTR("Unknown"), false); break;
        }

        // オートマウス機能の状態
        oled_write_P(aml_enable_sw ? PSTR("AML: ON \n") : PSTR("AML: OFF\n"), false);
        
        // 最後に押したキー（デバッグ用）
        oled_write_P(PSTR("Keyball39 "), false);
    } 
    // 反対側（Slave）の表示
    else {
        oled_write_ln_P(PSTR("Keyball39"), false);
        oled_write_ln_P(PSTR("  v0.1  "), false);
    }
    return false;
}
#endif
