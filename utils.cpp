//
// Created by 张小佳 on 2020-04-10.
//

#include "utils/uilts.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>


using namespace std;

MonoImage *image = NULL; //方法所在的image

void split_str(char *str, const char *split, std::vector<char *> &ret) {
    if (!str || !split) {
        LOGD(" split error");
        return;
    }
    char *p = strtok(str, split);
    while (p) {
        ret.push_back(p);
        p = strtok(NULL, split);
    }
}

//assembly为目标程序集，可以利用程序集获取image 进一步获取
static void get_target_image(MonoAssembly *assembly, void *target_img_name) {
    if (!image) {
        MonoImage *cur_image = mono_assembly_get_image(assembly); //获取当前程序集 image
        const char *cur_image_name = mono_image_get_name(cur_image); //通过当前image 获取imageName
        //const char *cur_image_name = (const char*)((intptr_t)image+0x14); //也可以通过image这个结构体（偏移地址0x14）来获取imageName

        //与传入的imageName比较
        if (strcmp(cur_image_name, (const char *) target_img_name) == 0) {
            image = cur_image;
        }
    }
}

MonoMethod *get_MonoMethod(std::vector<char *> vector) {
    char *imageName = vector[0];
    char *nameSpace = vector[1];
    char *className = vector[2];
    char *methodName = vector[3];
    MonoMethod *method = NULL;
    //遍历所有程序集，并获取程序集 并传入回调函数get_target_image 拿到目标image指针
    mono_assembly_foreach((MonoFunc) get_target_image, (void *) imageName);
    if (image) {
        //获取方法的MonoClass
        MonoClass *pClass = mono_class_from_name(image, nameSpace, className);
        if (pClass) {
            //拼凑完整的方法名 className::methodName
            string full_method_name;
            full_method_name.append(className);
            full_method_name.append("::");
            full_method_name.append(methodName);
            //通过完整方法名获取方法描述符
            MonoMethodDesc *pDesc = mono_method_desc_new(full_method_name.c_str(), false);
            //通过方法描述符在指定的MonoClass 寻找MonoMethod
            method = mono_method_desc_search_in_class(pDesc, pClass);
            //释放
            mono_method_desc_free(pDesc);
            return method;
        }
    }
    return NULL;
}

std::string getProcName() {
    std::string ret;
    char cmdline[256] = {0};
    FILE *fp;
    fp = fopen("/proc/self/cmdline", "r");
    if (fp) {
        fgets(cmdline, sizeof(cmdline), fp);
        fclose(fp);
        ret = cmdline;
    }
    return ret;
}