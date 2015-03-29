// EAX.H -- DirectSound Environmental Audio Extensions

#ifndef EAX1_H_INCLUDED
#define EAX1_H_INCLUDED

typedef enum 
{
	DSPROPERTY_EAX1_LISTENER_ALLPARAMETERS,		// all reverb properties
    DSPROPERTY_EAX1_LISTENER_ENVIRONMENT,		// standard environment no.
    DSPROPERTY_EAX1_LISTENER_VOLUME,			// loudness of the reverb
    DSPROPERTY_EAX1_LISTENER_DECAYTIME,			// how long the reverb lasts
    DSPROPERTY_EAX1_LISTENER_DAMPING			// the high frequencies decay faster
} DSPROPERTY_EAX1_REVERBPROPERTY;

// use this structure for get/set all properties...
typedef struct _EAX1_LISTENER_PROPERTIES {
    unsigned long	environment;		// 0 to EAX_ENVIRONMENT_COUNT-1
    float			fVolume;			// 0 to 1
    float			fDecayTime_sec;		// seconds, 0.1 to 100
    float			fDamping;			// 0 to 1
} EAX1_LISTENER_PROPERTIES, *LPEAX1_LISTENER_PROPERTIES;

typedef enum 
{
    DSPROPERTY_EAX1_BUFFER_ALLPARAMETERS,	// all reverb buffer properties
    DSPROPERTY_EAX1_BUFFER_REVERBMIX		// the wet source amount
} DSPROPERTY_EAX1_BUFFER_REVERBPROPERTY;

// use this structure for get/set all properties...
typedef struct _EAX1_BUFFER_PROPERTIES {
    float fMix;                          // linear factor, 0.0F to 1.0F
} EAX1_BUFFER_PROPERTIES, *LPEAX1_BUFFER_PROPERTIES;

#endif
