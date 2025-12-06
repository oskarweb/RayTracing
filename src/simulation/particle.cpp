#include "particle.hpp"

Particle::Particle()
    : m_charge(0.0), m_mass(0.0), m_affectingForce(Types::Vec3d(0.0)),
      m_acceleration(Types::Vec3d(0.0)), m_velocity(Types::Vec3d(0.0)),
      m_pos(Types::Vec3d(0.0)), m_movable(true), m_id(nextId++),
      m_statesMutex(std::make_unique<std::mutex>()),
      m_maxStep(std::make_unique<std::atomic<uint32_t>>(false)),
      m_trail(glm::vec3(0.0f))
{
    uploadModel(P_MODEL_NAME,
                std::make_unique<ParticleModel>(static_cast<glm::vec3>(m_pos)));
    uploadModel(F_VECTOR_MODEL_NAME,
                std::make_unique<VectorArrowModel>(
                    static_cast<glm::vec3>(m_pos),
                    static_cast<glm::vec3>(m_affectingForce), glm::vec3(0.0f)));
    setInitialState();
}

Particle::Particle(double charge, double mass, bool movable, Types::Vec3d pos,
                   Types::Vec3d vel, Types::OdeMethod method)
    : m_charge(charge), m_mass(mass), m_affectingForce(Types::Vec3d(0.0)),
      m_acceleration(Types::Vec3d(0.0)), m_velocity(vel), m_pos(pos),
      m_movable(movable), m_id(nextId++),
      m_statesMutex(std::make_unique<std::mutex>()),
      m_maxStep(std::make_unique<std::atomic<uint32_t>>(false)),
      m_trail(static_cast<glm::vec3>(pos)), m_method(method)
{
    uploadModel(P_MODEL_NAME,
                std::make_unique<ParticleModel>(static_cast<glm::vec3>(m_pos)));
    uploadModel(F_VECTOR_MODEL_NAME,
                std::make_unique<VectorArrowModel>(
                    static_cast<glm::vec3>(m_pos),
                    static_cast<glm::vec3>(m_affectingForce), glm::vec3(0.0f)));
    setInitialState();
}

Particle::Particle(double charge, double mass, bool movable,
                   Types::Vec3d affectingForce, Types::Vec3d pos,
                   Types::Vec3d vel)
    : m_charge(charge), m_mass(mass), m_affectingForce(affectingForce),
      m_acceleration(affectingForce / mass), m_velocity(vel), m_pos(pos),
      m_movable(movable), m_id(nextId++),
      m_statesMutex(std::make_unique<std::mutex>()),
      m_maxStep(std::make_unique<std::atomic<uint32_t>>(false)),
      m_trail(static_cast<glm::vec3>(pos))
{
    uploadModel(P_MODEL_NAME,
                std::make_unique<ParticleModel>(static_cast<glm::vec3>(m_pos)));
    uploadModel(F_VECTOR_MODEL_NAME,
                std::make_unique<VectorArrowModel>(
                    static_cast<glm::vec3>(m_pos),
                    static_cast<glm::vec3>(m_affectingForce), glm::vec3(0.0f)));
    setInitialState();
}

void Particle::update(Types::Vec3d affectingForce, double time)
{
    if (!m_movable)
    {
        return;
    }
    m_affectingForce = affectingForce;
    m_acceleration = m_affectingForce / m_mass;
    m_velocity += m_acceleration;
    m_pos += m_velocity * time;

    m_models[P_MODEL_NAME]->update(static_cast<glm::vec3>(m_pos));
    m_models[F_VECTOR_MODEL_NAME]->update(
        static_cast<glm::vec3>(m_pos), static_cast<glm::vec3>(m_affectingForce),
        glm::normalize(static_cast<glm::vec3>(m_affectingForce)) * 2.5f);
}

void Particle::update()
{
    m_models[P_MODEL_NAME]->update(static_cast<glm::vec3>(m_pos));
    m_models[F_VECTOR_MODEL_NAME]->update(
        static_cast<glm::vec3>(m_pos), static_cast<glm::vec3>(m_affectingForce),
        glm::normalize(static_cast<glm::vec3>(m_affectingForce)) * 2.5f);
}

void Particle::update(double time)
{
    if (!m_movable)
    {
        return;
    }
    m_acceleration = m_affectingForce / m_mass;
    m_velocity += m_acceleration;
    m_pos += m_velocity * time;

    m_models[P_MODEL_NAME]->update(static_cast<glm::vec3>(m_pos));
    m_models[F_VECTOR_MODEL_NAME]->update(
        static_cast<glm::vec3>(m_pos), static_cast<glm::vec3>(m_affectingForce),
        glm::normalize(static_cast<glm::vec3>(m_affectingForce)) * 2.5f);
}

void Particle::pushState(uint32_t idx)
{
    m_states.emplace(
        idx, State{m_affectingForce, m_acceleration, m_velocity, m_pos});
    *m_maxStep = idx;
}

void Particle::pushState(uint32_t &idx, Types::Vec3d &force,
                         Types::Vec3d &acceleration, Types::Vec3d &velocity,
                         Types::Vec3d &pos)
{
    m_states.emplace(idx, State{force, acceleration, velocity, pos});
    *m_maxStep = idx;
}

void Particle::pushState(uint32_t &idx, Types::Vec3d &&force,
                         Types::Vec3d &&acceleration, Types::Vec3d &&velocity,
                         Types::Vec3d &&pos)
{
    m_states.emplace(idx, State{force, acceleration, velocity, pos});
    *m_maxStep = idx;
}

void Particle::setInitialState()
{
    m_initialState = std::make_unique<State>(m_affectingForce, m_acceleration,
                                             m_velocity, m_pos);
    m_trail.setInitialPos(static_cast<glm::vec3>(m_pos));
}

bool Particle::updateFromPrecalcPos(uint32_t idx)
{
    if (m_states.contains(idx))
    {
        m_affectingForce = m_states[idx].affectingForce;
        m_acceleration = m_states[idx].acceleration;
        m_velocity = m_states[idx].velocity;
        m_pos = m_states[idx].pos;

        if (idx > 0)
        {
            switch (m_method)
            {
            case Types::OdeMethod::RK4:
                m_trail.addSectionRed(static_cast<glm::vec3>(m_pos));
                break;
            case Types::OdeMethod::ForwardEuler:
                m_trail.addSectionYellow(static_cast<glm::vec3>(m_pos));
                break;
            case Types::OdeMethod::Leapfrog:
                m_trail.addSectionGreen(static_cast<glm::vec3>(m_pos));
                break;
            }
        }
        m_models[P_MODEL_NAME]->update(static_cast<glm::vec3>(m_pos));
        m_models[F_VECTOR_MODEL_NAME]->update(
            static_cast<glm::vec3>(m_pos),
            static_cast<glm::vec3>(m_affectingForce),
            glm::normalize(static_cast<glm::vec3>(m_affectingForce)) * 2.5f);
        return true;
    }
    return false;
}

void Particle::cleanup()
{
    Node::cleanup();
    m_states.clear();
    m_trail.cleanup();
    m_maxStep = 0;
}