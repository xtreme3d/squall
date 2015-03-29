/******************************************************************
*
*  EAX.H - DirectSound3D Environmental Audio Extensions version 2.0
*  Updated July 8, 1999
*
*******************************************************************
*/

#ifndef EAX2_H_INCLUDED
#define EAX2_H_INCLUDED

#pragma pack(push, 4)

typedef enum
{
    DSPROPERTY_EAX2_LISTENER_NONE,
    DSPROPERTY_EAX2_LISTENER_ALLPARAMETERS,
    DSPROPERTY_EAX2_LISTENER_ROOM,
    DSPROPERTY_EAX2_LISTENER_ROOMHF,
    DSPROPERTY_EAX2_LISTENER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX2_LISTENER_DECAYTIME,
    DSPROPERTY_EAX2_LISTENER_DECAYHFRATIO,
    DSPROPERTY_EAX2_LISTENER_REFLECTIONS,
    DSPROPERTY_EAX2_LISTENER_REFLECTIONSDELAY,
    DSPROPERTY_EAX2_LISTENER_REVERB,
    DSPROPERTY_EAX2_LISTENER_REVERBDELAY,
    DSPROPERTY_EAX2_LISTENER_ENVIRONMENT,
    DSPROPERTY_EAX2_LISTENER_ENVIRONMENTSIZE,
    DSPROPERTY_EAX2_LISTENER_ENVIRONMENTDIFFUSION,
    DSPROPERTY_EAX2_LISTENER_AIRABSORPTIONHF,
    DSPROPERTY_EAX2_LISTENER_FLAGS
} DSPROPERTY_EAX2_LISTENERPROPERTY;
	
// OR these flags with property id
#define DSPROPERTY_EAX2_LISTENER_IMMEDIATE 0x00000000 // changes take effect immediately
#define DSPROPERTY_EAX2_LISTENER_DEFERRED  0x80000000 // changes take effect later
#define DSPROPERTY_EAX2_LISTENER_COMMITDEFERREDSETTINGS (DSPROPERTY_EAX2_LISTENER_NONE | \
                                                       DSPROPERTY_EAX2_LISTENER_IMMEDIATE)

// Use this structure for DSPROPERTY_EAX2_LISTENER_ALLPARAMETERS
// - all levels are hundredths of decibels
// - all times are in seconds
// - the reference for high frequency controls is 5 kHz
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
typedef struct _EAX2_LISTENER_PROPERTIES {
    long lRoom;                    // room effect level at low frequencies
    long lRoomHF;                  // room effect high-frequency level re. low frequency level
    float flRoomRolloffFactor;     // like DS3D flRolloffFactor but for room effect
    float flDecayTime;             // reverberation decay time at low frequencies
    float flDecayHFRatio;          // high-frequency to low-frequency decay time ratio
    long lReflections;             // early reflections level relative to room effect
    float flReflectionsDelay;      // initial reflection delay time
    long lReverb;                  // late reverberation level relative to room effect
    float flReverbDelay;           // late reverberation delay time relative to initial reflection
    unsigned long dwEnvironment;   // sets all listener properties
    float flEnvironmentSize;       // environment size in meters
    float flEnvironmentDiffusion;  // environment diffusion
    float flAirAbsorptionHF;       // change in level per meter at 5 kHz
    unsigned long dwFlags;         // modifies the behavior of properties
} EAX2_LISTENER_PROPERTIES, *LPEAX2_LISTENER_PROPERTIES;


// Used by DSPROPERTY_EAX2_LISTENER_FLAGS
//
// Note: The number and order of flags may change in future EAX versions.
//       It is recommended to use the flag defines as follows:
//              myFlags = EAXLISTENERFLAGS_DECAYTIMESCALE | EAXLISTENERFLAGS_REVERBSCALE;
//       instead of:
//              myFlags = 0x00000009;
//
// These flags determine what properties are affected by environment size.
#define EAX2_LISTENER_FLAGS_DECAYTIMESCALE        0x00000001 // reverberation decay time
#define EAX2_LISTENER_FLAGS_REFLECTIONSSCALE      0x00000002 // reflection level
#define EAX2_LISTENER_FLAGS_REFLECTIONSDELAYSCALE 0x00000004 // initial reflection delay time
#define EAX2_LISTENER_FLAGS_REVERBSCALE           0x00000008 // reflections level
#define EAX2_LISTENER_FLAGS_REVERBDELAYSCALE      0x00000010 // late reverberation delay time

// This flag limits high-frequency decay time according to air absorption.
#define EAX2_LISTENER_FLAGS_DECAYHFLIMIT          0x00000020
 
#define EAX2_LISTENER_FLAGS_RESERVED              0xFFFFFFC0 // reserved future use

// property ranges and defaults:

#define EAX2_LISTENER_MINROOM                       (-10000)
#define EAX2_LISTENER_MAXROOM                       0
#define EAX2_LISTENER_DEFAULTROOM                   (-1000)

#define EAX2_LISTENER_MINROOMHF                     (-10000)
#define EAX2_LISTENER_MAXROOMHF                     0
#define EAX2_LISTENER_DEFAULTROOMHF                 (-100)

#define EAX2_LISTENER_MINROOMROLLOFFFACTOR          0.0f
#define EAX2_LISTENER_MAXROOMROLLOFFFACTOR          10.0f
#define EAX2_LISTENER_DEFAULTROOMROLLOFFFACTOR      0.0f

#define EAX2_LISTENER_MINDECAYTIME                  0.1f
#define EAX2_LISTENER_MAXDECAYTIME                  20.0f
#define EAX2_LISTENER_DEFAULTDECAYTIME              1.49f

#define EAX2_LISTENER_MINDECAYHFRATIO               0.1f
#define EAX2_LISTENER_MAXDECAYHFRATIO               2.0f
#define EAX2_LISTENER_DEFAULTDECAYHFRATIO           0.83f

#define EAX2_LISTENER_MINREFLECTIONS                (-10000)
#define EAX2_LISTENER_MAXREFLECTIONS                1000
#define EAX2_LISTENER_DEFAULTREFLECTIONS            (-2602)

#define EAX2_LISTENER_MINREFLECTIONSDELAY           0.0f
#define EAX2_LISTENER_MAXREFLECTIONSDELAY           0.3f
#define EAX2_LISTENER_DEFAULTREFLECTIONSDELAY       0.007f

#define EAX2_LISTENER_MINREVERB                     (-10000)
#define EAX2_LISTENER_MAXREVERB                     2000
#define EAX2_LISTENER_DEFAULTREVERB                 200

#define EAX2_LISTENER_MINREVERBDELAY                0.0f
#define EAX2_LISTENER_MAXREVERBDELAY                0.1f
#define EAX2_LISTENER_DEFAULTREVERBDELAY            0.011f

#define EAX2_LISTENER_MINENVIRONMENT                0
#define EAX2_LISTENER_MAXENVIRONMENT                (EAX_ENVIRONMENT_COUNT-1)
#define EAX2_LISTENER_DEFAULTENVIRONMENT            EAX_ENVIRONMENT_GENERIC

#define EAX2_LISTENER_MINENVIRONMENTSIZE            1.0f
#define EAX2_LISTENER_MAXENVIRONMENTSIZE            100.0f
#define EAX2_LISTENER_DEFAULTENVIRONMENTSIZE        7.5f

#define EAX2_LISTENER_MINENVIRONMENTDIFFUSION       0.0f
#define EAX2_LISTENER_MAXENVIRONMENTDIFFUSION       1.0f
#define EAX2_LISTENER_DEFAULTENVIRONMENTDIFFUSION   1.0f

#define EAX2_LISTENER_MINAIRABSORPTIONHF            (-100.0f)
#define EAX2_LISTENER_MAXAIRABSORPTIONHF            0.0f
#define EAX2_LISTENER_DEFAULTAIRABSORPTIONHF        (-5.0f)

#define EAX2_LISTENER_DEFAULTFLAGS                  (EAX2_LISTENERFLAGS_DECAYTIMESCALE |        \
                                                   EAX2_LISTENERFLAGS_REFLECTIONSSCALE |      \
                                                   EAX2_LISTENERFLAGS_REFLECTIONSDELAYSCALE | \
                                                   EAX2_LISTENERFLAGS_REVERBSCALE |           \
                                                   EAX2_LISTENERFLAGS_REVERBDELAYSCALE |      \
                                                   EAX2_LISTENERFLAGS_DECAYHFLIMIT)



typedef enum
{
    DSPROPERTY_EAX2_BUFFER_NONE,
    DSPROPERTY_EAX2_BUFFER_ALLPARAMETERS,
    DSPROPERTY_EAX2_BUFFER_DIRECT,
    DSPROPERTY_EAX2_BUFFER_DIRECTHF,
    DSPROPERTY_EAX2_BUFFER_ROOM,
    DSPROPERTY_EAX2_BUFFER_ROOMHF, 
    DSPROPERTY_EAX2_BUFFER_ROOMROLLOFFFACTOR,
    DSPROPERTY_EAX2_BUFFER_OBSTRUCTION,
    DSPROPERTY_EAX2_BUFFER_OBSTRUCTIONLFRATIO,
    DSPROPERTY_EAX2_BUFFER_OCCLUSION, 
    DSPROPERTY_EAX2_BUFFER_OCCLUSIONLFRATIO,
    DSPROPERTY_EAX2_BUFFER_OCCLUSIONROOMRATIO,
    DSPROPERTY_EAX2_BUFFER_OUTSIDEVOLUMEHF,
    DSPROPERTY_EAX2_BUFFER_AIRABSORPTIONFACTOR,
    DSPROPERTY_EAX2_BUFFER_FLAGS
} DSPROPERTY_EAX2_BUFFER_PROPERTY;    

// OR these flags with property id
#define DSPROPERTY_EAX2_BUFFER_IMMEDIATE 0x00000000 // changes take effect immediately
#define DSPROPERTY_EAX2_BUFFER_DEFERRED  0x80000000 // changes take effect later
#define DSPROPERTY_EAX2_BUFFER_COMMITDEFERREDSETTINGS (DSPROPERTY_EAX2_BUFFER_NONE | \
                                                     DSPROPERTY_EAX2_BUFFER_IMMEDIATE)

// Use this structure for DSPROPERTY_EAXBUFFER_ALLPARAMETERS
// - all levels are hundredths of decibels
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
typedef struct _EAX2_BUFFER_PROPERTIES
{
    long lDirect;                // direct path level
    long lDirectHF;              // direct path level at high frequencies
    long lRoom;                  // room effect level
    long lRoomHF;                // room effect level at high frequencies
    float flRoomRolloffFactor;   // like DS3D flRolloffFactor but for room effect
    long lObstruction;           // main obstruction control (attenuation at high frequencies) 
    float flObstructionLFRatio;  // obstruction low-frequency level re. main control
    long lOcclusion;             // main occlusion control (attenuation at high frequencies)
    float flOcclusionLFRatio;    // occlusion low-frequency level re. main control
    float flOcclusionRoomRatio;  // occlusion room effect level re. main control
    long lOutsideVolumeHF;       // outside sound cone level at high frequencies
    float flAirAbsorptionFactor; // multiplies DSPROPERTY_EAXLISTENER_AIRABSORPTIONHF
    unsigned long dwFlags;       // modifies the behavior of properties
} EAX2_BUFFER_PROPERTIES, *LPEAX2_BUFFER_PROPERTIES;

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
#define EAX2_BUFFERFLAGS_DIRECTHFAUTO 0x00000001 // affects DSPROPERTY_EAXBUFFER_DIRECTHF
#define EAX2_BUFFERFLAGS_ROOMAUTO     0x00000002 // affects DSPROPERTY_EAXBUFFER_ROOM
#define EAX2_BUFFERFLAGS_ROOMHFAUTO   0x00000004 // affects DSPROPERTY_EAXBUFFER_ROOMHF

#define EAX2_BUFFERFLAGS_RESERVED     0xFFFFFFF8 // reserved future use

// property ranges and defaults:

#define EAX2_BUFFER_MINDIRECT                  (-10000)
#define EAX2_BUFFER_MAXDIRECT                  1000
#define EAX2_BUFFER_DEFAULTDIRECT              0

#define EAX2_BUFFER_MINDIRECTHF                (-10000)
#define EAX2_BUFFER_MAXDIRECTHF                0
#define EAX2_BUFFER_DEFAULTDIRECTHF            0

#define EAX2_BUFFER_MINROOM                    (-10000)
#define EAX2_BUFFER_MAXROOM                    1000
#define EAX2_BUFFER_DEFAULTROOM                0

#define EAX2_BUFFER_MINROOMHF                  (-10000)
#define EAX2_BUFFER_MAXROOMHF                  0
#define EAX2_BUFFER_DEFAULTROOMHF              0

#define EAX2_BUFFER_MINROOMROLLOFFFACTOR       0.0f
#define EAX2_BUFFER_MAXROOMROLLOFFFACTOR       10.f
#define EAX2_BUFFER_DEFAULTROOMROLLOFFFACTOR   0.0f

#define EAX2_BUFFER_MINOBSTRUCTION             (-10000)
#define EAX2_BUFFER_MAXOBSTRUCTION             0
#define EAX2_BUFFER_DEFAULTOBSTRUCTION         0

#define EAX2_BUFFER_MINOBSTRUCTIONLFRATIO      0.0f
#define EAX2_BUFFER_MAXOBSTRUCTIONLFRATIO      1.0f
#define EAX2_BUFFER_DEFAULTOBSTRUCTIONLFRATIO  0.0f

#define EAX2_BUFFER_MINOCCLUSION               (-10000)
#define EAX2_BUFFER_MAXOCCLUSION               0
#define EAX2_BUFFER_DEFAULTOCCLUSION           0

#define EAX2_BUFFER_MINOCCLUSIONLFRATIO        0.0f
#define EAX2_BUFFER_MAXOCCLUSIONLFRATIO        1.0f
#define EAX2_BUFFER_DEFAULTOCCLUSIONLFRATIO    0.25f

#define EAX2_BUFFER_MINOCCLUSIONROOMRATIO      0.0f
#define EAX2_BUFFER_MAXOCCLUSIONROOMRATIO      10.0f
#define EAX2_BUFFER_DEFAULTOCCLUSIONROOMRATIO  0.5f

#define EAX2_BUFFER_MINOUTSIDEVOLUMEHF         (-10000)
#define EAX2_BUFFER_MAXOUTSIDEVOLUMEHF         0
#define EAX2_BUFFER_DEFAULTOUTSIDEVOLUMEHF     0

#define EAX2_BUFFER_MINAIRABSORPTIONFACTOR     0.0f
#define EAX2_BUFFER_MAXAIRABSORPTIONFACTOR     10.0f
#define EAX2_BUFFER_DEFAULTAIRABSORPTIONFACTOR 1.0f

#define EAX2_BUFFER_DEFAULTFLAGS               (EAX2_BUFFERFLAGS_DIRECTHFAUTO | \
                                              EAX2_BUFFERFLAGS_ROOMAUTO |     \
                                              EAX2_BUFFERFLAGS_ROOMHFAUTO)

// Material transmission presets
// 3 values in this order:
//     1: occlusion (or obstruction)
//     2: occlusion LF Ratio (or obstruction LF Ratio)
//     3: occlusion Room Ratio

// Single window material preset
#define EAX2_MATERIAL_SINGLEWINDOW          (-2800)
#define EAX2_MATERIAL_SINGLEWINDOWLF        0.71f
#define EAX2_MATERIAL_SINGLEWINDOWROOMRATIO 0.43f

// Double window material preset
#define EAX2_MATERIAL_DOUBLEWINDOW          (-5000)
#define EAX2_MATERIAL_DOUBLEWINDOWHF        0.40f
#define EAX2_MATERIAL_DOUBLEWINDOWROOMRATIO 0.24f

// Thin door material preset
#define EAX2_MATERIAL_THINDOOR              (-1800)
#define EAX2_MATERIAL_THINDOORLF            0.66f
#define EAX2_MATERIAL_THINDOORROOMRATIO     0.66f

// Thick door material preset
#define EAX2_MATERIAL_THICKDOOR             (-4400)
#define EAX2_MATERIAL_THICKDOORLF           0.64f
#define EAX2_MATERIAL_THICKDOORROOMRTATION  0.27f

// Wood wall material preset
#define EAX2_MATERIAL_WOODWALL              (-4000)
#define EAX2_MATERIAL_WOODWALLLF            0.50f
#define EAX2_MATERIAL_WOODWALLROOMRATIO     0.30f

// Brick wall material preset
#define EAX2_MATERIAL_BRICKWALL             (-5000)
#define EAX2_MATERIAL_BRICKWALLLF           0.60f
#define EAX2_MATERIAL_BRICKWALLROOMRATIO    0.24f

// Stone wall material preset
#define EAX2_MATERIAL_STONEWALL             (-6000)
#define EAX2_MATERIAL_STONEWALLLF           0.68f
#define EAX2_MATERIAL_STONEWALLROOMRATIO    0.20f

// Curtain material preset
#define EAX2_MATERIAL_CURTAIN               (-1200)
#define EAX2_MATERIAL_CURTAINLF             0.15f
#define EAX2_MATERIAL_CURTAINROOMRATIO      1.00f

#pragma pack(pop)

#endif
