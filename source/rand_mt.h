#pragma once

#include <random>
//------------------------------------------------------------------------------
struct RandMT
{
  static const size_t NUM_GENERATED = 1;

  std::random_device m_device;
  std::mt19937 m_engine;
  std::uniform_real_distribution<float> m_distr;

  RandMT()
      : m_engine(m_device())
      , m_distr(std::uniform_real_distribution<float>(0.0, 1.0f))
  {
  }

  float generate() { return m_distr(m_engine); }
};

//------------------------------------------------------------------------------
