#include "hook/hook.h"
#include <jni.h>
#include <iostream>
#include <vector>
#include <dlfcn.h>
#include "utils/logger.h"
#include "utils/uilts.h"
#include "utils/AndHook.h"
#include "mono/metadata/threads.h"
#include <fstream>
#include <string.h>

using namespace std;

//自定dll的相关参数
struct DllParam {
    const char *path;  //路径
    const char *imageName;
    const char *nameSpace; //命名空间名
    const char *className; //类名
    const char *methodName; //方法名,一定要是静态函数
};

int delay; //延迟加载dll的时间，防止加载时机太早，依赖的mono导出函数无法正常使用

void hook(MonoString *target, MonoString *replace, MonoString *old) {

    //mono_string_to_utf8      monostring->char*
    char *target_name = mono_string_to_utf8(target); //获取目标方法名
    char *replace_name = mono_string_to_utf8(replace);  //获取替换函数名
    char *old_name = mono_string_to_utf8(old);  //获取目标方法名

    //声明三个vector 从c# 传来的方法全名分割以后 分别储存 imagename::namespace::class::name
    std::vector<char *> vecTarget;
    std::vector<char *> vecReplace;
    std::vector<char *> vecOld;

    //分割字符串 储存信息
    split_str(target_name, ".", vecTarget);
    split_str(replace_name, ".", vecReplace);
    split_str(old_name, ".", vecOld);

    if (vecTarget.size() != 4 || vecReplace.size() != 4 || vecOld.size() != 4) {
        LOGE("vector size is not correct!");
        return;
    }
    //分别获取MonoMethod
    MonoMethod *target_method = get_MonoMethod(vecTarget);
    MonoMethod *replace__method = get_MonoMethod(vecReplace);
    MonoMethod *old_method = get_MonoMethod(vecOld);

    //获取MonoMethod 失败
    if (!target_method || !replace__method || !old_method) {
        LOGE("method get failed");
        return;
    }

    //拿到MonoMethod 还需要进一步通过jit 编译成native code
    void *target_method_pointer = mono_compile_method(target_method);
    void *replace_method_pointer = mono_compile_method(replace__method);
    void *old_method_pointer = mono_compile_method(old_method);

    if (!target_method_pointer || !replace_method_pointer || !old_method_pointer) {
        LOGE("complie failed");
        return;
    }

    //备份原目标函数
    void *ori_target_method;

    //目标函数与replace替换,并将目标函数备份为ori_target_method
    AKHookFunction(target_method_pointer, replace_method_pointer, &ori_target_method);
    //old_method_pointer为自定donet方法编译，hook 实现原目标函数与自定义的old_method替换，在donet中调用old_method等于调用原target_method
    AKHookFunction(old_method_pointer, ori_target_method, NULL);
    //将donet中的方法名与native层的method挂钩，可以直接在donet层调用,目的是为了在donet完成操作hook时，使用old_method时，调用的是native层的原目标函数
    mono_add_internal_call(old_name, old_method);
    LOGD("hook success!");

}


void *mono_thread(void *args) {
    if (!args) {
        LOGD("args is NULL");
        return NULL;
    }
    DllParam *dllParam = (DllParam *) args;
    char libmonoPath[128] = {0};
    sprintf(libmonoPath, "/data/data/%s/lib/libmono.so", getProcName().c_str());
    void *mono_handle;
    //mono被正常加载
    do {
        mono_handle = dlopen(libmonoPath, RTLD_LAZY | RTLD_GLOBAL);
    } while (!mono_handle);
    dlclose(mono_handle);
    //延迟
    sleep(delay);

    MonoDomain *domain = mono_get_root_domain();
    if (!domain) {
        LOGE("domain get failed");
        return NULL;
    }
    MonoThread *pThread = mono_thread_attach(domain);
    if (!pThread) {
        LOGE("attach failed");
        return NULL;
    }

    ifstream dllStream(dllParam->path, ios::binary | ios::ate);
    if (!dllStream) {
        LOGE("dll is not exsit");
        return NULL;
    }
    size_t length = dllStream.tellg();
    dllStream.seekg(0, ios::beg);
    char buff[length];
    memset(buff, 0, length);
    dllStream.read(buff, length);
    MonoImage *pImage = mono_image_open_from_data_with_name(buff, length, false, NULL, false, dllParam->imageName);
    if (!pImage) {
        LOGE("image is null");
        return NULL;
    }
    MonoImageOpenStatus status;
    MonoAssembly *pAssembly = mono_assembly_load_from(pImage, dllParam->path, &status);
    if (pAssembly) {
        char donetMethodName[1024] = {0};
        //donet方法全名
        sprintf(donetMethodName, "%s.%s::MonoNativeHook", dllParam->nameSpace, dllParam->className);
        //与native层方法挂钩
        mono_add_internal_call((const char *) donetMethodName, (void *) hook);
        //通过assembly进一步处理得到image
        MonoImage *pMonoImage = mono_assembly_get_image(pAssembly);
        if (pMonoImage) {
            //通过命名空间和类名获取类的实例对象 MyNameSpace::MyClass
            MonoClass *pClass = mono_class_from_name(pMonoImage, dllParam->nameSpace, dllParam->className);
            if (pClass) {
                //通过class实例对象和方法名获取
                MonoMethod *pMethod = mono_class_get_method_from_name(pClass, dllParam->methodName, 0);
                if (pMethod) {
                    //调用方法，在dll里面声明一个最简单的static method，无需调用对象、无返回值、无参数，另外三个参数全为NULL
                    //C# :
                    //public static void MyMethod(){} 方法里面可以放自己想做的事，也可以什么都不放  ps:此方法一定要是静态函数
                    mono_runtime_invoke(pMethod, NULL, NULL, NULL);
                } else {
                    LOGE("method get failed");
                    return NULL;
                }

            } else {
                LOGE("pClass get failed");
                return NULL;
            }

        } else {
            LOGE("pMonoImage get failed");
            return NULL;
        }

    } else {
        LOGE("assembly get failed");
        return NULL;
    }
    mono_thread_detach(pThread);
    return NULL;

}

void load_dll(const char *_dllPath, const char *_imageName, const char *_nameSpace, const char *_className,
              const char *_methodName,
              int _delay) {
    pthread_t pd;
    //参数以一个Struct指针传入
    DllParam *param = new DllParam;
    param->path = _dllPath;
    param->imageName = _imageName;
    param->nameSpace = _nameSpace;
    param->className = _className;
    delay = _delay;
    pthread_create(&pd, NULL, mono_thread, param);
}