#include "app_govee.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "camera_index.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
static const char* TAG = "camera_govee";

/*
static esp_err_t capture_handler(httpd_req_t *req){
    esp_err_t res = ESP_OK;
    int64_t fr_start = esp_timer_get_time();

    fb = esp_camera_fb_get();
    if (!fb) {
        ESP_LOGE(TAG, "Camera capture failed");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");

        size_t fb_len = 0;
        if(fb->format == PIXFORMAT_JPEG){
            fb_len = fb->len;
	    ESP_LOGE("zzzzzz","yes it's jpeg\n");
            res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
        } else {
	    ESP_LOGE("zzzzzz","no it's not jpeg\n");
            jpg_chunking_t jchunk = {req, 0};
            res = frame2jpg_cb(fb, 80, jpg_encode_stream, &jchunk)?ESP_OK:ESP_FAIL;
            httpd_resp_send_chunk(req, NULL, 0);
            fb_len = jchunk.len;
        }
        esp_camera_fb_return(fb);
        int64_t fr_end = esp_timer_get_time();
        ESP_LOGI(TAG, "JPG: %uB %ums", (uint32_t)(fb_len), (uint32_t)((fr_end - fr_start)/1000));
        return res;
}

*/

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Write out data
                // printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}
const char endhead[] = "\r\n------WebKitFormBoundary8p8BYTRctIVvppjP--";
void upload_one(camera_fb_t *fb){
    char lengthStr[20];
    sprintf(lengthStr,"%d",fb->len);
    ESP_LOGE("LLLLLLLLLL"," = = %s",lengthStr);
    esp_http_client_config_t config = {
        .url = "http://192.168.1.100:8999/upload",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config); 
    esp_http_client_set_header(client, "Connection", "keep-alive"); 
    esp_http_client_set_header(client,"Content-Type", "image/jpeg"); 
    esp_http_client_set_header(client, "Content-Type","multipart/form-data; boundary=----WebKitFormBoundary8p8BYTRctIVvppjP");
    esp_http_client_set_header(client, "User-Agent", "9crk");
    esp_http_client_set_header(client, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8");
    esp_http_client_set_header(client, "Accept-Encoding", "gzip, deflate");
    esp_http_client_set_header(client, "Content-Length", lengthStr);
    esp_http_client_set_header(client,"Accept-Language","zh-CN,zh;q=0.9,en;q=0.8\r\n\r\n------WebKitFormBoundary8p8BYTRctIVvppjP");
    esp_http_client_set_header(client,"Content-Disposition", "form-data; name=\"myfile\"; filename=\"picture.jpg\"");
    esp_http_client_set_url(client, config.url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_err_t err;
    if ((err = esp_http_client_open(client, fb->len)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    	return;
    }
    esp_http_client_write(client, (char*)fb->buf, fb->len);
    esp_http_client_write(client,endhead,strlen(endhead));
    
    /*int content_length =  esp_http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER) {
		read_len = esp_http_client_read(client, buffer, content_length);
		if (read_len <= 0) {
			ESP_LOGE(TAG, "Error read data");
		}
		buffer[read_len] = 0;
		ESP_LOGI(TAG, "read_len = %d", read_len);
	}
	ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d",
*/
    esp_http_client_get_status_code(client);
    esp_http_client_get_content_length(client);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}
void upload_one_buf(uint8_t* buf,size_t len){
    char lengthStr[20];
    sprintf(lengthStr,"%d",len);
    ESP_LOGE("LLLLLLLLLL"," = = %s",lengthStr);
    esp_http_client_config_t config = {
        .url = "http://192.168.1.100:8999/upload",
        .event_handler = _http_event_handler,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config); 
    esp_http_client_set_header(client, "Connection", "keep-alive"); 
    esp_http_client_set_header(client,"Content-Type", "image/jpeg"); 
    esp_http_client_set_header(client, "Content-Type","multipart/form-data; boundary=----WebKitFormBoundary8p8BYTRctIVvppjP");
    esp_http_client_set_header(client, "User-Agent", "9crk");
    esp_http_client_set_header(client, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8");
    esp_http_client_set_header(client, "Accept-Encoding", "gzip, deflate");
    esp_http_client_set_header(client, "Content-Length", lengthStr);
    esp_http_client_set_header(client,"Accept-Language","zh-CN,zh;q=0.9,en;q=0.8\r\n\r\n------WebKitFormBoundary8p8BYTRctIVvppjP");
    esp_http_client_set_header(client,"Content-Disposition", "form-data; name=\"myfile\"; filename=\"picture.jpg\"");
    esp_http_client_set_url(client, config.url);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_err_t err;
    if ((err = esp_http_client_open(client, len)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
    	return;
    }
    esp_http_client_write(client, (char*)buf, len);
    esp_http_client_write(client,endhead,strlen(endhead));
    
    /*int content_length =  esp_http_client_fetch_headers(client);
    int total_read_len = 0, read_len;
    if (total_read_len < content_length && content_length <= MAX_HTTP_RECV_BUFFER) {
		read_len = esp_http_client_read(client, buffer, content_length);
		if (read_len <= 0) {
			ESP_LOGE(TAG, "Error read data");
		}
		buffer[read_len] = 0;
		ESP_LOGI(TAG, "read_len = %d", read_len);
	}
	ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d",
*/
    esp_http_client_get_status_code(client);
    esp_http_client_get_content_length(client);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}
int wifi_connected = 0;
void printb(uint8_t byte)
{
   int i;
   for(i=0;i<8;i++){
	if((byte >> i)&0x01)printf("1");
	else printf("0");
   }
   printf(" ");
}
void app_govee_main(){
    int i;
    camera_fb_t * fb = NULL;
    int64_t fr_start = esp_timer_get_time();
    while(1){
        fb = esp_camera_fb_get();
        fr_start = esp_timer_get_time();
        ESP_LOGE("ZZZZZ","get one size=%d t=%u\n",fb->len,(uint32_t)fr_start);
        ESP_LOGE("ZZZZZ","start upload %u\n",(uint32_t)fr_start);
	while(1){
            if(wifi_connected == 1){
	      ESP_LOGE("ZZZZZ","len = %d\n",fb->len);
	      //for(i=0;i<200;i++)printb(fb->buf[i]);
	      //printf("\n");
	      //for(i=0;i<200;i++)printb(fb->buf[640*480*2 - 201 +i]);
	      //printf("\n");
		
	      //fmt
	      uint8_t * _jpg_buf;
	      size_t _jpg_buf_len;
	      //fb->format = PIXFORMAT_YUV422SP; 
	      bool jpeg_converted = frame2jpg(fb, 90, &_jpg_buf, &_jpg_buf_len);
	      ESP_LOGE("ZZZZZ","len_jpg = %d\n",_jpg_buf_len);
	      //fb->format = PIXFORMAT_YUV422; 
              if(jpeg_converted){
	        //upload_one(fb);
		upload_one_buf(_jpg_buf,_jpg_buf_len);
		free(_jpg_buf);
	      }
	      //for(i=0;i<200;i++){fb->buf[i] = i;fb->buf[640*480*2-201+i]=i;}
	      //upload_one(fb);
	      break;
	    }
	    vTaskDelay(100); 
            ESP_LOGE("ZZZZZ","check ip\n");
	}
	vTaskDelay(100); 
        ESP_LOGE("ZZZZZ","upload done %u\n",(uint32_t)fr_start);
       
	esp_camera_fb_return(fb);
    }
}
