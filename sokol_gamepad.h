#ifndef SOKOL_GAMEPAD_INCLUDED
/*
sokol_gamepad.h -- cross-platform gamepad API

    Project URL: https://github.com/floooh/sokol

    Do this:
        #define SOKOL_IMPL
    before you include this file in *one* C or C++ file to create the
    implementation.

    This is a wrapper around each platform's native gamepad APIs:
    - xinput on Windows (7 and up, Win32 and UWP)
    - game controller framework on macOS/iOS/tvOS
    - input API on Android

    and abstracts gamepads to the layout of an idealized Xbox 360 gamepad:
    - one d-pad
    - two analog sticks acting as buttons when pressed
    - guide and back buttons
    - two analog shoulder buttons
    - two analog triggers
    This is the most popular and comprehensive configutration across game systems
    (PS1/PS2/PS3/PS4/Xbox/Xbox360/Xbox One/Switch map directly, just for start)

    To use:  
    - In you app initialization code call sgamepad_init()
    - At the exact time you want to record input state call sgamepad_record_state()
    - Get the state for a particular gamepad like this:
        sgamepad_gamepad_state state;
        sgamepad_get_gamepad_state(0, &state);
    
    sgamepad_gamepad_state's members are set to the contoller's state as recorded previously.

    Analog stick states are pre-processed to take dead zones into account: in most cases you should rely on
    direction_x/direction_y/magnitude for input processing.
*/

#define SOKOL_GAMEPAD_INCLUDED (1)
#include <stdint.h>
#include <stdbool.h>

#ifndef SOKOL_API_DECL
#if defined(_WIN32) && defined(SOKOL_DLL) && defined(SOKOL_IMPL)
#define SOKOL_API_DECL __declspec(dllexport)
#elif defined(_WIN32) && defined(SOKOL_DLL)
#define SOKOL_API_DECL __declspec(dllimport)
#else
#define SOKOL_API_DECL extern
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

    /* Bit flags for digital_inputs bitfield */
    typedef enum sgamepad_digital_inputs {
        SGAMEPAD_GAMEPAD_DPAD_UP     = 0x0001,
        SGAMEPAD_GAMEPAD_DPAD_DOWN   = 0x0002,
        SGAMEPAD_GAMEPAD_DPAD_LEFT   = 0x0004,
        SGAMEPAD_GAMEPAD_DPAD_RIGHT  = 0x0008,
        SGAMEPAD_GAMEPAD_START       = 0x0010,
        SGAMEPAD_GAMEPAD_BACK        = 0x0020, // Select on DualShock
        SGAMEPAD_GAMEPAD_A           = 0x0040, // X on DualShock
        SGAMEPAD_GAMEPAD_B           = 0x0080, // Circle on DualShock
        SGAMEPAD_GAMEPAD_X           = 0x0100, // Square on DualShock
        SGAMEPAD_GAMEPAD_Y           = 0x0200, // Triangle on DualShock
        SGAMEPAD_GAMEPAD_LEFT_THUMB  = 0x0400, // L3 on DualShock
        SGAMEPAD_GAMEPAD_RIGHT_THUMB = 0x0800, // R3 on DualShock
    } sgamepad_digital_inputs;

    typedef struct sgamepad_analog_stick_state {
        float normalized_x; //X component as reported by underlying API, scaled 0 to 1
        float normalized_y; //Y component as reported by underlying API, scaled 0 to 1
        float direction_x; //X component of normalized direction vector
        float direction_y; //Y component of normalized direction vector
        float magnitude;   //Normalized magnitude
    } sgamepad_analog_stick_state;

    typedef struct sgamepad_gamepad_state {
        uint16_t digital_inputs;
        sgamepad_analog_stick_state left_stick;
        sgamepad_analog_stick_state right_stick;
        float left_shoulder;
        float right_shoulder;
        float left_trigger;
        float right_trigger;
    } sgamepad_gamepad_state;

    SOKOL_API_DECL unsigned int sgamepad_get_max_supported_gamepads();

    SOKOL_API_DECL void sgamepad_init();

    SOKOL_API_DECL void sgamepad_record_state();

    SOKOL_API_DECL void sgamepad_get_gamepad_state(unsigned int index, sgamepad_gamepad_state* pstate);

#ifdef __cplusplus
} /* extern "C" */

/* reference-based equivalents for c++ */

#endif
#endif // SOKOL_GAMEPAD_INCLUDED

#ifdef SOKOL_IMPL
#define SOKOL_GAMEPAD_IMPL_INCLUDED (1)
#include <string.h> /* memset */
#include <math.h>

#ifndef SOKOL_API_IMPL
    #define SOKOL_API_IMPL
#endif
#ifndef SOKOL_DEBUG
    #ifndef NDEBUG
        #define SOKOL_DEBUG (1)
    #endif
#endif
#ifndef SOKOL_ASSERT
    #include <assert.h>
    #define SOKOL_ASSERT(c) assert(c)
#endif
#ifndef SOKOL_UNREACHABLE
    #define SOKOL_UNREACHABLE SOKOL_ASSERT(false)
#endif
#if !defined(SOKOL_CALLOC) || !defined(SOKOL_FREE)
    #include <stdlib.h>
#endif
#if !defined(SOKOL_CALLOC)
    #define SOKOL_CALLOC(n,s) calloc(n,s)
#endif
#if !defined(SOKOL_FREE)
    #define SOKOL_FREE(p) free(p)
#endif
#ifndef SOKOL_LOG
    #ifdef SOKOL_DEBUG
        #if defined(__ANDROID__)
            #include <android/log.h>
            #define SOKOL_LOG(s) { SOKOL_ASSERT(s); __android_log_write(ANDROID_LOG_INFO, "SOKOL_APP", s); }
        #else
            #include <stdio.h>
            #define SOKOL_LOG(s) { SOKOL_ASSERT(s); puts(s); }
        #endif
    #else
        #define SOKOL_LOG(s)
    #endif
#endif
#ifndef SOKOL_ABORT
    #include <stdlib.h>
    #define SOKOL_ABORT() abort()
#endif
#ifndef _SOKOL_PRIVATE
    #if defined(__GNUC__) || defined(__clang__)
        #define _SOKOL_PRIVATE __attribute__((unused)) static
    #else
        #define _SOKOL_PRIVATE static
    #endif
#endif
#ifndef _SOKOL_UNUSED
    #define _SOKOL_UNUSED(x) (void)(x)
#endif

/*== COMMON INCLUDES AND DEFINES ==================================*/

_SOKOL_PRIVATE float _sgamepad_normalize_analog_trigger(float value, float max_value, float activation_value) {
    if (value < activation_value) {
        return 0.0f;
    }

    float output = (value - activation_value) / (max_value - activation_value);
    return output;
}

_SOKOL_PRIVATE void _sgamepad_generate_analog_stick_state(float x_value, float y_value, float max_magnitude, float dead_zone_magnitude, sgamepad_analog_stick_state* pstate) {
    float magnitude = 0.0f;
    if (max_magnitude != 1.0f) {
        pstate->normalized_x = fmax(fmin(x_value/max_magnitude, 1.0f), -1.0f);
        pstate->normalized_y = fmax(fmin(y_value/max_magnitude, 1.0f), -1.0f);
        magnitude = sqrtf(x_value * x_value + y_value * y_value);
    }
    else {
        pstate->normalized_x = x_value;
        pstate->normalized_y = y_value;
        magnitude = fmin(sqrtf(x_value * x_value + y_value * y_value), 1.0f);
    }
        
    if (magnitude <= dead_zone_magnitude) {
        pstate->direction_x = 0.0f;
        pstate->direction_y = 0.0f;
        pstate->magnitude = 0.0f;
        return;
    }

    pstate->direction_x = x_value / magnitude;
    pstate->direction_y = y_value / magnitude;
    magnitude = fmin(magnitude, max_magnitude);
    pstate->magnitude = (magnitude - dead_zone_magnitude) / (max_magnitude - dead_zone_magnitude);
}

/*== PLATFORM SPECIFIC INCLUDES AND DEFINES ==================================*/
#if defined (_WIN32)
    #include <Windows.h>
    #include <Xinput.h>
    #define SGAMEPAD_MAX_SUPPORTED_GAMEPADS XUSER_MAX_COUNT

    #if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
        #pragma comment (lib, "Xinput9_1_0.lib")
    #elif WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
    #endif

#elif defined(__APPLE__)
    #define SGAMEPAD_MAX_SUPPORTED_GAMEPADS 4

    #if !defined(__cplusplus)
        #if __has_feature(objc_arc) && !__has_feature(objc_arc_fields)
            #error "sokol_app.h requires __has_feature(objc_arc_field) if ARC is enabled (use a more recent compiler version)"
        #endif
    #endif
    #include <GameController/GameController.h>

#elif defined(__ANDROID__)
    #define SGAMEPAD_MAX_SUPPORTED_GAMEPADS 4
    #include <android/input.h> 
    #include <android/keycodes.h>

    /* No access to InputManager without going through the JNI
    means no way to distinguish between devices generating events */
#else
    #define SGAMEPAD_MAX_SUPPORTED_GAMEPADS 0

#endif

typedef struct sgamepad {
    sgamepad_gamepad_state gamepad_states[SGAMEPAD_MAX_SUPPORTED_GAMEPADS];
#if defined(__ANDROID__)
    sgamepad_gamepad_state transient_gamepad_states[SGAMEPAD_MAX_SUPPORTED_GAMEPADS];
    int device_player_mappings[SGAMEPAD_MAX_SUPPORTED_GAMEPADS];
#endif
} sgamepad;

_SOKOL_PRIVATE sgamepad _sgamepad = {0};

/*== Windows Implementation ============================================*/
#if defined (_WIN32)

_SOKOL_PRIVATE void _sgamepad_record_state() {
    for (int i = 0; i < SGAMEPAD_MAX_SUPPORTED_GAMEPADS; i++)
    {
        sgamepad_gamepad_state* target = _sgamepad.gamepad_states + i;
      
        XINPUT_STATE xinput_state;
        if (XInputGetState(i, &xinput_state) != ERROR_SUCCESS)
        {
            continue;
        }

        WORD xinput_digital_buttons = xinput_state.Gamepad.wButtons;

        uint16_t new_flags = 0;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_DPAD_UP)
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_UP;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_DPAD_DOWN)
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_DOWN;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_DPAD_LEFT)
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_LEFT;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_DPAD_RIGHT)
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_RIGHT;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_START)
            new_flags |= SGAMEPAD_GAMEPAD_START;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_BACK)
            new_flags |= SGAMEPAD_GAMEPAD_BACK;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_A)
            new_flags |= SGAMEPAD_GAMEPAD_A;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_B)
            new_flags |= SGAMEPAD_GAMEPAD_B;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_X)
            new_flags |= SGAMEPAD_GAMEPAD_X;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_Y)
            new_flags |= SGAMEPAD_GAMEPAD_Y;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_LEFT_THUMB)
            new_flags |= SGAMEPAD_GAMEPAD_LEFT_THUMB;
        if (xinput_digital_buttons & XINPUT_GAMEPAD_RIGHT_THUMB)
            new_flags |= SGAMEPAD_GAMEPAD_RIGHT_THUMB;
        target->digital_inputs = new_flags;

        _sgamepad_generate_analog_stick_state(xinput_state.Gamepad.sThumbLX, xinput_state.Gamepad.sThumbLY, SHRT_MAX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, &(target->left_stick));
        _sgamepad_generate_analog_stick_state(xinput_state.Gamepad.sThumbRX, xinput_state.Gamepad.sThumbRY, SHRT_MAX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, &(target->right_stick));

        target->left_shoulder = (xinput_digital_buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 1.0f : 0.0f;
        target->right_shoulder = (xinput_digital_buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 1.0f : 0.0f;
        target->left_trigger = _sgamepad_normalize_analog_trigger(xinput_state.Gamepad.bLeftTrigger, 255, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
        target->right_trigger = _sgamepad_normalize_analog_trigger(xinput_state.Gamepad.bRightTrigger, 255, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
    }
}

/*== Apple Implementation ============================================*/
#elif defined(__APPLE__)

_SOKOL_PRIVATE void _sgamepad_record_state() {
    int target_index = 0;
    for (GCController* controller in GCController.controllers) {
        GCExtendedGamepad* extended_gamepad = controller.extendedGamepad;
        if (extended_gamepad == nil) {
            continue;
        }
        
        sgamepad_gamepad_state* target = _sgamepad.gamepad_states + target_index;
        target_index++;
        
        uint16_t new_flags = 0;
        if (extended_gamepad.dpad.up.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_UP;
        }
        if (extended_gamepad.dpad.down.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_DOWN;
        }
        if (extended_gamepad.dpad.left.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_LEFT;
        }
        if (extended_gamepad.dpad.right.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_DPAD_RIGHT;
        }
        if (extended_gamepad.buttonMenu.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_START;
        }
        if (extended_gamepad.buttonOptions.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_BACK;
        }
        if (extended_gamepad.buttonA.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_A;
        }
        if (extended_gamepad.buttonB.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_B;
        }
        if (extended_gamepad.buttonX.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_X;
        }
        if (extended_gamepad.buttonY.pressed) {
            new_flags |= SGAMEPAD_GAMEPAD_Y;
        }
        target->digital_inputs = new_flags;
        
        _sgamepad_generate_analog_stick_state(extended_gamepad.leftThumbstick.xAxis.value, extended_gamepad.leftThumbstick.yAxis.value, 1.0f, 0.01f, &(target->left_stick));
        _sgamepad_generate_analog_stick_state(extended_gamepad.rightThumbstick.xAxis.value, extended_gamepad.rightThumbstick.yAxis.value, 1.0f, 0.01f, &(target->right_stick));
        
        target->left_shoulder = extended_gamepad.leftShoulder.value;
        target->right_shoulder = extended_gamepad.rightShoulder.value;
        target->left_trigger = extended_gamepad.leftTrigger.value;
        target->right_trigger = extended_gamepad.rightTrigger.value;
    }
}

/*== Android Implementation ============================================*/
#elif defined(__ANDROID__)

_SOKOL_PRIVATE int _sgamepad_android_get_player_id(const AInputEvent* event) {
    int32_t source_type = AInputEvent_getSource(event);
    if (!(source_type & (AINPUT_SOURCE_DPAD | AINPUT_SOURCE_GAMEPAD | AINPUT_SOURCE_JOYSTICK))) {
        return -1;
    }

    int device_id = AInputEvent_getDeviceId(event);
    for (int i=0; i<SGAMEPAD_MAX_SUPPORTED_GAMEPADS; i++) {
        if (_sgamepad.device_player_mappings[i] == device_id) {
            return i;
        }
    }
    for (int i=0; i<SGAMEPAD_MAX_SUPPORTED_GAMEPADS; i++) {
        if (_sgamepad.device_player_mappings[i] == 0) {
            _sgamepad.device_player_mappings[i] = device_id;
            return i;
        }
    }
    return -1;
}

_SOKOL_PRIVATE bool _sgamepad_android_map_keycode_to_button(uint32_t key_code, sgamepad_digital_inputs* out_mapped_input) {
    bool mapped = true;
    switch (key_code) {
        case AKEYCODE_DPAD_UP:
        *out_mapped_input = SGAMEPAD_GAMEPAD_DPAD_UP;
        break;
        case AKEYCODE_DPAD_DOWN:
        *out_mapped_input = SGAMEPAD_GAMEPAD_DPAD_DOWN;
        break;
        case AKEYCODE_DPAD_LEFT:
        *out_mapped_input = SGAMEPAD_GAMEPAD_DPAD_LEFT;
        break;
        case AKEYCODE_DPAD_RIGHT:
        *out_mapped_input = SGAMEPAD_GAMEPAD_DPAD_RIGHT;
        break;
        case AKEYCODE_BUTTON_START:
        *out_mapped_input = SGAMEPAD_GAMEPAD_START;
        break;
        case AKEYCODE_BUTTON_SELECT:
        *out_mapped_input = SGAMEPAD_GAMEPAD_BACK;
        break;
        case AKEYCODE_BUTTON_A:
        *out_mapped_input = SGAMEPAD_GAMEPAD_A;
        break;
        case AKEYCODE_BUTTON_B:
        *out_mapped_input = SGAMEPAD_GAMEPAD_B;
        break;
        case AKEYCODE_BUTTON_X:
        *out_mapped_input = SGAMEPAD_GAMEPAD_X;
        break;
        case AKEYCODE_BUTTON_Y:
        *out_mapped_input = SGAMEPAD_GAMEPAD_Y;
        break;
        case AKEYCODE_BUTTON_THUMBL:
        *out_mapped_input = SGAMEPAD_GAMEPAD_LEFT_THUMB;
        break;
        case AKEYCODE_BUTTON_THUMBR:
        *out_mapped_input = SGAMEPAD_GAMEPAD_RIGHT_THUMB;
        break;
        default:
        mapped = false;
        break;
    }
    
    return mapped;
}

_SOKOL_PRIVATE bool _sgamepad_android_map_keycode_to_shoulder(uint32_t key_code, sgamepad_gamepad_state* target, float** out_mapped_input) {
    bool mapped = true;
    switch (key_code) {
        case AKEYCODE_BUTTON_L1:
        *out_mapped_input = &(target->left_shoulder);
        break;
        case AKEYCODE_BUTTON_R1:
        *out_mapped_input = &(target->right_shoulder);
        break;
        default:
        mapped = false;
        break;
    }

    return mapped;
}

_SOKOL_PRIVATE bool _sgamepad_android_key_handler(const AInputEvent* event) {    
    int player_id = _sgamepad_android_get_player_id(event);
    if (player_id < 0 || player_id >= SGAMEPAD_MAX_SUPPORTED_GAMEPADS) {
        return false;
    }

    sgamepad_gamepad_state* target = _sgamepad.transient_gamepad_states + player_id;

    uint32_t key_code = AKeyEvent_getKeyCode(event);
    uint32_t key_action = AKeyEvent_getAction(event);

    static sgamepad_digital_inputs mapped_button_input;
    if (_sgamepad_android_map_keycode_to_button(key_code, &mapped_button_input)) {
        switch (key_action) {
            case AKEY_EVENT_ACTION_DOWN:
            if(AKeyEvent_getRepeatCount(event) == 0) {
                target->digital_inputs |= mapped_button_input;
            }
            return true;
            case AKEY_EVENT_ACTION_UP:
            target->digital_inputs &= (~mapped_button_input);
            return true;
        }
    }

    static float* mapped_shoulder_input;
    if (_sgamepad_android_map_keycode_to_shoulder(key_code, target, &mapped_shoulder_input)) {
        switch (key_action) {
            case AKEY_EVENT_ACTION_DOWN:
            if(AKeyEvent_getRepeatCount(event) == 0) {
                *mapped_shoulder_input = 1.0f;
            }
            return true;
            case AKEY_EVENT_ACTION_UP:
            *mapped_shoulder_input = 0.0f;
            return true;
        }
    }

    return false;
}

_SOKOL_PRIVATE bool _sgamepad_android_motion_handler(const AInputEvent* event) {
    int player_id = _sgamepad_android_get_player_id(event);
    if (player_id < 0 || player_id >= SGAMEPAD_MAX_SUPPORTED_GAMEPADS) {
        return false;
    }

    if (AMotionEvent_getAction(event) != AMOTION_EVENT_ACTION_MOVE) {
        return false;
    }

    sgamepad_gamepad_state* target = _sgamepad.transient_gamepad_states + player_id;

    /* D-Pad mapped to analog */
    float d_pad_sample = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_X, 0);
    if (d_pad_sample < 0) {
        target->digital_inputs |= SGAMEPAD_GAMEPAD_DPAD_LEFT;
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_RIGHT);
    }
    else if(d_pad_sample > 0) {
        target->digital_inputs |= SGAMEPAD_GAMEPAD_DPAD_RIGHT;
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_LEFT);
    }
    else {
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_LEFT);
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_RIGHT);
    }
    d_pad_sample = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_HAT_Y, 0);
    if (d_pad_sample < 0) {
        target->digital_inputs |= SGAMEPAD_GAMEPAD_DPAD_UP;
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_DOWN);
    }
    else if(d_pad_sample > 0) {
        target->digital_inputs |= SGAMEPAD_GAMEPAD_DPAD_DOWN;
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_UP);
    }
    else {
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_UP);
        target->digital_inputs &= (~SGAMEPAD_GAMEPAD_DPAD_DOWN);
    }

    /* Analog sticks */
    float stick_x, stick_y;
    stick_x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);
    stick_y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0);
    _sgamepad_generate_analog_stick_state(stick_x, -stick_y, 1.0f, 0.01f, &(target->left_stick));
        
    stick_x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0);
    stick_y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0);
    _sgamepad_generate_analog_stick_state(stick_x, -stick_y, 1.0f, 0.01f, &(target->right_stick));

    target->left_trigger = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_LTRIGGER, 0);
    target->right_trigger = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RTRIGGER, 0);

    return false;
}

_SOKOL_PRIVATE bool _sgamepad_android_input_handler(const AInputEvent* event) {
    switch(AInputEvent_getType(event)) {
        case AINPUT_EVENT_TYPE_KEY:
        return _sgamepad_android_key_handler(event);
        case AINPUT_EVENT_TYPE_MOTION:
        return _sgamepad_android_motion_handler(event);
        default:
        return false;
    }
}

_SOKOL_PRIVATE void _sgamepad_android_init() {
    _sapp.android.gamepad_event_handler = _sgamepad_android_input_handler;
}

_SOKOL_PRIVATE void _sgamepad_record_state() {
    for(int i=0; i<SGAMEPAD_MAX_SUPPORTED_GAMEPADS; i++) {
        _sgamepad.gamepad_states[i] = _sgamepad.transient_gamepad_states[i];
    }
}

/*== Fallback dummy Implementation ============================================*/
#else

_SOKOL_PRIVATE void _sgamepad_record_state() {
}

#endif

/*=== PUBLIC API FUNCTIONS ===================================================*/
SOKOL_API_IMPL unsigned int sgamepad_get_max_supported_gamepads() {
    return SGAMEPAD_MAX_SUPPORTED_GAMEPADS;
}

SOKOL_API_IMPL void sgamepad_init() {
#if defined(__ANDROID__)
    _sgamepad_android_init();
#endif
}

SOKOL_API_IMPL void sgamepad_record_state() {
    memset(_sgamepad.gamepad_states, 0, sizeof(_sgamepad.gamepad_states));
    _sgamepad_record_state();
}

SOKOL_API_IMPL void sgamepad_get_gamepad_state(unsigned int index, sgamepad_gamepad_state* pstate) {
    if (index < SGAMEPAD_MAX_SUPPORTED_GAMEPADS) {
        *pstate = _sgamepad.gamepad_states[index];
    }
}

#endif /* SOKOL_IMPL */
