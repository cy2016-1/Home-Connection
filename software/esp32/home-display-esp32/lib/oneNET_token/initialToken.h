#ifndef INITIALTOKEN_H
#define INITIALTOKEN_H

//ʹ���Զ�ע��,���� ��ʹ���Զ�ע�Ṧ��
/**
 * @brief �����Զ�ע�Ṧ�ܣ�������ע�͵�����
 *
 */
// #define ONENET_DEVICE_DYNAMIC_ENABLE 
 /**
  * @brief oneNET �Ĳ�ƷID
  *
  */
#define ONENET_PRODUCT_ID ""  //��ƷID

#define ONENET_DEVICE_NAME ""  //�豸����

#ifdef ONENET_DEVICE_DYNAMIC_ENABLE
  /**
   * @brief oneNET �Ĳ�Ʒ��Կ
   *
   */
#define ONENET_PRODUCT_KEY "" //��ƷKEY

#else
  /**
   * @brief oneNET ���豸��Կ �������Զ�ע�Ṧ���������豸key
   *
   */
#define ONENET_DEVICE_KEY "" //�豸KEY

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

uint8_t onenet_connect_msg_init(oneNET_connect_msg_t* oneNET_connect_msg, method_t token_method, const char *mqtt_product_id, const char *mqtt_device_name, const char *mqtt_device_key);

#endif