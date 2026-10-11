#ifndef POKEPLATINUM_PARTICLE_SYSTEM_DEBUG_H
#define POKEPLATINUM_PARTICLE_SYSTEM_DEBUG_H

// Port-only particle instrumentation. Kept apart from particle_system.h so the
// C++ debug GUI can use it without pulling in the SPL headers.

#ifdef __cplusplus
extern "C" {
#endif

// One particle system's live state, for the debug overlay. Particle faults are
// hard to reason about statically because every stage fails the same way on
// screen - an effect that was never emitted, one whose particles all expired,
// and one being drawn fully transparent are indistinguishable as "nothing
// there". Reporting the stages separately says which one actually broke.
typedef struct ParticleSystemDebugInfo {
    int systemIndex;
    int emitterCount;
    int particleCount;
    int childParticleCount;
    int visibleCount; // particles whose drawn alpha is non-zero
    int childCapableCount; // emitters whose resource declares a child resource
} ParticleSystemDebugInfo;

// Fills up to maxCount entries, one per live particle system; returns how many.
int ParticleSystem_DebugSnapshot(ParticleSystemDebugInfo *out, int maxCount);

// One live emitter, with the state of its first particle. Counts alone cannot
// tell a particle that is drawn off-screen from one drawn at zero size or with
// a bogus texture - all three are alive, pass the alpha test, and show nothing.
typedef struct EmitterDebugInfo {
    int systemIndex;
    int drawType;
    int textureIndex;
    int particleCount;
    float posX; // first particle, relative to the emitter
    float posY;
    float posZ;
    float baseScale;
    float animScale;
    int alpha; // the value SPLDraw_Setup skips the particle on when zero
    unsigned int texAddr; // texture VRAM address; zero means the upload never happened
    unsigned int palAddr;
    int texFormat;
    int texWidth;
    int texHeight;
} EmitterDebugInfo;

// Flattens every live emitter across all systems; returns how many were written.
int ParticleSystem_DebugEmitters(EmitterDebugInfo *out, int maxCount);

#ifdef __cplusplus
}
#endif

#endif // POKEPLATINUM_PARTICLE_SYSTEM_DEBUG_H
