#include "navigation.hpp"
#include "screens.hpp"
#include "../quick_controls_screen.h"
namespace sim {
void initialize_navigation(){quick_controls_screen_create();}
void show_quick_controls(){leave_screen();quick_controls_screen_show();}
bool quick_controls_active(){return quick_controls_screen_is_active();}
void refresh_quick_controls(){quick_controls_screen_refresh();}
}
