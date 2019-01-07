#pragma once

///Manages animations and animation transitions

#include <fbxsdk.h>

class Animator;

class Animation {
	friend class Animator;

protected:
	FbxTime start;
	FbxTime end;

public:
	inline Animation(float from, float to) {
		start.SetSecondDouble(from);
		end.SetSecondDouble(to);
	}
};

class Animator{

protected:
	FbxTime start;
	FbxTime end;
	FbxTime current0;
	FbxTime current1;
	float weight = 1;//how much current0 should weigh over current1; at 1, the only animation shown is the one pointed to by current0; at 0, the only animation shown is current1; anything in between is a blend.
	float transitionTime = 0;//In how many seconds weight should go from 1 to 0

	Animation* anim0;//the current animation at slot 0, controlled by current0
	Animation* anim1 = nullptr;//the current animation at slot 1, controlled by current1

public:
	Animator(FbxTime& start, FbxTime& end);
	~Animator();

	void update(float dt);

	inline FbxTime& getCurrent0() { return current0; }
	inline FbxTime& getCurrent1() { return current1; }
	inline float getWeight0() { return weight; }

	///Transitions between anim0 and the anim passed within the timeframe presented.
	void transitionTo(Animation* anim, float transitionTime);
};

