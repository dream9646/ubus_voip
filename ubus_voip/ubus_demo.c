#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <uci.h>
#include <libubus.h>



#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
 
#include <libubus.h>
 
#include <libubox/uloop.h>
#include <libubox/list.h>
#include <libubox/blobmsg_json.h>
#include <json-c/json.h>


// .name是参数名称，.type是参数的数据类型
#define PACKAGE_NAME 0
#define SECTION_NAME 1
#define OPTION_NAME 2
#define __UCI_CMD_MAX 3
#define __UCI_GET_MAX 3

struct ubus_context *ctx;
struct blob_buf b;
static const struct blobmsg_policy uci_get_policy[__UCI_CMD_MAX] = {
    [PACKAGE_NAME] = {.name = "package", .type = BLOBMSG_TYPE_STRING},
    [SECTION_NAME] = {.name = "section", .type = BLOBMSG_TYPE_STRING},
    [OPTION_NAME] = {.name = "option", .type = BLOBMSG_TYPE_STRING}
};	
// * get对应的处理方法
static int get_uci(struct ubus_context *ctx, struct ubus_object *obj,
                     struct ubus_request_data *req, const char *method,
                     struct blob_attr *msg)
{
    char buf[2048] = {0};
    char value[512] = {0};
    char package_name[512] = {0};
    char section_name[512] = {0};
    char option_name[512] = {0};
    struct blob_attr *param[__UCI_GET_MAX] = {0};
    struct uci_context *uci_ctx = uci_alloc_context();	// 分析uci上下文内容
    struct uci_package *pkg = NULL;
    struct uci_section *sec = NULL;
    struct uci_option *opt = NULL;
    struct uci_ptr ptr = {0};
    struct blob_buf b_buf = {0};

    // * 解析传入的参数
    blobmsg_parse(uci_get_policy, ARRAY_SIZE(uci_get_policy), param, blob_data(msg), blob_len(msg));

    // * 处理过程
    if (uci_ctx == NULL) {
        return -1;
    }
    snprintf(package_name, sizeof(package_name), "%s", blobmsg_get_string(param[PACKAGE_NAME]));
    snprintf(section_name, sizeof(section_name), "%s", blobmsg_get_string(param[SECTION_NAME]));
    snprintf(option_name, sizeof(option_name), "%s", blobmsg_get_string(param[OPTION_NAME]));

    if (uci_load(uci_ctx, package_name, &pkg) != UCI_OK) {	// 加载配置文件到pkg中
        snprintf(package_name, sizeof(package_name), "package no exist");
        goto RESPONSE;
    }
    sec = uci_lookup_section(uci_ctx, pkg, section_name);	// 读取pkg中的配置节到sec中
    if (sec == NULL) {
        snprintf(section_name, sizeof(section_name), "section no exist");
        goto RESPONSE;
    }
    opt = uci_lookup_option(uci_ctx, sec, option_name);	// 读取sec中的选项内容到opt中
    if (opt == NULL) {
        snprintf(option_name, sizeof(option_name), "option no exist");
        goto RESPONSE;
    }

    // 根据buf读取内容到ptr中
    snprintf(buf, sizeof(buf), "%s.%s.%s", package_name, section_name, option_name);
    if (uci_lookup_ptr(uci_ctx, &ptr, buf, true) == UCI_OK) {	// 读取选项中的值到ptr中
        strncpy(value, ptr.o->v.string, sizeof(value) - 1);
    }
    
RESPONSE:
    uci_free_context(uci_ctx);	// 释放uci上下文内容

    // * 响应过程
    blob_buf_init(&b_buf, 0);	// 初始化blob_buf
    // * blobmsg_add_* 可以添加不同类型
    blobmsg_add_string(&b_buf, "response_config", package_name);
    blobmsg_add_string(&b_buf, "response_section", section_name);
    blobmsg_add_string(&b_buf, "response_option", option_name);
    blobmsg_add_string(&b_buf, "response_value", value);

    // 响应
    ubus_send_reply(ctx, req, b_buf.head);	// 响应内容给req
    blob_buf_free(&b_buf);	// 释放blob_buf
    return 0;
}
// .name是参数名称，.type是参数的数据类型
static const struct blobmsg_policy uci_set_policy[__UCI_CMD_MAX] = {
    [PACKAGE_NAME] = {.name = "package", .type = BLOBMSG_TYPE_STRING},
    [SECTION_NAME] = {.name = "section", .type = BLOBMSG_TYPE_STRING},
    [OPTION_NAME] = {.name = "option", .type = BLOBMSG_TYPE_STRING}
};	


// * ubus_object对象中注册的方法
static struct ubus_method demo_methods[] = {
    // * 添加成员方法的操作（policy是传入的参数） { .name = "uci_get", .handler = get_uci, .policy = stu_policy, .n_policy = ARRAY_SIZE(stu_policy) },
    // * 没有传参可以省略{ .name = "test", .handler = func }, 
    // * 使用宏定义的方法来简化操作 UBUS_METHOD(ubus调用名，对应处理函数，传入的参数)
    UBUS_METHOD("uci_get", get_uci, uci_get_policy)//,	// uci_get就是ubus中的接口名称，get_uci是调用的处理函数
    //UBUS_METHOD("uci_set", set_uci, uci_set_policy)		// 这边set的实现不详细讲
};

// * ubus_object对象中类型
static struct ubus_object_type demo_object_type = 
	UBUS_OBJECT_TYPE("test", demo_methods);	// 直接调宏定义函数即可




// * 设置ubus_object对象
static struct ubus_object demo_object = {
    .name = "test",	// 接口对象名称
    .type = &demo_object_type,	// object类型
    .methods = demo_methods,	// 对应调用的方法
    .n_methods = ARRAY_SIZE(demo_methods),	// 方法个数
};





int main(int argc, char* argv[])
{
    // 创建epoll句柄
    uloop_init();

    // 建立连接
    struct ubus_context *ctx = ubus_connect(NULL);
    if (ctx == NULL) {
        printf("ctx null...\n");
    } else {
        printf("ctx not null\n");
    }

    // 向ubusd请求增加一个新的object（这个object在第二部分中实现）
    ubus_add_object(ctx, &demo_object);

    // 把建立的连接加入到uloop中
    ubus_add_uloop(ctx);

    // 等待事件发生，调用对应函数
    uloop_run();

    // 关闭连接
    ubus_free(ctx);

    // 关闭epoll句柄
    uloop_done();

    return 0;
}

int ubus_doing()
{
        int ret;
        // 建立连接
        ctx = ubus_connect(NULL);
        if (!ctx) {
                fprintf(stderr, "Failed to connect to ubus\n");
                return -1;
        }
        // 把建立的连接加入到uloop中
        ubus_add_uloop(ctx);
 
        //ret = ubus_add_object(ctx, &test_object);
        // 向ubusd请求增加一个新的object（这个object在第二部分中实现）
        ret=ubus_add_object(ctx, &demo_object);
        if (ret)
                fprintf(stderr, "Failed to add object: %s\n", ubus_strerror(ret));
}
 /*
 
int main()
{
        int ret;
        // 创建epoll句柄
        uloop_init();
        ubus_doing();

        // 等待事件发生，调用对应函数
        uloop_run();


        // 关闭连接
        ubus_free(ctx);
        // 关闭epoll句柄
        uloop_done();
 
        return 0;
}*/