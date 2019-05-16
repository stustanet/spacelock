#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*callback_type)();
void add_systick_callback(callback_type cb);

void on_systick();

#ifdef __cplusplus
}
#endif