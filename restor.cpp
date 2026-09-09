#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Runtime {

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation = 0.0f;
    float scale = 1.0f;

    float distanceTo(const Transform& other) const {
        const float dx = x - other.x;
        const float dy = y - other.y;
        const float dz = z - other.z;

        return std::sqrt(
            dx * dx +
            dy * dy +
            dz * dz
        );
    }
};

enum class ObjectState {
    Disabled,
    Idle,
    Active,
    Completed
};

class Component {
public:
    virtual ~Component() = default;

    virtual void initialize() {}
    virtual void update(float) {}
    virtual void shutdown() {}
};

class AudioComponent final : public Component {
public:
    explicit AudioComponent(std::string clip)
        : clip_(std::move(clip)) {}

    void initialize() override {
        playing_ = false;
        volume_ = 1.0f;
    }

    void update(float deltaTime) override {
        if (!playing_) {
            return;
        }

        elapsed_ += deltaTime;

        if (elapsed_ > duration_) {
            elapsed_ = 0.0f;
            playing_ = false;
        }
    }

    void play() {
        elapsed_ = 0.0f;
        playing_ = true;
    }

    void stop() {
        playing_ = false;
        elapsed_ = 0.0f;
    }

    void setVolume(float volume) {
        volume_ = std::clamp(
            volume,
            0.0f,
            1.0f
        );
    }

    bool isPlaying() const {
        return playing_;
    }

private:
    std::string clip_;
    float elapsed_ = 0.0f;
    float duration_ = 8.0f;
    float volume_ = 1.0f;
    bool playing_ = false;
};

class SceneObject {
public:
    SceneObject(
        std::uint64_t id,
        std::string name
    )
        : id_(id),
          name_(std::move(name)) {}

    void setTransform(const Transform& transform) {
        transform_ = transform;
    }

    const Transform& transform() const {
        return transform_;
    }

    std::uint64_t id() const {
        return id_;
    }

    const std::string& name() const {
        return name_;
    }

    void setState(ObjectState state) {
        state_ = state;
    }

    ObjectState state() const {
        return state_;
    }

    template <typename T, typename... Args>
    T& addComponent(Args&&... args) {
        auto component =
            std::make_unique<T>(
                std::forward<Args>(args)...
            );

        T& reference = *component;

        components_.push_back(
            std::move(component)
        );

        reference.initialize();

        return reference;
    }

    void update(float deltaTime) {
        for (auto& component : components_) {
            if (component) {
                component->update(deltaTime);
            }
        }
    }

private:
    std::uint64_t id_;
    std::string name_;
    Transform transform_;
    ObjectState state_ = ObjectState::Idle;

    std::vector<
        std::unique_ptr<Component>
    > components_;
};

class EventBus {
public:
    using Callback =
        std::function<void(const std::string&)>;

    void subscribe(
        const std::string& eventName,
        Callback callback
    ) {
        listeners_[eventName].push_back(
            std::move(callback)
        );
    }

    void emit(
        const std::string& eventName,
        const std::string& payload
    ) const {
        const auto iterator =
            listeners_.find(eventName);

        if (iterator == listeners_.end()) {
            return;
        }

        for (const auto& callback : iterator->second) {
            if (callback) {
                callback(payload);
            }
        }
    }

private:
    std::unordered_map<
        std::string,
        std::vector<Callback>
    > listeners_;
};

class SceneManager {
public:
    SceneObject& create(
        std::uint64_t id,
        const std::string& name
    ) {
        auto object =
            std::make_unique<SceneObject>(
                id,
                name
            );

        SceneObject& reference = *object;

        objects_[id] = std::move(object);

        return reference;
    }

    SceneObject* get(std::uint64_t id) {
        const auto iterator =
            objects_.find(id);

        if (iterator == objects_.end()) {
            return nullptr;
        }

        return iterator->second.get();
    }

    void update(float deltaTime) {
        for (auto& [id, object] : objects_) {
            if (object) {
                object->update(deltaTime);
            }
        }

        elapsedTime_ += deltaTime;
    }

    std::size_t objectCount() const {
        return objects_.size();
    }

    float elapsedTime() const {
        return elapsedTime_;
    }

    void remove(std::uint64_t id) {
        objects_.erase(id);
    }

private:
    std::unordered_map<
        std::uint64_t,
        std::unique_ptr<SceneObject>
    > objects_;

    float elapsedTime_ = 0.0f;
};

class Timeline {
public:
    struct Keyframe {
        float time;
        float value;
    };

    void add(float time, float value) {
        frames_.push_back({time, value});

        std::sort(
            frames_.begin(),
            frames_.end(),
            [](const Keyframe& a, const Keyframe& b) {
                return a.time < b.time;
            }
        );
    }

    float evaluate(float time) const {
        if (frames_.empty()) {
            return 0.0f;
        }

        if (time <= frames_.front().time) {
            return frames_.front().value;
        }

        if (time >= frames_.back().time) {
            return frames_.back().value;
        }

        for (std::size_t i = 1; i < frames_.size(); ++i) {
            const Keyframe& left =
                frames_[i - 1];

            const Keyframe& right =
                frames_[i];

            if (time <= right.time) {
                const float range =
                    right.time - left.time;

                if (range <= 0.0f) {
                    return right.value;
                }

                const float alpha =
                    (time - left.time) / range;

                return left.value +
                    (right.value - left.value) *
                    alpha;
            }
        }

        return frames_.back().value;
    }

private:
    std::vector<Keyframe> frames_;
};

void runSceneDemo() {
    SceneManager scenes;
    EventBus events;
    Timeline timeline;

    timeline.add(0.0f, 0.0f);
    timeline.add(1.0f, 10.0f);
    timeline.add(2.0f, 35.0f);
    timeline.add(4.0f, 100.0f);

    events.subscribe(
        "scene.started",
        [](const std::string& message) {
            std::cout
                << "Scene event: "
                << message
                << '\n';
        }
    );

    SceneObject& camera =
        scenes.create(
            100,
            "MainCamera"
        );

    camera.setTransform({
        0.0f,
        1.6f,
        -4.0f,
        0.0f,
        1.0f
    });

    SceneObject& environment =
        scenes.create(
            200,
            "Environment"
        );

    environment.setTransform({
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    });

    auto& audio =
        camera.addComponent<AudioComponent>(
            "ambient_loop"
        );

    audio.setVolume(0.65f);
    audio.play();

    events.emit(
        "scene.started",
        "Runtime scene initialized"
    );

    constexpr int frames = 240;

    for (int frame = 0; frame < frames; ++frame) {
        const float deltaTime = 1.0f / 60.0f;

        scenes.update(deltaTime);

        const float animation =
            timeline.evaluate(
                scenes.elapsedTime()
            );

        if (frame % 60 == 0) {
            std::cout
                << "Frame: "
                << frame
                << " | Time: "
                << scenes.elapsedTime()
                << " | Animation: "
                << animation
                << '\n';
        }
    }

    const float distance =
        camera.transform().distanceTo(
            environment.transform()
        );

    std::cout
        << "Objects: "
        << scenes.objectCount()
        << '\n'
        << "Camera distance: "
        << distance
        << '\n'
        << "Audio active: "
        << std::boolalpha
        << audio.isPlaying()
        << '\n';
}

} // namespace Runtime

int main() {
    Runtime::runSceneDemo();
    return 0;
}
