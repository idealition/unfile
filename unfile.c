#include <windows.h>
#include <restartmanager.h>
#include <wchar.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int wmain(int argc, wchar_t *argv[]){
    /*设置控制台输出为UTF-8编码*/
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
    /*检查参数*/
    if(argc <= 1){
        fprintf(stderr, "\n格式: unfile -path [程序路径] -[参数]\n");
        return 1;
    } else if (wcscmp(argv[1] , L"-path") != 0){
        fprintf(stderr,"\n缺少关键参数 \'-path\'\n");
        return 1;
    } else if (argc <= 2){
        fprintf(stderr,"\n缺少关键参数 [程序路径]\n");
        return 1;
    }
    /*规范路径*/
    DWORD pathSize = GetFullPathNameW(argv[2], 0, NULL, NULL);
    if (pathSize == 0) {
        fprintf(stderr, "\n无法获取完整路径: %lu\n", GetLastError());
        return 1;
    }
    wchar_t *fullPath = (wchar_t *)malloc((pathSize + 1) * sizeof(wchar_t));
    if (fullPath == NULL) {
        fprintf(stderr, "\n内存分配失败\n");
        return 1;
    }
    DWORD result = GetFullPathNameW(argv[2], pathSize, fullPath, NULL);
    if (result == 0 || result >= pathSize) {
        fprintf(stderr, "\n无法获取完整路径: %lu\n", GetLastError());
        free(fullPath);
        return 1;
    }
    /*检查路径是否存在*/
    DWORD fileAttr = GetFileAttributesW(fullPath);
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "\n路径不存在: %ls\n", fullPath);
        free(fullPath);
        return 1;
    }
    /*未实现的参数解析功能*/
    /*
    int opt = 0;
    int state = 0;
    optind = 3;
    while((opt =getopt(argc,argv,"pif")) != -1){
        switch(opt){
            case 'p': state += 2;break;
            case 'i': state += 3;break;
            case 'f': state += 7;break;
            case '?': return 1;break;
            default : fprintf(stderr,"\nerror: UNKOWN 0xffff0001\n");
                      assert(0);break;
        }
    }*/

    /*启动会话*/
    DWORD sessionHandle = 0;
    WCHAR sessionKey[CCH_RM_SESSION_KEY + 1] = {0};
    DWORD outerror = RmStartSession(&sessionHandle, 0, sessionKey);
    /*检查会话启动结果*/
    if (outerror != ERROR_SUCCESS) {
        fprintf(stderr, "\nRmStartSession failed with error %lu\n", outerror);
        return 1;
    }
    /*注册文件*/
    PCWSTR filepath = fullPath;
    outerror = RmRegisterResources(sessionHandle, 1, &filepath, 0, NULL, 0, NULL);
    /*检查文件注册结果*/
    if (outerror != ERROR_SUCCESS) {
        fprintf(stderr, "\nRmRegisterResources failed with error %lu\n", outerror);
        RmEndSession(sessionHandle);
        return 1;
    }
    /*获取锁定的进程信息*/
    UINT needed = 0, fact_number = 0;
    DWORD reason;
    RM_PROCESS_INFO *output = NULL;
    while (1){ outerror = RmGetList(sessionHandle, &needed, &fact_number, output, &reason);
    /*检查获取进程信息结果*/
        if (outerror != ERROR_MORE_DATA && outerror != ERROR_SUCCESS) {
            fprintf(stderr, "\nRmGetList failed with error %lu\n", outerror);
            RmEndSession(sessionHandle);
            return 1;
        }
        
        if (outerror == ERROR_SUCCESS) {
            if (fact_number == 0) {
            fprintf(stderr, "\n没有锁定的进程\n");
            RmEndSession(sessionHandle);
            return 0;
            }
            break;
        }
        if (output != NULL){free(output);}
        output = (RM_PROCESS_INFO *)malloc(needed * sizeof(RM_PROCESS_INFO));
        if (output == NULL) {
        fprintf(stderr, "\n内存分配失败\n");
        RmEndSession(sessionHandle);
        return 1;
        }
        fact_number = needed;  // 告诉 RmGetList 缓冲区容量
    }
    /*处理获取到的进程信息*/
    for (UINT i = 0; i < fact_number; i++) {
        fprintf(stderr, "\n锁定进程: %ls (PID: %lu)\n", output[i].strAppName, output[i].Process.dwProcessId);
    }
    /*结束会话*/
    RmEndSession(sessionHandle);
    free(output);
    free(fullPath);
    return 0;
}