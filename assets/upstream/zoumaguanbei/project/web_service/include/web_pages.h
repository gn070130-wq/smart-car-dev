#ifndef WEB_PAGES_H
#define WEB_PAGES_H

#include <string>

namespace web_pages {

// Shared presentation used by all embedded web interfaces.
std::string common_style();

// One renderer per browser interface.
std::string home_html();
std::string capture_html();
std::string camera_settings_html();
std::string model_test_html();

} // namespace web_pages

#endif // WEB_PAGES_H
