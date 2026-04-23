#pragma once

void http_api_init();
void http_api_tick();   // call from loop() to handle pending requests
