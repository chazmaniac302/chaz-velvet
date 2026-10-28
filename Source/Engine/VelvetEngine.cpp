#include "VelvetEngine.h"

namespace chaz
{

void VelvetEngine::prepare (double sampleRate, int /*block*/) noexcept
{
    sr = sampleRate > 1.0 ? sampleRate : 44100.0;
    invSr = 1.0f / (float) sr;
    velvetSm.prepare (sr, 0.03);
    glowSm.prepare (sr, 0.03);
    spaceSm.prepare (sr, 0.04);
    dustSm.prepare (sr, 0.05);
    widthSm.prepare (sr, 0.03);
    masterSm.prepare (sr, 0.02);
    cutSm.prepare (sr, 0.025);
    reset();
    applyMacroTargets();
}

void VelvetEngine::reset() noexcept
{
    for (auto& v : voices)
    {
        v = Voice{};
        v.epLp.reset();
        v.padLp.reset();
        v.voiceLp.reset();
    }
    masterLpL.reset(); masterLpR.reset();
    airHpL.reset(); airHpR.reset();
    delayL.fill (0); delayR.fill (0);
    delayPos = 0;
    dustLp = 0;
    meter = 0;
}

int VelvetEngine::getActiveVoices() const noexcept
{
    int n = 0;
    for (const auto& v : voices) if (v.active) ++n;
    return n;
}

void VelvetEngine::applyMacroTargets() noexcept
{
    // Velvet: warm LPF (lower = darker)
    const float velvet = clamp01 (params.velvet);
    const float cutBase = clamp01 (params.cutoff);
    const float cutNorm = lerp (cutBase * 0.55f, cutBase, 1.0f - velvet * 0.65f);
    const float cutHz = 180.0f * std::pow (100.0f, cutNorm); // ~180..18k
    masterLpL.setCutoff (cutHz, sr);
    masterLpR.setCutoff (cutHz, sr);
    cutSm.setTarget (cutNorm);

    // Glow: high-shelf-ish via HP air blend amount (applied in process)
    glowSm.setTarget (clamp01 (params.glow));
    airHpL.setCutoff (2800.0f + clamp01 (params.glow) * 4200.0f, sr);
    airHpR.setCutoff (2800.0f + clamp01 (params.glow) * 4200.0f, sr);

    spaceSm.setTarget (clamp01 (params.space));
    dustSm.setTarget (clamp01 (params.dust));
    widthSm.setTarget (clamp01 (params.width));
    velvetSm.setTarget (velvet);
    masterSm.setTarget (dbToGain (params.masterGainDb));
}

float VelvetEngine::dustNoise() noexcept
{
    rng = rng * 1664525u + 1013904223u;
    float n = ((rng >> 8) & 0xffff) / 65535.0f * 2.0f - 1.0f;
    // soft vinyl crackle: occasional spikes
    if ((rng & 0x3fffu) < 18u)
        n += ((rng >> 16) & 0xff) / 255.0f * 2.0f - 1.0f;
    dustLp = dustLp * 0.97f + n * 0.03f;
    return dustLp * 0.55f + n * 0.15f;
}

VelvetEngine::Voice* VelvetEngine::allocVoice (int note) noexcept
{
    // Prefer free, else same note, else oldest
    Voice* free = nullptr;
    Voice* oldest = &voices[0];
    for (auto& v : voices)
    {
        if (! v.active) { free = &v; break; }
        if (v.age > oldest->age) oldest = &v;
    }
    if (free) return free;
    for (auto& v : voices)
        if (v.note == note) return &v;
    return oldest;
}

VelvetEngine::Voice* VelvetEngine::findVoice (int note) noexcept
{
    for (auto& v : voices)
        if (v.active && v.note == note && ! v.releasing)
            return &v;
    for (auto& v : voices)
        if (v.active && v.note == note)
            return &v;
    return nullptr;
}

void VelvetEngine::noteOn (int midiNote, float velocity) noexcept
{
    velocity = clamp01 (velocity);
    auto* v = allocVoice (midiNote);
    if (! v) return;
    v->active = true;
    v->note = midiNote;
    v->vel = 0.25f + 0.75f * velocity; // musical velocity curve
    v->age = 0;
    v->releasing = false;

    v->epPhase = 0; v->epMod = 0; v->epBellPhase = 0;
    v->epEnv = 0; v->epStage = 0;
    v->epLp.reset();

    v->padPhaseA = 0; v->padPhaseB = 0.37;
    v->padEnv = 0; v->padStage = 0;
    v->padLp.reset();

    v->sparkEnv = 1.0f;
    v->sparkPhase = 0; v->sparkMod = 0;

    v->subPhase = 0;
    v->subEnv = 1.0f;

    v->voiceLp.reset();
    v->tremPhase = 0;

    const float hz = midiToHz ((float) midiNote);
    v->epLp.setCutoff (hz * (2.5f + params.epTone * 6.0f), sr);
    v->padLp.setCutoff (lerp (400.0f, 4500.0f, clamp01 (params.padCutoff)), sr);
    v->voiceLp.setCutoff (hz * (3.0f + clamp01 (params.cutoff) * 8.0f), sr);
}

void VelvetEngine::noteOff (int midiNote) noexcept
{
    for (auto& v : voices)
        if (v.active && v.note == midiNote)
            v.releasing = true;
}

void VelvetEngine::allNotesOff() noexcept
{
    for (auto& v : voices)
        if (v.active)
            v.releasing = true;
}

static void advanceAdsr (float& env, int& stage, bool releasing,
                         float A, float D, float S, float R, float invSr, bool& active) noexcept
{
    A = std::max (0.001f, A);
    D = std::max (0.01f, D);
    R = std::max (0.01f, R);
    S = clamp01 (S);

    if (releasing && stage < 3)
        stage = 3;

    if (stage == 0)
    {
        env += invSr / A;
        if (env >= 1.0f) { env = 1.0f; stage = 1; }
    }
    else if (stage == 1)
    {
        env -= (1.0f - S) * invSr / D;
        if (env <= S) { env = S; stage = 2; }
    }
    else if (stage == 2)
    {
        env = S;
    }
    else
    {
        env -= invSr / R;
        if (env <= 0.0001f) { env = 0; active = false; }
    }
}

void VelvetEngine::renderVoice (Voice& v, float& L, float& R) noexcept
{
    ++v.age;
    const float hz = midiToHz ((float) v.note);
    const double w = 2.0 * M_PI * (double) hz * (double) invSr;

    // ---- Amp ADSR for EP path ----
    bool epAlive = true;
    advanceAdsr (v.epEnv, v.epStage, v.releasing,
                 params.ampAttack,
                 std::max (0.05f, params.epDecay * 0.35f),
                 clamp01 (params.ampSustain),
                 std::max (0.05f, params.ampRelease + params.epDecay * 0.25f),
                 invSr, epAlive);

    // ---- Pad ADSR ----
    bool padAlive = true;
    advanceAdsr (v.padEnv, v.padStage, v.releasing,
                 params.padAttack, params.padDecay, params.padSustain, params.padRelease,
                 invSr, padAlive);

    if (! epAlive && ! padAlive && v.sparkEnv < 0.001f && v.subEnv < 0.001f)
    {
        v.active = false;
        return;
    }
    // Keep voice if any layer still sounding
    if (! epAlive && ! padAlive)
    {
        // allow sparkle/sub to finish
        if (v.sparkEnv < 0.001f && v.subEnv < 0.001f)
        {
            v.active = false;
            return;
        }
    }

    float outL = 0, outR = 0;

    // ========== EP / Rhodes-ish (FM tine + sine body) ==========
    if (params.epOn && v.epEnv > 0.0001f)
    {
        const float fmAmt = clamp01 (params.epFm);
        v.epMod += w * (6.0 + fmAmt * 4.0); // classic EP modulator ratio ~7ish soft
        const float mod = fastSin (v.epMod) * (1.2f + fmAmt * 3.5f) * (0.4f + 0.6f * v.epEnv);
        v.epPhase += w + (double) (mod * fmAmt * 0.35f);
        v.epBellPhase += w * 14.0;

        float body = fastSin (v.epPhase);
        float tine = fastSin (v.epPhase + (double) mod) * 0.55f
                   + fastSin (v.epBellPhase) * clamp01 (params.epBell) * 0.35f * v.epEnv;
        float ep = body * 0.65f + tine;

        // tone + soft drive
        ep = v.epLp.process (ep);
        const float drive = 1.0f + clamp01 (params.epDrive) * 2.8f;
        ep = softClip (ep * drive) * (0.85f / (0.55f + clamp01 (params.epDrive) * 0.4f));
        ep = lerp (softClip (ep * 0.5f), ep, clamp01 (params.epTone));

        // tremolo
        v.tremPhase += 2.0f * (float) M_PI * std::max (0.1f, params.epTremRate) * invSr;
        const float trem = 1.0f - clamp01 (params.epTrem) * 0.5f * (1.0f - std::cos (v.tremPhase));
        ep *= v.epEnv * v.vel * trem * 0.55f;

        const float g = dbToGain (params.epGainDb);
        outL += ep * g * 0.98f;
        outR += ep * g * 1.02f;
    }

    // ========== Warm pad (detuned saw/triangle) ==========
    if (params.padOn && v.padEnv > 0.0001f)
    {
        const float det = clamp01 (params.padDetune) * 0.018f; // fraction of freq
        const double wA = w * (1.0 - det);
        const double wB = w * (1.0 + det * 1.15);
        v.padPhaseA += wA;
        v.padPhaseB += wB;

        auto shape = [&] (double phase) -> float
        {
            float ph01 = (float) std::fmod (phase / (2.0 * M_PI) + 10.0, 1.0);
            float tri = 1.0f - 4.0f * std::abs (ph01 - 0.5f); // -1..1 triangle-ish
            float saw = 2.0f * ph01 - 1.0f;
            return lerp (tri, softClip (saw * 1.1f), clamp01 (params.padShape));
        };

        float pad = 0.5f * (shape (v.padPhaseA) + shape (v.padPhaseB));
        pad = v.padLp.process (pad);
        pad *= v.padEnv * v.vel * 0.38f;
        const float g = dbToGain (params.padGainDb);
        // wide stereo from detune
        outL += pad * g * 0.92f;
        outR += pad * g * 1.08f;
    }

    // ========== Sparkle / bell oneshot ==========
    if (params.sparkleOn && v.sparkEnv > 0.0001f)
    {
        const float dec = std::max (0.05f, params.sparkleDecay);
        v.sparkEnv *= std::exp (-invSr * (3.2f / dec));
        const float ratio = 3.01f + clamp01 (params.sparkleTone) * 4.0f;
        v.sparkMod += w * (double) ratio;
        v.sparkPhase += w * 2.0 + (double) (fastSin (v.sparkMod) * 2.5f);
        float bell = fastSin (v.sparkPhase) * 0.6f + fastSin (v.sparkMod) * 0.4f;
        bell *= v.sparkEnv * clamp01 (params.sparkleMix) * v.vel * 0.35f;
        const float g = dbToGain (params.sparkleGainDb);
        outL += bell * g * 1.05f;
        outR += bell * g * 0.95f;
    }
    else
    {
        v.sparkEnv = 0;
    }

    // ========== Soft sub ==========
    if (params.subOn)
    {
        const float split = params.subSplit;
        float keyAmt = 1.0f - clamp01 (((float) v.note - (split - 12.0f)) / 24.0f);
        keyAmt = clamp01 (keyAmt);
        if (v.releasing)
            v.subEnv *= std::exp (-invSr * 2.5f);
        else
            v.subEnv = std::min (1.0f, v.subEnv + invSr * 8.0f);

        if (keyAmt > 0.01f && v.subEnv > 0.0001f)
        {
            const float subHz = hz * 0.5f;
            v.subPhase += 2.0 * M_PI * (double) subHz * (double) invSr;
            float sub = fastSin (v.subPhase);
            // gentle 2nd
            sub += 0.25f * fastSin (v.subPhase * 2.0) * clamp01 (params.subTone);
            sub = softClip (sub * 1.1f);
            sub *= v.subEnv * keyAmt * clamp01 (params.subMix) * v.vel * 0.55f;
            const float g = dbToGain (params.subGainDb);
            outL += sub * g;
            outR += sub * g;
        }
    }
    else
    {
        v.subEnv = 0;
    }

    // Per-voice soft filter (resonance as mild feedback boost)
    const float res = clamp01 (params.resonance);
    float fL = v.voiceLp.process (outL + outL * res * 0.15f);
    float fR = v.voiceLp.process (outR + outR * res * 0.15f); // shared LP state ok for soft keys

    L += fL;
    R += fR;
}

void VelvetEngine::process (float* left, float* right, int numSamples) noexcept
{
    applyMacroTargets();

    float peak = 0;
    for (int i = 0; i < numSamples; ++i)
    {
        float L = 0, R = 0;
        for (auto& v : voices)
            if (v.active)
                renderVoice (v, L, R);

        // Dust / vinyl character
        const float dust = dustSm.next();
        if (dust > 0.001f)
        {
            float n = dustNoise() * dust * 0.045f;
            L += n; R += n * 0.92f;
        }

        // Glow: blend in high-passed air
        const float glow = glowSm.next();
        if (glow > 0.001f)
        {
            float aL = airHpL.process (L);
            float aR = airHpR.process (R);
            L += aL * glow * 0.55f;
            R += aR * glow * 0.55f;
        }

        // Soft bus saturation for expensive feel
        L = softClip (L * 1.15f) * 0.92f;
        R = softClip (R * 1.15f) * 0.92f;

        // Space (plate-ish multitap)
        const float space = spaceSm.next();
        delayL[(size_t) delayPos] = L;
        delayR[(size_t) delayPos] = R;
        const int t1 = (delayPos + kDelayLen - (int) (0.023 * sr)) % kDelayLen;
        const int t2 = (delayPos + kDelayLen - (int) (0.041 * sr)) % kDelayLen;
        const int t3 = (delayPos + kDelayLen - (int) (0.067 * sr)) % kDelayLen;
        const int t4 = (delayPos + kDelayLen - (int) (0.097 * sr)) % kDelayLen;
        float wetL = delayL[(size_t) t1] * 0.40f + delayR[(size_t) t2] * 0.28f
                   + delayL[(size_t) t3] * 0.20f + delayR[(size_t) t4] * 0.12f;
        float wetR = delayR[(size_t) t1] * 0.40f + delayL[(size_t) t2] * 0.28f
                   + delayR[(size_t) t3] * 0.20f + delayL[(size_t) t4] * 0.12f;
        L = lerp (L, L + wetL * 0.85f, space);
        R = lerp (R, R + wetR * 0.85f, space);
        delayPos = (delayPos + 1) % kDelayLen;

        // Width
        const float width = widthSm.next();
        float mid = 0.5f * (L + R);
        float side = 0.5f * (L - R) * (1.0f + width * 1.9f);
        L = mid + side;
        R = mid - side;

        // Velvet master LPF + gain
        velvetSm.next();
        cutSm.next();
        L = masterLpL.process (L);
        R = masterLpR.process (R);
        const float mg = masterSm.next();
        L *= mg; R *= mg;

        left[i] = L;
        right[i] = R;
        peak = std::max (peak, std::max (std::abs (L), std::abs (R)));
    }
    meter = peak;
}

} // namespace chaz
