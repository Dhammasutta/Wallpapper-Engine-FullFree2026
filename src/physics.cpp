#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace Physics {

struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3 operator+(const Vector3& other) const {
        return {
            x + other.x,
            y + other.y,
            z + other.z
        };
    }

    Vector3 operator*(float value) const {
        return {
            x * value,
            y * value,
            z * value
        };
    }
};

class RigidBody {
public:
    explicit RigidBody(float mass)
        : mass_(std::max(mass, 0.001f)) {}

    void applyForce(const Vector3& force) {
        acceleration_ =
            acceleration_ +
            force * (1.0f / mass_);
    }

    void update(float deltaTime) {
        velocity_ =
            velocity_ +
            acceleration_ * deltaTime;

        position_ =
            position_ +
            velocity_ * deltaTime;

        acceleration_ = {};
    }

    void setPosition(const Vector3& position) {
        position_ = position;
    }

    const Vector3& position() const {
        return position_;
    }

    const Vector3& velocity() const {
        return velocity_;
    }

private:
    float mass_;
    Vector3 position_{};
    Vector3 velocity_{};
    Vector3 acceleration_{};
};

class PhysicsWorld {
public:
    RigidBody& createBody(float mass) {
        bodies_.emplace_back(mass);
        return bodies_.back();
    }

    void simulate(float deltaTime) {
        for (auto& body : bodies_) {
            body.applyForce({
                0.0f,
                -9.81f * 1.0f,
                0.0f
            });

            body.update(deltaTime);

            if (body.position().y < 0.0f) {
                Vector3 corrected =
                    body.position();

                corrected.y = 0.0f;

                body.setPosition(corrected);
            }
        }

        time_ += deltaTime;
    }

    float time() const {
        return time_;
    }

    std::size_t bodyCount() const {
        return bodies_.size();
    }

private:
    std::vector<RigidBody> bodies_;
    float time_ = 0.0f;
};

void runPhysicsDemo() {
    PhysicsWorld world;

    auto& object = world.createBody(5.0f);

    object.setPosition({
        0.0f,
        10.0f,
        0.0f
    });

    for (int i = 0; i < 120; ++i) {
        world.simulate(1.0f / 60.0f);
    }

    std::cout
        << "Physics simulation finished\n"
        << "Bodies: "
        << world.bodyCount()
        << '\n'
        << "Time: "
        << world.time()
        << '\n'
        << "Final height: "
        << object.position().y
        << '\n';
}

}

int main() {
    Physics::runPhysicsDemo();
    return 0;
}
