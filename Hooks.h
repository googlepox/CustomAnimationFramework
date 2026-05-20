#pragma once
#include "NiNodes.h"

class AnimSequenceBase
{
public:
	virtual void					Destructor(UInt8 Arg);
	virtual void					AddAnimGroupSequence(BSAnimGroupSequence* AnimGroupSequence);
	virtual void					Unk_02();
	virtual UInt8					IsSingle();
	virtual BSAnimGroupSequence* GetAnimGroupSequence(int Index); // Index is not used if Single (returns the anim); Index = -1 returns a random anim in the NiTList<BSAnimGroupSequence>* for Multiple
	virtual void					Unk_05();
};

class AnimSequenceSingle : public AnimSequenceBase
{
public:
	BSAnimGroupSequence* Anim;		// 04
};

class AnimSequenceSingleEx : public AnimSequenceSingle
{
public:
	BSAnimGroupSequence* ORAnim;		// 0C
};

class AnimSequenceMultiple : public AnimSequenceBase
{
public:
	NiTListBase<BSAnimGroupSequence>* Anims;	// 04
};

class AnimSequenceMultipleEx : public AnimSequenceMultiple
{
public:
	BSAnimGroupSequence* ORAnim;		// 0C
};

void Install();

// Credits to lStewieAl
[[nodiscard]] UInt32 __stdcall DetourVtable(UInt32 addr, UInt32 dst);