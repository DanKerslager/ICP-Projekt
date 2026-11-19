#include "PoseSmoother.h"

void PoseSmoother::apply(TrackingResult& result)
{
    // time step from capture times: correct also when frames were skipped
    auto time = result.frame->time;
    float dt = last_time == decltype(last_time){} ? 0.0f : std::chrono::duration<float>(time - last_time).count();
    if (dt <= 0.0f)
        dt = 1.0f / 30.0f;
    last_time = time;

    // a lost body/hand gets fresh filters, so a new detection does not slide in from the old position
    if (result.body.valid) {
        for (int k = 0; k < BODY_POINTS; ++k)
            result.body.points[k].pos = body[k].filter(result.body.points[k].pos, dt);
    }
    else {
        body = {};
    }

    for (int i = 0; i < 2; ++i) {
        if (result.hands[i].valid) {
            for (int k = 0; k < HAND_POINTS; ++k)
                result.hands[i].points[k] = hands[i][k].filter(result.hands[i].points[k], dt);
        }
        else {
            hands[i] = {};
        }
    }
}
