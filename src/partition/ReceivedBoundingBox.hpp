#pragma once

#include "Partition.hpp"
#include "logging/Logger.hpp"
#include <vector>
#include "mesh/Vertex.hpp"
#include "mesh/Mesh.hpp"


// Forward delcration to friend the boost test struct

namespace precice {
namespace partition {
/**
 * @brief A partition that is computed from a mesh received from another participant.
 *
 * A mesh is received by the master rank and re-partitioned among all slave ranks.
 * Afterwards necessary distribution data structures are set up.
 */
class ReceivedBoundingBox : public Partition
{
public:

  /// Defines the typ of geometric filter used
  enum GeometricFilter {
    // @brief undefined
    UNDEFINED,
    // @brief No geometric filter used (e.g. for RBF mappings)
    NO_FILTER,
    // @brief Filter at master and communicate only filtered mesh.
    FILTER_FIRST,
    // @brief Broadcast first and filter then
    BROADCAST_FILTER
  };

   /// Constructor
  ReceivedBoundingBox (mesh::PtrMesh mesh, double safetyFactor, GeometricFilter geometricFilter);
  virtual ~ReceivedBoundingBox() {}

  /// bounding box set is received from other participant
  virtual void communicateBoundingBox();

  /* bounding boxes are compared and feedback sent to other
     master to construct initial communication map
  */
  virtual void computeBoundingBox();
  
  /// The mesh is received from another participant.
  virtual void communicate ();
  
  /// The mesh is compared and all distribution data structures are set up.
  virtual void compute (); 
    
private:

  logging::Logger _log{"partition::ReceivedBoundingBox"};
  
  bool compareBoundingBox(mesh::Mesh::BoundingBox currentBB, mesh::Mesh::BoundingBox receivedBB);

  // void filterMesh(mesh::Mesh& filteredMesh, const bool filterByBB);

  void prepareBoundingBox();

  // bool isVertexInBB(const mesh::Vertex& vertex);

  virtual void createOwnerInformation();

  // /// Helper function for 'createOwnerFunction' to set local owner information
  // void setOwnerInformation(const std::vector<int> &ownerVec);

  // number of other particpant ranks
  int _remoteParComSize = 0;
  
  mesh::Mesh::BoundingBoxMap _globalBB;
  
  mesh::Mesh::BoundingBox _bb;

    // list of connected ranks -> list of vertices
  std::map<int, std::vector<int>> _localCommunicationMap; 

  int _dimensions;

  double _safetyFactor;
  
  GeometricFilter _geometricFilter;  
  
};

}} // namespace precice, partition