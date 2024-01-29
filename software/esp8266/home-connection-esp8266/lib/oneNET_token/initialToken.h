#ifndef INITIALTOKEN_H
#define INITIALTOKEN_H

//使能自动注册,屏蔽 则不使用自动注册功能
/**
 * @brief 启动自动注册功能，不适用注释掉即可
 *
 */
// #define ONENET_DEVICE_DYNAMIC_ENABLE 
 /**
  * @brief oneNET 的产品ID
  *
  */
#define ONENET_PRODUCT_ID "NF8Nx"  //产品ID

#define ONENET_DEVICE_NAME "desktop_led"  //设备名字

#ifdef ONENET_DEVICE_DYNAMIC_ENABLE
  /**
   * @brief oneNET 的产品密钥
   *
   */
#define ONENET_PRODUCT_KEY "" //产品KEY

#else
  /**
   * @brief oneNET 的设备密钥 不启用自动注册功能请填入设备key
   *
   */
#define ONENET_DEVICE_KEY "MN5OHU=" //设备KEY

#endif

typedef enum {
    ONENET_METHOD_MD5 = 0,
    ONENET_METHOD_SHA1,
    ONENET_METHOD_SHA256,
}method_t;

typedef struct ONENET_CONNECT_MSG
{
    char produt_id[32];
    char device_name[32];
    char token[512];
}oneNET_connect_msg_t;

uint8_t onenet_connect_msg_init(oneNET_connect_msg_t* oneNET_connect_msg, method_t token_method);

#endif