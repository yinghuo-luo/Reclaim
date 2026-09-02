// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ReclaimDeterministicRandom.h"

namespace
{
	uint32 MixSeed(uint32 Value)
	{
		Value ^= Value >> 16;
		Value *= 0x7feb352du;
		Value ^= Value >> 15;
		Value *= 0x846ca68bu;
		Value ^= Value >> 16;
		return Value;
	}

	uint32 HashCombineStable(uint32 A, uint32 B)
	{
		return MixSeed(A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2)));
	}
}

int32 UReclaimDeterministicRandom::MakeSubSeed(int32 RunSeed, EReclaimRandomDomain Domain, int32 StableA, int32 StableB, int32 StableC)
{
	uint32 Seed = static_cast<uint32>(RunSeed);
	Seed = HashCombineStable(Seed, static_cast<uint32>(Domain));
	Seed = HashCombineStable(Seed, static_cast<uint32>(StableA));
	Seed = HashCombineStable(Seed, static_cast<uint32>(StableB));
	Seed = HashCombineStable(Seed, static_cast<uint32>(StableC));
	return static_cast<int32>(Seed & 0x7fffffffu);
}

FRandomStream UReclaimDeterministicRandom::MakeStream(int32 RunSeed, EReclaimRandomDomain Domain, int32 StableA, int32 StableB, int32 StableC)
{
	return FRandomStream(MakeSubSeed(RunSeed, Domain, StableA, StableB, StableC));
}
