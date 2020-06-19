// Copyright 2017 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// buffer
#include "freeRTOS/FreeRTOS.h"
#include "freertos/ringbuf.h"

// cansu
//#include "esp_eddystone_demo.c"
//extern int beacon_main(void)


//  mqtt
#include "mdf_common.h"
#include "mesh_mqtt_handle.h"
#include "mwifi.h"

#define MEMORY_DEBUG

static const char *TAG = "mqtt_examples";


// esp_eddystone_demo
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

#include "esp_bt.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_defs.h"
#include "esp_gattc_api.h"
#include "esp_gap_ble_api.h"
#include "freeRTOS/FreeRTOS.h"
#include "freeRTOS/task.h"

#include "esp_eddystone_protocol.h"
#include "esp_eddystone_api.h"
#include "cJSON.h"



static const char* DEMO_TAG = "EDDYSTONE_DEMO";
int j = 1;
int buffercounter = 10;
uint64_t time1;
uint64_t time2;
uint64_t time_point_bf;
uint64_t time_point_nbf;

cJSON *root, *fmt;

TaskHandle_t node_write_task_handle;
TaskHandle_t node_read_task_handle;
RingbufHandle_t ringbuffer_handle;
char tx_item[200];

typedef struct Beacons {
	uint64_t UUID;
	int RSSI;
	char *ID;
	int time_stamp;
	bool arrived ;
	bool left ;
} Beacon;

Beacon beacon_arr[50];
Beacon beacon_arr_20[50];
Beacon beacon_arr_30[50];
int top=1;
int top_20=1;
int top_30=1;


/* declare static functions */
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);


static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,			.own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,	.scan_interval          = 0x50,
    .scan_window            = 0x30,							.scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

// get the timestamp in sec
int64_t xx_time_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}


static void beacon_stuctures_filtering(int k, Beacon *beacon, int RSSI,	uint64_t dev_addr, char *buffer, int counter) {
	bool unique = true;

	for (int m=0; m<k; m++){
		if (dev_addr == beacon[m].UUID){
			unique = false;
		//	printf("Not unique \n");
			break;
		}
	}

	if (unique){
		if ((RSSI>CONFIG_BLE_RSSI_THRESHOLD) && (strcmp(buffer, CONFIG_BLE_NAMESPACE_ID_FILTER)==0 )){
			j++;
			beacon[k].UUID = dev_addr;
			beacon[k].RSSI = RSSI;
			beacon[k].ID = buffer;
			beacon[k].time_stamp = counter;
			top=k+1;

			printf("'k': %d,  \n", k);
			printf("Detected Beacon ------------------------------------- \n");
			printf("'UUID': '%llx',\n", beacon[k].UUID);
			printf("'RSSI': %d,  \n", beacon[k].RSSI);
		//	printf("'Namespace ID': '%s' \n", beacon[k].ID);
			printf("'Time stamp': %d,  \n", beacon[k].time_stamp);
		}
		else {
			printf("Out of filter \n");
		}
	}
}

char *create_monitor(uint64_t mac_addr, bool detected)
{
	char buffer_mac_addr[100];
	char buffer_detection[50];
    char *string = NULL;
  //  cJSON *detection = NULL;

    cJSON *monitor = cJSON_CreateObject();

    if (monitor == NULL)
    {
        goto end;
    }

	sprintf( buffer_mac_addr, "%llx", mac_addr );
    sprintf( buffer_detection, "%d", detected );

    cJSON_AddStringToObject(monitor ,"MAC", buffer_mac_addr);
    cJSON_AddStringToObject(monitor,"room",		"triage room");
    cJSON_AddStringToObject(monitor ,"detected", buffer_detection);

    printf("JSON PRINT---------------------%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%-------------------------------------- \n");
    string = cJSON_Print(monitor);
    if (string == NULL)
    {
        fprintf(stderr, "Failed to print monitor.\n");
    }
    else{
    	//printf("%s \n", string);////// COMMENT IT !!!
        sprintf(tx_item, "%s", string);
    	//Send an item
    	UBaseType_t res =  xRingbufferSend(ringbuffer_handle, tx_item, sizeof(tx_item), pdMS_TO_TICKS(1000));
    	if (res != pdTRUE) {
    		printf("Failed to send item\n");
    	}
    }

end:
    cJSON_Delete(monitor);
    return string;
}


static void detect_arriving_beacons (){
	bool maybe_just_arrived=false;
	//printf("Detecting the arriving beacons--------------------- \n");

	for (int n=0; n<top; n++){ // for each beacon of arr10
		for (int m=0; m<top_20; m++){// compare with ten seconds before
			if (beacon_arr[n].UUID == beacon_arr_20[m].UUID){
				maybe_just_arrived=false;
				beacon_arr[n].arrived = false;
				break;// beacon is already arrived
			}
			else{
				maybe_just_arrived = true;
			}
		}
		// after checking all the indexes of beacon_arr_20, check with beacon_arr_30
		if (maybe_just_arrived == true){
			for (int x=0; x<top_30; x++){
				if (beacon_arr[n].UUID == beacon_arr_30[x].UUID){
					beacon_arr[n].arrived = false;
					break;//  in the room
				}
				else{
					beacon_arr[n].arrived = true;
				}
			}
			maybe_just_arrived=false;
			if (beacon_arr[n].arrived && (beacon_arr[n].UUID != 10)){ create_monitor(beacon_arr[n].UUID, true);}
		}
	}
}

static void detect_left_beacons (){
	bool maybe_left=false;
	printf("Detecting the leaving beacons--------------------- \n");
	for (int n=0; n<top_30; n++){ // for each beacon of arr3
		for (int m=0; m<top_20; m++){// compare with ten seconds after
			if (beacon_arr_30[n].UUID == beacon_arr_20[m].UUID){
				beacon_arr_30[n].left = false;
				maybe_left=false;
				break;// beacon is still being detected
			}
			else{
				maybe_left = true;
			}
		}

		if (maybe_left == true){
			for (int x=0; x<top; x++){
				if (beacon_arr_30[n].UUID == beacon_arr[x].UUID){
					beacon_arr_30[n].left = false;
					break;// still in the room
				}
				else{
					beacon_arr_30[n].left = true;
				}
			}
			maybe_left=false;
			if (beacon_arr_30[n].left && (beacon_arr_30[n].UUID != 10) ){ create_monitor(beacon_arr_30[n].UUID, false);}
		}
	}
}

void print_list(){

	for (int k=0; k<top; k++){
	//	if (beacon_arr[k].UUID != 10){
			printf("The LIST---------------------ARR_20 \n");
			printf("'UUID': '%llx',\n", beacon_arr[k].UUID);
		/*	printf("'RSSI': %d,  \n", beacon_arr[k].RSSI);
			printf("'Namespace ID': '%s' \n", beacon_arr[k].ID);*/
			printf("'Time stamp': %d,  \n", beacon_arr[k].time_stamp);
			printf("'arrived': %d,  \n", beacon_arr[k].arrived);
			printf("'left': %d,  \n", beacon_arr[k].left);
	//	}
	}
	for (int k=0; k<top_20; k++){
	//	if (beacon_arr_20[k].UUID != 10){
			printf("The LIST---------------------ARR_40 \n");
			printf("'UUID': '%llx',\n", beacon_arr_20[k].UUID);
		/*	printf("'RSSI': %d,  \n", beacon_arr_20[k].RSSI);
			printf("'Namespace ID': '%s' \n", beacon_arr_20[k].ID);*/
			printf("'Time stamp': %d,  \n", beacon_arr_20[k].time_stamp);
			printf("'arrived': %d,  \n", beacon_arr_20[k].arrived);
			printf("'left': %d,  \n", beacon_arr_20[k].left);
	//	}
	}
	for (int k=0; k<top_30; k++){
	//	if (beacon_arr_30[k].UUID != 10){
			printf("The LIST---------------------ARR_60 \n");
			printf("'UUID': '%llx',\n", beacon_arr_30[k].UUID);
		/*	printf("'RSSI': %d,  \n", beacon_arr_30[k].RSSI);
			printf("'Namespace ID': '%s' \n", beacon_arr_30[k].ID);*/
			printf("'Time stamp': %d,  \n", beacon_arr_30[k].time_stamp);
			printf("'arrived': %d,  \n", beacon_arr_30[k].arrived);
			printf("'left': %d,  \n", beacon_arr_30[k].left);
	//	}
	}
	
}

//create a monitor with a list of supported resolutions
//NOTE: Returns a heap allocated string, you are required to free it after use.



static void past_info(Beacon *beacon){
	printf("Shifting arrays \n");
	for(int i=0; i<top_20; i++){
		beacon_arr_30[i] = beacon_arr_20[i];
		top_30 =top_20;
	}
	for(int j=0; j<top; j++){
		beacon_arr_20[j] = beacon_arr[j];
		top_20=top;
	}
}


int counter = 0;
static void esp_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param)
{
	//printf("esp_gap_cb is called");
	counter++;
	esp_err_t err;
	uint64_t dev_addr;

	uint64_t namespace_id_1;
	uint64_t namespace_id_2;
	char buffer[150];
	char buffer_dev_addr[150];
	int RSSI;

    switch(event)
    {
        case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
			ESP_LOGI(DEMO_TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT");
            uint32_t duration = 0;
            esp_ble_gap_start_scanning(duration);
            break;
        }
        case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT: {
            if((err = param->scan_start_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(DEMO_TAG,"Scan start failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(DEMO_TAG,"Start scanning...");
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_RESULT_EVT: {
            esp_ble_gap_cb_param_t* scan_result = (esp_ble_gap_cb_param_t*)param;

            switch(scan_result->scan_rst.search_evt)
            {
                case ESP_GAP_SEARCH_INQ_RES_EVT: {
                	//	ESP_LOGI(DEMO_TAG,"ESP_GAP_SEARCH_INQ_RES_EVT");
                    esp_eddystone_result_t eddystone_res;
                    memset(&eddystone_res, 0, sizeof(eddystone_res));
                    esp_err_t ret = esp_eddystone_decode(scan_result->scan_rst.ble_adv, scan_result->scan_rst.adv_data_len, &eddystone_res);

                    time2 = xx_time_get_time();
                    if ((time2-time1)>20000){
                    	j=0;
                    	printf("'TIME DIFFFRENCE':" "%" PRIu64 "\n", time2-time1);
                    	time1 = xx_time_get_time();
						detect_left_beacons();
						detect_arriving_beacons();
						print_list();
						past_info(beacon_arr);
						beacon_stuctures_filtering(j, &beacon_arr, 0, 10, "476c6f62616c2d546167",counter); // assign our namespace id
               
                    	/*if (time2<90000){
                    		past_info(beacon_arr);
                    	}
                    	else{
            				detect_left_beacons();
            				detect_arriving_beacons();
            				print_list();
            			//	create_monitor(0,false);
            				past_info(beacon_arr);
            			}*/
                    }

                    if (ret) {
                  /*  	time_point_nbf = xx_time_get_time();
                    	if ((time_point_nbf-time_point_bf)>35000 && (time_point_nbf-time_point_bf)<120000){

                    		ESP_LOGI(DEMO_TAG, "--------NO BEACON Found----------");
                    		beacon_stuctures_filtering(0, &beacon_arr, 0, 10, "476c6f62616c2d546167",counter); // assign our namespace id

                    	}*/

                        // error:The received data is not an eddystone frame packet or a correct eddystone frame packet.
                        // just return
                        return;
                    }
					else {
						// The received adv data is a correct eddystone frame packet.
						// Here, we get the eddystone infomation in eddystone_res, we can use the data in res to do other things.
						// For example, just print them:
						ESP_LOGI(DEMO_TAG, "--------Eddystone Found----------");
						
					//	any_beacon = true;
						//esp_log_buffer_hex("EDDYSTONE_DEMO: Device address:", scan_result->scan_rst.bda, ESP_BD_ADDR_LEN);
						//ESP_LOGI(DEMO_TAG, "RSSI of packet:%d dbm", scan_result->scan_rst.rssi);
						RSSI = scan_result->scan_rst.rssi; //getting RSSI
						//getting  dev address
						dev_addr = ((uint64_t) scan_result->scan_rst.bda[0] << 40)
								| ((uint64_t) scan_result->scan_rst.bda[1] << 32)
								| ((uint64_t) scan_result->scan_rst.bda[2] << 24)
								| ((uint64_t) scan_result->scan_rst.bda[3] << 16)
								| ((uint64_t) scan_result->scan_rst.bda[4] << 8)
								| (scan_result->scan_rst.bda[5]);
						sprintf(buffer_dev_addr, "%llx", dev_addr);
						//getting namespace id
						namespace_id_1 = ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[0]   << 40)
										| ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[1]	<< 32)
										| ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[2]	<< 24)
										| ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[3]	<< 16)
										| ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[4]	<< 8)
										| ((&eddystone_res)->inform.uid.namespace_id[5]);
						namespace_id_2 =((uint64_t) (&eddystone_res)->inform.uid.namespace_id[6]    << 24)
										| ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[7]	<< 16)
										| ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[8]	<< 8)
										| ((uint64_t) (&eddystone_res)->inform.uid.namespace_id[9]);
						sprintf(buffer, "%llx%llx", namespace_id_1, namespace_id_2);
						/*if (strcmp(buffer, "476c6f62616c2d546167")==0)
                        {
						    time_point_bf = xx_time_get_time();
                        }*/
						//esp_eddystone_show_inform(&eddystone_res);
						beacon_stuctures_filtering(j, &beacon_arr, RSSI, dev_addr, &buffer,counter);
					}
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:{
            if((err = param->scan_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(DEMO_TAG,"Scan stop failed: %s", esp_err_to_name(err));
            }
            else {
                ESP_LOGI(DEMO_TAG,"Stop scan successfully");
            }
            break;
        }

        default:
            break;
    }
}



void esp_eddystone_init(void)
{
    esp_bluedroid_init();
    esp_bluedroid_enable();
    //appRegister

    esp_err_t status;
	ESP_LOGI(DEMO_TAG,"Register callback");

	/*<! register the scan callback function to the gap module */
	if((status = esp_ble_gap_register_callback(esp_gap_cb)) != ESP_OK) {
		ESP_LOGI(DEMO_TAG,"gap register error: %s", esp_err_to_name(status));
		return;
	}
	else{
		printf("The callback event was set successfully");
	}
}
void fill_arrays(void){
    beacon_arr[0].UUID = 10;
    beacon_arr[0].RSSI = -45;
    beacon_arr[0].ID =  "476c6f62616c2d546167";
    beacon_arr[0].time_stamp = 0;

    beacon_arr_20[0].UUID = 10;
    beacon_arr_20[0].RSSI = -45;
    beacon_arr_20[0].ID =  "476c6f62616c2d546167";
    beacon_arr_20[0].time_stamp = 0;

    beacon_arr_30[0].UUID = 10;
	beacon_arr_30[0].RSSI = -45;
	beacon_arr_30[0].ID =  "476c6f62616c2d546167";
	beacon_arr_30[0].time_stamp = 0;
}
void beacon_main(void)
{
	time1 = xx_time_get_time();
	time_point_nbf = xx_time_get_time();
	fill_arrays();
	print_list();
	ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_bt_controller_init(&bt_cfg);
    esp_bt_controller_enable(ESP_BT_MODE_BLE);

    esp_eddystone_init();

    /*<! set scan parameters */
    esp_ble_gap_set_scan_params(&ble_scan_params);
}

// esp_eddystone_demo end


void root_mesh2mqtt(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *buffer_item_allocated_string = NULL;
    size_t size = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = { 0x0 };
    mwifi_data_type_t data_type = { 0x0 };

    MDF_LOGI("root_mesh2mqtt is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        printf("**I AM ROOT**");
        
        if (!mesh_mqtt_is_connect()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        /**
         * @brief Recv data from node, and forward to mqtt server.
         */
        ret = mwifi_root_read(src_addr, &data_type, &buffer_item_allocated_string, &size, portMAX_DELAY);  //wait forever until receives a message in the MESH
        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mwifi_root_read", mdf_err_to_name(ret));

        ret = mesh_mqtt_write(src_addr, buffer_item_allocated_string, size, MESH_MQTT_DATA_JSON); //write the received message to mqtt

        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mesh_mqtt_publish", mdf_err_to_name(ret));

MEM_FREE:
        MDF_FREE(buffer_item_allocated_string);
    }

    MDF_LOGW("root_mesh2mqtt exit");
    mesh_mqtt_stop();
    vTaskDelete(NULL);
}

void root_read_task(void *arg)
{
    mdf_err_t ret = MDF_OK;

    MDF_LOGI("Root read task is running");

    while (mwifi_is_connected() && esp_mesh_get_layer() == MESH_ROOT) {
        if (!mesh_mqtt_is_connect()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        mesh_mqtt_data_t *request = NULL;
        mwifi_data_type_t data_type = { 0x0 };

        /**
         * @brief Recv data from mqtt data queue, and forward to special device.
         */
        ret = mesh_mqtt_read(&request, pdMS_TO_TICKS(500));

        if (ret != MDF_OK) {
            continue;
        }

        ret = mwifi_root_write(request->addrs_list, request->addrs_num, &data_type, request->data, request->size, true);
        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mwifi_root_write", mdf_err_to_name(ret));

MEM_FREE:
        MDF_FREE(request->addrs_list);
        MDF_FREE(request->data);
        MDF_FREE(request);
    }

    MDF_LOGW("Root read task is exit");
    mesh_mqtt_stop();
    vTaskDelete(NULL);
}

/**
 * @brief Print received item from the MESH
 */
static void mesh2terminal(void *arg)
{
    mdf_err_t ret = MDF_OK;
    char *data = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type = { 0x0 };
    uint8_t src_addr[MWIFI_ADDR_LEN] = { 0x0 };

    MDF_LOGI("Node read task is running");
    printf("mesh2terminal on core %d", xPortGetCoreID());

    for (;;) {

        if (!mwifi_is_connected()) {
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);  // forever, until it receives
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));
        MDF_LOGI("Received from MESH: Source Address: " MACSTR ", Size: %d, Data: %s", MAC2STR(src_addr), size, data);
		vTaskDelay(1000 / portTICK_RATE_MS);
    }
    printf("mesh2terminal on core %d finished", xPortGetCoreID());
    MDF_LOGW("mesh2terminal is exit");
    MDF_FREE(data);// The memory of data is allocated internally by mwifi_read, which needs to be released after the call.
    vTaskDelete(NULL);
}

/**
 * @brief If there is item in the ring buffer, sends the item to the mesh, being the root node the destination(NULL)
 */
static void buffer2mesh(RingbufHandle_t ringbuffer_handle)
{
    mdf_err_t ret = MDF_OK;
    size_t size = 0;
    char *buffer_item_allocated_string = NULL;
    mwifi_data_type_t data_type = { 0x0 };
    uint8_t sta_mac[MWIFI_ADDR_LEN] = { 0 };
    mesh_addr_t parent_mac = { 0 };

    MDF_LOGI("Node task is running");

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    
    printf("buffer2mesh on core %d started", xPortGetCoreID());

    for (;;) {
        if (!mwifi_is_connected() || !mwifi_get_root_status()) {  //checks: Whether the node is already connected to the parent node AND Whether the root is successfully connected to the server.
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        size_t item_size;
		
        char *ring_buffer_item = (char *)xRingbufferReceive(ringbuffer_handle, &item_size, pdMS_TO_TICKS(1000));
        //Check received item in the ring buffer
        if (ring_buffer_item != NULL) {
            //Print item
            for (int i = 0; i < item_size; i++) {
                printf("%c", ring_buffer_item[i]);
            }
            printf("\n");
            //Return Item
            //vRingbufferReturnItem(ringbuffer_handle, (void *)item);
        esp_mesh_get_parent_bssid(&parent_mac);
        size = asprintf(&buffer_item_allocated_string, "%s", ring_buffer_item);
        MDF_LOGD("Node send, size: %d, data: %s", size, buffer_item_allocated_string);
        ret = mwifi_write(NULL, &data_type, buffer_item_allocated_string, size, true);  // Send a packet to any node in the mesh network. Dest Address==NULL means root node.
        MDF_FREE(buffer_item_allocated_string);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_write", mdf_err_to_name(ret));
        } 
       /* else {
            //Failed to receive item
            printf("Item in the Ring Buffer is NULL\n");
        }*/
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
    printf("buffer2mesh on core %d finished", xPortGetCoreID());
    MDF_LOGW("Node task is exit");
    vTaskDelete(NULL);
}

/**
 * @brief Timed printing system information
 */
static void print_system_info_timercb(void *timer)
{
    uint8_t primary = 0;
    wifi_second_chan_t second = 0;
    mesh_addr_t parent_bssid = { 0 };
    uint8_t sta_mac[MWIFI_ADDR_LEN] = { 0 };
    mesh_assoc_t mesh_assoc = { 0x0 };
    wifi_sta_list_t wifi_sta_list = { 0x0 };

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    esp_wifi_vnd_mesh_get(&mesh_assoc);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("System information, channel: %d, layer: %d, self mac: " MACSTR ", parent bssid: " MACSTR
             ", parent rssi: %d, node num: %d, free heap: %u",
             primary,
             esp_mesh_get_layer(), MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),
             mesh_assoc.rssi, esp_mesh_get_total_node_num(), esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

#ifdef MEMORY_DEBUG

    if (!heap_caps_check_integrity_all(true)) {
        MDF_LOGE("At least one heap is corrupt");
    }

    //mdf_mem_print_heap();
    mdf_mem_print_record();
    //mdf_mem_print_task();
#endif /**< MEMORY_DEBUG */
}

static mdf_err_t wifi_init()
{
    mdf_err_t ret = nvs_flash_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        MDF_ERROR_ASSERT(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    MDF_ERROR_ASSERT(ret);

    tcpip_adapter_init();
    MDF_ERROR_ASSERT(esp_event_loop_init(NULL, NULL));
    MDF_ERROR_ASSERT(esp_wifi_init(&cfg));
    MDF_ERROR_ASSERT(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    MDF_ERROR_ASSERT(esp_wifi_set_mode(WIFI_MODE_STA));
    MDF_ERROR_ASSERT(esp_wifi_set_ps(WIFI_PS_NONE));
    MDF_ERROR_ASSERT(esp_mesh_set_6m_rate(false));
    MDF_ERROR_ASSERT(esp_wifi_start());

    return MDF_OK;
}

/**
 * @brief All module events will be sent to this task in esp-mdf
 *
 * @Note:
 *     1. Do not block or lengthy operations in the callback function.
 *     2. Do not consume a lot of memory in the callback function.
 *        The task memory of the callback function is only 4KB.
 */
static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            break;

        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");

            if (esp_mesh_is_root()) {
                mesh_mqtt_stop();
            }

            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE, total_num: %d", esp_mesh_get_total_node_num());

            if (esp_mesh_is_root() && mwifi_get_root_status()) {
                mdf_err_t err = mesh_mqtt_update_topo();

                if (err != MDF_OK) {
                    MDF_LOGE("Update topo failed");
                }
            }

            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");

            mesh_mqtt_start(CONFIG_MQTT_URL);

            break;
        }

        case MDF_EVENT_CUSTOM_MQTT_CONNECT:
            MDF_LOGI("MQTT connect");
            mdf_err_t err = mesh_mqtt_update_topo();

            if (err != MDF_OK) {
                MDF_LOGE("Update topo failed");
            }

            err = mesh_mqtt_subscribe();

            if (err != MDF_OK) {
                MDF_LOGE("Subscribe failed");
            }

            mwifi_post_root_status(true);

            xTaskCreate(root_mesh2mqtt, "root_mesh2mqtt", 4 * 1024,
                        NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            //xTaskCreate(root_read_task, "root_read", 4 * 1024,
            //            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL); this command is not needed since we ONLY SEND data to the root via mqtt, but not receive
            break;

        case MDF_EVENT_CUSTOM_MQTT_DISCONNECT:
            MDF_LOGI("MQTT disconnected");
            mwifi_post_root_status(false);
            break;

        default:
            break;
    }

    return MDF_OK;
}

void app_main()
{
    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config = {
        .router_ssid = CONFIG_ROUTER_SSID,
        .router_password = CONFIG_ROUTER_PASSWORD,
        .mesh_id = CONFIG_MESH_ID,
        .mesh_password = CONFIG_MESH_PASSWORD,
    };

    /**
     * @brief Set the log level for serial port printing.
     */
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    esp_log_level_set("mesh_mqtt", ESP_LOG_DEBUG);

    /**
     * @brief Initialize wifi mesh.
     */
    MDF_ERROR_ASSERT(mdf_event_loop_init(event_loop_cb));
    MDF_ERROR_ASSERT(wifi_init());
    MDF_ERROR_ASSERT(mwifi_init(&cfg));
    MDF_ERROR_ASSERT(mwifi_set_config(&config));
    MDF_ERROR_ASSERT(mwifi_start());


    /**
     * @brief Create ring buffer
     */   
    ringbuffer_handle = xRingbufferCreate(10280, RINGBUF_TYPE_NOSPLIT);
    if (ringbuffer_handle == NULL) {
        printf("Failed to create ring buffer\n");
    }
    
    /**
     * @brief Create tasks -> mesh2terminal AND buffer2mesh AND activate beacon routine
     */
    xTaskCreate(buffer2mesh, "buffer2mesh", 4 * 1024,
                ringbuffer_handle, 10, &node_write_task_handle);
   /* xTaskCreate(mesh2terminal, "mesh2terminal", 4 * 1024,
                NULL, 2, &node_read_task_handle);*/
    beacon_main();


    //TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS, true, NULL, print_system_info_timercb);                    
    //xTimerStart(timer, 0);
}