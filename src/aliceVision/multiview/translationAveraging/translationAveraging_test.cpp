// This file is part of the AliceVision project.
// Copyright (c) 2016 AliceVision contributors.
// Copyright (c) 2012 openMVG contributors.
// This Source Code Form is subject to the terms of the Mozilla Public License,
// v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "aliceVision/multiview/translationAveraging/common.hpp"
#include "aliceVision/multiview/translationAveraging/solver.hpp"
#include "aliceVision/multiview/translationAveraging/translationAveragingTest.hpp"

#include <fstream>
#include <map>
#include <utility>
#include <vector>

#define BOOST_TEST_MODULE translationAveraging

#include <boost/test/unit_test.hpp>
#include <boost/test/tools/floating_point_comparison.hpp>
#include <aliceVision/unitTest.hpp>

using namespace aliceVision;
using namespace aliceVision::translationAveraging;

BOOST_AUTO_TEST_CASE(translation_averaging_globalTi_from_tijs_Triplets_softL1_Ceres) {

  const int focal = 1000;
  const int principal_Point = 500;
  //-- Setup a circular camera rig or "cardioid".
  const int iNviews = 12;
  const int iNbPoints = 6;

  const bool bCardiod = true;
  const bool bRelative_Translation_PerTriplet = true;
  std::vector<aliceVision::translationAveraging::relativeInfo > vec_relative_estimates;

  const NViewDataSet d =
    Setup_RelativeTranslations_AndNviewDataset
    (
      vec_relative_estimates,
      focal, principal_Point, iNviews, iNbPoints,
      bCardiod, bRelative_Translation_PerTriplet
    );

  d.exportToPLY("global_translations_from_triplets_GT.ply");
  visibleCamPosToSVGSurface(d._C, "global_translations_from_triplets_GT.svg");

  // Solve the translation averaging problem:
  std::vector<Vec3> vec_translations;
  BOOST_CHECK(solve_translations_problem_softl1(
    vec_relative_estimates, bRelative_Translation_PerTriplet, iNviews, vec_translations));

  BOOST_CHECK_EQUAL(iNviews, vec_translations.size());

  // Check accuracy of the found translations
  for (unsigned i = 0; i < iNviews; ++i)
  {
    const Vec3 t = vec_translations[i];
    const Mat3 & Ri = d._R[i];
    const Vec3 C_computed = - Ri.transpose() * t;

    const Vec3 C_GT = d._C[i] - d._C[0];

    //-- Check that found camera position is equal to GT value
    if (i==0)  {
      EXPECT_MATRIX_NEAR(C_computed, C_GT, 1e-6);
    }
    else  {
     BOOST_CHECK_SMALL(DistanceLInfinity(C_computed.normalized(), C_GT.normalized()), 1e-6);
    }
  }
}

BOOST_AUTO_TEST_CASE(translation_averaging_globalTi_from_tijs_softl1_Ceres) {

  const int focal = 1000;
  const int principal_Point = 500;
  //-- Setup a circular camera rig or "cardiod".
  const int iNviews = 12;
  const int iNbPoints = 6;

  const bool bCardiod = true;
  const bool bRelative_Translation_PerTriplet = false;
  std::vector<aliceVision::translationAveraging::relativeInfo > vec_relative_estimates;

  const NViewDataSet d =
    Setup_RelativeTranslations_AndNviewDataset
    (
      vec_relative_estimates,
      focal, principal_Point, iNviews, iNbPoints,
      bCardiod, bRelative_Translation_PerTriplet
    );

  d.exportToPLY("global_translations_from_Tij_GT.ply");
  visibleCamPosToSVGSurface(d._C, "global_translations_from_Tij_GT.svg");

  // Solve the translation averaging problem:
  std::vector<Vec3> vec_translations;
  BOOST_CHECK(solve_translations_problem_softl1(
    vec_relative_estimates, bRelative_Translation_PerTriplet, iNviews, vec_translations));

  BOOST_CHECK_EQUAL(iNviews, vec_translations.size());

  // Check accuracy of the found translations
  for (unsigned i = 0; i < iNviews; ++i)
  {
    const Vec3 t = vec_translations[i];
    const Mat3 & Ri = d._R[i];
    const Vec3 C_computed = - Ri.transpose() * t;

    const Vec3 C_GT = d._C[i] - d._C[0];

    //-- Check that found camera position is equal to GT value
    if (i==0)  {
      EXPECT_MATRIX_NEAR(C_computed, C_GT, 1e-6);
    }
    else  {
     BOOST_CHECK_SMALL(DistanceLInfinity(C_computed.normalized(), C_GT.normalized()), 1e-6);
    }
  }
}

BOOST_AUTO_TEST_CASE(translation_averaging_globalTi_from_tijs_Triplets_l2_chordal) {
  makeRandomOperationsReproducible();

  const int focal = 1000;
  const int principal_Point = 500;
  //-- Setup a circular camera rig or "cardiod".
  const int iNviews = 12;
  const int iNbPoints = 6;

  const bool bCardiod = true;
  const bool bRelative_Translation_PerTriplet = true;
  std::vector<aliceVision::translationAveraging::relativeInfo > vec_relative_estimates;

  const NViewDataSet d =
    Setup_RelativeTranslations_AndNviewDataset
    (
      vec_relative_estimates,
      focal, principal_Point, iNviews, iNbPoints,
      bCardiod, bRelative_Translation_PerTriplet
    );

  d.exportToPLY("global_translations_from_Tij_GT.ply");
  visibleCamPosToSVGSurface(d._C, "global_translations_from_Tij_GT.svg");

  //-- Compute the global translations from the triplets of heading directions
  //-   with the L2 minimization of a Chordal distance
  std::vector<int> vec_edges;
  vec_edges.reserve(vec_relative_estimates.size() * 2);
  std::vector<double> vec_poses;
  vec_poses.reserve(vec_relative_estimates.size() * 3);
  std::vector<double> vec_weights;
  vec_weights.reserve(vec_relative_estimates.size());

  for(int i=0; i < vec_relative_estimates.size(); ++i)
  {
    const aliceVision::translationAveraging::relativeInfo & rel = vec_relative_estimates[i];
    vec_edges.push_back(rel.first.first);
    vec_edges.push_back(rel.first.second);

    const Vec3 EdgeDirection = -(d._R[rel.first.second].transpose() * rel.second.second.normalized());

    vec_poses.push_back(EdgeDirection(0));
    vec_poses.push_back(EdgeDirection(1));
    vec_poses.push_back(EdgeDirection(2));

    vec_weights.push_back(1.0);
  }

  const double function_tolerance=1e-7, parameter_tolerance=1e-8;
  const int max_iterations = 500;

  const double loss_width = 0.0;

  std::vector<double> X(iNviews*3);

  BOOST_CHECK(
    solve_translations_problem_l2_chordal(
      &vec_edges[0],
      &vec_poses[0],
      &vec_weights[0],
      vec_relative_estimates.size(),
      loss_width,
      &X[0],
      function_tolerance,
      parameter_tolerance,
      max_iterations));

  // Get back the camera translations in the global frame:
  for (size_t i = 0; i < iNviews; ++i)
  {
    if (i==0) {  //First camera supposed to be at Identity
      const Vec3 C0(X[0], X[1], X[2]);
      BOOST_CHECK_SMALL(DistanceLInfinity(C0, Vec3(0,0,0)), 1e-6);
    }
    else  {
      const Vec3 t_GT = (d._C[i] - d._C[0]);

      const Vec3 CI(X[i*3], X[i*3+1], X[i*3+2]);
      const Vec3 C0(X[0], X[1], X[2]);
      const Vec3 t_computed = CI - C0;

      //-- Check that vector are colinear
      BOOST_CHECK_SMALL(DistanceLInfinity(t_computed.normalized(), t_GT.normalized()), 1e-6);
    }
  }
}
