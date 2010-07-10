#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jvmti.h"

/* ------------------------------------------------------------------- */
/* Some constant maximum sizes */

#define MAX_TOKEN_LENGTH        16
#define MAX_THREAD_NAME_LENGTH  512
#define MAX_METHOD_NAME_LENGTH  1024
#define MAX_FRAMES        256

static jvmtiEnv *jvmti = NULL;
static jvmtiCapabilities capa;

/* Global agent data structure */

typedef struct {
    /* JVMTI Environment */
    jvmtiEnv      *jvmti;
    jboolean       vm_is_started;
    /* Data access Lock */
    jrawMonitorID  lock;
} GlobalAgentData;

static GlobalAgentData *gdata;


static jlong combined_size;
static int num_class_refs;
static int num_field_refs;
static int num_array_refs;
static int num_classloader_refs;
static int num_signer_refs;
static int num_protection_domain_refs;
static int num_interface_refs;
static int num_static_field_refs;
static int num_constant_pool_refs;


/* Every JVMTI interface returns an error code, which should be checked
 *   to avoid any cascading errors down the line.
 *   The interface GetErrorName() returns the actual enumeration constant
 *   name, making the error messages much easier to understand.
 */
static void check_jvmti_error(jvmtiEnv *jvmti, jvmtiError errnum, const char *str) {
    if ( errnum != JVMTI_ERROR_NONE ) {
        char       *errnum_str;

        errnum_str = NULL;
        (void)(*jvmti)->GetErrorName(jvmti, errnum, &errnum_str);

        printf("ERROR: JVMTI: %d(%s): %s\n", errnum, (errnum_str==NULL?"Unknown":errnum_str), (str==NULL?"":str));
    }
}


/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
static void enter_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error;

    error = (*jvmti)->RawMonitorEnter(jvmti, gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
static void exit_critical_section(jvmtiEnv *jvmti) {
    jvmtiError error;

    error = (*jvmti)->RawMonitorExit(jvmti, gdata->lock);
    check_jvmti_error(jvmti, error, "Cannot exit with raw monitor");
}

void describe(jvmtiError err) {
    jvmtiError err0;
    char *descr;
    err0 = (*jvmti)->GetErrorName(jvmti, err, &descr);
    if (err0 == JVMTI_ERROR_NONE) {
        printf("%s\n", descr);
    } else {
        printf("error [%d]\n", err);
    }
}

void describe2(const char *t, jvmtiError err) {
    printf("%s ", t);
    describe(err);
}

// Exception callback
static void JNICALL callbackException(jvmtiEnv *jvmti_env, JNIEnv* env, jthread thr, jmethodID method, jlocation location, jobject exception, jmethodID catch_method, jlocation catch_location) {

    jclass potsExceptionClass;
    potsExceptionClass = (*env)->FindClass(env, "pots/POTS$POTSException");

    if ((*env)->IsInstanceOf(env, exception, potsExceptionClass)) {


        enter_critical_section(jvmti);
        {

            jvmtiError err, err1, err2, error;
            jvmtiThreadInfo info, info1;
            jvmtiThreadGroupInfo groupInfo;
            jint num_monitors;
            jobject *arr_monitors;

            jvmtiFrameInfo frames[5];
            jint count;
            jint flag = 0;
            jint thr_st_ptr;
            jint thr_count;
            jthread *thr_ptr;

            jvmtiError err3;
            char *name;
            char *sig;
            char *gsig;

            {
                jobject dump;
                jclass dumpClass;

                {
                    jclass c;
                    jmethodID m;

                    c = (*env)->FindClass(env, "pots/POTS");
                    if ((*env)->ExceptionOccurred(env)) {
                        (*env)->ExceptionDescribe(env);
                    }
                    printf("class:%p\n",c);
                    m = (*env)->GetStaticMethodID(env, c, "newDump", "()Lpots/POTS$Dump;");
                    printf("method:%p\n",m);
                    dump = (*env)->CallStaticObjectMethod(env, c, m);
                    printf("dump: %p\n",dump);

                    dumpClass = (*env)->GetObjectClass(env, dump);
                }


                {
                    jvmtiStackInfo *stack_info;
                    jint thread_count;
                    int ti;

                    err = (*jvmti)->GetAllStackTraces(jvmti, MAX_FRAMES, &stack_info, &thread_count);
                    if (err != JVMTI_ERROR_NONE) {
                        describe2("GetAllStackTraces", err);
                    }
                    for (ti = 0; ti < thread_count; ++ti) {
                        jvmtiStackInfo *infop = &stack_info[ti];
                        jthread thread = infop->thread;
                        jint state = infop->state;
                        jvmtiFrameInfo *frames = infop->frame_buffer;
                        int fi;

                        {
                            jmethodID m;
                            jlong cpuTime;
                            (*jvmti)->GetThreadCpuTime(jvmti, thread, &cpuTime);
                            m=(*env)->GetMethodID(env, dumpClass, "visitThread", "(Ljava/lang/Thread;IJ)V");
                            (*env)->CallObjectMethod(env, dump, m, thread, state, cpuTime);
                        }

                        for (fi = 0; fi < infop->frame_count; fi++) {
                            char *methodName="";
                            jclass methodClass;
                            err = (*jvmti)->GetMethodName(jvmti, frames[fi].method, &methodName, NULL, NULL);
                            err = (*jvmti)->GetMethodDeclaringClass(jvmti, frames[fi].method, &methodClass);
                            //printf("frame (%s:%i)\n", methodName, frames[fi].location);
                            {
                                //Visit method
                                jmethodID m;
                                jstring jmethodName;

                                jmethodName=(*env)->NewStringUTF(env, methodName);

                                m=(*env)->GetMethodID(env, dumpClass, "visitMethod", "(Ljava/lang/Class;Ljava/lang/String;I)V");
                                (*env)->CallObjectMethod(env, dump, m, methodClass, jmethodName, frames[fi].location);
                            }

                            {
                                //Visit args
                                jint n;
                                int j,k;
                                jvmtiLocalVariableEntry* entries;

                                k=0;
                                for (j=0; j<10; j++) {
                                    jobject v1;
                                    jint v2;
                                    jlong v3;
                                    jfloat v4;
                                    jdouble v5;
                                    if ((err = (*jvmti)->GetLocalObject(jvmti, thread, fi, j, &v1)) == JVMTI_ERROR_NONE && v1 != NULL) {
                                        jmethodID m;
                                        m=(*env)->GetMethodID(env, dumpClass, "visitArg", "(Ljava/lang/Object;)V");
                                        (*env)->CallObjectMethod(env, dump, m, v1);
                                    } else if ((err = (*jvmti)->GetLocalInt(jvmti, thread, fi, j, &v2)) == JVMTI_ERROR_NONE && v2 != 0) {
                                        jmethodID m;
                                        m=(*env)->GetMethodID(env, dumpClass, "visitArg", "(I)V");
                                        (*env)->CallObjectMethod(env, dump, m, v2);
                                    } else if ((err = (*jvmti)->GetLocalLong(jvmti, thread, fi, j, &v3)) == JVMTI_ERROR_NONE && v3 != 0) {
                                        jmethodID m;
                                        m=(*env)->GetMethodID(env, dumpClass, "visitArg", "(J)V");
                                        (*env)->CallObjectMethod(env, dump, m, v3);
                                    } else if ((err = (*jvmti)->GetLocalFloat(jvmti, thread, fi, j, &v4)) == JVMTI_ERROR_NONE && v4 != 0) {
                                        jmethodID m;
                                        m=(*env)->GetMethodID(env, dumpClass, "visitArg", "(F)V");
                                        (*env)->CallObjectMethod(env, dump, m, v4);
                                    } else if ((err = (*jvmti)->GetLocalDouble(jvmti, thread, fi, j, &v5)) == JVMTI_ERROR_NONE && v5 != 0) {
                                        jmethodID m;
                                        m=(*env)->GetMethodID(env, dumpClass, "visitArg", "(D)V");
                                        (*env)->CallObjectMethod(env, dump, m, v5);
                                    } else {
                                        jmethodID m;
                                        m=(*env)->GetMethodID(env, dumpClass, "visitArg", "(Ljava/lang/Object;)V");
                                        (*env)->CallObjectMethod(env, dump, m, NULL);
                                    }
                                }
                            }
                            {
                                jmethodID m;
                                m=(*env)->GetMethodID(env, dumpClass, "visitMethodEnd", "()V");
                                (*env)->CallObjectMethod(env, dump, m);
                            }
                        }
                        {
                            jmethodID m;
                            m=(*env)->GetMethodID(env, dumpClass, "visitThreadEnd", "()V");
                            (*env)->CallObjectMethod(env, dump, m);
                        }
                    }
                    err = (*jvmti)->Deallocate(jvmti, (unsigned char *)stack_info);
                    {
                        jmethodID m;
                        m=(*env)->GetMethodID(env, dumpClass, "completed", "()V");
                        (*env)->CallObjectMethod(env, dump, m);
                    }
                }
            }
        }
        exit_critical_section(jvmti);
    }
}

// VM Death callback
static void JNICALL callbackVMDeath(jvmtiEnv *jvmti_env, JNIEnv* env) {
    enter_critical_section(jvmti);
    {
        jclass c;
        jmethodID m;

        c = (*env)->FindClass(env, "pots/POTS");
        if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionDescribe(env);
        }
        printf("class:%p\n",c);
        m = (*env)->GetStaticMethodID(env, c, "destroy", "()V");
        printf("method:%p\n",m);
        (*env)->CallStaticObjectMethod(env, c, m);
    }
    exit_critical_section(jvmti);
}


/* Get a name for a jthread */
static void get_thread_name(jvmtiEnv *jvmti, jthread thread, char *tname, int maxlen) {
    jvmtiThreadInfo info;
    jvmtiError      error;

    /* Make sure the stack variables are garbage free */
    (void)memset(&info,0, sizeof(info));

    /* Assume the name is unknown for now */
    (void)strcpy(tname, "Unknown");

    /* Get the thread information, which includes the name */
    error = (*jvmti)->GetThreadInfo(jvmti, thread, &info);
    check_jvmti_error(jvmti, error, "Cannot get thread info");

    /* The thread might not have a name, be careful here. */
    if ( info.name != NULL ) {
        int len;

        /* Copy the thread name into tname if it will fit */
        len = (int)strlen(info.name);
        if ( len < maxlen ) {
            (void)strcpy(tname, info.name);
        }

        /* Every string allocated by JVMTI needs to be freed */
        error = (*jvmti)->Deallocate(jvmti, (unsigned char *)info.name);
        if (error != JVMTI_ERROR_NONE) {
            printf("(get_thread_name) Error expected: %d, got: %d\n", JVMTI_ERROR_NONE, error);
            describe(error);
            printf("\n");
        }

    }
}


// VM init callback
static void JNICALL callbackVMInit(jvmtiEnv *jvmti_env, JNIEnv* env, jthread thread) {
    enter_critical_section(jvmti);
    {
        jvmtiError error;
        /*

        char  tname[MAX_THREAD_NAME_LENGTH];
        static jvmtiEvent events[] = { JVMTI_EVENT_THREAD_START, JVMTI_EVENT_THREAD_END };
        int        i;
        jvmtiFrameInfo frames[5];
        jvmtiError err, err1;
        jint count;

        printf("Got VM init event\n");
        get_thread_name(jvmti_env , thread, tname, sizeof(tname));
        printf("callbackVMInit:  %s thread\n", tname);


        */
        error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE, JVMTI_EVENT_EXCEPTION, (jthread)NULL);
        check_jvmti_error(jvmti_env, error, "Cannot set event notification");

        jclass c;
        jmethodID m;

        c = (*env)->FindClass(env, "pots/POTS");
        if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionDescribe(env);
        }
        printf("class:%p\n",c);
        m = (*env)->GetStaticMethodID(env, c, "init", "()V");
        printf("method:%p\n",m);
        (*env)->CallStaticObjectMethod(env, c, m);


    }
    exit_critical_section(jvmti);

}

JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options, void *reserved) {
    static GlobalAgentData data;
    jvmtiError error;
    jint res;
    jvmtiEventCallbacks callbacks;


    /* Setup initial global agent data area
    *   Use of static/extern data should be handled carefully here.
    *   We need to make sure that we are able to cleanup after ourselves
    *     so anything allocated in this library needs to be freed in
    *     the Agent_OnUnload() function.
    */
    (void)memset((void*)&data, 0, sizeof(data));
    gdata = &data;

    /*  We need to first get the jvmtiEnv* or JVMTI environment */

    res = (*jvm)->GetEnv(jvm, (void **) &jvmti, JVMTI_VERSION_1_0);

    if (res != JNI_OK || jvmti == NULL) {
        /* This means that the VM was unable to obtain this version of the
             *   JVMTI interface, this is a fatal error.
             */
        printf("ERROR: Unable to access JVMTI Version 1 (0x%x),"
               " is your J2SE a 1.5 or newer version?"
               " JNIEnv's GetEnv() returned %d\n",
               JVMTI_VERSION_1, res);
    }

    /* Here we save the jvmtiEnv* for Agent_OnUnload(). */
    gdata->jvmti = jvmti;

    (void)memset(&capa, 0, sizeof(jvmtiCapabilities));
    capa.can_generate_exception_events = 1;
    capa.can_access_local_variables = 1;
    capa.can_get_thread_cpu_time = 1;

    error = (*jvmti)->AddCapabilities(jvmti, &capa);
    check_jvmti_error(jvmti, error, "Unable to get necessary JVMTI capabilities.");


    (void)memset(&callbacks, 0, sizeof(callbacks));
    callbacks.VMInit = &callbackVMInit; /* JVMTI_EVENT_VM_INIT */
    callbacks.VMDeath = &callbackVMDeath; /* JVMTI_EVENT_VM_DEATH */
    callbacks.Exception = &callbackException;/* JVMTI_EVENT_EXCEPTION */


    error = (*jvmti)->SetEventCallbacks(jvmti, &callbacks, (jint)sizeof(callbacks));
    check_jvmti_error(jvmti, error, "Cannot set jvmti callbacks");

    /* At first the only initial events we are interested in are VM
     *   initialization, VM death, and Class File Loads.
     *   Once the VM is initialized we will request more events.
    */
    error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
            JVMTI_EVENT_VM_INIT, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");
    error = (*jvmti)->SetEventNotificationMode(jvmti, JVMTI_ENABLE,
            JVMTI_EVENT_VM_DEATH, (jthread)NULL);
    check_jvmti_error(jvmti, error, "Cannot set event notification");


    /* Here we create a raw monitor for our use in this agent to
     *   protect critical sections of code.
     */
    error = (*jvmti)->CreateRawMonitor(jvmti, "agent data", &(gdata->lock));
    check_jvmti_error(jvmti, error, "Cannot create raw monitor");

    /* We return JNI_OK to signify success */
    return JNI_OK;


}


/* Agent_OnUnload: This is called immediately before the shared library is
 *   unloaded. This is the last code executed.
 */
JNIEXPORT void JNICALL Agent_OnUnload(JavaVM *vm) {
    /* Make sure all malloc/calloc/strdup space is freed */
}

