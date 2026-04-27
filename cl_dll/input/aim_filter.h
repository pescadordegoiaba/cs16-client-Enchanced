// aim_filter.h
// CS16Client RawAim Direct Patch
// Objetivo: deixar a mira mais direta/responsiva por padrão.
// Defaults: filtro OFF, aceleração OFF, buffer relativo ON, drop do primeiro delta pós-foco ON.
//
// Como usar no backend FWGS:
//   - include "aim_filter.h"
//   - chame RawAim_RegisterCVars() no IN_Init()
//   - chame RawAim_DropNext() no IN_ActivateMouse()/IN_DeactivateMouse()/mudança de foco
//   - em IN_ClientLookEvent(): se RawAim_IsEnabled(), use RawAim_PushSample(...), senão fallback rel_yaw += ...
//   - no começo de IN_Move(): RawAim_ConsumeToLegacy(&rel_yaw, &rel_pitch, frametime)

#pragma once

#include <math.h>
#include <stddef.h>

#ifndef RAWAIM_QUEUE_SIZE
#define RAWAIM_QUEUE_SIZE 256
#endif

#ifndef RAWAIM_PI
#define RAWAIM_PI 3.14159265358979323846f
#endif

// Estes símbolos normalmente já existem no client antes deste header ser incluído:
//   - cvar_t
//   - gEngfuncs
//   - FCVAR_ARCHIVE

static cvar_t* rawaim_enable = NULL;              // 1 = usar caminho direto com fila; 0 = fallback original
static cvar_t* rawaim_filter = NULL;              // 0 = off; 1 = EMA; 2 = OneEuro. Default: OFF para 0% smooth.
static cvar_t* rawaim_tau = NULL;                 // EMA tau, só usado se rawaim_filter=1.
static cvar_t* rawaim_1e_min_cutoff = NULL;       // OneEuro, só usado se rawaim_filter=2.
static cvar_t* rawaim_1e_beta = NULL;
static cvar_t* rawaim_1e_dcutoff = NULL;
static cvar_t* rawaim_accel = NULL;               // 0 por padrão: sem aceleração.
static cvar_t* rawaim_accel_scale = NULL;
static cvar_t* rawaim_accel_exp = NULL;
static cvar_t* rawaim_accel_cap = NULL;
static cvar_t* rawaim_accel_threshold = NULL;
static cvar_t* rawaim_drop_next_delta = NULL;     // evita tranco depois de abrir menu/alt-tab/foco.
static cvar_t* rawaim_debug = NULL;

struct RawAimLP1
{
	bool initialized;
	float y;

	RawAimLP1() : initialized(false), y(0.0f) {}

	static float MaxF(float a, float b) { return a > b ? a : b; }
	static float AlphaFromCutoff(float cutoffHz, float dt)
	{
		cutoffHz = MaxF(cutoffHz, 0.001f);
		dt = MaxF(dt, 0.000001f);
		const float tau = 1.0f / (2.0f * RAWAIM_PI * cutoffHz);
		return dt / (dt + tau);
	}

	float Apply(float x, float a)
	{
		if (!initialized)
		{
			initialized = true;
			y = x;
			return y;
		}

		y += a * (x - y);
		return y;
	}

	void Reset()
	{
		initialized = false;
		y = 0.0f;
	}
};

struct RawAimOneEuroAxis
{
	RawAimLP1 xFilt;
	RawAimLP1 dxFilt;
	float prev;
	bool hasPrev;

	RawAimOneEuroAxis() : prev(0.0f), hasPrev(false) {}

	float Apply(float x, float dt, float minCutoff, float beta, float dCutoff)
	{
		float dx = 0.0f;
		if (hasPrev && dt > 0.0f)
			dx = (x - prev) / dt;

		prev = x;
		hasPrev = true;

		const float edx = dxFilt.Apply(dx, RawAimLP1::AlphaFromCutoff(dCutoff, dt));
		const float cutoff = minCutoff + beta * fabsf(edx);
		return xFilt.Apply(x, RawAimLP1::AlphaFromCutoff(cutoff, dt));
	}

	void Reset()
	{
		xFilt.Reset();
		dxFilt.Reset();
		prev = 0.0f;
		hasPrev = false;
	}
};

struct RawAimConditioner
{
	bool emaInit;
	float emaYaw;
	float emaPitch;
	RawAimOneEuroAxis euroYaw;
	RawAimOneEuroAxis euroPitch;

	RawAimConditioner() : emaInit(false), emaYaw(0.0f), emaPitch(0.0f) {}

	void Reset()
	{
		emaInit = false;
		emaYaw = 0.0f;
		emaPitch = 0.0f;
		euroYaw.Reset();
		euroPitch.Reset();
	}

	void ApplyEMA(float& yaw, float& pitch, float dt, float tauSec)
	{
		if (tauSec < 0.001f)
			tauSec = 0.001f;

		const float a = 1.0f - expf(-dt / tauSec);

		if (!emaInit)
		{
			emaInit = true;
			emaYaw = yaw;
			emaPitch = pitch;
		}
		else
		{
			emaYaw += a * (yaw - emaYaw);
			emaPitch += a * (pitch - emaPitch);
		}

		yaw = emaYaw;
		pitch = emaPitch;
	}

	void ApplyOneEuro(float& yaw, float& pitch, float dt, float minCutoff, float beta, float dCutoff)
	{
		yaw = euroYaw.Apply(yaw, dt, minCutoff, beta, dCutoff);
		pitch = euroPitch.Apply(pitch, dt, minCutoff, beta, dCutoff);
	}
};

struct RawAimSample
{
	float yaw;
	float pitch;
	double time;
};

static RawAimSample rawaim_queue[RAWAIM_QUEUE_SIZE];
static int rawaim_head = 0;
static int rawaim_tail = 0;
static bool rawaim_drop_next = true;
static RawAimConditioner rawaim_conditioner;

static inline int RawAim_NextIndex(int index)
{
	return (index + 1) % RAWAIM_QUEUE_SIZE;
}

static inline bool RawAim_IsEnabled()
{
	return rawaim_enable == NULL || rawaim_enable->value != 0.0f;
}

static inline int RawAim_FilterMode()
{
	return rawaim_filter ? (int)rawaim_filter->value : 0;
}

static inline void RawAim_ClearQueue()
{
	rawaim_head = 0;
	rawaim_tail = 0;
}

static inline void RawAim_Reset()
{
	RawAim_ClearQueue();
	rawaim_conditioner.Reset();
}

static inline void RawAim_DropNext()
{
	if (rawaim_drop_next_delta == NULL || rawaim_drop_next_delta->value != 0.0f)
		rawaim_drop_next = true;

	RawAim_Reset();
}

static inline void RawAim_RegisterCVars()
{
	// cl_aim_raw_direct=1: usa uma fila de deltas relativos e entrega direto para o IN_Move.
	// cl_aim_filter=0: sem EMA/OneEuro. É o preset competitivo: 0% smooth.
	rawaim_enable = gEngfuncs.pfnRegisterVariable("cl_aim_raw_direct", "1", FCVAR_ARCHIVE);
	rawaim_filter = gEngfuncs.pfnRegisterVariable("cl_aim_filter", "0", FCVAR_ARCHIVE);
	rawaim_tau = gEngfuncs.pfnRegisterVariable("cl_aim_tau", "0.003", FCVAR_ARCHIVE);
	rawaim_1e_min_cutoff = gEngfuncs.pfnRegisterVariable("cl_aim_1e_min_cutoff", "2.5", FCVAR_ARCHIVE);
	rawaim_1e_beta = gEngfuncs.pfnRegisterVariable("cl_aim_1e_beta", "0.30", FCVAR_ARCHIVE);
	rawaim_1e_dcutoff = gEngfuncs.pfnRegisterVariable("cl_aim_1e_dcutoff", "1.0", FCVAR_ARCHIVE);
	rawaim_accel = gEngfuncs.pfnRegisterVariable("cl_aim_accel", "0", FCVAR_ARCHIVE);
	rawaim_accel_scale = gEngfuncs.pfnRegisterVariable("cl_aim_accel_scale", "0.05", FCVAR_ARCHIVE);
	rawaim_accel_exp = gEngfuncs.pfnRegisterVariable("cl_aim_accel_exp", "1.5", FCVAR_ARCHIVE);
	rawaim_accel_cap = gEngfuncs.pfnRegisterVariable("cl_aim_accel_cap", "2.0", FCVAR_ARCHIVE);
	rawaim_accel_threshold = gEngfuncs.pfnRegisterVariable("cl_aim_accel_threshold", "0.25", FCVAR_ARCHIVE);
	rawaim_drop_next_delta = gEngfuncs.pfnRegisterVariable("cl_aim_drop_next_delta", "1", FCVAR_ARCHIVE);
	rawaim_debug = gEngfuncs.pfnRegisterVariable("cl_aim_debug", "0", FCVAR_ARCHIVE);
}

static inline void RawAim_PushSample(float yaw, float pitch, double time)
{
	if (rawaim_drop_next && (rawaim_drop_next_delta == NULL || rawaim_drop_next_delta->value != 0.0f))
	{
		rawaim_drop_next = false;
		return;
	}

	const int next = RawAim_NextIndex(rawaim_head);
	if (next == rawaim_tail)
		rawaim_tail = RawAim_NextIndex(rawaim_tail); // fila cheia: descarta o mais antigo

	rawaim_queue[rawaim_head].yaw = yaw;
	rawaim_queue[rawaim_head].pitch = pitch;
	rawaim_queue[rawaim_head].time = time;
	rawaim_head = next;
}

static inline bool RawAim_PopSample(RawAimSample& sample)
{
	if (rawaim_tail == rawaim_head)
		return false;

	sample = rawaim_queue[rawaim_tail];
	rawaim_tail = RawAim_NextIndex(rawaim_tail);
	return true;
}

static inline float RawAim_AccelGain(float yaw, float pitch, float dt)
{
	if (rawaim_accel == NULL || rawaim_accel->value == 0.0f || dt <= 0.0f)
		return 1.0f;

	const float speed = sqrtf(yaw * yaw + pitch * pitch) / dt;
	const float threshold = rawaim_accel_threshold ? rawaim_accel_threshold->value : 0.25f;

	if (speed <= threshold)
		return 1.0f;

	const float scale = rawaim_accel_scale ? rawaim_accel_scale->value : 0.05f;
	float exponent = rawaim_accel_exp ? rawaim_accel_exp->value : 1.5f;
	if (exponent < 1.0f)
		exponent = 1.0f;

	float gain = 1.0f + scale * powf(speed - threshold, exponent);
	float cap = rawaim_accel_cap ? rawaim_accel_cap->value : 2.0f;
	if (cap < 1.0f)
		cap = 1.0f;

	return gain > cap ? cap : gain;
}

static inline float RawAim_ClampDt(float dt)
{
	if (dt < 0.001f)
		return 0.001f;
	if (dt > 0.050f)
		return 0.050f;
	return dt;
}

static inline void RawAim_ConditionSample(float& yaw, float& pitch, float dt)
{
	const int mode = RawAim_FilterMode();
	if (mode == 1)
	{
		const float tau = rawaim_tau ? rawaim_tau->value : 0.003f;
		rawaim_conditioner.ApplyEMA(yaw, pitch, dt, tau);
	}
	else if (mode == 2)
	{
		const float minCutoff = rawaim_1e_min_cutoff ? rawaim_1e_min_cutoff->value : 2.5f;
		const float beta = rawaim_1e_beta ? rawaim_1e_beta->value : 0.30f;
		const float dCutoff = rawaim_1e_dcutoff ? rawaim_1e_dcutoff->value : 1.0f;
		rawaim_conditioner.ApplyOneEuro(yaw, pitch, dt, minCutoff, beta, dCutoff);
	}
}

static inline void RawAim_ConsumeToLegacy(float* rel_yaw, float* rel_pitch, float frametime)
{
	if (!RawAim_IsEnabled() || rel_yaw == NULL || rel_pitch == NULL)
		return;

	float outYaw = 0.0f;
	float outPitch = 0.0f;
	RawAimSample sample;
	double prevTime = -1.0;

	while (RawAim_PopSample(sample))
	{
		float dt = (prevTime < 0.0) ? frametime : (float)(sample.time - prevTime);
		prevTime = sample.time;
		dt = RawAim_ClampDt(dt);

		float yaw = sample.yaw;
		float pitch = sample.pitch;

		// Default competitivo: RawAim_FilterMode() == 0, então esta chamada não suaviza nada.
		RawAim_ConditionSample(yaw, pitch, dt);

		const float gain = RawAim_AccelGain(yaw, pitch, dt);
		outYaw += yaw * gain;
		outPitch += pitch * gain;
	}

	// Entrega para o código original como se fossem os deltas acumulados daquele frame.
	// Não multiplica por frametime: delta relativo é deslocamento, não velocidade.
	*rel_yaw = outYaw;
	*rel_pitch = outPitch;
}
