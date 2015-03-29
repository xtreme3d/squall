/*******************************************************************\
*
*  EAX.H - Environmental Audio Extensions version 3.0
*          for OpenAL and DirectSound3D	
*
********************************************************************/

#ifndef EAX3_H_INCLUDED
#define EAX3_H_INCLUDED

#pragma pack(push, 4)

typedef enum
{
    DSPROPERTY_EAX3_LISTENER_NONE,
    DSPROPERTY_EAX3_LISTENER_ALLPARAMETERS,
    DSPROPERTY_EAX3_LISTENER_ENVIRONMENT,
    DSPROPERTY_EAX3_LISTENER_ENVIRONMENTSIZE,
    DSPROPERTY_EAX3_LISTENER_ENVIRONMENTDIFFUSION,
    DSPROPERTY_EAX3_LISTENER_ROOM,
    DSPROPERTY_EAX3_LISTENER_ROOMHF,
    DSPROPERTY_EAX3_LISTENER_ROOMLF,
    DSPROPERTY_EAX3_LISTENER_DECAYTIME,
    DSPROPERTY_EAX3_LISTENER_DECAYHFRATIO,
    DSPROPERTY_EAX3_LISTENER_DECAYLFRATIO,
    DSPROPERTY_EAX3_LISTENER_REFLECTIONS,
    DSPROPERTY_EAX3_LISTENER_REFLECTIONSDELAY,
    DSPROPERTY_EAX3_LISTENER_REFLECTIONSPAN,
    DSPROPERTY_EAX3_LISTENER_REVERB,
    DSPROPERTY_EAX3_LISTENER_REVERBDELAY,
    DSPROPERTY_EAX3_LISTENER_REVERBPAN,
    DSPROPERTY_EAX3_LISTENER_ECHOTIME,
    DSPROPERTY_EAX3_LISTENER_ECHODEPTH,
    DSPROPERTY_EAX3_LISTENER_MODULATIONTIME,
    DSPROPERTY_EAX3_LISTENER_MODULATIONDEPTH,
    DSPROPERTY_EAX3_LISTENER_AIRABSORPTIONHF,
    DSPROPERTY_EAX3_LISTENER_HFREFERENCE,
    DSPROPERTY_EAX3_LISTENER_LFREFERENCE,
    DSPROPERTY_EAX3_LISTENER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX3_LISTENER_FLAGS
} DSPROPERTY_EAX3_LISTENERPROPERTY;

// OR these flags with property id
#define DSPROPERTY_EAX3_LISTENER_IMMEDIATE 0x00000000 // changes take effect immediately
#define DSPROPERTY_EAX3_LISTENER_DEFERRED  0x80000000 // changes take effect later
#define DSPROPERTY_EAX3_LISTENER_COMMITDEFERREDSETTINGS (DSPROPERTY_EAX3_LISTENER_NONE | \
                                                       DSPROPERTY_EAX3_LISTENER_IMMEDIATE)

typedef struct _EAX_VECTOR {
	float x;
	float y;
	float z;
} EAX_VECTOR;

// Use this structure for DSPROPERTY_EAX3_LISTENER_ALLPARAMETERS
// - all levels are hundredths of decibels
// - all times and delays are in seconds
//
// NOTE: This structure may change in future EAX versions.
//       It is recommended to initialize fields by name:
//              myListener.lRoom = -1000;
//              myListener.lRoomHF = -100;
//              ...
//              myListener.dwFlags = myFlags /* see EAXLISTENERFLAGS below */ ;
//       instead of:
//              myListener = { -1000, -100, ... , 0x00000009 };
//       If you want to save and load presets in binary form, you 
//       should define your own structure to insure future compatibility.
//
typedef struct _EAX3_LISTENER_PROPERTIES
{
    unsigned long ulEnvironment;   // sets all listener properties
    float flEnvironmentSize;       // environment size in meters
    float flEnvironmentDiffusion;  // environment diffusion
    long lRoom;                    // room effect level (at mid frequencies)
    long lRoomHF;                  // relative room effect level at high frequencies
    long lRoomLF;                  // relative room effect level at low frequencies  
    float flDecayTime;             // reverberation decay time at mid frequencies
    float flDecayHFRatio;          // high-frequency to mid-frequency decay time ratio
    float flDecayLFRatio;          // low-frequency to mid-frequency decay time ratio   
    long lReflections;             // early reflections level relative to room effect
    float flReflectionsDelay;      // initial reflection delay time
    EAX_VECTOR vReflectionsPan;     // early reflections panning vector
    LONG lReverb;                  // late reverberation level relative to room effect
    float flReverbDelay;           // late reverberation delay time relative to initial reflection
    EAX_VECTOR vReverbPan;          // late reverberation panning vector
    float flEchoTime;              // echo time
    float flEchoDepth;             // echo depth
    float flModulationTime;        // modulation time
    float flModulationDepth;       // modulation depth
    float flAirAbsorptionHF;       // change in level per meter at high frequencies
    float flHFReference;           // reference high frequency
    float flLFReference;           // reference low frequency 
    float flRoomRolloffFactor;     // like DS3D flRolloffFactor but for room effect
    unsigned long ulFlags;         // modifies the behavior of properties
} EAX3_LISTENER_PROPERTIES, *LPEAX3_LISTENER_PROPERTIES;

// Used by DSPROPERTY_EAXLISTENER_FLAGS
//
// Note: The number and order of flags may change in future EAX versions.
//       It is recommended to use the flag defines as follows:
//              myFlags = EAXLISTENERFLAGS_DECAYTIMESCALE | EAXLISTENERFLAGS_REVERBSCALE;
//       instead of:
//              myFlags = 0x00000009;
//
// These flags determine what properties are affected by environment size.
#define EAX3_LISTENER_FLAGS_DECAYTIMESCALE        0x00000001 // reverberation decay time
#define EAX3_LISTENER_FLAGS_REFLECTIONSSCALE      0x00000002 // reflection level
#define EAX3_LISTENER_FLAGS_REFLECTIONSDELAYSCALE 0x00000004 // initial reflection delay time
#define EAX3_LISTENER_FLAGS_REVERBSCALE           0x00000008 // reflections level
#define EAX3_LISTENER_FLAGS_REVERBDELAYSCALE      0x00000010 // late reverberation delay time
#define EAX3_LISTENER_FLAGS_ECHOTIMESCALE         0x00000040 // echo time
#define EAX3_LISTENER_FLAGS_MODULATIONTIMESCALE   0x00000080 // modulation time

// This flag limits high-frequency decay time according to air absorption.
#define EAX3_LISTENER_FLAGS_DECAYHFLIMIT          0x00000020
 
#define EAX3_LISTENER_FLAGS_RESERVED              0xFFFFFF00 // reserved future use

// Property ranges and defaults:

#define EAX3_LISTENER_MINENVIRONMENT                0
#define EAX3_LISTENER_MAXENVIRONMENT                (EAX_ENVIRONMENT_COUNT-1)
#define EAX3_LISTENER_DEFAULTENVIRONMENT            EAX_ENVIRONMENT_GENERIC

#define EAX3_LISTENER_MINENVIRONMENTSIZE            1.0f
#define EAX3_LISTENER_MAXENVIRONMENTSIZE            100.0f
#define EAX3_LISTENER_DEFAULTENVIRONMENTSIZE        7.5f

#define EAX3_LISTENER_MINENVIRONMENTDIFFUSION       0.0f
#define EAX3_LISTENER_MAXENVIRONMENTDIFFUSION       1.0f
#define EAX3_LISTENER_DEFAULTENVIRONMENTDIFFUSION   1.0f

#define EAX3_LISTENER_MINROOM                       (-10000)
#define EAX3_LISTENER_MAXROOM                       0
#define EAX3_LISTENER_DEFAULTROOM                   (-1000)

#define EAX3_LISTENER_MINROOMHF                     (-10000)
#define EAX3_LISTENER_MAXROOMHF                     0
#define EAX3_LISTENER_DEFAULTROOMHF                 (-100)

#define EAX3_LISTENER_MINROOMLF                     (-10000)
#define EAX3_LISTENER_MAXROOMLF                     0
#define EAX3_LISTENER_DEFAULTROOMLF                 0

#define EAX3_LISTENER_MINDECAYTIME                  0.1f
#define EAX3_LISTENER_MAXDECAYTIME                  20.0f
#define EAX3_LISTENER_DEFAULTDECAYTIME              1.49f

#define EAX3_LISTENER_MINDECAYHFRATIO               0.1f
#define EAX3_LISTENER_MAXDECAYHFRATIO               2.0f
#define EAX3_LISTENER_DEFAULTDECAYHFRATIO           0.83f

#define EAX3_LISTENER_MINDECAYLFRATIO               0.1f
#define EAX3_LISTENER_MAXDECAYLFRATIO               2.0f
#define EAX3_LISTENER_DEFAULTDECAYLFRATIO           1.00f

#define EAX3_LISTENER_MINREFLECTIONS                (-10000)
#define EAX3_LISTENER_MAXREFLECTIONS                1000
#define EAX3_LISTENER_DEFAULTREFLECTIONS            (-2602)

#define EAX3_LISTENER_MINREFLECTIONSDELAY           0.0f
#define EAX3_LISTENER_MAXREFLECTIONSDELAY           0.3f
#define EAX3_LISTENER_DEFAULTREFLECTIONSDELAY       0.007f

#define EAX3_LISTENER_MINREVERB                     (-10000)
#define EAX3_LISTENER_MAXREVERB                     2000
#define EAX3_LISTENER_DEFAULTREVERB                 200

#define EAXLISTENER_MINREVERBDELAY                0.0f
#define EAXLISTENER_MAXREVERBDELAY                0.1f
#define EAXLISTENER_DEFAULTREVERBDELAY            0.011f

#define EAX3_LISTENER_MINECHOTIME                   0.075f
#define EAX3_LISTENER_MAXECHOTIME	                  0.25f
#define EAX3_LISTENER_DEFAULTECHOTIME               0.25f

#define EAX3_LISTENER_MINECHODEPTH                  0.0f
#define EAX3_LISTENER_MAXECHODEPTH                  1.0f
#define EAX3_LISTENER_DEFAULTECHODEPTH              0.0f

#define EAX3_LISTENER_MINMODULATIONTIME             0.04f
#define EAX3_LISTENER_MAXMODULATIONTIME             4.0f
#define EAX3_LISTENER_DEFAULTMODULATIONTIME         0.25f

#define EAX3_LISTENER_MINMODULATIONDEPTH            0.0f
#define EAX3_LISTENER_MAXMODULATIONDEPTH            1.0f
#define EAX3_LISTENER_DEFAULTMODULATIONDEPTH        0.0f

#define EAX3_LISTENER_MINAIRABSORPTIONHF            (-100.0f)
#define EAX3_LISTENER_MAXAIRABSORPTIONHF            0.0f
#define EAX3_LISTENER_DEFAULTAIRABSORPTIONHF        (-5.0f)

#define EAX3_LISTENER_MINHFREFERENCE                1000.0f
#define EAX3_LISTENER_MAXHFREFERENCE                20000.0f
#define EAX3_LISTENER_DEFAULTHFREFERENCE            5000.0f

#define EAX3_LISTENER_MINLFREFERENCE                20.0f
#define EAX3_LISTENER_MAXLFREFERENCE                1000.0f
#define EAX3_LISTENER_DEFAULTLFREFERENCE            250.0f

#define EAX3_LISTENER_MINROOMROLLOFFFACTOR          0.0f
#define EAX3_LISTENER_MAXROOMROLLOFFFACTOR          10.0f
#define EAX3_LISTENER_DEFAULTROOMROLLOFFFACTOR      0.0f

#define EAX3_LISTENER_DEFAULTFLAGS                  (EAX3_LISTENER_FLAGS_DECAYTIMESCALE |        \
                                                   EAX3_LISTENER_FLAGS_REFLECTIONSSCALE |      \
                                                   EAX3_LISTENER_FLAGS_REFLECTIONSDELAYSCALE | \
                                                   EAX3_LISTENER_FLAGS_REVERBSCALE |           \
                                                   EAX3_LISTENER_FLAGS_REVERBDELAYSCALE |      \
                                                   EAX3_LISTENER_FLAGS_DECAYHFLIMIT)



typedef enum
{
    DSPROPERTY_EAX3_BUFFER_NONE,
    DSPROPERTY_EAX3_BUFFER_ALLPARAMETERS,
    DSPROPERTY_EAX3_BUFFER_OBSTRUCTIONPARAMETERS,
    DSPROPERTY_EAX3_BUFFER_OCCLUSIONPARAMETERS,
    DSPROPERTY_EAX3_BUFFER_EXCLUSIONPARAMETERS,
    DSPROPERTY_EAX3_BUFFER_DIRECT,
    DSPROPERTY_EAX3_BUFFER_DIRECTHF,
    DSPROPERTY_EAX3_BUFFER_ROOM,
    DSPROPERTY_EAX3_BUFFER_ROOMHF,
    DSPROPERTY_EAX3_BUFFER_OBSTRUCTION,
    DSPROPERTY_EAX3_BUFFER_OBSTRUCTIONLFRATIO,
    DSPROPERTY_EAX3_BUFFER_OCCLUSION, 
    DSPROPERTY_EAX3_BUFFER_OCCLUSIONLFRATIO,
    DSPROPERTY_EAX3_BUFFER_OCCLUSIONROOMRATIO,
    DSPROPERTY_EAX3_BUFFER_OCCLUSIONDIRECTRATIO,
    DSPROPERTY_EAX3_BUFFER_EXCLUSION, 
    DSPROPERTY_EAX3_BUFFER_EXCLUSIONLFRATIO,
    DSPROPERTY_EAX3_BUFFER_OUTSIDEVOLUMEHF, 
    DSPROPERTY_EAX3_BUFFER_DOPPLERFACTOR, 
    DSPROPERTY_EAX3_BUFFER_ROLLOFFFACTOR, 
    DSPROPERTY_EAX3_BUFFER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX3_BUFFER_AIRABSORPTIONFACTOR,
    DSPROPERTY_EAX3_BUFFER_FLAGS
} DSPROPERTY_EAX3_BUFFERPROPERTY;    

// OR these flags with property id
#define DSPROPERTY_EAX3_BUFFER_IMMEDIATE 0x00000000 // changes take effect immediately
#define DSPROPERTY_EAX3_BUFFER_DEFERRED  0x80000000 // changes take effect later
#define DSPROPERTY_EAX3_BUFFER_COMMITDEFERREDSETTINGS (DSPROPERTY_EAX3_BUFFER_NONE | \
                                                     DSPROPERTY_EAX3_BUFFER_IMMEDIATE)

// Use this structure for DSPROPERTY_EAXBUFFER_ALLPARAMETERS
// - all levels are hundredths of decibels
// - all delays are in seconds
//
// NOTE: This structure may change in future EAX versions.
//       It is recommended to initialize fields by name:
//              myBuffer.lDirect = 0;
//              myBuffer.lDirectHF = -200;
//              ...
//              myBuffer.dwFlags = myFlags /* see EAXBUFFERFLAGS below */ ;
//       instead of:
//              myBuffer = { 0, -200, ... , 0x00000003 };
//
typedef struct _EAX3_BUFFER_PROPERTIES
{
    long lDirect;                 // direct path level (at low and mid frequencies)
    long lDirectHF;               // relative direct path level at high frequencies
    long lRoom;                   // room effect level (at low and mid frequencies)
    long lRoomHF;                 // relative room effect level at high frequencies
    long lObstruction;            // main obstruction control (attenuation at high frequencies) 
    float flObstructionLFRatio;   // obstruction low-frequency level re. main control
    long lOcclusion;              // main occlusion control (attenuation at high frequencies)
    float flOcclusionLFRatio;     // occlusion low-frequency level re. main control
    float flOcclusionRoomRatio;   // relative occlusion control for room effect
    float flOcclusionDirectRatio; // relative occlusion control for direct path
    long lExclusion;              // main exlusion control (attenuation at high frequencies)
    float flExclusionLFRatio;     // exclusion low-frequency level re. main control
    long lOutsideVolumeHF;        // outside sound cone level at high frequencies
    float flDopplerFactor;        // like DS3D flDopplerFactor but per source
    float flRolloffFactor;        // like DS3D flRolloffFactor but per source
    float flRoomRolloffFactor;    // like DS3D flRolloffFactor but for room effect
    float flAirAbsorptionFactor;  // multiplies DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF
    unsigned long ulFlags;        // modifies the behavior of properties
} EAX3_BUFFER_PROPERTIES, *LPEAX3_BUFFER_PROPERTIES;

// Use this structure for DSPROPERTY_EAXBUFFER_OBSTRUCTION,
typedef struct _EAX3_OBSTRUCTION_PROPERTIES
{
    long lObstruction;
    float flObstructionLFRatio;
} EAX3_OBSTRUCTION_PROPERTIES, *LPEAX3_OBSTRUCTION_PROPERTIES;

// Use this structure for DSPROPERTY_EAXBUFFER_OCCLUSION
typedef struct _EAX3_OCCLUSION_PROPERTIES
{
    long lOcclusion;
    float flOcclusionLFRatio;
    float flOcclusionRoomRatio;
    float flOcclusionDirectRatio;
} EAX3_OCCLUSION_PROPERTIES, *LPEAX3_OCCLUSION_PROPERTIES;

// Use this structure for DSPROPERTY_EAXBUFFER_EXCLUSION
typedef struct _EAX3_EXCLUSION_PROPERTIES
{
    long lExclusion;
    float flExclusionLFRatio;
} EAX3_EXCLUSION_PROPERTIES, *LPEAX3_EXCLUSION_PROPERTIES;

// Used by DSPROPERTY_EAXBUFFER_FLAGS
//    TRUE:    value is computed automatically - property is an offset
//    FALSE:   value is used directly
//
// Note: The number and order of flags may change in future EAX versions.
//       To insure future compatibility, use flag defines as follows:
//              myFlags = EAXBUFFERFLAGS_DIRECTHFAUTO | EAXBUFFERFLAGS_ROOMAUTO;
//       instead of:
//              myFlags = 0x00000003;
//
#define EAX3_BUFFER_FLAGS_DIRECTHFAUTO          0x00000001 // affects DSPROPERTY_EAXBUFFER_DIRECTHF
#define EAX3_BUFFER_FLAGS_ROOMAUTO              0x00000002 // affects DSPROPERTY_EAXBUFFER_ROOM
#define EAX3_BUFFER_FLAGS_ROOMHFAUTO            0x00000004 // affects DSPROPERTY_EAXBUFFER_ROOMHF

#define EAX3_BUFFER_FLAGS_RESERVED              0xFFFFFFF8 // reserved future use

// Property ranges and defaults:

#define EAX3_BUFFER_MINDIRECT                    (-10000)
#define EAX3_BUFFER_MAXDIRECT                    1000
#define EAX3_BUFFER_DEFAULTDIRECT                0

#define EAX3_BUFFER_MINDIRECTHF                  (-10000)
#define EAX3_BUFFER_MAXDIRECTHF                  0
#define EAX3_BUFFER_DEFAULTDIRECTHF              0

#define EAX3_BUFFER_MINROOM                      (-10000)
#define EAX3_BUFFER_MAXROOM                      1000
#define EAX3_BUFFER_DEFAULTROOM                  0

#define EAX3_BUFFER_MINROOMHF                    (-10000)
#define EAX3_BUFFER_MAXROOMHF                    0
#define EAX3_BUFFER_DEFAULTROOMHF                0

#define EAX3_BUFFER_MINOBSTRUCTION               (-10000)
#define EAX3_BUFFER_MAXOBSTRUCTION               0
#define EAX3_BUFFER_DEFAULTOBSTRUCTION           0

#define EAX3_BUFFER_MINOBSTRUCTIONLFRATIO        0.0f
#define EAX3_BUFFER_MAXOBSTRUCTIONLFRATIO        1.0f
#define EAX3_BUFFER_DEFAULTOBSTRUCTIONLFRATIO    0.0f

#define EAX3_BUFFER_MINOCCLUSION                 (-10000)
#define EAX3_BUFFER_MAXOCCLUSION                 0
#define EAX3_BUFFER_DEFAULTOCCLUSION             0

#define EAX3_BUFFER_MINOCCLUSIONLFRATIO          0.0f
#define EAX3_BUFFER_MAXOCCLUSIONLFRATIO          1.0f
#define EAX3_BUFFER_DEFAULTOCCLUSIONLFRATIO      0.25f

#define EAX3_BUFFER_MINOCCLUSIONROOMRATIO        0.0f
#define EAX3_BUFFER_MAXOCCLUSIONROOMRATIO        10.0f
#define EAX3_BUFFER_DEFAULTOCCLUSIONROOMRATIO    1.5f

#define EAX3_BUFFER_MINOCCLUSIONDIRECTRATIO      0.0f
#define EAX3_BUFFER_MAXOCCLUSIONDIRECTRATIO      10.0f
#define EAX3_BUFFER_DEFAULTOCCLUSIONDIRECTRATIO  1.0f

#define EAX3_BUFFER_MINEXCLUSION                 (-10000)
#define EAX3_BUFFER_MAXEXCLUSION                 0
#define EAX3_BUFFER_DEFAULTEXCLUSION             0

#define EAX3_BUFFER_MINEXCLUSIONLFRATIO          0.0f
#define EAX3_BUFFER_MAXEXCLUSIONLFRATIO          1.0f
#define EAX3_BUFFER_DEFAULTEXCLUSIONLFRATIO      1.0f

#define EAX3_BUFFER_MINOUTSIDEVOLUMEHF           (-10000)
#define EAX3_BUFFER_MAXOUTSIDEVOLUMEHF           0
#define EAX3_BUFFER_DEFAULTOUTSIDEVOLUMEHF       0

#define EAX3_BUFFER_MINDOPPLERFACTOR             0.0f
#define EAX3_BUFFER_MAXDOPPLERFACTOR             10.f
#define EAX3_BUFFER_DEFAULTDOPPLERFACTOR         0.0f

#define EAX3_BUFFER_MINROLLOFFFACTOR             0.0f
#define EAX3_BUFFER_MAXROLLOFFFACTOR             10.f
#define EAX3_BUFFER_DEFAULTROLLOFFFACTOR         0.0f

#define EAX3_BUFFER_MINROOMROLLOFFFACTOR         0.0f
#define EAX3_BUFFER_MAXROOMROLLOFFFACTOR         10.f
#define EAX3_BUFFER_DEFAULTROOMROLLOFFFACTOR     0.0f

#define EAX3_BUFFER_MINAIRABSORPTIONFACTOR       0.0f
#define EAX3_BUFFER_MAXAIRABSORPTIONFACTOR       10.0f
#define EAX3_BUFFER_DEFAULTAIRABSORPTIONFACTOR   1.0f

#define EAX3_BUFFER_DEFAULTFLAGS                 (EAX3_BUFFER_FLAGS_DIRECTHFAUTO |       \
                                                EAX3_BUFFER_FLAGS_ROOMAUTO |           \
                                                EAX3_BUFFER_FLAGS_ROOMHFAUTO )

#pragma pack(pop)
#endif
