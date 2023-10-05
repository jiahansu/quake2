#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool R_Window_setup();
bool R_Window_createContext();
void R_Window_finalize();
bool R_Window_update(bool forceFlag);
void R_Frame_end();


#ifdef __cplusplus
}
#endif