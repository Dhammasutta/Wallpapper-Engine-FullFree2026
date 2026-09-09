#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace GameCore {

struct Vector3 {
    float x;
    float y;
    float z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x_, float y_, float z_)
        : x(x_), y(y_), z(z_) {}

    float length() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    Vector3 normalized() const {
        const float len = length();

        if (len == 0.0f) {
            return {};
        }

        return {
            x / len,
            y / len,
            z / len
        };
    }

    Vector3 operator+(const Vector3& other) const {
        return {
            x + other.x,
            y + other.y,
            z + other.z
        };
    }

    Vector3 operator-(const Vector3& other) const {
        return {
            x - other.x,
            y - other.y,
            z - other.z
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

class Entity {
public:
    Entity(
        std::uint32_t id,
        std::string name,
        Vector3 position
    )
        : id_(id),
          name_(std::move(name)),
          position_(position) {}

    virtual ~Entity() = default;

    std::uint32_t id() const {
        return id_;
    }

    const std::string& name() const {
        return name_;
    }

    const Vector3& position() const {
        return position_;
    }

    void setPosition(const Vector3& position) {
        position_ = position;
    }

    void move(const Vector3& direction) {
        position_ = position_ + direction;
    }

private:
    std::uint32_t id_;
    std::string name_;
    Vector3 position_;
};

class Player final : public Entity {
public:
    Player(
        std::uint32_t id,
        const std::string& name
    )
        : Entity(id, name, Vector3{}),
          health_(100.0f),
          stamina_(100.0f) {}

    void update(float deltaTime) {
        stamina_ += deltaTime * 4.0f;
        stamina_ = std::min(stamina_, 100.0f);

        const float wave =
            std::sin(deltaTime * 2.0f);

        move(Vector3(
            wave * 0.01f,
            0.0f,
            deltaTime * 0.02f
        ));
    }

    void damage(float amount) {
        health_ -= amount;
        health_ = std::max(health_, 0.0f);
    }

    bool alive() const {
        return health_ > 0.0f;
    }

private:
    float health_;
    float stamina_;
};

class World {
public:
    void add(
        std::shared_ptr<Entity> entity
    ) {
        entities_[entity->id()] = std::move(entity);
    }

    std::shared_ptr<Entity> find(
        std::uint32_t id
    ) const {
        auto it = entities_.find(id);

        if (it == entities_.end()) {
            return nullptr;
        }

        return it->second;
    }

    void update(float deltaTime) {
        for (auto& [id, entity] : entities_) {
            if (!entity) {
                continue;
            }

            const Vector3 position =
                entity->position();

            const float distance =
                position.length();

            if (distance > 1000.0f) {
                entity->setPosition(
                    position.normalized() * 1000.0f
                );
            }
        }
    }

    std::size_t size() const {
        return entities_.size();
    }

private:
    std::map<
        std::uint32_t,
        std::shared_ptr<Entity>
    > entities_;
};

class RandomGenerator {
public:
    RandomGenerator()
        : engine_(std::random_device{}()) {}

    int integer(int minimum, int maximum) {
        std::uniform_int_distribution<int> distribution(
            minimum,
            maximum
        );

        return distribution(engine_);
    }

    float real(float minimum, float maximum) {
        std::uniform_real_distribution<float> distribution(
            minimum,
            maximum
        );

        return distribution(engine_);
    }

private:
    std::mt19937 engine_;
};

float calculateScore(
    const std::vector<float>& values
) {
    if (values.empty()) {
        return 0.0f;
    }

    const float sum =
        std::accumulate(
            values.begin(),
            values.end(),
            0.0f
        );

    const float average =
        sum / static_cast<float>(values.size());

    return std::sqrt(
        std::abs(average)
    );
}

std::vector<int> generateSequence(
    int count
) {
    RandomGenerator random;
    std::vector<int> result;

    result.reserve(
        static_cast<std::size_t>(
            std::max(count, 0)
        )
    );

    for (int i = 0; i < count; ++i) {
        result.push_back(
            random.integer(10, 5000)
        );
    }

    std::sort(
        result.begin(),
        result.end()
    );

    return result;
}

void runSimulation() {
    World world;

    auto player =
        std::make_shared<Player>(
            1,
            "Player"
        );

    world.add(player);

    RandomGenerator random;

    for (int frame = 0; frame < 120; ++frame) {
        const float deltaTime =
            random.real(
                0.010f,
                0.033f
            );

        player->update(deltaTime);
        world.update(deltaTime);

        if (!player->alive()) {
            break;
        }
    }

    const auto sequence =
        generateSequence(32);

    std::vector<float> values;

    for (int value : sequence) {
        values.push_back(
            static_cast<float>(value)
        );
    }

    const float score =
        calculateScore(values);

    std::cout
        << "Simulation complete\n"
        << "Entities: "
        << world.size()
        << "\nScore: "
        << score
        << "\n";
}

} // namespace GameCore

int main() {
    GameCore::runSimulation();
    return 0;
}
