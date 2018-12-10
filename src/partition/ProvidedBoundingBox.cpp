#include "partition/ProvidedBoundingBox.hpp"
#include "com/CommunicateBoundingBox.hpp"
#include "com/CommunicateMesh.hpp"
#include "com/Communication.hpp"
#include "utils/MasterSlave.hpp"
#include "m2n/M2N.hpp"
#include "utils/EventTimings.hpp"
#include "utils/Parallel.hpp"
#include "mesh/Mesh.hpp"
#include "mapping/Mapping.hpp"

using precice::utils::Event;

namespace precice {
namespace partition {

ProvidedBoundingBox::ProvidedBoundingBox
(mesh::PtrMesh mesh,
 bool hasToSend,
 double safetyFactor)
:
    Partition (mesh),
    _hasToSend(hasToSend),
    _dimensions(mesh->getDimensions()),
    _safetyFactor(safetyFactor)
{}

void ProvidedBoundingBox::communicateBoundingBox()
{
  if (_hasToSend) {
    
    // each rank sends its bb to master
    if (utils::MasterSlave::_slaveMode) {//slave            
      com::CommunicateBoundingBox(utils::MasterSlave::_communication).sendBoundingBox(_mesh->getBoundingBox(), 0); 
    }
    else
    { // Master

      assertion(utils::MasterSlave::_rank==0);
      assertion(utils::MasterSlave::_size>1);      

      // master receives bbs from other ranks and store them into globalBB
      mesh::Mesh::BoundingBoxMap globalBB;
      mesh::Mesh::BoundingBox initialBB;
      for (int i=0; i < _dimensions; i++) {        
        initialBB.push_back(std::make_pair(-1,-1));
      }
      for (int remoteRank = 0; remoteRank < utils::MasterSlave::_size; remoteRank++ )
      {
        globalBB[remoteRank]= initialBB;
      }

      // master stores its bb into gloabalBB
      globalBB[0] = _mesh->getBoundingBox();     

      // master receives bbs from slaves and store them in global bb
      if (utils::MasterSlave::_size>1) {  
        for (int rankSlave = 1; rankSlave < utils::MasterSlave::_size; rankSlave++) {
          com::CommunicateBoundingBox(utils::MasterSlave::_communication).receiveBoundingBox(globalBB[rankSlave], rankSlave);                    
        }
      }
      
      // master sends set of boundingboxes (globalBB) to the other master
      _m2n->getMasterCommunication()->send(utils::MasterSlave::_size , 0);      
      com::CommunicateBoundingBox(_m2n->getMasterCommunication()).sendBoundingBoxMap(globalBB,0);
    }                  
  }   
}

void ProvidedBoundingBox::computeBoundingBox()
{
  // size of the feedbackmap that is received here
  int receivedFeedbackMapSize = 0;  
  std::map<int, std::vector<int>> receivedFeedbackMap;
  
  if (not utils::MasterSlave::_slaveMode) {//Master
    assertion(utils::MasterSlave::_size>1);  
    
    // master receives feedback map (list of other participant ranks -> connected ranks at this participant)
    // from other participants master    
    _m2n->getMasterCommunication()->receive(receivedFeedbackMapSize, 0 );    
    if (receivedFeedbackMapSize != 0)
    {
      for (int i=0; i < receivedFeedbackMapSize; i++)
      {        
        receivedFeedbackMap[i]={-1};
      }
      
      com::CommunicateBoundingBox(_m2n->getMasterCommunication()).receiveFeedbackMap(receivedFeedbackMap, 0 );
    }

    // master broadcasts the reveived feedbackmap to slaves! 
    utils::MasterSlave::_communication->broadcast(receivedFeedbackMapSize);
    if (receivedFeedbackMapSize != 0)
    {
      com::CommunicateBoundingBox(utils::MasterSlave::_communication).broadcastSendFeedbackMap(receivedFeedbackMap);      
    }
    
    
    // master checks which ranks are connected to it
    for (auto &otherRank : receivedFeedbackMap) {
      for (auto &includedRank: otherRank.second) {
        if (utils::MasterSlave::_rank == includedRank) {          
          _mesh->getInitialConnectionMap().push_back(otherRank.first);
        }
      }
    }    
  }
  else
  { // Slave
    
    utils::MasterSlave::_communication->broadcast(receivedFeedbackMapSize, 0);
    if (receivedFeedbackMapSize != 0)
    {
      for (int i=0; i < receivedFeedbackMapSize; i++)
      {
        receivedFeedbackMap[i]={-1};
      }
      com::CommunicateBoundingBox(utils::MasterSlave::_communication).broadcastReceiveFeedbackMap(receivedFeedbackMap);      
    }
       
    for (auto &otherRank : receivedFeedbackMap)
    {
      for (auto &includedRanks: otherRank.second)
      {
        if (utils::MasterSlave::_rank == includedRanks)
        {
          _mesh->getInitialConnectionMap().push_back(otherRank.first);
        }
      }
    }
  }
}

// these functions will be implemented in package 3
void ProvidedBoundingBox::communicate()
{}

void ProvidedBoundingBox::compute()
{}

void ProvidedBoundingBox::createOwnerInformation()
{}


}}
