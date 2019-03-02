/* ESPRESSIF MIT License
 * 
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 * 
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "app_camera.h"
#include "app_wifi.h"
//#include "app_httpd.h"
#include "app_govee.h"
#include "esp_timer.h"
#include "esp_log.h"
void app_main()
{
    int64_t fr_start = esp_timer_get_time();
    ESP_LOGE("ZZZZZ","mainmmmmmmmmmmmmmmmmmmmmme %u\n",(uint32_t)fr_start);
    app_camera_main();
    fr_start = esp_timer_get_time();
    ESP_LOGE("ZZZZZ","camera ok %u\n",(uint32_t)fr_start);
    app_wifi_main();
    fr_start = esp_timer_get_time();
    ESP_LOGE("ZZZZZ","wifi ok%u\n",(uint32_t)fr_start);
//    app_httpd_main();
    app_govee_main();
}
