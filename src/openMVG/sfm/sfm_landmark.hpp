// Copyright (c) 2015 Pierre Moulon.

// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef OPENMVG_SFM_LANDMARK_HPP
#define OPENMVG_SFM_LANDMARK_HPP

#include "openMVG/image/pixel_types.hpp"
#include "openMVG/numeric/numeric.h"
#include <cereal/cereal.hpp> // Serialization

namespace openMVG {
namespace sfm {

/// Define 3D-2D tracking data: 3D landmark with it's 2D observations
struct Observation
{
  Observation() {}
  Observation(const Vec2 & p, IndexT idFeat): x(p), id_feat(idFeat) {}

  Vec2 x;
  IndexT id_feat;

  // Serialization
  template <class Archive>
  void save( Archive & ar) const
  {
    ar(cereal::make_nvp("id_feat", id_feat ));
    const std::vector<double> pp = { x(0), x(1) };
    ar(cereal::make_nvp("x", pp));
  }

  // Serialization
  template <class Archive>
  void load( Archive & ar)
  {
    ar(cereal::make_nvp("id_feat", id_feat ));
    std::vector<double> p(2);
    ar(cereal::make_nvp("x", p));
    x << p[0], p[1];
  }
};
/// Observations are indexed by their View_id
typedef Hash_Map<IndexT, Observation> Observations;

/// Define a landmark (a 3D point, with it's 2d observations)
struct Landmark
{
  Observations obs;
  Vec3 X;
  openMVG::image::RGBColor rgb;    //!> the color associated to the point

  /// default constructor setting the color to white
  Landmark() : rgb(openMVG::image::WHITE) {}

  // Serialization
  template <class Archive>
  void save( Archive & ar) const
  {
    const std::vector<double> point = { X(0), X(1), X(2) };
    ar(cereal::make_nvp("X", point ));
    const std::vector<unsigned char> color = { rgb.r(), rgb.g(), rgb.b() };
    ar(cereal::make_nvp("rgb", color ));
    ar(cereal::make_nvp("observations", obs));
  }

  template <class Archive>
  void load( Archive & ar)
  {
    std::vector<double> point(3);
    std::vector<int> color(3);
    ar(cereal::make_nvp("X", point ));
    X << point[0], point[1], point[2];
    ar(cereal::make_nvp("rgb", color ));
    rgb = openMVG::image::RGBColor( color[0], color[1], color[2] );
    ar(cereal::make_nvp("observations", obs));
  }
};

} // namespace sfm
} // namespace openMVG

#endif // OPENMVG_SFM_LANDMARK_HPP
