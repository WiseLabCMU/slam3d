//
//  slam3d-jni.c
//
//  Created by John Miller on 1/22/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#include <malloc.h>
#include <jni.h>
#include <particleFilter.h>

JNIEXPORT jlong JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_newPf(
        JNIEnv* env, jclass clazz) {
    particleFilter_t* pf = (particleFilter_t*)malloc(sizeof(particleFilter_t));
    particleFilter_init(pf);
    return (jlong)pf;
}

JNIEXPORT jlong JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_newBcn(
        JNIEnv* env, jclass clazz) {
    bcn_t* bcn = (bcn_t*)malloc(sizeof(bcn_t));
    particleFilter_addBcn(bcn);
    return (jlong)bcn;
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_freePf(
        JNIEnv* env, jclass clazz, jlong pf) {
    free((particleFilter_t*)pf);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_freeBcn(
        JNIEnv* env, jclass clazz, jlong bcn) {
    free((bcn_t*)bcn);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_depositVio(
        JNIEnv* env, jclass clazz, jlong pf, jdouble t, jfloat x, jfloat y, jfloat z, jfloat dist) {
    particleFilter_depositVio((particleFilter_t*)pf, (float)t, (float)x, (float)y, (float)z, (float)dist);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_depositUwb(
        JNIEnv* env, jclass clazz, jlong pf, jlong bcn, jfloat range, jfloat stdRange, jlongArray bcnArray) {
    bcn_t** allBcns = (bcn_t**)(*env)->GetLongArrayElements(env, bcnArray, NULL);
    int numBcns = (int)(*env)->GetArrayLength(env, bcnArray);
    particleFilter_depositUwb((particleFilter_t*)pf, (bcn_t*)bcn, (float)range, (float)stdRange, allBcns, numBcns);
    (*env)->ReleaseLongArrayElements(env, bcnArray, (jlong*)allBcns, 0);
}

JNIEXPORT jobject JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_getTagLoc(
        JNIEnv* env, jobject thiz, jlong pf) {
    double t;
    float x, y, z, theta;
    jclass class = (*env)->FindClass(env, "Java/com/example/arslam/Slam3dJni/TagLocation");
    jmethodID cid = (*env)->GetMethodID(env, class, "<init>", "(DFFFF)V");
    particleFilter_getTagLoc((const particleFilter_t*)pf, &t, &x, &y, &z, &theta);
    return (*env)->NewObject(env, class, cid, t, x, y, z, theta);
}

JNIEXPORT jobject JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_getBcnLoc(
        JNIEnv* env, jclass clazz, jlong pf, jlong bcn) {
    double t;
    float x, y, z;
    jclass class = (*env)->FindClass(env, "Java/com/example/arslam/Slam3dJni/BcnLocation");
    jmethodID cid = (*env)->GetMethodID(env, class, "<init>", "(DFFF)V");
    particleFilter_getBcnLoc(pf, bcn, &t, &x, &y, &z);
    return (*env)->NewObject(env, class, cid, t, x, y, z);
}
