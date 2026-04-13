#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <WebServer.h>
#include <functional>
#include "config.h"

typedef std::function<void()> ConfigSavedCallback;

// Register web portal routes on the given server.
// onSave is called after configuration is saved or reset.
void webPortalSetup(WebServer &server, AppConfig &cfg, ConfigSavedCallback onSave = nullptr);

#endif // WEB_PORTAL_H
