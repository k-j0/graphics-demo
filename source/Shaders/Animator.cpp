#include "Animator.h"



Animator::Animator(FbxTime& start, FbxTime& end) : start(start), end(end) {
}


Animator::~Animator(){
}

void Animator::update(float dt) {
	//modify weight to transition between animations
	if (anim1) {
		weight -= dt * transitionTime;
		if (weight <= 0) {
			weight = 1;
			anim0 = anim1;
			current0 = current1;
			anim1 = nullptr;
		}
	}

	//increment times for animations
	FbxTime increment;
	increment.SetSecondDouble(dt);
	//Note: it's assumed that anim0 is never null when coming to Animator::update(). At least one animation should have been transitioned to in 0 seconds.
	current0 += increment;
	if (current0 > anim0->end)
		current0 = anim0->start;
	if (anim1) {
		current1 += increment;
		if (current1 > anim1->end)
			current1 = anim1->start;
	}

}

void Animator::transitionTo(Animation* anim, float transitionTime) {
	this->transitionTime = transitionTime;
	if (transitionTime == 0) {
		anim0 = anim;
		weight = 1;
		current0 = anim->start;
	}
	else {
		anim1 = anim;
		weight = 1;//will slowly go to 0; once reaching 0, anim1 will become anim0
		current1 = anim->start;
	}
}