#include QMK_KEYBOARD_H

/* ==========================================================================
   1. オートマウスレイヤー(AML) の設定値
   ========================================================================== */
#define MOUSE_LAYER_INDEX 2              // マウスレイヤーの番号
#define AUTO_MOUSE_LAYER_KEEP_TIME 30000 // 通常の維持時間（30秒）
#define MOUSE_TIMEOUT_AFTER_CLICK 500    // クリック後の維持時間（0.5秒）
#define AML_ACTIVATE_THRESHOLD 10        // 起動しきい値（誤爆防止）

// 状態管理変数
uint16_t aml_timer = 0;
uint16_t aml_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
bool aml_active = false;      // 現在マウスレイヤーが有効か
bool aml_enable_sw = true;    // オートマウス機能自体のON/OFF（AML_TOキーで操作）
static int16_t aml_x = 0;
static int16_t aml_y = 0;

#define ABS(x) ((x) < 0 ? -(x) : (x))

/* ==========================================================================
   2. キーマップ定義 (ご提示の universal レイアウトを完全維持)
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
   3. ロジック実装部 (オートマウス機能)
   ========================================================================== */

// ボールの動きを検知
void report_mouse_user(report_mouse_t* mouse_report) {
    // 1. システム側のAML状態を取得（AML_TOキーと連動させるため）
    // もしシステム側で無効化されていたら何もしない
    if (!aml_enable_sw) return;

    if (mouse_report->x != 0 || mouse_report->y != 0) {
        if (!aml_active) {
            aml_x += mouse_report->x;
            aml_y += mouse_report->y;
            // 4. 指定のしきい値を超えた時だけ起動
            if (ABS(aml_x) > AML_ACTIVATE_THRESHOLD || ABS(aml_y) > AML_ACTIVATE_THRESHOLD) {
                layer_on(MOUSE_LAYER_INDEX);
                aml_active = true;
                aml_x = 0; aml_y = 0;
            }
        }
        
        if (aml_active) {
            // 3. 動かしている間は常に30秒にリセット
            aml_timeout = AUTO_MOUSE_LAYER_KEEP_TIME;
            aml_timer = timer_read();
        }
    }
}

// キー操作（クリック後の離脱処理 & AML_TOの連動）
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case AML_TO:
            if (record->event.pressed) {
                aml_enable_sw = !aml_enable_sw;
            }
            break;
        case KC_MS_BTN1 ... KC_MS_BTN5:
            if (aml_active && !record->event.pressed) {
                // 2. マウスボタンを離した瞬間、0.5秒のクイック離脱モードへ
                aml_timeout = MOUSE_TIMEOUT_AFTER_CLICK;
                aml_timer = timer_read();
            }
            break;
    }
    return true; 
}

// 常に動く監視処理（タイムアウトの実行）
void matrix_scan_user(void) {
    if (aml_active) {
        if (timer_elapsed(aml_timer) > aml_timeout) {
            layer_off(MOUSE_LAYER_INDEX);
            aml_active = false;
            aml_x = 0; aml_y = 0;
        }
    }
}
