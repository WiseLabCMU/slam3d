
# includes
INCLUDE_DIRECTORIES(
    .
    ../particlefilter/include
    ./cJSON
    )

IF (WIN32)
  	ADD_DEFINITIONS(/DCMAKE_BUILD /D_CRT_SECURE_NO_DEPRECATE)
ENDIF()

# source files
ADD_EXECUTABLE(
	mqttlocalize mqttlocalize.c
	../particlefilter/src/particleFilter.c 
	../particlefilter/src/pfInit.c 
	../particlefilter/src/pfMeasurement.c 
	../particlefilter/src/pfRandom.c
	../particlefilter/src/pfResample.c
	./cJSON/cJSON.c)

TARGET_LINK_LIBRARIES(mqttlocalize paho-mqtt3c -lm)
