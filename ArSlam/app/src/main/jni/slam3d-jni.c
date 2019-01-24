//
//  slam3d-jni.c
//
//  Created by John Miller on 1/22/19.
//  Copyright © 2019 CMU. All rights reserved.
//

#include <jni.h>
#include <particleFilter.h>

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_init(
        JNIEnv* env, jobject thiz, particleFilter_t* pf) {
    particleFilter_init(pf);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_depositVio(
        JNIEnv* env, jobject thiz, particleFilter_t* pf, double t, float x, float y, float z, float dist) {
    particleFilter_depositVio(pf, t, x, y, z, dist);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_depositUwb(
        JNIEnv* env, jobject thiz, particleFilter_t* pf, bcn_t* bcn, float range, float stdRange) {
    particleFilter_depositUwb(pf, bcn, range, stdRange);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_getTagLoc(
        JNIEnv* env, jobject thiz, const particleFilter_t* pf, double* t, float* x, float* y, float* z, float* theta) {
    particleFilter_getTagLoc(pf, t, x, y, z, theta);
}

JNIEXPORT void JNICALL Java_com_example_arslam_Slam3dJni_particleFilter_getBcnLoc(
        JNIEnv* env, jobject thiz, const particleFilter_t* pf, const bcn_t* bcn, double* t, float* x, float* y, float* z) {
    particleFilter_getBcnLoc(pf, bcn, t, x, y, z);
}
