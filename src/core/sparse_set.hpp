#pragma once

#include <cstdint>
#include <vector>

template <typename T> class SparseSet
{
public:
    struct Handle
    {
        uint32_t index = 0u;
        uint32_t generation = 0u;
    };

    Handle insert(const T &value)
    {
        uint32_t sidx;

        if (!m_free.empty())
        {
            sidx = m_free.back();
            m_free.pop_back();
        }
        else
        {
            sidx = m_sparse.size();
            m_sparse.push_back({});
        }

        uint32_t didx = m_dense.size();
        m_dense.emplace_back(value, sidx);

        auto &s = m_sparse[sidx];
        s.denseIndex = didx;
        s.generation++;
        s.alive = true;

        return {sidx, s.generation};
    }

    void remove(Handle h)
    {
        if (!valid(h))
            return;

        auto &s = m_sparse[h.index];
        uint32_t dead = s.denseIndex;
        uint32_t last = m_dense.size() - 1;

        std::swap(m_dense[dead], m_dense[last]);

        m_sparse[m_dense[dead].sparseIndex].denseIndex = dead;

        m_dense.pop_back();

        s.alive = false;
        m_free.push_back(h.index);
    }

    T *get(Handle h)
    {
        if (!valid(h))
            return nullptr;
        return &m_dense[m_sparse[h.index].denseIndex].value;
    }

    struct DenseEntry
    {
        DenseEntry(const T &v, uint32_t si) : value(v), sparseIndex(si) {}

        T value;
        uint32_t sparseIndex = 0;
    };

    struct SparseEntry
    {
        uint32_t denseIndex = 0;
        uint32_t generation = 0;
        bool alive = false;
    };

    std::vector<DenseEntry> &dense() { return m_dense; }

private:
    bool valid(Handle h) const
    {
        return h.index < m_sparse.size() && m_sparse[h.index].alive && m_sparse[h.index].generation == h.generation;
    }

    std::vector<DenseEntry> m_dense;
    std::vector<SparseEntry> m_sparse;
    std::vector<uint32_t> m_free;
};