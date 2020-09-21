//
//  slam3d-jni.c
//
//  Created by John Miller on 1/22/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#include <android/log.h>
#include <malloc.h>
#include <jni.h>
#include <particleFilter.h>

#define APPNAME "ArSlam"

JNIEXPORT jlong JNICALL Java_com_example_arslam_Slam3dJni_particleFilterNewPf(
        JNIEnv* env, jclass clazz) {
    particleFilterSlam_t* pf = (particleFilterSlam_t*)malloc(sizeof(particleFilterSlam_t));
    particleFilterSlam_init(pf);
    return (jlong)pf;
}

JNIEXPORT jlong JNICALL Java_com_example_arslam_Slam3dJni_particleFilterNewBcn(
        JNIEnv* env, jclass clazz) {
    bcn_t* bcn = (bcn_t*)malloc(sizeof(bcn_t));
    particleFilterSlam_addBcn(bcn);
    return (jlong)bcn;
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilterFreePf(
        JNIEnv* env, jclass clazz, jlong pf) {
    free((particleFilterSlam_t*)pf);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilterFreeBcn(
        JNIEnv* env, jclass clazz, jlong bcn) {
    free((bcn_t*)bcn);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilterDepositTagVio(
        JNIEnv* env, jclass clazz, jlong pf, jdouble t, jfloat x, jfloat y, jfloat z, jfloat dist) {
    particleFilterSlam_depositTagVio((particleFilterSlam_t*)pf, (double)t, (float)x, (float)y, (float)z, (float)dist);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilterDepositBcnVio(
        JNIEnv *env, jclass clazz, jlong bcn, jdouble t, jfloat x, jfloat y, jfloat z, jfloat dist) {
    particleFilterSlam_depositBcnVio((bcn_t*)bcn, (double)t, (float)x, (float)y, (float)z, (float)dist);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilterDepositRange(
        JNIEnv* env, jclass clazz, jlong pf, jlong bcn, jfloat range, jfloat stdRange, jlongArray bcnArray) {
    bcn_t** allBcns = (bcn_t**)(*env)->GetLongArrayElements(env, bcnArray, NULL);
    int numBcns = (int)(*env)->GetArrayLength(env, bcnArray);
    particleFilterSlam_depositRange((particleFilterSlam_t*)pf, (bcn_t*)bcn, (float)range, (float)stdRange, allBcns, numBcns);
    (*env)->ReleaseLongArrayElements(env, bcnArray, (jlong*)allBcns, 0);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilterDepositRssi(
        JNIEnv* env, jclass clazz, jlong pf, jlong bcn, jint rssi, jlongArray bcnArray) {
    bcn_t** allBcns = (bcn_t**)(*env)->GetLongArrayElements(env, bcnArray, NULL);
    int numBcns = (int)(*env)->GetArrayLength(env, bcnArray);
    particleFilterSlam_depositRssi((particleFilterSlam_t*)pf, (bcn_t*)bcn, (int)rssi, allBcns, numBcns);
    (*env)->ReleaseLongArrayElements(env, bcnArray, (jlong*)allBcns, 0);
}

JNIEXPORT jobject JNICALL Java_com_example_arslam_Slam3dJni_particleFilterGetTagLoc(
        JNIEnv* env, jclass clazz, jlong pf) {
    double t;
    float x, y, z, theta;
    jclass class = (*env)->FindClass(env, "com/example/arslam/Slam3dJni$TagLocation");
    jmethodID cid = (*env)->GetMethodID(env, class, "<init>", "(DFFFF)V");
    particleFilterSlam_getTagLoc((const particleFilterSlam_t*)pf, &t, &x, &y, &z, &theta);
    return (*env)->NewObject(env, class, cid, t, x, y, z, theta);
}

JNIEXPORT jobject JNICALL Java_com_example_arslam_Slam3dJni_particleFilterGetBcnLoc(
        JNIEnv* env, jclass clazz, jlong pf, jlong bcn) {
    double t;
    float x, y, z, theta;
    jclass class = (*env)->FindClass(env, "com/example/arslam/Slam3dJni$BcnLocation");
    jmethodID cid = (*env)->GetMethodID(env, class, "<init>", "(DFFFF)V");
    particleFilterSlam_getBcnLoc((const particleFilterSlam_t*)pf, (const bcn_t*)bcn, &t, &x, &y, &z, &theta);
    return (*env)->NewObject(env, class, cid, t, x, y, z, theta);
}
