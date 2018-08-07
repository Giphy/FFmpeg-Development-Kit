//
// Created by Ilja Kosynkin on 25.03.2016.
//
#include "logjam.h"

#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int ffmpeg_main(int level, int argc, char **argv);

int invoke_ffmpeg(JNIEnv *env, jint loglevel, jobjectArray args);

JNIEXPORT jint JNICALL Java_com_giphy_messenger_util_VideoKit_run(JNIEnv *env, __unused jclass class, jint loglevel, jobjectArray args) {
    pid_t pid = fork();
    if (pid == 0) {
        int retCode = invoke_ffmpeg(env, loglevel, args);
        _exit(retCode);
    } else if (pid > 0) {
        int wstatus;
        pid = waitpid(pid, &wstatus, 0);
        if (pid == -1) {
            LOGW("Failed to wait for ffmpeg process, errno: %d", errno);
        } else if (WIFEXITED(wstatus)) {
            return WEXITSTATUS(wstatus);
        } else if (WIFSIGNALED(wstatus)) {
            LOGW("ffmpeg process killed by signal %d", WTERMSIG(wstatus));
        }
    } else {
        LOGW("Failed to fork ffmpeg process, errno: %d", errno);
    }
    return -1;
}

int invoke_ffmpeg(JNIEnv *env, jint loglevel, jobjectArray args) {
    int i = 0;
    int argc = 0;
    char **argv = NULL;
    jstring *strr = NULL;

    if (args != NULL) {
        argc = (*env)->GetArrayLength(env, args);
        argv = (char **) malloc(sizeof(char *) * argc);
        strr = (jstring *) malloc(sizeof(jstring) * argc);

        for (i = 0; i < argc; ++i) {
            strr[i] = (jstring) (*env)->GetObjectArrayElement(env, args, i);
            argv[i] = (char *) (*env)->GetStringUTFChars(env, strr[i], 0);
            if (loglevel == 2) {
                LOGI("Option: %s", argv[i]);
            }
        }
    }

    int retcode = ffmpeg_main(loglevel, argc, argv);
    if (loglevel == 2) {
        LOGI("Main ended with status %d", retcode);
    }

    for (i = 0; i < argc; ++i) {
        (*env)->ReleaseStringUTFChars(env, strr[i], argv[i]);
    }
    free(argv);
    free(strr);

    return retcode;
}
