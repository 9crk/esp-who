#include "app_govee.h"
#include "app_wifi.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "img_converters.h"
#include "fb_gfx.h"
#include "camera_index.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "driver/rtc_io.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <esp_wifi.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>

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

static int connect_server(char* ipaddr, int port)
{   
    int sockfd = -1;
    struct sockaddr_in s_add;
        
        sockfd = socket(AF_INET, SOCK_STREAM, 0);  //创建socket套接字
    if(sockfd < 0)
    {   
        printf("socket fail ! \r\n");
        return -1;
    }
        
        bzero(&s_add,sizeof(struct sockaddr_in));
    s_add.sin_family = AF_INET;
    s_add.sin_addr.s_addr = inet_addr(ipaddr);
    s_add.sin_port = htons(port);
        
        if(connect(sockfd, (struct sockaddr *)(&s_add), sizeof(struct sockaddr)) < 0)
    {   
        printf("connect fail !\r\n");
        return -1;
    }
    
    return sockfd;
}

static int http_request_perform(int sockfd, char* ipaddr, char* data, int len)
{
    char *header = malloc(320);// = {0};
    char *form_data = malloc(256);// = {0};
    char* boundary = "---------------------------wxsstyNbsjcknlMcc903c";
    char end[64] = {0};
    int header_size = 0;
    int total_size = 0;
    static int i = 0;

    sprintf(form_data, "--%s\r\nContent-Disposition: form-data; name=\"myfile\"; filename=\"picture_%d.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n",boundary, i++);
    sprintf(end, "\r\n--%s--\r\n", boundary);
    total_size = len + strlen(form_data) + strlen(end);

    printf("strlen(boundary) = %d, strlen(end) = %d\n", strlen(boundary), strlen(end));

    header_size = sprintf(header, "POST %s HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=%s\r\ncache-control: no-cache\r\nUser-Agent: %s\r\nAccept: */*\r\nHost: %s\r\naccept-encoding: gzip, deflate\r\ncontent-length: %d\r\nConnection: close\r\n\r\n","/upload", boundary, "iHoment/1.0.0", ipaddr, total_size);

    printf("%s%s\n", header, form_data);

    send(sockfd, header, header_size, 0);
    send(sockfd, form_data, strlen(form_data), 0);
    send(sockfd, data, len, 0);
    send(sockfd, end, strlen(end), 0);

    memset(header, 0, 320);
    recv(sockfd, header, 320, 0);

    printf("\n%s\n", header);
    free(header);
    free(form_data);
    return 0;
}


int http_post_file(char* ipaddr, int port, char* data, int len)
{
    int sockfd = -1;

    sockfd = connect_server(ipaddr, port);

    http_request_perform(sockfd, ipaddr, data, len);
    close(sockfd);

    return 0;
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
    esp_http_client_set_header(client,"Content-Type", "image/jpeg\r\n"); 
    esp_http_client_set_header(client, "Content-Type","multipart/form-data; boundary=----WebKitFormBoundary8p8BYTRctIVvppjP\r\n");
    esp_http_client_set_header(client, "User-Agent", "9crk");
    esp_http_client_set_header(client, "Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n");
    esp_http_client_set_header(client, "Accept-Encoding", "gzip, deflate");
    esp_http_client_set_header(client, "Content-Length", lengthStr);
    esp_http_client_set_header(client,"Accept-Language","zh-CN,zh;q=0.9,en;q=0.8\r\n\r\n------WebKitFormBoundary8p8BYTRctIVvppjP\r\n");
    esp_http_client_set_header(client,"Content-Disposition", "form-data; name=\"myfile\"; filename=\"picture.jpg\"\r\n");
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
typedef struct jpgInfo{
uint8_t *buf;
size_t len;
}jpgInfo;
jpgInfo jpgs[5];
int n;
void led(int val){
  gpio_set_level(GPIO_NUM_4,val); 
}
void app_govee_main(){
    int i;
    n = 0;
    camera_fb_t * fb = NULL;
    int64_t fr_start;
#ifdef FAST_MODE

#endif
  gpio_pad_select_gpio(GPIO_NUM_4);
   gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
    for (i=0;i<5;i++){
	uint8_t * _jpg_buf=NULL;
        size_t _jpg_buf_len=0;
	led(1);
        fb = esp_camera_fb_get();
	led(0);
        fr_start = esp_timer_get_time();
	printf("\n\nget one %u",(unsigned int)fr_start);
	frame2jpg(fb, 90, &_jpg_buf, &_jpg_buf_len);	
        jpgs[i].buf = _jpg_buf;
	jpgs[i].len = _jpg_buf_len;	
	esp_camera_fb_return(fb);
   }
   app_wifi_main();
   while(1){
       n++; 
	if(wifi_connected == 1){
	   for(i=0;i<5;i++){
		http_post_file("192.168.43.174",8999,(char*)jpgs[i].buf,jpgs[i].len);	
           	//upload_one_buf(jpgs[i].buf,jpgs[i].len);
	   	free(jpgs[i].buf);
           }
	   break;
	}
	vTaskDelay(100); 
        ESP_LOGE("ZZZZZ","check ip\n");
	if(n > 6){
		esp_restart();
		n=0;
		esp_wifi_disconnect();
		esp_wifi_connect();
	}

   }
   ESP_LOGD("DDDDD","system done");
   gpio_pad_select_gpio(GPIO_NUM_12);
   gpio_set_direction(GPIO_NUM_12, GPIO_MODE_INPUT);
   gpio_pad_pullup(GPIO_NUM_12);
   rtc_gpio_pullup_en(GPIO_NUM_12);
   esp_sleep_enable_ext0_wakeup(GPIO_NUM_12,0);
   esp_deep_sleep_start();
}
