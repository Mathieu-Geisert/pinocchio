//
// Copyright (c) 2015-2017 CNRS
//
// This file is part of Pinocchio
// Pinocchio is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// Pinocchio is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Lesser Public License for more details. You should have
// received a copy of the GNU Lesser General Public License along with
// Pinocchio If not, see
// <http://www.gnu.org/licenses/>.

#ifndef __se3_algo_geometry_hxx__
#define __se3_algo_geometry_hxx__

#include <boost/foreach.hpp>

namespace se3 
{
  /* --- GEOMETRY PLACEMENTS -------------------------------------------------------- */
  /* --- GEOMETRY PLACEMENTS -------------------------------------------------------- */
  /* --- GEOMETRY PLACEMENTS -------------------------------------------------------- */
  inline void updateGeometryPlacements(const Model & model,
                                       Data & data,
                                       const GeometryModel & geomModel,
                                       GeometryData & geomData,
                                       const Eigen::VectorXd & q
                                       )
  {
    assert(model.check(data) && "data is not consistent with model.");
    
    forwardKinematics(model, data, q);
    updateGeometryPlacements(model, data, geomModel, geomData);
  }
  
  inline void  updateGeometryPlacements(const Model & model,
                                        const Data & data,
                                        const GeometryModel & geomModel,
                                        GeometryData & geomData
                                        )
  {
    assert(model.check(data) && "data is not consistent with model.");
    
    for (GeomIndex i=0; i < (GeomIndex) geomModel.ngeoms; ++i)
    {
      const Model::JointIndex & joint = geomModel.geometryObjects[i].parentJoint;
      if (joint>0) geomData.oMg[i] =  (data.oMi[joint] * geomModel.geometryObjects[i].placement);
      else         geomData.oMg[i] =  geomModel.geometryObjects[i].placement;
#ifdef WITH_HPP_FCL  
      geomData.collisionObjects[i].setTransform( toFclTransform3f(geomData.oMg[i]) );
#endif // WITH_HPP_FCL
    }
  }
#ifdef WITH_HPP_FCL  

  /* --- COLLISIONS ----------------------------------------------------------------- */
  /* --- COLLISIONS ----------------------------------------------------------------- */
  /* --- COLLISIONS ----------------------------------------------------------------- */

  inline bool computeCollision(const GeometryModel & geomModel,
                               GeometryData & geomData,
                               const PairIndex& pairId)
  {
    assert( pairId < geomModel.collisionPairs.size() );
    const CollisionPair & pair = geomModel.collisionPairs[pairId];

    assert( pairId      < geomData.collisionResults.size() );
    assert( pair.first  < geomData.collisionObjects.size() );
    assert( pair.second < geomData.collisionObjects.size() );

    fcl::CollisionResult& collisionResult = geomData.collisionResults[pairId];
    collisionResult.clear();
    fcl::collide (&geomData.collisionObjects[pair.first],
                  &geomData.collisionObjects[pair.second],
                  geomData.collisionRequest,
                  collisionResult);

    return collisionResult.isCollision();
  }
  
  inline bool computeCollisions(const GeometryModel & geomModel,
                                GeometryData & geomData,
                                const bool stopAtFirstCollision = true)
  {
    bool isColliding = false;
    
    for (std::size_t cpt = 0; cpt < geomModel.collisionPairs.size(); ++cpt)
    {
      if(geomData.activeCollisionPairs[cpt])
        {
          computeCollision(geomModel,geomData,cpt);
          isColliding |= geomData.collisionResults[cpt].isCollision();
          if(isColliding && stopAtFirstCollision)
            return true;
        }
    }
    
    return isColliding;
  }
  
  // WARNING, if stopAtFirstcollision = true, then the collisions vector will not be fulfilled.
  inline bool computeCollisions(const Model & model,
                                Data & data,
                                const GeometryModel & geomModel,
                                GeometryData & geomData,
                                const Eigen::VectorXd & q,
                                const bool stopAtFirstCollision
                                )
  {
    assert(model.check(data) && "data is not consistent with model.");
    
    updateGeometryPlacements (model, data, geomModel, geomData, q);
    
    return computeCollisions(geomModel,geomData, stopAtFirstCollision);
  }

  /* --- DISTANCES ----------------------------------------------------------------- */
  /* --- DISTANCES ----------------------------------------------------------------- */
  /* --- DISTANCES ----------------------------------------------------------------- */

  inline fcl::DistanceResult & computeDistance(const GeometryModel & geomModel,
                                               GeometryData & geomData,
                                               const PairIndex & pairId )
  {
    assert( pairId < geomModel.collisionPairs.size() );
    const CollisionPair & pair = geomModel.collisionPairs[pairId];

    assert( pairId      < geomData.distanceResults.size() );
    assert( pair.first  < geomData.collisionObjects.size() );
    assert( pair.second < geomData.collisionObjects.size() );

    geomData.distanceResults[pairId].clear();
    fcl::distance ( &geomData.collisionObjects[pair.first],
                    &geomData.collisionObjects[pair.second],
                    geomData.distanceRequest,
                    geomData.distanceResults[pairId]);

    return geomData.distanceResults[pairId];
  }
  

  template <bool COMPUTE_SHORTEST>
  inline std::size_t computeDistances(const GeometryModel & geomModel,
                                      GeometryData & geomData)
  {
    std::size_t min_index = geomModel.collisionPairs.size();
    double min_dist = std::numeric_limits<double>::infinity();
    for (std::size_t cpt = 0; cpt < geomModel.collisionPairs.size(); ++cpt)
    {
      if(geomData.activeCollisionPairs[cpt])
        {
          computeDistance(geomModel,geomData,cpt);
          if (COMPUTE_SHORTEST && geomData.distanceResults[cpt].min_distance < min_dist)
            {
              min_index = cpt;
              min_dist = geomData.distanceResults[cpt].min_distance;
            }
        }
    }
    return min_index;
  }
  
  // Required to have a default template argument on templated free function
  inline std::size_t computeDistances(const GeometryModel& geomModel,
                                      GeometryData & geomData)
  {
    return computeDistances<true>(geomModel,geomData);
  }
  
  // Required to have a default template argument on templated free function
  inline std::size_t computeDistances(const Model & model,
                               Data & data,
                               const GeometryModel & geomModel,
                               GeometryData & geomData,
                               const Eigen::VectorXd & q
                               )
  {
    return computeDistances<true>(model, data, geomModel, geomData, q);
  }

  template <bool ComputeShortest>
  inline std::size_t computeDistances(const Model & model,
                               Data & data,
                               const GeometryModel & geomModel,
                               GeometryData & geomData,
                               const Eigen::VectorXd & q
                               )
  {
    assert(model.check(data) && "data is not consistent with model.");
    updateGeometryPlacements (model, data, geomModel, geomData, q);
    return computeDistances<ComputeShortest>(geomModel,geomData);
  }

  /* --- RADIUS -------------------------------------------------------------------- */
  /* --- RADIUS -------------------------------------------------------------------- */
  /* --- RADIUS -------------------------------------------------------------------- */

  /// Given p1..3 being either "min" or "max", return one of the corners of the 
  /// AABB cub of the FCL object.
#define SE3_GEOM_AABB(FCL,p1,p2,p3)                                     \
  SE3::Vector3(                                                         \
    FCL->aabb_local.p1##_ [0],                                          \
    FCL->aabb_local.p2##_ [1],                                          \
    FCL->aabb_local.p3##_ [2])

  /// For all bodies of the model, compute the point of the geometry model
  /// that is the further from the center of the joint. This quantity is used 
  /// in some continuous collision test.
  inline void computeBodyRadius(const Model &         model,
                                const GeometryModel & geomModel,
                                GeometryData &        geomData)
  {
    geomData.radius.resize(model.joints.size(),0);
    BOOST_FOREACH(const GeometryObject & geom,geomModel.geometryObjects)
    {
      const boost::shared_ptr<const fcl::CollisionGeometry> & fcl
        = geom.fcl;
      const SE3 & jMb = geom.placement; // placement in joint.
      const Model::JointIndex & i = geom.parentJoint;
      assert (i<geomData.radius.size());

      double radius = geomData.radius[i] * geomData.radius[i];

      // The radius is simply the one of the 8 corners of the AABB cube, expressed 
      // in the joint frame, whose norm is the highest.
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,min,min,min)).squaredNorm(),radius);
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,min,min,max)).squaredNorm(),radius);
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,min,max,min)).squaredNorm(),radius);
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,min,max,max)).squaredNorm(),radius);
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,max,min,min)).squaredNorm(),radius);
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,max,min,max)).squaredNorm(),radius);
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,max,max,min)).squaredNorm(),radius);
      radius = std::max (jMb.act(SE3_GEOM_AABB(fcl,max,max,max)).squaredNorm(),radius);

      // Don't forget to sqroot the squared norm before storing it.
      geomData.radius[i] = sqrt(radius);
    }
  }

#undef SE3_GEOM_AABB
#endif // WITH_HPP_FCL

  /* --- APPEND GEOMETRY MODEL ----------------------------------------------------------- */

  inline void appendGeometryModel(GeometryModel & geomModel1,
                                  const GeometryModel & geomModel2)
  {
    assert (geomModel1.ngeoms == geomModel1.geometryObjects.size());
    Index nGeom1 = geomModel1.geometryObjects.size();
    Index nColPairs1 = geomModel1.collisionPairs.size();
    assert (geomModel2.ngeoms == geomModel2.geometryObjects.size());
    Index nGeom2 = geomModel2.geometryObjects.size();
    Index nColPairs2 = geomModel2.collisionPairs.size();

    /// Append the geometry objects and geometry positions
    geomModel1.geometryObjects.insert(geomModel1.geometryObjects.end(),
        geomModel2.geometryObjects.begin(), geomModel2.geometryObjects.end());
    geomModel1.ngeoms += nGeom2;

    /// 1. copy the collision pairs and update geomData1 accordingly.
    geomModel1.collisionPairs.reserve(nColPairs1 + nColPairs2 + nGeom1 * nGeom2);
    for (Index i = 0; i < nColPairs2; ++i)
    {
      const CollisionPair& cp = geomModel2.collisionPairs[i];
      geomModel1.collisionPairs.push_back(
          CollisionPair (cp.first + nGeom1, cp.second + nGeom1)
          );
    }

    /// 2. add the collision pairs between geomModel1 and geomModel2.
    for (Index i = 0; i < nGeom1; ++i) {
      for (Index j = 0; j < nGeom2; ++j) {
        geomModel1.collisionPairs.push_back(CollisionPair(i, nGeom1 + j));
      }
    }
  }

} // namespace se3

#endif // ifnded __se3_algo_geometry_hxx__
