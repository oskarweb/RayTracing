#pragma once

#include "extras.hpp"
#include "models.hpp"
#include "node.hpp"
#include "renderer_extras.hpp"
#include "trail.hpp"

#include <glm/glm.hpp>

#include <cmath>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

class Particle : public Node
{
public:
    struct State
    {
        Types::Vec3d affectingForce{};
        Types::Vec3d acceleration{};
        Types::Vec3d velocity{};
        Types::Vec3d pos{};
    };

    Particle();
    Particle(double charge, double mass, bool movable, Types::Vec3d pos, Types::Vec3d vel, Types::OdeMethod method);
    Particle(double charge, double mass, bool movable, Types::Vec3d affectingForce, Types::Vec3d pos, Types::Vec3d vel);

    Particle(const Particle &) = delete;
    Particle &operator=(const Particle &) = delete;
    Particle(Particle &&) noexcept = default;
    Particle &operator=(Particle &&) noexcept = default;

    void update();
    void update(Types::Vec3d affectingForce, double time);
    void update(double time);
    void pushState(uint32_t idx);
    void pushState(uint32_t &idx, Types::Vec3d &force, Types::Vec3d &acceleration, Types::Vec3d &velocity,
                   Types::Vec3d &pos);
    void pushState(uint32_t &idx, Types::Vec3d &&force, Types::Vec3d &&acceleration, Types::Vec3d &&velocity,
                   Types::Vec3d &&pos);
    void setInitialState();
    inline void clearStates() { m_states.clear(); }
    bool updateFromPrecalcPos(uint32_t idx);
    void cleanup() override;
    void resetTrail() { m_trail.cleanup(); }

    inline Types::Vec3d getCoulombForce(Particle &other) const
    {
        // F = k * |q1 * q2| / r^2
        Types::Vec3d distanceV = m_pos - other.getPos();
        return COULOMB_CONSTANT * m_charge * other.getCharge() * distanceV.normalized() /
               distanceV.length2(distanceSoftening);
    }

    inline Types::Vec3d getCoulombForce(uint32_t stateIdx, Particle &other, Types::Vec3d distMod)
    {
        Types::Vec3d distanceV = m_states[stateIdx].pos + distMod - other.statesData()[stateIdx].pos;
        return COULOMB_CONSTANT * m_charge * other.getCharge() * distanceV.normalized() /
               distanceV.length2(distanceSoftening);
    }

    inline Types::Vec3d getCoulombForce(uint32_t stateIdx, Particle &other)
    {
        Types::Vec3d distanceV = m_states[stateIdx].pos - other.statesData()[stateIdx].pos;
        return COULOMB_CONSTANT * m_charge * other.getCharge() * distanceV.normalized() /
               distanceV.length2(distanceSoftening);
    }

    inline Types::Vec3d getCoulombForcePosOverwrite(uint32_t stateIdx, Particle &other, Types::Vec3d posOverwrite)
    {
        Types::Vec3d distanceV = posOverwrite - other.statesData()[stateIdx].pos;
        return COULOMB_CONSTANT * m_charge * other.getCharge() * distanceV.normalized() /
               distanceV.length2(distanceSoftening);
    }

    std::mutex &mutexData() { return *m_statesMutex; }

    std::map<uint32_t, State> &statesData() { return m_states; }

    uint32_t getBufferedStepCount() const { return static_cast<uint32_t>(m_states.size()); }
    void setMaxStep(uint32_t maxStep) { *m_maxStep = maxStep; }
    uint32_t getMaxStep() const { return m_maxStep->load(); }
    State *initialStateData() { return m_initialState.get(); }
    State &getInitialState() { return *m_initialState; }
    double &chargeData() { return m_charge; }
    double &massData() { return m_mass; }
    Types::Vec3d &affectingForceData() { return m_affectingForce; }
    Types::Vec3d &accelerationData() { return m_acceleration; }
    Types::Vec3d &velocityData() { return m_velocity; }
    Types::Vec3d &posData() { return m_pos; }
    const double &getCharge() const { return m_charge; }
    const double &getMass() const { return m_mass; }
    const Types::Vec3d &getAffectingForce() const { return m_affectingForce; }
    const Types::Vec3d &getAcceleration() const { return m_acceleration; }
    const Types::Vec3d &getVelocity() const { return m_velocity; }
    const Types::Vec3d &getPos() const { return m_pos; }
    void setCharge(double charge) { m_charge = charge; }
    void setMass(double mass) { m_mass = std::clamp(mass, 0.0, 1000.0); };
    void setAffectingForce(Types::Vec3d affectingForce) { m_affectingForce = affectingForce; }
    void setAcceleration(Types::Vec3d acceleration) { m_acceleration = acceleration; }
    void setVelocity(Types::Vec3d velocity) { m_velocity = velocity; }
    void setPos(Types::Vec3d pos) { m_pos = pos; }

    bool setMovable(bool movable)
    {
        m_movable = movable;
        return m_movable;
    }
    bool &movableData() { return m_movable; }
    bool isMovable() const { return m_movable; }
    const uint64_t &getId() const { return m_id; }
    static void resetId() { nextId = 0; }
    void setMethodMask(Types::OdeMethod method) { m_method = method; }
    const Types::OdeMethod &getMethodMask() const { return m_method; }

    inline static constexpr const double ELECTRIC_CONSTANT = 8.854187817e-12;     // [F / m]
    inline static constexpr const double COULOMB_CONSTANT = 8.9875517873681764e9; // 1 / (4 * pi * ELECTRIC_CONSTANT) [N
                                                                                  // * m^2 / C^2]
    inline static constexpr const double DEFAULT_DISTANCE_SOFTENING = 0.6;
    inline static constexpr const double MIN_DISTANCE_SOFTENING = 0.0;
    inline static constexpr const double MAX_DISTANCE_SOFTENING = 1.0;

    inline static const std::string &F_VECTOR_MODEL_NAME = "forceVector";
    inline static const std::string &P_MODEL_NAME = "particle";
    inline static constexpr const glm::vec3 F_VECTOR_OFFEST = glm::vec3(0.0f, 0.0f, 0.0f);

    inline static double distanceSoftening = DEFAULT_DISTANCE_SOFTENING;

private:
    Types::OdeMethod m_method = Types::OdeMethod::RK4;
    double m_charge;
    double m_mass;
    bool m_movable;
    Types::Vec3d m_pos;
    Types::Vec3d m_velocity;
    Types::Vec3d m_acceleration;
    Types::Vec3d m_affectingForce;

    Trail m_trail;

    std::unique_ptr<State> m_initialState;
    std::map<uint32_t, State> m_states;

    std::unique_ptr<std::mutex> m_statesMutex;

    std::unique_ptr<std::atomic<uint32_t>> m_maxStep = 0;

    uint64_t m_id = 0;
    inline static uint64_t nextId = 0;
};